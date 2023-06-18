typedef CINESTATE (*opcode_func)(int);


/* a lot of jump table entries evaluate to the same thing */
#define opINP_AA_AA opINP_A_AA
#define opINP_BB_AA opINP_A_AA
#define opOUTbi_AA_A opOUTbi_A_A
#define opOUTbi_BB_A opOUTbi_A_A
#define opOUT16_AA_A opOUT16_A_A
#define opOUT16_BB_A opOUT16_A_A
#define opOUT64_AA_A opOUT64_A_A
#define opOUT64_BB_A opOUT64_A_A
#define opOUTWW_AA_A opOUTWW_A_A
#define opOUTWW_BB_A opOUTWW_A_A
#define opLDAimm_AA_AA opLDAimm_A_AA
#define opLDAimm_BB_AA opLDAimm_A_AA
#define opLDAdir_AA_AA opLDAdir_A_AA
#define opLDAdir_BB_AA opLDAdir_A_AA
#define opLDAirg_AA_AA opLDAirg_A_AA
#define opLDAirg_BB_AA opLDAirg_A_AA
#define opADDimm_AA_AA opADDimm_A_AA
#define opADDimm_BB_AA opADDimm_A_AA
#define opADDimmX_AA_AA opADDimmX_A_AA
#define opADDimmX_BB_AA opADDimmX_A_AA
#define opADDdir_AA_AA opADDdir_A_AA
#define opADDdir_BB_AA opADDdir_A_AA
#define opAWDirg_AA_AA opAWDirg_A_AA
#define opAWDirg_BB_AA opAWDirg_A_AA
#define opADDirg_A_AA opAWDirg_A_AA
#define opADDirg_AA_AA opAWDirg_A_AA
#define opADDirg_BB_AA opAWDirg_A_AA
#define opADDirg_B_AA opAWDirg_B_AA
#define opSUBimm_AA_AA opSUBimm_A_AA
#define opSUBimm_BB_AA opSUBimm_A_AA
#define opSUBimmX_AA_AA opSUBimmX_A_AA
#define opSUBimmX_BB_AA opSUBimmX_A_AA
#define opSUBdir_AA_AA opSUBdir_A_AA
#define opSUBdir_BB_AA opSUBdir_A_AA
#define opSUBirg_AA_AA opSUBirg_A_AA
#define opSUBirg_BB_AA opSUBirg_A_AA
#define opCMPdir_AA_AA opCMPdir_A_AA
#define opCMPdir_BB_AA opCMPdir_A_AA
#define opANDirg_AA_AA opANDirg_A_AA
#define opANDirg_BB_AA opANDirg_A_AA
#define opLDJimm_AA_A opLDJimm_A_A
#define opLDJimm_BB_A opLDJimm_A_A
#define opLDJirg_AA_A opLDJirg_A_A
#define opLDJirg_BB_A opLDJirg_A_A
#define opLDPimm_AA_A opLDPimm_A_A
#define opLDPimm_BB_A opLDPimm_A_A
#define opLDIdir_AA_A opLDIdir_A_A
#define opLDIdir_BB_A opLDIdir_A_A
#define opSTAdir_AA_A opSTAdir_A_A
#define opSTAdir_BB_A opSTAdir_A_A
#define opSTAirg_AA_A opSTAirg_A_A
#define opSTAirg_BB_A opSTAirg_A_A
#define opXLT_AA_AA opXLT_A_AA
#define opXLT_BB_AA opXLT_A_AA
#define opMULirg_AA_AA opMULirg_A_AA
#define opMULirg_BB_AA opMULirg_A_AA
#define opLSRe_AA_AA opLSRe_A_AA
#define opLSRe_BB_AA opLSRe_A_AA
#define opLSRf_AA_AA opLSRf_A_AA
#define opLSRf_BB_AA opLSRf_A_AA
#define opLSLe_AA_AA opLSLe_A_AA
#define opLSLe_BB_AA opLSLe_A_AA
#define opLSLf_AA_AA opLSLf_A_AA
#define opLSLf_BB_AA opLSLf_A_AA
#define opASRe_AA_AA opASRe_A_AA
#define opASRe_BB_AA opASRe_A_AA
#define opASRf_AA_AA opASRf_A_AA
#define opASRf_BB_AA opASRf_A_AA
#define opASRDe_AA_AA opASRDe_A_AA
#define opASRDe_BB_AA opASRDe_A_AA
#define opASRDf_AA_AA opASRDf_A_AA
#define opASRDf_BB_AA opASRDf_A_AA
#define opLSLDe_AA_AA opLSLDe_A_AA
#define opLSLDe_BB_AA opLSLDe_A_AA
#define opLSLDf_AA_AA opLSLDf_A_AA
#define opLSLDf_BB_AA opLSLDf_A_AA
#define opJMP_AA_A opJMP_A_A
#define opJMP_BB_A opJMP_A_A
#define opJEI_AA_A opJEI_A_A
#define opJEI_BB_A opJEI_A_A
#define opJEI_AA_B opJEI_A_B
#define opJEI_BB_B opJEI_A_B
#define opJLT_AA_A opJLT_A_A
#define opJLT_BB_A opJLT_A_A
#define opJEQ_AA_A opJEQ_A_A
#define opJEQ_BB_A opJEQ_A_A
#define opJA0_AA_A opJA0_A_A
#define opJA0_BB_A opJA0_A_A
#define opJNC_AA_A opJNC_A_A
#define opJNC_BB_A opJNC_A_A
#define opJDR_AA_A opJDR_A_A
#define opJDR_BB_A opJDR_A_A
#define opNOP_AA_A opNOP_A_A
#define opNOP_BB_A opNOP_A_A
#define opJPP32_AA_B opJPP32_A_B
#define opJPP32_BB_B opJPP32_A_B
#define opJPP16_AA_B opJPP16_A_B
#define opJPP16_BB_B opJPP16_A_B
#define opJPP8_AA_B opJPP8_A_B
#define opJPP8_BB_B opJPP8_A_B
#define opJLT_AA_B opJLT_A_B
#define opJLT_BB_B opJLT_A_B
#define opJEQ_AA_B opJEQ_A_B
#define opJEQ_BB_B opJEQ_A_B
#define opJA0_AA_B opJA0_A_B
#define opJA0_BB_B opJA0_A_B
#define opJNC_AA_B opJNC_A_B
#define opJNC_BB_B opJNC_A_B
#define opJDR_AA_B opJDR_A_B
#define opJDR_BB_B opJDR_A_B
#define opNOP_AA_B opNOP_A_B
#define opNOP_BB_B opNOP_A_B
#define opLLT_AA_AA opLLT_A_AA
#define opLLT_BB_AA opLLT_A_AA
#define opVIN_AA_A opVIN_A_A
#define opVIN_BB_A opVIN_A_A
#define opWAI_AA_A opWAI_A_A
#define opWAI_BB_A opWAI_A_A
#define opVDR_AA_A opVDR_A_A
#define opVDR_BB_A opVDR_A_A
#define tOUT_AA_A tOUT_A_A
#define tOUT_BB_A tOUT_B_BB
#define tJMI_B_BB2 tJMI_B_BB1
#define tJPP_AA_B tJPP_A_B
#define tJPP_BB_B tJPP_A_B


/* prototypes for the function handlers */
static CINESTATE opADDdir_A_AA (int opcode);
static CINESTATE opADDdir_B_AA (int opcode);
static CINESTATE opADDimmX_A_AA (int opcode);
static CINESTATE opADDimmX_B_AA (int opcode);
static CINESTATE opADDimm_A_AA (int opcode);
static CINESTATE opADDimm_B_AA (int opcode);
static CINESTATE opANDirg_A_AA (int opcode);
static CINESTATE opANDirg_B_AA (int opcode);
static CINESTATE opASRDe_A_AA (int opcode);
static CINESTATE opASRDe_B_AA (int opcode);
static CINESTATE opASRDf_A_AA (int opcode);
static CINESTATE opASRDf_B_AA (int opcode);
static CINESTATE opASRe_A_AA (int opcode);
static CINESTATE opASRe_B_AA (int opcode);
static CINESTATE opASRf_A_AA (int opcode);
static CINESTATE opASRf_B_AA (int opcode);
static CINESTATE opAWDirg_A_AA (int opcode);
static CINESTATE opAWDirg_B_AA (int opcode);
static CINESTATE opCMPdir_A_AA (int opcode);
static CINESTATE opCMPdir_B_AA (int opcode);
static CINESTATE opINP_A_AA (int opcode);
static CINESTATE opINP_B_AA (int opcode);
static CINESTATE opJA0_A_A (int opcode);
static CINESTATE opJA0_A_B (int opcode);
static CINESTATE opJA0_B_BB (int opcode);
static CINESTATE opJDR_A_A (int opcode);
static CINESTATE opJDR_A_B (int opcode);
static CINESTATE opJDR_B_BB (int opcode);
static CINESTATE opJEQ_A_A (int opcode);
static CINESTATE opJEQ_A_B (int opcode);
static CINESTATE opJEQ_B_BB (int opcode);
static CINESTATE opJLT_A_A (int opcode);
static CINESTATE opJLT_A_B (int opcode);
static CINESTATE opJLT_B_BB (int opcode);
static CINESTATE opJMP_A_A (int opcode);
static CINESTATE opJMP_B_BB (int opcode);
static CINESTATE opJNC_A_A (int opcode);
static CINESTATE opJNC_A_B (int opcode);
static CINESTATE opJNC_B_BB (int opcode);
static CINESTATE opLDAdir_A_AA (int opcode);
static CINESTATE opLDAdir_B_AA (int opcode);
static CINESTATE opLDAimm_A_AA (int opcode);
static CINESTATE opLDAimm_B_AA (int opcode);
static CINESTATE opLDAirg_A_AA (int opcode);
static CINESTATE opLDAirg_B_AA (int opcode);
static CINESTATE opLDIdir_A_A (int opcode);
static CINESTATE opLDIdir_B_BB (int opcode);
static CINESTATE opLDJimm_A_A (int opcode);
static CINESTATE opLDJimm_B_BB (int opcode);
static CINESTATE opLDJirg_A_A (int opcode);
static CINESTATE opLDJirg_B_BB (int opcode);
static CINESTATE opLDPimm_A_A (int opcode);
static CINESTATE opLDPimm_B_BB (int opcode);
static CINESTATE opLLT_A_AA (int opcode);
static CINESTATE opLLT_B_AA (int opcode);
static CINESTATE opLSLDe_A_AA (int opcode);
static CINESTATE opLSLDe_B_AA (int opcode);
static CINESTATE opLSLDf_A_AA (int opcode);
static CINESTATE opLSLDf_B_AA (int opcode);
static CINESTATE opLSLe_A_AA (int opcode);
static CINESTATE opLSLe_B_AA (int opcode);
static CINESTATE opLSLf_A_AA (int opcode);
static CINESTATE opLSLf_B_AA (int opcode);
static CINESTATE opLSRe_A_AA (int opcode);
static CINESTATE opLSRe_B_AA (int opcode);
static CINESTATE opLSRf_A_AA (int opcode);
static CINESTATE opLSRf_B_AA (int opcode);
static CINESTATE opMULirg_A_AA (int opcode);
static CINESTATE opMULirg_B_AA (int opcode);
static CINESTATE opNOP_A_A (int opcode);
static CINESTATE opNOP_A_B (int opcode);
static CINESTATE opNOP_B_BB (int opcode);
static CINESTATE opSTAdir_A_A (int opcode);
static CINESTATE opSTAdir_B_BB (int opcode);
static CINESTATE opSTAirg_A_A (int opcode);
static CINESTATE opSTAirg_B_BB (int opcode);
static CINESTATE opSUBdir_A_AA (int opcode);
static CINESTATE opSUBdir_B_AA (int opcode);
static CINESTATE opSUBimmX_A_AA (int opcode);
static CINESTATE opSUBimmX_B_AA (int opcode);
static CINESTATE opSUBimm_A_AA (int opcode);
static CINESTATE opSUBimm_B_AA (int opcode);
static CINESTATE opSUBirg_A_AA (int opcode);
static CINESTATE opSUBirg_B_AA (int opcode);
static CINESTATE opVDR_A_A (int opcode);
static CINESTATE opVDR_B_BB (int opcode);
static CINESTATE opVIN_A_A (int opcode);
static CINESTATE opVIN_B_BB (int opcode);
static CINESTATE opWAI_A_A (int opcode);
static CINESTATE opWAI_B_BB (int opcode);
static CINESTATE opXLT_A_AA (int opcode);
static CINESTATE opXLT_B_AA (int opcode);
static CINESTATE tJMI_AA_A (int opcode);
static CINESTATE tJMI_AA_B (int opcode);
static CINESTATE tJMI_A_A (int opcode);
static CINESTATE tJMI_A_B (int opcode);
static CINESTATE tJMI_BB_A (int opcode);
static CINESTATE tJMI_BB_B (int opcode);
static CINESTATE tJMI_B_BB1 (int opcode);
static CINESTATE tJPP_AA_B (int opcode);
/*static CINESTATE tJPP_A_B (int opcode);*/
/*static CINESTATE tJPP_BB_B (int opcode);*/
static CINESTATE tJPP_B_BB (int opcode);
static CINESTATE tOUT_A_A (int opcode);
static CINESTATE tOUT_B_BB (int opcode);



/* Opcode cycle counts according to Zonn's cineinst.txt */
static UINT8 ccpu_cycles[] =
{
	/*    0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
  /*0*/	  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* LDA */
  /*1*/	  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* INP */
  /*2*/	  3,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* ADD */
  /*3*/	  3,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* SUB */
  /*4*/	  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, /* LDJ */
  /*5*/	  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* Jumps: 2 extra cycles if jump made */
  /*6*/	  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, /* ADD */
  /*7*/	  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, /* SUB */
  /*8*/	  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* LDP */
  /*9*/	  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* OUT */
  /*A*/	  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, /* LDA */
  /*B*/	  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, /* CMP */
  /*C*/	  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, /* LDI */
  /*D*/	  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* STA */
  /*E*/	  1,  2,  7,  2,  0,  0,  2,  2,  2,  2,  2,  1,  1,  1,  1,  1, /* LTT and WAI have special timing */
  /*F*/	  1,  2,  7,  2,  0,  0,  2,  2,  2,  2,  2,  1,  1,  1,  1,  1  /* same as E */
};

/* the main opcode table */
static opcode_func cineops[4][256] =
{
	{
		/* table for state "A" -- Use this table if the last opcode was not
		 * an ACC related opcode, and was not a B flip/flop operation.
		 * Translation:
		 *   Any ACC related routine will use A-reg and go on to opCodeTblAA
		 *   Any B flip/flop instructions will jump to opCodeTblB
		 *   All other instructions remain in opCodeTblA
		 *   JMI will use the current sign of the A-reg
		 */
		opLDAimm_A_AA, opLDAimm_A_AA, opLDAimm_A_AA, opLDAimm_A_AA, opLDAimm_A_AA, opLDAimm_A_AA, opLDAimm_A_AA, opLDAimm_A_AA,
		opLDAimm_A_AA, opLDAimm_A_AA, opLDAimm_A_AA, opLDAimm_A_AA, opLDAimm_A_AA, opLDAimm_A_AA, opLDAimm_A_AA, opLDAimm_A_AA,
		opINP_A_AA,    opINP_A_AA,    opINP_A_AA,    opINP_A_AA,    opINP_A_AA,    opINP_A_AA,    opINP_A_AA,    opINP_A_AA,
		opINP_A_AA,    opINP_A_AA,    opINP_A_AA,    opINP_A_AA,    opINP_A_AA,    opINP_A_AA,    opINP_A_AA,    opINP_A_AA,

		opADDimmX_A_AA,opADDimm_A_AA, opADDimm_A_AA, opADDimm_A_AA, opADDimm_A_AA, opADDimm_A_AA, opADDimm_A_AA, opADDimm_A_AA,
		opADDimm_A_AA, opADDimm_A_AA, opADDimm_A_AA, opADDimm_A_AA, opADDimm_A_AA, opADDimm_A_AA, opADDimm_A_AA, opADDimm_A_AA,
		opSUBimmX_A_AA,opSUBimm_A_AA, opSUBimm_A_AA, opSUBimm_A_AA, opSUBimm_A_AA, opSUBimm_A_AA, opSUBimm_A_AA, opSUBimm_A_AA,
		opSUBimm_A_AA, opSUBimm_A_AA, opSUBimm_A_AA, opSUBimm_A_AA, opSUBimm_A_AA, opSUBimm_A_AA, opSUBimm_A_AA, opSUBimm_A_AA,

		opLDJimm_A_A,  opLDJimm_A_A,  opLDJimm_A_A,  opLDJimm_A_A,  opLDJimm_A_A,  opLDJimm_A_A,  opLDJimm_A_A,  opLDJimm_A_A,
		opLDJimm_A_A,  opLDJimm_A_A,  opLDJimm_A_A,  opLDJimm_A_A,  opLDJimm_A_A,  opLDJimm_A_A,  opLDJimm_A_A,  opLDJimm_A_A,
		tJPP_A_B,      tJMI_A_B,      opJDR_A_B,     opJLT_A_B,     opJEQ_A_B,     opJNC_A_B,     opJA0_A_B,     opNOP_A_B,
		opJMP_A_A,     tJMI_A_A,      opJDR_A_A,     opJLT_A_A,     opJEQ_A_A,     opJNC_A_A,     opJA0_A_A,     opNOP_A_A,

		opADDdir_A_AA, opADDdir_A_AA, opADDdir_A_AA, opADDdir_A_AA, opADDdir_A_AA, opADDdir_A_AA, opADDdir_A_AA, opADDdir_A_AA,
		opADDdir_A_AA, opADDdir_A_AA, opADDdir_A_AA, opADDdir_A_AA, opADDdir_A_AA, opADDdir_A_AA, opADDdir_A_AA, opADDdir_A_AA,
		opSUBdir_A_AA, opSUBdir_A_AA, opSUBdir_A_AA, opSUBdir_A_AA, opSUBdir_A_AA, opSUBdir_A_AA, opSUBdir_A_AA, opSUBdir_A_AA,
		opSUBdir_A_AA, opSUBdir_A_AA, opSUBdir_A_AA, opSUBdir_A_AA, opSUBdir_A_AA, opSUBdir_A_AA, opSUBdir_A_AA, opSUBdir_A_AA,

		opLDPimm_A_A,  opLDPimm_A_A,  opLDPimm_A_A,  opLDPimm_A_A,  opLDPimm_A_A,  opLDPimm_A_A,  opLDPimm_A_A,  opLDPimm_A_A,
		opLDPimm_A_A,  opLDPimm_A_A,  opLDPimm_A_A,  opLDPimm_A_A,  opLDPimm_A_A,  opLDPimm_A_A,  opLDPimm_A_A,  opLDPimm_A_A,
		tOUT_A_A,      tOUT_A_A,      tOUT_A_A,      tOUT_A_A,      tOUT_A_A,      tOUT_A_A,      tOUT_A_A,      tOUT_A_A,
		tOUT_A_A,      tOUT_A_A,      tOUT_A_A,      tOUT_A_A,      tOUT_A_A,      tOUT_A_A,      tOUT_A_A,      tOUT_A_A,

		opLDAdir_A_AA, opLDAdir_A_AA, opLDAdir_A_AA, opLDAdir_A_AA, opLDAdir_A_AA, opLDAdir_A_AA, opLDAdir_A_AA, opLDAdir_A_AA,
		opLDAdir_A_AA, opLDAdir_A_AA, opLDAdir_A_AA, opLDAdir_A_AA, opLDAdir_A_AA, opLDAdir_A_AA, opLDAdir_A_AA, opLDAdir_A_AA,
		opCMPdir_A_AA, opCMPdir_A_AA, opCMPdir_A_AA, opCMPdir_A_AA, opCMPdir_A_AA, opCMPdir_A_AA, opCMPdir_A_AA, opCMPdir_A_AA,
		opCMPdir_A_AA, opCMPdir_A_AA, opCMPdir_A_AA, opCMPdir_A_AA, opCMPdir_A_AA, opCMPdir_A_AA, opCMPdir_A_AA, opCMPdir_A_AA,

		opLDIdir_A_A,  opLDIdir_A_A,  opLDIdir_A_A,  opLDIdir_A_A,  opLDIdir_A_A,  opLDIdir_A_A,  opLDIdir_A_A,  opLDIdir_A_A,
		opLDIdir_A_A,  opLDIdir_A_A,  opLDIdir_A_A,  opLDIdir_A_A,  opLDIdir_A_A,  opLDIdir_A_A,  opLDIdir_A_A,  opLDIdir_A_A,
		opSTAdir_A_A,  opSTAdir_A_A,  opSTAdir_A_A,  opSTAdir_A_A,  opSTAdir_A_A,  opSTAdir_A_A,  opSTAdir_A_A,  opSTAdir_A_A,
		opSTAdir_A_A,  opSTAdir_A_A,  opSTAdir_A_A,  opSTAdir_A_A,  opSTAdir_A_A,  opSTAdir_A_A,  opSTAdir_A_A,  opSTAdir_A_A,

		opVDR_A_A,     opLDJirg_A_A,  opXLT_A_AA,    opMULirg_A_AA, opLLT_A_AA,    opWAI_A_A,     opSTAirg_A_A,  opADDirg_A_AA,
		opSUBirg_A_AA, opANDirg_A_AA, opLDAirg_A_AA, opLSRe_A_AA,   opLSLe_A_AA,   opASRe_A_AA,   opASRDe_A_AA,  opLSLDe_A_AA,
		opVIN_A_A,     opLDJirg_A_A,  opXLT_A_AA,    opMULirg_A_AA, opLLT_A_AA,    opWAI_A_A,     opSTAirg_A_A,  opAWDirg_A_AA,
		opSUBirg_A_AA, opANDirg_A_AA, opLDAirg_A_AA, opLSRf_A_AA,   opLSLf_A_AA,   opASRf_A_AA,   opASRDf_A_AA,  opLSLDf_A_AA
	},

	{
		/* opcode table AA -- Use this table if the last opcode was an ACC
		 * related opcode. Translation:
		 *   Any ACC related routine will use A-reg and remain in OpCodeTblAA
		 *   Any B flip/flop instructions will jump to opCodeTblB
		 *   All other instructions will jump to opCodeTblA
		 *   JMI will use the sign of acc_old
		 */

		opLDAimm_AA_AA, opLDAimm_AA_AA, opLDAimm_AA_AA, opLDAimm_AA_AA, opLDAimm_AA_AA, opLDAimm_AA_AA, opLDAimm_AA_AA, opLDAimm_AA_AA,
		opLDAimm_AA_AA, opLDAimm_AA_AA, opLDAimm_AA_AA, opLDAimm_AA_AA, opLDAimm_AA_AA, opLDAimm_AA_AA, opLDAimm_AA_AA, opLDAimm_AA_AA,
		opINP_AA_AA,    opINP_AA_AA,    opINP_AA_AA,    opINP_AA_AA,    opINP_AA_AA,    opINP_AA_AA,    opINP_AA_AA,    opINP_AA_AA,
		opINP_AA_AA,    opINP_AA_AA,    opINP_AA_AA,    opINP_AA_AA,    opINP_AA_AA,    opINP_AA_AA,    opINP_AA_AA,    opINP_AA_AA,

		opADDimmX_AA_AA,opADDimm_AA_AA, opADDimm_AA_AA, opADDimm_AA_AA, opADDimm_AA_AA, opADDimm_AA_AA, opADDimm_AA_AA, opADDimm_AA_AA,
		opADDimm_AA_AA, opADDimm_AA_AA, opADDimm_AA_AA, opADDimm_AA_AA, opADDimm_AA_AA, opADDimm_AA_AA, opADDimm_AA_AA, opADDimm_AA_AA,
		opSUBimmX_AA_AA,opSUBimm_AA_AA, opSUBimm_AA_AA, opSUBimm_AA_AA, opSUBimm_AA_AA, opSUBimm_AA_AA, opSUBimm_AA_AA, opSUBimm_AA_AA,
		opSUBimm_AA_AA, opSUBimm_AA_AA, opSUBimm_AA_AA, opSUBimm_AA_AA, opSUBimm_AA_AA, opSUBimm_AA_AA, opSUBimm_AA_AA, opSUBimm_AA_AA,

		opLDJimm_AA_A,  opLDJimm_AA_A,  opLDJimm_AA_A,  opLDJimm_AA_A,  opLDJimm_AA_A,  opLDJimm_AA_A,  opLDJimm_AA_A,  opLDJimm_AA_A,
		opLDJimm_AA_A,  opLDJimm_AA_A,  opLDJimm_AA_A,  opLDJimm_AA_A,  opLDJimm_AA_A,  opLDJimm_AA_A,  opLDJimm_AA_A,  opLDJimm_AA_A,
		tJPP_AA_B,      tJMI_AA_B,      opJDR_AA_B,     opJLT_AA_B,     opJEQ_AA_B,     opJNC_AA_B,     opJA0_AA_B,     opNOP_AA_B,
		opJMP_AA_A,     tJMI_AA_A,      opJDR_AA_A,     opJLT_AA_A,     opJEQ_AA_A,     opJNC_AA_A,     opJA0_AA_A,     opNOP_AA_A,

		opADDdir_AA_AA, opADDdir_AA_AA, opADDdir_AA_AA, opADDdir_AA_AA, opADDdir_AA_AA, opADDdir_AA_AA, opADDdir_AA_AA, opADDdir_AA_AA,
		opADDdir_AA_AA, opADDdir_AA_AA, opADDdir_AA_AA, opADDdir_AA_AA, opADDdir_AA_AA, opADDdir_AA_AA, opADDdir_AA_AA, opADDdir_AA_AA,
		opSUBdir_AA_AA, opSUBdir_AA_AA, opSUBdir_AA_AA, opSUBdir_AA_AA, opSUBdir_AA_AA, opSUBdir_AA_AA, opSUBdir_AA_AA, opSUBdir_AA_AA,
		opSUBdir_AA_AA, opSUBdir_AA_AA, opSUBdir_AA_AA, opSUBdir_AA_AA, opSUBdir_AA_AA, opSUBdir_AA_AA, opSUBdir_AA_AA, opSUBdir_AA_AA,

		opLDPimm_AA_A,  opLDPimm_AA_A,  opLDPimm_AA_A,  opLDPimm_AA_A,  opLDPimm_AA_A,  opLDPimm_AA_A,  opLDPimm_AA_A,  opLDPimm_AA_A,
		opLDPimm_AA_A,  opLDPimm_AA_A,  opLDPimm_AA_A,  opLDPimm_AA_A,  opLDPimm_AA_A,  opLDPimm_AA_A,  opLDPimm_AA_A,  opLDPimm_AA_A,
		tOUT_AA_A,      tOUT_AA_A,      tOUT_AA_A,      tOUT_AA_A,      tOUT_AA_A,      tOUT_AA_A,      tOUT_AA_A,      tOUT_AA_A,
		tOUT_AA_A,      tOUT_AA_A,      tOUT_AA_A,      tOUT_AA_A,      tOUT_AA_A,      tOUT_AA_A,      tOUT_AA_A,      tOUT_AA_A,

		opLDAdir_AA_AA, opLDAdir_AA_AA, opLDAdir_AA_AA, opLDAdir_AA_AA, opLDAdir_AA_AA, opLDAdir_AA_AA, opLDAdir_AA_AA, opLDAdir_AA_AA,
		opLDAdir_AA_AA, opLDAdir_AA_AA, opLDAdir_AA_AA, opLDAdir_AA_AA, opLDAdir_AA_AA, opLDAdir_AA_AA, opLDAdir_AA_AA, opLDAdir_AA_AA,
		opCMPdir_AA_AA, opCMPdir_AA_AA, opCMPdir_AA_AA, opCMPdir_AA_AA, opCMPdir_AA_AA, opCMPdir_AA_AA, opCMPdir_AA_AA, opCMPdir_AA_AA,
		opCMPdir_AA_AA, opCMPdir_AA_AA, opCMPdir_AA_AA, opCMPdir_AA_AA, opCMPdir_AA_AA, opCMPdir_AA_AA, opCMPdir_AA_AA, opCMPdir_AA_AA,

		opLDIdir_AA_A,  opLDIdir_AA_A,  opLDIdir_AA_A,  opLDIdir_AA_A,  opLDIdir_AA_A,  opLDIdir_AA_A,  opLDIdir_AA_A,  opLDIdir_AA_A,
		opLDIdir_AA_A,  opLDIdir_AA_A,  opLDIdir_AA_A,  opLDIdir_AA_A,  opLDIdir_AA_A,  opLDIdir_AA_A,  opLDIdir_AA_A,  opLDIdir_AA_A,
		opSTAdir_AA_A,  opSTAdir_AA_A,  opSTAdir_AA_A,  opSTAdir_AA_A,  opSTAdir_AA_A,  opSTAdir_AA_A,  opSTAdir_AA_A,  opSTAdir_AA_A,
		opSTAdir_AA_A,  opSTAdir_AA_A,  opSTAdir_AA_A,  opSTAdir_AA_A,  opSTAdir_AA_A,  opSTAdir_AA_A,  opSTAdir_AA_A,  opSTAdir_AA_A,

		opVDR_AA_A,     opLDJirg_AA_A,  opXLT_AA_AA,    opMULirg_AA_AA, opLLT_AA_AA,    opWAI_AA_A,     opSTAirg_AA_A,  opADDirg_AA_AA,
		opSUBirg_AA_AA, opANDirg_AA_AA, opLDAirg_AA_AA, opLSRe_AA_AA,   opLSLe_AA_AA,   opASRe_AA_AA,   opASRDe_AA_AA,  opLSLDe_AA_AA,
		opVIN_AA_A,     opLDJirg_AA_A,  opXLT_AA_AA,    opMULirg_AA_AA, opLLT_AA_AA,    opWAI_AA_A,     opSTAirg_AA_A,  opAWDirg_AA_AA,
		opSUBirg_AA_AA, opANDirg_AA_AA, opLDAirg_AA_AA, opLSRf_AA_AA,   opLSLf_AA_AA,   opASRf_AA_AA,   opASRDf_AA_AA,  opLSLDf_AA_AA
	},

	{
		/* opcode table B -- use this table if the last opcode was a B-reg flip/flop
		 * Translation:
		 *   Any ACC related routine uses B-reg, and goes to opCodeTblAA
		 *   All other instructions will jump to table opCodeTblBB (including
		 *     B flip/flop related instructions)
		 *   JMI will use current sign of the A-reg
		 */
		opLDAimm_B_AA, opLDAimm_B_AA, opLDAimm_B_AA, opLDAimm_B_AA, opLDAimm_B_AA, opLDAimm_B_AA, opLDAimm_B_AA, opLDAimm_B_AA,
		opLDAimm_B_AA, opLDAimm_B_AA, opLDAimm_B_AA, opLDAimm_B_AA, opLDAimm_B_AA, opLDAimm_B_AA, opLDAimm_B_AA, opLDAimm_B_AA,
		opINP_B_AA,    opINP_B_AA,    opINP_B_AA,    opINP_B_AA,    opINP_B_AA,    opINP_B_AA,    opINP_B_AA,    opINP_B_AA,
		opINP_B_AA,    opINP_B_AA,    opINP_B_AA,    opINP_B_AA,    opINP_B_AA,    opINP_B_AA,    opINP_B_AA,    opINP_B_AA,

		opADDimmX_B_AA,opADDimm_B_AA, opADDimm_B_AA, opADDimm_B_AA, opADDimm_B_AA, opADDimm_B_AA, opADDimm_B_AA, opADDimm_B_AA,
		opADDimm_B_AA, opADDimm_B_AA, opADDimm_B_AA, opADDimm_B_AA, opADDimm_B_AA, opADDimm_B_AA, opADDimm_B_AA, opADDimm_B_AA,
		opSUBimmX_B_AA,opSUBimm_B_AA, opSUBimm_B_AA, opSUBimm_B_AA, opSUBimm_B_AA, opSUBimm_B_AA, opSUBimm_B_AA, opSUBimm_B_AA,
		opSUBimm_B_AA, opSUBimm_B_AA, opSUBimm_B_AA, opSUBimm_B_AA, opSUBimm_B_AA, opSUBimm_B_AA, opSUBimm_B_AA, opSUBimm_B_AA,

		opLDJimm_B_BB, opLDJimm_B_BB, opLDJimm_B_BB, opLDJimm_B_BB, opLDJimm_B_BB, opLDJimm_B_BB, opLDJimm_B_BB, opLDJimm_B_BB,
		opLDJimm_B_BB, opLDJimm_B_BB, opLDJimm_B_BB, opLDJimm_B_BB, opLDJimm_B_BB, opLDJimm_B_BB, opLDJimm_B_BB, opLDJimm_B_BB,
		tJPP_B_BB,     tJMI_B_BB1,    opJDR_B_BB,    opJLT_B_BB,    opJEQ_B_BB,    opJNC_B_BB,    opJA0_B_BB,    opNOP_B_BB,
		opJMP_B_BB,    tJMI_B_BB2,    opJDR_B_BB,    opJLT_B_BB,    opJEQ_B_BB,    opJNC_B_BB,    opJA0_B_BB,    opNOP_B_BB,

		opADDdir_B_AA, opADDdir_B_AA, opADDdir_B_AA, opADDdir_B_AA, opADDdir_B_AA, opADDdir_B_AA, opADDdir_B_AA, opADDdir_B_AA,
		opADDdir_B_AA, opADDdir_B_AA, opADDdir_B_AA, opADDdir_B_AA, opADDdir_B_AA, opADDdir_B_AA, opADDdir_B_AA, opADDdir_B_AA,
		opSUBdir_B_AA, opSUBdir_B_AA, opSUBdir_B_AA, opSUBdir_B_AA, opSUBdir_B_AA, opSUBdir_B_AA, opSUBdir_B_AA, opSUBdir_B_AA,
		opSUBdir_B_AA, opSUBdir_B_AA, opSUBdir_B_AA, opSUBdir_B_AA, opSUBdir_B_AA, opSUBdir_B_AA, opSUBdir_B_AA, opSUBdir_B_AA,

		opLDPimm_B_BB, opLDPimm_B_BB, opLDPimm_B_BB, opLDPimm_B_BB, opLDPimm_B_BB, opLDPimm_B_BB, opLDPimm_B_BB, opLDPimm_B_BB,
		opLDPimm_B_BB, opLDPimm_B_BB, opLDPimm_B_BB, opLDPimm_B_BB, opLDPimm_B_BB, opLDPimm_B_BB, opLDPimm_B_BB, opLDPimm_B_BB,
		tOUT_B_BB,     tOUT_B_BB,     tOUT_B_BB,     tOUT_B_BB,     tOUT_B_BB,     tOUT_B_BB,     tOUT_B_BB,     tOUT_B_BB,
		tOUT_B_BB,     tOUT_B_BB,     tOUT_B_BB,     tOUT_B_BB,     tOUT_B_BB,     tOUT_B_BB,     tOUT_B_BB,     tOUT_B_BB,

		opLDAdir_B_AA, opLDAdir_B_AA, opLDAdir_B_AA, opLDAdir_B_AA, opLDAdir_B_AA, opLDAdir_B_AA, opLDAdir_B_AA, opLDAdir_B_AA,
		opLDAdir_B_AA, opLDAdir_B_AA, opLDAdir_B_AA, opLDAdir_B_AA, opLDAdir_B_AA, opLDAdir_B_AA, opLDAdir_B_AA, opLDAdir_B_AA,
		opCMPdir_B_AA, opCMPdir_B_AA, opCMPdir_B_AA, opCMPdir_B_AA, opCMPdir_B_AA, opCMPdir_B_AA, opCMPdir_B_AA, opCMPdir_B_AA,
		opCMPdir_B_AA, opCMPdir_B_AA, opCMPdir_B_AA, opCMPdir_B_AA, opCMPdir_B_AA, opCMPdir_B_AA, opCMPdir_B_AA, opCMPdir_B_AA,

		opLDIdir_B_BB, opLDIdir_B_BB, opLDIdir_B_BB, opLDIdir_B_BB, opLDIdir_B_BB, opLDIdir_B_BB, opLDIdir_B_BB, opLDIdir_B_BB,
		opLDIdir_B_BB, opLDIdir_B_BB, opLDIdir_B_BB, opLDIdir_B_BB, opLDIdir_B_BB, opLDIdir_B_BB, opLDIdir_B_BB, opLDIdir_B_BB,
		opSTAdir_B_BB, opSTAdir_B_BB, opSTAdir_B_BB, opSTAdir_B_BB, opSTAdir_B_BB, opSTAdir_B_BB, opSTAdir_B_BB, opSTAdir_B_BB,
		opSTAdir_B_BB, opSTAdir_B_BB, opSTAdir_B_BB, opSTAdir_B_BB, opSTAdir_B_BB, opSTAdir_B_BB, opSTAdir_B_BB, opSTAdir_B_BB,

		opVDR_B_BB,    opLDJirg_B_BB, opXLT_B_AA,    opMULirg_B_AA, opLLT_B_AA,    opWAI_B_BB,    opSTAirg_B_BB, opADDirg_B_AA,
		opSUBirg_B_AA, opANDirg_B_AA, opLDAirg_B_AA, opLSRe_B_AA,   opLSLe_B_AA,   opASRe_B_AA,   opASRDe_B_AA,  opLSLDe_B_AA,
		opVIN_B_BB,    opLDJirg_B_BB, opXLT_B_AA,    opMULirg_B_AA, opLLT_B_AA,    opWAI_B_BB,    opSTAirg_B_BB, opAWDirg_B_AA,
		opSUBirg_B_AA, opANDirg_B_AA, opLDAirg_B_AA, opLSRf_B_AA,   opLSLf_B_AA,   opASRf_B_AA,   opASRDf_B_AA,  opLSLDf_B_AA,
	},

	{
		/* opcode table BB -- use this table if the last opcode was not an ACC
		 * related opcode, but instruction before that was a B-flip/flop instruction.
		 * Translation:
		 *   Any ACC related routine will use A-reg and go to opCodeTblAA
		 *   Any B flip/flop instructions will jump to opCodeTblB
		 *   All other instructions will jump to table opCodeTblA
		 *   JMI will use the current state of the B-reg
		 */
		opLDAimm_BB_AA, opLDAimm_BB_AA, opLDAimm_BB_AA, opLDAimm_BB_AA, opLDAimm_BB_AA, opLDAimm_BB_AA, opLDAimm_BB_AA, opLDAimm_BB_AA,
		opLDAimm_BB_AA, opLDAimm_BB_AA, opLDAimm_BB_AA, opLDAimm_BB_AA, opLDAimm_BB_AA, opLDAimm_BB_AA, opLDAimm_BB_AA, opLDAimm_BB_AA,
		opINP_BB_AA,    opINP_BB_AA,    opINP_BB_AA,    opINP_BB_AA,    opINP_BB_AA,    opINP_BB_AA,    opINP_BB_AA,    opINP_BB_AA,
		opINP_BB_AA,    opINP_BB_AA,    opINP_BB_AA,    opINP_BB_AA,    opINP_BB_AA,    opINP_BB_AA,    opINP_BB_AA,    opINP_BB_AA,

		opADDimmX_BB_AA,opADDimm_BB_AA, opADDimm_BB_AA, opADDimm_BB_AA, opADDimm_BB_AA, opADDimm_BB_AA, opADDimm_BB_AA, opADDimm_BB_AA,
		opADDimm_BB_AA, opADDimm_BB_AA, opADDimm_BB_AA, opADDimm_BB_AA, opADDimm_BB_AA, opADDimm_BB_AA, opADDimm_BB_AA, opADDimm_BB_AA,
		opSUBimmX_BB_AA,opSUBimm_BB_AA, opSUBimm_BB_AA, opSUBimm_BB_AA, opSUBimm_BB_AA, opSUBimm_BB_AA, opSUBimm_BB_AA, opSUBimm_BB_AA,
		opSUBimm_BB_AA, opSUBimm_BB_AA, opSUBimm_BB_AA, opSUBimm_BB_AA, opSUBimm_BB_AA, opSUBimm_BB_AA, opSUBimm_BB_AA, opSUBimm_BB_AA,

		opLDJimm_BB_A,  opLDJimm_BB_A,  opLDJimm_BB_A,  opLDJimm_BB_A,  opLDJimm_BB_A,  opLDJimm_BB_A,  opLDJimm_BB_A,  opLDJimm_BB_A,
		opLDJimm_BB_A,  opLDJimm_BB_A,  opLDJimm_BB_A,  opLDJimm_BB_A,  opLDJimm_BB_A,  opLDJimm_BB_A,  opLDJimm_BB_A,  opLDJimm_BB_A,
		tJPP_BB_B,      tJMI_BB_B,      opJDR_BB_B,     opJLT_BB_B,     opJEQ_BB_B,     opJNC_BB_B,     opJA0_BB_B,     opNOP_BB_B,
		opJMP_BB_A,     tJMI_BB_A,      opJDR_BB_A,     opJLT_BB_A,     opJEQ_BB_A,     opJNC_BB_A,     opJA0_BB_A,     opNOP_BB_A,

		opADDdir_BB_AA, opADDdir_BB_AA, opADDdir_BB_AA, opADDdir_BB_AA, opADDdir_BB_AA, opADDdir_BB_AA, opADDdir_BB_AA, opADDdir_BB_AA,
		opADDdir_BB_AA, opADDdir_BB_AA, opADDdir_BB_AA, opADDdir_BB_AA, opADDdir_BB_AA, opADDdir_BB_AA, opADDdir_BB_AA, opADDdir_BB_AA,
		opSUBdir_BB_AA, opSUBdir_BB_AA, opSUBdir_BB_AA, opSUBdir_BB_AA, opSUBdir_BB_AA, opSUBdir_BB_AA, opSUBdir_BB_AA, opSUBdir_BB_AA,
		opSUBdir_BB_AA, opSUBdir_BB_AA, opSUBdir_BB_AA, opSUBdir_BB_AA, opSUBdir_BB_AA, opSUBdir_BB_AA, opSUBdir_BB_AA, opSUBdir_BB_AA,

		opLDPimm_BB_A,  opLDPimm_BB_A,  opLDPimm_BB_A,  opLDPimm_BB_A,  opLDPimm_BB_A,  opLDPimm_BB_A,  opLDPimm_BB_A,  opLDPimm_BB_A,
		opLDPimm_BB_A,  opLDPimm_BB_A,  opLDPimm_BB_A,  opLDPimm_BB_A,  opLDPimm_BB_A,  opLDPimm_BB_A,  opLDPimm_BB_A,  opLDPimm_BB_A,
		tOUT_BB_A,      tOUT_BB_A,      tOUT_BB_A,      tOUT_BB_A,      tOUT_BB_A,      tOUT_BB_A,      tOUT_BB_A,      tOUT_BB_A,
		tOUT_BB_A,      tOUT_BB_A,      tOUT_BB_A,      tOUT_BB_A,      tOUT_BB_A,      tOUT_BB_A,      tOUT_BB_A,      tOUT_BB_A,

		opLDAdir_BB_AA, opLDAdir_BB_AA, opLDAdir_BB_AA, opLDAdir_BB_AA, opLDAdir_BB_AA, opLDAdir_BB_AA, opLDAdir_BB_AA, opLDAdir_BB_AA,
		opLDAdir_BB_AA, opLDAdir_BB_AA, opLDAdir_BB_AA, opLDAdir_BB_AA, opLDAdir_BB_AA, opLDAdir_BB_AA, opLDAdir_BB_AA, opLDAdir_BB_AA,
		opCMPdir_BB_AA, opCMPdir_BB_AA, opCMPdir_BB_AA, opCMPdir_BB_AA, opCMPdir_BB_AA, opCMPdir_BB_AA, opCMPdir_BB_AA, opCMPdir_BB_AA,
		opCMPdir_BB_AA, opCMPdir_BB_AA, opCMPdir_BB_AA, opCMPdir_BB_AA, opCMPdir_BB_AA, opCMPdir_BB_AA, opCMPdir_BB_AA, opCMPdir_BB_AA,

		opLDIdir_BB_A,  opLDIdir_BB_A,  opLDIdir_BB_A,  opLDIdir_BB_A,  opLDIdir_BB_A,  opLDIdir_BB_A,  opLDIdir_BB_A,  opLDIdir_BB_A,
		opLDIdir_BB_A,  opLDIdir_BB_A,  opLDIdir_BB_A,  opLDIdir_BB_A,  opLDIdir_BB_A,  opLDIdir_BB_A,  opLDIdir_BB_A,  opLDIdir_BB_A,
		opSTAdir_BB_A,  opSTAdir_BB_A,  opSTAdir_BB_A,  opSTAdir_BB_A,  opSTAdir_BB_A,  opSTAdir_BB_A,  opSTAdir_BB_A,  opSTAdir_BB_A,
		opSTAdir_BB_A,  opSTAdir_BB_A,  opSTAdir_BB_A,  opSTAdir_BB_A,  opSTAdir_BB_A,  opSTAdir_BB_A,  opSTAdir_BB_A,  opSTAdir_BB_A,

		opVDR_BB_A,     opLDJirg_BB_A,  opXLT_BB_AA,    opMULirg_BB_AA, opLLT_BB_AA,    opWAI_BB_A,     opSTAirg_BB_A,  opADDirg_BB_AA,
		opSUBirg_BB_AA, opANDirg_BB_AA, opLDAirg_BB_AA, opLSRe_BB_AA,   opLSLe_BB_AA,   opASRe_BB_AA,   opASRDe_BB_AA,  opLSLDe_BB_AA,
		opVIN_BB_A,     opLDJirg_BB_A,  opXLT_BB_AA,    opMULirg_BB_AA, opLLT_BB_AA,    opWAI_BB_A,     opSTAirg_BB_A,  opAWDirg_BB_AA,
		opSUBirg_BB_AA, opANDirg_BB_AA, opLDAirg_BB_AA, opLSRf_BB_AA,   opLSLf_BB_AA,   opASRf_BB_AA,   opASRDf_BB_AA,  opLSLDf_BB_AA,
	}
};
