/****************************************************************************/
/*            real mode i286 emulator by Fabrice Frances                    */
/*           (initial work based on David Hedley's pcemu)                   */
/*                                                                          */
/****************************************************************************/

#include "i86.h"

void i286_set_address_mask(offs_t mask);

#undef GetMemB
#undef GetMemW
#undef PutMemB
#undef PutMemW

/* ASG 971005 -- changed to cpu_readmem20/cpu_writemem20 */
#define GetMemB(Seg,Off) ( (BYTE)cpu_readmem24((DefaultBase(Seg)+(Off))&AMASK))
#define GetMemW(Seg,Off) ( (WORD)GetMemB(Seg,Off)+(WORD)(GetMemB(Seg,(Off)+1)<<8))
#define PutMemB(Seg,Off,x) { cpu_writemem24((DefaultBase(Seg)+(Off))&AMASK,(x)); }
#define PutMemW(Seg,Off,x) { PutMemB(Seg,Off,(BYTE)(x)); PutMemB(Seg,(Off)+1,(BYTE)((x)>>8)); }

#undef PEEKBYTE
#define PEEKBYTE(ea) ((BYTE)cpu_readmem24((ea)&AMASK))

#undef ReadByte
#undef ReadWord
#undef WriteByte
#undef WriteWord

#define ReadByte(ea) ((BYTE)cpu_readmem24((ea)&AMASK))
#define ReadWord(ea) (cpu_readmem24((ea)&AMASK)+(cpu_readmem24(((ea)+1)&AMASK)<<8))
#define WriteByte(ea,val) { cpu_writemem24((ea)&AMASK,val); }
#define WriteWord(ea,val) { cpu_writemem24((ea)&AMASK,(BYTE)(val)); cpu_writemem24(((ea)+1)&AMASK,(val)>>8); }

#undef CHANGE_PC
#define CHANGE_PC(addr) change_pc24(addr)
