#!/usr/bin/env python3
"""Remove absolute build paths embedded in Binutils' diagnostic string literals.

Input and output hashes must be recorded separately. This is a byte-length-preserving
.rodata string normalization, not a source or executable-code substitution.
"""
import hashlib
from pathlib import Path
import re
import sys

for name in sys.argv[1:]:
    path = Path(name)
    before = path.read_bytes()
    if not before.startswith(b'\x7fELF'):
        raise SystemExit(f'not ELF: {name}')
    hits = 0
    def replace(match):
        global hits
        old = match.group(0)
        if b'/binutils-2.47/' not in old:
            raise ValueError('unexpected absolute build path')
        rel = old.split(b'/binutils-2.47/', 1)[1]
        replacement = b'/usr/src/binutils-2.47/' + rel
        if len(replacement) > len(old):
            raise ValueError('replacement longer than original')
        hits += 1
        return replacement + b'\x00' * (len(old) - len(replacement))
    after = re.sub(rb'/home/dan/[^\x00\r\n]{1,500}', replace, before)
    if hits == 0 or b'/home/dan/' in after or len(after) != len(before):
        raise ValueError('normalization incomplete')
    path.write_bytes(after)
    print(f'{path.name} original={hashlib.sha256(before).hexdigest()} normalized={hashlib.sha256(after).hexdigest()} paths={hits}')
