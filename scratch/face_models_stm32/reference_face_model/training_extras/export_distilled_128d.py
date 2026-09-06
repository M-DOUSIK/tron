#!/usr/bin/env python3
"""Project distilled 512D ONNX to 128D via PCA, re-export ONNX, quantize, Vela compile.

Pipeline:
1. Run FP32 ONNX on calibration images to collect 512D embeddings.
2. PCA -> 128D projection matrix (512,128) + mean (512,).
3. Bake projection into a PyTorch wrapper model, export ONNX.
4. onnx2tf -> INT8 TFLite -> Vela.

Usage:
    uv run python training/export_distilled_128d.py
"""
import argparse
import os
import shutil
import subprocess
from pathlib import Path

import numpy as np
import onnxruntime as ort
import tensorflow as tf
import torch
import torch.nn as nn
from PIL import Image

SCRIPT_DIR = Path(__file__).resolve().parent
PROJ_ROOT = SCRIPT_DIR.parent
CALIB_DIR = PROJ_ROOT / "calibration_data" / "qat_112"


def collect_embeddings(onnx_path: Path, n: int) -> np.ndarray:
    sess = ort.InferenceSession(str(onnx_path), providers=['CPUExecutionProvider'])
    iname = sess.get_inputs()[0].name
    paths = sorted(CALIB_DIR.glob("*.jpg"))[:n]
    print(f"Collecting embeddings from {len(paths)} calib images...")
    embs = np.zeros((len(paths), 512), dtype=np.float32)
    for i, p in enumerate(paths):
        img = Image.open(p).convert('RGB')
        if img.size != (112, 112):
            img = img.resize((112, 112), Image.BILINEAR)
        arr = (np.asarray(img, dtype=np.float32) / 127.5) - 1.0
        x = arr.transpose(2, 0, 1)[None, ...]  # NCHW
        embs[i] = sess.run(None, {iname: x})[0][0]
        if (i + 1) % 500 == 0:
            print(f"  {i+1}/{len(paths)}")
    return embs


def pca_projection(embs: np.ndarray, out_dim: int):
    mean = embs.mean(axis=0)
    centered = embs - mean
    # SVD on centered data
    U, S, Vt = np.linalg.svd(centered, full_matrices=False)
    proj = Vt[:out_dim].T.astype(np.float32)  # (512, out_dim)
    var_kept = (S[:out_dim] ** 2).sum() / (S ** 2).sum()
    print(f"PCA: kept variance = {var_kept*100:.2f}%  proj shape={proj.shape}")
    return mean.astype(np.float32), proj


class Projected128D(nn.Module):
    """Wraps a PyTorch backbone with a linear 512->128 projection (bias = -mean @ W)."""
    def __init__(self, backbone, mean512, proj_512_to_128):
        super().__init__()
        self.backbone = backbone
        proj = nn.Linear(512, 128, bias=True)
        with torch.no_grad():
            proj.weight.copy_(torch.from_numpy(proj_512_to_128.T))  # (128, 512)
            proj.bias.copy_(-torch.from_numpy(mean512) @ torch.from_numpy(proj_512_to_128))
        self.proj = proj

    def forward(self, x):
        return self.proj(self.backbone(x))


def build_pytorch_512d_backbone(qat_pt: Path):
    import sys
    sys.path.insert(0, str(SCRIPT_DIR))
    from qat_finetune import MobileFaceNet
    m = MobileFaceNet(num_features=512, blocks=(1, 4, 6, 2), scale=1)
    sd_qat = torch.load(qat_pt, map_location='cpu', weights_only=False)
    clean_keys = set(m.state_dict().keys())
    sd_clean = {k.replace('.block.', '.'): v for k, v in sd_qat.items()
                if k.replace('.block.', '.') in clean_keys}
    m.load_state_dict(sd_clean, strict=True)
    return m.eval()


def export_onnx(model, out_path: Path):
    dummy = torch.randn(1, 3, 112, 112)
    # Static batch=1, no dynamic_axes: avoids PyTorch emitting Shape+Gather+Reshape
    # for view(x.size(0), -1) — firmware TFLite Micro lacks Shape op kernel.
    torch.onnx.export(model, dummy, str(out_path),
        input_names=['input'], output_names=['embedding'],
        opset_version=11, dynamo=False)
    print(f"Wrote {out_path} ({out_path.stat().st_size/1024/1024:.2f} MB)")


def onnx_to_int8_tflite(onnx_path: Path, out_dir: Path, calib_imgs: np.ndarray) -> Path:
    import onnx2tf
    tf_dir = out_dir / "saved_model"
    if tf_dir.exists():
        shutil.rmtree(tf_dir)
    onnx2tf.convert(input_onnx_file_path=str(onnx_path),
                    output_folder_path=str(tf_dir),
                    non_verbose=True,
                    copy_onnx_input_output_names_to_tflite=True)
    def rep():
        for img in calib_imgs:
            yield [img[np.newaxis, ...].astype(np.float32)]
    conv = tf.lite.TFLiteConverter.from_saved_model(str(tf_dir))
    conv.optimizations = [tf.lite.Optimize.DEFAULT]
    conv.representative_dataset = rep
    conv.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    conv.inference_input_type = tf.int8
    conv.inference_output_type = tf.int8
    int8 = out_dir / "model_distilled_qat_128d.int8.tflite"
    int8.write_bytes(conv.convert())
    print(f"INT8 TFLite: {int8} ({int8.stat().st_size/1024:.1f} KiB)")
    return int8


def run_vela(int8: Path, out_dir: Path):
    vela = shutil.which("vela")
    res = subprocess.run([vela, str(int8),
        "--accelerator-config", "ethos-u55-64",
        "--optimise", "Performance",
        "--output-dir", str(out_dir)], capture_output=True, text=True)
    print(res.stdout[-2000:])
    if res.returncode:
        print(res.stderr); raise RuntimeError("vela failed")


def load_calib_for_quant(n=500):
    paths = sorted(CALIB_DIR.glob("*.jpg"))[:n]
    out = []
    for p in paths:
        img = Image.open(p).convert('RGB')
        if img.size != (112, 112): img = img.resize((112, 112), Image.BILINEAR)
        out.append((np.asarray(img, dtype=np.float32) / 127.5) - 1.0)
    return np.asarray(out, dtype=np.float32)


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--qat-pt', type=Path,
        default=PROJ_ROOT / "training/output/qat_distilled/model_qat.pt")
    p.add_argument('--fp32-onnx', type=Path,
        default=PROJ_ROOT / "training/output/qat_distilled/model_qat_clean_fixed.onnx")
    p.add_argument('--out', type=Path,
        default=PROJ_ROOT / "training/output/qat_distilled_128d")
    p.add_argument('--num-pca', type=int, default=2000)
    p.add_argument('--num-calib', type=int, default=500)
    args = p.parse_args()

    args.out.mkdir(parents=True, exist_ok=True)

    # 1. PCA from FP32 ONNX embeddings
    embs = collect_embeddings(args.fp32_onnx, args.num_pca)
    mean, proj = pca_projection(embs, 128)
    np.savez(args.out / "pca_128.npz", mean=mean, projection=proj)

    # 2. Build Projected128D model, export ONNX
    backbone = build_pytorch_512d_backbone(args.qat_pt)
    model = Projected128D(backbone, mean, proj).eval()
    # smoke
    with torch.no_grad():
        e = model(torch.randn(2, 3, 112, 112))
    print(f"Smoke output: {tuple(e.shape)}  xnorm={float(torch.norm(e, dim=1).mean()):.2f}")
    onnx_path = args.out / "model_distilled_qat_128d.onnx"
    export_onnx(model, onnx_path)

    # 3. INT8 + Vela
    calib = load_calib_for_quant(args.num_calib)
    int8 = onnx_to_int8_tflite(onnx_path, args.out, calib)
    run_vela(int8, args.out)


if __name__ == "__main__":
    main()
