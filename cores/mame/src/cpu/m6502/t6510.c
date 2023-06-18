/*****************************************************************************
 *
 *	 tbl6510.c
 *   6510 opcode functions and function pointer table
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
 *	 - Opcode information based on an Intel 386 '6510.asm' core
 *	   written by R.F. van Ee (1993)
 *	 - Cycle counts are guesswork :-)
 *
 *****************************************************************************/
/* 
   PeT 11.August 2000
       added NMOS rw opcodes address access behaviour
	   emulation of this currently NOT in NMOS 6502, N2A03, M6509
	   only done in the official opcodes
	   (inofficial are too hard to guess and test)
   NMOS does rw opcodes
   (inc 00) as read 00 into alu, write alu into 00, inc alu, write alu into 00
   CMOS 
   (inc 00) as read 00 into alu, read alu into 00, inc alu, write alu into 00
   needed in many C64,c16 games
   c16 lone07 does asl ff09, 1st write cycles quits interrupt
*/


#undef	OP
#define OP(nn) static INLINE void m6510_##nn(void)

/*****************************************************************************
 *****************************************************************************
 *
 *	 overrides for 6510 opcodes
 *
 *****************************************************************************
 ********** insn   temp 	cycles			   rdmem   opc	wrmem	**********/
#define m6510_00 m6502_00									/* 7 BRK */
#define m6510_20 m6502_20									/* 6 JSR */
#define m6510_40 m6502_40									/* 6 RTI */
#define m6510_60 m6502_60									/* 6 RTS */
OP(80) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
#define m6510_a0 m6502_a0									/* 2 LDY IMM */
#define m6510_c0 m6502_c0									/* 2 CPY IMM */
#define m6510_e0 m6502_e0									/* 2 CPX IMM */

#define m6510_10 m6502_10									/* 2 BPL */
#define m6510_30 m6502_30									/* 2 BMI */
#define m6510_50 m6502_50									/* 2 BVC */
#define m6510_70 m6502_70									/* 2 BVS */
#define m6510_90 m6502_90									/* 2 BCC */
#define m6510_b0 m6502_b0									/* 2 BCS */
#define m6510_d0 m6502_d0									/* 2 BNE */
#define m6510_f0 m6502_f0									/* 2 BEQ */

#define m6510_01 m6502_01									/* 6 ORA IDX */
#define m6510_21 m6502_21									/* 6 AND IDX */
#define m6510_41 m6502_41									/* 6 EOR IDX */
#define m6510_61 m6502_61									/* 6 ADC IDX */
#define m6510_81 m6502_81									/* 6 STA IDX */
#define m6510_a1 m6502_a1									/* 6 LDA IDX */
#define m6510_c1 m6502_c1									/* 6 CMP IDX */
#define m6510_e1 m6502_e1									/* 6 SBC IDX */

#define m6510_11 m6502_11									/* 5 ORA IDY */
#define m6510_31 m6502_31									/* 5 AND IDY */
#define m6510_51 m6502_51									/* 5 EOR IDY */
#define m6510_71 m6502_71									/* 5 ADC IDY */
#define m6510_91 m6502_91									/* 6 STA IDY */
#define m6510_b1 m6502_b1									/* 5 LDA IDY */
#define m6510_d1 m6502_d1									/* 5 CMP IDY */
#define m6510_f1 m6502_f1									/* 5 SBC IDY */

OP(02) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(22) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(42) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(62) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(82) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
#define m6510_a2 m6502_a2									/* 2 LDX IMM */
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
#define m6510_24 m6502_24									/* 3 BIT ZPG */
OP(44) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(64) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
#define m6510_84 m6502_84									/* 3 STY ZPG */
#define m6510_a4 m6502_a4									/* 3 LDY ZPG */
#define m6510_c4 m6502_c4									/* 3 CPY ZPG */
#define m6510_e4 m6502_e4									/* 3 CPX ZPG */

OP(14) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(34) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(54) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(74) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
#define m6510_94 m6502_94									/* 4 STY ZP_X */
#define m6510_b4 m6502_b4									/* 4 LDY ZP_X */
OP(d4) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(f4) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */

#define m6510_05 m6502_05									/* 3 ORA ZPG */
#define m6510_25 m6502_25									/* 3 AND ZPG */
#define m6510_45 m6502_45									/* 3 EOR ZPG */
#define m6510_65 m6502_65									/* 3 ADC ZPG */
#define m6510_85 m6502_85									/* 3 STA ZPG */
#define m6510_a5 m6502_a5									/* 3 LDA ZPG */
#define m6510_c5 m6502_c5									/* 3 CMP ZPG */
#define m6510_e5 m6502_e5									/* 3 SBC ZPG */

#define m6510_15 m6502_15									/* 4 ORA ZPX */
#define m6510_35 m6502_35									/* 4 AND ZPX */
#define m6510_55 m6502_55									/* 4 EOR ZPX */
#define m6510_75 m6502_75									/* 4 ADC ZPX */
#define m6510_95 m6502_95									/* 4 STA ZPX */
#define m6510_b5 m6502_b5									/* 4 LDA ZPX */
#define m6510_d5 m6502_d5									/* 4 CMP ZPX */
#define m6510_f5 m6502_f5									/* 4 SBC ZPX */

OP(06) { int tmp; m6502_ICount -= 5; RD_ZPG; WB_EA; ASL; WB_EA;  } /* 5 ASL ZPG */
OP(26) { int tmp; m6502_ICount -= 5; RD_ZPG; WB_EA; ROL; WB_EA;  } /* 5 ROL ZPG */
OP(46) { int tmp; m6502_ICount -= 5; RD_ZPG; WB_EA; LSR; WB_EA;  } /* 5 LSR ZPG */
OP(66) { int tmp; m6502_ICount -= 5; RD_ZPG; WB_EA; ROR; WB_EA;  } /* 5 ROR ZPG */
#define m6510_86 m6502_86									/* 3 STX ZPG */
#define m6510_a6 m6502_a6									/* 3 LDX ZPG */
OP(c6) { int tmp; m6502_ICount -= 5; RD_ZPG; WB_EA; DEC; WB_EA;  } /* 5 DEC ZPG */
OP(e6) { int tmp; m6502_ICount -= 5; RD_ZPG; WB_EA; INC; WB_EA;  } /* 5 INC ZPG */

OP(16) { int tmp; m6502_ICount -= 6; RD_ZPX; WB_EA; ASL; WB_EA;  } /* 6 ASL ZPX */
OP(36) { int tmp; m6502_ICount -= 6; RD_ZPX; WB_EA; ROL; WB_EA;  } /* 6 ROL ZPX */
OP(56) { int tmp; m6502_ICount -= 6; RD_ZPX; WB_EA; LSR; WB_EA;  } /* 6 LSR ZPX */
OP(76) { int tmp; m6502_ICount -= 6; RD_ZPX; WB_EA; ROR; WB_EA;  } /* 6 ROR ZPX */
#define m6510_96 m6502_96									/* 4 STX ZPY */
#define m6510_b6 m6502_b6									/* 4 LDX ZPY */
OP(d6) { int tmp; m6502_ICount -= 6; RD_ZPX; WB_EA; DEC; WB_EA;  } /* 6 DEC ZPX */
OP(f6) { int tmp; m6502_ICount -= 6; RD_ZPX; WB_EA; INC; WB_EA;  } /* 6 INC ZPX */

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

#define m6510_08 m6502_08									/* 2 PHP */
#define m6510_28 m6502_28									/* 2 PLP */
#define m6510_48 m6502_48									/* 2 PHA */
#define m6510_68 m6502_68									/* 2 PLA */
#define m6510_88 m6502_88									/* 2 DEY */
#define m6510_a8 m6502_a8									/* 2 TAY */
#define m6510_c8 m6502_c8									/* 2 INY */
#define m6510_e8 m6502_e8									/* 2 INX */

#define m6510_18 m6502_18									/* 2 CLC */
#define m6510_38 m6502_38									/* 2 SEC */
#define m6510_58 m6502_58									/* 2 CLI */
#define m6510_78 m6502_78									/* 2 SEI */
#define m6510_98 m6502_98									/* 2 TYA */
#define m6510_b8 m6502_b8									/* 2 CLV */
#define m6510_d8 m6502_d8									/* 2 CLD */
#define m6510_f8 m6502_f8									/* 2 SED */

#define m6510_09 m6502_09									/* 2 ORA IMM */
#define m6510_29 m6502_29									/* 2 AND IMM */
#define m6510_49 m6502_49									/* 2 EOR IMM */
#define m6510_69 m6502_69									/* 2 ADC IMM */
OP(89) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
#define m6510_a9 m6502_a9									/* 2 LDA IMM */
#define m6510_c9 m6502_c9									/* 2 CMP IMM */
#define m6510_e9 m6502_e9									/* 2 SBC IMM */

#define m6510_19 m6502_19									/* 4 ORA ABY */
#define m6510_39 m6502_39									/* 4 AND ABY */
#define m6510_59 m6502_59									/* 4 EOR ABY */
#define m6510_79 m6502_79									/* 4 ADC ABY */
#define m6510_99 m6502_99									/* 5 STA ABY */
#define m6510_b9 m6502_b9									/* 4 LDA ABY */
#define m6510_d9 m6502_d9									/* 4 CMP ABY */
#define m6510_f9 m6502_f9									/* 4 SBC ABY */

#define m6510_0a m6502_0a									/* 2 ASL A */
#define m6510_2a m6502_2a									/* 2 ROL A */
#define m6510_4a m6502_4a									/* 2 LSR A */
#define m6510_6a m6502_6a									/* 2 ROR A */
#define m6510_8a m6502_8a									/* 2 TXA */
#define m6510_aa m6502_aa									/* 2 TAX */
#define m6510_ca m6502_ca									/* 2 DEX */
#define m6510_ea m6502_ea									/* 2 NOP */

OP(1a) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */
OP(3a) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */
OP(5a) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */
OP(7a) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */
#define m6510_9a m6502_9a									/* 2 TXS */
#define m6510_ba m6502_ba									/* 2 TSX */
OP(da) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */
OP(fa) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */

OP(0b) { int tmp; m6502_ICount -= 2; RD_IMM; ANC;		  } /* 2 ANC IMM */
OP(2b) { int tmp; m6502_ICount -= 2; RD_IMM; ANC;		  } /* 2 ANC IMM */
OP(4b) { int tmp; m6502_ICount -= 2; RD_IMM; ASR; WB_ACC; } /* 2 ASR IMM */
OP(6b) { int tmp; m6502_ICount -= 2; RD_IMM; ARR; WB_ACC; } /* 2 ARR IMM */
OP(8b) { int tmp; m6502_ICount -= 2; RD_IMM; AXA;         } /* 2 AXA IMM */
OP(ab) { int tmp; m6502_ICount -= 2; RD_IMM; OAL;         } /* 2 OAL IMM */
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
#define m6510_2c m6502_2c									/* 4 BIT ABS */
#define m6510_4c m6502_4c									/* 3 JMP ABS */
#define m6510_6c m6502_6c									/* 5 JMP IND */
#define m6510_8c m6502_8c									/* 4 STY ABS */
#define m6510_ac m6502_ac									/* 4 LDY ABS */
#define m6510_cc m6502_cc									/* 4 CPY ABS */
#define m6510_ec m6502_ec									/* 4 CPX ABS */

OP(1c) {		  m6502_ICount -= 2;		 TOP;		  } /* 2 TOP */
OP(3c) {		  m6502_ICount -= 2;		 TOP;		  } /* 2 TOP */
OP(5c) {		  m6502_ICount -= 2;		 TOP;		  } /* 2 TOP */
OP(7c) {		  m6502_ICount -= 2;		 TOP;		  } /* 2 TOP */
OP(9c) { int tmp; m6502_ICount -= 5; EA_ABX; SYH; WB_EA;  } /* 5 SYH ABX */
#define m6510_bc m6502_bc									/* 4 LDY ABX */
OP(dc) {		  m6502_ICount -= 2;		 TOP;		  } /* 2 TOP */
OP(fc) {		  m6502_ICount -= 2;		 TOP;		  } /* 2 TOP */

#define m6510_0d m6502_0d									/* 4 ORA ABS */
#define m6510_2d m6502_2d									/* 4 AND ABS */
#define m6510_4d m6502_4d									/* 4 EOR ABS */
#define m6510_6d m6502_6d									/* 4 ADC ABS */
#define m6510_8d m6502_8d									/* 4 STA ABS */
#define m6510_ad m6502_ad									/* 4 LDA ABS */
#define m6510_cd m6502_cd									/* 4 CMP ABS */
#define m6510_ed m6502_ed									/* 4 SBC ABS */

#define m6510_1d m6502_1d									/* 4 ORA ABX */
#define m6510_3d m6502_3d									/* 4 AND ABX */
#define m6510_5d m6502_5d									/* 4 EOR ABX */
#define m6510_7d m6502_7d									/* 4 ADC ABX */
#define m6510_9d m6502_9d									/* 5 STA ABX */
#define m6510_bd m6502_bd									/* 4 LDA ABX */
#define m6510_dd m6502_dd									/* 4 CMP ABX */
#define m6510_fd m6502_fd									/* 4 SBC ABX */


OP(0e) { int tmp; m6502_ICount -= 6; RD_ABS; WB_EA; ASL; WB_EA;  } /* 6 ASL ABS */
OP(2e) { int tmp; m6502_ICount -= 6; RD_ABS; WB_EA; ROL; WB_EA;  } /* 6 ROL ABS */
OP(4e) { int tmp; m6502_ICount -= 6; RD_ABS; WB_EA; LSR; WB_EA;  } /* 6 LSR ABS */
OP(6e) { int tmp; m6502_ICount -= 6; RD_ABS; WB_EA; ROR; WB_EA;  } /* 6 ROR ABS */
#define m6510_8e m6502_8e									/* 5 STX ABS */
#define m6510_ae m6502_ae									/* 4 LDX ABS */
OP(ce) { int tmp; m6502_ICount -= 6; RD_ABS; WB_EA; DEC; WB_EA;  } /* 6 DEC ABS */
OP(ee) { int tmp; m6502_ICount -= 6; RD_ABS; WB_EA; INC; WB_EA;  } /* 6 INC ABS */

OP(1e) { int tmp; m6502_ICount -= 7; RD_ABX; WB_EA; ASL; WB_EA;  } /* 7 ASL ABX */
OP(3e) { int tmp; m6502_ICount -= 7; RD_ABX; WB_EA; ROL; WB_EA;  } /* 7 ROL ABX */
OP(5e) { int tmp; m6502_ICount -= 7; RD_ABX; WB_EA; LSR; WB_EA;  } /* 7 LSR ABX */
OP(7e) { int tmp; m6502_ICount -= 7; RD_ABX; WB_EA; ROR; WB_EA;  } /* 7 ROR ABX */
OP(9e) { int tmp; m6502_ICount -= 2; EA_ABY; SXH; WB_EA;  } /* 2 SXH ABY */
#define m6510_be m6502_be									/* 4 LDX ABY */
OP(de) { int tmp; m6502_ICount -= 7; RD_ABX; WB_EA; DEC; WB_EA;  } /* 7 DEC ABX */
OP(fe) { int tmp; m6502_ICount -= 7; RD_ABX; WB_EA; INC; WB_EA;  } /* 7 INC ABX */

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

static void (*insn6510[0x100])(void) = {
	m6510_00,m6510_01,m6510_02,m6510_03,m6510_04,m6510_05,m6510_06,m6510_07,
	m6510_08,m6510_09,m6510_0a,m6510_0b,m6510_0c,m6510_0d,m6510_0e,m6510_0f,
	m6510_10,m6510_11,m6510_12,m6510_13,m6510_14,m6510_15,m6510_16,m6510_17,
	m6510_18,m6510_19,m6510_1a,m6510_1b,m6510_1c,m6510_1d,m6510_1e,m6510_1f,
	m6510_20,m6510_21,m6510_22,m6510_23,m6510_24,m6510_25,m6510_26,m6510_27,
	m6510_28,m6510_29,m6510_2a,m6510_2b,m6510_2c,m6510_2d,m6510_2e,m6510_2f,
	m6510_30,m6510_31,m6510_32,m6510_33,m6510_34,m6510_35,m6510_36,m6510_37,
	m6510_38,m6510_39,m6510_3a,m6510_3b,m6510_3c,m6510_3d,m6510_3e,m6510_3f,
	m6510_40,m6510_41,m6510_42,m6510_43,m6510_44,m6510_45,m6510_46,m6510_47,
	m6510_48,m6510_49,m6510_4a,m6510_4b,m6510_4c,m6510_4d,m6510_4e,m6510_4f,
	m6510_50,m6510_51,m6510_52,m6510_53,m6510_54,m6510_55,m6510_56,m6510_57,
	m6510_58,m6510_59,m6510_5a,m6510_5b,m6510_5c,m6510_5d,m6510_5e,m6510_5f,
	m6510_60,m6510_61,m6510_62,m6510_63,m6510_64,m6510_65,m6510_66,m6510_67,
	m6510_68,m6510_69,m6510_6a,m6510_6b,m6510_6c,m6510_6d,m6510_6e,m6510_6f,
	m6510_70,m6510_71,m6510_72,m6510_73,m6510_74,m6510_75,m6510_76,m6510_77,
	m6510_78,m6510_79,m6510_7a,m6510_7b,m6510_7c,m6510_7d,m6510_7e,m6510_7f,
	m6510_80,m6510_81,m6510_82,m6510_83,m6510_84,m6510_85,m6510_86,m6510_87,
	m6510_88,m6510_89,m6510_8a,m6510_8b,m6510_8c,m6510_8d,m6510_8e,m6510_8f,
	m6510_90,m6510_91,m6510_92,m6510_93,m6510_94,m6510_95,m6510_96,m6510_97,
	m6510_98,m6510_99,m6510_9a,m6510_9b,m6510_9c,m6510_9d,m6510_9e,m6510_9f,
	m6510_a0,m6510_a1,m6510_a2,m6510_a3,m6510_a4,m6510_a5,m6510_a6,m6510_a7,
	m6510_a8,m6510_a9,m6510_aa,m6510_ab,m6510_ac,m6510_ad,m6510_ae,m6510_af,
	m6510_b0,m6510_b1,m6510_b2,m6510_b3,m6510_b4,m6510_b5,m6510_b6,m6510_b7,
	m6510_b8,m6510_b9,m6510_ba,m6510_bb,m6510_bc,m6510_bd,m6510_be,m6510_bf,
	m6510_c0,m6510_c1,m6510_c2,m6510_c3,m6510_c4,m6510_c5,m6510_c6,m6510_c7,
	m6510_c8,m6510_c9,m6510_ca,m6510_cb,m6510_cc,m6510_cd,m6510_ce,m6510_cf,
	m6510_d0,m6510_d1,m6510_d2,m6510_d3,m6510_d4,m6510_d5,m6510_d6,m6510_d7,
	m6510_d8,m6510_d9,m6510_da,m6510_db,m6510_dc,m6510_dd,m6510_de,m6510_df,
	m6510_e0,m6510_e1,m6510_e2,m6510_e3,m6510_e4,m6510_e5,m6510_e6,m6510_e7,
	m6510_e8,m6510_e9,m6510_ea,m6510_eb,m6510_ec,m6510_ed,m6510_ee,m6510_ef,
	m6510_f0,m6510_f1,m6510_f2,m6510_f3,m6510_f4,m6510_f5,m6510_f6,m6510_f7,
	m6510_f8,m6510_f9,m6510_fa,m6510_fb,m6510_fc,m6510_fd,m6510_fe,m6510_ff
};
