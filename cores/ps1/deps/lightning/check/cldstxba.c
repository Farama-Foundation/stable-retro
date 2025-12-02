#include <lightning.h>
#include <stdio.h>

#if !defined(offsetof)
#  define offsetof(type, field) ((char *)&((type *)0)->field - (char *)0)
#endif

int
main(int argc, char *argv[])
{
    jit_state_t		*_jit;
    jit_node_t		*jmp, *fail;
    void		(*code)(void);
#if defined(__x86_64__) || defined(__i386__)
    /* test lodsb stosb special cases */
    struct data_t {
	signed char	sc;
	unsigned char	uc;
	signed short	ss;
	unsigned short	us;
	signed int	si;
	unsigned int	ui;
	unsigned long	ul;
    } data;


    init_jit(argv[0]);
    _jit = jit_new_state();
    jit_prolog();
    fail = jit_forward();

#define SC_VAL		-3
    jit_movi(_RDI, (jit_word_t)&data + offsetof(struct data_t, sc));
    jit_movi(_RAX, SC_VAL);
    jit_movr(_RSI, _RDI);
    jit_stxai_c(1, _RDI, _RAX);
    jit_subr(_RDI, _RDI, _RSI);
    jmp = jit_bnei(_RDI, 1);
    jit_patch_at(jmp, fail);
    data.uc = 0xa3;

#define SS_VAL		-31
    jit_movi(_RDI, (jit_word_t)&data + offsetof(struct data_t, ss));
    jit_movi(_RAX, SS_VAL);
    jit_movr(_RSI, _RDI);
    jit_stxai_s(2, _RDI, _RAX);
    jit_subr(_RDI, _RDI, _RSI);
    jmp = jit_bnei(_RDI, 2);
    jit_patch_at(jmp, fail);
    data.us = 0x5aa5;

#define SI_VAL		-511
    jit_movi(_RDI, (jit_word_t)&data + offsetof(struct data_t, si));
    jit_movi(_RAX, SI_VAL);
    jit_movr(_RSI, _RDI);
    jit_stxai_i(4, _RDI, _RAX);
    jit_subr(_RDI, _RDI, _RSI);
    jmp = jit_bnei(_RDI, 4);
    jit_patch_at(jmp, fail);
    data.ui = 0xabcddcba;

#  if __X64 && !__X64_32
#define UL_VAL		0x123456789abcdef
    jit_movi(_RDI, (jit_word_t)&data + offsetof(struct data_t, ul));
    jit_movi(_RAX, UL_VAL);
    jit_movr(_RSI, _RDI);
    jit_stxai_l(8, _RDI, _RAX);
    jit_subr(_RDI, _RDI, _RSI);
    jmp = jit_bnei(_RDI, 8);
    jit_patch_at(jmp, fail);
#  endif

    jit_movi(_RSI, (jit_word_t)&data + offsetof(struct data_t, sc));
    jit_movr(_RDI, _RSI);
    jit_ldxai_c(_RAX, _RSI, 1);
    jmp = jit_bnei(_RAX, SC_VAL);
    jit_patch_at(jmp, fail);
    jit_subr(_RDI, _RDI, _RSI);
    jmp = jit_bnei(_RDI, -1);
    jit_patch_at(jmp, fail);
    jit_movi(_RSI, (jit_word_t)&data + offsetof(struct data_t, uc));
    jit_movr(_RDI, _RSI);
    jit_ldxai_uc(_RAX, _RSI, 1);
    jmp = jit_bnei(_RAX, data.uc);
    jit_patch_at(jmp, fail);
    jit_subr(_RDI, _RDI, _RSI);
    jmp = jit_bnei(_RDI, -1);
    jit_patch_at(jmp, fail);
    jit_movi(_RSI, (jit_word_t)&data + offsetof(struct data_t, ss));
    jit_movr(_RDI, _RSI);
    jit_ldxai_s(_RAX, _RSI, 2);
    jmp = jit_bnei(_RAX, SS_VAL);
    jit_patch_at(jmp, fail);
    jit_subr(_RDI, _RDI, _RSI);
    jmp = jit_bnei(_RDI, -2);
    jit_patch_at(jmp, fail);
    jit_movi(_RSI, (jit_word_t)&data + offsetof(struct data_t, us));
    jit_movr(_RDI, _RSI);
    jit_ldxai_us(_RAX, _RSI, 2);
    jmp = jit_bnei(_RAX, data.us);
    jit_patch_at(jmp, fail);
    jit_subr(_RDI, _RDI, _RSI);
    jmp = jit_bnei(_RDI, -2);
    jit_patch_at(jmp, fail);
    jit_movi(_RSI, (jit_word_t)&data + offsetof(struct data_t, si));
    jit_movr(_RDI, _RSI);
    jit_ldxai_i(_RAX, _RSI, 4);
    jmp = jit_bnei(_RAX, SI_VAL);
    jit_patch_at(jmp, fail);
    jit_subr(_RDI, _RDI, _RSI);
    jmp = jit_bnei(_RDI, -4);
    jit_patch_at(jmp, fail);
#  if __X64 && !__X64_32
    jit_movi(_RSI, (jit_word_t)&data + offsetof(struct data_t, ui));
    jit_movr(_RDI, _RSI);
    jit_ldxai_ui(_RAX, _RSI, 4);
    jmp = jit_bnei(_RAX, data.ui);
    jit_patch_at(jmp, fail);
    jit_subr(_RDI, _RDI, _RSI);
    jmp = jit_bnei(_RDI, -4);
    jit_patch_at(jmp, fail);
    jit_movi(_RSI, (jit_word_t)&data + offsetof(struct data_t, ul));
    jit_movr(_RDI, _RSI);
    jit_ldxai_l(_RAX, _RSI, 8);
    jmp = jit_bnei(_RAX, UL_VAL);
    jit_patch_at(jmp, fail);
    jit_subr(_RDI, _RDI, _RSI);
    jmp = jit_bnei(_RDI, -8);
    jit_patch_at(jmp, fail);
#  endif

    jmp = jit_jmpi();
    jit_link(fail);
    jit_calli(abort);
    jit_patch(jmp);
    jit_prepare();
    {
	jit_pushargi((jit_word_t)"ok");
    }
    jit_finishi(puts);
    jit_ret();
    jit_epilog();
    code = jit_emit();
    jit_clear_state();

    (*code)();

    jit_destroy_state();
    finish_jit();

#elif defined(__arm__)
    /* make sure to test ldmia and stmia cases */
    struct data_t {
	float		f1;
	float		f2;
	double		d3;
	double		d4;
    } data;

    init_jit(argv[0]);
    _jit = jit_new_state();
    jit_prolog();
    fail = jit_forward();

#define F1_VAL		1
    jit_movi(JIT_R0, (jit_word_t)&data + offsetof(struct data_t, f1));
    jit_movi_f(JIT_F0, F1_VAL);
    jit_movr(JIT_R1, JIT_R0);
    jit_stxai_f(4, JIT_R0, JIT_F0);
    jit_subr(JIT_R1, JIT_R0, JIT_R1);
    jmp = jit_bnei(JIT_R1, 4);
    jit_patch_at(jmp, fail);
    data.f2 = 2;
#define D3_VAL		3
    jit_movi(JIT_R0, (jit_word_t)&data + offsetof(struct data_t, d3));
    jit_movi_d(JIT_F0, D3_VAL);
    jit_movr(JIT_R1, JIT_R0);
    jit_stxai_d(8, JIT_R0, JIT_F0);
    jit_subr(JIT_R1, JIT_R0, JIT_R1);
    jmp = jit_bnei(JIT_R1, 8);
    jit_patch_at(jmp, fail);
    data.d4 = 4;

    jit_movi(JIT_R0, (jit_word_t)&data + offsetof(struct data_t, f1));
    jit_movr(JIT_R1, JIT_R0);
    jit_ldxai_f(JIT_F0, JIT_R0, 4);
    jmp = jit_bnei_f(JIT_F0, F1_VAL);
    jit_patch_at(jmp, fail);
    jit_subr(JIT_R1, JIT_R0, JIT_R1);
    jmp = jit_bnei(JIT_R1, 4);
    jit_patch_at(jmp, fail);

    jit_movi(JIT_R0, (jit_word_t)&data + offsetof(struct data_t, d3));
    jit_movr(JIT_R1, JIT_R0);
    jit_ldxai_d(JIT_F0, JIT_R0, 8);
    jmp = jit_bnei_d(JIT_F0, D3_VAL);
    jit_patch_at(jmp, fail);
    jit_subr(JIT_R1, JIT_R0, JIT_R1);
    jmp = jit_bnei(JIT_R1, 8);
    jit_patch_at(jmp, fail);

    jmp = jit_jmpi();
    jit_link(fail);
    jit_calli(abort);
    jit_patch(jmp);
    jit_prepare();
    {
	jit_pushargi((jit_word_t)"ok");
    }
    jit_finishi(puts);
    jit_ret();
    jit_epilog();
    code = jit_emit();
    jit_clear_state();

    (*code)();

    jit_destroy_state();
    finish_jit();
#else
    puts("ok");
#endif
    return (0);
}
