#!/usr/bin/env bash
# Linux-hosted target binutils; does not create in-guest as/ld.
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
w=${SOURCE_WORK:-$repo/work}; jobs=${JOBS:-2}
[[ $jobs =~ ^[12]$ ]] || { echo 'JOBS must be 1 or 2' >&2; exit 2; }
test -x "$w/binutils-2.47/configure" || { echo 'Run prepare-sources.sh first' >&2; exit 1; }
mkdir -p "$w/binutils-host"
if [[ ! -f $w/binutils-host/Makefile ]]; then
  (cd "$w/binutils-host"; "$w/binutils-2.47/configure" --target=subleq-unknown-linux-gnu --prefix="$w/binutils-host-install" --disable-werror --disable-nls --disable-gdb --disable-gdbserver --disable-sim --disable-gold --disable-gprof --disable-gprofng --disable-shared --enable-static --disable-plugins --without-zstd --without-debuginfod)
fi
make -C "$w/binutils-host" -j"$jobs" all-binutils all-gas all-ld
printf 'Host tools: %s/binutils-host/gas/as-new and ld/ld-new. Guest execution NOT tested.\n' "$w"
