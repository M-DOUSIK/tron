#!/bin/bash
# QAT fine-tuning runner for vast.ai instances.
# Prerequisite: setup_vast.sh must have been run first (installs PyTorch, insightface, datasets).
#
# Usage:
#   bash run_qat_vast.sh [/path/to/model.pt] [output_dir]
#
# Defaults:
#   MODEL=/workspace/mfn_training/model.pt (copied from local)
#   OUTPUT=/workspace/output/qat_finetuned

set -euo pipefail

MODEL="${1:-/workspace/mfn_training/model_512d_lrelu.pt}"
OUTPUT="${2:-/workspace/output/qat_finetuned}"
EPOCHS="${EPOCHS:-2}"
LR="${LR:-0.001}"
BATCH_SIZE="${BATCH_SIZE:-128}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$(dirname "$SCRIPT_DIR")/training"

echo "========================================="
echo " QAT Fine-tuning for 512D LeakyReLU MFN"
echo " Start: $(date)"
echo " Model:  $MODEL"
echo " Output: $OUTPUT"
echo " Epochs: $EPOCHS  LR: $LR  Batch: $BATCH_SIZE"
echo "========================================="

# Activate venv (created by setup_vast.sh)
source /workspace/venv/bin/activate

# Ensure numpy version is correct (huggingface_hub may upgrade it)
uv pip install -p /workspace/venv/bin/python "numpy<2" 2>/dev/null || true

# Verify model exists
if [ ! -f "$MODEL" ]; then
    echo "ERROR: Model not found at $MODEL"
    exit 1
fi
echo "Model size: $(du -h "$MODEL" | cut -f1)"

# Verify data exists
DATA_DIR="${DATA_DIR:-/data/ms1m-retinaface-t1}"
if [ ! -f "$DATA_DIR/train.rec" ]; then
    echo "ERROR: Training data not found at $DATA_DIR/train.rec"
    echo "Run setup_vast.sh first to download datasets."
    exit 1
fi

# Check GPU
echo ""
echo "=== GPU Info ==="
nvidia-smi --query-gpu=name,memory.total,memory.free --format=csv,noheader 2>/dev/null || echo "No GPU found"

# Copy QAT script to workspace
cp "$SRC_DIR/qat_finetune.py" /workspace/qat_finetune.py

# Run QAT fine-tuning
echo ""
echo "=== Running QAT fine-tuning ==="
START_TS=$(date +%s)

python3 /workspace/qat_finetune.py \
    --model "$MODEL" \
    --output "$OUTPUT" \
    --data "$DATA_DIR" \
    --epochs "$EPOCHS" \
    --lr "$LR" \
    --batch-size "$BATCH_SIZE" \
    --num-workers 4

ELAPSED=$(( $(date +%s) - START_TS ))
echo ""
echo "QAT fine-tuning time: $((ELAPSED/60))m $((ELAPSED%60))s"

# Show results
echo ""
echo "=== Output ==="
ls -lh "$OUTPUT"/

echo ""
echo "=== QAT Complete: $(date) ==="
echo "Output: $OUTPUT"
echo "QAT_COMPLETE" > /workspace/QAT_DONE
