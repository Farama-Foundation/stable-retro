#include <stdio.h>
# include <stdlib.h>
# include <string.h>
#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <encodings/crc32.h>

#include "gles2N64.h"
#include "GBI.h"
#include "RDP.h"
#include "RSP.h"
#include "F3D.h"
#include "F3DEX.h"
#include "F3DEX2.h"
#include "L3D.h"
#include "L3DEX.h"
#include "L3DEX2.h"
#include "S2DEX.h"
#include "S2DEX2.h"
#include "F3DDKR.h"
#include "F3DSWSE.h"
#include "F3DWRUS.h"
#include "F3DPD.h"
#include "F3DEX2CBFD.h"
#include "ZSort.h"
# include "convert.h"
#include "Common.h"

#include "CRC.h"
#include "Debug.h"

#define uc_crc gln64uc_crc

uint32_t uc_crc, uc_dcrc;
char uc_str[256];

SpecialMicrocodeInfo specialMicrocodes[] =
{
	{ F3D,		false,	0xe62a706d, "Fast3D" },
	{ F3D,		false,	0x7d372819, "Fast3D" },
	{ F3D,		false,	0x2edee7be, "Fast3D" },
	{ F3D,		false,	0xe01e14be, "Fast3D" },
   { F3D,		false,	0x4AED6B3B, "Fast3D" }, /* Vivid Dolls [ALECK64] */

	{ F3DWRUS,	false,	0xd17906e2, "RSP SW Version: 2.0D, 04-01-96" },
	{ F3DSWSE,	false,	0x94c4c833, "RSP SW Version: 2.0D, 04-01-96" },
	{ F3DEX,	true,	0x637b4b58, "RSP SW Version: 2.0D, 04-01-96" },
	{ F3D,		true,	0x54c558ba, "RSP SW Version: 2.0D, 04-01-96" }, // Pilot Wings
   { F3D,		true,	0x302bca09, "RSP SW Version: 2.0G, 09-30-96" }, // GoldenEye

	{ S2DEX,	false,	0x9df31081, "RSP Gfx ucode S2DEX  1.06 Yoshitaka Yasumoto Nintendo." },

	{ F3DDKR,	false,	0x8d91244f, "Diddy Kong Racing" },
	{ F3DDKR,	false,	0x6e6fc893, "Diddy Kong Racing" },
	{ F3DJFG,	false,	0xbde9d1fb, "Jet Force Gemini" },
	{ F3DPD,	true,	0x1c4f7869, "Perfect Dark" },
	{ Turbo3D,	false,	0x2bdcfc8a, "Turbo3D" },
	{ F3DEX2CBFD, true, 0x1b4ace88, "Conker's Bad Fur Day" }
};

uint32_t G_RDPHALF_1, G_RDPHALF_2, G_RDPHALF_CONT;
uint32_t G_SPNOOP;
uint32_t G_SETOTHERMODE_H, G_SETOTHERMODE_L;
uint32_t G_DL, G_ENDDL, G_CULLDL, G_BRANCH_Z;
uint32_t G_LOAD_UCODE;
uint32_t G_MOVEMEM, G_MOVEWORD;
uint32_t G_MTX, G_POPMTX;
uint32_t G_GEOMETRYMODE, G_SETGEOMETRYMODE, G_CLEARGEOMETRYMODE;
uint32_t G_TEXTURE;
uint32_t G_DMA_IO, G_DMA_DL, G_DMA_TRI, G_DMA_MTX, G_DMA_VTX, G_DMA_TEX_OFFSET, G_DMA_OFFSETS;
uint32_t G_SPECIAL_1, G_SPECIAL_2, G_SPECIAL_3;
uint32_t G_VTX, G_MODIFYVTX, G_VTXCOLORBASE;
uint32_t G_TRI1, G_TRI2, G_TRI4;
uint32_t G_QUAD, G_LINE3D;
uint32_t G_RESERVED0, G_RESERVED1, G_RESERVED2, G_RESERVED3;
uint32_t G_SPRITE2D_BASE;
uint32_t G_BG_1CYC, G_BG_COPY;
uint32_t G_OBJ_RECTANGLE, G_OBJ_SPRITE, G_OBJ_MOVEMEM;
uint32_t G_SELECT_DL, G_OBJ_RENDERMODE, G_OBJ_RECTANGLE_R;
uint32_t G_OBJ_LOADTXTR, G_OBJ_LDTX_SPRITE, G_OBJ_LDTX_RECT, G_OBJ_LDTX_RECT_R;
uint32_t G_RDPHALF_0;

/* TODO/FIXME - remove? */
uint32_t G_TRI_UNKNOWN;

uint32_t G_MTX_STACKSIZE;
uint32_t G_MTX_MODELVIEW;
uint32_t G_MTX_PROJECTION;
uint32_t G_MTX_MUL;
uint32_t G_MTX_LOAD;
uint32_t G_MTX_NOPUSH;
uint32_t G_MTX_PUSH;

uint32_t G_TEXTURE_ENABLE;
uint32_t G_SHADING_SMOOTH;
uint32_t G_CULL_FRONT;
uint32_t G_CULL_BACK;
uint32_t G_CULL_BOTH;
uint32_t G_CLIPPING;

uint32_t G_MV_VIEWPORT;

uint32_t G_MWO_aLIGHT_1, G_MWO_bLIGHT_1;
uint32_t G_MWO_aLIGHT_2, G_MWO_bLIGHT_2;
uint32_t G_MWO_aLIGHT_3, G_MWO_bLIGHT_3;
uint32_t G_MWO_aLIGHT_4, G_MWO_bLIGHT_4;
uint32_t G_MWO_aLIGHT_5, G_MWO_bLIGHT_5;
uint32_t G_MWO_aLIGHT_6, G_MWO_bLIGHT_6;
uint32_t G_MWO_aLIGHT_7, G_MWO_bLIGHT_7;
uint32_t G_MWO_aLIGHT_8, G_MWO_bLIGHT_8;

GBIInfo GBI;

static uint32_t current_type;

uint32_t GBI_GetCurrentMicrocodeType(void)
{
   return current_type;
}

void GBI_Unknown( uint32_t w0, uint32_t w1 )
{
#ifdef DEBUG
	if (Debug.level == DEBUG_LOW)
		DebugMsg( DEBUG_LOW | DEBUG_UNKNOWN, "UNKNOWN GBI COMMAND 0x%02X", _SHIFTR( w0, 24, 8 ) );
	if (Debug.level == DEBUG_MEDIUM)
		DebugMsg( DEBUG_MEDIUM | DEBUG_UNKNOWN, "Unknown GBI Command 0x%02X", _SHIFTR( w0, 24, 8 ) );
	else if (Debug.level == DEBUG_HIGH)
		DebugMsg( DEBUG_HIGH | DEBUG_UNKNOWN, "// Unknown GBI Command 0x%02X", _SHIFTR( w0, 24, 8 ) );
#endif
}

static int MicrocodeDialog(void)
{
    return 0;
}

MicrocodeInfo *GBI_AddMicrocode(void)
{
    MicrocodeInfo *newtop = (MicrocodeInfo*)malloc( sizeof( MicrocodeInfo ) );

    newtop->lower = GBI.top;
    newtop->higher = NULL;

    if (GBI.top)
        GBI.top->higher = newtop;

    if (!GBI.bottom)
        GBI.bottom = newtop;

    GBI.top = newtop;

    GBI.numMicrocodes++;


    return newtop;
}

void GBI_Init(void)
{
   uint32_t i;
   GBI.top = NULL;
   GBI.bottom = NULL;
   GBI.current = NULL;
   GBI.numMicrocodes = 0;

   for (i = 0; i <= 0xFF; i++)
      GBI.cmd[i] = GBI_Unknown;
}

void GBI_Destroy(void)
{
   while (GBI.bottom)
   {
      MicrocodeInfo *newBottom = GBI.bottom->higher;

      if (GBI.bottom == GBI.top)
         GBI.top = NULL;

      free( GBI.bottom );

      GBI.bottom = newBottom;

      if (GBI.bottom)
         GBI.bottom->lower = NULL;

      GBI.numMicrocodes--;
   }
}

static INLINE bool _isDigit(char _c)
{
	return _c >= '0' && _c <= '9';
}

MicrocodeInfo *GBI_DetectMicrocode( uint32_t uc_start, uint32_t uc_dstart, uint16_t uc_dsize )
{
   unsigned i;
   char uc_data[2048];
   MicrocodeInfo *current;

   for (i = 0; i < GBI.numMicrocodes; i++)
   {
      current = GBI.top;

      while (current)
      {
         if ((current->address == uc_start) && (current->dataAddress == uc_dstart) && (current->dataSize == uc_dsize))
            return current;

         current = current->lower;
      }
   }

   current = GBI_AddMicrocode();

   current->address = uc_start;
   current->dataAddress = uc_dstart;
   current->dataSize = uc_dsize;
   current->NoN = false;
   current->type = NONE;

   // See if we can identify it by CRC
   uc_crc = encoding_crc32(0xffffffff, &gfx_info.RDRAM[uc_start & 0x1FFFFFFF], 4096);
   LOG(LOG_MINIMAL, "UCODE CRC=0x%x\n", uc_crc);

   for (i = 0; i < sizeof( specialMicrocodes ) / sizeof( SpecialMicrocodeInfo ); i++)
   {
      if (uc_crc == specialMicrocodes[i].crc)
      {
         current->type = specialMicrocodes[i].type;
         current_type  = current->type;
         return current;
      }
   }

   // See if we can identify it by text
   UnswapCopyWrap(gfx_info.RDRAM, uc_dstart & 0x1FFFFFFF, (uint8_t*)uc_data, 0, 0x7FF, 2048);
   strcpy( uc_str, "Not Found" );

   for (i = 0; i < 2048; i++)
   {
      if ((uc_data[i] == 'R') && (uc_data[i+1] == 'S') && (uc_data[i+2] == 'P'))
      {
         uint32_t j = 0;
         int type = NONE;

         while (uc_data[i+j] > 0x0A)
         {
            uc_str[j] = uc_data[i+j];
            j++;
         }

         uc_str[j] = 0x00;

         if (strncmp( &uc_str[4], "SW", 2 ) == 0)
            type = F3D;
         else if (strncmp( &uc_str[4], "Gfx", 3 ) == 0)
         {
            current->NoN = (strncmp( &uc_str[20], ".NoN", 4 ) == 0);

            if (strncmp( &uc_str[14], "F3D", 3 ) == 0)
            {
					if (uc_str[28] == '1' || strncmp(&uc_str[28], "0.95", 4) == 0 || strncmp(&uc_str[28], "0.96", 4) == 0)
						type = F3DEX;
					else if (uc_str[31] == '2')
						type = F3DEX2;
            }
            else if (strncmp( &uc_str[14], "L3D", 3 ) == 0)
            {
					uint32_t t = 22;
					while (!_isDigit(uc_str[t]) && t++ < j);
					if (uc_str[t] == '1')
						type = L3DEX;
					else if (uc_str[t] == '2')
						type = L3DEX2;
            }
            else if (strncmp( &uc_str[14], "S2D", 3 ) == 0)
            {
					uint32_t t = 20;
					while (!_isDigit(uc_str[t]) && t++ < j);
					if (uc_str[t] == '1')
						type = S2DEX;
					else if (uc_str[t] == '2')
						type = S2DEX2;
            }
				else if (strncmp(&uc_str[14], "ZSortp", 6) == 0) 
					type = ZSortp;
         }

         LOG(LOG_VERBOSE, "UCODE STRING=%s\n", uc_str);

         if (type != NONE)
         {
            current->type = type;
            current_type = type;
            return current;
         }

         break;
      }
   }


   for (i = 0; i < sizeof( specialMicrocodes ) / sizeof( SpecialMicrocodeInfo ); i++)
   {
      if (strcmp( uc_str, specialMicrocodes[i].text ) == 0)
      {
         current->type = specialMicrocodes[i].type;
         current_type = current->type;
         return current;
      }
   }

   // Let the user choose the microcode
   LOG(LOG_ERROR, "[gles2n64]: Warning - unknown ucode!!!\n");
   if(last_good_ucode != (uint32_t)-1)
      current->type=last_good_ucode;
   else
      current->type = MicrocodeDialog();
   current_type = current->type;
   return current;
}

void GBI_MakeCurrent( MicrocodeInfo *current )
{
   int i;
   if (current != GBI.top)
   {
      if (current == GBI.bottom)
      {
         GBI.bottom = current->higher;
         GBI.bottom->lower = NULL;
      }
      else
      {
         current->higher->lower = current->lower;
         current->lower->higher = current->higher;
      }

      current->higher = NULL;
      current->lower = GBI.top;
      GBI.top->higher = current;
      GBI.top = current;
   }

   if (!GBI.current || (GBI.current->type != current->type))
   {

      for (i = 0; i <= 0xFF; i++)
         GBI.cmd[i] = GBI_Unknown;

      RDP_Init();
      switch (current->type)
      {
         case F3D:       F3D_Init();     break;
         case F3DEX:     F3DEX_Init();   break;
         case F3DEX2:    F3DEX2_Init();  break;
         case L3D:       L3D_Init();     break;
         case L3DEX:     L3DEX_Init();   break;
         case L3DEX2:    L3DEX2_Init();  break;
         case S2DEX:     S2DEX_Init();   break;
         case S2DEX2:    S2DEX2_Init();  break;
         case F3DDKR:    F3DDKR_Init();  break;
			case F3DJFG:	 F3DJFG_Init();	break;
			case F3DSWSE:	 F3DSWSE_Init();	break;
         case F3DWRUS:   F3DWRUS_Init(); break;
         case F3DPD:     F3DPD_Init();   break;
			case Turbo3D:	F3D_Init();		break;
			case ZSortp:	ZSort_Init();	break;
			case F3DEX2CBFD:F3DEX2CBFD_Init(); break;
      }

#ifdef NEW
		if (current->NoN)
      {
         // Disable near and far plane clipping
         glEnable(GL_DEPTH_CLAMP);
         // Enable Far clipping plane in vertex shader
         glEnable(GL_CLIP_DISTANCE0);
      } else {
			glDisable(GL_DEPTH_CLAMP);
			glDisable(GL_CLIP_DISTANCE0);
		}
#endif
   }
#ifdef NEW
   else if (current->NoN != current->NoN)
   {
		if (current->NoN) {
			// Disable near and far plane clipping
			glEnable(GL_DEPTH_CLAMP);
			// Enable Far clipping plane in vertex shader
			glEnable(GL_CLIP_DISTANCE0);
		}
		else {
			glDisable(GL_DEPTH_CLAMP);
			glDisable(GL_CLIP_DISTANCE0);
		}
	}
#endif

   GBI.current = current;
}
