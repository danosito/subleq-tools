# SUBLEQ developer-tools port — source-only preview

This public repository publishes *source changes and reproducible source pins*, not a working GCC/GDB system nor an ext4 binary disk. It does **not** redistribute the existing private development disk, Linux/uClibc/LLVM SDK, initramfs, credentials, fixtures or service state. No binary source-compliance claim follows from this preview. The separate optional developer disk may be released only after its exact contents and every linked component's corresponding source, build scripts and licenses have been audited and made available; never replace the service-state disk with it.

## Source and license inventory

`SOURCES.tsv` gives SHA-256 and public upstream download URL for GNU Binutils 2.47 (GPLv3+ and library licenses), GCC 16.2.0 (GPLv3+ with GCC Runtime Library Exception for runtime), GDB 18.1 (GPLv3+), GMP 6.3.0 (LGPLv3+/GPLv2+), MPFR 4.2.2 (LGPLv3+), MPC 1.3.1 (LGPLv3+), nano 9.3 (GPLv3+), ncurses 6.6 (MIT-like ncurses license), and Vim 9.2.1398 at commit `9f0f146b4fe23d54faf3ff5c0ba156c7f5a54433` (Vim license). **The actual license text in each upstream archive controls**. The source bundle release asset contains these unmodified upstream tarballs, including their license files, with hashes; the repository contains the complete local port diffs for GCC, Binutils, GDB and Vim. `nano`/`ncurses` builds in the old lane use an inline `config.sub` SUBLEQ CPU recognition modification; it is documented below, not part of an existing unmodified release binary.

LLVM SUBLEQ backend, uClibc-ng, Linux SUBLEQ port and SJLJ C++ runtime are **additional required source components** for a native build and for corresponding source of any statically linked binaries. Historical pin identifiers only: LLVM `bb79724351cc201742c30aa4dbb9aaaa43be3654`, uClibc-ng `073fbd989f5d9b2f4fd93f3da53820ae65cb7063`, Linux `c06286b9aa8a2245af9fbea92b12d812a77a9aa1`. This repository does not contain their complete patches, public origin verification or corresponding build scripts. An upstream commit ID alone is **not** sufficient corresponding source for local modifications. Therefore the disk and all target binaries are intentionally withheld. The hard-coded symbol allowlist in `patches/binutils/0002-as-ld.patch` comes from an old kernel and is not universally applicable.

## Linux source preparation

Requires Linux, `bash`, `curl`, `tar`, `xz`, `patch`, `python3`, `sha256sum` and sufficient free space (at least 4 GiB for source extraction; full tool builds can exceed 35 GiB). No private host paths, VM access or service credentials are required:

```sh
bash recipes/prepare-sources.sh    # download, verify SHA256, extract, apply --fuzz=0
JOBS=2 bash recipes/build-host-binutils.sh
# Only after supplying a source-built compatible sysroot, target assembler/linker:
SUBLEQ_SYSROOT=/absolute/sysroot SUBLEQ_AS=/absolute/as SUBLEQ_LD=/absolute/ld \
  JOBS=2 bash recipes/build-cross-gcc.sh
```

The source preparation was exercised against locally cached upstream archives; no public native build, source-built SDK, or clean-room binary reproduction has been established. `build-host-binutils.sh` and `build-cross-gcc.sh` are provided as recipes, **not as successful build assertions**. Source bundle downloads can be used as `SOURCE_CACHE=/path/to/extracted-bundle`. To build nano/ncurses from the same archives, add `subleq` to the CPU alternatives in both source trees' `config.sub` and configure using the SUBLEQ uClibc target, a SUBLEQ-capable Clang + sysroot, static wide ncurses, and `--without-shared`; Vim additionally applies `patches/vim/subleq-nommu.patch`. No complete editor binary reproduction is claimed. Native GCC additionally needs target-built GMP/MPFR/MPC, SJLJ C++ dependencies, libgcc, a source-built SDK, and a Linux kernel/loader with sufficient contiguous memory. GDB needs those components, a target ncurses build, and candidate ptrace/auxv kernel changes **not installed by this repository**.

## Honest current limitations

Historical private tests established actual x86/ARM-host cross compilation and selected mixed-object guest smoke, but **not** a production-ready native compiler. The first native GCC driver failed to spawn cc1 (`posix_spawn` ENOSYS); a private vfork relink fixed that blocker, then cc1 failed before preprocessing because the old NOMMU kernel's largest contiguous allocation was smaller than cc1's roughly 229 MiB load segment. An experimental larger buddy order loaded `cc1 --version` (exit 0 without text), but a later tiny compile lost the Telnet session with no result; no native compile/link/run pass exists. Version output or static ELF validity does not establish function. Native GDB linked to a static ~117 MB ELF and host file inspection worked, but no in-guest ptrace acceptance was obtained. Breakpoints, stepping, watchpoints and reliable native debugging are **not supported**. Binutils GAS/ld host cross tests existed in historical lanes; native in-guest tool acceptance remains unverified here. No live VM or service disk was touched in preparing this repository.

`patches/gdb/gdb-18.1-subleq.patch` includes GDB-target changes; the Binutils inspection patch is filtered to omit `binutils/*` when applied to the GDB release, which lacks that file. `patches/gdb/gdb-single-threaded-bfd.patch` is a separate single-threaded build adjustment. Kernel ptrace/auxv experiments are deliberately not represented as a deployed debugger feature.
