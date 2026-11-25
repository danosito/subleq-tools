# Optional standalone SUBLEQ developer-tools disk v0.2

**Separate from the service-state disk.** The release asset `subleq-devtools-v0.2.ext4.xz` is a fresh, 256 MiB no-journal ext4 filesystem (xz-compressed), intended to be attached **read-only** as an additional virtio block device to a compatible SUBLEQ Linux machine and mounted at `/opt/gcc`. It is not a boot disk, does not change the existing state volume and must not replace the primary ROM/media or service-state image. It contains **no GCC, GDB, Vim, kernel, initramfs, user accounts, keys, challenge artifacts or service code**. The tool binaries are C-based GNU as/ld, objdump/nm/readelf/size/strings (Binutils 2.47), and nano 9.3; there is ncurses 6.6 terminfo, `/etc/nanorc`, upstream nano/ncurses licenses, and on-disk file/symlink manifests. There is one symlink, `usr/share/terminfo/v/vt100-am -> vt100`. No external in-guest runtime libraries are included; the executables were linked as SUBLEQ static PIE against the pinned uClibc SDK.

SHA-256 compressed: `bf95808f8426d3dc9dfee37f6fe60ef3ce4b733f5763e6282f4042945996fddd` (14 MiB). SHA-256 decompressed ext4: `68fe8210ef38811906fff3d5c4431983323222a725884617e78e5a50e7b740e9` (268,435,456 bytes). Verify before use:

```sh
sha256sum subleq-devtools-v0.2.ext4.xz
xz -dk subleq-devtools-v0.2.ext4.xz
sha256sum subleq-devtools-v0.2.ext4
sudo e2fsck -fn subleq-devtools-v0.2.ext4
sudo mount -o loop,ro,noload subleq-devtools-v0.2.ext4 /mnt/subleq-tools
(cd /mnt/subleq-tools && sha256sum -c usr/share/doc/subleq-tools/FILES.sha256)
sudo umount /mnt/subleq-tools
```

An independent `debugfs rdump` comparison of **every file byte** against the allowlisted stage found no extra/missing/mismatched files; the single symlink matched, all eight executables have mode 0755, and `e2fsck -fn` passed. The ext4 root also has the usual empty `lost+found`. Binary and path scans rejected service paths, BF markers, credentials and private-key filenames. The old 640 MiB tools disk was inspected only; it was **not** reused. The new disk has not been run as a complete package inside a guest. Prior guest observations include nano startup and Binutils inspection, but those do not establish every new-disk tool's guest behavior; **GNU as/ld guest execution has not been accepted**. Do not conflate this optional disk with a working native GCC toolchain.

## Exact binary origins and corresponding source

`nano` and its license/terminfo files came only from the independently digest-verified GNU nano/ncurses package candidate. `nm`, `objdump`, `readelf`, `size`, `strings` came only from the independently digest-verified Binutils inspection package. `as`, `ld` came only from the separately digest-verified GNU Binutils link-port candidate. Each GNU Binutils binary embeds absolute **diagnostic source filename strings** from the historical build machine. `recipes/normalize-binutils-paths.py` deterministically rewrites *only* those NUL-terminated strings to `/usr/src/binutils-2.47/<relative source filename>` without changing ELF size or executable instructions. `BINARY-PROVENANCE.txt` records original and published SHA-256, plus the rewritten-string count for all seven. This step is disclosed, not misrepresented as a reproducible original link. The disk's `/usr/share/doc/subleq-tools/FILES.sha256` contains the actual published binary hashes.

All sources needed to study/rebuild these C-only binaries are available from this repository and **assets on the same GitHub release**: original, hash-pinned GNU Binutils, nano, ncurses tarballs in `subleq-tools-upstream-sources-v0.1.tar`; original Git-archive snapshots `llvm-project.tar.gz`, `uclibc-ng.tar.gz`, `linux.tar.gz` from public `adriancable` repositories at `sdk/source-commits.txt` pins; local SDK SSP patches and recorded uClibc configuration under `sdk/`; Binutils patches under `patches/binutils/`; Linux SDK/guest build and disk composition recipes under `sdk/` and `recipes/`. The three SDK archive hashes match **independently regenerated** `git archive ... | gzip -n` outputs at the published commits. The SDK binary itself is not shipped. Licenses are inside the respective source archives (LLVM Apache-2.0 with LLVM exception; Linux GPL-2.0; uClibc LGPL-2.1 and exceptions; GNU Binutils GPLv3+ and library licenses; nano GPLv3+; ncurses permissive). Follow each individual source file's notices. No GCC/GDB/C++ runtime binary is distributed.

## Rebuild on Linux without private paths

Install `gcc`, `g++`, `cmake`, `ninja`, `make`, `flex`, `bison`, `patch`, `python3`, `curl`, `xz`, `tic` (`ncurses-bin`) and `e2fsprogs`. Allow at least 35 GiB to build the SDK. Extract the three SDK release tarballs into a directory containing the three `.tar.gz` files. From the repository root:

```sh
SDK_SOURCE_DIR=/absolute/sdk-archives SDK_WORK=/absolute/new-sdk-work JOBS=2 bash sdk/build-sdk-linux.sh
bash recipes/prepare-sources.sh
SDK_WORK=/absolute/new-sdk-work JOBS=2 bash recipes/build-guest-binutils.sh
SDK_WORK=/absolute/new-sdk-work JOBS=2 bash recipes/build-guest-nano.sh
# After source-built candidates have been audited and staged to the exact allowlist:
python3 recipes/package-tools-disk.py /absolute/allowlisted-stage /absolute/NEW.ext4
```

These portable recipes are published build materials, **not** a claim that a clean-room reproduction was completed; compare rebuilt binary behavior/ELF ABI with the known candidate and audit every stage. `package-tools-disk.py` refuses all files except the exact disk allowlist and refuses to overwrite an existing image. `nano`/Binutils port archives are hash-verified by `prepare-sources.sh`. The historical provenance hashes are in `BINARY-PROVENANCE.txt`; the newly normalized binaries need not be byte-identical after an independent compiler build. The source-built SDK config substitutes a local absolute kernel-header path for the sanitized historical config; historical `sdk/SDK.sha256` and `sdk/patched-sources.sha256` are provenance references, not expected bit-for-bit output hashes on other build hosts.
