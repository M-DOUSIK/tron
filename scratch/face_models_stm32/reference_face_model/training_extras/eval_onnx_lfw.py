#!/usr/bin/env python3
"""LFW eval using ONNX Runtime directly (bypasses onnx2tf)."""
import argparse, pickle
from io import BytesIO
from pathlib import Path
import numpy as np, onnxruntime as ort
from PIL import Image
import sklearn.preprocessing, sklearn.model_selection


def load_bin(path):
    with open(path, 'rb') as f:
        data = pickle.load(f, encoding='bytes')
    bins, issame = data[0], data[1]
    n_pairs = len(issame)
    imgs = np.zeros((2 * n_pairs, 3, 112, 112), dtype=np.float32)
    for i in range(2 * n_pairs):
        img = Image.open(BytesIO(bins[i])).convert('RGB')
        if img.size != (112, 112): img = img.resize((112, 112), Image.BILINEAR)
        arr = np.asarray(img, dtype=np.float32)
        arr = (arr / 127.5) - 1.0           # [-1, 1]
        imgs[i] = arr.transpose(2, 0, 1)     # HWC -> CHW (PyTorch ONNX format)
    return imgs, np.asarray(issame, bool)


def acc_kfold(emb, issame, k=10):
    emb = sklearn.preprocessing.normalize(emb)
    e1, e2 = emb[0::2], emb[1::2]
    d = np.sum((e1 - e2) ** 2, axis=1)
    th = np.arange(0, 4, 0.01)
    kf = sklearn.model_selection.KFold(k, shuffle=False)
    accs = []
    for tr, te in kf.split(d):
        best_a = 0; best_t = 0
        for t in th:
            a = np.mean((d[tr] < t) == issame[tr])
            if a > best_a: best_a, best_t = a, t
        accs.append(np.mean((d[te] < best_t) == issame[te]))
    return float(np.mean(accs)), float(np.std(accs))


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--onnx', required=True)
    p.add_argument('--bins', nargs='+', required=True)
    args = p.parse_args()
    sess = ort.InferenceSession(args.onnx, providers=['CPUExecutionProvider'])
    iname = sess.get_inputs()[0].name
    print(f'ONNX input: {sess.get_inputs()[0].shape}  output: {sess.get_outputs()[0].shape}')
    for bp in args.bins:
        print(f'\n=== {Path(bp).stem} ===')
        imgs, issame = load_bin(bp)
        print(f'pairs={len(issame)}')
        out_dim = sess.get_outputs()[0].shape[-1] if isinstance(sess.get_outputs()[0].shape[-1], int) else sess.run(None, {iname: imgs[0:1]})[0].shape[-1]
        e1 = np.zeros((imgs.shape[0], out_dim), dtype=np.float32)
        e2 = np.zeros_like(e1)
        imgs_f = imgs[:, :, :, ::-1].copy()  # mirror W axis (CHW)
        bs = 128
        for off in range(0, imgs.shape[0], bs):
            e1[off:off+bs] = sess.run(None, {iname: imgs[off:off+bs]})[0]
            e2[off:off+bs] = sess.run(None, {iname: imgs_f[off:off+bs]})[0]
        fused = e1 + e2
        acc, std = acc_kfold(fused, issame, 10)
        xn = float(np.linalg.norm(e1, axis=1).mean())
        print(f'  [{Path(bp).stem}] acc={acc*100:.2f}% (+/-{std*100:.2f})  xnorm={xn:.2f}')


if __name__ == '__main__':
    main()
