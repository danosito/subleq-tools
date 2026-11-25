# SUBLEQ developer-tools port and optional ext4 disk

This repository publishes source changes, reproducible source pins and an **optional, separate, read-only developer-tools disk**. See [DISK.md](DISK.md) for exact contents, source provenance, verification and mounting. The disk contains eight binaries: nano, GNU as/ld, objdump/nm/readelf/size/strings; **not GCC, GDB or Vim**. It does not contain a kernel, initramfs, credentials, fixtures or service state. Never replace the service-state disk with it.

## Linux x86-64 cross C SDK (v0.3)

The [v0.3-cross-sdk release](https://github.com/danosito/subleq-tools/releases/tag/v0.3-cross-sdk) provides the **actual host cross-compiler used for guest C programs**: SUBLEQ Clang 22.1.0 (ESI), LLD, LLVM ELF utilities, kernel UAPI and a static uClibc-ng target sysroot. Download `subleq-cross-sdk-v0.3-linux-x86_64.tar.zst` and its `.sha256` sidecar, verify it with `sha256sum -c`, then extract with `tar --use-compress-program=zstd -xf ...`. From inside the extracted directory in Bash, run `./check-host.sh` and `source ./env-setup.sh`; `"$CC" -O2 hello.c -o hello.elf` produces a static SUBLEQ Linux executable for a compatible guest. The archive's README lists host dependencies, source pins, licensing/relinking instructions and a verified guest execution test. The SDK relocates even when extracted to a path containing spaces. It is **not** GNU GCC, a guest-native compiler, C++ runtime, GDB or a complete decompiler project. LLVM objdump currently has no SUBLEQ instruction disassembler (`-d`); Clang `-S` emits target assembly. Only a **Linux amd64 host** binary is published in v0.3; the ARM64 host SDK build is unfinished and is not included or represented as tested.

SHA-256 of the release archive: `7aebcf4130014fa3bd6f033a8fcdadd6c3ab24b2d2e27b77d4ecb8938b728787` (91,181,017 bytes). The source repository's `sdk/build-sdk-linux.sh` now includes the verified build-recipe corrections and pinned supplemental host LLD header; LLVM/Linux/uClibc source archives remain available as separate v0.2 release assets. Static uClibc-ng applications must be accompanied by corresponding source and materials sufficient to relink with a modified library under the applicable LGPL terms.

## Source and license inventory

`SOURCES.tsv` gives SHA-256 and public upstream download URL for GNU Binutils 2.47 (GPLv3+ and library licenses), GCC 16.2.0 (GPLv3+ with GCC Runtime Library Exception for runtime), GDB 18.1 (GPLv3+), GMP 6.3.0 (LGPLv3+/GPLv2+), MPFR 4.2.2 (LGPLv3+), MPC 1.3.1 (LGPLv3+), nano 9.3 (GPLv3+), ncurses 6.6 (MIT-like ncurses license), and Vim 9.2.1398 at commit `9f0f146b4fe23d54faf3ff5c0ba156c7f5a54433` (Vim license). **The actual license text in each upstream archive controls**. The source bundle release asset contains these unmodified upstream tarballs, including their license files, with hashes; the repository contains the complete local port diffs for GCC, Binutils, GDB and Vim. `nano`/`ncurses` builds in the old lane use an inline `config.sub` SUBLEQ CPU recognition modification; it is documented below, not part of an existing unmodified release binary.

The disk's linked SDK source consists of public `adriancable/{llvm-project,uclibc-ng,linux}` git archives at the exact `sdk/source-commits.txt` pins plus the two `sdk/*ssp.patch` deltas, `sdk/uclibc.config`, and `sdk/build-sdk-linux.sh`; original git-archive hashes were independently reproduced and equal the release assets. SJLJ C++ runtime sources are **not** included, so C++ tools (GCC/GDB) are excluded. Native GCC/GDB require more source and acceptance work. The hard-coded symbol allowlist in `patches/binutils/0002-as-ld.patch` comes from an old kernel and is not universally applicable.

## Linux source preparation

Requires Linux, `bash`, `curl`, `tar`, `xz`, `patch`, `python3`, `sha256sum` and sufficient free space (at least 4 GiB for source extraction; full tool builds can exceed 35 GiB). No private host paths, VM access or service credentials are required:

```sh
bash recipes/prepare-sources.sh    # download, verify SHA256, extract, apply --fuzz=0
JOBS=2 bash recipes/build-host-binutils.sh
# Only after supplying a source-built compatible sysroot, target assembler/linker:
SUBLEQ_SYSROOT=/absolute/sysroot SUBLEQ_AS=/absolute/as SUBLEQ_LD=/absolute/ld \
  JOBS=2 bash recipes/build-cross-gcc.sh
```

The source preparation was exercised against locally cached upstream archives; no clean-room recompile of the released binaries has been established. `build-host-binutils.sh` and `build-cross-gcc.sh` are provided as recipes, **not as successful build assertions**. Source bundle downloads can be used as `SOURCE_CACHE=/path/to/extracted-bundle`. To build nano/ncurses from the same archives, add `subleq` to the CPU alternatives in both source trees' `config.sub` and configure using the SUBLEQ uClibc target, a SUBLEQ-capable Clang + sysroot, static wide ncurses, and `--without-shared`; Vim additionally applies `patches/vim/subleq-nommu.patch`. No complete editor binary reproduction is claimed. Native GCC additionally needs target-built GMP/MPFR/MPC, SJLJ C++ dependencies, libgcc, a source-built SDK, and a Linux kernel/loader with sufficient contiguous memory. GDB needs those components, a target ncurses build, and candidate ptrace/auxv kernel changes **not installed by this repository**.

## Honest current limitations

Historical private tests established actual x86/ARM-host cross compilation and selected mixed-object guest smoke, but **not** a production-ready native compiler. The first native GCC driver failed to spawn cc1 (`posix_spawn` ENOSYS); a private vfork relink fixed that blocker, then cc1 failed before preprocessing because the old NOMMU kernel's largest contiguous allocation was smaller than cc1's roughly 229 MiB load segment. An experimental larger buddy order loaded `cc1 --version` (exit 0 without text), but a later tiny compile lost the Telnet session with no result; no native compile/link/run pass exists. Version output or static ELF validity does not establish function. Native GDB linked to a static ~117 MB ELF and host file inspection worked, but no in-guest ptrace acceptance was obtained. Breakpoints, stepping, watchpoints and reliable native debugging are **not supported**. Binutils GAS/ld host cross tests existed in historical lanes; native in-guest tool acceptance remains unverified here. No live VM or service disk was touched in preparing this repository.

`patches/gdb/gdb-18.1-subleq.patch` includes GDB-target changes; the Binutils inspection patch is filtered to omit `binutils/*` when applied to the GDB release, which lacks that file. `patches/gdb/gdb-single-threaded-bfd.patch` is a separate single-threaded build adjustment. Kernel ptrace/auxv experiments are deliberately not represented as a deployed debugger feature.
