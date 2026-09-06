#!/bin/bash
# Monitors vast.ai training instance. When done, downloads artifacts and destroys instance.
# Run: nohup bash watch_training.sh > watch.log 2>&1 &

# Configure via environment variables before running:
#   VAST_INSTANCE_ID / VAST_SSH_HOST / VAST_SSH_PORT  -- your vast.ai instance
#   REPO_ROOT                                         -- path to this repository checkout

INSTANCE_ID="${VAST_INSTANCE_ID:?set VAST_INSTANCE_ID}"
SSH_HOST="${VAST_SSH_HOST:?set VAST_SSH_HOST}"
SSH_PORT="${VAST_SSH_PORT:?set VAST_SSH_PORT}"
REPO_ROOT="${REPO_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
LOCAL_OUTPUT="${LOCAL_OUTPUT:-$REPO_ROOT/training_output}"
CHECK_INTERVAL=900  # 15 minutes

mkdir -p "$LOCAL_OUTPUT"

while true; do
    # Check if DONE flag exists
    RESULT=$(unset https_proxy http_proxy HTTPS_PROXY HTTP_PROXY; ssh -o StrictHostKeyChecking=no -o ConnectTimeout=10 -p "$SSH_PORT" root@"$SSH_HOST" 'cat /workspace/DONE 2>/dev/null' 2>/dev/null)

    if echo "$RESULT" | grep -q "TRAINING_COMPLETE"; then
        echo "$(date): Training complete! Downloading artifacts..."

        # Download output
        unset https_proxy http_proxy HTTPS_PROXY HTTP_PROXY
        scp -o StrictHostKeyChecking=no -r -P "$SSH_PORT" \
            root@"$SSH_HOST":/workspace/output/mfn128_*/mfn128_float32.onnx \
            "$LOCAL_OUTPUT/" 2>/dev/null

        scp -o StrictHostKeyChecking=no -r -P "$SSH_PORT" \
            root@"$SSH_HOST":/workspace/output/mfn128_*/model.pt \
            "$LOCAL_OUTPUT/" 2>/dev/null

        scp -o StrictHostKeyChecking=no -P "$SSH_PORT" \
            root@"$SSH_HOST":/workspace/train.log \
            "$LOCAL_OUTPUT/" 2>/dev/null

        echo "$(date): Download complete. Files in $LOCAL_OUTPUT"
        ls -lh "$LOCAL_OUTPUT/"

        # Destroy instance
        echo "$(date): Destroying instance $INSTANCE_ID..."
        unset https_proxy http_proxy HTTPS_PROXY HTTP_PROXY
        cd "$REPO_ROOT"
        echo "y" | uv run vastai destroy instance "$INSTANCE_ID" 2>&1
        echo "$(date): Done."
        break
    fi

    echo "$(date): Still training... checking again in 15min"
    sleep "$CHECK_INTERVAL"
done
