# cmake -B build .  -DCMAKE_INSTALL_PREFIX=/home/wupuqing/workspace/PIM-ANNS/third-party/upmem-2024.2.0-Linux-x86_64/

# fix path
SCRIPT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UPMEM_PREFIX="$(cd -- "$SCRIPT_DIR/../.." && pwd)"
cmake -B build . -DCMAKE_INSTALL_PREFIX="$UPMEM_PREFIX"

make -C build -j

make -C build install 