#!/usr/bin/env bash
# Cross-build C-only GNU Binutils tools; requires sdk/build-sdk-linux.sh output.
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
: "${SDK_WORK:?Set to directory built by sdk/build-sdk-linux.sh}"
w=${SOURCE_WORK:-$repo/work}; src=$w/binutils-2.47
[[ ${JOBS:-2} =~ ^[12]$ ]] || exit 2
for f in "$src/configure" "$SDK_WORK/llvm/bin/clang" "$SDK_WORK/sysroot/lib/libc.a"; do test -e "$f" || { echo "Missing $f" >&2; exit 1; }; done
build=$($src/config.guess); target=subleq-unknown-linux-gnu
bin=$SDK_WORK/llvm/bin; sysroot=$SDK_WORK/sysroot
mkdir -p "$w/binutils-guest" "$w/binutils-guest-bin"
if [[ ! -f $w/binutils-guest/Makefile ]]; then
  (cd "$w/binutils-guest"; env ac_cv_tls=none CC="$bin/clang --target=subleq-unknown-linux --sysroot=$sysroot" CXX="$bin/clang++ --target=subleq-unknown-linux --sysroot=$sysroot" AR="$bin/llvm-ar" RANLIB="$bin/llvm-ar s" NM="$bin/llvm-nm" LD="$bin/ld.lld" CFLAGS="-Oz -I$sysroot/kernel-headers/include" LDFLAGS='-static -Wl,--gc-sections' "$src/configure" --build="$build" --host=subleq-unknown-linux-gnu --target="$target" --disable-werror --disable-nls --disable-gdb --disable-gdbserver --disable-sim --disable-gold --disable-gprof --disable-gprofng --disable-shared --enable-static --disable-plugins --without-zstd --without-debuginfod)
fi
make -C "$w/binutils-guest" -j"${JOBS:-2}" MAKEINFO=true all-binutils all-gas all-ld
for n in objdump readelf size strings; do cp "$w/binutils-guest/binutils/$n" "$w/binutils-guest-bin/$n"; done
cp "$w/binutils-guest/binutils/nm-new" "$w/binutils-guest-bin/nm"
cp "$w/binutils-guest/gas/as-new" "$w/binutils-guest-bin/as"
cp "$w/binutils-guest/ld/ld-new" "$w/binutils-guest-bin/ld"
for f in "$w/binutils-guest-bin/"*; do "$bin/llvm-readelf" -h "$f" | grep -q 'Machine:.*Subleq'; done
printf 'Guest binary build candidates at %s/binutils-guest-bin; not guest-tested\n' "$w"
