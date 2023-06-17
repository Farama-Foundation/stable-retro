/*
 * 2asm: Convert binary files to 80*86 assembler. Version 1.00
 * Adapted by Andrea Mazzoleni for use with MAME
 * HJB 990321:
 * Changed output of hex values from 0xxxxh to $xxxx format
 * Removed "ptr" from "byte ptr", "word ptr" and "dword ptr"
 * OB 990721:
 * Changed to the needs of the new NEC core and FIXED opcode & arg fetching
 * from ROM/RAM pointers to support encrypted cpus
 * stripped most of obsolete code, since NEC's are 16 Bit only (until V60)

Mish:

add aw,%Iv
add aw,%Iv

*/

/* 2asm comments

License:

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

Comments:

   The code was originally snaffled from the GNU C++ debugger, as ported
   to DOS by DJ Delorie and Kent Williams (williams@herky.cs.uiowa.edu).
   Extensively modified by Robin Hilliard in Jan and May 1992.

   This source compiles under Turbo C v2.01.  The disassembler is entirely
   table driven so it's fairly easy to change to suit your own tastes.

   The instruction table has been modified to correspond with that in
   `Programmer's Technical Reference: The Processor and Coprocessor',
   Robert L. Hummel, Ziff-Davis Press, 1992.  Missing (read "undocumented")
   instructions were added and many mistakes and omissions corrected.

Any comments/updates/bug reports to:

   Robin Hilliard, Lough Guitane, Killarney, Co. Kerry, Ireland.
   Tel:         [+353] 64-54014
   Internet:    softloft@iruccvax.ucc.ie
   Compu$erve:  100042, 1237

   If you feel like registering, and possibly get notices of updates and
   other items of software, then send me a post card of your home town.

   Thanks and enjoy!
*/


#include "driver.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef MAME_DEBUG
/*#include "nechost.h"*/

/* Little endian uint read */
#define	le_uint8(ptr) (*(UINT8*)ptr)
static INLINE UINT16 le_uint16(const void* ptr) {
	const UINT8* ptr8 = (const UINT8*)ptr;
	return (UINT16)ptr8[0] | (UINT16)ptr8[1] << 8;
}
static INLINE UINT32 le_uint32(const void* ptr) {
	const UINT8* ptr8 = (const UINT8*)ptr;
	return (UINT32)ptr8[0] | (UINT32)ptr8[1] << 8 |	(UINT32)ptr8[2] << 16 | (UINT32)ptr8[3] << 24;
}

/* Little endian int read */
#define le_int8(ptr) ((INT8)le_uint8(ptr))
#define le_int16(ptr) ((INT16)le_uint16(ptr))
#define le_int32(ptr) ((INT32)le_uint32(ptr))

#define fp_segment(dw) ((dw >> 16) & 0xFFFFU)
#define fp_offset(dw) (dw & 0xFFFFU)

static UINT8 must_do_size;   /* used with size of operand */
static int wordop;           /* dealing with word or byte operand */
static int instruction_offset;

static char* ubufs;           /* start of buffer */
static char* ubufp;           /* last position of buffer */
static int invalid_opcode = 0;
static int first_space = 1;

static int prefix;            /* segment override prefix byte */
static int modrmv;            /* flag for getting modrm byte */
static int sibv;              /* flag for getting sib byte   */
static int opsize;            /* just like it says ...       */
static int addrsize;

/* some defines for extracting instruction bit fields from bytes */

#define MOD(a)	  (((a)>>6)&3)	/* mod has only 2 bits - OB */
#define REG(a)	  (((a)>>3)&7)
#define RM(a)	  ((a)&7)
#define SCALE(a)  (((a)>>6)&7)
#define INDEX(a)  (((a)>>3)&7)
#define BASE(a)   ((a)&7)

/* Percent tokens in strings:
   First char after '%':
	A - direct address
	C - reg of r/m picks control register
	D - reg of r/m picks debug register
	E - r/m picks operand
	F - flags register
	G - reg of r/m picks general register
	I - immediate data
	J - relative IP offset
+       K - call/jmp distance
	M - r/m picks memory
	O - no r/m, offset only
	R - mod of r/m picks register only
	S - reg of r/m picks segment register
	T - reg of r/m picks test register
	X - DS:ESI
	Y - ES:EDI
	2 - prefix of two-byte opcode
+       e - put in 'e' if use32 (second char is part of reg name)
+           put in 'w' for use16 or 'd' for use32 (second char is 'w')
+       j - put in 'e' in jcxz if prefix==0x66
	f - floating point (second char is esc value)
	g - do r/m group 'n', n==0..7
	p - prefix
	s - size override (second char is a,o)
+       d - put d if double arg, nothing otherwise (pushfd, popfd &c)
+       w - put w if word, d if double arg, nothing otherwise (lodsw/lodsd)
+       P - simple prefix

   Second char after '%':
	a - two words in memory (BOUND)
	b - byte
	c - byte or word
	d - dword
+       f - far call/jmp
+       n - near call/jmp
        p - 32 or 48 bit pointer
+       q - byte/word thingy
	s - six byte pseudo-descriptor
	v - word or dword
        w - word
+       x - sign extended byte
	1-8 - group number, esc value, etc
*/

/* watch out for aad && aam with odd operands */

/*static*/ const char *opmap1[256] = {
/* 0 */
  "add %Eb,%Gb",      "add %Ev,%Gv",     "add %Gb,%Eb",    "add %Gv,%Ev",
  "add al,%Ib",       "add aw,%Iv",      "push es",        "pop es",
  "or %Eb,%Gb",       "or %Ev,%Gv",      "or %Gb,%Eb",     "or %Gv,%Ev",
  "or al,%Ib",        "or aw,%Iv",       "push cs",        "%2 ",
/* 1 */
  "adc %Eb,%Gb",      "adc %Ev,%Gv",     "adc %Gb,%Eb",    "adc %Gv,%Ev",
  "adc al,%Ib",       "adc aw,%Iv",      "push ss",        "pop ss",
  "sbb %Eb,%Gb",      "sbb %Ev,%Gv",     "sbb %Gb,%Eb",    "sbb %Gv,%Ev",
  "sbb al,%Ib",       "sbb aw,%Iv",      "push ds",        "pop ds",
/* 2 */
  "and %Eb,%Gb",      "and %Ev,%Gv",     "and %Gb,%Eb",    "and %Gv,%Ev",
  "and al,%Ib",       "and aw,%Iv",      "%pe",            "adj4a",
  "sub %Eb,%Gb",      "sub %Ev,%Gv",     "sub %Gb,%Eb",    "sub %Gv,%Ev",
  "sub al,%Ib",       "sub aw,%Iv",      "%pc",            "adj4s",
/* 3 */
  "xor %Eb,%Gb",      "xor %Ev,%Gv",     "xor %Gb,%Eb",    "xor %Gv,%Ev",
  "xor al,%Ib",       "xor aw,%Iv",      "%ps",            "adjba",
  "cmp %Eb,%Gb",      "cmp %Ev,%Gv",     "cmp %Gb,%Eb",    "cmp %Gv,%Ev",
  "cmp al,%Ib",       "cmp aw,%Iv",      "%pd",            "adjbs",
/* 4 */
  "inc aw",           "inc cw",          "inc dw",         "inc bw",
  "inc sp",           "inc bp",          "inc ix",         "inc iy",
  "dec aw",           "dec cw",          "dec dw",         "dec bw",
  "dec sp",           "dec bp",          "dec ix",         "dec iy",
/* 5 */
  "push aw",          "push cw",         "push dw",        "push bw",
  "push sp",          "push bp",         "push ix",        "push iy",
  "pop aw",           "pop cw",          "pop dw",         "pop bw",
  "pop sp",           "pop bp",          "pop ix",         "pop iy",
/* 6 */
  "pusha",            "popa",            "chkind %Gv,%Ma", 0,
  "repnc %p",         "repc %p",         0,                0, /* FPO isn't supported */
  "push %Iw",         "imul %Gw,%Ew,%Iw","push %Ix",       "imul %Gw,%Ew,%Ib",
  "insb",             "insw",            "outsb",          "outsw",
/* 7 */
  "jo %Jb",           "jno %Jb",         "jc %Jb",         "jnc %Jb",
  "je %Jb",           "jne %Jb",         "jbe %Jb",        "ja %Jb",
  "js %Jb",           "jns %Jb",         "jpe %Jb",        "jpo %Jb",
  "jl %Jb",           "jge %Jb",         "jle %Jb",        "jg %Jb",
/* 8 */
/*  "%g0 %Eb,%Ib",      "%g0 %Ev,%Iv",     "%g0 %Ev,%Ib",    "%g0 %Ev,%Ib", */
  "%g0 %Eb,%Ib",      "%g0 %Ew,%Iw",     "%g0 %Ew,%Ix",    "%g0 %Ew,%Ix",
  "test %Eb,%Gb",     "test %Ew,%Gw",    "xch %Eb,%Gb",    "xch %Ew,%Gw",
  "mov %Eb,%Gb",      "mov %Ev,%Gw",     "mov %Gb,%Eb",    "mov %Gw,%Ew",
  "mov %Ew,%Sw",      "ldea %Gw,%M ",    "mov %Sw,%Ew",    "pop %Ev",
/* 9 */
/*above LDEA is wrong!*/

  "nop",              "xch cw,aw",       "xch dw,aw",      "xch bw,aw",
  "xch sp,aw",        "xch bp,aw",       "xch ix,aw",      "xch iy,aw",
  "cvtbw",            "cvtwl",           "call %Ap",       "fwait",
  "pushf%d ",         "popf%d ",         "sahf",           "lahf",
/* a */
  "mov al,%Oc",       "mov aw,%Ov",      "mov %Oc,al",     "mov %Ov,aw",
  "%P movsb",         "%P movsw",        "%P cmpsb",       "%P cmpsw ",
  "test al,%Ib",      "test aw,%Iv",     "%P stosb",       "%P stosw ",
  "%P lodsb",         "%P lodsw ",       "%P scasb",       "%P scasw ",
/* b */
  "mov al,%Ib",       "mov cl,%Ib",      "mov dl,%Ib",     "mov bl,%Ib",
  "mov ah,%Ib",       "mov ch,%Ib",      "mov dh,%Ib",     "mov bh,%Ib",
  "mov aw,%Iv",       "mov cw,%Iv",      "mov dw,%Iv",     "mov bw,%Iv",
  "mov sp,%Iv",       "mov bp,%Iv",      "mov ix,%Iv",     "mov iy,%Iv",
/* c */
  "%g1 %Eb,%Ib",      "%g1 %Ev,%Ib",     "ret %Iw",        "ret",
  "les %Gv,%Mp",      "lds %Gv,%Mp",     "mov %Eb,%Ib",    "mov %Ev,%Iv",
  "enter %Iw,%Ib",    "leave",           "retf %Iw",       "retf",
  "int 03",           "int %Ib",         "into",           "iret",
/* d */
  "%g1 %Eb,1",        "%g1 %Ev,1",       "%g1 %Eb,cl",     "%g1 %Ev,cl",
  "aam ; %Ib",        "aad ; %Ib",       0,                "trans",
  "%f0",              "%f1",             "%f2",            "%f3",
  "%f4",              "%f5",             "%f6",            "%f7",
/* e */
  "loopne %Jb",       "loope %Jb",       "loop %Jb",       "j%j cxz %Jb",
  "in al,%Ib",        "in aw,%Ib",       "out %Ib,al",     "out %Ib,aw",
  "call %Jv",         "jmp %Jv",         "jmp %Ap",        "jmp %Ks%Jb",
  "in al,dw",         "in aw,dw",        "out dw,al",      "out dx,aw",
/* f */
  "lock %p ",         0,                 "repne %p ",      "repe %p ",
  "hlt",              "not1 CY(cmc)",    "%g2",            "%g8",
  "clc",              "stc",             "di",             "ei",
  "cld",              "std",             "%g3",            "%g4"
};


static const char *second[] = {
/* 0 */
  "%g5",              "%g6",             "lar %Gv,%Ew",    "lsl %Gv,%Ew",
  0,                  "loadall",         "clts",           "loadall",
  "invd",             "wbinvd",          0,                0,
  0,                  0,                 0,                0,
/* 1 */
  "test1 %Eb,cl",     "test1 %Ew,cl",  	 "clr1 %Eb,cl",    "clr1 %Ew,cl",
  "set1 %Eb,cl",      "set1 %Ew,cl",     "not1 %Eb,cl",    "not1 %Ew,cl",
  "test1 %Eb,%Ib",    "test1 %Ew,%Ib",   "clr1 %Eb,%Ib",   "clr1 %Ew,%Ib",
  "set1 %Eb,%Ib",     "set1 %Ew,%Ib",    "not1 %Eb,%Ib",   "not1 %Ew,%Ib",
/* 2 */
  "add4s",            "mov %Rd,%Dd",     "sub4s",          0,
  "movspa",           0,                 "cmp4s",          0,
  "rol4 %Eb",         0,                 "ror4 %Eb",       0,
  0,                  "brkcs %Ib",       0,                0,
/* 3 */
  0,                  "ins %Eb,%Gb",     0,                "ext %Eb,%Gb",
  0, 0, 0, 0,
  0,                  "ins %Eb,%Ib",     0,                "ext %Eb,%Ib",
  0, 0, 0, 0,
/* 4 */
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
/* 5 */
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
/* 6 */
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
/* 7 */
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
/* 8 */
  "jo %Jv",           "jno %Jv",         "jb %Jv",         "jnb %Jv",
  "jz %Jv",           "jnz %Jv",         "jbe %Jv",        "ja %Jv",
  "js %Jv",           "jns %Jv",         "jp %Jv",         "jnp %Jv",
  "jl %Jv",           "jge %Jv",         "jle %Jv",        "jg %Jv",

  0, 0, "fint", 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,

/* a */
  "push fs",          "pop fs",          0,                "bt %Ev,%Gv",
  "shld %Ev,%Gv,%Ib", "shld %Ev,%Gv,cl", 0,                0,
  "push gs",          "pop gs",          0,                "bts %Ev,%Gv",
  "shrd %Ev,%Gv,%Ib", "shrd %Ev,%Gv,cl", 0,                "imul %Gv,%Ev",
/* b */
  "cmpxchg %Eb,%Gb",  "cmpxchg %Ev,%Gv", "lss %Mp",        "btr %Ev,%Gv",
  "lfs %Mp",          "lgs %Mp",         "movzx %Gv,%Eb",  "movzx %Gv,%Ew",
  0,                  0,                 "%g7 %Ev,%Ib",    "btc %Ev,%Gv",
  "bsf %Gv,%Ev",      "bsr %Gv,%Ev",     "movsx %Gv,%Eb",  "movsx %Gv,%Ew",
/* c */
  "xadd %Eb,%Gb",     "xadd %Ev,%Gv",    0,                0,
  0,                  0,                 0,                0,
  0,                  0,                 0,                0,
  0,                  0,                 0,                0,
 /* d */
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
/* e */
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
/* f */
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, "brkem %Ib",
};

static const char *groups[][8] = {   /* group 0 is group 3 for %Ev set */
/* 0 */
  { "add",            "or",              "adc",            "sbb",
    "and",            "sub",             "xor",            "cmp"           },
/* 1 */
  { "rol",            "ror",             "rcl",            "rcr",
    "shl",            "shr",             "shl",            "sar"           },
/* 2 - NEC 'group 2' byte */
  { "test %Eq,%Iq",   "db f6 Illegal",   "not %Eb",        "neg %Eb",
    "mul %Ec",        "imul %Ec",        "div %Ec",        "idiv %Ec" },
/* 3 */
  { "inc %Eb",        "dec %Eb",         0,                0,
    0,                0,                 0,                0               },
/* 4 */
  { "inc %Ev",        "dec %Ev",         "call %Kn%Ev",  "call %Kf%Ep",
    "jmp %Kn%Ev",     "jmp %Kf%Ep",      "push %Ev",       0               },
/* 5 */
  { "sldt %Ew",       "str %Ew",         "lldt %Ew",       "ltr %Ew",
    "verr %Ew",       "verw %Ew",        0,                0               },
/* 6 */
  { "sgdt %Ms",       "sidt %Ms",        "lgdt %Ms",       "lidt %Ms",
    "smsw %Ew",       0,                 "lmsw %Ew",       0               },
/* 7 */
  { 0,                0,                 0,                0,
    "bt",             "bts",             "btr",            "btc"           },
/* 8 - NEC 'group 2' word */
  { "test %Eq,%Iq",   "db f6 Illegal",   "not %Ev",        "neg %Ev",
    "mul %Ec",        "imul %Ec",        "div %Ec",        "idiv %Ec" },
};


static char *addr_to_hex(UINT32 addr, int splitup) {
  static char buffer[11];

  if (splitup) {
    if (fp_segment(addr)==0 || fp_offset(addr)==0xffff) /* 'coz of wraparound */
      sprintf(buffer, "%04X", (unsigned)fp_offset(addr) );
    else
/*      sprintf(buffer, "%04X:%04X", (unsigned)fp_segment(addr), (unsigned)fp_offset(addr) );*/
      sprintf(buffer, "%04X:%04X", (unsigned)addr >> 4, (unsigned)addr - ((addr >> 4) << 4) );

  } else {
    if (fp_segment(addr)==0 || fp_segment(addr)==0xffff) /* 'coz of wraparound */
      sprintf(buffer, "%04X", (unsigned)fp_offset(addr) );
    else
      sprintf(buffer, "%08lX", (unsigned long)addr );
  }

  return buffer;
}

/* in nec.c */
unsigned nec_get_reg(int regnum);

static UINT8 getopcode(void)
{
	UINT8 res;

	int pc_masked = (instruction_offset++)&0xfffff;
	change_pc20(pc_masked);
	res = OP_ROM[pc_masked];
	change_pc20(nec_get_reg(REG_PC));
	return res;
}

static UINT8 getbyte(void) {
	UINT8 res;

	int pc_masked = (instruction_offset++)&0xfffff;
	change_pc20(pc_masked);
	res = OP_RAM[pc_masked];
	change_pc20(nec_get_reg(REG_PC));
	return res;
}

static int modrm(void)
{
  if (modrmv == -1)
    modrmv = getbyte();
  return modrmv;
}

/*------------------------------------------------------------------------*/

static void uprintf(const char *s, ...)
{
        va_list	arg_ptr;
	va_start (arg_ptr, s);
	vsprintf(ubufp, s, arg_ptr);
        while (*ubufp)
              ubufp++;
}

static void uputchar(char c)
{
  *ubufp++ = c;
  *ubufp = 0;
}

/*------------------------------------------------------------------------*/

static int bytes(char c)
{
  switch (c) {
  case 'b':
       return 1;
  case 'w':
       return 2;
  case 'd':
       return 4;
  case 'v':
       if (opsize == 32)
         return 4;
       else
         return 2;
  }
  return 0;
}

/*------------------------------------------------------------------------*/
static void outhex(char subtype, int extend, int optional, int defsize, int sign)
{
  int n=0, s=0, i;
  INT32 delta = 0;
  unsigned char buff[6];
  char *name;
  char  signchar;

  switch (subtype) {
  case 'q':
       if (wordop) {
         if (opsize==16) {
           n = 2;
         } else {
           n = 4;
         }
       } else {
         n = 1;
       }
       break;

  case 'a':
       break;
  case 'x':
       extend = 2;
       n = 1;
       break;
  case 'b':
       n = 1;
       break;
  case 'w':
       n = 2;
       break;
  case 'd':
       n = 4;
       break;
  case 's':
       n = 6;
       break;
  case 'c':
  case 'v':
       if (defsize == 32)
         n = 4;
       else
         n = 2;
       break;
  case 'p':
       if (defsize == 32)
         n = 6;
       else
         n = 4;
       s = 1;
       break;
  }
  for (i=0; i<n; i++)
    buff[i] = getbyte();
  for (; i<extend; i++)
    buff[i] = (buff[i-1] & 0x80) ? 0xff : 0;
  if (s) {
    uprintf("%02X%02X:", (unsigned)buff[n-1], (unsigned)buff[n-2]);
    n -= 2;
  }
  switch (n) {
  case 1:
       delta = le_int8(buff);
       break;
  case 2:
       delta = le_int16(buff);
       break;
  case 4:
       delta = le_int32(buff);
       break;
  }
  if (extend > n) {
    if (subtype!='x') {
      if (delta<0) {
        delta = -delta;
        signchar = '-';
      } else
        signchar = '+';
      if (delta || !optional)
		uprintf("%c$%0*lX", (char)signchar, (int)(extend), (long)delta);
    } else {
      if (extend==2)
        delta = (UINT16)delta;
	  uprintf("$%0.*lX", (int)(2*extend), (long)delta );
    }
    return;
  }
  if ((n == 4) && !sign) {
    name = addr_to_hex(delta, 0);
    uprintf("%s", name);
    return;
  }
  switch (n) {
  case 1:
       if (sign && (char)delta<0) {
         delta = -delta;
         signchar = '-';
       } else
         signchar = '+';
       if (sign)
		 uprintf("%c$%02lX", (char)signchar, delta & 0xFFL);
       else
		 uprintf("$%02lX", delta & 0xFFL);
       break;

  case 2:
       if (sign && delta<0) {
         signchar = '-';
         delta = -delta;
       } else
         signchar = '+';
       if (sign)
		 uprintf("%c$%04lX", (char)signchar, delta & 0xFFFFL);
       else
		 uprintf("$%04lX", delta & 0xFFFFL);
       break;

  case 4:
       if (sign && delta<0) {
         delta = -delta;
         signchar = '-';
       } else
         signchar = '+';
       if (sign)
		 uprintf("%c$%08lX", (char)signchar, delta & 0xFFFFFFFFL);
       else
		 uprintf("$%08lX", delta & 0xFFFFFFFFL);
       break;
  }
}


/*------------------------------------------------------------------------*/

static void reg_name(int regnum, char size)
{
  if ((size=='q' || size == 'b' || size=='c') && !wordop) {
    uputchar("acdbacdb"[regnum]);
    uputchar("llllhhhh"[regnum]);
  } else {
    uputchar("acdbsbii"[regnum]);
    uputchar("wwwwppxy"[regnum]);
  }
}


/*------------------------------------------------------------------------*/

static void ua_str(const char *str);

/*------------------------------------------------------------------------*/
static void do_modrm(char subtype)
{
  int mod = MOD(modrm());
  int rm = RM(modrm());


  if (mod == 3) { /* specifies two registers */
    reg_name(rm, subtype);
    return;
  }
  if (must_do_size) {
    if (wordop) {
		ua_str("word ");
    } else {
	  ua_str("byte ");
    }
  }
  if ((mod == 0) && (rm == 6) && (addrsize == 16)) { /* 16 bit dsplcmnt */
    ua_str("%p:[");
    outhex('w', 2, 0, addrsize, 0);
    uputchar(']');
    return;
  }
  if ((addrsize != 32) || (rm != 4))
    ua_str("%p:[");
  if (addrsize == 16) {
    switch (rm) {
    case 0: uprintf("bw+ix"); break;
    case 1: uprintf("bw+iy"); break;
    case 2: uprintf("bp+ix"); break;
    case 3: uprintf("bp+iy"); break;
    case 4: uprintf("ix"); break;
    case 5: uprintf("iy"); break;
    case 6: uprintf("bp"); break;
    case 7: uprintf("bw"); break;
    }
  }
  switch (mod) {
  case 1:
       outhex('b', 2, 1, addrsize, 0);
       break;
  case 2:
       outhex('v', 2, 1, addrsize, 1);
       break;
  }
  uputchar(']');
}



/*------------------------------------------------------------------------*/
/* Main table driver                                                      */

static void percent(char type, char subtype)
{
  INT32 vofs = 0;
  char *name;
  /*int extend = (addrsize == 32) ? 4 : 2;*/
  int extend = 2;	/* NEC only has 16 Bit*/

  UINT8 c;
  unsigned d;

  switch (type) {
  case 'A':                          /* direct address */
       outhex(subtype, extend, 0, addrsize, 0);
       break;

  case 'C':                          /* reg(r/m) picks control reg */
       uprintf("C%d", REG(modrm()));
       must_do_size = 0;
       break;

  case 'D':                          /* reg(r/m) picks debug reg */
       uprintf("D%d", REG(modrm()));
       must_do_size = 0;
       break;

  case 'E':                          /* r/m picks operand */
       do_modrm(subtype);
       break;

  case 'G':                          /* reg(r/m) picks register */
    /*   if (subtype == 'F')                 /* 80*87 operand?   */*/
    /*     reg_name(RM(modrm()), subtype);*/
    /*   else*/
         reg_name(REG(modrm()), subtype);
       must_do_size = 0;
       break;

  case 'I':                            /* immed data */
       outhex(subtype, 0, 0, opsize, 0);
       break;

  case 'J':                            /* relative IP offset */
       switch (bytes(subtype)) {              /* sizeof offset value */
       case 1:
            vofs = (INT8)getbyte();
            break;
       case 2:
            vofs = getbyte();
            vofs |= getbyte()<<8;
            break;
       case 4:
            vofs = (UINT32)getbyte();           /* yuk! */
            vofs |= (UINT32)getbyte() << 8;
            vofs |= (UINT32)getbyte() << 16;
            vofs |= (UINT32)getbyte() << 24;
            break;
       }
       name = addr_to_hex(vofs+instruction_offset,1);
	   uprintf("$%s ($%+d)", name, vofs);
       break;

  case 'K':
       switch (subtype) {
       case 'f':
            ua_str("far ");
            break;
       case 'n':
            ua_str("near ");
            break;
       case 's':
            ua_str("short ");
            break;
       }
       break;

  case 'M':                            /* r/m picks memory */
       do_modrm(subtype);
       break;

  case 'O':                            /* offset only */
       ua_str("%p:[");
       outhex(subtype, extend, 0, addrsize, 0);
       uputchar(']');
       break;

  case 'P':                            /* prefix byte (rh) */
       ua_str("%p:");
       break;

  case 'R':                            /* mod(r/m) picks register */
       reg_name(REG(modrm()), subtype);      /* rh */
       must_do_size = 0;
       break;

  case 'S':                            /* reg(r/m) picks segment reg */
       uputchar("ecsdfg"[REG(modrm())]);
       uputchar('s');
       must_do_size = 0;
       break;

  case 'X':                            /* ds:si type operator */
       uprintf("ds:[");
       if (addrsize == 32)
         uputchar('e');
       uprintf("ix]");
       break;

  case 'Y':                            /* es:di type operator */
       uprintf("es:[");
       if (addrsize == 32)
         uputchar('e');
       uprintf("iy]");
       break;

  case '2':
       d=getbyte();	/* 0f xx*/
       /*wordop = d & 1;*/
       ua_str(second[d]);
       break;

  case 'g':                            /* modrm group `subtype' (0--7) */
       ua_str(groups[subtype-'0'][REG(modrm())]);
       break;

  case 'd':                             /* sizeof operand==dword? */
       if (opsize == 32)
         uputchar('d');
       uputchar(subtype);
       break;

  case 'w':                             /* insert explicit size specifier */
       if (opsize == 32)
         uputchar('d');
       else
         uputchar('w');
       uputchar(subtype);
       break;

  case 'p':                    /* prefix byte */
       switch (subtype)  {
       case 'c':
       case 'd':
       case 'e':
       case 'f':
       case 'g':
       case 's':
            prefix = subtype;
			c = getopcode();
            wordop = c & 1;
            ua_str(opmap1[c]);
            break;
       case ':':
            if (prefix)
              uprintf("%cs:", prefix);
            break;
       case ' ':
            c = getopcode();
            wordop = c & 1;
            ua_str(opmap1[c]);
            break;
       }
       break;

  case 's':                           /* size override */
       switch (subtype) {
       case 'a':
            addrsize = 48 - addrsize;
			c = getopcode();
            wordop = c & 1;
            ua_str(opmap1[c]);
            break;
       case 'o':
            opsize = 48 - opsize;
			c = getopcode();
            wordop = c & 1;
            ua_str(opmap1[c]);
            break;
       }
       break;
   }
}


static void ua_str(const char *str)
{
  char c;

  if (str == 0) {
    invalid_opcode = 1;
    uprintf("?");
    return;
  }

  if (strpbrk(str, "CDFGRST")) /* specifiers for registers=>no size 2b specified */
    must_do_size = 0;

  while ((c = *str++) != 0) {
	if (c == ' ' && first_space)
	{
		first_space = 0;
		do
		{
			uputchar(' ');
		} while ( (int)(ubufp - ubufs) < 5 );
	}
	else
    if (c == '%') {
      c = *str++;
      percent(c, *str++);
    } else {
      uputchar(c);
    }
  }
}

unsigned Dasmnec(char* buffer, unsigned pc)
{
  	unsigned c;

	instruction_offset = pc;

	/* output buffer */
	ubufs = buffer;
	ubufp = buffer;
	first_space = 1;

	prefix = 0;
	modrmv = -1;   /* read next byte as modrm */
	sibv = -1;     /* set modrm and sib flags */
	opsize = addrsize = 16;

	/*c = getbyte();			/* read opcode */*/
	c = getopcode();
	wordop = c & 1;
	/*wordop=0;*/
	must_do_size = 1;
	invalid_opcode = 0;
	ua_str(opmap1[c]);

  	if (invalid_opcode) {
		/* restart output buffer */
		ubufp = buffer;
		/* invalid instruction, use db xx */
		uprintf("db %02Xh", (unsigned)c);
		return 1;
	}

  	return instruction_offset - pc;
}

#endif	/* MAME_DEBUG */
