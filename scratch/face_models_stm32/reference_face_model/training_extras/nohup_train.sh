#!/bin/bash
# MFN128 Training + ONNX export. Safe for nohup.
set -euo pipefail

export TRAIN_OUTPUT="${TRAIN_OUTPUT:-/workspace/output/mfn128_$(date +%Y%m%d_%H%M%S)}"
export DATA_DIR="${DATA_DIR:-/data/ms1m-retinaface-t1}"
export NUM_GPUS="${NUM_GPUS:-4}"

mkdir -p "$TRAIN_OUTPUT"
LOGFILE="${TRAIN_OUTPUT}/full.log"
exec > >(tee -a "$LOGFILE") 2>&1

echo "========================================="
echo " MFN128 LeakyReLU Training"
echo " Start: $(date)"
echo " Output: $TRAIN_OUTPUT"
echo " GPUs: $NUM_GPUS"
echo "========================================="

source /workspace/venv/bin/activate
TRAIN_DIR=/workspace/insightface/recognition/arcface_torch
cd "$TRAIN_DIR"

# Pre-flight: fix numpy (huggingface_hub may have upgraded it)
source /workspace/venv/bin/activate
uv pip install -p /workspace/venv/bin/python "numpy<2" 2>/dev/null || true

echo ""
echo "=== Model ==="
python3 -c "
from backbones import get_model
m = get_model('mfn128', fp16=False, num_features=128)
n = sum(p.numel() for p in m.parameters())
print(f'LeakyReLU MobileFaceNet scale=1, 128D: {n:,} params ({n/1e6:.2f}M)')
"

echo ""
echo "=== GPUs ==="
nvidia-smi --query-gpu=name,memory.total --format=csv,noheader 2>/dev/null | head -"$NUM_GPUS"

echo ""
echo "=== Training ==="
START_TS=$(date +%s)

if [ "$NUM_GPUS" -gt 1 ]; then
    torchrun --nproc_per_node="$NUM_GPUS" train_v2.py configs/config_mfn128.py
else
    python3 train_v2.py configs/config_mfn128.py
fi

ELAPSED=$(( $(date +%s) - START_TS ))
echo "Training time: $((ELAPSED/3600))h $((ELAPSED%3600/60))m"

echo ""
echo "=== ONNX Export ==="
CKPT="$TRAIN_OUTPUT/model.pt"
ONNX="$TRAIN_OUTPUT/mfn128_lrelu_float32.onnx"

python3 <<PYEOF
import torch, os, sys
sys.path.insert(0, '$TRAIN_DIR')
from backbones import get_model

model = get_model('mfn128', fp16=False, num_features=128)
ckpt = torch.load('$CKPT', map_location='cpu')

if isinstance(ckpt, dict) and 'state_dict_backbone' in ckpt:
    sd = ckpt['state_dict_backbone']
elif isinstance(ckpt, dict):
    sd = {k: v for k, v in ckpt.items()}
else:
    sd = ckpt
sd = {k.replace('module.', ''): v for k, v in sd.items()}

missing, unexpected = model.load_state_dict(sd, strict=False)
print(f'Missing: {len(missing)}, Unexpected: {len(unexpected)}')
model.eval()

x = torch.randn(1, 3, 112, 112)
torch.onnx.export(model, x, '$ONNX',
    input_names=['input'], output_names=['embedding'],
    opset_version=11)
print(f'ONNX: $ONNX ({os.path.getsize("$ONNX")/1024/1024:.1f} MB)')
PYEOF

echo ""
echo "========================================="
echo " Complete: $(date)"
echo " Output: $TRAIN_OUTPUT"
ls -lh "$TRAIN_OUTPUT"/
echo "========================================="
echo "TRAINING_COMPLETE" > /workspace/DONE
