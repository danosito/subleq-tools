/* GCC RTL backend for SUBLEQ Linux.
   SPDX-License-Identifier: GPL-3.0-or-later */
#define IN_TARGET_CODE 1
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "target.h"
#include "rtl.h"
#include "tree.h"
#include "stringpool.h"
#include "attribs.h"
#include "builtins.h"
#include "cgraph.h"
#include "df.h"
#include "memmodel.h"
#include "emit-rtl.h"
#include "expr.h"
#include "explow.h"
#include "output.h"
#include "tm_p.h"
#include "real.h"
#include "regs.h"
#include "calls.h"
#include "diagnostic-core.h"
#include "targhooks.h"
#include "target-def.h"

namespace {
constexpr int z = 12, zero = 144, one = 156, minus_one = 152;
constexpr int t0 = 160, t1 = 164, t2 = 168;
unsigned long local_label;

/* One operand per directive: '.' in the C word is instruction PC + 8. */
void
triple (int a, int b, int c = -1)
{
  fprintf (asm_out_file, "\t.word %d\n\t.word %d\n", a, b);
  if (c < 0)
    fputs ("\t.word .+4\n", asm_out_file);
  else
    fprintf (asm_out_file, "\t.word %d\n", c);
}

void
label (unsigned long l)
{
  fprintf (asm_out_file, ".Lsq%lu:\n", l);
}

void
branch (int a, int b, unsigned long l)
{
  fprintf (asm_out_file, "\t.word %d\n\t.word %d\n\t.word .Lsq%lu\n", a, b, l);
}

void
copy_word (int src, int dst)
{
  if (src == dst)
    return;
  triple (z, z);
  triple (src, z);
  triple (dst, dst);
  triple (z, dst);
}

/* The address of an assembly expression lives in a relocated literal.
   A negative relocation lets a SUBLEQ instruction load it directly. */
unsigned long
literal (rtx value, bool negative)
{
  unsigned long l = local_label++;
  fputs ("\t.pushsection .rodata\n\t.p2align 2\n", asm_out_file);
  label (l);
  fputs ("\t.word ", asm_out_file);
  if (negative)
    fputs ("-(", asm_out_file);
  output_addr_const (asm_out_file, value);
  if (negative)
    fputc (')', asm_out_file);
  fputs ("\n\t.popsection\n", asm_out_file);
  return l;
}

void
subtract_literal (unsigned long l, int dst)
{
  fprintf (asm_out_file, "\t.word .Lsq%lu\n\t.word %d\n\t.word .+4\n", l, dst);
}

void
load_literal (rtx value, int dst)
{
  if (CONST_DOUBLE_P (value))
    {
      long bits;
      REAL_VALUE_TO_TARGET_SINGLE (*CONST_DOUBLE_REAL_VALUE (value), bits);
      value = gen_int_mode (bits, SImode);
    }
  triple (dst, dst);
  if (value != const0_rtx)
    subtract_literal (literal (value, true), dst);
}

int
register_address (rtx x)
{
  unsigned regno = REGNO (x);
  gcc_assert (regno < FRAME_POINTER_REGNUM);
  return regno * 4;
}

void
effective_address (rtx x, int dst)
{
  if (REG_P (x))
    copy_word (register_address (x), dst);
  else if (GET_CODE (x) == PLUS && REG_P (XEXP (x, 0)))
    {
      copy_word (register_address (XEXP (x, 0)), dst);
      subtract_literal (literal (XEXP (x, 1), true), dst);
    }
  else
    load_literal (x, dst);
}

int
word_address (rtx x)
{
  if (REG_P (x))
    return register_address (x) + 1;
  effective_address (x, t0);
  return t0 + 1;
}

/* Branch on the sign bit without changing SRC. Testing (x <= 0) and
   then (x+1 <= 0) distinguishes negative from zero without overflow. */
void
branch_negative (int src, unsigned long negative, unsigned long nonnegative)
{
  unsigned long le_zero = local_label++;
  copy_word (src, t2);
  branch (zero, t2, le_zero);
  branch (z, z, nonnegative);
  label (le_zero);
  branch (minus_one, t2, negative);
  branch (z, z, nonnegative);
}

void
branch_le (int a, int b, bool unsigned_p, unsigned long yes, unsigned long no)
{
  unsigned long a_neg = local_label++, a_nonneg = local_label++;
  unsigned long same_sign = local_label++;
  branch_negative (a, a_neg, a_nonneg);
  label (a_neg);
  branch_negative (b, same_sign, unsigned_p ? no : yes);
  label (a_nonneg);
  branch_negative (b, unsigned_p ? yes : no, same_sign);
  label (same_sign);
  copy_word (a, t1);
  branch (b, t1, yes);
  branch (z, z, no);
}

rtx
frame_slot (HOST_WIDE_INT offset)
{
  return gen_frame_mem (SImode, plus_constant (Pmode, hard_frame_pointer_rtx, offset));
}

bool
save_register (unsigned r)
{
  return !fixed_regs[r] && !call_used_regs[r] && df_regs_ever_live_p (r);
}

HOST_WIDE_INT
frame_size ()
{
  HOST_WIDE_INT size = 8 + get_frame_size ().to_constant ();
  for (unsigned r = 0; r < FIRST_PSEUDO_REGISTER; ++r)
    if (save_register (r))
      size += 4;
  return ROUND_UP (size + crtl->outgoing_args_size.to_constant (), 4);
}
} // namespace

bool
subleq_base_register_p (int r)
{
  return r == SUBLEQ_SP || (r >= 7 && r <= 35) || r == SUBLEQ_FP
    || r == FRAME_POINTER_REGNUM || r == ARG_POINTER_REGNUM;
}

static bool
subleq_legitimate_address_p (machine_mode, rtx x, bool strict, code_helper)
{
  if (CONSTANT_ADDRESS_P (x))
    return true;
  if (GET_CODE (x) == PLUS && CONST_INT_P (XEXP (x, 1)))
    x = XEXP (x, 0);
  if (!REG_P (x))
    return false;
  int r = REGNO (x);
  return r < FIRST_PSEUDO_REGISTER ? subleq_base_register_p (r)
    : !strict || (reg_renumber && subleq_base_register_p (reg_renumber[r]));
}

static void
subleq_init_sections ()
{
  text_section = get_unnamed_section (SECTION_CODE, output_section_asm_op, "\t.text");
  data_section = get_unnamed_section (SECTION_WRITE, output_section_asm_op, "\t.data");
  bss_section = get_unnamed_section (SECTION_WRITE | SECTION_BSS, output_section_asm_op, "\t.bss");
}

static void
subleq_globalize_label (FILE *file, const char *name)
{
  fputs ("\t.globl\t", file);
  assemble_name (file, name);
  fputc ('\n', file);
}

static void
subleq_print_operand (FILE *file, rtx x, int code)
{
  if (code != 0)
    output_operand_lossage ("unsupported SUBLEQ operand modifier %qc", code);
  else if (REG_P (x))
    fprintf (file, "%d", register_address (x));
  else if (MEM_P (x))
    output_addr_const (file, XEXP (x, 0));
  else
    output_addr_const (file, x);
}

static void
subleq_print_operand_address (FILE *file, machine_mode, rtx x)
{
  subleq_print_operand (file, x, 0);
}

void
subleq_expand_move (rtx *operands, machine_mode mode)
{
  if (can_create_pseudo_p () && MEM_P (operands[0]) && !register_operand (operands[1], mode))
    operands[1] = force_reg (mode, operands[1]);
}

const char *
subleq_output_move (rtx *operands)
{
  rtx dst = operands[0], src = operands[1];
  if (MEM_P (dst))
    copy_word (register_address (src), word_address (XEXP (dst, 0)));
  else if (MEM_P (src))
    copy_word (word_address (XEXP (src, 0)), register_address (dst));
  else if (REG_P (src))
    copy_word (register_address (src), register_address (dst));
  else
    load_literal (src, register_address (dst));
  return "";
}

const char *
subleq_output_add (rtx *operands, bool subtract)
{
  int dst = register_address (operands[0]);
  int a = register_address (operands[1]);
  rtx b = operands[2];
  if (CONSTANT_P (b))
    {
      copy_word (a, dst);
      subtract_literal (literal (b, !subtract), dst);
    }
  else
    {
      int baddr = register_address (b);
      if (subtract && dst == a)
        triple (baddr, dst);
      else
        {
          copy_word (a, t0);
          if (subtract)
            triple (baddr, t0);
          else
            {
              triple (z, z);
              triple (baddr, z);
              triple (z, t0);
            }
          copy_word (t0, dst);
        }
    }
  return "";
}

const char *
subleq_output_call (rtx target)
{
  unsigned long ret = local_label++, negret = local_label++;
  fputs ("\t.pushsection .rodata\n\t.p2align 2\n", asm_out_file);
  label (negret);
  fprintf (asm_out_file, "\t.word -.Lsq%lu\n\t.popsection\n", ret);
  triple (SUBLEQ_RA * 4, SUBLEQ_RA * 4);
  subtract_literal (negret, SUBLEQ_RA * 4);
  if (REG_P (target))
    triple (z, z, register_address (target) + 1);
  else
    {
      fputs ("\t.word 12\n\t.word 12\n\t.word ", asm_out_file);
      output_addr_const (asm_out_file, target);
      fputc ('\n', asm_out_file);
    }
  label (ret);
  return "";
}

namespace {
void
emit_comparison (rtx *operands, unsigned long taken, unsigned long fallthrough)
{
  int a = register_address (operands[1]);
  int b = operands[2] == const0_rtx ? zero : register_address (operands[2]);
  auto code = GET_CODE (operands[0]);
  switch (code)
    {
    case EQ: case NE:
      {
        unsigned long le = local_label++;
        unsigned long eq = code == EQ ? taken : fallthrough;
        unsigned long ne = code == EQ ? fallthrough : taken;
        copy_word (a, t1);
        branch (b, t1, le);
        branch (z, z, ne);
        label (le);
        branch (minus_one, t1, ne);
        branch (z, z, eq);
        break;
      }
    case LE: case LEU: branch_le (a, b, code == LEU, taken, fallthrough); break;
    case GE: case GEU: branch_le (b, a, code == GEU, taken, fallthrough); break;
    case LT: case LTU: branch_le (b, a, code == LTU, fallthrough, taken); break;
    case GT: case GTU: branch_le (a, b, code == GTU, fallthrough, taken); break;
    default: gcc_unreachable ();
    }
}
} // namespace

const char *
subleq_output_branch (rtx *operands)
{
  unsigned long taken = local_label++, fallthrough = local_label++;
  emit_comparison (operands, taken, fallthrough);
  label (taken);
  output_asm_insn (".word 12, 12, %l3", operands);
  label (fallthrough);
  return "";
}

const char *
subleq_output_cstore (rtx *operands)
{
  unsigned long yes = local_label++, no = local_label++, done = local_label++;
  emit_comparison (operands + 1, yes, no);
  int dst = register_address (operands[0]);
  label (yes);
  copy_word (one, dst);
  branch (z, z, done);
  label (no);
  triple (dst, dst);
  label (done);
  return "";
}

void
subleq_expand_prologue ()
{
  /* For a split register/stack parameter, GCC homes the register prefix
     immediately below the physical incoming SP. FP is the start of that
     pretend area (which is also GCC's virtual incoming-args address).
     The remaining frame grows below FP; outgoing arguments are at its base. */
  HOST_WIDE_INT size = frame_size ();
  HOST_WIDE_INT pretend = crtl->args.pretend_args_size;
  rtx old_fp = gen_rtx_REG (Pmode, SUBLEQ_FP);
  rtx ra = gen_rtx_REG (Pmode, SUBLEQ_RA);
  HOST_WIDE_INT offset = -size + crtl->outgoing_args_size.to_constant ();
  rtx_insn *adjust = emit_insn (gen_addsi3 (stack_pointer_rtx, stack_pointer_rtx, GEN_INT (-size - pretend)));
  RTX_FRAME_RELATED_P (adjust) = 1;
  rtx save_fp = gen_frame_mem (SImode, plus_constant (Pmode, stack_pointer_rtx, offset + size));
  RTX_FRAME_RELATED_P (emit_move_insn (save_fp, old_fp)) = 1;
  RTX_FRAME_RELATED_P (emit_insn (gen_addsi3 (hard_frame_pointer_rtx, stack_pointer_rtx, GEN_INT (size)))) = 1;
  offset += 4;
  RTX_FRAME_RELATED_P (emit_move_insn (frame_slot (offset), ra)) = 1;
  for (unsigned r = 0; r < FIRST_PSEUDO_REGISTER; ++r)
    if (save_register (r))
      {
        offset += 4;
        RTX_FRAME_RELATED_P (emit_move_insn (frame_slot (offset), gen_rtx_REG (SImode, r))) = 1;
      }
}

void
subleq_expand_epilogue ()
{
  HOST_WIDE_INT offset = -frame_size () + crtl->outgoing_args_size.to_constant ();
  emit_move_insn (gen_rtx_REG (Pmode, SUBLEQ_RA), frame_slot (offset + 4));
  HOST_WIDE_INT reg_offset = offset + 4;
  for (unsigned r = 0; r < FIRST_PSEUDO_REGISTER; ++r)
    if (save_register (r))
      {
        reg_offset += 4;
        emit_move_insn (gen_rtx_REG (SImode, r), frame_slot (reg_offset));
      }
  HOST_WIDE_INT pretend = crtl->args.pretend_args_size;
  if (pretend)
    emit_insn (gen_addsi3 (stack_pointer_rtx, hard_frame_pointer_rtx, GEN_INT (pretend)));
  else
    emit_move_insn (stack_pointer_rtx, hard_frame_pointer_rtx);
  emit_move_insn (hard_frame_pointer_rtx, frame_slot (offset));
  emit_jump_insn (gen_subleq_return ());
}

static unsigned
arg_words (const function_arg_info &arg)
{
  return ROUND_UP (arg.promoted_size_in_bytes ().to_constant (), 4) / 4;
}

static rtx
subleq_function_arg (cumulative_args_t ca, const function_arg_info &arg)
{
  int n = get_cumulative_args (ca)->words;
  if (arg.end_marker_p () || n >= SUBLEQ_NARGS)
    return NULL_RTX;
  return gen_rtx_REG (arg_words (arg) > (unsigned)(SUBLEQ_NARGS - n) ? SImode : arg.mode,
                      SUBLEQ_ARG0 + n);
}

static void
subleq_function_arg_advance (cumulative_args_t ca, const function_arg_info &arg)
{
  get_cumulative_args (ca)->words += arg_words (arg);
}

static int
subleq_arg_partial_bytes (cumulative_args_t ca, const function_arg_info &arg)
{
  int n = get_cumulative_args (ca)->words;
  return n < SUBLEQ_NARGS && arg_words (arg) > (unsigned)(SUBLEQ_NARGS - n)
    ? (SUBLEQ_NARGS - n) * 4 : 0;
}

static bool
subleq_pass_by_reference (cumulative_args_t, const function_arg_info &arg)
{
  return arg.type && AGGREGATE_TYPE_P (arg.type);
}

/* Pass a copy on outgoing calls. Also allow GCC callees to receive a raw
   aggregate address: the pinned LLVM LowerCall passes the byval pointer
   directly and can therefore point at the caller's original object. */
static bool
subleq_callee_copies (cumulative_args_t ca, const function_arg_info &arg)
{
  const auto *args = get_cumulative_args (ca);
  return args && args->incoming && subleq_pass_by_reference (ca, arg);
}

static bool
subleq_return_in_memory (const_tree t, const_tree)
{
  return AGGREGATE_TYPE_P (t) || TYPE_MODE (t) == BLKmode || int_size_in_bytes (t) > 4;
}

static rtx
subleq_function_value (const_tree type, const_tree fn, bool)
{
  int unsigned_p = TYPE_UNSIGNED (type);
  return gen_rtx_REG (promote_function_mode (type, TYPE_MODE (type), &unsigned_p, fn, 1), SUBLEQ_RESULT);
}

static rtx
subleq_libcall_value (machine_mode mode, const_rtx)
{
  return gen_rtx_REG (mode, SUBLEQ_RESULT);
}

static bool
subleq_function_value_regno_p (unsigned r)
{
  return r == SUBLEQ_RESULT;
}

static bool
subleq_hard_regno_mode_ok (unsigned r, machine_mode mode)
{
  if (!subleq_base_register_p (r) && r != SUBLEQ_RA)
    return false;
  auto size = GET_MODE_SIZE (mode);
  if (size <= 4)
    return true;
  return r >= 7 && r + (size - 1) / 4 <= 35;
}

#undef TARGET_LEGITIMATE_ADDRESS_P
#define TARGET_LEGITIMATE_ADDRESS_P subleq_legitimate_address_p
#undef TARGET_ASM_INIT_SECTIONS
#define TARGET_ASM_INIT_SECTIONS subleq_init_sections
#undef TARGET_ASM_GLOBALIZE_LABEL
#define TARGET_ASM_GLOBALIZE_LABEL subleq_globalize_label
#undef TARGET_PRINT_OPERAND
#define TARGET_PRINT_OPERAND subleq_print_operand
#undef TARGET_PRINT_OPERAND_ADDRESS
#define TARGET_PRINT_OPERAND_ADDRESS subleq_print_operand_address
#undef TARGET_FRAME_POINTER_REQUIRED
#define TARGET_FRAME_POINTER_REQUIRED hook_bool_void_true
#undef TARGET_FUNCTION_ARG
#define TARGET_FUNCTION_ARG subleq_function_arg
#undef TARGET_FUNCTION_ARG_ADVANCE
#define TARGET_FUNCTION_ARG_ADVANCE subleq_function_arg_advance
#undef TARGET_ARG_PARTIAL_BYTES
#define TARGET_ARG_PARTIAL_BYTES subleq_arg_partial_bytes
#undef TARGET_PASS_BY_REFERENCE
#define TARGET_PASS_BY_REFERENCE subleq_pass_by_reference
#undef TARGET_CALLEE_COPIES
#define TARGET_CALLEE_COPIES subleq_callee_copies
#undef TARGET_RETURN_IN_MEMORY
#define TARGET_RETURN_IN_MEMORY subleq_return_in_memory
#undef TARGET_FUNCTION_VALUE
#define TARGET_FUNCTION_VALUE subleq_function_value
#undef TARGET_LIBCALL_VALUE
#define TARGET_LIBCALL_VALUE subleq_libcall_value
#undef TARGET_FUNCTION_VALUE_REGNO_P
#define TARGET_FUNCTION_VALUE_REGNO_P subleq_function_value_regno_p
#undef TARGET_PROMOTE_FUNCTION_MODE
#define TARGET_PROMOTE_FUNCTION_MODE default_promote_function_mode_always_promote
#undef TARGET_HARD_REGNO_MODE_OK
#define TARGET_HARD_REGNO_MODE_OK subleq_hard_regno_mode_ok

struct gcc_target targetm = TARGET_INITIALIZER;

namespace {
/* Narrow moves can be LRA reloads and cannot expose call clobbers. The
   loader helpers preserve the callee-saved ABI registers; save the five
   caller-saved ones and RA in a private, balanced stack area. Input values in
   T0/T1 are consumed before the call, not assumed live across the helper. */
void
kernel_helper (const char *name, int nargs, int result, int dst)
{
  constexpr int saved[] = { 20, 96, 100, 104, 108, 112 };
  auto plus_four = literal (GEN_INT (4), false);
  auto minus_four = literal (GEN_INT (-4), false);
  for (int r : saved)
    {
      subtract_literal (plus_four, 16);
      copy_word (r, 17);
    }
  copy_word (t0, 100);
  if (nargs == 2)
    copy_word (t1, 104);
  subleq_output_call (gen_rtx_SYMBOL_REF (Pmode, name));
  if (dst >= 0)
    copy_word (result, t2);
  for (int i = ARRAY_SIZE (saved) - 1; i >= 0; --i)
    {
      copy_word (17, saved[i]);
      subtract_literal (minus_four, 16);
    }
  if (dst >= 0)
    copy_word (t2, dst);
}

void
load_narrow (rtx src, int dst, machine_mode mode)
{
  effective_address (XEXP (src, 0), t0);
  kernel_helper (mode == QImode ? "__subleq_lb" : "__subleq_lh", 1, 96, dst);
}
} // namespace

const char *
subleq_output_narrow_move (rtx *operands, machine_mode mode)
{
  rtx dst = operands[0], src = operands[1];
  if (MEM_P (dst))
    {
      effective_address (XEXP (dst, 0), t0);
      copy_word (register_address (src), t1);
      kernel_helper (mode == QImode ? "__subleq_sb" : "__subleq_sh", 2, -1, -1);
    }
  else if (MEM_P (src))
    load_narrow (src, register_address (dst), mode);
  else
    subleq_output_move (operands);
  return "";
}

const char *
subleq_output_extend (rtx *operands, machine_mode mode, bool sign)
{
  int dst = register_address (operands[0]);
  int limit = mode == QImode ? 256 : 65536;
  rtx src = operands[1];
  if (MEM_P (src))
    load_narrow (src, dst, mode);
  else
    {
      copy_word (register_address (src), t0);
      load_literal (GEN_INT (limit - 1), t1);
      kernel_helper ("__subleq_and", 2, 96, dst);
    }
  if (sign)
    {
      unsigned long negative = local_label++, done = local_label++;
      load_literal (GEN_INT (limit / 2), t0);
      branch (dst, t0, negative);
      branch (z, z, done);
      label (negative);
      subtract_literal (literal (GEN_INT (limit), false), dst);
      label (done);
    }
  return "";
}

const char *
subleq_output_binary_helper (rtx *operands, const char *name, int result)
{
  copy_word (register_address (operands[1]), t0);
  copy_word (register_address (operands[2]), t1);
  kernel_helper (name, 2, result, register_address (operands[0]));
  return "";
}

void
subleq_split_wide_move (rtx *operands)
{
  rtx dst = operands[0], src = operands[1];
  machine_mode mode = GET_MODE (dst);
  bool high_first = REG_P (dst) && REG_P (src) && REGNO (dst) == REGNO (src) + 1;
  for (int i = 0; i < 2; ++i)
    {
      int word = high_first ? 1 - i : i;
      rtx to = operand_subword (dst, word, 1, mode);
      rtx from = operand_subword (src, word, 1, mode);
      gcc_assert (to && from);
      emit_move_insn (to, from);
    }
}
