#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="/home/jiet/sift100M"
DST_DIR="/home/dlrkdals/datasets/sift100M_pim"
RESULT_DIR="$PROJECT_ROOT/SIFT100M16384M32_DIR"
TEMPLATE_CONFIG="$PROJECT_ROOT/sift100M-16384-32.json"
ACTIVE_CONFIG="$PROJECT_ROOT/config.json"
BACKUP_CONFIG="$PROJECT_ROOT/config.json.backup.before_sift100m"

mkdir -p "$DST_DIR"

for name in \
  "sift100M.16384.32.8.index" \
  "sift100M_query.fvecs" \
  "sift100M_groundtruth.ivecs"
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

# if [[ -f "$ACTIVE_CONFIG" ]]; then
#   cp "$ACTIVE_CONFIG" "$BACKUP_CONFIG"
# fi

cp "$TEMPLATE_CONFIG" "$ACTIVE_CONFIG"

echo "staged SIFT100M local dataset under $DST_DIR"
echo "config template: $TEMPLATE_CONFIG"
echo "active config: $ACTIVE_CONFIG"
echo "config backup: $BACKUP_CONFIG"
echo
echo "Notes:"
echo "  - This stages symlinks only. Original files under /home/jiet/sift100M are unchanged."
echo "  - query and groundtruth stay in raw fvecs/ivecs format and are readable by the current runtime."
echo "  - This only switches config.json. You still need dataset.h to match SIFT100M."
echo "  - Recommended dataset.h values: TEST_DPU, MY_PQ_M=32, DIM=128, QUERY_TYPE=2."
