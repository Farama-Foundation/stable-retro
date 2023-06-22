/*
Copyright (C) 2003 Rice1964

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef __TEXTUREHANDLER_H__
#define __TEXTUREHANDLER_H__

#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <strings.h>
#endif

#include "../../Graphics/RDP/gDP_state.h"
#include "typedefs.h"
#include "Texture.h"
#define absi(x)     ((x)>=0?(x):(-x))
#define S_FLAG  0
#define T_FLAG  1

class TxtrInfo
{
public:
    uint32_t WidthToCreate;
    uint32_t HeightToCreate;

    uint32_t Address;
    void  *pPhysicalAddress;

    uint32_t Format;
    uint32_t Size;

    int  LeftToLoad;
    int  TopToLoad;
    uint32_t WidthToLoad;
    uint32_t HeightToLoad;
    uint32_t Pitch;

    uint8_t *PalAddress;
    uint32_t TLutFmt;
    uint32_t Palette;
    
    bool  bSwapped;
    
    uint32_t maskS;
    uint32_t maskT;

    bool  clampS;
    bool  clampT;
    bool  mirrorS;
    bool  mirrorT;

    int   tileNo;

    inline TxtrInfo& operator = (const TxtrInfo& src)
    {
        memcpy(this, &src, sizeof( TxtrInfo ));
        return *this;
    }

    inline TxtrInfo& operator = (const gDPTile& tile)
    {
        Format  = tile.format;
        Size    = tile.size;
        Palette = tile.palette;
        
        maskS   = tile.masks;
        maskT   = tile.maskt;
        mirrorS = tile.mirrors;
        mirrorT = tile.mirrort;
        clampS  = tile.clamps;
        clampT  = tile.clampt;

        return *this;
    }

    inline bool operator == ( const TxtrInfo& sec)
    {
        return (
            Address == sec.Address &&
            WidthToLoad == sec.WidthToLoad &&
            HeightToLoad == sec.HeightToLoad &&
            WidthToCreate == sec.WidthToCreate &&
            HeightToCreate == sec.HeightToCreate &&
            maskS == sec.maskS &&
            maskT == sec.maskT &&
            TLutFmt == sec.TLutFmt &&
            PalAddress == sec.PalAddress &&
            Palette == sec.Palette &&
            LeftToLoad == sec.LeftToLoad &&
            TopToLoad == sec.TopToLoad &&
            Format == sec.Format &&
            Size == sec.Size &&
            Pitch == sec.Pitch &&
            bSwapped == sec.bSwapped &&
            mirrorS == sec.mirrorS &&
            mirrorT == sec.mirrorT &&
            clampS == sec.clampS &&
            clampT == sec.clampT
            );
    }

    inline bool isEqual(const TxtrInfo& sec)
    {
        return (*this == sec);
    }
    
} ;



typedef struct TxtrCacheEntry
{
    TxtrCacheEntry():
        pTexture(NULL),pEnhancedTexture(NULL),txtrBufIdx(0) {}

    ~TxtrCacheEntry()
    {
       if (pTexture)
          free(pTexture);
       if (pEnhancedTexture)
          free(pEnhancedTexture);

       pTexture         = NULL;
       pEnhancedTexture = NULL;
    }
    
    struct TxtrCacheEntry *pNext;       // Must be first element!

    struct TxtrCacheEntry *pNextYoungest;
    struct TxtrCacheEntry *pLastYoungest;

    TxtrInfo ti;
    uint32_t      dwCRC;
    uint32_t      dwPalCRC;
    int         maxCI;

    uint32_t  dwUses;         // Total times used (for stats)
    uint32_t  dwTimeLastUsed; // timeGetTime of time of last usage
    uint32_t  FrameLastUsed;  // Frame # that this was last used
    uint32_t  FrameLastUpdated;

    CTexture    *pTexture;
    CTexture    *pEnhancedTexture;

    uint32_t      dwEnhancementFlag;
    int         txtrBufIdx;
    bool        bExternalTxtrChecked;

    TxtrCacheEntry *lastEntry;
} TxtrCacheEntry;


//*****************************************************************************
// Texture cache implementation
//*****************************************************************************
class CTextureManager
{
protected:
    TxtrCacheEntry * CreateNewCacheEntry(uint32_t dwAddr, uint32_t dwWidth, uint32_t dwHeight);
    void AddTexture(TxtrCacheEntry *pEntry);
    void RemoveTexture(TxtrCacheEntry * pEntry);
    void RecycleTexture(TxtrCacheEntry *pEntry);
    TxtrCacheEntry * ReviveTexture( uint32_t width, uint32_t height );
    TxtrCacheEntry * GetTxtrCacheEntry(TxtrInfo * pti);
    
    void ConvertTexture(TxtrCacheEntry * pEntry, bool fromTMEM);
    void ConvertTexture_16(TxtrCacheEntry * pEntry, bool fromTMEM);

    void ClampS32(uint32_t *array, uint32_t width, uint32_t towidth, uint32_t arrayWidth, uint32_t rows);
    void ClampS16(uint16_t *array, uint32_t width, uint32_t towidth, uint32_t arrayWidth, uint32_t rows);
    void ClampT32(uint32_t *array, uint32_t height, uint32_t toheight, uint32_t arrayWidth, uint32_t cols);
    void ClampT16(uint16_t *array, uint32_t height, uint32_t toheight, uint32_t arrayWidth, uint32_t cols);

    void MirrorS32(uint32_t *array, uint32_t width, uint32_t mask, uint32_t towidth, uint32_t arrayWidth, uint32_t rows);
    void MirrorS16(uint16_t *array, uint32_t width, uint32_t mask, uint32_t towidth, uint32_t arrayWidth, uint32_t rows);
    void MirrorT32(uint32_t *array, uint32_t height, uint32_t mask, uint32_t toheight, uint32_t arrayWidth, uint32_t cols);
    void MirrorT16(uint16_t *array, uint32_t height, uint32_t mask, uint32_t toheight, uint32_t arrayWidth, uint32_t cols);

    void WrapS32(uint32_t *array, uint32_t width, uint32_t mask, uint32_t towidth, uint32_t arrayWidth, uint32_t rows);
    void WrapS16(uint16_t *array, uint32_t width, uint32_t mask, uint32_t towidth, uint32_t arrayWidth, uint32_t rows);
    void WrapT32(uint32_t *array, uint32_t height, uint32_t mask, uint32_t toheight, uint32_t arrayWidth, uint32_t cols);
    void WrapT16(uint16_t *array, uint32_t height, uint32_t mask, uint32_t toheight, uint32_t arrayWidth, uint32_t cols);

    void ExpandTextureS(TxtrCacheEntry * pEntry);
    void ExpandTextureT(TxtrCacheEntry * pEntry);
    void ExpandTexture(TxtrCacheEntry * pEntry, uint32_t sizeOfLoad, uint32_t sizeToCreate, uint32_t sizeCreated,
        int arrayWidth, int flag, int mask, int mirror, int clamp, uint32_t otherSize);

    uint32_t Hash(uint32_t dwValue);
    bool TCacheEntryIsLoaded(TxtrCacheEntry *pEntry);

    void updateColorTexture(CTexture *ptexture, uint32_t color);
    
public:
    void Wrap(void *array, uint32_t width, uint32_t mask, uint32_t towidth, uint32_t arrayWidth, uint32_t rows, int flag, int size );
    void Clamp(void *array, uint32_t width, uint32_t towidth, uint32_t arrayWidth, uint32_t rows, int flag, int size );
    void Mirror(void *array, uint32_t width, uint32_t mask, uint32_t towidth, uint32_t arrayWidth, uint32_t rows, int flag, int size );
    
protected:
    TxtrCacheEntry * m_pHead;
    TxtrCacheEntry ** m_pCacheTxtrList;
    uint32_t m_numOfCachedTxtrList;

    TxtrCacheEntry m_blackTextureEntry;
    TxtrCacheEntry m_PrimColorTextureEntry;
    TxtrCacheEntry m_EnvColorTextureEntry;
    TxtrCacheEntry m_LODFracTextureEntry;
    TxtrCacheEntry m_PrimLODFracTextureEntry;
    TxtrCacheEntry * GetPrimColorTexture(uint32_t color);
    TxtrCacheEntry * GetEnvColorTexture(uint32_t color);
    TxtrCacheEntry * GetLODFracTexture(uint8_t fac);
    TxtrCacheEntry * GetPrimLODFracTexture(uint8_t fac);

    void MakeTextureYoungest(TxtrCacheEntry *pEntry);
    unsigned int m_currentTextureMemUsage;
    TxtrCacheEntry *m_pYoungestTexture;
    TxtrCacheEntry *m_pOldestTexture;

public:
    CTextureManager();
    ~CTextureManager();

    TxtrCacheEntry * GetBlackTexture(void);
    TxtrCacheEntry * GetConstantColorTexture(uint32_t constant);
    TxtrCacheEntry * GetTexture(TxtrInfo * pgti, bool fromTMEM, bool doCRCCheck, bool AutoExtendTexture);
    
    void PurgeOldTextures();
    void RecycleAllTextures();
    void RecheckHiresForAllTextures();
    bool CleanUp();
    
#ifdef DEBUGGER
    TxtrCacheEntry * GetCachedTexture(uint32_t tex);
    uint32_t GetNumOfCachedTexture();
#endif
};

extern CTextureManager gTextureManager;     // The global instance of CTextureManager class
extern void DumpCachedTexture(TxtrCacheEntry &entry);

#endif

