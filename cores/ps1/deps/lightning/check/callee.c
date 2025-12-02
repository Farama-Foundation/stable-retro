#include <lightning.h>
#include <stdio.h>

//#define DEBUG	1

jit_state_t	*_jit;

int
main(int argc, char *argv[])
{
    int			  off;
    void		(*code)(void);
    jit_node_t		 *jmp, *inner, *fail;

    init_jit(argv[0]);
    _jit = jit_new_state();

    jmp = jit_jmpi();

    /* Create a simple function that changes all available JIT_Vx */
    inner = jit_label();
    jit_prolog();
    for (off = JIT_R_NUM - 1; off >= 0; --off) {
	if (jit_callee_save_p(JIT_R(off)))
	    jit_movi(JIT_R(off), (off + 1) * 2);
    }
    for (off = JIT_V_NUM - 1; off >= 0; --off)
	jit_movi(JIT_V(off), -(off + 1));
    /* If fprs are callee save, also test them */
    for (off = JIT_F_NUM - 1; off >= 0; --off) {
	if (jit_callee_save_p(JIT_F(off)))
	    jit_movi_d(JIT_F(off), -(off + 1));
    }
    /* Add some noise as there might be some error in the stack frame and
     * a standard C function might clobber registers saved in the stack */
    jit_prepare();
    jit_pushargi((jit_word_t)stderr);
    jit_pushargi((jit_word_t)
		 "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
		 "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n");
    jit_ellipsis();
    jit_pushargi(1);
    jit_pushargi(2);
    jit_pushargi(3);
    jit_pushargi(4);
    jit_pushargi(5);
    jit_pushargi(6);
    jit_pushargi(7);
    jit_pushargi(8);
    jit_pushargi(9);
    jit_pushargi(10);
    jit_pushargi(11);
    jit_pushargi(12);
    jit_pushargi(13);
    jit_pushargi(14);
    jit_pushargi(15);
    jit_pushargi(16);
    jit_pushargi(17);
    jit_pushargi(18);
    jit_pushargi(19);
    jit_pushargi(20);
    jit_pushargi_d(1);
    jit_pushargi_d(2);
    jit_pushargi_d(3);
    jit_pushargi_d(4);
    jit_pushargi_d(5);
    jit_pushargi_d(6);
    jit_pushargi_d(7);
    jit_pushargi_d(8);
    jit_pushargi_d(9);
    jit_pushargi_d(10);
    jit_pushargi_d(11);
    jit_pushargi_d(12);
    jit_pushargi_d(13);
    jit_pushargi_d(14);
    jit_pushargi_d(15);
    jit_pushargi_d(16);
    jit_pushargi_d(17);
    jit_pushargi_d(18);
    jit_pushargi_d(19);
    jit_pushargi_d(20);
    jit_finishi(fprintf);
    jit_ret();
    jit_epilog();

    jit_patch(jmp);
    jit_prolog();

    for (off = JIT_R_NUM - 1; off >= 0; --off) {
	if (jit_callee_save_p(JIT_R(off)))
	    jit_movi(JIT_R(off), -(off + 1) * 2);
    }
    for (off = JIT_V_NUM - 1; off >= 0; --off)
	jit_movi(JIT_V(off), 0x7fffffff - (off + 1));
    /* If fprs are callee save, also test them */
    for (off = JIT_F_NUM - 1; off >= 0; --off) {
	if (jit_callee_save_p(JIT_F(off)))
	    jit_movi_d(JIT_F(off), 0x7fffffff - (off + 1));
    }
    jit_patch_at(jit_calli(NULL), inner);

    /* Now validate no register has been clobbered */
    fail = jit_forward();

    for (off = JIT_R_NUM - 1; off >= 0; --off) {
	if (jit_callee_save_p(JIT_R(off))) {
#if DEBUG
	    jmp = jit_beqi(JIT_R(off), -(off + 1) * 2);
	    jit_calli(abort);
	    jit_patch(jmp);
#else
	    jit_patch_at(jit_bnei(JIT_R(off), -(off + 1) * 2), fail);
#endif
	}
    }
    for (off = JIT_V_NUM - 1; off >= 0; --off) {
#if DEBUG
	jmp = jit_beqi(JIT_V(off), 0x7fffffff - (off + 1));
	jit_calli(abort);
	jit_patch(jmp);
#else
	jit_patch_at(jit_bnei(JIT_V(off), 0x7fffffff - (off + 1)), fail);
#endif
    }
    for (off = JIT_F_NUM - 1; off >= 0; --off) {
	if (jit_callee_save_p(JIT_F(off))) {
#if DEBUG
	    jmp = jit_beqi_d(JIT_F(off), 0x7fffffff - (off + 1));
	    jit_calli(abort);
	    jit_patch(jmp);
#else
	    jit_patch_at(jit_bnei_d(JIT_F(off), 0x7fffffff - (off + 1)), fail);
#endif
	}
    }
#if !DEBUG
    /* Done if passed all tests */
    jmp = jit_jmpi();
    /* Where to land if there was any register clobber */
    jit_link(fail);
    jit_calli(abort);
    /* done */
    jit_patch(jmp);
#endif

    jit_ret();
    jit_epilog();

    code = jit_emit();
    jit_clear_state();

    (*code)();

    jit_destroy_state();
    finish_jit();
    return (0);
}
