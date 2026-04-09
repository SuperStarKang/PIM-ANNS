#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

NPROBE="${1:-11}"

source /home/dlrkdals/miniconda3/etc/profile.d/conda.sh
conda activate pim-anns

export UPMEM_ROOT="/home/dlrkdals/.local/pim-upmem-2024.2"
export CMAKE_PREFIX_PATH="${CONDA_PREFIX}${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"
export PATH="${CONDA_PREFIX}/bin:${PATH}"
export LD_LIBRARY_PATH="${UPMEM_ROOT}/lib:${CONDA_PREFIX}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export LIBRARY_PATH="${UPMEM_ROOT}/lib:${CONDA_PREFIX}/lib${LIBRARY_PATH:+:${LIBRARY_PATH}}"
export PIM_ANNS_COROUTINE_STACK_SIZE="${PIM_ANNS_COROUTINE_STACK_SIZE:-4194304}"

mkdir -p MSMARCO10M4096M128_DIR/DPU_DIR \
         MSMARCO10M4096M128_DIR/CPU_DIR \
         MSMARCO10M4096M128_DIR/BATCH_DPU_DIR \
         log

cmake -S . -B build-dpu \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH" \
  -DUPMEM_ROOT="$UPMEM_ROOT"

cmake --build build-dpu --target main -j 1

LOG_PATH="/home/dlrkdals/PIM-ANNS/log/pim-anns-msmarco10m-nprobe${NPROBE}.stderr.log"
echo "stderr log: ${LOG_PATH}"
echo "coroutine stack: ${PIM_ANNS_COROUTINE_STACK_SIZE}"

env_args=(
  PATH="$PATH"
  LD_LIBRARY_PATH="$LD_LIBRARY_PATH"
  LIBRARY_PATH="$LIBRARY_PATH"
  CMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH"
  UPMEM_ROOT="$UPMEM_ROOT"
  CONDA_PREFIX="$CONDA_PREFIX"
  PIM_ANNS_COROUTINE_STACK_SIZE="$PIM_ANNS_COROUTINE_STACK_SIZE"
)

if [[ -n "${PIM_ANNS_DISABLE_REPORTER:-}" ]]; then
  env_args+=("PIM_ANNS_DISABLE_REPORTER=$PIM_ANNS_DISABLE_REPORTER")
fi

if [[ -n "${PIM_ANNS_DISABLE_LEVEL2_TIMER:-}" ]]; then
  env_args+=("PIM_ANNS_DISABLE_LEVEL2_TIMER=$PIM_ANNS_DISABLE_LEVEL2_TIMER")
fi

sudo env "${env_args[@]}" \
  ./build-dpu/main "$NPROBE" \
  2> >(tee "$LOG_PATH" >&2)
