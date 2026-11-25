#!/usr/bin/env bash
# Clean Linux SDK source build; requires ~35 GiB free. Never opens a guest or service image.
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
: "${SDK_SOURCE_DIR:?directory with pinned linux.tar.gz, llvm-project.tar.gz, uclibc-ng.tar.gz}"
: "${SDK_WORK:?absolute, empty working directory for SDK}"
[[ ${JOBS:-2} =~ ^[12]$ ]] || { echo 'JOBS must be 1 or 2' >&2; exit 2; }
# Upstream makefiles split some unquoted compiler/resource paths.
[[ $SDK_WORK == /* && ! $SDK_WORK =~ [[:space:]] ]] || { echo 'SDK_WORK must be an absolute path without whitespace' >&2; exit 2; }
export LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=0
unset MAKEFLAGS MFLAGS CMAKE_BUILD_PARALLEL_LEVEL
for tool in cmake ninja gcc g++ make flex bison patch sha256sum rsync perl python3; do command -v "$tool" >/dev/null; done
for name in linux llvm-project uclibc-ng; do
  want=$(awk -v n="$name.tar.gz" '$2==n {print $1}' "$repo/sdk/INPUTS.sha256")
  printf '%s  %s\n' "$want" "$SDK_SOURCE_DIR/$name.tar.gz" | sha256sum -c -
done
mkdir -- "$SDK_WORK"; work=$(realpath "$SDK_WORK"); mkdir "$work/src" "$work/sysroot"
for name in linux llvm-project uclibc-ng; do tar -xzf "$SDK_SOURCE_DIR/$name.tar.gz" -C "$work/src"; done
patch --batch --fuzz=0 -d "$work/src/llvm-project" -p1 < "$repo/sdk/llvm-subleq-ssp.patch"
patch --batch --fuzz=0 -d "$work/src/uclibc-ng" -p1 < "$repo/sdk/uclibc-subleq-ssp.patch"
(cd "$work"; sha256sum --quiet -c "$repo/sdk/patched-sources.sha256")
# Historical host LLD also compiled its Mach-O frontend. The trimmed LLVM
# snapshot lacks its one required libunwind header (not a target runtime).
header=libunwind/include/mach-o/compact_unwind_encoding.h
(cd "$repo/sdk"; sha256sum -c compact-unwind.sha256)
install -Dm644 "$repo/sdk/$header" "$work/src/llvm-project/$header"
cmake -G Ninja -S "$work/src/llvm-project/llvm" -B "$work/llvm" \
  -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Release \
  '-DLLVM_ENABLE_PROJECTS=clang;lld' '-DLLVM_TARGETS_TO_BUILD=' \
  -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=Subleq \
  -DLLVM_DEFAULT_TARGET_TRIPLE=subleq-unknown-linux \
  -DLLVM_FORCE_VC_REPOSITORY=ESI '-DLLVM_FORCE_VC_REVISION=' \
  -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF \
  -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_PARALLEL_LINK_JOBS=1
ninja -C "$work/llvm" -j"${JOBS:-2}" clang lld llvm-ar llvm-nm llvm-readelf llvm-strip llvm-objcopy llvm-objdump clang-resource-headers
bin=$work/llvm/bin
make -C "$work/src/linux" -j"${JOBS:-2}" ARCH=subleq INSTALL_HDR_PATH="$work/sysroot/kernel-headers" headers_install
conf=$work/src/uclibc-ng/.config
python3 - "$repo/sdk/uclibc.config" "$conf" "$work/sysroot/kernel-headers/include" <<'PY'
from pathlib import Path
import sys
text=Path(sys.argv[1]).read_text().replace('/REPLACED_BY_BUILD_RECIPE/kernel-headers/include', sys.argv[3])
Path(sys.argv[2]).write_text(text)
PY
export PATH="$bin:$PATH"
for goal in olddefconfig all install_dev; do
  make -C "$work/src/uclibc-ng" -j"${JOBS:-2}" ARCH=subleq CC="$bin/clang" LD="$bin/ld.lld" AR="$bin/llvm-ar" NM="$bin/llvm-nm" STRIPTOOL="$bin/llvm-strip" PREFIX="$work/sysroot" DEVEL_PREFIX=/ RUNTIME_PREFIX=/ "$goal"
done
"$bin/clang" --target=subleq-unknown-linux --sysroot="$work/sysroot" -isystem "$work/sysroot/kernel-headers/include" -static -fuse-ld=lld -Wl,--threads=1 "$repo/sdk/smoke.c" -o "$work/smoke.elf"
"$bin/llvm-readelf" -Whld "$work/smoke.elf"
printf 'SDK built at %s; ELF inspected but not executed\n' "$work"
