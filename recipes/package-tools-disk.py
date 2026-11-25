#!/usr/bin/env python3
"""Make a NEW optional ext4 disk from an allowlisted, source-attributed package stage.

Never accepts a base/service image. Linux requires e2fsprogs (mke2fs/e2fsck).
The stage must contain precisely the named tools, terminfo, license files and nanorc.
"""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import stat
import struct
import subprocess

BINS = ('as', 'ld', 'nano', 'nm', 'objdump', 'readelf', 'size', 'strings')
FILES = {'usr/bin/' + x for x in BINS} | {
    'etc/nanorc', 'usr/share/licenses/nano/COPYING',
    'usr/share/licenses/nano/COPYING.DOC', 'usr/share/licenses/ncurses/COPYING',
    'usr/share/terminfo/v/vt100', 'usr/share/terminfo/x/xterm',
    'usr/share/terminfo/x/xterm-256color',
}
LINKS = {'usr/share/terminfo/v/vt100-am': 'vt100'}
BAD = ('/home/dan/', '/Users/', '/root/.ssh/', 'flag{', 'id_rsa', 'bf_service', 'service-state', 'dev/qemu-subleq/lanes/')


def run(*args):
    subprocess.run(args, check=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('stage', type=Path, help='fresh, allowlisted package tree')
    ap.add_argument('output', type=Path, help='new .ext4 path; must not exist')
    a = ap.parse_args()
    root = a.stage.resolve(strict=True)
    out = a.output.absolute()
    if out.exists() or out.is_symlink():
        ap.error('output already exists; refusing overwrite')
    files, links = {}, {}
    for base, dirs, names in os.walk(root, followlinks=False):
        for name in dirs + names:
            path = Path(base, name)
            rel = path.relative_to(root).as_posix()
            if any(word.lower() in rel.lower() for word in BAD):
                ap.error(f'prohibited name: {rel}')
            if path.is_symlink():
                links[rel] = os.readlink(path)
            elif path.is_file():
                files[rel] = path
            elif not path.is_dir():
                ap.error(f'unsupported filesystem entry: {rel}')
    if set(files) != FILES or links != LINKS:
        ap.error(f'allowlist mismatch: extra={sorted(set(files)-FILES)} missing={sorted(FILES-set(files))} links={links}')
    for name in BINS:
        path = files['usr/bin/' + name]
        data = path.read_bytes()
        if not data.startswith(b'\x7fELF') or data[4:6] != b'\x01\x01' or struct.unpack_from('<H', data, 18)[0] != 0xb124:
            ap.error(f'not ELF32 little-endian SUBLEQ: {name}')
        if any(word.encode().lower() in data.lower() for word in BAD):
            ap.error(f'prohibited content in binary: {name}')
        path.chmod(0o755)
    for p in root.rglob('*'):
        if p.is_dir() and not p.is_symlink():
            p.chmod(0o755)
    doc = root / 'usr/share/doc/subleq-tools'
    doc.mkdir(parents=True, exist_ok=False)
    lines = [f'{hashlib.sha256(path.read_bytes()).hexdigest()}  /{rel}' for rel, path in sorted(files.items())]
    (doc / 'FILES.sha256').write_text('\n'.join(lines) + '\n')
    (doc / 'SYMLINKS.txt').write_text('\n'.join(f'/{p} -> {target}' for p, target in sorted(links.items())) + '\n')
    # No journal: this disk is an optional read-only mount, never a state volume.
    blocks = 65536  # 256 MiB at 4 KiB; > 80 MiB of payload.
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open('xb') as f:
        f.truncate(blocks * 4096)
    try:
        run(shutil.which('mke2fs') or '/opt/homebrew/opt/e2fsprogs/sbin/mke2fs', '-F', '-q', '-t', 'ext4', '-b', '4096',
            '-L', 'subleq-tools', '-O', '^has_journal,^64bit,^metadata_csum', '-d', str(root), str(out))
        run(shutil.which('e2fsck') or '/opt/homebrew/opt/e2fsprogs/sbin/e2fsck', '-fn', str(out))
    except Exception:
        out.unlink()
        raise
    print(f'{hashlib.sha256(out.read_bytes()).hexdigest()}  {out}')

if __name__ == '__main__':
    main()
