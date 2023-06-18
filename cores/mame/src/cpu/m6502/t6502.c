/*****************************************************************************
 *
 *	 tbl6502.c
 *	 6502 opcode functions and function pointer table
 *
 *	 Copyright (c) 1998,1999,2000 Juergen Buchmueller, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 *	 2003-05-26  Fixed PHP, PLP, PHA, PLA cycle counts. [SJ]
 *
 *****************************************************************************/

#undef	OP
#define OP(nn) static INLINE void m6502_##nn(void)

/*****************************************************************************
 *****************************************************************************
 *
 *	 plain vanilla 6502 opcodes
 *
 *****************************************************************************
 * op	 temp	  cycles			 rdmem	 opc  wrmem   ********************/

OP(00) {		  m6502_ICount -= 7;		 BRK;		  } /* 7 BRK */
OP(20) {		  m6502_ICount -= 6;		 JSR;		  } /* 6 JSR */
OP(40) {		  m6502_ICount -= 6;		 RTI;		  } /* 6 RTI */
OP(60) {		  m6502_ICount -= 6;		 RTS;		  } /* 6 RTS */
OP(80) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(a0) { int tmp; m6502_ICount -= 2; RD_IMM; LDY;		  } /* 2 LDY IMM */
OP(c0) { int tmp; m6502_ICount -= 2; RD_IMM; CPY;		  } /* 2 CPY IMM */
OP(e0) { int tmp; m6502_ICount -= 2; RD_IMM; CPX;		  } /* 2 CPX IMM */

OP(10) { int tmp;							 BPL;		  } /* 2 BPL REL */
OP(30) { int tmp;							 BMI;		  } /* 2 BMI REL */
OP(50) { int tmp;							 BVC;		  } /* 2 BVC REL */
OP(70) { int tmp;							 BVS;		  } /* 2 BVS REL */
OP(90) { int tmp;							 BCC;		  } /* 2 BCC REL */
OP(b0) { int tmp;							 BCS;		  } /* 2 BCS REL */
OP(d0) { int tmp;							 BNE;		  } /* 2 BNE REL */
OP(f0) { int tmp;							 BEQ;		  } /* 2 BEQ REL */

OP(01) { int tmp; m6502_ICount -= 6; RD_IDX; ORA;		  } /* 6 ORA IDX */
OP(21) { int tmp; m6502_ICount -= 6; RD_IDX; AND;		  } /* 6 AND IDX */
OP(41) { int tmp; m6502_ICount -= 6; RD_IDX; EOR;		  } /* 6 EOR IDX */
OP(61) { int tmp; m6502_ICount -= 6; RD_IDX; ADC;		  } /* 6 ADC IDX */
OP(81) { int tmp; m6502_ICount -= 6;		 STA; WR_IDX; } /* 6 STA IDX */
OP(a1) { int tmp; m6502_ICount -= 6; RD_IDX; LDA;		  } /* 6 LDA IDX */
OP(c1) { int tmp; m6502_ICount -= 6; RD_IDX; CMP;		  } /* 6 CMP IDX */
OP(e1) { int tmp; m6502_ICount -= 6; RD_IDX; SBC;		  } /* 6 SBC IDX */

OP(11) { int tmp; m6502_ICount -= 5; RD_IDY; ORA;		  } /* 5 ORA IDY */
OP(31) { int tmp; m6502_ICount -= 5; RD_IDY; AND;		  } /* 5 AND IDY */
OP(51) { int tmp; m6502_ICount -= 5; RD_IDY; EOR;		  } /* 5 EOR IDY */
OP(71) { int tmp; m6502_ICount -= 5; RD_IDY; ADC;		  } /* 5 ADC IDY */
OP(91) { int tmp; m6502_ICount -= 6;		 STA; WR_IDY; } /* 6 STA IDY */
OP(b1) { int tmp; m6502_ICount -= 5; RD_IDY; LDA;		  } /* 5 LDA IDY */
OP(d1) { int tmp; m6502_ICount -= 5; RD_IDY; CMP;		  } /* 5 CMP IDY */
OP(f1) { int tmp; m6502_ICount -= 5; RD_IDY; SBC;		  } /* 5 SBC IDY */

OP(02) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(22) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(42) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(62) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(82) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(a2) { int tmp; m6502_ICount -= 2; RD_IMM; LDX;		  } /* 2 LDX IMM */
OP(c2) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(e2) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */

OP(12) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(32) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(52) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(72) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(92) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(b2) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(d2) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(f2) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */

OP(03) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(23) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(43) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(63) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(83) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(a3) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(c3) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(e3) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */

OP(13) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(33) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(53) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(73) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(93) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(b3) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(d3) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(f3) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */

OP(04) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(24) { int tmp; m6502_ICount -= 3; RD_ZPG; BIT;		  } /* 3 BIT ZPG */
OP(44) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(64) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(84) { int tmp; m6502_ICount -= 3;		 STY; WR_ZPG; } /* 3 STY ZPG */
OP(a4) { int tmp; m6502_ICount -= 3; RD_ZPG; LDY;		  } /* 3 LDY ZPG */
OP(c4) { int tmp; m6502_ICount -= 3; RD_ZPG; CPY;		  } /* 3 CPY ZPG */
OP(e4) { int tmp; m6502_ICount -= 3; RD_ZPG; CPX;		  } /* 3 CPX ZPG */

OP(14) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(34) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(54) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(74) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(94) { int tmp; m6502_ICount -= 4;		 STY; WR_ZPX; } /* 4 STY ZPX */
OP(b4) { int tmp; m6502_ICount -= 4; RD_ZPX; LDY;		  } /* 4 LDY ZPX */
OP(d4) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(f4) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */

OP(05) { int tmp; m6502_ICount -= 3; RD_ZPG; ORA;		  } /* 3 ORA ZPG */
OP(25) { int tmp; m6502_ICount -= 3; RD_ZPG; AND;		  } /* 3 AND ZPG */
OP(45) { int tmp; m6502_ICount -= 3; RD_ZPG; EOR;		  } /* 3 EOR ZPG */
OP(65) { int tmp; m6502_ICount -= 3; RD_ZPG; ADC;		  } /* 3 ADC ZPG */
OP(85) { int tmp; m6502_ICount -= 3;		 STA; WR_ZPG; } /* 3 STA ZPG */
OP(a5) { int tmp; m6502_ICount -= 3; RD_ZPG; LDA;		  } /* 3 LDA ZPG */
OP(c5) { int tmp; m6502_ICount -= 3; RD_ZPG; CMP;		  } /* 3 CMP ZPG */
OP(e5) { int tmp; m6502_ICount -= 3; RD_ZPG; SBC;		  } /* 3 SBC ZPG */

OP(15) { int tmp; m6502_ICount -= 4; RD_ZPX; ORA;		  } /* 4 ORA ZPX */
OP(35) { int tmp; m6502_ICount -= 4; RD_ZPX; AND;		  } /* 4 AND ZPX */
OP(55) { int tmp; m6502_ICount -= 4; RD_ZPX; EOR;		  } /* 4 EOR ZPX */
OP(75) { int tmp; m6502_ICount -= 4; RD_ZPX; ADC;		  } /* 4 ADC ZPX */
OP(95) { int tmp; m6502_ICount -= 4;		 STA; WR_ZPX; } /* 4 STA ZPX */
OP(b5) { int tmp; m6502_ICount -= 4; RD_ZPX; LDA;		  } /* 4 LDA ZPX */
OP(d5) { int tmp; m6502_ICount -= 4; RD_ZPX; CMP;		  } /* 4 CMP ZPX */
OP(f5) { int tmp; m6502_ICount -= 4; RD_ZPX; SBC;		  } /* 4 SBC ZPX */

OP(06) { int tmp; m6502_ICount -= 5; RD_ZPG; ASL; WB_EA;  } /* 5 ASL ZPG */
OP(26) { int tmp; m6502_ICount -= 5; RD_ZPG; ROL; WB_EA;  } /* 5 ROL ZPG */
OP(46) { int tmp; m6502_ICount -= 5; RD_ZPG; LSR; WB_EA;  } /* 5 LSR ZPG */
OP(66) { int tmp; m6502_ICount -= 5; RD_ZPG; ROR; WB_EA;  } /* 5 ROR ZPG */
OP(86) { int tmp; m6502_ICount -= 3;		 STX; WR_ZPG; } /* 3 STX ZPG */
OP(a6) { int tmp; m6502_ICount -= 3; RD_ZPG; LDX;		  } /* 3 LDX ZPG */
OP(c6) { int tmp; m6502_ICount -= 5; RD_ZPG; DEC; WB_EA;  } /* 5 DEC ZPG */
OP(e6) { int tmp; m6502_ICount -= 5; RD_ZPG; INC; WB_EA;  } /* 5 INC ZPG */

OP(16) { int tmp; m6502_ICount -= 6; RD_ZPX; ASL; WB_EA;  } /* 6 ASL ZPX */
OP(36) { int tmp; m6502_ICount -= 6; RD_ZPX; ROL; WB_EA;  } /* 6 ROL ZPX */
OP(56) { int tmp; m6502_ICount -= 6; RD_ZPX; LSR; WB_EA;  } /* 6 LSR ZPX */
OP(76) { int tmp; m6502_ICount -= 6; RD_ZPX; ROR; WB_EA;  } /* 6 ROR ZPX */
OP(96) { int tmp; m6502_ICount -= 4;		 STX; WR_ZPY; } /* 4 STX ZPY */
OP(b6) { int tmp; m6502_ICount -= 4; RD_ZPY; LDX;		  } /* 4 LDX ZPY */
OP(d6) { int tmp; m6502_ICount -= 6; RD_ZPX; DEC; WB_EA;  } /* 6 DEC ZPX */
OP(f6) { int tmp; m6502_ICount -= 6; RD_ZPX; INC; WB_EA;  } /* 6 INC ZPX */

OP(07) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(27) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(47) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(67) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(87) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(a7) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(c7) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(e7) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */

OP(17) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(37) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(57) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(77) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(97) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(b7) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(d7) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(f7) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */

OP(08) {		  m6502_ICount -= 3;		 PHP;		  } /* 2 PHP */
OP(28) {		  m6502_ICount -= 4;		 PLP;		  } /* 2 PLP */
OP(48) {		  m6502_ICount -= 3;		 PHA;		  } /* 2 PHA */
OP(68) {		  m6502_ICount -= 4;		 PLA;		  } /* 2 PLA */
OP(88) {		  m6502_ICount -= 2;		 DEY;		  } /* 2 DEY */
OP(a8) {		  m6502_ICount -= 2;		 TAY;		  } /* 2 TAY */
OP(c8) {		  m6502_ICount -= 2;		 INY;		  } /* 2 INY */
OP(e8) {		  m6502_ICount -= 2;		 INX;		  } /* 2 INX */

OP(18) {		  m6502_ICount -= 2;		 CLC;		  } /* 2 CLC */
OP(38) {		  m6502_ICount -= 2;		 SEC;		  } /* 2 SEC */
OP(58) {		  m6502_ICount -= 2;		 CLI;		  } /* 2 CLI */
OP(78) {		  m6502_ICount -= 2;		 SEI;		  } /* 2 SEI */
OP(98) {		  m6502_ICount -= 2;		 TYA;		  } /* 2 TYA */
OP(b8) {		  m6502_ICount -= 2;		 CLV;		  } /* 2 CLV */
OP(d8) {		  m6502_ICount -= 2;		 CLD;		  } /* 2 CLD */
OP(f8) {		  m6502_ICount -= 2;		 SED;		  } /* 2 SED */

OP(09) { int tmp; m6502_ICount -= 2; RD_IMM; ORA;		  } /* 2 ORA IMM */
OP(29) { int tmp; m6502_ICount -= 2; RD_IMM; AND;		  } /* 2 AND IMM */
OP(49) { int tmp; m6502_ICount -= 2; RD_IMM; EOR;		  } /* 2 EOR IMM */
OP(69) { int tmp; m6502_ICount -= 2; RD_IMM; ADC;		  } /* 2 ADC IMM */
OP(89) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(a9) { int tmp; m6502_ICount -= 2; RD_IMM; LDA;		  } /* 2 LDA IMM */
OP(c9) { int tmp; m6502_ICount -= 2; RD_IMM; CMP;		  } /* 2 CMP IMM */
OP(e9) { int tmp; m6502_ICount -= 2; RD_IMM; SBC;		  } /* 2 SBC IMM */

OP(19) { int tmp; m6502_ICount -= 4; RD_ABY; ORA;		  } /* 4 ORA ABY */
OP(39) { int tmp; m6502_ICount -= 4; RD_ABY; AND;		  } /* 4 AND ABY */
OP(59) { int tmp; m6502_ICount -= 4; RD_ABY; EOR;		  } /* 4 EOR ABY */
OP(79) { int tmp; m6502_ICount -= 4; RD_ABY; ADC;		  } /* 4 ADC ABY */
OP(99) { int tmp; m6502_ICount -= 5;		 STA; WR_ABY; } /* 5 STA ABY */
OP(b9) { int tmp; m6502_ICount -= 4; RD_ABY; LDA;		  } /* 4 LDA ABY */
OP(d9) { int tmp; m6502_ICount -= 4; RD_ABY; CMP;		  } /* 4 CMP ABY */
OP(f9) { int tmp; m6502_ICount -= 4; RD_ABY; SBC;		  } /* 4 SBC ABY */

OP(0a) { int tmp; m6502_ICount -= 2; RD_ACC; ASL; WB_ACC; } /* 2 ASL A */
OP(2a) { int tmp; m6502_ICount -= 2; RD_ACC; ROL; WB_ACC; } /* 2 ROL A */
OP(4a) { int tmp; m6502_ICount -= 2; RD_ACC; LSR; WB_ACC; } /* 2 LSR A */
OP(6a) { int tmp; m6502_ICount -= 2; RD_ACC; ROR; WB_ACC; } /* 2 ROR A */
OP(8a) {		  m6502_ICount -= 2;		 TXA;		  } /* 2 TXA */
OP(aa) {		  m6502_ICount -= 2;		 TAX;		  } /* 2 TAX */
OP(ca) {		  m6502_ICount -= 2;		 DEX;		  } /* 2 DEX */
OP(ea) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */

OP(1a) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(3a) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(5a) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(7a) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(9a) {		  m6502_ICount -= 2;		 TXS;		  } /* 2 TXS */
OP(ba) {		  m6502_ICount -= 2;		 TSX;		  } /* 2 TSX */
OP(da) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(fa) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */

OP(0b) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(2b) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(4b) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(6b) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(8b) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(ab) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(cb) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(eb) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */

OP(1b) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(3b) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(5b) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(7b) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(9b) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(bb) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(db) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(fb) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */

OP(0c) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(2c) { int tmp; m6502_ICount -= 4; RD_ABS; BIT;		  } /* 4 BIT ABS */
OP(4c) {		  m6502_ICount -= 3; EA_ABS; JMP;		  } /* 3 JMP ABS */
OP(6c) { int tmp; m6502_ICount -= 5; EA_IND; JMP;		  } /* 5 JMP IND */
OP(8c) { int tmp; m6502_ICount -= 4;		 STY; WR_ABS; } /* 4 STY ABS */
OP(ac) { int tmp; m6502_ICount -= 4; RD_ABS; LDY;		  } /* 4 LDY ABS */
OP(cc) { int tmp; m6502_ICount -= 4; RD_ABS; CPY;		  } /* 4 CPY ABS */
OP(ec) { int tmp; m6502_ICount -= 4; RD_ABS; CPX;		  } /* 4 CPX ABS */

OP(1c) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(3c) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(5c) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(7c) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(9c) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(bc) { int tmp; m6502_ICount -= 4; RD_ABX; LDY;		  } /* 4 LDY ABX */
OP(dc) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(fc) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */

OP(0d) { int tmp; m6502_ICount -= 4; RD_ABS; ORA;		  } /* 4 ORA ABS */
OP(2d) { int tmp; m6502_ICount -= 4; RD_ABS; AND;		  } /* 4 AND ABS */
OP(4d) { int tmp; m6502_ICount -= 4; RD_ABS; EOR;		  } /* 4 EOR ABS */
OP(6d) { int tmp; m6502_ICount -= 4; RD_ABS; ADC;		  } /* 4 ADC ABS */
OP(8d) { int tmp; m6502_ICount -= 4;		 STA; WR_ABS; } /* 4 STA ABS */
OP(ad) { int tmp; m6502_ICount -= 4; RD_ABS; LDA;		  } /* 4 LDA ABS */
OP(cd) { int tmp; m6502_ICount -= 4; RD_ABS; CMP;		  } /* 4 CMP ABS */
OP(ed) { int tmp; m6502_ICount -= 4; RD_ABS; SBC;		  } /* 4 SBC ABS */

OP(1d) { int tmp; m6502_ICount -= 4; RD_ABX; ORA;		  } /* 4 ORA ABX */
OP(3d) { int tmp; m6502_ICount -= 4; RD_ABX; AND;		  } /* 4 AND ABX */
OP(5d) { int tmp; m6502_ICount -= 4; RD_ABX; EOR;		  } /* 4 EOR ABX */
OP(7d) { int tmp; m6502_ICount -= 4; RD_ABX; ADC;		  } /* 4 ADC ABX */
OP(9d) { int tmp; m6502_ICount -= 5;		 STA; WR_ABX; } /* 5 STA ABX */
OP(bd) { int tmp; m6502_ICount -= 4; RD_ABX; LDA;		  } /* 4 LDA ABX */
OP(dd) { int tmp; m6502_ICount -= 4; RD_ABX; CMP;		  } /* 4 CMP ABX */
OP(fd) { int tmp; m6502_ICount -= 4; RD_ABX; SBC;		  } /* 4 SBC ABX */

OP(0e) { int tmp; m6502_ICount -= 6; RD_ABS; ASL; WB_EA;  } /* 6 ASL ABS */
OP(2e) { int tmp; m6502_ICount -= 6; RD_ABS; ROL; WB_EA;  } /* 6 ROL ABS */
OP(4e) { int tmp; m6502_ICount -= 6; RD_ABS; LSR; WB_EA;  } /* 6 LSR ABS */
OP(6e) { int tmp; m6502_ICount -= 6; RD_ABS; ROR; WB_EA;  } /* 6 ROR ABS */
OP(8e) { int tmp; m6502_ICount -= 5;		 STX; WR_ABS; } /* 5 STX ABS */
OP(ae) { int tmp; m6502_ICount -= 4; RD_ABS; LDX;		  } /* 4 LDX ABS */
OP(ce) { int tmp; m6502_ICount -= 6; RD_ABS; DEC; WB_EA;  } /* 6 DEC ABS */
OP(ee) { int tmp; m6502_ICount -= 6; RD_ABS; INC; WB_EA;  } /* 6 INC ABS */

OP(1e) { int tmp; m6502_ICount -= 7; RD_ABX; ASL; WB_EA;  } /* 7 ASL ABX */
OP(3e) { int tmp; m6502_ICount -= 7; RD_ABX; ROL; WB_EA;  } /* 7 ROL ABX */
OP(5e) { int tmp; m6502_ICount -= 7; RD_ABX; LSR; WB_EA;  } /* 7 LSR ABX */
OP(7e) { int tmp; m6502_ICount -= 7; RD_ABX; ROR; WB_EA;  } /* 7 ROR ABX */
OP(9e) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(be) { int tmp; m6502_ICount -= 4; RD_ABY; LDX;		  } /* 4 LDX ABY */
OP(de) { int tmp; m6502_ICount -= 7; RD_ABX; DEC; WB_EA;  } /* 7 DEC ABX */
OP(fe) { int tmp; m6502_ICount -= 7; RD_ABX; INC; WB_EA;  } /* 7 INC ABX */

OP(0f) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(2f) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(4f) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(6f) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(8f) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(af) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(cf) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(ef) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */

OP(1f) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(3f) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(5f) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(7f) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(9f) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(bf) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(df) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */
OP(ff) {		  m6502_ICount -= 2;		 ILL;		  } /* 2 ILL */

/* and here's the array of function pointers */

static void (*insn6502[0x100])(void) = {
	m6502_00,m6502_01,m6502_02,m6502_03,m6502_04,m6502_05,m6502_06,m6502_07,
	m6502_08,m6502_09,m6502_0a,m6502_0b,m6502_0c,m6502_0d,m6502_0e,m6502_0f,
	m6502_10,m6502_11,m6502_12,m6502_13,m6502_14,m6502_15,m6502_16,m6502_17,
	m6502_18,m6502_19,m6502_1a,m6502_1b,m6502_1c,m6502_1d,m6502_1e,m6502_1f,
	m6502_20,m6502_21,m6502_22,m6502_23,m6502_24,m6502_25,m6502_26,m6502_27,
	m6502_28,m6502_29,m6502_2a,m6502_2b,m6502_2c,m6502_2d,m6502_2e,m6502_2f,
	m6502_30,m6502_31,m6502_32,m6502_33,m6502_34,m6502_35,m6502_36,m6502_37,
	m6502_38,m6502_39,m6502_3a,m6502_3b,m6502_3c,m6502_3d,m6502_3e,m6502_3f,
	m6502_40,m6502_41,m6502_42,m6502_43,m6502_44,m6502_45,m6502_46,m6502_47,
	m6502_48,m6502_49,m6502_4a,m6502_4b,m6502_4c,m6502_4d,m6502_4e,m6502_4f,
	m6502_50,m6502_51,m6502_52,m6502_53,m6502_54,m6502_55,m6502_56,m6502_57,
	m6502_58,m6502_59,m6502_5a,m6502_5b,m6502_5c,m6502_5d,m6502_5e,m6502_5f,
	m6502_60,m6502_61,m6502_62,m6502_63,m6502_64,m6502_65,m6502_66,m6502_67,
	m6502_68,m6502_69,m6502_6a,m6502_6b,m6502_6c,m6502_6d,m6502_6e,m6502_6f,
	m6502_70,m6502_71,m6502_72,m6502_73,m6502_74,m6502_75,m6502_76,m6502_77,
	m6502_78,m6502_79,m6502_7a,m6502_7b,m6502_7c,m6502_7d,m6502_7e,m6502_7f,
	m6502_80,m6502_81,m6502_82,m6502_83,m6502_84,m6502_85,m6502_86,m6502_87,
	m6502_88,m6502_89,m6502_8a,m6502_8b,m6502_8c,m6502_8d,m6502_8e,m6502_8f,
	m6502_90,m6502_91,m6502_92,m6502_93,m6502_94,m6502_95,m6502_96,m6502_97,
	m6502_98,m6502_99,m6502_9a,m6502_9b,m6502_9c,m6502_9d,m6502_9e,m6502_9f,
	m6502_a0,m6502_a1,m6502_a2,m6502_a3,m6502_a4,m6502_a5,m6502_a6,m6502_a7,
	m6502_a8,m6502_a9,m6502_aa,m6502_ab,m6502_ac,m6502_ad,m6502_ae,m6502_af,
	m6502_b0,m6502_b1,m6502_b2,m6502_b3,m6502_b4,m6502_b5,m6502_b6,m6502_b7,
	m6502_b8,m6502_b9,m6502_ba,m6502_bb,m6502_bc,m6502_bd,m6502_be,m6502_bf,
	m6502_c0,m6502_c1,m6502_c2,m6502_c3,m6502_c4,m6502_c5,m6502_c6,m6502_c7,
	m6502_c8,m6502_c9,m6502_ca,m6502_cb,m6502_cc,m6502_cd,m6502_ce,m6502_cf,
	m6502_d0,m6502_d1,m6502_d2,m6502_d3,m6502_d4,m6502_d5,m6502_d6,m6502_d7,
	m6502_d8,m6502_d9,m6502_da,m6502_db,m6502_dc,m6502_dd,m6502_de,m6502_df,
	m6502_e0,m6502_e1,m6502_e2,m6502_e3,m6502_e4,m6502_e5,m6502_e6,m6502_e7,
	m6502_e8,m6502_e9,m6502_ea,m6502_eb,m6502_ec,m6502_ed,m6502_ee,m6502_ef,
	m6502_f0,m6502_f1,m6502_f2,m6502_f3,m6502_f4,m6502_f5,m6502_f6,m6502_f7,
	m6502_f8,m6502_f9,m6502_fa,m6502_fb,m6502_fc,m6502_fd,m6502_fe,m6502_ff
};
