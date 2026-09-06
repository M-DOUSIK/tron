#!/bin/bash
# Cron job: checks if training is done, downloads artifacts, destroys instance.
# Run: crontab -e
# Add:  0 * * * * /bin/bash /path/to/auto_fetch.sh >> /path/to/auto_fetch.log 2>&1

# Configure via environment variables before running:
#   VAST_INSTANCE_ID / VAST_SSH_HOST / VAST_SSH_PORT  -- your vast.ai instance
#   REPO_ROOT                                         -- path to this repository checkout

set -euo pipefail

INSTANCE_ID="${VAST_INSTANCE_ID:?set VAST_INSTANCE_ID}"
SSH_HOST="${VAST_SSH_HOST:?set VAST_SSH_HOST}"
SSH_PORT="${VAST_SSH_PORT:?set VAST_SSH_PORT}"
REPO_ROOT="${REPO_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
LOCAL_OUTPUT="${LOCAL_OUTPUT:-$REPO_ROOT/training_output}"
LOCKFILE="/tmp/auto_fetch_mfn128.lock"

# Prevent overlapping runs (macOS compatible)
if [ -f "$LOCKFILE" ]; then
    LOCK_PID=$(cat "$LOCKFILE" 2>/dev/null)
    if kill -0 "$LOCK_PID" 2>/dev/null; then
        echo "$(date): Another fetch is running (PID $LOCK_PID), skipping."
        exit 0
    fi
fi
echo $$ > "$LOCKFILE"
trap "rm -f $LOCKFILE" EXIT

# ---------- Step 1: Check if training is done ----------
echo "$(date): Checking training status..."
DONE=$(unset https_proxy http_proxy HTTPS_PROXY HTTP_PROXY; ssh -o StrictHostKeyChecking=no -o ConnectTimeout=10 -p "$SSH_PORT" root@"$SSH_HOST" 'cat /workspace/DONE 2>/dev/null' 2>/dev/null || echo "")

if ! echo "$DONE" | grep -q "TRAINING_COMPLETE"; then
    # Also check if ONNX exists as fallback
    ONNX_EXISTS=$(unset https_proxy http_proxy HTTPS_PROXY HTTP_PROXY; ssh -o StrictHostKeyChecking=no -o ConnectTimeout=10 -p "$SSH_PORT" root@"$SSH_HOST" 'ls /workspace/output/mfn128_*/mfn128_float32.onnx 2>/dev/null' 2>/dev/null || echo "")
    if [ -z "$ONNX_EXISTS" ]; then
        echo "$(date): Training still in progress. Will check again next hour."
        exit 0
    fi
    echo "$(date): ONNX found but no DONE flag. Proceeding with download anyway."
fi

# ---------- Step 2: Create local output directory ----------
OUTDIR="${LOCAL_OUTPUT}/mfn128_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUTDIR"
echo "$(date): Training complete! Downloading to $OUTDIR"

unset https_proxy http_proxy HTTPS_PROXY HTTP_PROXY

# ---------- Step 3: Download model checkpoint ----------
echo "$(date): Downloading model checkpoint..."
scp -o StrictHostKeyChecking=no -P "$SSH_PORT" \
    root@"$SSH_HOST":/workspace/output/mfn128_*/model.pt \
    "$OUTDIR/" 2>&1
echo "$(date): model.pt downloaded."

# ---------- Step 4: Download ONNX ----------
echo "$(date): Downloading ONNX..."
scp -o StrictHostKeyChecking=no -P "$SSH_PORT" \
    root@"$SSH_HOST":/workspace/output/mfn128_*/mfn128_float32.onnx \
    "$OUTDIR/" 2>&1
echo "$(date): ONNX downloaded."

# ---------- Step 5: Download training logs ----------
echo "$(date): Downloading training logs..."
scp -o StrictHostKeyChecking=no -P "$SSH_PORT" \
    root@"$SSH_HOST":/workspace/train.log \
    "$OUTDIR/" 2>/dev/null || true
scp -o StrictHostKeyChecking=no -P "$SSH_PORT" \
    root@"$SSH_HOST":/workspace/output/mfn128_*/training.log \
    "$OUTDIR/" 2>/dev/null || true
scp -o StrictHostKeyChecking=no -P "$SSH_PORT" \
    root@"$SSH_HOST":/workspace/setup.log \
    "$OUTDIR/" 2>/dev/null || true
echo "$(date): Logs downloaded."

# ---------- Step 6: Verify all artifacts exist ----------
MISSING=0
for f in model.pt mfn128_float32.onnx train.log; do
    if [ ! -f "$OUTDIR/$f" ]; then
        echo "ERROR: $f is missing!"
        MISSING=1
    fi
done

if [ "$MISSING" -eq 1 ]; then
    echo "$(date): ABORT: Some artifacts failed to download. Instance NOT destroyed."
    echo "$(date): Check $OUTDIR and retry manually."
    exit 1
fi

# ---------- Step 7: Verify file integrity ----------
MODEL_SIZE=$(stat -f%z "$OUTDIR/model.pt" 2>/dev/null || stat -c%s "$OUTDIR/model.pt" 2>/dev/null || echo 0)
ONNX_SIZE=$(stat -f%z "$OUTDIR/mfn128_float32.onnx" 2>/dev/null || stat -c%s "$OUTDIR/mfn128_float32.onnx" 2>/dev/null || echo 0)
LOG_SIZE=$(stat -f%z "$OUTDIR/train.log" 2>/dev/null || stat -c%s "$OUTDIR/train.log" 2>/dev/null || echo 0)

if [ "$MODEL_SIZE" -lt 1000000 ]; then
    echo "ERROR: model.pt too small ($MODEL_SIZE bytes), likely corrupted."
    exit 1
fi
if [ "$ONNX_SIZE" -lt 100000 ]; then
    echo "ERROR: ONNX too small ($ONNX_SIZE bytes), likely corrupted."
    exit 1
fi

echo "$(date): All artifacts verified OK."
echo "  model.pt: $((MODEL_SIZE/1024/1024)) MB"
echo "  ONNX:     $((ONNX_SIZE/1024/1024)) MB"
echo "  train.log: $((LOG_SIZE/1024)) KB"

# ---------- Step 8: Destroy instance (only after confirmation) ----------
echo "$(date): Destroying instance $INSTANCE_ID..."
cd "$REPO_ROOT"
echo "y" | uv run vastai destroy instance "$INSTANCE_ID" 2>&1
echo "$(date): Instance destroyed. All done."
echo ""
echo "Artifacts saved to: $OUTDIR"
ls -lh "$OUTDIR/"
