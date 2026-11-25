/* SPDX-License-Identifier: GPL-3.0-or-later */
extern bool subleq_base_register_p (int);
#ifdef RTX_CODE
extern void subleq_expand_prologue (void);
extern void subleq_expand_epilogue (void);
extern void subleq_expand_move (rtx *, machine_mode);
extern const char *subleq_output_move (rtx *);
extern const char *subleq_output_add (rtx *, bool);
extern const char *subleq_output_call (rtx);
extern const char *subleq_output_branch (rtx *);
extern const char *subleq_output_cstore (rtx *);
extern const char *subleq_output_narrow_move (rtx *, machine_mode);
extern const char *subleq_output_extend (rtx *, machine_mode, bool);
extern const char *subleq_output_binary_helper (rtx *, const char *, int);
extern void subleq_split_wide_move (rtx *);
#endif
