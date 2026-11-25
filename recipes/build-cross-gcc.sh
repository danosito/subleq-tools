#!/usr/bin/env bash
# Linux only. Cross compiler, NOT a validated in-guest GCC.
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
w=${SOURCE_WORK:-$repo/work}
: "${SUBLEQ_SYSROOT:?Set to a publicly redistributable SUBLEQ Linux/uClibc sysroot you built from source}"
: "${SUBLEQ_AS:?Set to a target-capable assembler (LLVM IAS wrapper or GNU as)}"
: "${SUBLEQ_LD:?Set to a target-capable linker (LLD wrapper or GNU ld)}"
for file in "$w/gcc-16.2.0/configure" "$SUBLEQ_SYSROOT/include/stdio.h" "$SUBLEQ_AS" "$SUBLEQ_LD"; do test -e "$file" || { printf 'Missing: %s\n' "$file" >&2; exit 1; }; done
jobs=${JOBS:-2}; [[ $jobs =~ ^[12]$ ]] || { echo 'JOBS must be 1 or 2' >&2; exit 2; }
mkdir -p "$w/host-mpc" "$w/gcc-cross"
if [[ ! -f $w/host-mpc/install/lib/libmpc.a ]]; then
  test -d "$w/mpc-1.3.1" || tar -xf "$repo/downloads/mpc-1.3.1.tar.gz" -C "$w"
  (cd "$w/host-mpc"; "$w/mpc-1.3.1/configure" --prefix="$w/host-mpc/install" --disable-shared; make -j"$jobs"; make install)
fi
if [[ ! -f $w/gcc-cross/Makefile ]]; then
  (cd "$w/gcc-cross"; "$w/gcc-16.2.0/configure" --target=subleq-unknown-linux-uclibc --prefix="$w/install-cross" --with-sysroot="$SUBLEQ_SYSROOT" --with-native-system-header-dir=/include --with-as="$SUBLEQ_AS" --with-ld="$SUBLEQ_LD" --with-mpc="$w/host-mpc/install" --enable-languages=c --enable-checking=release --disable-bootstrap --disable-multilib --disable-nls --disable-threads --disable-shared --disable-libssp --disable-libquadmath --disable-libatomic --disable-libgomp --disable-libstdcxx --without-isl --disable-lto)
fi
make -C "$w/gcc-cross" -j"$jobs" all-gcc
printf 'Cross compiler: %s/gcc-cross/gcc/xgcc (guest compile/link/run NOT tested)\n' "$w"
