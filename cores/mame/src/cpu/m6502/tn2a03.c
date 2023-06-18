/*****************************************************************************
 *
 *	 tbl2a03.c
 *	 2a03 opcode functions and function pointer table
 *
 *	 The 2a03 is a 6502 CPU that does not support the decimal mode
 *	 of the ADC and SBC instructions, so all opcodes except ADC/SBC
 *	 are simply mapped to the m6502 ones.
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

/*
  based on the nmos 6502
  illegal opcodes NOT added yet!
  (no program to test, all illegal opcodes not using adc/sbc should
  be the same as in nmos6502/6510!?)

  b flag handling might be changed,
  although only nmos series b-flag handling is quite sure
*/


#undef	OP
#define OP(nn) static INLINE void n2a03_##nn(void)

/*****************************************************************************
 *****************************************************************************
 *
 *	 overrides for 2a03 opcodes
 *
 *****************************************************************************
 ********** insn   temp 	cycles			   rdmem   opc	wrmem	**********/
#define n2a03_00 m6502_00									/* 7 BRK */
#define n2a03_20 m6502_20									/* 6 JSR */
#define n2a03_40 m6502_40									/* 6 RTI */
#define n2a03_60 m6502_60									/* 6 RTS */
#define n2a03_80 m6502_80									/* 2 ILL */
#define n2a03_a0 m6502_a0									/* 2 LDY IMM */
#define n2a03_c0 m6502_c0									/* 2 CPY IMM */
#define n2a03_e0 m6502_e0									/* 2 CPX IMM */

#define n2a03_10 m6502_10									/* 2 BPL */
#define n2a03_30 m6502_30									/* 2 BMI */
#define n2a03_50 m6502_50									/* 2 BVC */
#define n2a03_70 m6502_70									/* 2 BVS */
#define n2a03_90 m6502_90									/* 2 BCC */
#define n2a03_b0 m6502_b0									/* 2 BCS */
#define n2a03_d0 m6502_d0									/* 2 BNE */
#define n2a03_f0 m6502_f0									/* 2 BEQ */

#define n2a03_01 m6502_01									/* 6 ORA IDX */
#define n2a03_21 m6502_21									/* 6 AND IDX */
#define n2a03_41 m6502_41									/* 6 EOR IDX */
OP(61) { int tmp; m6502_ICount -= 6; RD_IDX; ADC_NES;	  } /* 6 ADC IDX */
#define n2a03_81 m6502_81									/* 6 STA IDX */
#define n2a03_a1 m6502_a1									/* 6 LDA IDX */
#define n2a03_c1 m6502_c1									/* 6 CMP IDX */
OP(e1) { int tmp; m6502_ICount -= 6; RD_IDX; SBC_NES;	  } /* 6 SBC IDX */

#define n2a03_11 m6502_11									/* 5 ORA IDY */
#define n2a03_31 m6502_31									/* 5 AND IDY */
#define n2a03_51 m6502_51									/* 5 EOR IDY */
OP(71) { int tmp; m6502_ICount -= 5; RD_IDY; ADC_NES;	  } /* 5 ADC IDY */
#define n2a03_91 m6502_91									/* 6 STA IDY */
#define n2a03_b1 m6502_b1									/* 5 LDA IDY */
#define n2a03_d1 m6502_d1									/* 5 CMP IDY */
OP(f1) { int tmp; m6502_ICount -= 5; RD_IDY; SBC_NES;	  } /* 5 SBC IDY */

#define n2a03_02 m6502_02									/* 2 ILL */
#define n2a03_22 m6502_22									/* 2 ILL */
#define n2a03_42 m6502_42									/* 2 ILL */
#define n2a03_62 m6502_62									/* 2 ILL */
#define n2a03_82 m6502_82									/* 2 ILL */
#define n2a03_a2 m6502_a2									/* 2 LDX IMM */
#define n2a03_c2 m6502_c2									/* 2 ILL */
#define n2a03_e2 m6502_e2									/* 2 ILL */

#define n2a03_12 m6502_12									/* 2 ILL */
#define n2a03_32 m6502_32									/* 2 ILL */
#define n2a03_52 m6502_52									/* 2 ILL */
#define n2a03_72 m6502_72									/* 2 ILL */
#define n2a03_92 m6502_92									/* 2 ILL */
#define n2a03_b2 m6502_b2									/* 2 ILL */
#define n2a03_d2 m6502_d2									/* 2 ILL */
#define n2a03_f2 m6502_f2									/* 2 ILL */

#define n2a03_03 m6502_03									/* 2 ILL */
#define n2a03_23 m6502_23									/* 2 ILL */
#define n2a03_43 m6502_43									/* 2 ILL */
#define n2a03_63 m6502_63									/* 2 ILL */
#define n2a03_83 m6502_83									/* 2 ILL */
#define n2a03_a3 m6502_a3									/* 2 ILL */
#define n2a03_c3 m6502_c3									/* 2 ILL */
#define n2a03_e3 m6502_e3									/* 2 ILL */

#define n2a03_13 m6502_13									/* 2 ILL */
#define n2a03_33 m6502_33									/* 2 ILL */
#define n2a03_53 m6502_53									/* 2 ILL */
#define n2a03_73 m6502_73									/* 2 ILL */
#define n2a03_93 m6502_93									/* 2 ILL */
#define n2a03_b3 m6502_b3									/* 2 ILL */
#define n2a03_d3 m6502_d3									/* 2 ILL */
#define n2a03_f3 m6502_f3									/* 2 ILL */

#define n2a03_04 m6502_04									/* 2 ILL */
#define n2a03_24 m6502_24									/* 3 BIT ZPG */
#define n2a03_44 m6502_44									/* 2 ILL */
#define n2a03_64 m6502_64									/* 2 ILL */
#define n2a03_84 m6502_84									/* 3 STY ZPG */
#define n2a03_a4 m6502_a4									/* 3 LDY ZPG */
#define n2a03_c4 m6502_c4									/* 3 CPY ZPG */
#define n2a03_e4 m6502_e4									/* 3 CPX ZPG */

#define n2a03_14 m6502_14									/* 2 ILL */
#define n2a03_34 m6502_34									/* 2 ILL */
#define n2a03_54 m6502_54									/* 2 ILL */
#define n2a03_74 m6502_74									/* 2 ILL */
#define n2a03_94 m6502_94									/* 4 STY ZP_X */
#define n2a03_b4 m6502_b4									/* 4 LDY ZP_X */
#define n2a03_d4 m6502_d4									/* 2 ILL */
#define n2a03_f4 m6502_f4									/* 2 ILL */

#define n2a03_05 m6502_05									/* 3 ORA ZPG */
#define n2a03_25 m6502_25									/* 3 AND ZPG */
#define n2a03_45 m6502_45									/* 3 EOR ZPG */
OP(65) { int tmp; m6502_ICount -= 3; RD_ZPG; ADC_NES;	  } /* 3 ADC ZPG */
#define n2a03_85 m6502_85									/* 3 STA ZPG */
#define n2a03_a5 m6502_a5									/* 3 LDA ZPG */
#define n2a03_c5 m6502_c5									/* 3 CMP ZPG */
OP(e5) { int tmp; m6502_ICount -= 3; RD_ZPG; SBC_NES;	  } /* 3 SBC ZPG */

#define n2a03_15 m6502_15									/* 4 ORA ZPX */
#define n2a03_35 m6502_35									/* 4 AND ZPX */
#define n2a03_55 m6502_55									/* 4 EOR ZPX */
OP(75) { int tmp; m6502_ICount -= 4; RD_ZPX; ADC_NES;	  } /* 4 ADC ZPX */
#define n2a03_95 m6502_95									/* 4 STA ZPX */
#define n2a03_b5 m6502_b5									/* 4 LDA ZPX */
#define n2a03_d5 m6502_d5									/* 4 CMP ZPX */
OP(f5) { int tmp; m6502_ICount -= 4; RD_ZPX; SBC_NES;	  } /* 4 SBC ZPX */

#define n2a03_06 m6502_06									/* 5 ASL ZPG */
#define n2a03_26 m6502_26									/* 5 ROL ZPG */
#define n2a03_46 m6502_46									/* 5 LSR ZPG */
#define n2a03_66 m6502_66									/* 5 ROR ZPG */
#define n2a03_86 m6502_86									/* 3 STX ZPG */
#define n2a03_a6 m6502_a6									/* 3 LDX ZPG */
#define n2a03_c6 m6502_c6									/* 5 DEC ZPG */
#define n2a03_e6 m6502_e6									/* 5 INC ZPG */

#define n2a03_16 m6502_16									/* 6 ASL ZPX */
#define n2a03_36 m6502_36									/* 6 ROL ZPX */
#define n2a03_56 m6502_56									/* 6 LSR ZPX */
#define n2a03_76 m6502_76									/* 6 ROR ZPX */
#define n2a03_96 m6502_96									/* 4 STX ZPY */
#define n2a03_b6 m6502_b6									/* 4 LDX ZPY */
#define n2a03_d6 m6502_d6									/* 6 DEC ZPX */
#define n2a03_f6 m6502_f6									/* 6 INC ZPX */

#define n2a03_07 m6502_07									/* 2 ILL */
#define n2a03_27 m6502_27									/* 2 ILL */
#define n2a03_47 m6502_47									/* 2 ILL */
#define n2a03_67 m6502_67									/* 2 ILL */
#define n2a03_87 m6502_87									/* 2 ILL */
#define n2a03_a7 m6502_a7									/* 2 ILL */
#define n2a03_c7 m6502_c7									/* 2 ILL */
#define n2a03_e7 m6502_e7									/* 2 ILL */

#define n2a03_17 m6502_17									/* 2 ILL */
#define n2a03_37 m6502_37									/* 2 ILL */
#define n2a03_57 m6502_57									/* 2 ILL */
#define n2a03_77 m6502_77									/* 2 ILL */
#define n2a03_97 m6502_97									/* 2 ILL */
#define n2a03_b7 m6502_b7									/* 2 ILL */
#define n2a03_d7 m6502_d7									/* 2 ILL */
#define n2a03_f7 m6502_f7									/* 2 ILL */

#define n2a03_08 m6502_08									/* 2 PHP */
#define n2a03_28 m6502_28									/* 2 PLP */
#define n2a03_48 m6502_48									/* 2 PHA */
#define n2a03_68 m6502_68									/* 2 PLA */
#define n2a03_88 m6502_88									/* 2 DEY */
#define n2a03_a8 m6502_a8									/* 2 TAY */
#define n2a03_c8 m6502_c8									/* 2 INY */
#define n2a03_e8 m6502_e8									/* 2 INX */

#define n2a03_18 m6502_18									/* 2 CLC */
#define n2a03_38 m6502_38									/* 2 SEC */
#define n2a03_58 m6502_58									/* 2 CLI */
#define n2a03_78 m6502_78									/* 2 SEI */
#define n2a03_98 m6502_98									/* 2 TYA */
#define n2a03_b8 m6502_b8									/* 2 CLV */
#define n2a03_d8 m6502_d8									/* 2 CLD */
#define n2a03_f8 m6502_f8									/* 2 SED */

#define n2a03_09 m6502_09									/* 2 ORA IMM */
#define n2a03_29 m6502_29									/* 2 AND IMM */
#define n2a03_49 m6502_49									/* 2 EOR IMM */
OP(69) { int tmp; m6502_ICount -= 2; RD_IMM; ADC_NES;	  } /* 2 ADC IMM */
#define n2a03_89 m6502_89									/* 2 ILL */
#define n2a03_a9 m6502_a9									/* 2 LDA IMM */
#define n2a03_c9 m6502_c9									/* 2 CMP IMM */
OP(e9) { int tmp; m6502_ICount -= 2; RD_IMM; SBC_NES;	  } /* 2 SBC IMM */

#define n2a03_19 m6502_19									/* 4 ORA ABY */
#define n2a03_39 m6502_39									/* 4 AND ABY */
#define n2a03_59 m6502_59									/* 4 EOR ABY */
OP(79) { int tmp; m6502_ICount -= 4; RD_ABY; ADC_NES;	  } /* 4 ADC ABY */
#define n2a03_99 m6502_99									/* 5 STA ABY */
#define n2a03_b9 m6502_b9									/* 4 LDA ABY */
#define n2a03_d9 m6502_d9									/* 4 CMP ABY */
OP(f9) { int tmp; m6502_ICount -= 4; RD_ABY; SBC_NES;	  } /* 4 SBC ABY */

#define n2a03_0a m6502_0a									/* 2 ASL A */
#define n2a03_2a m6502_2a									/* 2 ROL A */
#define n2a03_4a m6502_4a									/* 2 LSR A */
#define n2a03_6a m6502_6a									/* 2 ROR A */
#define n2a03_8a m6502_8a									/* 2 TXA */
#define n2a03_aa m6502_aa									/* 2 TAX */
#define n2a03_ca m6502_ca									/* 2 DEX */
#define n2a03_ea m6502_ea									/* 2 NOP */

#define n2a03_1a m6502_1a									/* 2 ILL */
#define n2a03_3a m6502_3a									/* 2 ILL */
#define n2a03_5a m6502_5a									/* 2 ILL */
#define n2a03_7a m6502_7a									/* 2 ILL */
#define n2a03_9a m6502_9a									/* 2 TXS */
#define n2a03_ba m6502_ba									/* 2 TSX */
#define n2a03_da m6502_da									/* 2 ILL */
#define n2a03_fa m6502_fa									/* 2 ILL */

#define n2a03_0b m6502_0b									/* 2 ILL */
#define n2a03_2b m6502_2b									/* 2 ILL */
#define n2a03_4b m6502_4b									/* 2 ILL */
#define n2a03_6b m6502_6b									/* 2 ILL */
#define n2a03_8b m6502_8b									/* 2 ILL */
#define n2a03_ab m6502_ab									/* 2 ILL */
#define n2a03_cb m6502_cb									/* 2 ILL */
#define n2a03_eb m6502_eb									/* 2 ILL */

#define n2a03_1b m6502_1b									/* 2 ILL */
#define n2a03_3b m6502_3b									/* 2 ILL */
#define n2a03_5b m6502_5b									/* 2 ILL */
#define n2a03_7b m6502_7b									/* 2 ILL */
#define n2a03_9b m6502_9b									/* 2 ILL */
#define n2a03_bb m6502_bb									/* 2 ILL */
#define n2a03_db m6502_db									/* 2 ILL */
#define n2a03_fb m6502_fb									/* 2 ILL */

#define n2a03_0c m6502_0c									/* 2 ILL */
#define n2a03_2c m6502_2c									/* 4 BIT ABS */
#define n2a03_4c m6502_4c									/* 3 JMP ABS */
#define n2a03_6c m6502_6c									/* 5 JMP IND */
#define n2a03_8c m6502_8c									/* 4 STY ABS */
#define n2a03_ac m6502_ac									/* 4 LDY ABS */
#define n2a03_cc m6502_cc									/* 4 CPY ABS */
#define n2a03_ec m6502_ec									/* 4 CPX ABS */

#define n2a03_1c m6502_1c									/* 2 ILL */
#define n2a03_3c m6502_3c									/* 2 ILL */
#define n2a03_5c m6502_5c									/* 2 ILL */
#define n2a03_7c m6502_7c									/* 2 ILL */
#define n2a03_9c m6502_9c									/* 2 ILL */
#define n2a03_bc m6502_bc									/* 4 LDY ABX */
#define n2a03_dc m6502_dc									/* 2 ILL */
#define n2a03_fc m6502_fc									/* 2 ILL */

#define n2a03_0d m6502_0d									/* 4 ORA ABS */
#define n2a03_2d m6502_2d									/* 4 AND ABS */
#define n2a03_4d m6502_4d									/* 4 EOR ABS */
OP(6d) { int tmp; m6502_ICount -= 4; RD_ABS; ADC_NES;	  } /* 4 ADC ABS */
#define n2a03_8d m6502_8d									/* 4 STA ABS */
#define n2a03_ad m6502_ad									/* 4 LDA ABS */
#define n2a03_cd m6502_cd									/* 4 CMP ABS */
OP(ed) { int tmp; m6502_ICount -= 4; RD_ABS; SBC_NES;	  } /* 4 SBC ABS */

#define n2a03_1d m6502_1d									/* 4 ORA ABX */
#define n2a03_3d m6502_3d									/* 4 AND ABX */
#define n2a03_5d m6502_5d									/* 4 EOR ABX */
OP(7d) { int tmp; m6502_ICount -= 4; RD_ABX; ADC_NES;	  } /* 4 ADC ABX */
#define n2a03_9d m6502_9d									/* 5 STA ABX */
#define n2a03_bd m6502_bd									/* 4 LDA ABX */
#define n2a03_dd m6502_dd									/* 4 CMP ABX */
OP(fd) { int tmp; m6502_ICount -= 4; RD_ABX; SBC_NES;	  } /* 4 SBC ABX */

#define n2a03_0e m6502_0e									/* 6 ASL ABS */
#define n2a03_2e m6502_2e									/* 6 ROL ABS */
#define n2a03_4e m6502_4e									/* 6 LSR ABS */
#define n2a03_6e m6502_6e									/* 6 ROR ABS */
#define n2a03_8e m6502_8e									/* 5 STX ABS */
#define n2a03_ae m6502_ae									/* 4 LDX ABS */
#define n2a03_ce m6502_ce									/* 6 DEC ABS */
#define n2a03_ee m6502_ee									/* 6 INC ABS */

#define n2a03_1e m6502_1e									/* 7 ASL ABX */
#define n2a03_3e m6502_3e									/* 7 ROL ABX */
#define n2a03_5e m6502_5e									/* 7 LSR ABX */
#define n2a03_7e m6502_7e									/* 7 ROR ABX */
#define n2a03_9e m6502_9e									/* 2 SXH ABY */
#define n2a03_be m6502_be									/* 4 LDX ABY */
#define n2a03_de m6502_de									/* 7 DEC ABX */
#define n2a03_fe m6502_fe									/* 7 INC ABX */

#define n2a03_0f m6502_0f									/* 2 ILL */
#define n2a03_2f m6502_2f									/* 2 ILL */
#define n2a03_4f m6502_4f									/* 2 ILL */
#define n2a03_6f m6502_6f									/* 2 ILL */
#define n2a03_8f m6502_8f									/* 2 ILL */
#define n2a03_af m6502_af									/* 2 ILL */
#define n2a03_cf m6502_cf									/* 2 ILL */
#define n2a03_ef m6502_ef									/* 2 ILL */

#define n2a03_1f m6502_1f									/* 2 ILL */
#define n2a03_3f m6502_3f									/* 2 ILL */
#define n2a03_5f m6502_5f									/* 2 ILL */
#define n2a03_7f m6502_7f									/* 2 ILL */
#define n2a03_9f m6502_9f									/* 2 ILL */
#define n2a03_bf m6502_bf									/* 2 ILL */
#define n2a03_df m6502_df									/* 2 ILL */
#define n2a03_ff m6502_ff									/* 2 ILL */

static void (*insn2a03[0x100])(void) = {
	n2a03_00,n2a03_01,n2a03_02,n2a03_03,n2a03_04,n2a03_05,n2a03_06,n2a03_07,
	n2a03_08,n2a03_09,n2a03_0a,n2a03_0b,n2a03_0c,n2a03_0d,n2a03_0e,n2a03_0f,
	n2a03_10,n2a03_11,n2a03_12,n2a03_13,n2a03_14,n2a03_15,n2a03_16,n2a03_17,
	n2a03_18,n2a03_19,n2a03_1a,n2a03_1b,n2a03_1c,n2a03_1d,n2a03_1e,n2a03_1f,
	n2a03_20,n2a03_21,n2a03_22,n2a03_23,n2a03_24,n2a03_25,n2a03_26,n2a03_27,
	n2a03_28,n2a03_29,n2a03_2a,n2a03_2b,n2a03_2c,n2a03_2d,n2a03_2e,n2a03_2f,
	n2a03_30,n2a03_31,n2a03_32,n2a03_33,n2a03_34,n2a03_35,n2a03_36,n2a03_37,
	n2a03_38,n2a03_39,n2a03_3a,n2a03_3b,n2a03_3c,n2a03_3d,n2a03_3e,n2a03_3f,
	n2a03_40,n2a03_41,n2a03_42,n2a03_43,n2a03_44,n2a03_45,n2a03_46,n2a03_47,
	n2a03_48,n2a03_49,n2a03_4a,n2a03_4b,n2a03_4c,n2a03_4d,n2a03_4e,n2a03_4f,
	n2a03_50,n2a03_51,n2a03_52,n2a03_53,n2a03_54,n2a03_55,n2a03_56,n2a03_57,
	n2a03_58,n2a03_59,n2a03_5a,n2a03_5b,n2a03_5c,n2a03_5d,n2a03_5e,n2a03_5f,
	n2a03_60,n2a03_61,n2a03_62,n2a03_63,n2a03_64,n2a03_65,n2a03_66,n2a03_67,
	n2a03_68,n2a03_69,n2a03_6a,n2a03_6b,n2a03_6c,n2a03_6d,n2a03_6e,n2a03_6f,
	n2a03_70,n2a03_71,n2a03_72,n2a03_73,n2a03_74,n2a03_75,n2a03_76,n2a03_77,
	n2a03_78,n2a03_79,n2a03_7a,n2a03_7b,n2a03_7c,n2a03_7d,n2a03_7e,n2a03_7f,
	n2a03_80,n2a03_81,n2a03_82,n2a03_83,n2a03_84,n2a03_85,n2a03_86,n2a03_87,
	n2a03_88,n2a03_89,n2a03_8a,n2a03_8b,n2a03_8c,n2a03_8d,n2a03_8e,n2a03_8f,
	n2a03_90,n2a03_91,n2a03_92,n2a03_93,n2a03_94,n2a03_95,n2a03_96,n2a03_97,
	n2a03_98,n2a03_99,n2a03_9a,n2a03_9b,n2a03_9c,n2a03_9d,n2a03_9e,n2a03_9f,
	n2a03_a0,n2a03_a1,n2a03_a2,n2a03_a3,n2a03_a4,n2a03_a5,n2a03_a6,n2a03_a7,
	n2a03_a8,n2a03_a9,n2a03_aa,n2a03_ab,n2a03_ac,n2a03_ad,n2a03_ae,n2a03_af,
	n2a03_b0,n2a03_b1,n2a03_b2,n2a03_b3,n2a03_b4,n2a03_b5,n2a03_b6,n2a03_b7,
	n2a03_b8,n2a03_b9,n2a03_ba,n2a03_bb,n2a03_bc,n2a03_bd,n2a03_be,n2a03_bf,
	n2a03_c0,n2a03_c1,n2a03_c2,n2a03_c3,n2a03_c4,n2a03_c5,n2a03_c6,n2a03_c7,
	n2a03_c8,n2a03_c9,n2a03_ca,n2a03_cb,n2a03_cc,n2a03_cd,n2a03_ce,n2a03_cf,
	n2a03_d0,n2a03_d1,n2a03_d2,n2a03_d3,n2a03_d4,n2a03_d5,n2a03_d6,n2a03_d7,
	n2a03_d8,n2a03_d9,n2a03_da,n2a03_db,n2a03_dc,n2a03_dd,n2a03_de,n2a03_df,
	n2a03_e0,n2a03_e1,n2a03_e2,n2a03_e3,n2a03_e4,n2a03_e5,n2a03_e6,n2a03_e7,
	n2a03_e8,n2a03_e9,n2a03_ea,n2a03_eb,n2a03_ec,n2a03_ed,n2a03_ee,n2a03_ef,
	n2a03_f0,n2a03_f1,n2a03_f2,n2a03_f3,n2a03_f4,n2a03_f5,n2a03_f6,n2a03_f7,
	n2a03_f8,n2a03_f9,n2a03_fa,n2a03_fb,n2a03_fc,n2a03_fd,n2a03_fe,n2a03_ff
};
