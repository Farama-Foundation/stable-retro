#ifndef DAC_H
#define DAC_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_DAC 8

struct DACinterface
{
	int num;	/* total number of DACs */
	int mixing_level[MAX_DAC];
};

int DAC_sh_start(const struct MachineSound *msound);
void DAC_data_w(int num,int data);
void DAC_signed_data_w(int num,int data);
void DAC_data_16_w(int num,int data);
void DAC_signed_data_16_w(int num,int data);

WRITE_HANDLER( DAC_0_data_w );
WRITE_HANDLER( DAC_1_data_w );
WRITE_HANDLER( DAC_0_signed_data_w );
WRITE_HANDLER( DAC_1_signed_data_w );

#ifdef __cplusplus
}
#endif

#endif
