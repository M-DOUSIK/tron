#!/usr/bin/env bash
set -euo pipefail

cd "${REPO_ROOT:?set REPO_ROOT to the repository checkout}"

uv run python -u train_mfn_student_pair_finetune.py \
    --width 1.0 \
    --max-pairs 20 \
    --epochs 1 \
    --batch-size 16 \
    --out-dir official_mobilefacenet/student_pairft_smoke \
    --init-weights official_mobilefacenet/student_distill_w1_hardneg/mfn_w1_distill_128d.weights.h5 \
    --checkpoint-every 0 \
    --num-calib 10
