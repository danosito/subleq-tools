/* GCC target definitions for SUBLEQ Linux.
   SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef GCC_SUBLEQ_H
#define GCC_SUBLEQ_H

#define TARGET_CPU_CPP_BUILTINS() do { \
  builtin_define ("__subleq__"); \
  builtin_define ("__SUBLEQ__"); \
  builtin_define ("__SUBLEQ_WORD_ADDRESSABLE__"); \
  builtin_define ("__SOFTFP__"); \
} while (0)
#define TARGET_OS_CPP_BUILTINS() GNU_USER_TARGET_OS_CPP_BUILTINS ()

/* The NOMMU kernel relocates absolute address words, including in text. */
#undef STARTFILE_SPEC
#define STARTFILE_SPEC "%{!shared:%{!r:crt1.o%s crti.o%s}}"
#undef ENDFILE_SPEC
#define ENDFILE_SPEC "%{!shared:%{!r:crtn.o%s}}"
#undef LIB_SPEC
#define LIB_SPEC "%{!r:%{pthread:-lpthread} -lc}"
#undef LIBGCC_SPEC
#define LIBGCC_SPEC "%{!r:%:if-exists(libgcc.a%s)}"
#undef CPP_SPEC
#define CPP_SPEC "%{!nostdinc:-idirafter %R/kernel-headers/include}"
#undef LINK_SPEC
#define LINK_SPEC "%{shared:-shared} %{!shared:%{!r:%{!no-pie:-pie} --no-dynamic-linker -Bstatic -L%R/lib}}"
#define ASM_COMMENT_START "#"
#undef ASM_APP_ON
#define ASM_APP_ON ""
#undef ASM_APP_OFF
#define ASM_APP_OFF ""
#define ASM_OUTPUT_ALIGN(FILE, LOG) fprintf (FILE, "\t.p2align\t%d\n", (LOG))

#define BITS_BIG_ENDIAN 0
#define BYTES_BIG_ENDIAN 0
#define WORDS_BIG_ENDIAN 0
#define UNITS_PER_WORD 4
#define POINTER_SIZE 32
#define INT_TYPE_SIZE 32
#define LONG_TYPE_SIZE 32
#define LONG_LONG_TYPE_SIZE 64
#define DEFAULT_SIGNED_CHAR 1
#undef SIZE_TYPE
#define SIZE_TYPE "unsigned int"
#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "int"
#define PARM_BOUNDARY 32
#define STACK_BOUNDARY 32
#define FUNCTION_BOUNDARY 32
#define BIGGEST_ALIGNMENT 32
#define STRICT_ALIGNMENT 1
#define PCC_BITFIELD_TYPE_MATTERS 1
#define DATA_ALIGNMENT(TYPE, ALIGN) MAX (32, (ALIGN))
#define SLOW_BYTE_ACCESS 1
#define MOVE_MAX 4
#define Pmode SImode
#define FUNCTION_MODE QImode
#define CASE_VECTOR_MODE SImode

/* Hard register numbers are the ABI word locations. Registers 40 and 41
   are GCC-only frame and argument pointers; neither is ever emitted. */
#define FIRST_PSEUDO_REGISTER 42
#define REGISTER_NAMES { \
  "?0", "?1", "?2", "z", "sp", "ra", "?6", "r3", \
  "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", \
  "r12", "r13", "r14", "r15", "r16", "r17", "r18", "r19", \
  "r20", "r21", "r22", "r23", "r24", "r25", "r26", "r27", \
  "r28", "r29", "r30", "r31", "zero", "fp", "?38", "?39", "?vfp", "?ap" }
#define FIXED_REGISTERS { \
  1,1,1,1,1,1,1,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, \
  0,0,0,0,0,0,0,0, 0,0,0,0,1,1,1,1,1,1 }
#define CALL_USED_REGISTERS { \
  1,1,1,1,1,1,1,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, \
  1,1,1,1,1,0,0,0, 0,0,0,0,1,1,1,1,1,1 }
#define SUBLEQ_SP 4
#define SUBLEQ_RA 5
#define SUBLEQ_RESULT 24
#define SUBLEQ_ARG0 25
#define SUBLEQ_NARGS 4
#define SUBLEQ_FP 37
#define STACK_POINTER_REGNUM SUBLEQ_SP
#define HARD_FRAME_POINTER_REGNUM SUBLEQ_FP
#define FRAME_POINTER_REGNUM 40
#define ARG_POINTER_REGNUM 41
#define DWARF_FRAME_RETURN_COLUMN SUBLEQ_RA
#define INCOMING_RETURN_ADDR_RTX gen_rtx_REG (Pmode, SUBLEQ_RA)
#define EPILOGUE_USES(REGNO) ((REGNO) == SUBLEQ_RA)
#define STACK_GROWS_DOWNWARD 1
#define FRAME_GROWS_DOWNWARD 1
#define FIRST_PARM_OFFSET(FNDECL) 0
#define ACCUMULATE_OUTGOING_ARGS 1
#define ELIMINABLE_REGS { \
  { FRAME_POINTER_REGNUM, HARD_FRAME_POINTER_REGNUM }, \
  { ARG_POINTER_REGNUM, HARD_FRAME_POINTER_REGNUM } }
#define INITIAL_ELIMINATION_OFFSET(FROM, TO, OFFSET) ((OFFSET) = 0)
#define FUNCTION_ARG_REGNO_P(R) ((R) >= SUBLEQ_ARG0 && (R) < SUBLEQ_ARG0 + SUBLEQ_NARGS)
struct subleq_args { int words; bool incoming; };
#define CUMULATIVE_ARGS subleq_args
#define INIT_CUMULATIVE_ARGS(CUM, FNTYPE, LIBNAME, FNDECL, N) do { \
  (CUM).words = ((FNTYPE) && stdarg_p (FNTYPE)) ? SUBLEQ_NARGS : 0; \
  (CUM).incoming = (N) < 0; \
} while (0)
#define PROMOTE_MODE(MODE, UNSIGNED_P, TYPE) \
  if (GET_MODE_CLASS (MODE) == MODE_INT && GET_MODE_SIZE (MODE) < UNITS_PER_WORD) \
    (MODE) = SImode

/* Both the architectural SP/FP and virtual bases can occur in RTL. */
enum reg_class { NO_REGS, GENERAL_REGS, ALL_REGS, LIM_REG_CLASSES };
#define N_REG_CLASSES LIM_REG_CLASSES
#define REG_CLASS_NAMES { "NO_REGS", "GENERAL_REGS", "ALL_REGS" }
#define REG_CLASS_CONTENTS { {0,0}, {0xffffffb0,0x32f}, {0xffffffff,0x3ff} }
#define REGNO_REG_CLASS(R) ((subleq_base_register_p (R) || (R) == SUBLEQ_RA) ? GENERAL_REGS : NO_REGS)
#define REGNO_OK_FOR_BASE_P(R) subleq_base_register_p (R)
#define REGNO_OK_FOR_INDEX_P(R) false

#define BASE_REG_CLASS GENERAL_REGS
#define INDEX_REG_CLASS NO_REGS
#define MAX_REGS_PER_ADDRESS 1
#define TRAMPOLINE_SIZE 0

#define HAS_LONG_UNCOND_BRANCH true
#define FUNCTION_PROFILER(FILE, LABEL) sorry ("profiling on SUBLEQ")
#endif
