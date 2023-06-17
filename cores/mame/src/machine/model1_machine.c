/*
** Model 1 coprocessor TGP simulation
*/

#include "driver.h"
#include "state.h"
#include "machine/random.h"
#include <math.h>

enum {FIFO_SIZE = 256};
enum {MAT_STACK_SIZE = 32};

int model1_dump;

static int fifoin_rpos, fifoin_wpos;
static UINT32 fifoin_data[FIFO_SIZE];
static int model1_swa;

static int fifoin_cbcount;
static void (*fifoin_cb)(void);

static int fifoout_rpos, fifoout_wpos;
static UINT32 fifoout_data[FIFO_SIZE];

static UINT32 list_length;

static float cmat[12], mat_stack[MAT_STACK_SIZE][12], mat_vector[21][12];
static int mat_stack_pos;
static float acc;

static float tgp_vf_xmin, tgp_vf_xmax, tgp_vf_zmin, tgp_vf_zmax, tgp_vf_ygnd, tgp_vf_yflr, tgp_vf_yjmp;
static float tgp_vr_circx, tgp_vr_circy, tgp_vr_circrad, tgp_vr_cbox[12];
static int tgp_vr_select;
static UINT16 ram_adr, ram_latch[2], ram_scanadr;
static UINT32 *ram_data;

static float u2f(UINT32 v)
{
	union {
		float ff;
		UINT32 vv;
	} u;
	u.vv = v;
	return u.ff;
}

static UINT32 f2u(float f)
{
	union {
		float ff;
		UINT32 vv;
	} u;
	u.ff = f;
	return u.vv;
}

static UINT32 fifoout_pop(void)
{
	UINT32 v;
	if(fifoout_wpos == fifoout_rpos) {
		logerror("TGP FIFOOUT underflow (%x)\n", activecpu_get_pc());
		exit(0);
	}
	v = fifoout_data[fifoout_rpos++];
	if(fifoout_rpos == FIFO_SIZE)
		fifoout_rpos = 0;
	return v;
}

static int puuu = 0;

static void fifoout_push(UINT32 data)
{
	if(!puuu)
		logerror("TGP: Push %d\n", data);
	else
		puuu = 0;
	fifoout_data[fifoout_wpos++] = data;
	if(fifoout_wpos == FIFO_SIZE)
		fifoout_wpos = 0;
	if(fifoout_wpos == fifoout_rpos)
		logerror("TGP FIFOOUT overflow\n");
}

static void fifoout_push_f(float data)
{
	puuu = 1;

	logerror("TGP: Push %f\n", data);
	fifoout_push(f2u(data));
}

static UINT32 fifoin_pop(void)
{
	UINT32 v;
	if(fifoin_wpos == fifoin_rpos)
		logerror("TGP FIFOIN underflow\n");
	v = fifoin_data[fifoin_rpos++];
	if(fifoin_rpos == FIFO_SIZE)
		fifoin_rpos = 0;
	return v;
}

static void fifoin_push(UINT32 data)
{
	/*	logerror("TGP FIFOIN write %08x (%x)\n", data, activecpu_get_pc()); */
	fifoin_data[fifoin_wpos++] = data;
	if(fifoin_wpos == FIFO_SIZE)
		fifoin_wpos = 0;
	if(fifoin_wpos == fifoin_rpos)
		logerror("TGP FIFOIN overflow\n");
	fifoin_cbcount--;
	if(!fifoin_cbcount)
		fifoin_cb();
}

static float fifoin_pop_f(void)
{
	return u2f(fifoin_pop());
}

static void function_get_vf(void);
static void function_get_swa(void);

static void next_fn(void)
{
	fifoin_cbcount = 1;
	fifoin_cb = model1_swa ? function_get_swa : function_get_vf;
}

static float tcos(INT16 a)
{
	if(a == 16384 || a == -16384)
		return 0;
	else if(a == -32768)
		return -1;
	else if(a == 0)
		return 1;
	else
		return cos(a*(2*M_PI/65536.0));
}

static float tsin(INT16 a)
{
	if(a == 0 || a == -32768)
		return 0;
	else if(a == 16384)
		return 1;
	else if(a == -16384)
		return -1;
	else
		return sin(a*(2*M_PI/65536.0));
}

static UINT16 ram_get_i(void)
{
	return ram_data[ram_scanadr++];
}

static float ram_get_f(void)
{
	return u2f(ram_data[ram_scanadr++]);
}

static void fadd(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float r = a+b;
	logerror("TGP fadd %f+%f=%f (%x)\n", a, b, r, activecpu_get_pc());
	fifoout_push_f(r);
	next_fn();
}

static void fsub(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float r = a-b;
	model1_dump = 1;
	logerror("TGP fsub %f-%f=%f (%x)\n", a, b, r, activecpu_get_pc());
	fifoout_push_f(r);
	next_fn();
}

static void fmul(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float r = a*b;
	logerror("TGP fmul %f*%f=%f (%x)\n", a, b, r, activecpu_get_pc());
	fifoout_push_f(r);
	next_fn();
}

static void fdiv(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float r = !b ? 1e39 : a/b;
	logerror("TGP fdiv %f/%f=%f (%x)\n", a, b, r, activecpu_get_pc());
	fifoout_push_f(r);
	next_fn();
}

static void matrix_push(void)
{
	if(mat_stack_pos != MAT_STACK_SIZE) {
		memcpy(mat_stack[mat_stack_pos], cmat, sizeof(cmat));
		mat_stack_pos++;
	}
	logerror("TGP matrix_push (depth=%d, pc=%x)\n", mat_stack_pos, activecpu_get_pc());
	next_fn();
}

static void matrix_pop(void)
{
	if(mat_stack_pos) {
		mat_stack_pos--;
		memcpy(cmat, mat_stack[mat_stack_pos], sizeof(cmat));
	}
	logerror("TGP matrix_pop (depth=%d, pc=%x)\n", mat_stack_pos, activecpu_get_pc());
	next_fn();
}

static void matrix_write(void)
{
	int i;
	for(i=0; i<12; i++)
		cmat[i] = fifoin_pop_f();
	logerror("TGP matrix_write %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f) (%x)\n",
			 cmat[0], cmat[1], cmat[2], cmat[3], cmat[4], cmat[5], cmat[6], cmat[7], cmat[8], cmat[9], cmat[10], cmat[11],
			 activecpu_get_pc());
	next_fn();
}

static void clear_stack(void)
{
	logerror("TGP clear_stack (%x)\n", activecpu_get_pc());
	mat_stack_pos = 0;
	next_fn();
}

static void matrix_mul(void)
{
	float m[12];
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	float g = fifoin_pop_f();
	float h = fifoin_pop_f();
	float i = fifoin_pop_f();
	float j = fifoin_pop_f();
	float k = fifoin_pop_f();
	float l = fifoin_pop_f();
	logerror("TGP matrix_mul %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f (%x)\n", a, b, c, d, e, f, g, h, i, j, k, l, activecpu_get_pc());
	m[0]  = a*cmat[0] + b*cmat[3] + c*cmat[6];
	m[1]  = a*cmat[1] + b*cmat[4] + c*cmat[7];
	m[2]  = a*cmat[2] + b*cmat[5] + c*cmat[8];
	m[3]  = d*cmat[0] + e*cmat[3] + f*cmat[6];
	m[4]  = d*cmat[1] + e*cmat[4] + f*cmat[7];
	m[5]  = d*cmat[2] + e*cmat[5] + f*cmat[8];
	m[6]  = g*cmat[0] + h*cmat[3] + i*cmat[6];
	m[7]  = g*cmat[1] + h*cmat[4] + i*cmat[7];
	m[8]  = g*cmat[2] + h*cmat[5] + i*cmat[8];
	m[9]  = j*cmat[0] + k*cmat[3] + l*cmat[6] + cmat[9];
	m[10] = j*cmat[1] + k*cmat[4] + l*cmat[7] + cmat[10];
	m[11] = j*cmat[2] + k*cmat[5] + l*cmat[8] + cmat[11];

	memcpy(cmat, m, sizeof(m));
	next_fn();
}

static void anglev(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	logerror("TGP anglev %f, %f (%x)\n", a, b, activecpu_get_pc());
	if(!b) {
		if(a>=0)
			fifoout_push(0);
		else
			fifoout_push(-32768);
	} else if(!a) {
		if(b>=0)
			fifoout_push(16384);
		else
			fifoout_push(-16384);
	} else
		fifoout_push((INT16)(atan2(b, a)*32768/M_PI));
	next_fn();
}

static void f11(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	float g = fifoin_pop_f();
	float h = fifoin_pop_f();
	float i = fifoin_pop_f();
	logerror("TGP f11 %f, %f, %f, %f, %f, %f, %f, %f, %f (%x)\n", a, b, c, d, e, f, g, h, i, activecpu_get_pc());
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	next_fn();
}

static void normalize(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float n = sqrt(a*a+b*b+c*c);
	logerror("TGP normalize %f, %f, %f (%x)\n", a, b, c, activecpu_get_pc());
	fifoout_push_f(a/n);
	fifoout_push_f(b/n);
	fifoout_push_f(c/n);
	next_fn();
}

static void acc_seti(void)
{
	INT32 a = fifoin_pop();
	model1_dump = 1;
	logerror("TGP acc_seti %d (%x)\n", a, activecpu_get_pc());
	acc = a;
	next_fn();
}

static void track_select(void)
{
	INT32 a = fifoin_pop();
	logerror("TGP track_select %d (%x)\n", a, activecpu_get_pc());
	tgp_vr_select = a;
	next_fn();
}

static void f14(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	int r = 0;
	logerror("TGP f14 %f, %f, %f, %f (%x)\n", a, b, c, d, activecpu_get_pc());
	/*	fprintf(stderr, "TGP f14 %f, %f, %f, %f (%x)\n", a, b, c, d, activecpu_get_pc()); */

	switch(activecpu_get_pc()) {
	case 0xc7eea: r=1; break; /* stars */
	case 0xc7f19: r=2; break; /* stars too, ties? */
	case 0xcac91: r=3; break; /* visee??? */
	case 0xc2ad0: r=4; break;
	}

	if(r == 4) {
		cmat[9] += mame_rand();
		cmat[10] += mame_rand();
		cmat[11] += mame_rand();
	}

	next_fn();
}

static void f15_swa(void)
{
	logerror("TGP f15_swa (%x)\n", activecpu_get_pc());
	/*	fprintf(stderr, "TGP f15_swa (%x)\n", activecpu_get_pc()); */

	next_fn();
}

static void anglep(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	logerror("TGP anglep %f, %f, %f, %f (%x)\n", a, b, c, d, activecpu_get_pc());
	c = a - c;
	d = b - d;
	if(!d) {
		if(c>=0)
			fifoout_push(0);
		else
			fifoout_push(-32768);
	} else if(!c) {
		if(d>=0)
			fifoout_push(16384);
		else
			fifoout_push(-16384);
	} else
		fifoout_push((INT16)(atan2(d, c)*32768/M_PI));
	next_fn();
}

static void matrix_ident(void)
{
	logerror("TGP matrix_ident (%x)\n", activecpu_get_pc());
	memset(cmat, 0, sizeof(cmat));
	cmat[0] = 1.0;
	cmat[4] = 1.0;
	cmat[8] = 1.0;
	next_fn();
}

static void matrix_read(void)
{
	int i;
	logerror("TGP matrix_read (%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f) (%x)\n",
			 cmat[0], cmat[1], cmat[2], cmat[3], cmat[4], cmat[5], cmat[6], cmat[7], cmat[8], cmat[9], cmat[10], cmat[11], activecpu_get_pc());
	for(i=0; i<12; i++)
		fifoout_push_f(cmat[i]);
	next_fn();
}

static void matrix_trans(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();

	logerror("TGP matrix_trans (%f, %f, %f) (%x)\n", a, b, c, activecpu_get_pc());
	if(0 && activecpu_get_pc() == 0xfeeaa5)
		/*		fprintf(stderr, "TGP matrix_trans (%f, %f, %f) (%x)\n", a, b, c, activecpu_get_pc()); */
		a = 0;

	if(activecpu_get_pc() == 0x22f59) {
		next_fn();
		return;
	}

	cmat[ 9] += cmat[0]*a+cmat[3]*b+cmat[6]*c;
	cmat[10] += cmat[1]*a+cmat[4]*b+cmat[7]*c;
	cmat[11] += cmat[2]*a+cmat[5]*b+cmat[8]*c;
	next_fn();
}

static void matrix_scale(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	logerror("TGP matrix_scale %f, %f, %f (%x)\n", a, b, c, activecpu_get_pc());
	cmat[0] *= a;
	cmat[1] *= a;
	cmat[2] *= a;
	cmat[3] *= b;
	cmat[4] *= b;
	cmat[5] *= b;
	cmat[6] *= c;
	cmat[7] *= c;
	cmat[8] *= c;
	next_fn();
}

static void matrix_rotx(void)
{
	INT16 a = fifoin_pop();
	double s = tsin(a);
	double c = tcos(a);
	double t1, t2;
	logerror("TGP matrix_rotx %d (%x)\n", a, activecpu_get_pc());
	t1 = cmat[3];
	t2 = cmat[6];
	cmat[3] = c*t1-s*t2;
	cmat[6] = s*t1+c*t2;
	t1 = cmat[4];
	t2 = cmat[7];
	cmat[4] = c*t1-s*t2;
	cmat[7] = s*t1+c*t2;
	t1 = cmat[5];
	t2 = cmat[8];
	cmat[5] = c*t1-s*t2;
	cmat[8] = s*t1+c*t2;
	next_fn();
}

static void matrix_roty(void)
{
	INT16 a = fifoin_pop();
	double s = tsin(a);
	double c = tcos(a);
	double t1, t2;

	logerror("TGP matrix_roty %d (%x)\n", a, activecpu_get_pc());
	t1 = cmat[6];
	t2 = cmat[0];
	cmat[6] = c*t1-s*t2;
	cmat[0] = s*t1+c*t2;
	t1 = cmat[7];
	t2 = cmat[1];
	cmat[7] = c*t1-s*t2;
	cmat[1] = s*t1+c*t2;
	t1 = cmat[8];
	t2 = cmat[2];
	cmat[8] = c*t1-s*t2;
	cmat[2] = s*t1+c*t2;
	next_fn();
}

static void matrix_rotz(void)
{
	INT16 a = fifoin_pop();
	double s = tsin(a);
	double c = tcos(a);
	double t1, t2;

	logerror("TGP matrix_rotz %d (%x)\n", a, activecpu_get_pc());
	t1 = cmat[0];
	t2 = cmat[3];
	cmat[0] = c*t1-s*t2;
	cmat[3] = s*t1+c*t2;
	t1 = cmat[1];
	t2 = cmat[4];
	cmat[1] = c*t1-s*t2;
	cmat[4] = s*t1+c*t2;
	t1 = cmat[2];
	t2 = cmat[5];
	cmat[2] = c*t1-s*t2;
	cmat[5] = s*t1+c*t2;
	next_fn();
}

static void f24(void)
{
	UINT32 a = fifoin_pop();
	logerror("TGP f24 %d (%x)\n", a, activecpu_get_pc());
	fifoout_push_f(1);
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(1);
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(1);
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	next_fn();
}

static void f24_swa(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	UINT32 g = fifoin_pop();
	logerror("TGP f24_swa %f, %f, %f, %f, %f, %f, %x (%x)\n", a, b, c, d, e, f, g, activecpu_get_pc());
	fifoout_push_f(0);
	next_fn();
}

static void transform_point(void)
{
	float x = fifoin_pop_f();
	float y = fifoin_pop_f();
	float z = fifoin_pop_f();
	logerror("TGP transform_point %f, %f, %f (%x)\n", x, y, z, activecpu_get_pc());
	fifoout_push_f(cmat[0]*x+cmat[3]*y+cmat[6]*z+cmat[9]);
	fifoout_push_f(cmat[1]*x+cmat[4]*y+cmat[7]*z+cmat[10]);
	fifoout_push_f(cmat[2]*x+cmat[5]*y+cmat[8]*z+cmat[11]);
	next_fn();
}

static void fcos(void)
{
    INT16 a = fifoin_pop();
	logerror("TGP fcos %d (%x)\n", a, activecpu_get_pc());
	fifoout_push_f(tcos(a));
	next_fn();
}

static void fsin(void)
{
    INT16 a = fifoin_pop();
	logerror("TGP fsin %d (%x)\n", a, activecpu_get_pc());
	fifoout_push_f(tsin(a));
	next_fn();
}

static void fcosm(void)
{
    INT16 a = fifoin_pop();
	float b = fifoin_pop_f();
	logerror("TGP fcosm %d, %f (%x)\n", a, b, activecpu_get_pc());
	fifoout_push_f(b*tcos(a));
	next_fn();
}

static void fsinm(void)
{
    INT16 a = fifoin_pop();
	float b = fifoin_pop_f();
	model1_dump = 1;
	logerror("TGP fsinm %d, %f (%x)\n", a, b, activecpu_get_pc());
	fifoout_push_f(b*tsin(a));
	next_fn();
}

static void distance3(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	logerror("TGP distance3 (%f, %f, %f), (%f, %f, %f) (%x)\n", a, b, c, d, e, f, activecpu_get_pc());
	a -= d;
	b -= e;
	c -= f;
	fifoout_push_f(sqrt(a*a+b*b+c*c));
	next_fn();
}

static void ftoi(void)
{
	float a = fifoin_pop_f();
	logerror("TGP ftoi %f (%x)\n", a, activecpu_get_pc());
	fifoout_push((int)a);
	next_fn();
}

static void itof(void)
{
	INT32 a = fifoin_pop();
	logerror("TGP itof %d (%x)\n", a, activecpu_get_pc());
	fifoout_push_f(a);
	next_fn();
}

static void acc_set(void)
{
	float a = fifoin_pop_f();
	logerror("TGP acc_set %f (%x)\n", a, activecpu_get_pc());
	acc = a;
	next_fn();
}

static void acc_get(void)
{
	logerror("TGP acc_get (%x)\n", activecpu_get_pc());
	fifoout_push_f(acc);
	next_fn();
}

static void acc_add(void)
{
	float a = fifoin_pop_f();
	logerror("TGP acc_add %f (%x)\n", a, activecpu_get_pc());
	acc += a;
	next_fn();
}

static void acc_sub(void)
{
	float a = fifoin_pop_f();
	logerror("TGP acc_sub %f (%x)\n", a, activecpu_get_pc());
	acc -= a;
	next_fn();
}

static void acc_mul(void)
{
	float a = fifoin_pop_f();
	logerror("TGP acc_mul %f (%x)\n", a, activecpu_get_pc());
	acc *= a;
	next_fn();
}

static void acc_div(void)
{
	float a = fifoin_pop_f();
	logerror("TGP acc_div %f (%x)\n", a, activecpu_get_pc());
	acc /= a;
	next_fn();
}

static void f42(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	logerror("TGP f42 %f, %f, %f (%x)\n", a, b, c, activecpu_get_pc());
	/*	fifoout_push_f((mame_rand() % 1000) - 500); */
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	next_fn();
}


/* r = (x2 + y2 + z2)1/2,     f = tan-1(y/(x2+z2)1/2),     q = tan-1(z/x) */

static void xyz2rqf(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float norm;
	logerror("TGP xyz2rqf %f, %f, %f (%x)\n", a, b, c, activecpu_get_pc());
	fifoout_push_f(sqrt(a*a+b*b+c*c));
	norm = sqrt(a*a+c*c);
	if(!c) {
		if(a>=0)
			fifoout_push(0);
		else
			fifoout_push(-32768);
	} else if(!a) {
		if(c>=0)
			fifoout_push(16384);
		else
			fifoout_push(-16384);
	} else
		fifoout_push((INT16)(atan2(c, a)*32768/M_PI));

	if(!b)
		fifoout_push(0);
	else if(!norm) {
		if(b>=0)
			fifoout_push(16384);
		else
			fifoout_push(-16384);
	} else
		fifoout_push((INT16)(atan2(b, norm)*32768/M_PI));

	next_fn();
}

static void f43(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	logerror("TGP f43 %f, %f, %f, %f, %f, %f (%x)\n", a, b, c, d, e, f, activecpu_get_pc());
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	next_fn();
}

static void f43_swa(void)
{
	float a = fifoin_pop_f();
	int b = fifoin_pop();
	int c = fifoin_pop();
	logerror("TGP f43_swa %f, %d, %d (%x)\n", a, b, c, activecpu_get_pc());
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	next_fn();
}

static void f44(void)
{
	float a = fifoin_pop_f();
	logerror("TGP f44 %f (%x)\n", a, activecpu_get_pc());
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	next_fn();
}

static void matrix_sdir(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float norm = sqrt(a*a+b*b+c*c);
	float t[9], m[9];
	logerror("TGP matrix_sdir %f, %f, %f (%x)\n", a, b, c, activecpu_get_pc());

	memset(t, 0, sizeof(t));

	if(!norm) {
		memset(t, 0, 9*sizeof(t[0]));
		t[0] = 1.0;
		t[4] = 1.0;
		t[8] = 1.0;
	} else {
		t[0] = -c / norm;
		t[1] = b / norm;
		t[2] = a / norm;

		norm = sqrt(a*a+c*c);
		t[6] = a/norm;
		t[7] = 0;
		t[8] = c/norm;

		t[3] = -b*c;
		t[4] = a*a+c*c;
		t[5] = -b*a;
		norm = sqrt(t[3]*t[3]+t[4]*t[4]+t[5]*t[5]);
		t[3] /= norm;
		t[4] /= norm;
		t[5] /= norm;
	}

	m[0]  = t[0]*cmat[0] + t[1]*cmat[3] + t[2]*cmat[6];
	m[1]  = t[0]*cmat[1] + t[1]*cmat[4] + t[2]*cmat[7];
	m[2]  = t[0]*cmat[2] + t[1]*cmat[5] + t[2]*cmat[8];
	m[3]  = t[3]*cmat[0] + t[4]*cmat[3] + t[5]*cmat[6];
	m[4]  = t[3]*cmat[1] + t[4]*cmat[4] + t[5]*cmat[7];
	m[5]  = t[3]*cmat[2] + t[4]*cmat[5] + t[5]*cmat[8];
	m[6]  = t[6]*cmat[0] + t[7]*cmat[3] + t[8]*cmat[6];
	m[7]  = t[6]*cmat[1] + t[7]*cmat[4] + t[8]*cmat[7];
	m[8]  = t[6]*cmat[2] + t[7]*cmat[5] + t[8]*cmat[8];

	memcpy(cmat, m, sizeof(m));

	next_fn();
}

static void f45(void)
{
	float a = fifoin_pop_f();
	logerror("TGP f45 %f (%x)\n", a, activecpu_get_pc());
	fifoout_push_f(0);
	next_fn();
}

static void vlength(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	logerror("TGP vlength %f, %f, %f (%x)\n", a, b, c, activecpu_get_pc());
	fifoout_push_f(sqrt(a*a+b*b+c*c));
	next_fn();
}

static void f47(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	logerror("TGP f47 %f, %f, %f (%x)\n", a, b, c, activecpu_get_pc());
	fifoout_push_f(a+c);
	fifoout_push_f(b+c);
	next_fn();
}

static void f48(void)
{
    INT16 a = fifoin_pop();
	logerror("TGP f48 %d (%x)\n", a, activecpu_get_pc());
	fifoout_push(1000+a);
	next_fn();
}

static void colbox_set(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	float g = fifoin_pop_f();
	float h = fifoin_pop_f();
	float i = fifoin_pop_f();
	float j = fifoin_pop_f();
	float k = fifoin_pop_f();
	float l = fifoin_pop_f();
	logerror("TGP colbox_set %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f (%x)\n", a, b, c, d, e, f, g, h, i, j, k, l, activecpu_get_pc());
	tgp_vr_cbox[ 0] = a;
	tgp_vr_cbox[ 1] = b;
	tgp_vr_cbox[ 2] = c;
	tgp_vr_cbox[ 3] = d;
	tgp_vr_cbox[ 4] = e;
	tgp_vr_cbox[ 5] = f;
	tgp_vr_cbox[ 6] = g;
	tgp_vr_cbox[ 7] = h;
	tgp_vr_cbox[ 8] = i;
	tgp_vr_cbox[ 9] = j;
	tgp_vr_cbox[10] = k;
	tgp_vr_cbox[11] = l;
	next_fn();
}

static void colbox_test(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	logerror("TGP colbox_test %f, %f, %f (%x)\n", a, b, c, activecpu_get_pc());

	/* #### Wrong, need to check with the tgp_vr_cbox coordinates */
	/* Game only test sign, negative = collision */
	fifoout_push_f(-1);
	next_fn();
}

static void f49_swa(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	logerror("TGP f49_swa %f, %f, %f, %f, %f, %f (%x)\n", a, b, c, d, e, f, activecpu_get_pc());
	next_fn();
}

static void f50_swa(void)
{
    float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	logerror("TGP f50_swa %f, %f, %f, %f (%x)\n", a, b, c, d, activecpu_get_pc());
	fifoout_push_f(d);
	next_fn();
}

static void f52(void)
{
	logerror("TGP f52 (%x)\n", activecpu_get_pc());
	next_fn();
}

static void matrix_rdir(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float norm = sqrt(a*a+c*c);
	float t1, t2;

	logerror("TGP matrix_rdir %f, %f, %f (%x)\n", a, b, c, activecpu_get_pc());

	if(!norm) {
		c = 1;
		a = 0;
	} else {
		c /= norm;
		a /= norm;
	}

	t1 = cmat[6];
	t2 = cmat[0];
	cmat[6] = c*t1-a*t2;
	cmat[0] = a*t1+c*t2;
	t1 = cmat[7];
	t2 = cmat[1];
	cmat[7] = c*t1-a*t2;
	cmat[1] = a*t1+c*t2;
	t1 = cmat[8];
	t2 = cmat[2];
	cmat[8] = c*t1-a*t2;
	cmat[2] = a*t1+c*t2;
	next_fn();
}

static void track_lookup(void)
{
	const UINT32 *tgp_data = (const UINT32 *)memory_region(REGION_USER2);
	float a = fifoin_pop_f();
	UINT32 b = fifoin_pop();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	int offi, offd, len;
	float dist;
	int offe=0, pt=0, i;
	unsigned int behaviour;
	float height;

	logerror("TGP track_lookup %f, 0x%x, %f, %f (%x)\n", a, b, c, d, activecpu_get_pc());

	offi = tgp_data[0x10+tgp_vr_select] + b;
	offd = tgp_data[0x20+tgp_vr_select];

	len = tgp_data[offi++];

	dist = -1;
	if(len>20)
		len = 0;

	for(i=0; i<len; i++) {
		int j;
		int posd = offd + tgp_data[offi++]*0x10;
		const float *pts = (const float *)(tgp_data+posd);
		for(j=0; j<5; j++) {
			float dx = c-pts[3*j+0];
			float dy = a-pts[3*j+1];
			float dz = d-pts[3*j+2];
			float dd = dx*dx+dy*dy+dz*dz;
			if(dist == -1 || dd<dist) {
				dist = dd;
				offe = posd;
				pt = j;
			}
		}
	}

	if(dist == -1) {
		behaviour = 0;
		height = 0.0;
	} else {
		/* Maybe it's doing some kind of interpolation, go figure */
		behaviour = tgp_data[offe+15];
		height = u2f(tgp_data[offe+pt*3+1]);
	}

	ram_data[0x0000] = 0; /* non zero = still computing */
	ram_data[0x8001] = f2u(height);
	ram_data[0x8002] = behaviour;

	next_fn();
}

static void f56(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	UINT32 g = fifoin_pop();

	logerror("TGP f56 %f, %f, %f, %f, %f, %f, %d (%x)\n", a, b, c, d, e, f, g, activecpu_get_pc());
	fifoout_push(0);
	next_fn();
}

static void f57(void)
{
	logerror("TGP f57 (%x)\n", activecpu_get_pc());
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	next_fn();
}

static void matrix_readt(void)
{
	logerror("TGP matrix_readt (%x)\n", activecpu_get_pc());
	fifoout_push_f(cmat[9]);
	fifoout_push_f(cmat[10]);
	fifoout_push_f(cmat[11]);
	next_fn();
}

static void acc_geti(void)
{
	logerror("TGP acc_geti (%x)\n", activecpu_get_pc());
	fifoout_push((int)acc);
	next_fn();
}

static void f60(void)
{
	logerror("TGP f60 (%x)\n", activecpu_get_pc());
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	next_fn();
}

static void col_setcirc(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	logerror("TGP col_setcirc %f, %f, %f (%x)\n", a, b, c, activecpu_get_pc());
	tgp_vr_circx = a;
	tgp_vr_circy = b;
	tgp_vr_circrad = c;
	next_fn();
}

static void col_testpt(void)
{
	float x, y;
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	logerror("TGP col_testpt %f, %f (%x)\n", a, b, activecpu_get_pc());
	x = a - tgp_vr_circx;
	y = b - tgp_vr_circy;
	fifoout_push_f(sqrt(x*x+y*y) - tgp_vr_circrad);
	next_fn();
}

static void push_and_ident(void)
{
	if(mat_stack_pos != MAT_STACK_SIZE) {
		memcpy(mat_stack[mat_stack_pos], cmat, sizeof(cmat));
		mat_stack_pos++;
	}
	logerror("TGP push_and_ident (depth=%d, pc=%x)\n", mat_stack_pos, activecpu_get_pc());
	memset(cmat, 0, sizeof(cmat));
	cmat[0] = 1.0;
	cmat[4] = 1.0;
	cmat[8] = 1.0;
	next_fn();
}

void tgp_tick(void)
{
}

static void catmull_rom(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	float g = fifoin_pop_f();
	float h = fifoin_pop_f();
	float i = fifoin_pop_f();
	float j = fifoin_pop_f();
	float k = fifoin_pop_f();
	float l = fifoin_pop_f();
	float m = fifoin_pop_f();
	float m2, m3;
	float w1, w2, w3, w4;

	logerror("TGP catmull_rom %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f (%x)\n", a, b, c, d, e, f, g, h, i, j, k, l, m, activecpu_get_pc());

	m2 = m*m;
	m3 = m*m*m;

	w1 = 0.5*(-m3+2*m2-m);
	w2 = 0.5*(3*m3-5*m2+2);
	w3 = 0.5*(-3*m3+4*m2+m);
	w4 = 0.5*(m3-m2);

	fifoout_push_f(a*w1+d*w2+g*w3+j*w4);
	fifoout_push_f(b*w1+e*w2+h*w3+k*w4);
	fifoout_push_f(c*w1+f*w2+i*w3+l*w4);
	next_fn();
}

static void distance(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	logerror("TGP distance (%f, %f), (%f, %f) (%x)\n", a, b, c, d, activecpu_get_pc());
	c -= a;
	d -= b;
	fifoout_push_f(sqrt(c*c+d*d));
	next_fn();
}

static void car_move(void)
{
	INT16 a = fifoin_pop();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float dx, dy;
	logerror("TGP car_move (%d, %f), (%f, %f) (%x)\n", a, b, c, d, activecpu_get_pc());

	dx = b*tsin(a);
	dy = b*tcos(a);

	fifoout_push_f(dx);
	fifoout_push_f(dy);
	fifoout_push_f(c+dx);
	fifoout_push_f(d+dy);
	next_fn();
}

static void cpa(void)
{
	float dv_x, dv_y, dv_z, dv2, dw_x, dw_y, dw_z, dt;

	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	float g = fifoin_pop_f();
	float h = fifoin_pop_f();
	float i = fifoin_pop_f();
	float j = fifoin_pop_f();
	float k = fifoin_pop_f();
	float l = fifoin_pop_f();
	logerror("TGP cpa %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f (%x)\n", a, b, c, d, e, f, g, h, i, j, k, l, activecpu_get_pc());

	dv_x = (b-a) - (d-c);
	dv_y = (f-e) - (h-g);
	dv_z = (j-i) - (l-k);
	dv2 = dv_x*dv_x + dv_y*dv_y + dv_z*dv_z;
	if(dv2 < 0.001)
		dt = 0;
	else {
		dw_x = a-c;
		dw_y = e-g;
		dw_z = i-k;
		dt = -(dw_x*dv_x + dw_y*dv_y + dw_z*dv_z)/dv2;
	}
	if(dt < 0)
		dt = 0;
	else if(dt > 1.0)
		dt = 1.0;

	dv_x = (a-c)*(1-dt) + (b-d)*dt;
	dv_y = (e-g)*(1-dt) + (f-h)*dt;
	dv_z = (i-k)*(1-dt) + (j-l)*dt;
	dv2 = dv_x*dv_x + dv_y*dv_y + dv_z*dv_z;

	fifoout_push_f(sqrt(dv2));
	next_fn();
}

static void vmat_store(void)
{
	UINT32 a = fifoin_pop();
	if(a<21)
		memcpy(mat_vector[a], cmat, sizeof(cmat));
	else
		logerror("TGP ERROR bad vector index\n");
	logerror("TGP vmat_store %d (%x)\n", a, activecpu_get_pc());
	next_fn();
}

static void vmat_restore(void)
{
	UINT32 a = fifoin_pop();
	if(a<21)
		memcpy(cmat, mat_vector[a], sizeof(cmat));
	else
		logerror("TGP ERROR bad vector index\n");
	logerror("TGP vmat_restore %d (%x)\n", a, activecpu_get_pc());
	next_fn();
}

static void vmat_mul(void)
{
	UINT32 a = fifoin_pop();
	UINT32 b = fifoin_pop();
	if(a<21 && b<21) {
		mat_vector[b][0]  = mat_vector[a][ 0]*cmat[0] + mat_vector[a][ 1]*cmat[3] + mat_vector[a][ 2]*cmat[6];
		mat_vector[b][1]  = mat_vector[a][ 0]*cmat[1] + mat_vector[a][ 1]*cmat[4] + mat_vector[a][ 2]*cmat[7];
		mat_vector[b][2]  = mat_vector[a][ 0]*cmat[2] + mat_vector[a][ 1]*cmat[5] + mat_vector[a][ 2]*cmat[8];
		mat_vector[b][3]  = mat_vector[a][ 3]*cmat[0] + mat_vector[a][ 4]*cmat[3] + mat_vector[a][ 5]*cmat[6];
		mat_vector[b][4]  = mat_vector[a][ 3]*cmat[1] + mat_vector[a][ 4]*cmat[4] + mat_vector[a][ 5]*cmat[7];
		mat_vector[b][5]  = mat_vector[a][ 3]*cmat[2] + mat_vector[a][ 4]*cmat[5] + mat_vector[a][ 5]*cmat[8];
		mat_vector[b][6]  = mat_vector[a][ 6]*cmat[0] + mat_vector[a][ 7]*cmat[3] + mat_vector[a][ 8]*cmat[6];
		mat_vector[b][7]  = mat_vector[a][ 6]*cmat[1] + mat_vector[a][ 7]*cmat[4] + mat_vector[a][ 8]*cmat[7];
		mat_vector[b][8]  = mat_vector[a][ 6]*cmat[2] + mat_vector[a][ 7]*cmat[5] + mat_vector[a][ 8]*cmat[8];
		mat_vector[b][9]  = mat_vector[a][ 9]*cmat[0] + mat_vector[a][10]*cmat[3] + mat_vector[a][11]*cmat[6] + cmat[9];
		mat_vector[b][10] = mat_vector[a][ 9]*cmat[1] + mat_vector[a][10]*cmat[4] + mat_vector[a][11]*cmat[7] + cmat[10];
		mat_vector[b][11] = mat_vector[a][ 9]*cmat[2] + mat_vector[a][10]*cmat[5] + mat_vector[a][11]*cmat[8] + cmat[11];
	} else
		logerror("TGP ERROR bad vector index\n");
	logerror("TGP vmat_mul %d, %d (%x)\n", a, b, activecpu_get_pc());
	next_fn();
}

static void vmat_read(void)
{
	UINT32 a = fifoin_pop();
	logerror("TGP vmat_read %d (%x)\n", a, activecpu_get_pc());
	if(a<21) {
		int i;
		for(i=0; i<12; i++)
			fifoout_push_f(mat_vector[a][i]);
	} else {
		int i;
		logerror("TGP ERROR bad vector index\n");
		for(i=0; i<12; i++)
			fifoout_push_f(0);
	}
	next_fn();
}

static void matrix_rtrans(void)
{
	logerror("TGP matrix_rtrans (%x)\n", activecpu_get_pc());
	fifoout_push_f(cmat[ 9]);
	fifoout_push_f(cmat[10]);
	fifoout_push_f(cmat[11]);
	next_fn();
}

static void matrix_unrot(void)
{
	logerror("TGP matrix_unrot (%x)\n", activecpu_get_pc());
	memset(cmat, 0, 9*sizeof(cmat[0]));
	cmat[0] = 1.0;
	cmat[4] = 1.0;
	cmat[8] = 1.0;
	next_fn();
}

static void f80(void)
{
	logerror("TGP f80 (%x)\n", activecpu_get_pc());
	/*	cmat[9] = cmat[10] = cmat[11] = 0; */
	next_fn();
}

static void vmat_save(void)
{
	UINT32 a = fifoin_pop();
	int i;
	logerror("TGP vmat_save 0x%x (%x)\n", a, activecpu_get_pc());
	for(i=0; i<16; i++)
		memcpy(ram_data+a+0x10*i, mat_vector[i], sizeof(cmat));
	next_fn();
}

static void vmat_load(void)
{
	UINT32 a = fifoin_pop();
	int i;
	logerror("TGP vmat_load 0x%x (%x)\n", a, activecpu_get_pc());
	for(i=0; i<16; i++)
		memcpy(mat_vector[i], ram_data+a+0x10*i, sizeof(cmat));
	next_fn();
}

static void ram_setadr(void)
{
    ram_scanadr = fifoin_pop() - 0x8000;
	logerror("TGP f0 ram_setadr 0x%x (%x)\n", ram_scanadr+0x8000, activecpu_get_pc());
	next_fn();
}

static void groundbox_test(void)
{
	int out_x, out_y, out_z;
	float x, y, z;
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();

	logerror("TGP groundbox_test %f, %f, %f (%x)\n", a, b, c, activecpu_get_pc());
	x = cmat[0]*a+cmat[3]*b+cmat[6]*c+cmat[9];
	y = cmat[1]*a+cmat[4]*b+cmat[7]*c+cmat[10];
	z = cmat[2]*a+cmat[5]*b+cmat[8]*c+cmat[11];

	out_x = x < tgp_vf_xmin || x > tgp_vf_xmax;
	out_z = z < tgp_vf_zmin || z > tgp_vf_zmax;
	out_y = 1; /* Wrong, but untestable it seems. */

	fifoout_push(out_x);
	fifoout_push(out_y);
	fifoout_push(out_z);
	next_fn();
}

static void f89(void)
{
    UINT32 a = fifoin_pop();
	UINT32 b = fifoin_pop();
    UINT32 c = fifoin_pop();
	UINT32 d = fifoin_pop();
	logerror("TGP list set base 0x%x, 0x%x, %d, length=%d (%x)\n", a, b, c, d, activecpu_get_pc());
	list_length = d;
	next_fn();
}

static void f92(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	logerror("TGP f92 %f, %f, %f, %f (%x)\n", a, b, c, d, activecpu_get_pc());
	next_fn();
}

static void f93(void)
{
	float a = fifoin_pop_f();
	logerror("TGP f93 %f (%x)\n", a, activecpu_get_pc());
	next_fn();
}

static void f94(void)
{
	UINT32 a = fifoin_pop();
	logerror("TGP f94 %d (%x)\n", a, activecpu_get_pc());
	next_fn();
}

static void vmat_flatten(void)
{
	int i;
	float m[12];
	logerror("TGP vmat_flatten (%x)\n", activecpu_get_pc());

	for(i=0; i<16; i++) {
		memcpy(m, mat_vector[i], sizeof(cmat));
		m[1] = m[4] = m[7] = m[10] = 0;

		mat_vector[i][0]  = m[ 0]*cmat[0] + m[ 1]*cmat[3] + m[ 2]*cmat[6];
		mat_vector[i][1]  = m[ 0]*cmat[1] + m[ 1]*cmat[4] + m[ 2]*cmat[7];
		mat_vector[i][2]  = m[ 0]*cmat[2] + m[ 1]*cmat[5] + m[ 2]*cmat[8];
		mat_vector[i][3]  = m[ 3]*cmat[0] + m[ 4]*cmat[3] + m[ 5]*cmat[6];
		mat_vector[i][4]  = m[ 3]*cmat[1] + m[ 4]*cmat[4] + m[ 5]*cmat[7];
		mat_vector[i][5]  = m[ 3]*cmat[2] + m[ 4]*cmat[5] + m[ 5]*cmat[8];
		mat_vector[i][6]  = m[ 6]*cmat[0] + m[ 7]*cmat[3] + m[ 8]*cmat[6];
		mat_vector[i][7]  = m[ 6]*cmat[1] + m[ 7]*cmat[4] + m[ 8]*cmat[7];
		mat_vector[i][8]  = m[ 6]*cmat[2] + m[ 7]*cmat[5] + m[ 8]*cmat[8];
		mat_vector[i][9]  = m[ 9]*cmat[0] + m[10]*cmat[3] + m[11]*cmat[6] + cmat[9];
		mat_vector[i][10] = m[ 9]*cmat[1] + m[10]*cmat[4] + m[11]*cmat[7] + cmat[10];
		mat_vector[i][11] = m[ 9]*cmat[2] + m[10]*cmat[5] + m[11]*cmat[8] + cmat[11];
	}
	next_fn();
}

static void vmat_load1(void)
{
	UINT32 a = fifoin_pop();
	logerror("TGP vmat_load1 0x%x (%x)\n", a, activecpu_get_pc());
	memcpy(cmat, ram_data+a, sizeof(cmat));
	next_fn();
}

static void ram_trans(void)
{
	float a = ram_get_f();
	float b = ram_get_f();
	float c = ram_get_f();
	logerror("TGP ram_trans (%x)\n", activecpu_get_pc());
	cmat[ 9] += cmat[0]*a+cmat[3]*b+cmat[6]*c;
	cmat[10] += cmat[1]*a+cmat[4]*b+cmat[7]*c;
	cmat[11] += cmat[2]*a+cmat[5]*b+cmat[8]*c;
	next_fn();
}

static void f98_load(void)
{
	int i;
	for(i=0; i<list_length; i++) {
		float f = fifoin_pop_f();
		logerror("TGP load list (%2d/%2d) %f (%x)\n", i, list_length, f, activecpu_get_pc());
	}
	next_fn();
}

static void f98(void)
{
    UINT32 a = fifoin_pop();
	logerror("TGP load list start %d (%x)\n", a, activecpu_get_pc());
	fifoin_cbcount = list_length;
	fifoin_cb = f98_load;
}

static void f99(void)
{
	logerror("TGP f99 (%x)\n", activecpu_get_pc());
	next_fn();
}

static void f100(void)
{
	int i;
	logerror("TGP f100 get list (%x)\n", activecpu_get_pc());
	for(i=0; i<list_length; i++)
		fifoout_push_f((mame_rand() % 1000)/100.0);
	next_fn();
}

static void groundbox_set(void)
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	float g = fifoin_pop_f();
	logerror("TGP groundbox_set %f, %f, %f, %f, %f, %f, %f (%x)\n", a, b, c, d, e, f, g, activecpu_get_pc());
	tgp_vf_xmin = e;
	tgp_vf_xmax = d;
	tgp_vf_zmin = g;
	tgp_vf_zmax = f;
	tgp_vf_ygnd = b;
	tgp_vf_yflr = a;
	tgp_vf_yjmp = c;

	next_fn();
}

static void f102(void)
{
	static int ccount = 0;
	float px, py, pz;
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	UINT32 f = fifoin_pop();
	UINT32 g = fifoin_pop();
	UINT32 h = fifoin_pop();

	ccount++;

	logerror("TGP f0 mve_calc %f, %f, %f, %f, %f, %d, %d, %d (%d) (%x)\n", a, b, c, d, e, f, g, h, ccount, activecpu_get_pc());

	px = u2f(ram_data[ram_scanadr+0x16]);
	py = u2f(ram_data[ram_scanadr+0x17]);
	pz = u2f(ram_data[ram_scanadr+0x18]);

	/*	memset(cmat, 0, sizeof(cmat)); */
	/*	cmat[0] = 1.0; */
	/*	cmat[4] = 1.0; */
	/*	cmat[8] = 1.0; */

	px = c;
	py = d;
	pz = e;

#if 1
	cmat[ 9] += cmat[0]*a+cmat[3]*b+cmat[6]*c;
	cmat[10] += cmat[1]*a+cmat[4]*b+cmat[7]*c;
	cmat[11] += cmat[2]*a+cmat[5]*b+cmat[8]*c;
#else
	cmat[ 9] += px;
	cmat[10] += py;
	cmat[11] += pz;
#endif

	logerror("    f0 mve_calc %f, %f, %f\n", px, py, pz);

	fifoout_push_f(c);
	fifoout_push_f(d);
	fifoout_push_f(e);
	fifoout_push(f);
	fifoout_push(g);
	fifoout_push(h);


	next_fn();
}

static void f103(void)
{
    ram_scanadr = fifoin_pop() - 0x8000;
	logerror("TGP f0 mve_setadr 0x%x (%x)\n", ram_scanadr, activecpu_get_pc());
	ram_get_i();
	next_fn();
}

struct function {
	int nb;
	void (*cb)(void);
	int count;
};

static struct function ftab_vf[] = {
	{   0, fadd,            2 },
	{   1, fsub,            2 },
	{   2, fmul,            2 },
	{   3, fdiv,            2 },
	{   5, matrix_push,     0 },
	{   6, matrix_pop,      0 },
	{   7, matrix_write,   12 },
	{   8, clear_stack,     0 },
	{  10, anglev,          2 },
	{  13, track_select,    1 },
	{  14, f14,   4 },
	{  15, anglep,          4 },
	{  16, matrix_ident,    0 },
	{  17, matrix_read,     0 },
	{  18, matrix_trans,    3 },
	{  19, matrix_scale,    3 },
	{  20, matrix_rotx,     1 },
	{  21, matrix_roty,     1 },
	{  22, matrix_rotz,     1 },
	{  24, f24, 1 },
	{  26, transform_point, 3 },
	{  27, fsin,            1 },
	{  28, fcos,            1 },
	{  29, fsinm,           2 },
	{  30, fcosm,           2 },
	{  31, distance3,       6 },
	{  36, acc_set,         1 },
	{  37, acc_get,         0 },
	{  38, acc_add,         1 },
	{  39, acc_sub,         1 },
	{  40, acc_mul,         1 },
	{  41, acc_div,         1 }, /* not used ? */
	{  42, f42,   3 },
	{  43, f43,   6 },
	{  44, f44,   1 },
	{  45, f45,   1 },
	{  46, vlength,         3 },
	{  48, f48,   1 },
	{  49, colbox_set,     12 },
	{  50, colbox_test,     3 },
	{  54, track_lookup,    4 },
	{  64, col_setcirc,     3 },
	{  65, col_testpt,      2 },
	{  67, distance,        4 },
	{  71, car_move,        4 },
	{  72, cpa,            12 },
	{  74, vmat_store,      1 },
	{  75, vmat_restore,    1 },
	{  77, vmat_mul,        2 },
	{  78, vmat_read,       1 },
	{  79, matrix_unrot,    0 },
	{  80, f80,   0 },
	{  82, matrix_rtrans,   0 },
	{  84, vmat_save,       1 },
	{  85, vmat_load,       1 },
	{  86, ram_setadr,      1 },
	{  87, groundbox_test,  3 },
	{  89, f89,   4 },
	{  92, f92,   4 },
	{  93, f93,   1 },
	{  94, f94,   1 },
	{  95, vmat_flatten,    0 },
	{  96, vmat_load1,      1 },
	{  97, ram_trans,       0 },
	{  98, f98,   1 },
	{  99, f99,   0 },
	{ 100, f100,  0 },
	{ 101, groundbox_set,   7 },
	{ 102, f102,  8 },
	{ 103, f103,  1 },
	{ 0, 0, 0 }
};
/* Used in swa scene 1 and unemulated: */
/*   f14 */
/*   f49_swa */
/*   f15_swa */

static struct function ftab_swa[] = {
	{   0, fadd,            2 },
	{   1, fsub,            2 },
	{   2, fmul,            2 },
	{   3, fdiv,            2 },
	{   5, matrix_push,     0 },
	{   6, matrix_pop,      0 },
	{   7, matrix_write,   12 },
	{   8, clear_stack,     0 },
	{   9, matrix_mul,     12 },
	{  10, anglev,          2 },
	{  11, f11,             9 },
	{  12, normalize,       3 },
	{  13, acc_seti,        1 },
	{  14, f14,   4 },
	{  15, f15_swa, 0 },
	{  16, matrix_ident,    0 },
	{  17, matrix_read,     0 },
	{  18, matrix_trans,    3 },
	{  19, matrix_scale,    3 },
	{  20, matrix_rotx,     1 },
	{  21, matrix_roty,     1 },
	{  22, matrix_rotz,     1 },
	{  24, f24_swa,   7 },
	{  26, transform_point, 3 },
	{  27, fsin,            1 },
	{  28, fcos,            1 },
	{  29, fsinm,           2 },
	{  30, fcosm,           2 },
	{  31, distance3,       6 },
	{  34, ftoi,            1 },
	{  35, itof,            1 },
	{  36, acc_set,         1 },
	{  37, acc_get,         0 },
	{  38, acc_add,         1 },
	{  39, acc_sub,         1 },
	{  40, acc_mul,         1 },
	{  41, acc_div,         1 }, /* not used ? */
	{  42, xyz2rqf,         3 },
	{  43, f43_swa, 3 },
	{  44, matrix_sdir,     3 },
	{  45, f45,   1 },
	{  46, vlength,         3 },
	{  47, f47,   3 },
	{  49, f49_swa, 6 },
	{  50, f50_swa, 4 },
	{  52, f52,   0 },
	{  53, matrix_rdir,     3 },
	{  56, f56,   7 },
	{  57, f57,   0 },
	{  58, matrix_readt,    0 },
	{  59, acc_geti,        0 },
	{  60, f60,   0 },
	{  65, push_and_ident,  0 },
	{  67, catmull_rom,     13 },
	{ 0, 0, 0 }
};


static void dump(void)
{
	logerror("TGP FIFOIN write %08x (%x)\n", fifoin_pop(), activecpu_get_pc());
	fifoin_cbcount = 1;
	fifoin_cb = dump;
}

static void function_get_vf(void)
{
	UINT32 f = fifoin_pop() >> 23;
	struct function *fct;

	for(fct = ftab_vf; fct->cb && fct->nb != f; fct++);

	if(fifoout_rpos != fifoout_wpos) {
		int count = fifoout_wpos - fifoout_rpos;
		if(count < 0)
			count += FIFO_SIZE;
		logerror("TGP function called with sizeout = %d\n", count);
	}
	if(fct->cb) {
		fifoin_cbcount = fct->count;
		fifoin_cb = fct->cb;
		/*		logerror("TGP function %d request, %d parameters\n", f, fifoin_cbcount); */
		if(!fifoin_cbcount)
			fifoin_cb();
	} else {
		logerror("TGP function %d unimplemented (%x)\n", f, activecpu_get_pc());
		fifoin_cbcount = 1;
		fifoin_cb = dump;
	}
}

static void function_get_swa(void)
{
	UINT32 f = fifoin_pop();
	struct function *fct;

	for(fct = ftab_swa; fct->cb && fct->nb != f; fct++);

	if(fifoout_rpos != fifoout_wpos) {
		int count = fifoout_wpos - fifoout_rpos;
		if(count < 0)
			count += FIFO_SIZE;
		logerror("TGP function called with sizeout = %d\n", count);
	}
	if(fct->cb) {
		fifoin_cbcount = fct->count;
		fifoin_cb = fct->cb;
		/*		logerror("TGP function %d request, %d parameters\n", f, fifoin_cbcount); */
		if(!fifoin_cbcount)
			fifoin_cb();
	} else {
		logerror("TGP function %d unimplemented (%x)\n", f, activecpu_get_pc());
		fifoin_cbcount = 1;
		fifoin_cb = dump;
	}
}

READ16_HANDLER( model1_tgp_copro_r )
{
	static UINT32 cur;
	if(!offset) {
		cur = fifoout_pop();
		return cur;
	} else
		return cur >> 16;
}

WRITE16_HANDLER( model1_tgp_copro_w )
{
	static UINT32 cur;
	if(offset) {
		cur = (cur & 0x0000ffff) | (data << 16);
		fifoin_push(cur);
	} else
		cur = (cur & 0xffff0000) | data;
}

READ16_HANDLER( model1_tgp_copro_adr_r )
{
	return ram_adr;
}

WRITE16_HANDLER( model1_tgp_copro_adr_w )
{
	COMBINE_DATA(&ram_adr);
}

READ16_HANDLER( model1_tgp_copro_ram_r )
{
	if(!offset) {
		logerror("TGP f0 ram read %04x, %08x (%f) (%x)\n", ram_adr, ram_data[ram_adr], u2f(ram_data[ram_adr]), activecpu_get_pc());
		return ram_data[ram_adr];
	} else
		return ram_data[ram_adr++] >> 16;
}

WRITE16_HANDLER( model1_tgp_copro_ram_w )
{
	COMBINE_DATA(ram_latch+offset);
	if(offset) {
		UINT32 v = ram_latch[0]|(ram_latch[1]<<16);
		logerror("TGP f0 ram write %04x, %08x (%f) (%x)\n", ram_adr, v, u2f(v), activecpu_get_pc());
		ram_data[ram_adr] = v;
		ram_adr++;
	}
}

void model1_tgp_reset(int swa)
{
	ram_adr = 0;
	ram_data = auto_malloc(0x10000*4);
	memset(ram_data, 0, 0x10000*4);

	fifoout_rpos = 0;
	fifoout_wpos = 0;
	fifoin_rpos = 0;
	fifoin_wpos = 0;

	acc = 0;
	mat_stack_pos = 0;
	memset(cmat, 0, sizeof(cmat));
	cmat[0] = 1.0;
	cmat[4] = 1.0;
	cmat[8] = 1.0;

	model1_dump = 0;
	model1_swa = swa;
	next_fn();

	state_save_register_UINT32("tgp", 0, "ram_data",       ram_data, 0x10000);
	state_save_register_UINT16("tgp", 0, "ram_adr",       &ram_adr, 1);
	state_save_register_UINT16("tgp", 0, "ram_scanadr",   &ram_scanadr, 1);
	state_save_register_UINT16("tgp", 0, "ram_latch",      ram_latch, 2);
	state_save_register_int   ("tgp", 0, "fifoout_rpos",  &fifoout_rpos);
	state_save_register_int   ("tgp", 0, "fifoout_wpos",  &fifoout_wpos);
	state_save_register_UINT32("tgp", 0, "fifoout_data",   fifoout_data, FIFO_SIZE);
	state_save_register_int   ("tgp", 0, "fifoin_rpos",   &fifoin_rpos);
	state_save_register_int   ("tgp", 0, "fifoin_wpos",   &fifoin_wpos);
	state_save_register_UINT32("tgp", 0, "fifoin_data",    fifoin_data, FIFO_SIZE);
	state_save_register_float ("tgp", 0, "cmat",           cmat, 12);
	state_save_register_float ("tgp", 0, "mat_stack",     &mat_stack[0][0],  12*MAT_STACK_SIZE);
	state_save_register_float ("tgp", 0, "mat_vector",    &mat_vector[0][0], 12*21);
	state_save_register_int   ("tgp", 0, "mat_stack_pos", &mat_stack_pos);
	state_save_register_float ("tgp", 0, "acc",           &acc, 1);
	state_save_register_UINT32("tgp", 0, "list_length",   &list_length, 1);
}
