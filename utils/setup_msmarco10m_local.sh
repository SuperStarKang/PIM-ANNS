#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="/home/jiet/msmarco"
DST_DIR="/home/dlrkdals/datasets/msmarco10M_pim"
RESULT_DIR="$PROJECT_ROOT/MSMARCO10M4096M128_DIR"
TEMPLATE_CONFIG="$PROJECT_ROOT/msmarco10M-4096-128.json"

mkdir -p "$DST_DIR"

for name in \
  "msmarco10M.4096.128.8.index" \
  "query_1024_fp32.fvecs" \
  "query_1024_fp32.bin" \
  "groundtruth.ivecs"
do
  src="$SRC_DIR/$name"
  dst="$DST_DIR/$name"
  if [[ ! -f "$src" ]]; then
    echo "missing source file: $src" >&2
    exit 1
  fi
  ln -sfn "$src" "$dst"
done

mkdir -p "$RESULT_DIR/CPU_DIR" "$RESULT_DIR/DPU_DIR" "$RESULT_DIR/BATCH_DPU_DIR"

echo "staged MSMARCO10M local dataset under $DST_DIR"
echo "config template: $TEMPLATE_CONFIG"
echo
echo "Notes:"
echo "  - This stages symlinks only. Original files under /home/jiet/msmarco are unchanged."
echo "  - query_1024_fp32.fvecs and groundtruth.ivecs are now readable by the current runtime."
echo "  - msmarco10M.4096.128.8.index is suitable for CPU sanity checks."
echo "  - DPU mode still requires a smaller-PQ index such as 4096/16/8 or 4096/32/8."
