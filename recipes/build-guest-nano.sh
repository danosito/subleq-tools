#!/usr/bin/env bash
# Cross-build nano+ncurses. Requires the source-built SUBLEQ SDK and host tic.
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
: "${SDK_WORK:?Set to directory built by sdk/build-sdk-linux.sh}"
w=${SOURCE_WORK:-$repo/work}; bin=$SDK_WORK/llvm/bin; sysroot=$SDK_WORK/sysroot
[[ ${JOBS:-2} =~ ^[12]$ ]] || exit 2
for f in "$w/nano-9.3/configure" "$w/ncurses-6.6/configure" "$bin/clang" "$sysroot/lib/libc.a"; do test -e "$f" || { echo "Missing $f" >&2; exit 1; }; done
command -v tic >/dev/null
# Upstream config.sub does not yet recognize SUBLEQ; add only the CPU token.
python3 - "$w/nano-9.3/config.sub" "$w/ncurses-6.6/config.sub" <<'PY'
from pathlib import Path
import sys
for f in sys.argv[1:]:
    p=Path(f); s=p.read_text()
    if '| subleq \\' not in s:
        marker='| sh64 '
        if marker not in s: raise SystemExit(f'config.sub layout unknown: {p}')
        s=s.replace(marker, '| subleq \\\n\t\t\t| sh64 ', 1)
        p.write_text(s)
PY
build=$($w/ncurses-6.6/config.guess); target=subleq-unknown-linux-uclibc
export CC="$bin/clang --target=subleq-unknown-linux --sysroot=$sysroot" AR="$bin/llvm-ar" RANLIB="$bin/llvm-ar s" LD="$bin/ld.lld" NM="$bin/llvm-nm"
export CFLAGS='-O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2'
export LDFLAGS='-static -Wl,--threads=1,-z,noexecstack'
mkdir -p "$w/ncurses-build" "$w/ncurses-install" "$w/nano-build" "$w/nano-stage/usr/bin" "$w/nano-stage/usr/share/terminfo"
if [[ ! -f $w/ncurses-build/Makefile ]]; then
  (cd "$w/ncurses-build"; "$w/ncurses-6.6/configure" --build="$build" --host="$target" --prefix=/usr --with-build-cc="$(command -v gcc)" --enable-widec --with-normal --without-shared --without-debug --without-profile --without-cxx --without-cxx-binding --without-ada --without-progs --without-tests --without-manpages --without-gpm --without-dlsym --disable-db-install --with-default-terminfo-dir=/usr/share/terminfo)
fi
make -C "$w/ncurses-build" -j"${JOBS:-2}"
make -C "$w/ncurses-build" -j1 DESTDIR="$w/ncurses-install" install
for name in vt100 xterm xterm-256color; do : "$name"; done
tic -x -e vt100,xterm,xterm-256color -o "$w/nano-stage/usr/share/terminfo" "$w/ncurses-6.6/misc/terminfo.src"
u=$w/ncurses-install/usr
export NCURSESW_CFLAGS="-I$u/include -I$u/include/ncursesw -D_XOPEN_SOURCE_EXTENDED=1" NCURSESW_LIBS="-L$u/lib -lncursesw"
export LDFLAGS="$LDFLAGS -L$u/lib"
if [[ ! -f $w/nano-build/Makefile ]]; then
  (cd "$w/nano-build"; "$w/nano-9.3/configure" --build="$build" --host="$target" --prefix=/usr --sysconfdir=/etc --enable-utf8 --disable-nls --disable-libmagic --disable-speller --disable-formatter --disable-linter --disable-extra)
fi
make -C "$w/nano-build" -j"${JOBS:-2}"
cp "$w/nano-build/src/nano" "$w/nano-stage/usr/bin/nano"
"$bin/llvm-readelf" -Whld "$w/nano-stage/usr/bin/nano" | grep -q 'Machine:.*Subleq'
printf 'Guest nano build candidate at %s/nano-stage; not guest-tested\n' "$w"
