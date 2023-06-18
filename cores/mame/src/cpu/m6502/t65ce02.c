/*****************************************************************************
 *
 *	 t65ce02.c
 *	 65ce02 opcode functions and function pointer table
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
 *****************************************************************************/

#undef	OP
#ifdef M4510
#define OP(nn) static INLINE void m4510_##nn(void)
#else
#define OP(nn) static INLINE void m65ce02_##nn(void)
#endif

/*****************************************************************************
 *****************************************************************************
 *
 *	 overrides for 65CE02 opcodes
 *
 *****************************************************************************
 * op	 temp	  cycles			 rdmem	 opc  wrmem   ********************/
OP(00) {		  m65ce02_ICount-=7;		 BRK;		  } /* 7 BRK */
OP(20) {		  m65ce02_ICount-=5;		 JSR;		  } /* 6 JSR */
OP(40) {		  m65ce02_ICount-=5;		 RTI;		  } /* 6 RTI */
OP(60) {		  m65ce02_ICount-=4;		 RTS;		  } /* 6 RTS */
OP(80) { int tmp; m65ce02_ICount-=2;		 BRA(1);	  } /* 2 BRA */
OP(a0) { int tmp; m65ce02_ICount-=2; RD_IMM; LDY;		  } /* 2 LDY IMM */
OP(c0) { int tmp; m65ce02_ICount-=2; RD_IMM; CPY;		  } /* 2 CPY IMM */
OP(e0) { int tmp; m65ce02_ICount-=2; RD_IMM; CPX;		  } /* 2 CPX IMM */

OP(10) { int tmp; m65ce02_ICount-=2;		 BPL;		  } /* 2 BPL REL */
OP(30) { int tmp; m65ce02_ICount-=2;		 BMI;		  } /* 2 BMI REL */
OP(50) { int tmp; m65ce02_ICount-=2;		 BVC;		  } /* 2 BVC REL */
OP(70) { int tmp; m65ce02_ICount-=2;		 BVS;		  } /* 2 BVS REL */
OP(90) { int tmp; m65ce02_ICount-=2;		 BCC;		  } /* 2 BCC REL */
OP(b0) { int tmp; m65ce02_ICount-=2;		 BCS;		  } /* 2 BCS REL */
OP(d0) { int tmp; m65ce02_ICount-=2;		 BNE;		  } /* 2 BNE REL */
OP(f0) { int tmp; m65ce02_ICount-=2;		 BEQ;		  } /* 2 BEQ REL */

OP(01) { int tmp; m65ce02_ICount-=5; RD_IDX; ORA;		  } /* 6 ORA IDX */
OP(21) { int tmp; m65ce02_ICount-=5; RD_IDX; AND;		  } /* 6 AND IDX */
OP(41) { int tmp; m65ce02_ICount-=5; RD_IDX; EOR;		  } /* 6 EOR IDX */
OP(61) { int tmp; m65ce02_ICount-=5; RD_IDX; ADC;		  } /* 6 ADC IDX */
OP(81) { int tmp; m65ce02_ICount-=5;		 STA; WR_IDX; } /* 6 STA IDX */
OP(a1) { int tmp; m65ce02_ICount-=5; RD_IDX; LDA;		  } /* 6 LDA IDX */
OP(c1) { int tmp; m65ce02_ICount-=5; RD_IDX; CMP;		  } /* 6 CMP IDX */
OP(e1) { int tmp; m65ce02_ICount-=5; RD_IDX; SBC;		  } /* 6 SBC IDX */

OP(11) { int tmp; m65ce02_ICount-=5; RD_IDY; ORA;		  } /* 5 ORA IDY */
OP(31) { int tmp; m65ce02_ICount-=5; RD_IDY; AND;		  } /* 5 AND IDY */
OP(51) { int tmp; m65ce02_ICount-=5; RD_IDY; EOR;		  } /* 5 EOR IDY */
OP(71) { int tmp; m65ce02_ICount-=5; RD_IDY; ADC;		  } /* 5 ADC IDY */
OP(91) { int tmp; m65ce02_ICount-=5;		 STA; WR_IDY; } /* 6 STA IDY */
OP(b1) { int tmp; m65ce02_ICount-=5; RD_IDY; LDA;		  } /* 5 LDA IDY */
OP(d1) { int tmp; m65ce02_ICount-=5; RD_IDY; CMP;		  } /* 5 CMP IDY */
OP(f1) { int tmp; m65ce02_ICount-=5; RD_IDY; SBC;		  } /* 5 SBC IDY */

OP(02) {		  m65ce02_ICount-=2;		 CLE;		  } /* ? CLE */
OP(22) {		  m65ce02_ICount-=7;		 JSR_IND;	  } /* ? JSR IND */
OP(42) {		  m65ce02_ICount-=2;		 NEG;		  } /* 2 NEG */
OP(62) { int tmp; m65ce02_ICount-=7; RD_IMM;  RTN;		  } /* ? RTN IMM */
OP(82) { int tmp; m65ce02_ICount-=6; RD_INSY; STA;		  } /* 5 STA INSY */
OP(a2) { int tmp; m65ce02_ICount-=2; RD_IMM; LDX;		  } /* 2 LDX IMM */
OP(c2) { int tmp; m65ce02_ICount-=2; RD_IMM; CPZ;		  } /* 2 CPZ IMM */
OP(e2) { int tmp; m65ce02_ICount-=6; RD_INSY; LDA;		  } /* ? LDA INSY */

OP(12) { int tmp; m65ce02_ICount-=5; RD_IDZ; ORA;		  } /* 5 ORA IDZ */
OP(32) { int tmp; m65ce02_ICount-=5; RD_IDZ; AND;		  } /* 5 AND IDZ */
OP(52) { int tmp; m65ce02_ICount-=5; RD_IDZ; EOR;		  } /* 5 EOR IDZ */
OP(72) { int tmp; m65ce02_ICount-=5; RD_IDZ; ADC;		  } /* 5 ADC IDZ */
OP(92) { int tmp; m65ce02_ICount-=5; RD_IDZ; STA;		  } /* 5 STA IDZ */
OP(b2) { int tmp; m65ce02_ICount-=5; RD_IDZ; LDA;		  } /* 5 LDA IDZ */
OP(d2) { int tmp; m65ce02_ICount-=5; RD_IDZ; CMP;		  } /* 5 CMP IDZ */
OP(f2) { int tmp; m65ce02_ICount-=5; RD_IDZ; SBC;		  } /* 5 SBC IDZ */

OP(03) {		  m65ce02_ICount-=2;		 SEE;		  } /* ? SEE */
OP(23) {		  m65ce02_ICount-=7;		 JSR_INDX;	  } /* ? JSR INDX */
OP(43) { int tmp; m65ce02_ICount-=2; RD_ACC; ASR_65CE02; WB_ACC; } /* 2 ASR A */
OP(63) {		  m65ce02_ICount-=5;		 BSR;		  } /* ? BSR */
OP(83) {		  m65ce02_ICount-=3;		 BRA_WORD(1); } /* ? BRA REL WORD */
OP(a3) { int tmp; m65ce02_ICount-=2; RD_IMM; LDZ;		  } /* 2 LDZ IMM */
OP(c3) { PAIR tmp; m65ce02_ICount-=6; RD_ZPG_WORD; DEW; WB_EA_WORD;  } /* ? DEW ABS */
OP(e3) { PAIR tmp; m65ce02_ICount-=6; RD_ZPG_WORD; INW; WB_EA_WORD;  } /* ? INW ABS */

OP(13) {		m65ce02_ICount-=3;			 BPL_WORD;	   } /* ? BPL REL WORD */
OP(33) {		m65ce02_ICount-=3;			 BMI_WORD;	   } /* ? BMI REL WORD */
OP(53) {		m65ce02_ICount-=3;			 BVC_WORD;	   } /* ? BVC REL WORD */
OP(73) {		m65ce02_ICount-=3;			 BVS_WORD;	   } /* ? BVS REL WORD */
OP(93) {		m65ce02_ICount-=3;			 BCC_WORD;	   } /* ? BCC REL WORD */
OP(b3) {		m65ce02_ICount-=3;			 BCS_WORD;	   } /* ? BCS REL WORD */
OP(d3) {		m65ce02_ICount-=3;			 BNE_WORD;	   } /* ? BNE REL WORD */
OP(f3) {		m65ce02_ICount-=3;			 BEQ_WORD;	   } /* ? BEQ REL WORD */

OP(04) { int tmp; m65ce02_ICount-=4; RD_ZPG; TSB; WB_EA;  } /* 3 TSB ZPG */
OP(24) { int tmp; m65ce02_ICount-=4; RD_ZPG; BIT;		  } /* 3 BIT ZPG */
OP(44) { int tmp; m65ce02_ICount-=4; RD_ZPG; ASR_65CE02; WB_EA;  } /* 5 ASR ZPG */
OP(64) { int tmp; m65ce02_ICount-=3;		 STZ_65CE02; WR_ZPG; } /* 3 STZ ZPG */
OP(84) { int tmp; m65ce02_ICount-=3;		 STY; WR_ZPG; } /* 3 STY ZPG */
OP(a4) { int tmp; m65ce02_ICount-=3; RD_ZPG; LDY;		  } /* 3 LDY ZPG */
OP(c4) { int tmp; m65ce02_ICount-=3; RD_ZPG; CPY;		  } /* 3 CPY ZPG */
OP(e4) { int tmp; m65ce02_ICount-=3; RD_ZPG; CPX;		  } /* 3 CPX ZPG */

OP(14) { int tmp; m65ce02_ICount-=4; RD_ZPG; TRB; WB_EA;  } /* 3 TRB ZPG */
OP(34) { int tmp; m65ce02_ICount-=4; RD_ZPX; BIT;		  } /* 4 BIT ZPX */
OP(54) { int tmp; m65ce02_ICount-=4; RD_ZPX; ASR_65CE02; WB_EA;  } /* 6 ASR ZPX */
OP(74) { int tmp; m65ce02_ICount-=3;		 STZ_65CE02; WR_ZPX; } /* 4 STZ ZPX */
OP(94) { int tmp; m65ce02_ICount-=3;		 STY; WR_ZPX; } /* 4 STY ZPX */
OP(b4) { int tmp; m65ce02_ICount-=3; RD_ZPX; LDY;		  } /* 4 LDY ZPX */
OP(d4) { int tmp; m65ce02_ICount-=3; RD_ZPG; CPZ;		  } /* 3 CPZ ZPG */
OP(f4) { PAIR tmp; m65ce02_ICount-=5; RD_IMM_WORD; PUSH_WORD(tmp); } /* ? PHW imm16 */

OP(05) { int tmp; m65ce02_ICount-=3; RD_ZPG; ORA;		  } /* 3 ORA ZPG */
OP(25) { int tmp; m65ce02_ICount-=3; RD_ZPG; AND;		  } /* 3 AND ZPG */
OP(45) { int tmp; m65ce02_ICount-=3; RD_ZPG; EOR;		  } /* 3 EOR ZPG */
OP(65) { int tmp; m65ce02_ICount-=3; RD_ZPG; ADC;		  } /* 3 ADC ZPG */
OP(85) { int tmp; m65ce02_ICount-=3;		 STA; WR_ZPG; } /* 3 STA ZPG */
OP(a5) { int tmp; m65ce02_ICount-=3; RD_ZPG; LDA;		  } /* 3 LDA ZPG */
OP(c5) { int tmp; m65ce02_ICount-=3; RD_ZPG; CMP;		  } /* 3 CMP ZPG */
OP(e5) { int tmp; m65ce02_ICount-=3; RD_ZPG; SBC;		  } /* 3 SBC ZPG */

OP(15) { int tmp; m65ce02_ICount-=3; RD_ZPX; ORA;		  } /* 4 ORA ZPX */
OP(35) { int tmp; m65ce02_ICount-=3; RD_ZPX; AND;		  } /* 4 AND ZPX */
OP(55) { int tmp; m65ce02_ICount-=3; RD_ZPX; EOR;		  } /* 4 EOR ZPX */
OP(75) { int tmp; m65ce02_ICount-=3; RD_ZPX; ADC;		  } /* 4 ADC ZPX */
OP(95) { int tmp; m65ce02_ICount-=3;		 STA; WR_ZPX; } /* 4 STA ZPX */
OP(b5) { int tmp; m65ce02_ICount-=3; RD_ZPX; LDA;		  } /* 4 LDA ZPX */
OP(d5) { int tmp; m65ce02_ICount-=3; RD_ZPX; CMP;		  } /* 4 CMP ZPX */
OP(f5) { int tmp; m65ce02_ICount-=3; RD_ZPX; SBC;		  } /* 4 SBC ZPX */

OP(06) { int tmp; m65ce02_ICount-=4; RD_ZPG; ASL; WB_EA;  } /* 5 ASL ZPG */
OP(26) { int tmp; m65ce02_ICount-=4; RD_ZPG; ROL; WB_EA;  } /* 5 ROL ZPG */
OP(46) { int tmp; m65ce02_ICount-=4; RD_ZPG; LSR; WB_EA;  } /* 5 LSR ZPG */
OP(66) { int tmp; m65ce02_ICount-=4; RD_ZPG; ROR; WB_EA;  } /* 5 ROR ZPG */
OP(86) { int tmp; m65ce02_ICount-=3;		 STX; WR_ZPG; } /* 3 STX ZPG */
OP(a6) { int tmp; m65ce02_ICount-=3; RD_ZPG; LDX;		  } /* 3 LDX ZPG */
OP(c6) { int tmp; m65ce02_ICount-=4; RD_ZPG; DEC; WB_EA;  } /* 5 DEC ZPG */
OP(e6) { int tmp; m65ce02_ICount-=4; RD_ZPG; INC; WB_EA;  } /* 5 INC ZPG */

OP(16) { int tmp; m65ce02_ICount-=4; RD_ZPX; ASL; WB_EA;  } /* 6 ASL ZPX */
OP(36) { int tmp; m65ce02_ICount-=4; RD_ZPX; ROL; WB_EA;  } /* 6 ROL ZPX */
OP(56) { int tmp; m65ce02_ICount-=4; RD_ZPX; LSR; WB_EA;  } /* 6 LSR ZPX */
OP(76) { int tmp; m65ce02_ICount-=4; RD_ZPX; ROR; WB_EA;  } /* 6 ROR ZPX */
OP(96) { int tmp; m65ce02_ICount-=3;		 STX; WR_ZPY; } /* 4 STX ZPY */
OP(b6) { int tmp; m65ce02_ICount-=3; RD_ZPY; LDX;		  } /* 4 LDX ZPY */
OP(d6) { int tmp; m65ce02_ICount-=4; RD_ZPX; DEC; WB_EA;  } /* 6 DEC ZPX */
OP(f6) { int tmp; m65ce02_ICount-=4; RD_ZPX; INC; WB_EA;  } /* 6 INC ZPX */

OP(07) { int tmp; m65ce02_ICount-=4; RD_ZPG; RMB(0);WB_EA;} /* 5 RMB0 ZPG */
OP(27) { int tmp; m65ce02_ICount-=4; RD_ZPG; RMB(2);WB_EA;} /* 5 RMB2 ZPG */
OP(47) { int tmp; m65ce02_ICount-=4; RD_ZPG; RMB(4);WB_EA;} /* 5 RMB4 ZPG */
OP(67) { int tmp; m65ce02_ICount-=4; RD_ZPG; RMB(6);WB_EA;} /* 5 RMB6 ZPG */
OP(87) { int tmp; m65ce02_ICount-=4; RD_ZPG; SMB(0);WB_EA;} /* 5 SMB0 ZPG */
OP(a7) { int tmp; m65ce02_ICount-=4; RD_ZPG; SMB(2);WB_EA;} /* 5 SMB2 ZPG */
OP(c7) { int tmp; m65ce02_ICount-=4; RD_ZPG; SMB(4);WB_EA;} /* 5 SMB4 ZPG */
OP(e7) { int tmp; m65ce02_ICount-=4; RD_ZPG; SMB(6);WB_EA;} /* 5 SMB6 ZPG */

OP(17) { int tmp; m65ce02_ICount-=4; RD_ZPG; RMB(1);WB_EA;} /* 5 RMB1 ZPG */
OP(37) { int tmp; m65ce02_ICount-=4; RD_ZPG; RMB(3);WB_EA;} /* 5 RMB3 ZPG */
OP(57) { int tmp; m65ce02_ICount-=4; RD_ZPG; RMB(5);WB_EA;} /* 5 RMB5 ZPG */
OP(77) { int tmp; m65ce02_ICount-=4; RD_ZPG; RMB(7);WB_EA;} /* 5 RMB7 ZPG */
OP(97) { int tmp; m65ce02_ICount-=4; RD_ZPG; SMB(1);WB_EA;} /* 5 SMB1 ZPG */
OP(b7) { int tmp; m65ce02_ICount-=4; RD_ZPG; SMB(3);WB_EA;} /* 5 SMB3 ZPG */
OP(d7) { int tmp; m65ce02_ICount-=4; RD_ZPG; SMB(5);WB_EA;} /* 5 SMB5 ZPG */
OP(f7) { int tmp; m65ce02_ICount-=4; RD_ZPG; SMB(7);WB_EA;} /* 5 SMB7 ZPG */

OP(08) {		  m65ce02_ICount-=3;		 PHP;		  } /* 2 PHP */
OP(28) {		  m65ce02_ICount-=3;		 PLP;		  } /* 2 PLP */
OP(48) {		  m65ce02_ICount-=3;		 PHA;		  } /* 2 PHA */
OP(68) {		  m65ce02_ICount-=3;		 PLA;		  } /* 2 PLA */
OP(88) {		  m65ce02_ICount-=1;		 DEY;		  } /* 2 DEY */
OP(a8) {		  m65ce02_ICount-=1;		 TAY;		  } /* 2 TAY */
OP(c8) {		  m65ce02_ICount-=1;		 INY;		  } /* 2 INY */
OP(e8) {		  m65ce02_ICount-=1;		 INX;		  } /* 2 INX */

OP(18) {		  m65ce02_ICount-=1;		 CLC;		  } /* 2 CLC */
OP(38) {		  m65ce02_ICount-=1;		 SEC;		  } /* 2 SEC */
OP(58) {		  m65ce02_ICount-=2;		 CLI;		  } /* 2 CLI */
OP(78) {		  m65ce02_ICount-=2;		 SEI;		  } /* 2 SEI */
OP(98) {		  m65ce02_ICount-=1;		 TYA;		  } /* 2 TYA */
OP(b8) {		  m65ce02_ICount-=1;		 CLV;		  } /* 2 CLV */
OP(d8) {		  m65ce02_ICount-=1;		 CLD;		  } /* 2 CLD */
OP(f8) {		  m65ce02_ICount-=1;		 SED;		  } /* 2 SED */

OP(09) { int tmp; m65ce02_ICount-=2; RD_IMM; ORA;		  } /* 2 ORA IMM */
OP(29) { int tmp; m65ce02_ICount-=2; RD_IMM; AND;		  } /* 2 AND IMM */
OP(49) { int tmp; m65ce02_ICount-=2; RD_IMM; EOR;		  } /* 2 EOR IMM */
OP(69) { int tmp; m65ce02_ICount-=2; RD_IMM; ADC;		  } /* 2 ADC IMM */
OP(89) { int tmp; m65ce02_ICount-=2; RD_IMM; BIT;		  } /* 2 BIT IMM */
OP(a9) { int tmp; m65ce02_ICount-=2; RD_IMM; LDA;		  } /* 2 LDA IMM */
OP(c9) { int tmp; m65ce02_ICount-=2; RD_IMM; CMP;		  } /* 2 CMP IMM */
OP(e9) { int tmp; m65ce02_ICount-=2; RD_IMM; SBC;		  } /* 2 SBC IMM */

OP(19) { int tmp; m65ce02_ICount-=4; RD_ABY; ORA;		  } /* 4 ORA ABY */
OP(39) { int tmp; m65ce02_ICount-=4; RD_ABY; AND;		  } /* 4 AND ABY */
OP(59) { int tmp; m65ce02_ICount-=4; RD_ABY; EOR;		  } /* 4 EOR ABY */
OP(79) { int tmp; m65ce02_ICount-=4; RD_ABY; ADC;		  } /* 4 ADC ABY */
OP(99) { int tmp; m65ce02_ICount-=4;		 STA; WR_ABY; } /* 5 STA ABY */
OP(b9) { int tmp; m65ce02_ICount-=4; RD_ABY; LDA;		  } /* 4 LDA ABY */
OP(d9) { int tmp; m65ce02_ICount-=4; RD_ABY; CMP;		  } /* 4 CMP ABY */
OP(f9) { int tmp; m65ce02_ICount-=4; RD_ABY; SBC;		  } /* 4 SBC ABY */

OP(0a) { int tmp; m65ce02_ICount-=1; RD_ACC; ASL; WB_ACC; } /* 2 ASL A */
OP(2a) { int tmp; m65ce02_ICount-=1; RD_ACC; ROL; WB_ACC; } /* 2 ROL A */
OP(4a) { int tmp; m65ce02_ICount-=1; RD_ACC; LSR; WB_ACC; } /* 2 LSR A */
OP(6a) { int tmp; m65ce02_ICount-=1; RD_ACC; ROR; WB_ACC; } /* 2 ROR A */
OP(8a) {		  m65ce02_ICount-=1;		 TXA;		  } /* 2 TXA */
OP(aa) {		  m65ce02_ICount-=1;		 TAX;		  } /* 2 TAX */
OP(ca) {		  m65ce02_ICount-=1;		 DEX;		  } /* 2 DEX */
OP(ea) {		  m65ce02_ICount-=1;		 NOP;		  } /* 2 NOP */

OP(1a) {		  m65ce02_ICount-=1;		 INA;		  } /* 2 INA */
OP(3a) {		  m65ce02_ICount-=1;		 DEA;		  } /* 2 DEA */
OP(5a) {		  m65ce02_ICount-=3;		 PHY;		  } /* 3 PHY */
OP(7a) {		  m65ce02_ICount-=3;		 PLY;		  } /* 4 PLY */
OP(9a) {		  m65ce02_ICount-=1;		 TXS;		  } /* 2 TXS */
OP(ba) {		  m65ce02_ICount-=1;		 TSX;		  } /* 2 TSX */
OP(da) {		  m65ce02_ICount-=3;		 PHX;		  } /* 3 PHX */
OP(fa) {		  m65ce02_ICount-=3;		 PLX;		  } /* 4 PLX */

OP(0b) {		  m65ce02_ICount-=1;		 TSY;		  } /* 2 TSY */
OP(2b) {		  m65ce02_ICount-=1;		 TYS;		  } /* 2 TYS */
OP(4b) {		  m65ce02_ICount-=1;		 TAZ;		  } /* 2 TAZ */
OP(6b) {		  m65ce02_ICount-=1;		 TZA;		  } /* 2 TZA */
OP(8b) { int tmp; m65ce02_ICount-=4;		 STY; WR_ABX; } /* 5 STY ABX */
OP(ab) { int tmp; m65ce02_ICount-=4; RD_ABS; LDZ;		  } /* 4 LDZ ABS */
OP(cb) { PAIR tmp; m65ce02_ICount-=7; RD_ABS_WORD; ASW; WB_EA_WORD;  } /* ? ASW ABS */
OP(eb) { PAIR tmp; m65ce02_ICount-=6; RD_ABS_WORD; ROW; WB_EA_WORD;  } /* ? roW ABS */

OP(1b) {		  m65ce02_ICount-=1;		 INZ;		  } /* 2 INZ */
OP(3b) {		  m65ce02_ICount-=1;		 DEZ;		  } /* 2 DEZ */
OP(5b) {		  m65ce02_ICount-=1;		 TAB;		  } /* 2 TAB */
OP(7b) {		  m65ce02_ICount-=1;		 TBA;		  } /* 2 TBA */
OP(9b) { int tmp; m65ce02_ICount-=4;		 STX; WR_ABY; } /* 5 STX ABY */
OP(bb) { int tmp; m65ce02_ICount-=4; RD_ABX; LDZ;		  } /* 4 LDZ ABX */
OP(db) {		  m65ce02_ICount-=3;		 PHZ;		  } /* 2 PHZ */
OP(fb) {		  m65ce02_ICount-=3;		 PLZ;		  } /* 2 PLZ */

OP(0c) { int tmp; m65ce02_ICount-=5; RD_ABS; TSB; WB_EA;  } /* 4 TSB ABS */
OP(2c) { int tmp; m65ce02_ICount-=5; RD_ABS; BIT;		  } /* 4 BIT ABS */
OP(4c) {		  m65ce02_ICount-=3; EA_ABS; JMP;		  } /* 3 JMP ABS */
OP(6c) { int tmp; m65ce02_ICount-=5; EA_IND; JMP;		  } /* 5 JMP IND */
OP(8c) { int tmp; m65ce02_ICount-=4;		 STY; WR_ABS; } /* 4 STY ABS */
OP(ac) { int tmp; m65ce02_ICount-=4; RD_ABS; LDY;		  } /* 4 LDY ABS */
OP(cc) { int tmp; m65ce02_ICount-=4; RD_ABS; CPY;		  } /* 4 CPY ABS */
OP(ec) { int tmp; m65ce02_ICount-=4; RD_ABS; CPX;		  } /* 4 CPX ABS */

OP(1c) { int tmp; m65ce02_ICount-=5; RD_ABS; TRB; WB_EA;  } /* 4 TRB ABS */
OP(3c) { int tmp; m65ce02_ICount-=5; RD_ABX; BIT;		  } /* 4 BIT ABX */
#ifdef M4510
OP(5c) {		  m65ce02_ICount-=4;		 MAP;		  } /* ? MAP */
#else
OP(5c) { int t1,t2,t3; m65ce02_ICount-=4;	  AUG;		  } /* ? AUGMENT/no operation */
#endif
OP(7c) { int tmp; m65ce02_ICount-=5; EA_IAX; JMP;		  } /* 6 JMP IAX */
OP(9c) { int tmp; m65ce02_ICount-=4;		 STZ_65CE02; WR_ABS; } /* 4 STZ ABS */
OP(bc) { int tmp; m65ce02_ICount-=4; RD_ABX; LDY;		  } /* 4 LDY ABX */
OP(dc) { int tmp; m65ce02_ICount-=4; RD_ABS; CPZ;		  } /* 3 CPZ ABS */
OP(fc) { PAIR tmp; m65ce02_ICount-=7; RD_ABS_WORD; PUSH_WORD(tmp); } /* ? PHW ab */

OP(0d) { int tmp; m65ce02_ICount-=4; RD_ABS; ORA;		  } /* 4 ORA ABS */
OP(2d) { int tmp; m65ce02_ICount-=4; RD_ABS; AND;		  } /* 4 AND ABS */
OP(4d) { int tmp; m65ce02_ICount-=4; RD_ABS; EOR;		  } /* 4 EOR ABS */
OP(6d) { int tmp; m65ce02_ICount-=4; RD_ABS; ADC;		  } /* 4 ADC ABS */
OP(8d) { int tmp; m65ce02_ICount-=4;		 STA; WR_ABS; } /* 4 STA ABS */
OP(ad) { int tmp; m65ce02_ICount-=4; RD_ABS; LDA;		  } /* 4 LDA ABS */
OP(cd) { int tmp; m65ce02_ICount-=4; RD_ABS; CMP;		  } /* 4 CMP ABS */
OP(ed) { int tmp; m65ce02_ICount-=3; RD_ABS; SBC;		  } /* 4 SBC ABS */

OP(1d) { int tmp; m65ce02_ICount-=4; RD_ABX; ORA;		  } /* 4 ORA ABX */
OP(3d) { int tmp; m65ce02_ICount-=4; RD_ABX; AND;		  } /* 4 AND ABX */
OP(5d) { int tmp; m65ce02_ICount-=4; RD_ABX; EOR;		  } /* 4 EOR ABX */
OP(7d) { int tmp; m65ce02_ICount-=4; RD_ABX; ADC;		  } /* 4 ADC ABX */
OP(9d) { int tmp; m65ce02_ICount-=4;		 STA; WR_ABX; } /* 5 STA ABX */
OP(bd) { int tmp; m65ce02_ICount-=4; RD_ABX; LDA;		  } /* 4 LDA ABX */
OP(dd) { int tmp; m65ce02_ICount-=4; RD_ABX; CMP;		  } /* 4 CMP ABX */
OP(fd) { int tmp; m65ce02_ICount-=4; RD_ABX; SBC;		  } /* 4 SBC ABX */

OP(0e) { int tmp; m65ce02_ICount-=5; RD_ABS; ASL; WB_EA;  } /* 6 ASL ABS */
OP(2e) { int tmp; m65ce02_ICount-=5; RD_ABS; ROL; WB_EA;  } /* 6 ROL ABS */
OP(4e) { int tmp; m65ce02_ICount-=5; RD_ABS; LSR; WB_EA;  } /* 6 LSR ABS */
OP(6e) { int tmp; m65ce02_ICount-=5; RD_ABS; ROR; WB_EA;  } /* 6 ROR ABS */
OP(8e) { int tmp; m65ce02_ICount-=4;		 STX; WR_ABS; } /* 5 STX ABS */
OP(ae) { int tmp; m65ce02_ICount-=5; RD_ABS; LDX;		  } /* 4 LDX ABS */
OP(ce) { int tmp; m65ce02_ICount-=5; RD_ABS; DEC; WB_EA;  } /* 6 DEC ABS */
OP(ee) { int tmp; m65ce02_ICount-=5; RD_ABS; INC; WB_EA;  } /* 6 INC ABS */

OP(1e) { int tmp; m65ce02_ICount-=5; RD_ABX; ASL; WB_EA;  } /* 7 ASL ABX */
OP(3e) { int tmp; m65ce02_ICount-=5; RD_ABX; ROL; WB_EA;  } /* 7 ROL ABX */
OP(5e) { int tmp; m65ce02_ICount-=5; RD_ABX; LSR; WB_EA;  } /* 7 LSR ABX */
OP(7e) { int tmp; m65ce02_ICount-=5; RD_ABX; ROR; WB_EA;  } /* 7 ROR ABX */
OP(9e) { int tmp; m65ce02_ICount-=4;		 STZ_65CE02; WR_ABX; } /* 5 STZ ABX */
OP(be) { int tmp; m65ce02_ICount-=4; RD_ABY; LDX;		  } /* 4 LDX ABY */
OP(de) { int tmp; m65ce02_ICount-=5; RD_ABX; DEC; WB_EA;  } /* 7 DEC ABX */
OP(fe) { int tmp; m65ce02_ICount-=4; RD_ABX; INC; WB_EA;  } /* 7 INC ABX */

OP(0f) { int tmp; m65ce02_ICount-=4; RD_ZPG; BBR(0);	  } /* 5 BBR0 ZPG */
OP(2f) { int tmp; m65ce02_ICount-=4; RD_ZPG; BBR(2);	  } /* 5 BBR2 ZPG */
OP(4f) { int tmp; m65ce02_ICount-=4; RD_ZPG; BBR(4);	  } /* 5 BBR4 ZPG */
OP(6f) { int tmp; m65ce02_ICount-=4; RD_ZPG; BBR(6);	  } /* 5 BBR6 ZPG */
OP(8f) { int tmp; m65ce02_ICount-=4; RD_ZPG; BBS(0);	  } /* 5 BBS0 ZPG */
OP(af) { int tmp; m65ce02_ICount-=4; RD_ZPG; BBS(2);	  } /* 5 BBS2 ZPG */
OP(cf) { int tmp; m65ce02_ICount-=4; RD_ZPG; BBS(4);	  } /* 5 BBS4 ZPG */
OP(ef) { int tmp; m65ce02_ICount-=4; RD_ZPG; BBS(6);	  } /* 5 BBS6 ZPG */

OP(1f) { int tmp; m65ce02_ICount-=4; RD_ZPG; BBR(1);	  } /* 5 BBR1 ZPG */
OP(3f) { int tmp; m65ce02_ICount-=4; RD_ZPG; BBR(3);	  } /* 5 BBR3 ZPG */
OP(5f) { int tmp; m65ce02_ICount-=4; RD_ZPG; BBR(5);	  } /* 5 BBR5 ZPG */
OP(7f) { int tmp; m65ce02_ICount-=4; RD_ZPG; BBR(7);	  } /* 5 BBR7 ZPG */
OP(9f) { int tmp; m65ce02_ICount-=4; RD_ZPG; BBS(1);	  } /* 5 BBS1 ZPG */
OP(bf) { int tmp; m65ce02_ICount-=4; RD_ZPG; BBS(3);	  } /* 5 BBS3 ZPG */
OP(df) { int tmp; m65ce02_ICount-=4; RD_ZPG; BBS(5);	  } /* 5 BBS5 ZPG */
OP(ff) { int tmp; m65ce02_ICount-=4; RD_ZPG; BBS(7);	  } /* 5 BBS7 ZPG */

#ifdef M4510
static void (*insn4510[0x100])(void) = {
	m4510_00,m4510_01,m4510_02,m4510_03,m4510_04,m4510_05,m4510_06,m4510_07,
	m4510_08,m4510_09,m4510_0a,m4510_0b,m4510_0c,m4510_0d,m4510_0e,m4510_0f,
	m4510_10,m4510_11,m4510_12,m4510_13,m4510_14,m4510_15,m4510_16,m4510_17,
	m4510_18,m4510_19,m4510_1a,m4510_1b,m4510_1c,m4510_1d,m4510_1e,m4510_1f,
	m4510_20,m4510_21,m4510_22,m4510_23,m4510_24,m4510_25,m4510_26,m4510_27,
	m4510_28,m4510_29,m4510_2a,m4510_2b,m4510_2c,m4510_2d,m4510_2e,m4510_2f,
	m4510_30,m4510_31,m4510_32,m4510_33,m4510_34,m4510_35,m4510_36,m4510_37,
	m4510_38,m4510_39,m4510_3a,m4510_3b,m4510_3c,m4510_3d,m4510_3e,m4510_3f,
	m4510_40,m4510_41,m4510_42,m4510_43,m4510_44,m4510_45,m4510_46,m4510_47,
	m4510_48,m4510_49,m4510_4a,m4510_4b,m4510_4c,m4510_4d,m4510_4e,m4510_4f,
	m4510_50,m4510_51,m4510_52,m4510_53,m4510_54,m4510_55,m4510_56,m4510_57,
	m4510_58,m4510_59,m4510_5a,m4510_5b,m4510_5c,m4510_5d,m4510_5e,m4510_5f,
	m4510_60,m4510_61,m4510_62,m4510_63,m4510_64,m4510_65,m4510_66,m4510_67,
	m4510_68,m4510_69,m4510_6a,m4510_6b,m4510_6c,m4510_6d,m4510_6e,m4510_6f,
	m4510_70,m4510_71,m4510_72,m4510_73,m4510_74,m4510_75,m4510_76,m4510_77,
	m4510_78,m4510_79,m4510_7a,m4510_7b,m4510_7c,m4510_7d,m4510_7e,m4510_7f,
	m4510_80,m4510_81,m4510_82,m4510_83,m4510_84,m4510_85,m4510_86,m4510_87,
	m4510_88,m4510_89,m4510_8a,m4510_8b,m4510_8c,m4510_8d,m4510_8e,m4510_8f,
	m4510_90,m4510_91,m4510_92,m4510_93,m4510_94,m4510_95,m4510_96,m4510_97,
	m4510_98,m4510_99,m4510_9a,m4510_9b,m4510_9c,m4510_9d,m4510_9e,m4510_9f,
	m4510_a0,m4510_a1,m4510_a2,m4510_a3,m4510_a4,m4510_a5,m4510_a6,m4510_a7,
	m4510_a8,m4510_a9,m4510_aa,m4510_ab,m4510_ac,m4510_ad,m4510_ae,m4510_af,
	m4510_b0,m4510_b1,m4510_b2,m4510_b3,m4510_b4,m4510_b5,m4510_b6,m4510_b7,
	m4510_b8,m4510_b9,m4510_ba,m4510_bb,m4510_bc,m4510_bd,m4510_be,m4510_bf,
	m4510_c0,m4510_c1,m4510_c2,m4510_c3,m4510_c4,m4510_c5,m4510_c6,m4510_c7,
	m4510_c8,m4510_c9,m4510_ca,m4510_cb,m4510_cc,m4510_cd,m4510_ce,m4510_cf,
	m4510_d0,m4510_d1,m4510_d2,m4510_d3,m4510_d4,m4510_d5,m4510_d6,m4510_d7,
	m4510_d8,m4510_d9,m4510_da,m4510_db,m4510_dc,m4510_dd,m4510_de,m4510_df,
	m4510_e0,m4510_e1,m4510_e2,m4510_e3,m4510_e4,m4510_e5,m4510_e6,m4510_e7,
	m4510_e8,m4510_e9,m4510_ea,m4510_eb,m4510_ec,m4510_ed,m4510_ee,m4510_ef,
	m4510_f0,m4510_f1,m4510_f2,m4510_f3,m4510_f4,m4510_f5,m4510_f6,m4510_f7,
	m4510_f8,m4510_f9,m4510_fa,m4510_fb,m4510_fc,m4510_fd,m4510_fe,m4510_ff
};
#else
static void (*insn65ce02[0x100])(void) = {
	m65ce02_00,m65ce02_01,m65ce02_02,m65ce02_03,m65ce02_04,m65ce02_05,m65ce02_06,m65ce02_07,
	m65ce02_08,m65ce02_09,m65ce02_0a,m65ce02_0b,m65ce02_0c,m65ce02_0d,m65ce02_0e,m65ce02_0f,
	m65ce02_10,m65ce02_11,m65ce02_12,m65ce02_13,m65ce02_14,m65ce02_15,m65ce02_16,m65ce02_17,
	m65ce02_18,m65ce02_19,m65ce02_1a,m65ce02_1b,m65ce02_1c,m65ce02_1d,m65ce02_1e,m65ce02_1f,
	m65ce02_20,m65ce02_21,m65ce02_22,m65ce02_23,m65ce02_24,m65ce02_25,m65ce02_26,m65ce02_27,
	m65ce02_28,m65ce02_29,m65ce02_2a,m65ce02_2b,m65ce02_2c,m65ce02_2d,m65ce02_2e,m65ce02_2f,
	m65ce02_30,m65ce02_31,m65ce02_32,m65ce02_33,m65ce02_34,m65ce02_35,m65ce02_36,m65ce02_37,
	m65ce02_38,m65ce02_39,m65ce02_3a,m65ce02_3b,m65ce02_3c,m65ce02_3d,m65ce02_3e,m65ce02_3f,
	m65ce02_40,m65ce02_41,m65ce02_42,m65ce02_43,m65ce02_44,m65ce02_45,m65ce02_46,m65ce02_47,
	m65ce02_48,m65ce02_49,m65ce02_4a,m65ce02_4b,m65ce02_4c,m65ce02_4d,m65ce02_4e,m65ce02_4f,
	m65ce02_50,m65ce02_51,m65ce02_52,m65ce02_53,m65ce02_54,m65ce02_55,m65ce02_56,m65ce02_57,
	m65ce02_58,m65ce02_59,m65ce02_5a,m65ce02_5b,m65ce02_5c,m65ce02_5d,m65ce02_5e,m65ce02_5f,
	m65ce02_60,m65ce02_61,m65ce02_62,m65ce02_63,m65ce02_64,m65ce02_65,m65ce02_66,m65ce02_67,
	m65ce02_68,m65ce02_69,m65ce02_6a,m65ce02_6b,m65ce02_6c,m65ce02_6d,m65ce02_6e,m65ce02_6f,
	m65ce02_70,m65ce02_71,m65ce02_72,m65ce02_73,m65ce02_74,m65ce02_75,m65ce02_76,m65ce02_77,
	m65ce02_78,m65ce02_79,m65ce02_7a,m65ce02_7b,m65ce02_7c,m65ce02_7d,m65ce02_7e,m65ce02_7f,
	m65ce02_80,m65ce02_81,m65ce02_82,m65ce02_83,m65ce02_84,m65ce02_85,m65ce02_86,m65ce02_87,
	m65ce02_88,m65ce02_89,m65ce02_8a,m65ce02_8b,m65ce02_8c,m65ce02_8d,m65ce02_8e,m65ce02_8f,
	m65ce02_90,m65ce02_91,m65ce02_92,m65ce02_93,m65ce02_94,m65ce02_95,m65ce02_96,m65ce02_97,
	m65ce02_98,m65ce02_99,m65ce02_9a,m65ce02_9b,m65ce02_9c,m65ce02_9d,m65ce02_9e,m65ce02_9f,
	m65ce02_a0,m65ce02_a1,m65ce02_a2,m65ce02_a3,m65ce02_a4,m65ce02_a5,m65ce02_a6,m65ce02_a7,
	m65ce02_a8,m65ce02_a9,m65ce02_aa,m65ce02_ab,m65ce02_ac,m65ce02_ad,m65ce02_ae,m65ce02_af,
	m65ce02_b0,m65ce02_b1,m65ce02_b2,m65ce02_b3,m65ce02_b4,m65ce02_b5,m65ce02_b6,m65ce02_b7,
	m65ce02_b8,m65ce02_b9,m65ce02_ba,m65ce02_bb,m65ce02_bc,m65ce02_bd,m65ce02_be,m65ce02_bf,
	m65ce02_c0,m65ce02_c1,m65ce02_c2,m65ce02_c3,m65ce02_c4,m65ce02_c5,m65ce02_c6,m65ce02_c7,
	m65ce02_c8,m65ce02_c9,m65ce02_ca,m65ce02_cb,m65ce02_cc,m65ce02_cd,m65ce02_ce,m65ce02_cf,
	m65ce02_d0,m65ce02_d1,m65ce02_d2,m65ce02_d3,m65ce02_d4,m65ce02_d5,m65ce02_d6,m65ce02_d7,
	m65ce02_d8,m65ce02_d9,m65ce02_da,m65ce02_db,m65ce02_dc,m65ce02_dd,m65ce02_de,m65ce02_df,
	m65ce02_e0,m65ce02_e1,m65ce02_e2,m65ce02_e3,m65ce02_e4,m65ce02_e5,m65ce02_e6,m65ce02_e7,
	m65ce02_e8,m65ce02_e9,m65ce02_ea,m65ce02_eb,m65ce02_ec,m65ce02_ed,m65ce02_ee,m65ce02_ef,
	m65ce02_f0,m65ce02_f1,m65ce02_f2,m65ce02_f3,m65ce02_f4,m65ce02_f5,m65ce02_f6,m65ce02_f7,
	m65ce02_f8,m65ce02_f9,m65ce02_fa,m65ce02_fb,m65ce02_fc,m65ce02_fd,m65ce02_fe,m65ce02_ff
};
#endif
