/*
 * Copyright (C) 2020  Free Software Foundation, Inc.
 *
 * This file is part of GNU lightning.
 *
 * GNU lightning is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU lightning is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * Authors:
 *	Paul Cercueil
 */

#ifndef _jit_sh_h
#define _jit_sh_h

#define JIT_HASH_CONSTS		0
#define JIT_NUM_OPERANDS	2

typedef enum {
#define jit_r(i)		(JIT_R0 + (i))
#define jit_r_num()		3
#define jit_v(i)		(JIT_V0 + (i))
#define jit_v_num()		6
#define jit_f(i)		(JIT_F0 - (i) * 2)
#ifdef __SH_FPU_ANY__
#    define jit_f_num()		8
#else
#    define jit_f_num()		0
#endif
	_R0,

	/* caller-saved temporary registers */
#define JIT_R0			_R1
#define JIT_R1			_R2
#define JIT_R2			_R3
	_R1,	_R2,	_R3,

	/* argument registers */
	_R4,	_R5,	_R6,	_R7,

	/* callee-saved registers */
#define JIT_V0			_R8
#define JIT_V1			_R9
#define JIT_V2			_R10
#define JIT_V3			_R11
#define JIT_V4			_R12
#define JIT_V5			_R13
	_R8,	_R9,	_R10,	_R11,	_R12,	_R13,

#define JIT_FP			_R14
	_R14,
	_R15,

	_GBR,

	/* floating-point registers */
#define JIT_F0			_F14
#define JIT_F1			_F12
#define JIT_F2			_F10
#define JIT_F3			_F8
#define JIT_F4			_F6
#define JIT_F5			_F4
#define JIT_F6			_F2
#define JIT_F7			_F0
	_F0,	_F1,	_F2,	_F3,	_F4,	_F5,	_F6,	_F7,
	_F8,	_F9,	_F10,	_F11,	_F12,	_F13,	_F14,	_F15,

	/* Banked floating-point registers */
	_XF0,	_XF1,	_XF2,	_XF3,	_XF4,	_XF5,	_XF6,	_XF7,
	_XF8,	_XF9,	_XF10,	_XF11,	_XF12,	_XF13,	_XF14,	_XF15,

#define JIT_NOREG		_NOREG
	_NOREG,
} jit_reg_t;

#endif /* _jit_sh_h */
