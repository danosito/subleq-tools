#!/usr/bin/env bash
# Linux: verified upstream sources + unmodified, separately tracked SUBLEQ patches.
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
cache=${SOURCE_CACHE:-$repo/downloads}
out=${SOURCE_WORK:-$repo/work}
mkdir -p "$cache" "$out"
while read -r hash name url; do
  [[ $hash == \#* || -z ${hash:-} ]] && continue
  if [[ ! -f $cache/$name ]]; then curl -fL --retry 3 "$url" -o "$cache/$name.part" && mv "$cache/$name.part" "$cache/$name"; fi
  printf '%s  %s\n' "$hash" "$cache/$name" | sha256sum -c -
done < "$repo/SOURCES.tsv"
extract() { local name=$1 dir=$2; if [[ ! -d $out/$dir ]]; then tar -xf "$cache/$name" -C "$out"; fi; }
extract binutils-2.47.tar.xz binutils-2.47
extract gcc-16.2.0.tar.xz gcc-16.2.0
extract gdb-18.1.tar.xz gdb-18.1
extract nano-9.3.tar.xz nano-9.3
extract ncurses-6.6.tar.gz ncurses-6.6
# The Vim archive was produced from the exact upstream commit recorded in SOURCES.tsv.
if [[ ! -d $out/vim ]]; then mkdir -p "$out/vim"; tar -xf "$cache/vim-9.2.1398.tar.gz" --strip-components=1 -C "$out/vim"; fi
apply() { local dir=$1 file=$2; patch --batch --fuzz=0 -d "$out/$dir" -p1 < "$repo/$file"; }
if [[ ! -e $out/.subleq-prepared ]]; then
  apply binutils-2.47 patches/binutils/0001-inspection.patch
  apply binutils-2.47 patches/binutils/0002-as-ld.patch
  apply gcc-16.2.0 patches/gcc/0001-config.patch
  mkdir -p "$out/gcc-16.2.0/gcc/config/subleq"
  cp "$repo"/ports/gcc/subleq.{cc,h,md} "$repo/ports/gcc/subleq-protos.h" "$out/gcc-16.2.0/gcc/config/subleq/"
  # GDB release already bundles BFD/opcodes; the inspection diff's binutils/readelf.c
  # hunk does not belong to GDB's distribution. Everything else is required.
  python3 - "$repo/patches/binutils/0001-inspection.patch" "$out/gdb-inspection.patch" <<'PY'
import pathlib,sys
parts=pathlib.Path(sys.argv[1]).read_text().split('diff --git ')
pathlib.Path(sys.argv[2]).write_text(parts[0]+''.join('diff --git '+p for p in parts[1:] if not p.startswith('a/binutils/')))
PY
  patch --batch --fuzz=0 -d "$out/gdb-18.1" -p1 < "$out/gdb-inspection.patch"
  apply gdb-18.1 patches/gdb/gdb-18.1-subleq.patch
  apply gdb-18.1 patches/gdb/gdb-single-threaded-bfd.patch
  apply vim patches/vim/subleq-nommu.patch
  touch "$out/.subleq-prepared"
fi
printf 'Prepared sources in %s (not a completed toolchain build)\n' "$out"
