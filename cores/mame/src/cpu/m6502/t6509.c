/*****************************************************************************
 *
 *	 tbl6509.c
 *	 6509 opcode functions and function pointer table
 *
 *	 Copyright (c) 2000 Peter Trauner, all rights reserved.
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
 *	 - Opcode information based on an Intel 386 '6510.asm' core
 *	   written by R.F. van Ee (1993)
 *	 - Cycle counts are guesswork :-)
 *
 *****************************************************************************/

#undef	OP
#define OP(nn) static INLINE void m6509_##nn(void)

OP(00) {		  m6509_ICount -= 7;		 BRK;		  } /* 7 BRK */
OP(20) {		  m6509_ICount -= 6;		 JSR;		  } /* 6 JSR */
OP(40) {		  m6509_ICount -= 6;		 RTI;		  } /* 6 RTI */
OP(60) {		  m6509_ICount -= 6;		 RTS;		  } /* 6 RTS */
OP(80) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(a0) { int tmp; m6509_ICount -= 2; RD_IMM; LDY;		  } /* 2 LDY IMM */
OP(c0) { int tmp; m6509_ICount -= 2; RD_IMM; CPY;		  } /* 2 CPY IMM */
OP(e0) { int tmp; m6509_ICount -= 2; RD_IMM; CPX;		  } /* 2 CPX IMM */

OP(10) { int tmp;							 BPL;		  } /* 2 BPL REL */
OP(30) { int tmp;							 BMI;		  } /* 2 BMI REL */
OP(50) { int tmp;							 BVC;		  } /* 2 BVC REL */
OP(70) { int tmp;							 BVS;		  } /* 2 BVS REL */
OP(90) { int tmp;							 BCC;		  } /* 2 BCC REL */
OP(b0) { int tmp;							 BCS;		  } /* 2 BCS REL */
OP(d0) { int tmp;							 BNE;		  } /* 2 BNE REL */
OP(f0) { int tmp;							 BEQ;		  } /* 2 BEQ REL */

OP(01) { int tmp; m6509_ICount -= 6; RD_IDX; ORA;		  } /* 6 ORA IDX */
OP(21) { int tmp; m6509_ICount -= 6; RD_IDX; AND;		  } /* 6 AND IDX */
OP(41) { int tmp; m6509_ICount -= 6; RD_IDX; EOR;		  } /* 6 EOR IDX */
OP(61) { int tmp; m6509_ICount -= 6; RD_IDX; ADC;		  } /* 6 ADC IDX */
OP(81) { int tmp; m6509_ICount -= 6;		 STA; WR_IDX; } /* 6 STA IDX */
OP(a1) { int tmp; m6509_ICount -= 6; RD_IDX; LDA;		  } /* 6 LDA IDX */
OP(c1) { int tmp; m6509_ICount -= 6; RD_IDX; CMP;		  } /* 6 CMP IDX */
OP(e1) { int tmp; m6509_ICount -= 6; RD_IDX; SBC;		  } /* 6 SBC IDX */

OP(11) { int tmp; m6509_ICount -= 5; RD_IDY; ORA;		  } /* 5 ORA IDY */
OP(31) { int tmp; m6509_ICount -= 5; RD_IDY; AND;		  } /* 5 AND IDY */
OP(51) { int tmp; m6509_ICount -= 5; RD_IDY; EOR;		  } /* 5 EOR IDY */
OP(71) { int tmp; m6509_ICount -= 5; RD_IDY; ADC;		  } /* 5 ADC IDY */
OP(91) { int tmp; m6509_ICount -= 6;		 STA; WR_IDY_6509; } /* 6 STA IDY */
OP(b1) { int tmp; m6509_ICount -= 5; RD_IDY_6509; LDA;	  } /* 5 LDA IDY */
OP(d1) { int tmp; m6509_ICount -= 5; RD_IDY; CMP;		  } /* 5 CMP IDY */
OP(f1) { int tmp; m6509_ICount -= 5; RD_IDY; SBC;		  } /* 5 SBC IDY */

OP(02) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(22) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(42) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(62) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(82) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(a2) { int tmp; m6509_ICount -= 2; RD_IMM; LDX;		  } /* 2 LDX IMM */
OP(c2) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(e2) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */

OP(12) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(32) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(52) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(72) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(92) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(b2) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(d2) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(f2) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */

OP(03) { int tmp; m6502_ICount -= 7; RD_IDX; SLO; WB_EA;  } /* 7 SLO IDX */
OP(23) { int tmp; m6502_ICount -= 7; RD_IDX; RLA; WB_EA;  } /* 7 RLA IDX */
OP(43) { int tmp; m6502_ICount -= 7; RD_IDX; SRE; WB_EA;  } /* 7 SRE IDX */
OP(63) { int tmp; m6502_ICount -= 7; RD_IDX; RRA; WB_EA;  } /* 7 RRA IDX */
OP(83) { int tmp; m6502_ICount -= 6;		 SAX; WR_IDX; } /* 6 SAX IDX */
OP(a3) { int tmp; m6502_ICount -= 6; RD_IDX; LAX;		  } /* 6 LAX IDX */
OP(c3) { int tmp; m6502_ICount -= 7; RD_IDX; DCP; WB_EA;  } /* 7 DCP IDX */
OP(e3) { int tmp; m6502_ICount -= 7; RD_IDX; ISB; WB_EA;  } /* 7 ISB IDX */

OP(13) { int tmp; m6502_ICount -= 6; RD_IDY; SLO; WB_EA;  } /* 6 SLO IDY */
OP(33) { int tmp; m6502_ICount -= 6; RD_IDY; RLA; WB_EA;  } /* 6 RLA IDY */
OP(53) { int tmp; m6502_ICount -= 6; RD_IDY; SRE; WB_EA;  } /* 6 SRE IDY */
OP(73) { int tmp; m6502_ICount -= 6; RD_IDY; RRA; WB_EA;  } /* 6 RRA IDY */
OP(93) { int tmp; m6502_ICount -= 5; EA_IDY; SAH; WB_EA;  } /* 5 SAH IDY */
OP(b3) { int tmp; m6502_ICount -= 5; RD_IDY; LAX;		  } /* 5 LAX IDY */
OP(d3) { int tmp; m6502_ICount -= 6; RD_IDY; DCP; WB_EA;  } /* 6 DCP IDY */
OP(f3) { int tmp; m6502_ICount -= 6; RD_IDY; ISB; WB_EA;  } /* 6 ISB IDY */

OP(04) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(24) { int tmp; m6509_ICount -= 3; RD_ZPG; BIT;		  } /* 3 BIT ZPG */
OP(44) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(64) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(84) { int tmp; m6509_ICount -= 3;		 STY; WR_ZPG; } /* 3 STY ZPG */
OP(a4) { int tmp; m6509_ICount -= 3; RD_ZPG; LDY;		  } /* 3 LDY ZPG */
OP(c4) { int tmp; m6509_ICount -= 3; RD_ZPG; CPY;		  } /* 3 CPY ZPG */
OP(e4) { int tmp; m6509_ICount -= 3; RD_ZPG; CPX;		  } /* 3 CPX ZPG */

OP(14) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(34) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(54) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(74) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(94) { int tmp; m6509_ICount -= 4;		 STY; WR_ZPX; } /* 4 STY ZPX */
OP(b4) { int tmp; m6509_ICount -= 4; RD_ZPX; LDY;		  } /* 4 LDY ZPX */
OP(d4) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(f4) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */

OP(05) { int tmp; m6509_ICount -= 3; RD_ZPG; ORA;		  } /* 3 ORA ZPG */
OP(25) { int tmp; m6509_ICount -= 3; RD_ZPG; AND;		  } /* 3 AND ZPG */
OP(45) { int tmp; m6509_ICount -= 3; RD_ZPG; EOR;		  } /* 3 EOR ZPG */
OP(65) { int tmp; m6509_ICount -= 3; RD_ZPG; ADC;		  } /* 3 ADC ZPG */
OP(85) { int tmp; m6509_ICount -= 3;		 STA; WR_ZPG; } /* 3 STA ZPG */
OP(a5) { int tmp; m6509_ICount -= 3; RD_ZPG; LDA;		  } /* 3 LDA ZPG */
OP(c5) { int tmp; m6509_ICount -= 3; RD_ZPG; CMP;		  } /* 3 CMP ZPG */
OP(e5) { int tmp; m6509_ICount -= 3; RD_ZPG; SBC;		  } /* 3 SBC ZPG */

OP(15) { int tmp; m6509_ICount -= 4; RD_ZPX; ORA;		  } /* 4 ORA ZPX */
OP(35) { int tmp; m6509_ICount -= 4; RD_ZPX; AND;		  } /* 4 AND ZPX */
OP(55) { int tmp; m6509_ICount -= 4; RD_ZPX; EOR;		  } /* 4 EOR ZPX */
OP(75) { int tmp; m6509_ICount -= 4; RD_ZPX; ADC;		  } /* 4 ADC ZPX */
OP(95) { int tmp; m6509_ICount -= 4;		 STA; WR_ZPX; } /* 4 STA ZPX */
OP(b5) { int tmp; m6509_ICount -= 4; RD_ZPX; LDA;		  } /* 4 LDA ZPX */
OP(d5) { int tmp; m6509_ICount -= 4; RD_ZPX; CMP;		  } /* 4 CMP ZPX */
OP(f5) { int tmp; m6509_ICount -= 4; RD_ZPX; SBC;		  } /* 4 SBC ZPX */

OP(06) { int tmp; m6509_ICount -= 5; RD_ZPG; ASL; WB_EA;  } /* 5 ASL ZPG */
OP(26) { int tmp; m6509_ICount -= 5; RD_ZPG; ROL; WB_EA;  } /* 5 ROL ZPG */
OP(46) { int tmp; m6509_ICount -= 5; RD_ZPG; LSR; WB_EA;  } /* 5 LSR ZPG */
OP(66) { int tmp; m6509_ICount -= 5; RD_ZPG; ROR; WB_EA;  } /* 5 ROR ZPG */
OP(86) { int tmp; m6509_ICount -= 3;		 STX; WR_ZPG; } /* 3 STX ZPG */
OP(a6) { int tmp; m6509_ICount -= 3; RD_ZPG; LDX;		  } /* 3 LDX ZPG */
OP(c6) { int tmp; m6509_ICount -= 5; RD_ZPG; DEC; WB_EA;  } /* 5 DEC ZPG */
OP(e6) { int tmp; m6509_ICount -= 5; RD_ZPG; INC; WB_EA;  } /* 5 INC ZPG */

OP(16) { int tmp; m6509_ICount -= 6; RD_ZPX; ASL; WB_EA;  } /* 6 ASL ZPX */
OP(36) { int tmp; m6509_ICount -= 6; RD_ZPX; ROL; WB_EA;  } /* 6 ROL ZPX */
OP(56) { int tmp; m6509_ICount -= 6; RD_ZPX; LSR; WB_EA;  } /* 6 LSR ZPX */
OP(76) { int tmp; m6509_ICount -= 6; RD_ZPX; ROR; WB_EA;  } /* 6 ROR ZPX */
OP(96) { int tmp; m6509_ICount -= 4;		 STX; WR_ZPY; } /* 4 STX ZPY */
OP(b6) { int tmp; m6509_ICount -= 4; RD_ZPY; LDX;		  } /* 4 LDX ZPY */
OP(d6) { int tmp; m6509_ICount -= 6; RD_ZPX; DEC; WB_EA;  } /* 6 DEC ZPX */
OP(f6) { int tmp; m6509_ICount -= 6; RD_ZPX; INC; WB_EA;  } /* 6 INC ZPX */

OP(07) { int tmp; m6502_ICount -= 5; RD_ZPG; SLO; WB_EA;  } /* 5 SLO ZPG */
OP(27) { int tmp; m6502_ICount -= 5; RD_ZPG; RLA; WB_EA;  } /* 5 RLA ZPG */
OP(47) { int tmp; m6502_ICount -= 5; RD_ZPG; SRE; WB_EA;  } /* 5 SRE ZPG */
OP(67) { int tmp; m6502_ICount -= 5; RD_ZPG; RRA; WB_EA;  } /* 5 RRA ZPG */
OP(87) { int tmp; m6502_ICount -= 3;		 SAX; WR_ZPG; } /* 3 SAX ZPG */
OP(a7) { int tmp; m6502_ICount -= 3; RD_ZPG; LAX;		  } /* 3 LAX ZPG */
OP(c7) { int tmp; m6502_ICount -= 5; RD_ZPG; DCP; WB_EA;  } /* 5 DCP ZPG */
OP(e7) { int tmp; m6502_ICount -= 5; RD_ZPG; ISB; WB_EA;  } /* 5 ISB ZPG */

OP(17) { int tmp; m6502_ICount -= 6; RD_ZPX; SLO; WB_EA;  } /* 4 SLO ZPX */
OP(37) { int tmp; m6502_ICount -= 6; RD_ZPX; RLA; WB_EA;  } /* 4 RLA ZPX */
OP(57) { int tmp; m6502_ICount -= 6; RD_ZPX; SRE; WB_EA;  } /* 4 SRE ZPX */
OP(77) { int tmp; m6502_ICount -= 6; RD_ZPX; RRA; WB_EA;  } /* 4 RRA ZPX */
OP(97) { int tmp; m6502_ICount -= 4;		 SAX; WR_ZPY; } /* 4 SAX ZPY */
OP(b7) { int tmp; m6502_ICount -= 4; RD_ZPY; LAX;		  } /* 4 LAX ZPY */
OP(d7) { int tmp; m6502_ICount -= 6; RD_ZPX; DCP; WB_EA;  } /* 6 DCP ZPX */
OP(f7) { int tmp; m6502_ICount -= 6; RD_ZPX; ISB; WB_EA;  } /* 6 ISB ZPX */

OP(08) {		  m6509_ICount -= 2;		 PHP;		  } /* 2 PHP */
OP(28) {		  m6509_ICount -= 2;		 PLP;		  } /* 2 PLP */
OP(48) {		  m6509_ICount -= 2;		 PHA;		  } /* 2 PHA */
OP(68) {		  m6509_ICount -= 2;		 PLA;		  } /* 2 PLA */
OP(88) {		  m6509_ICount -= 2;		 DEY;		  } /* 2 DEY */
OP(a8) {		  m6509_ICount -= 2;		 TAY;		  } /* 2 TAY */
OP(c8) {		  m6509_ICount -= 2;		 INY;		  } /* 2 INY */
OP(e8) {		  m6509_ICount -= 2;		 INX;		  } /* 2 INX */

OP(18) {		  m6509_ICount -= 2;		 CLC;		  } /* 2 CLC */
OP(38) {		  m6509_ICount -= 2;		 SEC;		  } /* 2 SEC */
OP(58) {		  m6509_ICount -= 2;		 CLI;		  } /* 2 CLI */
OP(78) {		  m6509_ICount -= 2;		 SEI;		  } /* 2 SEI */
OP(98) {		  m6509_ICount -= 2;		 TYA;		  } /* 2 TYA */
OP(b8) {		  m6509_ICount -= 2;		 CLV;		  } /* 2 CLV */
OP(d8) {		  m6509_ICount -= 2;		 CLD;		  } /* 2 CLD */
OP(f8) {		  m6509_ICount -= 2;		 SED;		  } /* 2 SED */

OP(09) { int tmp; m6509_ICount -= 2; RD_IMM; ORA;		  } /* 2 ORA IMM */
OP(29) { int tmp; m6509_ICount -= 2; RD_IMM; AND;		  } /* 2 AND IMM */
OP(49) { int tmp; m6509_ICount -= 2; RD_IMM; EOR;		  } /* 2 EOR IMM */
OP(69) { int tmp; m6509_ICount -= 2; RD_IMM; ADC;		  } /* 2 ADC IMM */
OP(89) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(a9) { int tmp; m6509_ICount -= 2; RD_IMM; LDA;		  } /* 2 LDA IMM */
OP(c9) { int tmp; m6509_ICount -= 2; RD_IMM; CMP;		  } /* 2 CMP IMM */
OP(e9) { int tmp; m6509_ICount -= 2; RD_IMM; SBC;		  } /* 2 SBC IMM */

OP(19) { int tmp; m6509_ICount -= 4; RD_ABY; ORA;		  } /* 4 ORA ABY */
OP(39) { int tmp; m6509_ICount -= 4; RD_ABY; AND;		  } /* 4 AND ABY */
OP(59) { int tmp; m6509_ICount -= 4; RD_ABY; EOR;		  } /* 4 EOR ABY */
OP(79) { int tmp; m6509_ICount -= 4; RD_ABY; ADC;		  } /* 4 ADC ABY */
OP(99) { int tmp; m6509_ICount -= 5;		 STA; WR_ABY; } /* 5 STA ABY */
OP(b9) { int tmp; m6509_ICount -= 4; RD_ABY; LDA;		  } /* 4 LDA ABY */
OP(d9) { int tmp; m6509_ICount -= 4; RD_ABY; CMP;		  } /* 4 CMP ABY */
OP(f9) { int tmp; m6509_ICount -= 4; RD_ABY; SBC;		  } /* 4 SBC ABY */

OP(0a) { int tmp; m6509_ICount -= 2; RD_ACC; ASL; WB_ACC; } /* 2 ASL A */
OP(2a) { int tmp; m6509_ICount -= 2; RD_ACC; ROL; WB_ACC; } /* 2 ROL A */
OP(4a) { int tmp; m6509_ICount -= 2; RD_ACC; LSR; WB_ACC; } /* 2 LSR A */
OP(6a) { int tmp; m6509_ICount -= 2; RD_ACC; ROR; WB_ACC; } /* 2 ROR A */
OP(8a) {		  m6509_ICount -= 2;		 TXA;		  } /* 2 TXA */
OP(aa) {		  m6509_ICount -= 2;		 TAX;		  } /* 2 TAX */
OP(ca) {		  m6509_ICount -= 2;		 DEX;		  } /* 2 DEX */
OP(ea) {		  m6509_ICount -= 2;		 NOP;		  } /* 2 NOP */

OP(1a) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */
OP(3a) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */
OP(5a) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */
OP(7a) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */
OP(9a) {		  m6509_ICount -= 2;		 TXS;		  } /* 2 TXS */
OP(ba) {		  m6509_ICount -= 2;		 TSX;		  } /* 2 TSX */
OP(da) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */
OP(fa) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */

OP(0b) { int tmp; m6502_ICount -= 2; RD_IMM; ANC;		  } /* 2 ANC IMM */
OP(2b) { int tmp; m6502_ICount -= 2; RD_IMM; ANC;		  } /* 2 ANC IMM */
OP(4b) { int tmp; m6502_ICount -= 2; RD_IMM; ASR; WB_ACC; } /* 2 ASR IMM */
OP(6b) { int tmp; m6502_ICount -= 2; RD_IMM; ARR; WB_ACC; } /* 2 ARR IMM */
OP(8b) { int tmp; m6502_ICount -= 2; RD_IMM; AXA;         } /* 2 AXA IMM */
OP(ab) { int tmp; m6502_ICount -= 2; RD_IMM; OAL;		  } /* 2 OAL IMM */
OP(cb) { int tmp; m6502_ICount -= 2; RD_IMM; ASX;		  } /* 2 ASX IMM */
OP(eb) { int tmp; m6502_ICount -= 2; RD_IMM; SBC;		  } /* 2 SBC IMM */

OP(1b) { int tmp; m6502_ICount -= 4; RD_ABY; SLO; WB_EA;  } /* 4 SLO ABY */
OP(3b) { int tmp; m6502_ICount -= 4; RD_ABY; RLA; WB_EA;  } /* 4 RLA ABY */
OP(5b) { int tmp; m6502_ICount -= 4; RD_ABY; SRE; WB_EA;  } /* 4 SRE ABY */
OP(7b) { int tmp; m6502_ICount -= 4; RD_ABY; RRA; WB_EA;  } /* 4 RRA ABY */
OP(9b) { int tmp; m6502_ICount -= 5; EA_ABY; SSH; WB_EA;  } /* 5 SSH ABY */
OP(bb) { int tmp; m6502_ICount -= 4; RD_ABY; AST;		  } /* 4 AST ABY */
OP(db) { int tmp; m6502_ICount -= 6; RD_ABY; DCP; WB_EA;  } /* 6 DCP ABY */
OP(fb) { int tmp; m6502_ICount -= 6; RD_ABY; ISB; WB_EA;  } /* 6 ISB ABY */

OP(0c) {		  m6502_ICount -= 2;		 TOP;		  } /* 2 TOP */
OP(2c) { int tmp; m6509_ICount -= 4; RD_ABS; BIT;		  } /* 4 BIT ABS */
OP(4c) {		  m6509_ICount -= 3; EA_ABS; JMP;		  } /* 3 JMP ABS */
OP(6c) { int tmp; m6509_ICount -= 5; EA_IND; JMP;		  } /* 5 JMP IND */
OP(8c) { int tmp; m6509_ICount -= 4;		 STY; WR_ABS; } /* 4 STY ABS */
OP(ac) { int tmp; m6509_ICount -= 4; RD_ABS; LDY;		  } /* 4 LDY ABS */
OP(cc) { int tmp; m6509_ICount -= 4; RD_ABS; CPY;		  } /* 4 CPY ABS */
OP(ec) { int tmp; m6509_ICount -= 4; RD_ABS; CPX;		  } /* 4 CPX ABS */

OP(1c) {		  m6502_ICount -= 2;		 TOP;		  } /* 2 TOP */
OP(3c) {		  m6502_ICount -= 2;		 TOP;		  } /* 2 TOP */
OP(5c) {		  m6502_ICount -= 2;		 TOP;		  } /* 2 TOP */
OP(7c) {		  m6502_ICount -= 2;		 TOP;		  } /* 2 TOP */
OP(9c) { int tmp; m6502_ICount -= 5; EA_ABX; SYH; WB_EA;  } /* 5 SYH ABX */
OP(bc) { int tmp; m6509_ICount -= 4; RD_ABX; LDY;		  } /* 4 LDY ABX */
OP(dc) {		  m6502_ICount -= 2;		 TOP;		  } /* 2 TOP */
OP(fc) {		  m6502_ICount -= 2;		 TOP;		  } /* 2 TOP */

OP(0d) { int tmp; m6509_ICount -= 4; RD_ABS; ORA;		  } /* 4 ORA ABS */
OP(2d) { int tmp; m6509_ICount -= 4; RD_ABS; AND;		  } /* 4 AND ABS */
OP(4d) { int tmp; m6509_ICount -= 4; RD_ABS; EOR;		  } /* 4 EOR ABS */
OP(6d) { int tmp; m6509_ICount -= 4; RD_ABS; ADC;		  } /* 4 ADC ABS */
OP(8d) { int tmp; m6509_ICount -= 4;		 STA; WR_ABS; } /* 4 STA ABS */
OP(ad) { int tmp; m6509_ICount -= 4; RD_ABS; LDA;		  } /* 4 LDA ABS */
OP(cd) { int tmp; m6509_ICount -= 4; RD_ABS; CMP;		  } /* 4 CMP ABS */
OP(ed) { int tmp; m6509_ICount -= 4; RD_ABS; SBC;		  } /* 4 SBC ABS */

OP(1d) { int tmp; m6509_ICount -= 4; RD_ABX; ORA;		  } /* 4 ORA ABX */
OP(3d) { int tmp; m6509_ICount -= 4; RD_ABX; AND;		  } /* 4 AND ABX */
OP(5d) { int tmp; m6509_ICount -= 4; RD_ABX; EOR;		  } /* 4 EOR ABX */
OP(7d) { int tmp; m6509_ICount -= 4; RD_ABX; ADC;		  } /* 4 ADC ABX */
OP(9d) { int tmp; m6509_ICount -= 5;		 STA; WR_ABX; } /* 5 STA ABX */
OP(bd) { int tmp; m6509_ICount -= 4; RD_ABX; LDA;		  } /* 4 LDA ABX */
OP(dd) { int tmp; m6509_ICount -= 4; RD_ABX; CMP;		  } /* 4 CMP ABX */
OP(fd) { int tmp; m6509_ICount -= 4; RD_ABX; SBC;		  } /* 4 SBC ABX */

OP(0e) { int tmp; m6509_ICount -= 6; RD_ABS; ASL; WB_EA;  } /* 6 ASL ABS */
OP(2e) { int tmp; m6509_ICount -= 6; RD_ABS; ROL; WB_EA;  } /* 6 ROL ABS */
OP(4e) { int tmp; m6509_ICount -= 6; RD_ABS; LSR; WB_EA;  } /* 6 LSR ABS */
OP(6e) { int tmp; m6509_ICount -= 6; RD_ABS; ROR; WB_EA;  } /* 6 ROR ABS */
OP(8e) { int tmp; m6509_ICount -= 5;		 STX; WR_ABS; } /* 5 STX ABS */
OP(ae) { int tmp; m6509_ICount -= 4; RD_ABS; LDX;		  } /* 4 LDX ABS */
OP(ce) { int tmp; m6509_ICount -= 6; RD_ABS; DEC; WB_EA;  } /* 6 DEC ABS */
OP(ee) { int tmp; m6509_ICount -= 6; RD_ABS; INC; WB_EA;  } /* 6 INC ABS */

OP(1e) { int tmp; m6509_ICount -= 7; RD_ABX; ASL; WB_EA;  } /* 7 ASL ABX */
OP(3e) { int tmp; m6509_ICount -= 7; RD_ABX; ROL; WB_EA;  } /* 7 ROL ABX */
OP(5e) { int tmp; m6509_ICount -= 7; RD_ABX; LSR; WB_EA;  } /* 7 LSR ABX */
OP(7e) { int tmp; m6509_ICount -= 7; RD_ABX; ROR; WB_EA;  } /* 7 ROR ABX */
OP(9e) { int tmp; m6502_ICount -= 2; EA_ABY; SXH; WB_EA;  } /* 2 SXH ABY */
OP(be) { int tmp; m6509_ICount -= 4; RD_ABY; LDX;		  } /* 4 LDX ABY */
OP(de) { int tmp; m6509_ICount -= 7; RD_ABX; DEC; WB_EA;  } /* 7 DEC ABX */
OP(fe) { int tmp; m6509_ICount -= 7; RD_ABX; INC; WB_EA;  } /* 7 INC ABX */

OP(0f) { int tmp; m6502_ICount -= 6; RD_ABS; SLO; WB_EA;  } /* 4 SLO ABS */
OP(2f) { int tmp; m6502_ICount -= 6; RD_ABS; RLA; WB_EA;  } /* 4 RLA ABS */
OP(4f) { int tmp; m6502_ICount -= 6; RD_ABS; SRE; WB_EA;  } /* 4 SRE ABS */
OP(6f) { int tmp; m6502_ICount -= 6; RD_ABS; RRA; WB_EA;  } /* 4 RRA ABS */
OP(8f) { int tmp; m6502_ICount -= 4;		 SAX; WR_ABS; } /* 4 SAX ABS */
OP(af) { int tmp; m6502_ICount -= 5; RD_ABS; LAX;		  } /* 4 LAX ABS */
OP(cf) { int tmp; m6502_ICount -= 6; RD_ABS; DCP; WB_EA;  } /* 6 DCP ABS */
OP(ef) { int tmp; m6502_ICount -= 6; RD_ABS; ISB; WB_EA;  } /* 6 ISB ABS */

OP(1f) { int tmp; m6502_ICount -= 4; RD_ABX; SLO; WB_EA;  } /* 4 SLO ABX */
OP(3f) { int tmp; m6502_ICount -= 4; RD_ABX; RLA; WB_EA;  } /* 4 RLA ABX */
OP(5f) { int tmp; m6502_ICount -= 4; RD_ABX; SRE; WB_EA;  } /* 4 SRE ABX */
OP(7f) { int tmp; m6502_ICount -= 4; RD_ABX; RRA; WB_EA;  } /* 4 RRA ABX */
OP(9f) { int tmp; m6502_ICount -= 6; EA_ABY; SAH; WB_EA;  } /* 5 SAH ABY */
OP(bf) { int tmp; m6502_ICount -= 6; RD_ABY; LAX;		  } /* 4 LAX ABY */
OP(df) { int tmp; m6502_ICount -= 7; RD_ABX; DCP; WB_EA;  } /* 7 DCP ABX */
OP(ff) { int tmp; m6502_ICount -= 7; RD_ABX; ISB; WB_EA;  } /* 7 ISB ABX */

static void (*insn6509[0x100])(void) = {
	m6509_00,m6509_01,m6509_02,m6509_03,m6509_04,m6509_05,m6509_06,m6509_07,
	m6509_08,m6509_09,m6509_0a,m6509_0b,m6509_0c,m6509_0d,m6509_0e,m6509_0f,
	m6509_10,m6509_11,m6509_12,m6509_13,m6509_14,m6509_15,m6509_16,m6509_17,
	m6509_18,m6509_19,m6509_1a,m6509_1b,m6509_1c,m6509_1d,m6509_1e,m6509_1f,
	m6509_20,m6509_21,m6509_22,m6509_23,m6509_24,m6509_25,m6509_26,m6509_27,
	m6509_28,m6509_29,m6509_2a,m6509_2b,m6509_2c,m6509_2d,m6509_2e,m6509_2f,
	m6509_30,m6509_31,m6509_32,m6509_33,m6509_34,m6509_35,m6509_36,m6509_37,
	m6509_38,m6509_39,m6509_3a,m6509_3b,m6509_3c,m6509_3d,m6509_3e,m6509_3f,
	m6509_40,m6509_41,m6509_42,m6509_43,m6509_44,m6509_45,m6509_46,m6509_47,
	m6509_48,m6509_49,m6509_4a,m6509_4b,m6509_4c,m6509_4d,m6509_4e,m6509_4f,
	m6509_50,m6509_51,m6509_52,m6509_53,m6509_54,m6509_55,m6509_56,m6509_57,
	m6509_58,m6509_59,m6509_5a,m6509_5b,m6509_5c,m6509_5d,m6509_5e,m6509_5f,
	m6509_60,m6509_61,m6509_62,m6509_63,m6509_64,m6509_65,m6509_66,m6509_67,
	m6509_68,m6509_69,m6509_6a,m6509_6b,m6509_6c,m6509_6d,m6509_6e,m6509_6f,
	m6509_70,m6509_71,m6509_72,m6509_73,m6509_74,m6509_75,m6509_76,m6509_77,
	m6509_78,m6509_79,m6509_7a,m6509_7b,m6509_7c,m6509_7d,m6509_7e,m6509_7f,
	m6509_80,m6509_81,m6509_82,m6509_83,m6509_84,m6509_85,m6509_86,m6509_87,
	m6509_88,m6509_89,m6509_8a,m6509_8b,m6509_8c,m6509_8d,m6509_8e,m6509_8f,
	m6509_90,m6509_91,m6509_92,m6509_93,m6509_94,m6509_95,m6509_96,m6509_97,
	m6509_98,m6509_99,m6509_9a,m6509_9b,m6509_9c,m6509_9d,m6509_9e,m6509_9f,
	m6509_a0,m6509_a1,m6509_a2,m6509_a3,m6509_a4,m6509_a5,m6509_a6,m6509_a7,
	m6509_a8,m6509_a9,m6509_aa,m6509_ab,m6509_ac,m6509_ad,m6509_ae,m6509_af,
	m6509_b0,m6509_b1,m6509_b2,m6509_b3,m6509_b4,m6509_b5,m6509_b6,m6509_b7,
	m6509_b8,m6509_b9,m6509_ba,m6509_bb,m6509_bc,m6509_bd,m6509_be,m6509_bf,
	m6509_c0,m6509_c1,m6509_c2,m6509_c3,m6509_c4,m6509_c5,m6509_c6,m6509_c7,
	m6509_c8,m6509_c9,m6509_ca,m6509_cb,m6509_cc,m6509_cd,m6509_ce,m6509_cf,
	m6509_d0,m6509_d1,m6509_d2,m6509_d3,m6509_d4,m6509_d5,m6509_d6,m6509_d7,
	m6509_d8,m6509_d9,m6509_da,m6509_db,m6509_dc,m6509_dd,m6509_de,m6509_df,
	m6509_e0,m6509_e1,m6509_e2,m6509_e3,m6509_e4,m6509_e5,m6509_e6,m6509_e7,
	m6509_e8,m6509_e9,m6509_ea,m6509_eb,m6509_ec,m6509_ed,m6509_ee,m6509_ef,
	m6509_f0,m6509_f1,m6509_f2,m6509_f3,m6509_f4,m6509_f5,m6509_f6,m6509_f7,
	m6509_f8,m6509_f9,m6509_fa,m6509_fb,m6509_fc,m6509_fd,m6509_fe,m6509_ff
};
