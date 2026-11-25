;; GCC RTL patterns for SUBLEQ Linux.
;; SPDX-License-Identifier: GPL-3.0-or-later
(include "constraints.md")

(define_constants [(RA 5)])

(define_predicate "subleq_comparison_operator"
  (match_code "eq,ne,lt,ltu,le,leu,gt,gtu,ge,geu"))

(define_expand "movsi"
  [(set (match_operand:SI 0 "nonimmediate_operand")
        (match_operand:SI 1 "general_operand"))]
  ""
  { subleq_expand_move (operands, SImode); })

(define_insn "*movsi"
  [(set (match_operand:SI 0 "nonimmediate_operand" "=r,r,m")
        (match_operand:SI 1 "general_operand" "ri,m,r"))]
  ""
  { return subleq_output_move (operands); })

(define_insn "addsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (plus:SI (match_operand:SI 1 "register_operand" "r")
                 (match_operand:SI 2 "nonmemory_operand" "ri")))]
  ""
  { return subleq_output_add (operands, false); })

(define_insn "subsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (minus:SI (match_operand:SI 1 "register_operand" "r")
                  (match_operand:SI 2 "nonmemory_operand" "ri")))]
  ""
  { return subleq_output_add (operands, true); })

(define_insn "nop" [(const_int 0)] "" ".word 12, 12, .+4")

(define_insn "jump"
  [(set (pc) (label_ref (match_operand 0 "")))]
  "" ".word 12, 12, %l0")

(define_insn "indirect_jump"
  [(set (pc) (match_operand:SI 0 "register_operand" "r"))]
  "" ".word 12, 12, %0+1")

(define_insn "cbranchsi4"
  [(set (pc)
        (if_then_else (match_operator 0 "subleq_comparison_operator"
                         [(match_operand:SI 1 "register_operand" "r")
                          (match_operand:SI 2 "nonmemory_operand" "rJ")])
                      (label_ref (match_operand 3 ""))
                      (pc)))]
  "" { return subleq_output_branch (operands); })

(define_insn "cstoresi4"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (match_operator:SI 1 "subleq_comparison_operator"
          [(match_operand:SI 2 "register_operand" "r")
           (match_operand:SI 3 "nonmemory_operand" "rJ")]))]
  "" { return subleq_output_cstore (operands); })

(define_expand "call"
  [(parallel [(call (match_operand:QI 0 "memory_operand") (match_operand 1 ""))
              (clobber (reg:SI RA))])]
  "")

(define_insn "*call"
  [(call (mem:QI (match_operand:SI 0 "nonmemory_operand" "ri")) (match_operand 1 ""))
   (clobber (reg:SI RA))]
  "" { return subleq_output_call (operands[0]); })

(define_expand "call_value"
  [(parallel [(set (match_operand 0 "register_operand")
                   (call (match_operand:QI 1 "memory_operand") (match_operand 2 "")))
              (clobber (reg:SI RA))])]
  "")

(define_insn "*call_value"
  [(set (match_operand 0 "register_operand" "=r")
        (call (mem:QI (match_operand:SI 1 "nonmemory_operand" "ri")) (match_operand 2 "")))
   (clobber (reg:SI RA))]
  "" { return subleq_output_call (operands[1]); })

(define_expand "prologue" [(const_int 0)] ""
  { subleq_expand_prologue (); DONE; })

(define_expand "epilogue" [(return)] ""
  { subleq_expand_epilogue (); DONE; })

(define_insn "subleq_return" [(return) (use (reg:SI RA))]
  "reload_completed" ".word 12, 12, 21")

(define_mode_iterator Narrow [QI HI])
(define_expand "mov<mode>"
  [(set (match_operand:Narrow 0 "nonimmediate_operand")
        (match_operand:Narrow 1 "general_operand"))]
  "" { subleq_expand_move (operands, <MODE>mode); })

(define_insn "*mov<mode>"
  [(set (match_operand:Narrow 0 "nonimmediate_operand" "=r,r,m")
        (match_operand:Narrow 1 "general_operand" "ri,m,r"))]
  "" { return subleq_output_narrow_move (operands, <MODE>mode); })

(define_code_iterator Extend [zero_extend sign_extend])
(define_code_attr extend [ (zero_extend "zero_extend") (sign_extend "extend") ])
(define_code_attr is_signed [(zero_extend "false") (sign_extend "true")])
(define_insn "<extend><mode>si2"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (Extend:SI (match_operand:Narrow 1 "nonimmediate_operand" "rm")))]
  "" { return subleq_output_extend (operands, <MODE>mode, <is_signed>); })

(define_expand "movsf"
  [(set (match_operand:SF 0 "nonimmediate_operand")
        (match_operand:SF 1 "general_operand"))]
  "" { subleq_expand_move (operands, SFmode); })

(define_insn "*movsf"
  [(set (match_operand:SF 0 "nonimmediate_operand" "=r,r,m")
        (match_operand:SF 1 "general_operand" "ri,m,r"))]
  "" { return subleq_output_move (operands); })

(define_mode_iterator Wide [DI DF])
(define_expand "mov<mode>"
  [(set (match_operand:Wide 0 "nonimmediate_operand")
        (match_operand:Wide 1 "general_operand"))]
  "" { subleq_expand_move (operands, <MODE>mode); })

(define_insn_and_split "*mov<mode>"
  [(set (match_operand:Wide 0 "nonimmediate_operand" "=r,&r,m")
        (match_operand:Wide 1 "general_operand" "ri,m,r"))]
  "" "#" "reload_completed" [(const_int 0)]
  { subleq_split_wide_move (operands); DONE; })

(define_code_iterator KernelOp [mult div udiv mod umod and ior xor ashift ashiftrt lshiftrt])
(define_code_attr optab [(mult "mul") (div "div") (udiv "udiv")
  (mod "mod") (umod "umod") (and "and") (ior "ior") (xor "xor")
  (ashift "ashl") (ashiftrt "ashr") (lshiftrt "lshr")])
(define_code_attr kernel_name [(mult "mul") (div "sdivrem") (udiv "udivrem")
  (mod "sdivrem") (umod "udivrem") (and "and") (ior "or") (xor "xor")
  (ashift "shl") (ashiftrt "sra") (lshiftrt "srl")])
(define_code_attr kernel_result [(mult "96") (div "100") (udiv "100")
  (mod "104") (umod "104") (and "96") (ior "96") (xor "96")
  (ashift "96") (ashiftrt "96") (lshiftrt "96")])
(define_insn "<optab>si3"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (KernelOp:SI (match_operand:SI 1 "register_operand" "r")
                     (match_operand:SI 2 "register_operand" "r")))]
  ""
  { return subleq_output_binary_helper (operands, "__subleq_<kernel_name>", <kernel_result>); })

(define_insn "negsi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (neg:SI (match_operand:SI 1 "register_operand" "r")))]
  "" ".word 12, 12, .+4\n\t.word %1, 12, .+4\n\t.word 160, 160, .+4\n\t.word 12, 160, .+4\n\t.word %0, %0, .+4\n\t.word 160, %0, .+4")

(define_insn "one_cmplsi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (not:SI (match_operand:SI 1 "register_operand" "r")))]
  "" ".word 12, 12, .+4\n\t.word %1, 12, .+4\n\t.word 160, 160, .+4\n\t.word 12, 160, .+4\n\t.word %0, %0, .+4\n\t.word 160, %0, .+4\n\t.word 156, %0, .+4")
