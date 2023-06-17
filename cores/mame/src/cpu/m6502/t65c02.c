/*****************************************************************************
 *
 *	 tbl65c02.c
 *	 65c02 opcode functions and function pointer table
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
 *****************************************************************************/

#undef	OP
#define OP(nn) static INLINE void m65c02_##nn(void)

/*****************************************************************************
 *****************************************************************************
 *
 *	 overrides for 65C02 opcodes
 *
 *****************************************************************************
 * op	 temp	  cycles			 rdmem	 opc  wrmem   ********************/
OP(00) {		  m6502_ICount -= 7;		 BRK;		  } /* 7 BRK */
#define m65c02_20 m6502_20									/* 6 JSR ABS */
#define m65c02_40 m6502_40									/* 6 RTI */
#define m65c02_60 m6502_60									/* 6 RTS */
OP(80) { int tmp;							 BRA(1);	  } /* 2 BRA */
#define m65c02_a0 m6502_a0									/* 2 LDY IMM */
#define m65c02_c0 m6502_c0									/* 2 CPY IMM */
#define m65c02_e0 m6502_e0									/* 2 CPX IMM */

#define m65c02_10 m6502_10									/* 2 BPL */
#define m65c02_30 m6502_30									/* 2 BMI */
#define m65c02_50 m6502_50									/* 2 BVC */
#define m65c02_70 m6502_70									/* 2 BVS */
#define m65c02_90 m6502_90									/* 2 BCC */
#define m65c02_b0 m6502_b0									/* 2 BCS */
#define m65c02_d0 m6502_d0									/* 2 BNE */
#define m65c02_f0 m6502_f0									/* 2 BEQ */

#define m65c02_01 m6502_01									/* 6 ORA IDX */
#define m65c02_21 m6502_21									/* 6 AND IDX */
#define m65c02_41 m6502_41									/* 6 EOR IDX */
OP(61) { int tmp; m6502_ICount -= 6; RD_IDX; ADC;		  } /* 6 ADC IDX */
#define m65c02_81 m6502_81									/* 6 STA IDX */
#define m65c02_a1 m6502_a1									/* 6 LDA IDX */
#define m65c02_c1 m6502_c1									/* 6 CMP IDX */
OP(e1) { int tmp; m6502_ICount -= 6; RD_IDX; SBC;		  } /* 6 SBC IDX */

#define m65c02_11 m6502_11									/* 5 ORA IDY; */
#define m65c02_31 m6502_31									/* 5 AND IDY; */
#define m65c02_51 m6502_51									/* 5 EOR IDY; */
OP(71) { int tmp; m6502_ICount -= 5; RD_IDY; ADC;		  } /* 5 ADC IDY */
#define m65c02_91 m6502_91									/* 6 STA IDY; */
#define m65c02_b1 m6502_b1									/* 5 LDA IDY; */
#define m65c02_d1 m6502_d1									/* 5 CMP IDY; */
OP(f1) { int tmp; m6502_ICount -= 5; RD_IDY; SBC;		  } /* 5 SBC IDY */

#define m65c02_02 m6502_02									/* 2 ILL */
#define m65c02_22 m6502_22									/* 2 ILL */
#define m65c02_42 m6502_42									/* 2 ILL */
#define m65c02_62 m6502_62									/* 2 ILL */
#define m65c02_82 m6502_82									/* 2 ILL */
#define m65c02_a2 m6502_a2									/* 2 LDX IMM */
#define m65c02_c2 m6502_c2									/* 2 ILL */
#define m65c02_e2 m6502_e2									/* 2 ILL */

#ifndef CORE_M65CE02
OP(12) { int tmp; m6502_ICount -= 3; RD_ZPI; ORA;		  } /* 3 ORA ZPI */
OP(32) { int tmp; m6502_ICount -= 3; RD_ZPI; AND;		  } /* 3 AND ZPI */
OP(52) { int tmp; m6502_ICount -= 3; RD_ZPI; EOR;		  } /* 3 EOR ZPI */
OP(72) { int tmp; m6502_ICount -= 3; RD_ZPI; ADC;		  } /* 3 ADC ZPI */
OP(92) { int tmp; m6502_ICount -= 4;		 STA; WR_ZPI; } /* 3 STA ZPI */
OP(b2) { int tmp; m6502_ICount -= 3; RD_ZPI; LDA;		  } /* 3 LDA ZPI */
OP(d2) { int tmp; m6502_ICount -= 3; RD_ZPI; CMP;		  } /* 3 CMP ZPI */
OP(f2) { int tmp; m6502_ICount -= 3; RD_ZPI; SBC;		  } /* 3 SBC ZPI */
#endif

#define m65c02_03 m6502_03									/* 2 ILL */
#define m65c02_23 m6502_23									/* 2 ILL */
#define m65c02_43 m6502_43									/* 2 ILL */
#define m65c02_63 m6502_63									/* 2 ILL */
#define m65c02_83 m6502_83									/* 2 ILL */
#define m65c02_a3 m6502_a3									/* 2 ILL */
#define m65c02_c3 m6502_c3									/* 2 ILL */
#define m65c02_e3 m6502_e3									/* 2 ILL */

#define m65c02_13 m6502_13									/* 2 ILL */
#define m65c02_33 m6502_33									/* 2 ILL */
#define m65c02_53 m6502_53									/* 2 ILL */
#define m65c02_73 m6502_73									/* 2 ILL */
#define m65c02_93 m6502_93									/* 2 ILL */
#define m65c02_b3 m6502_b3									/* 2 ILL */
#define m65c02_d3 m6502_d3									/* 2 ILL */
#define m65c02_f3 m6502_f3									/* 2 ILL */

OP(04) { int tmp; m6502_ICount -= 3; RD_ZPG; TSB; WB_EA;  } /* 3 TSB ZPG */
#define m65c02_24 m6502_24									/* 3 BIT ZPG */
#define m65c02_44 m6502_44									/* 2 ILL */
OP(64) { int tmp; m6502_ICount -= 2;		 STZ; WR_ZPG; } /* 3 STZ ZPG */
#define m65c02_84 m6502_84									/* 3 STY ZPG */
#define m65c02_a4 m6502_a4									/* 3 LDY ZPG */
#define m65c02_c4 m6502_c4									/* 3 CPY ZPG */
#define m65c02_e4 m6502_e4									/* 3 CPX ZPG */

OP(14) { int tmp; m6502_ICount -= 3; RD_ZPG; TRB; WB_EA;  } /* 3 TRB ZPG */
OP(34) { int tmp; m6502_ICount -= 4; RD_ZPX; BIT;		  } /* 4 BIT ZPX */
#define m65c02_54 m6502_54									/* 2 ILL */
OP(74) { int tmp; m6502_ICount -= 4;		 STZ; WR_ZPX; } /* 4 STZ ZPX */
#define m65c02_94 m6502_94									/* 4 STY ZPX */
#define m65c02_b4 m6502_b4									/* 4 LDY ZPX */
#define m65c02_d4 m6502_d4									/* 2 ILL */
#define m65c02_f4 m6502_f4									/* 2 ILL */

#define m65c02_05 m6502_05									/* 3 ORA ZPG */
#define m65c02_25 m6502_25									/* 3 AND ZPG */
#define m65c02_45 m6502_45									/* 3 EOR ZPG */
OP(65) { int tmp; m6502_ICount -= 3; RD_ZPG; ADC;		  } /* 3 ADC ZPG */
#define m65c02_85 m6502_85									/* 3 STA ZPG */
#define m65c02_a5 m6502_a5									/* 3 LDA ZPG */
#define m65c02_c5 m6502_c5									/* 3 CMP ZPG */
OP(e5) { int tmp; m6502_ICount -= 3; RD_ZPG; SBC;		  } /* 3 SBC ZPG */

#define m65c02_15 m6502_15									/* 4 ORA ZPX */
#define m65c02_35 m6502_35									/* 4 AND ZPX */
#define m65c02_55 m6502_55									/* 4 EOR ZPX */
OP(75) { int tmp; m6502_ICount -= 4; RD_ZPX; ADC;		  } /* 4 ADC ZPX */
#define m65c02_95 m6502_95									/* 4 STA ZPX */
#define m65c02_b5 m6502_b5									/* 4 LDA ZPX */
#define m65c02_d5 m6502_d5									/* 4 CMP ZPX */
OP(f5) { int tmp; m6502_ICount -= 4; RD_ZPX; SBC;		  } /* 4 SBC ZPX */

#define m65c02_06 m6502_06									/* 5 ASL ZPG */
#define m65c02_26 m6502_26									/* 5 ROL ZPG */
#define m65c02_46 m6502_46									/* 5 LSR ZPG */
#define m65c02_66 m6502_66									/* 5 ROR ZPG */
#define m65c02_86 m6502_86									/* 3 STX ZPG */
#define m65c02_a6 m6502_a6									/* 3 LDX ZPG */
#define m65c02_c6 m6502_c6									/* 5 DEC ZPG */
#define m65c02_e6 m6502_e6									/* 5 INC ZPG */

#define m65c02_16 m6502_16									/* 6 ASL ZPX */
#define m65c02_36 m6502_36									/* 6 ROL ZPX */
#define m65c02_56 m6502_56									/* 6 LSR ZPX */
#define m65c02_76 m6502_76									/* 6 ROR ZPX */
#define m65c02_96 m6502_96									/* 4 STX ZPY */
#define m65c02_b6 m6502_b6									/* 4 LDX ZPY */
#define m65c02_d6 m6502_d6									/* 6 DEC ZPX */
#define m65c02_f6 m6502_f6									/* 6 INC ZPX */

OP(07) { int tmp; m6502_ICount -= 5; RD_ZPG; RMB(0);WB_EA;} /* 5 RMB0 ZPG */
OP(27) { int tmp; m6502_ICount -= 5; RD_ZPG; RMB(2);WB_EA;} /* 5 RMB2 ZPG */
OP(47) { int tmp; m6502_ICount -= 5; RD_ZPG; RMB(4);WB_EA;} /* 5 RMB4 ZPG */
OP(67) { int tmp; m6502_ICount -= 5; RD_ZPG; RMB(6);WB_EA;} /* 5 RMB6 ZPG */
OP(87) { int tmp; m6502_ICount -= 5; RD_ZPG; SMB(0);WB_EA;} /* 5 SMB0 ZPG */
OP(a7) { int tmp; m6502_ICount -= 5; RD_ZPG; SMB(2);WB_EA;} /* 5 SMB2 ZPG */
OP(c7) { int tmp; m6502_ICount -= 5; RD_ZPG; SMB(4);WB_EA;} /* 5 SMB4 ZPG */
OP(e7) { int tmp; m6502_ICount -= 5; RD_ZPG; SMB(6);WB_EA;} /* 5 SMB6 ZPG */

OP(17) { int tmp; m6502_ICount -= 5; RD_ZPG; RMB(1);WB_EA;} /* 5 RMB1 ZPG */
OP(37) { int tmp; m6502_ICount -= 5; RD_ZPG; RMB(3);WB_EA;} /* 5 RMB3 ZPG */
OP(57) { int tmp; m6502_ICount -= 5; RD_ZPG; RMB(5);WB_EA;} /* 5 RMB5 ZPG */
OP(77) { int tmp; m6502_ICount -= 5; RD_ZPG; RMB(7);WB_EA;} /* 5 RMB7 ZPG */
OP(97) { int tmp; m6502_ICount -= 5; RD_ZPG; SMB(1);WB_EA;} /* 5 SMB1 ZPG */
OP(b7) { int tmp; m6502_ICount -= 5; RD_ZPG; SMB(3);WB_EA;} /* 5 SMB3 ZPG */
OP(d7) { int tmp; m6502_ICount -= 5; RD_ZPG; SMB(5);WB_EA;} /* 5 SMB5 ZPG */
OP(f7) { int tmp; m6502_ICount -= 5; RD_ZPG; SMB(7);WB_EA;} /* 5 SMB7 ZPG */

#define m65c02_08 m6502_08									/* 3 PHP */
#define m65c02_28 m6502_28									/* 4 PLP */
#define m65c02_48 m6502_48									/* 3 PHA */
#define m65c02_68 m6502_68									/* 4 PLA */
#define m65c02_88 m6502_88									/* 2 DEY */
#define m65c02_a8 m6502_a8									/* 2 TAY */
#define m65c02_c8 m6502_c8									/* 2 INY */
#define m65c02_e8 m6502_e8									/* 2 INX */

#define m65c02_18 m6502_18									/* 2 CLC */
#define m65c02_38 m6502_38									/* 2 SEC */
#define m65c02_58 m6502_58									/* 2 CLI */
#define m65c02_78 m6502_78									/* 2 SEI */
#define m65c02_98 m6502_98									/* 2 TYA */
#define m65c02_b8 m6502_b8									/* 2 CLV */
#define m65c02_d8 m6502_d8									/* 2 CLD */
#define m65c02_f8 m6502_f8									/* 2 SED */

#define m65c02_09 m6502_09									/* 2 ORA IMM */
#define m65c02_29 m6502_29									/* 2 AND IMM */
#define m65c02_49 m6502_49									/* 2 EOR IMM */
OP(69) { int tmp; m6502_ICount -= 2; RD_IMM; ADC;		  } /* 2 ADC IMM */
OP(89) { int tmp; m6502_ICount -= 2; RD_IMM; BIT;		  } /* 2 BIT IMM */
#define m65c02_a9 m6502_a9									/* 2 LDA IMM */
#define m65c02_c9 m6502_c9									/* 2 CMP IMM */
OP(e9) { int tmp; m6502_ICount -= 2; RD_IMM; SBC;		  } /* 2 SBC IMM */

#define m65c02_19 m6502_19									/* 4 ORA ABY */
#define m65c02_39 m6502_39									/* 4 AND ABY */
#define m65c02_59 m6502_59									/* 4 EOR ABY */
OP(79) { int tmp; m6502_ICount -= 4; RD_ABY; ADC;		  } /* 4 ADC ABY */
#define m65c02_99 m6502_99									/* 5 STA ABY */
#define m65c02_b9 m6502_b9									/* 4 LDA ABY */
#define m65c02_d9 m6502_d9									/* 4 CMP ABY */
OP(f9) { int tmp; m6502_ICount -= 4; RD_ABY; SBC;		  } /* 4 SBC ABY */

#define m65c02_0a m6502_0a									/* 2 ASL */
#define m65c02_2a m6502_2a									/* 2 ROL */
#define m65c02_4a m6502_4a									/* 2 LSR */
#define m65c02_6a m6502_6a									/* 2 ROR */
#define m65c02_8a m6502_8a									/* 2 TXA */
#define m65c02_aa m6502_aa									/* 2 TAX */
#define m65c02_ca m6502_ca									/* 2 DEX */
#define m65c02_ea m6502_ea									/* 2 NOP */

OP(1a) {		  m6502_ICount -= 2;		 INA;		  } /* 2 INA */
OP(3a) {		  m6502_ICount -= 2;		 DEA;		  } /* 2 DEA */
OP(5a) {		  m6502_ICount -= 3;		 PHY;		  } /* 3 PHY */
OP(7a) {		  m6502_ICount -= 4;		 PLY;		  } /* 4 PLY */
#define m65c02_9a m6502_9a									/* 2 TXS */
#define m65c02_ba m6502_ba									/* 2 TSX */
OP(da) {		  m6502_ICount -= 3;		 PHX;		  } /* 3 PHX */
OP(fa) {		  m6502_ICount -= 4;		 PLX;		  } /* 4 PLX */

#define m65c02_0b m6502_0b									/* 2 ILL */
#define m65c02_2b m6502_2b									/* 2 ILL */
#define m65c02_4b m6502_4b									/* 2 ILL */
#define m65c02_6b m6502_6b									/* 2 ILL */
#define m65c02_8b m6502_8b									/* 2 ILL */
#define m65c02_ab m6502_ab									/* 2 ILL */
#define m65c02_cb m6502_cb									/* 2 ILL */
#define m65c02_eb m6502_eb									/* 2 ILL */

#define m65c02_1b m6502_1b									/* 2 ILL */
#define m65c02_3b m6502_3b									/* 2 ILL */
#define m65c02_5b m6502_5b									/* 2 ILL */
#define m65c02_7b m6502_7b									/* 2 ILL */
#define m65c02_9b m6502_9b									/* 2 ILL */
#define m65c02_bb m6502_bb									/* 2 ILL */
#define m65c02_db m6502_db									/* 2 ILL */
#define m65c02_fb m6502_fb									/* 2 ILL */

OP(0c) { int tmp; m6502_ICount -= 2; RD_ABS; TSB; WB_EA;  } /* 4 TSB ABS */
#define m65c02_2c m6502_2c									/* 4 BIT ABS */
#define m65c02_4c m6502_4c									/* 3 JMP ABS */
OP(6c) { int tmp; m6502_ICount -= 5; EA_IND; JMP;		  } /* 5 JMP IND */
#define m65c02_8c m6502_8c									/* 4 STY ABS */
#define m65c02_ac m6502_ac									/* 4 LDY ABS */
#define m65c02_cc m6502_cc									/* 4 CPY ABS */
#define m65c02_ec m6502_ec									/* 4 CPX ABS */

OP(1c) { int tmp; m6502_ICount -= 4; RD_ABS; TRB; WB_EA;  } /* 4 TRB ABS */
OP(3c) { int tmp; m6502_ICount -= 4; RD_ABX; BIT;		  } /* 4 BIT ABX */
#define m65c02_5c m6502_5c									/* 2 ILL */
OP(7c) { int tmp; m6502_ICount -= 2; EA_IAX; JMP;		  } /* 6 JMP IAX */
OP(9c) { int tmp; m6502_ICount -= 4;		 STZ; WR_ABS; } /* 4 STZ ABS */
#define m65c02_bc m6502_bc									/* 4 LDY ABX */
#define m65c02_dc m6502_dc									/* 2 ILL */
#define m65c02_fc m6502_fc									/* 2 ILL */

#define m65c02_0d m6502_0d									/* 4 ORA ABS */
#define m65c02_2d m6502_2d									/* 4 AND ABS */
#define m65c02_4d m6502_4d									/* 4 EOR ABS */
OP(6d) { int tmp; m6502_ICount -= 4; RD_ABS; ADC;		  } /* 4 ADC ABS */
#define m65c02_8d m6502_8d									/* 4 STA ABS */
#define m65c02_ad m6502_ad									/* 4 LDA ABS */
#define m65c02_cd m6502_cd									/* 4 CMP ABS */
OP(ed) { int tmp; m6502_ICount -= 4; RD_ABS; SBC;		  } /* 4 SBC ABS */

#define m65c02_1d m6502_1d									/* 4 ORA ABX */
#define m65c02_3d m6502_3d									/* 4 AND ABX */
#define m65c02_5d m6502_5d									/* 4 EOR ABX */
OP(7d) { int tmp; m6502_ICount -= 4; RD_ABX; ADC;		  } /* 4 ADC ABX */
#define m65c02_9d m6502_9d									/* 5 STA ABX */
#define m65c02_bd m6502_bd									/* 4 LDA ABX */
#define m65c02_dd m6502_dd									/* 4 CMP ABX */
OP(fd) { int tmp; m6502_ICount -= 4; RD_ABX; SBC;		  } /* 4 SBC ABX */

#define m65c02_0e m6502_0e									/* 6 ASL ABS */
#define m65c02_2e m6502_2e									/* 6 ROL ABS */
#define m65c02_4e m6502_4e									/* 6 LSR ABS */
#define m65c02_6e m6502_6e									/* 6 ROR ABS */
#define m65c02_8e m6502_8e									/* 4 STX ABS */
#define m65c02_ae m6502_ae									/* 4 LDX ABS */
#define m65c02_ce m6502_ce									/* 6 DEC ABS */
#define m65c02_ee m6502_ee									/* 6 INC ABS */

#define m65c02_1e m6502_1e									/* 7 ASL ABX */
#define m65c02_3e m6502_3e									/* 7 ROL ABX */
#define m65c02_5e m6502_5e									/* 7 LSR ABX */
#define m65c02_7e m6502_7e									/* 7 ROR ABX */
OP(9e) { int tmp; m6502_ICount -= 5;		 STZ; WR_ABX; } /* 5 STZ ABX */
#define m65c02_be m6502_be									/* 4 LDX ABY */
#define m65c02_de m6502_de									/* 7 DEC ABX */
#define m65c02_fe m6502_fe									/* 7 INC ABX */

OP(0f) { int tmp; m6502_ICount -= 5; RD_ZPG; BBR(0);	  } /* 5 BBR0 ZPG */
OP(2f) { int tmp; m6502_ICount -= 5; RD_ZPG; BBR(2);	  } /* 5 BBR2 ZPG */
OP(4f) { int tmp; m6502_ICount -= 5; RD_ZPG; BBR(4);	  } /* 5 BBR4 ZPG */
OP(6f) { int tmp; m6502_ICount -= 5; RD_ZPG; BBR(6);	  } /* 5 BBR6 ZPG */
OP(8f) { int tmp; m6502_ICount -= 5; RD_ZPG; BBS(0);	  } /* 5 BBS0 ZPG */
OP(af) { int tmp; m6502_ICount -= 5; RD_ZPG; BBS(2);	  } /* 5 BBS2 ZPG */
OP(cf) { int tmp; m6502_ICount -= 5; RD_ZPG; BBS(4);	  } /* 5 BBS4 ZPG */
OP(ef) { int tmp; m6502_ICount -= 5; RD_ZPG; BBS(6);	  } /* 5 BBS6 ZPG */

OP(1f) { int tmp; m6502_ICount -= 5; RD_ZPG; BBR(1);	  } /* 5 BBR1 ZPG */
OP(3f) { int tmp; m6502_ICount -= 5; RD_ZPG; BBR(3);	  } /* 5 BBR3 ZPG */
OP(5f) { int tmp; m6502_ICount -= 5; RD_ZPG; BBR(5);	  } /* 5 BBR5 ZPG */
OP(7f) { int tmp; m6502_ICount -= 5; RD_ZPG; BBR(7);	  } /* 5 BBR7 ZPG */
OP(9f) { int tmp; m6502_ICount -= 5; RD_ZPG; BBS(1);	  } /* 5 BBS1 ZPG */
OP(bf) { int tmp; m6502_ICount -= 5; RD_ZPG; BBS(3);	  } /* 5 BBS3 ZPG */
OP(df) { int tmp; m6502_ICount -= 5; RD_ZPG; BBS(5);	  } /* 5 BBS5 ZPG */
OP(ff) { int tmp; m6502_ICount -= 5; RD_ZPG; BBS(7);	  } /* 5 BBS7 ZPG */

static void (*insn65c02[0x100])(void) = {
	m65c02_00,m65c02_01,m65c02_02,m65c02_03,m65c02_04,m65c02_05,m65c02_06,m65c02_07,
	m65c02_08,m65c02_09,m65c02_0a,m65c02_0b,m65c02_0c,m65c02_0d,m65c02_0e,m65c02_0f,
	m65c02_10,m65c02_11,m65c02_12,m65c02_13,m65c02_14,m65c02_15,m65c02_16,m65c02_17,
	m65c02_18,m65c02_19,m65c02_1a,m65c02_1b,m65c02_1c,m65c02_1d,m65c02_1e,m65c02_1f,
	m65c02_20,m65c02_21,m65c02_22,m65c02_23,m65c02_24,m65c02_25,m65c02_26,m65c02_27,
	m65c02_28,m65c02_29,m65c02_2a,m65c02_2b,m65c02_2c,m65c02_2d,m65c02_2e,m65c02_2f,
	m65c02_30,m65c02_31,m65c02_32,m65c02_33,m65c02_34,m65c02_35,m65c02_36,m65c02_37,
	m65c02_38,m65c02_39,m65c02_3a,m65c02_3b,m65c02_3c,m65c02_3d,m65c02_3e,m65c02_3f,
	m65c02_40,m65c02_41,m65c02_42,m65c02_43,m65c02_44,m65c02_45,m65c02_46,m65c02_47,
	m65c02_48,m65c02_49,m65c02_4a,m65c02_4b,m65c02_4c,m65c02_4d,m65c02_4e,m65c02_4f,
	m65c02_50,m65c02_51,m65c02_52,m65c02_53,m65c02_54,m65c02_55,m65c02_56,m65c02_57,
	m65c02_58,m65c02_59,m65c02_5a,m65c02_5b,m65c02_5c,m65c02_5d,m65c02_5e,m65c02_5f,
	m65c02_60,m65c02_61,m65c02_62,m65c02_63,m65c02_64,m65c02_65,m65c02_66,m65c02_67,
	m65c02_68,m65c02_69,m65c02_6a,m65c02_6b,m65c02_6c,m65c02_6d,m65c02_6e,m65c02_6f,
	m65c02_70,m65c02_71,m65c02_72,m65c02_73,m65c02_74,m65c02_75,m65c02_76,m65c02_77,
	m65c02_78,m65c02_79,m65c02_7a,m65c02_7b,m65c02_7c,m65c02_7d,m65c02_7e,m65c02_7f,
	m65c02_80,m65c02_81,m65c02_82,m65c02_83,m65c02_84,m65c02_85,m65c02_86,m65c02_87,
	m65c02_88,m65c02_89,m65c02_8a,m65c02_8b,m65c02_8c,m65c02_8d,m65c02_8e,m65c02_8f,
	m65c02_90,m65c02_91,m65c02_92,m65c02_93,m65c02_94,m65c02_95,m65c02_96,m65c02_97,
	m65c02_98,m65c02_99,m65c02_9a,m65c02_9b,m65c02_9c,m65c02_9d,m65c02_9e,m65c02_9f,
	m65c02_a0,m65c02_a1,m65c02_a2,m65c02_a3,m65c02_a4,m65c02_a5,m65c02_a6,m65c02_a7,
	m65c02_a8,m65c02_a9,m65c02_aa,m65c02_ab,m65c02_ac,m65c02_ad,m65c02_ae,m65c02_af,
	m65c02_b0,m65c02_b1,m65c02_b2,m65c02_b3,m65c02_b4,m65c02_b5,m65c02_b6,m65c02_b7,
	m65c02_b8,m65c02_b9,m65c02_ba,m65c02_bb,m65c02_bc,m65c02_bd,m65c02_be,m65c02_bf,
	m65c02_c0,m65c02_c1,m65c02_c2,m65c02_c3,m65c02_c4,m65c02_c5,m65c02_c6,m65c02_c7,
	m65c02_c8,m65c02_c9,m65c02_ca,m65c02_cb,m65c02_cc,m65c02_cd,m65c02_ce,m65c02_cf,
	m65c02_d0,m65c02_d1,m65c02_d2,m65c02_d3,m65c02_d4,m65c02_d5,m65c02_d6,m65c02_d7,
	m65c02_d8,m65c02_d9,m65c02_da,m65c02_db,m65c02_dc,m65c02_dd,m65c02_de,m65c02_df,
	m65c02_e0,m65c02_e1,m65c02_e2,m65c02_e3,m65c02_e4,m65c02_e5,m65c02_e6,m65c02_e7,
	m65c02_e8,m65c02_e9,m65c02_ea,m65c02_eb,m65c02_ec,m65c02_ed,m65c02_ee,m65c02_ef,
	m65c02_f0,m65c02_f1,m65c02_f2,m65c02_f3,m65c02_f4,m65c02_f5,m65c02_f6,m65c02_f7,
	m65c02_f8,m65c02_f9,m65c02_fa,m65c02_fb,m65c02_fc,m65c02_fd,m65c02_fe,m65c02_ff
};
