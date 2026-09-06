#!/bin/bash
# One-shot setup using uv for fast installs (3-5x faster than pip)
set -euo pipefail
START=$(date +%s)

SRC=/workspace/mfn_training
VENV=/workspace/venv
INSIGHT=/workspace/insightface
TRAIN_DIR=$INSIGHT/recognition/arcface_torch
DATA=/data/ms1m-retinaface-t1

echo "=== System deps ==="
apt-get update -qq && apt-get install -y -qq unzip git wget > /dev/null 2>&1

echo "=== Install uv ==="
pip install -q uv

echo "=== Create venv (uv) ==="
uv venv $VENV --python 3.10
source $VENV/bin/activate

echo "=== PyTorch CUDA 12.4 ==="
uv pip install -p $VENV/bin/python torch torchvision --index-url https://download.pytorch.org/whl/cu124

echo "=== Training deps ==="
uv pip install -p $VENV/bin/python "numpy<2" easydict mxnet==1.9.1 scikit-learn tensorboard

echo "=== Fix mxnet np.bool ==="
MXU=$VENV/lib/python3.10/site-packages/mxnet/numpy/utils.py
sed -i '1s/^/import numpy as _np; _np.bool = bool\n/' $MXU

echo "=== Clone insightface ==="
cd /workspace
rm -rf $INSIGHT
git clone --depth 1 https://github.com/deepinsight/insightface.git $INSIGHT

echo "=== Register mfn128 backbone ==="
cp $SRC/backbones.py $TRAIN_DIR/backbones/mfn128.py
cp $SRC/__init__.py $TRAIN_DIR/backbones/__init__.py
cp $SRC/config_mfn128.py $TRAIN_DIR/configs/config_mfn128.py
sed -i 's/label = torch.tensor(label, dtype=torch.long)/label = torch.tensor(int(label), dtype=torch.long)/' $TRAIN_DIR/dataset.py

uv pip install -p $VENV/bin/python onnx onnxruntime

python3 -c "
import sys; sys.path.insert(0,'$TRAIN_DIR')
from backbones import get_model
m = get_model('mfn128', fp16=False, num_features=128)
n = sum(p.numel() for p in m.parameters())
print(f'mfn128 OK: {n:,} params')
"

echo "=== Download datasets ==="
uv pip install -p $VENV/bin/python huggingface_hub
mkdir -p $DATA && cd $DATA
python3 -c "
from huggingface_hub import snapshot_download
snapshot_download('gaunernst/ms1mv3-recordio', local_dir='.', repo_type='dataset')
" && echo "Train data ready"

python3 -c "
from huggingface_hub import snapshot_download
snapshot_download('gaunernst/ms1mv3-recordio', local_dir='.',
    allow_patterns=['lfw.bin','cfp_fp.bin','agedb_30.bin'], repo_type='dataset')
print('Eval data ready')
" 2>/dev/null || true

ls -lh $DATA/train.rec $DATA/train.idx 2>/dev/null

ELAPSED=$(( $(date +%s) - START ))
echo "=== Setup done in ${ELAPSED}s ==="
echo "Start: nohup bash $SRC/nohup_train.sh > /workspace/train.log 2>&1 &"
