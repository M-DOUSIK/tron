#!/usr/bin/env python3
"""LFW/CFP_FP eval for an INT8 TFLite face embedding model.

Usage:
    uv run python training/eval_tflite_lfw.py \\
        --tflite training/output/qat_distilled/model_distilled_qat.int8.tflite \\
        --bins training/eval_bins/lfw.bin training/eval_bins/cfp_fp.bin
"""
import argparse
import pickle
from io import BytesIO
from pathlib import Path

import numpy as np
import sklearn.model_selection
import sklearn.preprocessing
import tensorflow as tf
from PIL import Image


def load_bin(path):
    """Load insightface .bin: pickle of (jpeg_bytes_list_of_2N, [issame_N])."""
    with open(path, 'rb') as f:
        data = pickle.load(f, encoding='bytes')
    bins, issame = data[0], data[1]
    # bins: 2N entries, pairs (i, i+1) form an image pair
    n_pairs = len(issame)
    imgs = np.zeros((2 * n_pairs, 112, 112, 3), dtype=np.float32)
    for i in range(2 * n_pairs):
        img = Image.open(BytesIO(bins[i])).convert('RGB')
        if img.size != (112, 112):
            img = img.resize((112, 112), Image.BILINEAR)
        # normalize to [-1, 1]
        arr = np.asarray(img, dtype=np.float32)
        imgs[i] = (arr / 127.5) - 1.0
    return imgs, np.asarray(issame, dtype=bool)


def run_tflite(interpreter, imgs):
    """Run inference, return [N, D] embeddings (FP32, post-dequant)."""
    in_det = interpreter.get_input_details()[0]
    out_det = interpreter.get_output_details()[0]
    in_scale, in_zp = in_det['quantization']
    out_scale, out_zp = out_det['quantization']
    in_dtype = in_det['dtype']
    out_dtype = out_det['dtype']

    n = imgs.shape[0]
    # Determine output dim from a dry run
    sample = imgs[0:1]
    if in_dtype == np.int8 or in_dtype == np.uint8:
        q = np.round(sample / in_scale + in_zp).astype(in_dtype)
    else:
        q = sample.astype(in_dtype)
    interpreter.set_tensor(in_det['index'], q)
    interpreter.invoke()
    out0 = interpreter.get_tensor(out_det['index'])
    out_dim = out0.shape[-1]
    embeddings = np.zeros((n, out_dim), dtype=np.float32)

    for i in range(n):
        sample = imgs[i:i+1]
        if in_dtype == np.int8 or in_dtype == np.uint8:
            q = np.round(sample / in_scale + in_zp).astype(in_dtype)
        else:
            q = sample.astype(in_dtype)
        interpreter.set_tensor(in_det['index'], q)
        interpreter.invoke()
        out = interpreter.get_tensor(out_det['index']).astype(np.float32)
        if out_dtype == np.int8 or out_dtype == np.uint8:
            out = (out - out_zp) * out_scale
        embeddings[i] = out[0]
        if (i + 1) % 1000 == 0:
            print(f'  inferred {i+1}/{n}', flush=True)
    return embeddings


def lfw_accuracy(embeddings, issame, nfolds=10):
    """KFold-CV accuracy at best threshold per fold, mirrors insightface."""
    embeddings = sklearn.preprocessing.normalize(embeddings)
    e1 = embeddings[0::2]; e2 = embeddings[1::2]
    # Squared Euclidean distance: ||e1 - e2||^2 = 2 - 2 cos
    diff = e1 - e2
    dist = np.sum(diff * diff, axis=1)
    issame = np.asarray(issame)

    thresholds = np.arange(0, 4, 0.01)
    accs = []
    kf = sklearn.model_selection.KFold(n_splits=nfolds, shuffle=False)
    for train_idx, test_idx in kf.split(dist):
        # best threshold on train
        best_acc = 0; best_thr = 0
        for thr in thresholds:
            pred = dist[train_idx] < thr
            acc = np.mean(pred == issame[train_idx])
            if acc > best_acc:
                best_acc = acc; best_thr = thr
        # eval on test
        pred = dist[test_idx] < best_thr
        accs.append(np.mean(pred == issame[test_idx]))
    return float(np.mean(accs)), float(np.std(accs))


def eval_one(interpreter, bin_path):
    name = Path(bin_path).stem
    print(f'\n=== {name} ({bin_path}) ===', flush=True)
    print(f'Loading bin...', flush=True)
    imgs, issame = load_bin(bin_path)
    print(f'  pairs={len(issame)}  imgs={imgs.shape[0]}', flush=True)

    # No-flip pass
    print('No-flip pass...', flush=True)
    e1 = run_tflite(interpreter, imgs)
    # Flip pass (mirror horizontal)
    print('Flip pass...', flush=True)
    imgs_f = imgs[:, :, ::-1, :].copy()
    e2 = run_tflite(interpreter, imgs_f)
    fused = e1 + e2

    acc, std = lfw_accuracy(fused, issame, nfolds=10)
    xn = float(np.linalg.norm(e1, axis=1).mean())
    print(f'  [{name}] acc={acc*100:.2f}% (+/-{std*100:.2f})  xnorm={xn:.2f}', flush=True)
    return acc, std


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--tflite', required=True)
    p.add_argument('--bins', nargs='+', required=True)
    args = p.parse_args()

    print(f'Loading {args.tflite}', flush=True)
    interpreter = tf.lite.Interpreter(model_path=args.tflite)
    interpreter.allocate_tensors()
    in_det = interpreter.get_input_details()[0]
    out_det = interpreter.get_output_details()[0]
    print(f'  input  {in_det["shape"]} {in_det["dtype"]} scale={in_det["quantization"]}')
    print(f'  output {out_det["shape"]} {out_det["dtype"]} scale={out_det["quantization"]}')

    for b in args.bins:
        eval_one(interpreter, b)


if __name__ == '__main__':
    main()
