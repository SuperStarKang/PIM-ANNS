#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="/home/jiet/msmarco"
DST_DIR="/home/dlrkdals/datasets/msmarco138M_pim"
RESULT_DIR="$PROJECT_ROOT/MSMARCO138M65536M128_DIR"
TEMPLATE_CONFIG="$PROJECT_ROOT/msmarco138M-65536-128.json"

mkdir -p "$DST_DIR"

for name in \
  "msmarco138M.65536.128.8.index" \
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

echo "staged MSMARCO138M local dataset under $DST_DIR"
echo "config template: $TEMPLATE_CONFIG"
echo
echo "Notes:"
echo "  - This stages symlinks only. Original files under /home/jiet/msmarco are unchanged."
echo "  - This template is for smoke-testing msmarco138M.65536.128.8.index with the current DPU path."
echo "  - Keep common/dataset.h at DIM=1024, MY_PQ_M=128, LUT_TILE_M=32."
echo "  - For now, keep DIST_TYPE=int32_t if the goal is only to verify end-to-end execution."
