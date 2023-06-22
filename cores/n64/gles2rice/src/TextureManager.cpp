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

#include <math.h>

#include "ConvertImage.h"
#include "DeviceBuilder.h"
#include "FrameBuffer.h"
#include "RenderBase.h"
#include "TextureManager.h"

CTextureManager gTextureManager;

unsigned int g_maxTextureMemUsage = (5*1024*1024);
unsigned int g_amountToFree = (512*1024);
bool g_bUseSetTextureMem = false;

// Returns the first prime greater than or equal to nFirst
inline int GetNextPrime(int nFirst)
{
   int nCurrent = nFirst;

   // Just make sure it's odd
   if ((nCurrent % 2) == 0)
      nCurrent++;

   for (;;)
   {
      // nSqrtCurrent = nCurrent^0.5 + 1 (round up)
      int nSqrtCurrent  = (int)sqrt((double)nCurrent) + 1;
      bool bIsComposite = false;

      // Test all odd numbers from 3..nSqrtCurrent
      for (int i = 3; i <= nSqrtCurrent; i+=2)
      {
         if ((nCurrent % i) == 0)
         {
            bIsComposite = true;
            break;
         }
      }

      if (!bIsComposite)
         return nCurrent;

      // Select next odd candidate...
      nCurrent += 2;
   }
}

///////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////
CTextureManager::CTextureManager() :
    m_pHead(NULL),
    m_pCacheTxtrList(NULL),
    m_numOfCachedTxtrList(809)
{
    m_numOfCachedTxtrList = GetNextPrime(800);

    m_currentTextureMemUsage    = 0;
    m_pYoungestTexture          = NULL;
    m_pOldestTexture            = NULL;

    m_pCacheTxtrList = new TxtrCacheEntry *[m_numOfCachedTxtrList];

    for (uint32_t i = 0; i < m_numOfCachedTxtrList; i++)
        m_pCacheTxtrList[i] = NULL;

    memset(&m_blackTextureEntry, 0, sizeof(TxtrCacheEntry));
    memset(&m_PrimColorTextureEntry, 0, sizeof(TxtrCacheEntry));
    memset(&m_EnvColorTextureEntry, 0, sizeof(TxtrCacheEntry));
    memset(&m_LODFracTextureEntry, 0, sizeof(TxtrCacheEntry));
    memset(&m_PrimLODFracTextureEntry, 0, sizeof(TxtrCacheEntry));
}

CTextureManager::~CTextureManager()
{
    CleanUp();

    delete []m_pCacheTxtrList;
    m_pCacheTxtrList = NULL;    
}


//  Delete all textures.
bool CTextureManager::CleanUp()
{
    RecycleAllTextures();

    if (!g_bUseSetTextureMem)
    {
        while (m_pHead)
        {
            TxtrCacheEntry * pVictim = m_pHead;
            m_pHead = pVictim->pNext;

            delete pVictim;
        }
    }

    if( m_blackTextureEntry.pTexture )      delete m_blackTextureEntry.pTexture;    
    if( m_PrimColorTextureEntry.pTexture )  delete m_PrimColorTextureEntry.pTexture;
    if( m_EnvColorTextureEntry.pTexture )   delete m_EnvColorTextureEntry.pTexture;
    if( m_LODFracTextureEntry.pTexture )    delete m_LODFracTextureEntry.pTexture;
    if( m_PrimLODFracTextureEntry.pTexture )    delete m_PrimLODFracTextureEntry.pTexture;
    memset(&m_blackTextureEntry, 0, sizeof(TxtrCacheEntry));
    memset(&m_PrimColorTextureEntry, 0, sizeof(TxtrCacheEntry));
    memset(&m_EnvColorTextureEntry, 0, sizeof(TxtrCacheEntry));
    memset(&m_LODFracTextureEntry, 0, sizeof(TxtrCacheEntry));
    memset(&m_PrimLODFracTextureEntry, 0, sizeof(TxtrCacheEntry));

    return true;
}

bool CTextureManager::TCacheEntryIsLoaded(TxtrCacheEntry *pEntry)
{
    for (int i = 0; i < MAX_TEXTURES; i++)
    {
        if (g_textures[i].pTextureEntry == pEntry)
            return true;
    }

    return false;
}

// Purge any textures whos last usage was over 5 seconds ago
void CTextureManager::PurgeOldTextures()
{
    if (m_pCacheTxtrList == NULL)
        return;
    
    if (g_bUseSetTextureMem)
        return;

    static const uint32_t dwFramesToKill = 5*30;          // 5 secs at 30 fps
    static const uint32_t dwFramesToDelete = 30*30;       // 30 secs at 30 fps
    
    for ( uint32_t i = 0; i < m_numOfCachedTxtrList; i++ )
    {
        TxtrCacheEntry * pEntry;
        TxtrCacheEntry * pNext;
        
        pEntry = m_pCacheTxtrList[i];
        while (pEntry)
        {
            pNext = pEntry->pNext;
            
            if ( status.gDlistCount - pEntry->FrameLastUsed > dwFramesToKill && !TCacheEntryIsLoaded(pEntry))
                RemoveTexture(pEntry);
            pEntry = pNext;
        }
    }
    
    
    // Remove any old textures that haven't been recycled in 1 minute or so
    // Normally these would be reused
    TxtrCacheEntry* pPrev = NULL;
    TxtrCacheEntry* pCurr = m_pHead;
    TxtrCacheEntry * pNext;

    while (pCurr)
    {
        pNext = pCurr->pNext;
        
        if ( status.gDlistCount - pCurr->FrameLastUsed > dwFramesToDelete && !TCacheEntryIsLoaded(pCurr) )
        {
            if (pPrev != NULL)
               pPrev->pNext = pCurr->pNext;
            else
               m_pHead      = pCurr->pNext;
            
            delete pCurr;
            pCurr = pNext;  
        }
        else
        {
            pPrev = pCurr;
            pCurr = pNext;
        }
    }
}

void CTextureManager::RecycleAllTextures()
{
    if (m_pCacheTxtrList == NULL)
        return;
    
    uint32_t dwCount = 0;
    uint32_t dwTotalUses = 0;
    
    m_pYoungestTexture          = NULL;
    m_pOldestTexture            = NULL;

    for (uint32_t i = 0; i < m_numOfCachedTxtrList; i++)
    {
        while (m_pCacheTxtrList[i])
        {
            TxtrCacheEntry *pTVictim = m_pCacheTxtrList[i];
            m_pCacheTxtrList[i] = pTVictim->pNext;
            
            dwTotalUses += pTVictim->dwUses;
            dwCount++;
            if (g_bUseSetTextureMem)
                delete pTVictim;
            else
               RecycleTexture(pTVictim);
        }
    }
}

void CTextureManager::RecheckHiresForAllTextures()
{
    if (m_pCacheTxtrList == NULL)
        return;

    for (uint32_t i = 0; i < m_numOfCachedTxtrList; i++)
    {
        while (m_pCacheTxtrList[i])
        {
            TxtrCacheEntry *pTVictim = m_pCacheTxtrList[i];
            m_pCacheTxtrList[i] = pTVictim->pNext;
            pTVictim->bExternalTxtrChecked = false;

        }
    }
}


// Add to the recycle list
void CTextureManager::RecycleTexture(TxtrCacheEntry *pEntry)
{
   if (g_bUseSetTextureMem)
      return;

   // Fix me, why I can not reuse the texture in OpenGL,
   // how can I unload texture from video card memory for OpenGL
   delete pEntry;
}

// Search for a texture of the specified dimensions to recycle
TxtrCacheEntry * CTextureManager::ReviveTexture( uint32_t width, uint32_t height )
{
    if (g_bUseSetTextureMem)
        return NULL;

    TxtrCacheEntry* pPrev = NULL;
    TxtrCacheEntry* pCurr = m_pHead;

    while (pCurr)
    {
        if (pCurr->ti.WidthToCreate == width &&
            pCurr->ti.HeightToCreate == height)
        {
            // Remove from list
            if (pPrev != NULL) pPrev->pNext        = pCurr->pNext;
            else               m_pHead = pCurr->pNext;
            
            return pCurr;
        }
        
        pPrev = pCurr;
        pCurr = pCurr->pNext;
    }
    
    return NULL;
}


uint32_t CTextureManager::Hash(uint32_t dwValue)
{
    // Divide by four, because most textures will be on a 4 byte boundry, so bottom four
    // bits are null
    return (dwValue>>2) % m_numOfCachedTxtrList;
}

void CTextureManager::MakeTextureYoungest(TxtrCacheEntry *pEntry)
{
    if (!g_bUseSetTextureMem)
        return;

    if (pEntry == m_pYoungestTexture)
        return;

    // if its the oldest, then change the oldest pointer
    if (pEntry == m_pOldestTexture)
        m_pOldestTexture = pEntry->pNextYoungest;

    // if its a not a new texture, close the gap in the age list
    // where pEntry use to reside
    if (pEntry->pNextYoungest != NULL || pEntry->pLastYoungest != NULL)
    {
        if (pEntry->pNextYoungest != NULL)
        {
            pEntry->pNextYoungest->pLastYoungest = pEntry->pLastYoungest;
        }
        if (pEntry->pLastYoungest != NULL)
        {
            pEntry->pLastYoungest->pNextYoungest = pEntry->pNextYoungest;
        }
    }

    // this texture is now the youngest, so place it on the end of the list
    if (m_pYoungestTexture != NULL)
        m_pYoungestTexture->pNextYoungest = pEntry;

    pEntry->pNextYoungest = NULL;
    pEntry->pLastYoungest = m_pYoungestTexture;
    m_pYoungestTexture = pEntry;
     
    // if this is the first texture in memory then its also the oldest
    if (m_pOldestTexture == NULL)
    {
        m_pOldestTexture = pEntry;
    }
}

void CTextureManager::AddTexture(TxtrCacheEntry *pEntry)
{   
    uint32_t dwKey = Hash(pEntry->ti.Address);
    
    if (m_pCacheTxtrList == NULL)
        return;
    
    //TxtrCacheEntry **p = &m_pCacheTxtrList[dwKey];
    
    // Add to head (not tail, for speed - new textures are more likely to be accessed next)
    pEntry->pNext = m_pCacheTxtrList[dwKey];
    m_pCacheTxtrList[dwKey] = pEntry;

    // Move the texture to the top of the age list
    MakeTextureYoungest(pEntry);
}



TxtrCacheEntry * CTextureManager::GetTxtrCacheEntry(TxtrInfo * pti)
{
    TxtrCacheEntry *pEntry;
    
    if (m_pCacheTxtrList == NULL)
        return NULL;
    
    // See if it is already in the hash table
    uint32_t dwKey = Hash(pti->Address);

    for (pEntry = m_pCacheTxtrList[dwKey]; pEntry; pEntry = pEntry->pNext)
    {
        if ( pEntry->ti == *pti )
        {
            MakeTextureYoungest(pEntry);
            return pEntry;
        }
    }

    return NULL;
}



void CTextureManager::RemoveTexture(TxtrCacheEntry * pEntry)
{
    if (m_pCacheTxtrList == NULL)
        return;

    // See if it is already in the hash table
    uint32_t dwKey = Hash(pEntry->ti.Address);

    TxtrCacheEntry* pPrev = NULL;
    TxtrCacheEntry* pCurr = m_pCacheTxtrList[dwKey];

    while (pCurr)
    {
        // Check that the attributes match
        if ( pCurr->ti == pEntry->ti )
        {
            if (pPrev != NULL) 
                pPrev->pNext = pCurr->pNext;
            else
               m_pCacheTxtrList[dwKey] = pCurr->pNext;

            if (g_bUseSetTextureMem)
            {
                // remove the texture from the age list
                if (pEntry->pNextYoungest != NULL)
                    pEntry->pNextYoungest->pLastYoungest = pEntry->pLastYoungest;
                if (pEntry->pLastYoungest != NULL)
                    pEntry->pLastYoungest->pNextYoungest = pEntry->pNextYoungest;

                // decrease the mem usage counter
                m_currentTextureMemUsage -= (pEntry->pTexture->m_dwWidth * pEntry->pTexture->m_dwHeight * 4);
            
                delete pEntry;
            }
            else
                RecycleTexture(pEntry);

            break;
        }

        pPrev = pCurr;
        pCurr = pCurr->pNext;
    }
}
    
TxtrCacheEntry * CTextureManager::CreateNewCacheEntry(uint32_t dwAddr, uint32_t dwWidth, uint32_t dwHeight)
{
   TxtrCacheEntry * pEntry = NULL;

   if (g_bUseSetTextureMem)
   {
      uint32_t widthToCreate = dwWidth;
      uint32_t heightToCreate = dwHeight;
      unsigned int freeUpSize = (widthToCreate * heightToCreate * 4) + g_amountToFree;

      // make sure there is enough room for the new texture by deleting old textures
      while ((m_currentTextureMemUsage + freeUpSize) > g_maxTextureMemUsage && m_pOldestTexture != NULL)
      {
         TxtrCacheEntry *nextYoungest = m_pOldestTexture->pNextYoungest;

         RemoveTexture(m_pOldestTexture);

         m_pOldestTexture = nextYoungest;

         //printf("Freeing Texture\n");
      }

      m_currentTextureMemUsage += widthToCreate * heightToCreate * 4;
   }
   else
   {
      // Find a used texture
      pEntry = ReviveTexture(dwWidth, dwHeight);
   }

   if (pEntry == NULL || g_bUseSetTextureMem)
   {
      // Couldn't find on - recreate!
      pEntry = new TxtrCacheEntry;
      if (pEntry == NULL)
      {
         _VIDEO_DisplayTemporaryMessage("Error to create an texture entry");
         return NULL;
      }

      pEntry->pTexture = CDeviceBuilder::GetBuilder()->CreateTexture(dwWidth, dwHeight);
      if (pEntry->pTexture == NULL || pEntry->pTexture->GetTexture() == NULL)
      {
         _VIDEO_DisplayTemporaryMessage("Error to create an texture");
         TRACE2("Warning, unable to create %d x %d texture!", dwWidth, dwHeight);
      }
      else
      {
         pEntry->pTexture->m_bScaledS = false;
         pEntry->pTexture->m_bScaledT = false;
      }
   }

   // Initialize
   pEntry->ti.Address = dwAddr;
   pEntry->pNext = NULL;
   pEntry->pNextYoungest = NULL;
   pEntry->pLastYoungest = NULL;
   pEntry->dwUses = 0;
   pEntry->dwTimeLastUsed = status.gRDPTime;
   pEntry->dwCRC = 0;
   pEntry->FrameLastUsed = status.gDlistCount;
   pEntry->FrameLastUpdated = 0;
   pEntry->lastEntry = NULL;
   pEntry->bExternalTxtrChecked = false;
   pEntry->maxCI = -1;

   // Add to the hash table
   AddTexture(pEntry);
   return pEntry;  
}

// If already in table, return
// Otherwise, create surfaces, and load texture into memory
uint32_t dwAsmHeight;
uint32_t dwAsmPitch;
uint32_t dwAsmdwBytesPerLine;
uint32_t dwAsmCRC;
uint32_t dwAsmCRC2;
uint8_t* pAsmStart;

TxtrCacheEntry *g_lastTextureEntry=NULL;
bool lastEntryModified = false;

TxtrCacheEntry * CTextureManager::GetTexture(TxtrInfo * pgti, bool fromTMEM, bool doCRCCheck, bool AutoExtendTexture)
{
    TxtrCacheEntry *pEntry;

    if( g_curRomInfo.bDisableTextureCRC )
        doCRCCheck = false;

    gRDP.texturesAreReloaded = true;

    dwAsmCRC = 0;
    uint32_t dwPalCRC = 0;

    pEntry = GetTxtrCacheEntry(pgti);
    bool loadFromTextureBuffer=false;
    int txtBufIdxToLoadFrom = -1;
    if( (frameBufferOptions.bCheckRenderTextures&&!frameBufferOptions.bWriteBackBufToRDRAM) || (frameBufferOptions.bCheckBackBufs&&!frameBufferOptions.bWriteBackBufToRDRAM) )
    {
        txtBufIdxToLoadFrom = g_pFrameBufferManager->CheckAddrInRenderTextures(pgti->Address, true);
        if( txtBufIdxToLoadFrom >= 0 )
        {
            loadFromTextureBuffer = true;
            // Check if it is the same size,
            RenderTextureInfo &info = gRenderTextureInfos[txtBufIdxToLoadFrom];
            //if( info.pRenderTexture && info.CI_Info.dwAddr == pgti->Address && info.CI_Info.dwFormat == pgti->Format 
            if( info.pRenderTexture && info.CI_Info.dwFormat == pgti->Format 
                && info.CI_Info.dwSize == pgti->Size )
            {
                info.txtEntry.ti = *pgti;
                return &info.txtEntry;
            }
        }
    }

    if (frameBufferOptions.bCheckBackBufs && g_pFrameBufferManager->CheckAddrInBackBuffers(pgti->Address, pgti->HeightToLoad*pgti->Pitch, false) >= 0)
    {
        if( !frameBufferOptions.bWriteBackBufToRDRAM )
        {
            // Load the texture from recent back buffer
            txtBufIdxToLoadFrom = g_pFrameBufferManager->CheckAddrInRenderTextures(pgti->Address, true);
            if( txtBufIdxToLoadFrom >= 0 )
            {
                loadFromTextureBuffer = true;
                // Check if it is the same size,
                RenderTextureInfo &info = gRenderTextureInfos[txtBufIdxToLoadFrom];
                //if( info.pRenderTexture && info.CI_Info.dwAddr == pgti->Address && info.CI_Info.dwFormat == pgti->Format 
                if( info.pRenderTexture && info.CI_Info.dwFormat == pgti->Format 
                    && info.CI_Info.dwSize == pgti->Size )
                {
                    info.txtEntry.ti = *pgti;
                    return &info.txtEntry;
                }
            }
        }
    }

    if (pEntry && pEntry->dwTimeLastUsed == status.gRDPTime && status.gDlistCount != 0 && !status.bN64FrameBufferIsUsed )       // This is not good, Palatte may changes
    {
        // We've already calculated a CRC this frame!
        dwAsmCRC = pEntry->dwCRC;
    }
    else
    {
        if ( doCRCCheck )
        {
            if( loadFromTextureBuffer )
                dwAsmCRC = gRenderTextureInfos[txtBufIdxToLoadFrom].crcInRDRAM;
            else
                CalculateRDRAMCRC(pgti->pPhysicalAddress, pgti->LeftToLoad, pgti->TopToLoad, pgti->WidthToLoad, pgti->HeightToLoad, pgti->Size, pgti->Pitch);
        }
    }

    int maxCI = 0;
    if ( doCRCCheck && (pgti->Format == G_IM_FMT_CI || (pgti->Format == G_IM_FMT_RGBA && pgti->Size <= G_IM_SIZ_8b )))
    {
        //maxCI = pgti->Size == G_IM_SIZ_8b ? 255 : 15;
        extern unsigned char CalculateMaxCI(void *pPhysicalAddress, uint32_t left, uint32_t top, uint32_t width, uint32_t height, uint32_t size, uint32_t pitchInBytes );

        if( !pEntry || pEntry->dwCRC != dwAsmCRC || pEntry->maxCI < 0 )
            maxCI = CalculateMaxCI(pgti->pPhysicalAddress, pgti->LeftToLoad, pgti->TopToLoad, pgti->WidthToLoad, pgti->HeightToLoad, pgti->Size, pgti->Pitch);
        else
            maxCI = pEntry->maxCI;

        //Check PAL CRC
        uint8_t * pStart;
        uint32_t dwPalSize = 16;
        uint32_t dwOffset;

        if( pgti->Size == G_IM_SIZ_8b )
        {
            dwPalSize = 256;
            dwOffset = 0;
        }
        else
        {
            dwOffset = pgti->Palette << 4;
        }

        pStart = (uint8_t*)pgti->PalAddress+dwOffset*2;
        //uint32_t y;
        //for (y = 0; y < dwPalSize*2; y+=4)
        //{
        //  dwPalCRC = (dwPalCRC + *(uint32_t*)&pStart[y]);
        //}

        uint32_t dwAsmCRCSave = dwAsmCRC;
        //dwPalCRC = CalculateRDRAMCRC(pStart, 0, 0, dwPalSize, 1, G_IM_SIZ_16b, dwPalSize*2);
        dwPalCRC = CalculateRDRAMCRC(pStart, 0, 0, maxCI+1, 1, G_IM_SIZ_16b, dwPalSize*2);
        dwAsmCRC = dwAsmCRCSave;
    }

    if (pEntry && doCRCCheck )
    {
        if(pEntry->dwCRC == dwAsmCRC && pEntry->dwPalCRC == dwPalCRC &&
            (!loadFromTextureBuffer || gRenderTextureInfos[txtBufIdxToLoadFrom].updateAtFrame < pEntry->FrameLastUsed ) )
        {
            // Tile is ok, return
            pEntry->dwUses++;
            pEntry->dwTimeLastUsed = status.gRDPTime;
            pEntry->FrameLastUsed = status.gDlistCount;
            pEntry->lastEntry = g_lastTextureEntry;
            g_lastTextureEntry = pEntry;
            lastEntryModified = false;

            return pEntry;
        }
        else
        {
            //Do something
        }
    }

    if (pEntry == NULL)
    {
        // We need to create a new entry, and add it
        //  to the hash table.
        pEntry = CreateNewCacheEntry(pgti->Address, pgti->WidthToCreate, pgti->HeightToCreate);

        if (pEntry == NULL)
        {
            g_lastTextureEntry = pEntry;
            _VIDEO_DisplayTemporaryMessage("Fail to create new texture entry");
            return NULL;
        }
    }

    pEntry->ti = *pgti;
    pEntry->dwCRC = dwAsmCRC;
    pEntry->dwPalCRC = dwPalCRC;
    pEntry->bExternalTxtrChecked = false;
    pEntry->maxCI = maxCI;

    if (pEntry->pTexture != NULL)
    {
       if( pEntry->pTexture->m_dwCreatedTextureWidth < pgti->WidthToCreate )
       {
          pEntry->ti.WidthToLoad = pEntry->pTexture->m_dwCreatedTextureWidth;
          pEntry->pTexture->m_bScaledS = false;
          pEntry->pTexture->m_bScaledT = false;
       }
       if( pEntry->pTexture->m_dwCreatedTextureHeight < pgti->HeightToCreate )
       {
          pEntry->ti.HeightToLoad = pEntry->pTexture->m_dwCreatedTextureHeight;
          pEntry->pTexture->m_bScaledT = false;
          pEntry->pTexture->m_bScaledS = false;
       }

       TextureFmt dwType = pEntry->pTexture->GetSurfaceFormat();

       if (pEntry->pEnhancedTexture)
          free(pEntry->pEnhancedTexture);
       pEntry->pEnhancedTexture  = NULL;
       pEntry->dwEnhancementFlag = TEXTURE_NO_ENHANCEMENT;

       if (dwType != TEXTURE_FMT_UNKNOWN)
       {
          if( loadFromTextureBuffer )
          {
             g_pFrameBufferManager->LoadTextureFromRenderTexture(pEntry, txtBufIdxToLoadFrom);

             extern void ConvertTextureRGBAtoI(TxtrCacheEntry* pEntry, bool alpha);
             if( g_pRenderTextureInfo->CI_Info.dwFormat == G_IM_FMT_I )
             {
                // Convert texture from RGBA to I
                ConvertTextureRGBAtoI(pEntry,false);
             }
             else if( g_pRenderTextureInfo->CI_Info.dwFormat == G_IM_FMT_IA )
             {
                // Convert texture from RGBA to IA
                ConvertTextureRGBAtoI(pEntry,true);
             }
          }
          else
          {
             if (dwType == TEXTURE_FMT_A8R8G8B8)
                ConvertTexture(pEntry, fromTMEM);
             else
                ConvertTexture_16(pEntry, fromTMEM);
             pEntry->FrameLastUpdated = status.gDlistCount;

             if (pEntry->pEnhancedTexture)
                free(pEntry->pEnhancedTexture);
             pEntry->pEnhancedTexture  = NULL;
             pEntry->dwEnhancementFlag = TEXTURE_NO_ENHANCEMENT;
          }
       }

       pEntry->ti.WidthToLoad = pgti->WidthToLoad;
       pEntry->ti.HeightToLoad = pgti->HeightToLoad;

       if( AutoExtendTexture )
       {
          ExpandTextureS(pEntry);
          ExpandTextureT(pEntry);
       }

#ifdef DEBUGGER
       if( pauseAtNext && eventToPause == NEXT_NEW_TEXTURE )
       {
          CRender::g_pRender->SetCurrentTexture( 0, pEntry->pTexture, pEntry->ti.WidthToCreate, pEntry->ti.HeightToCreate, pEntry);
          CRender::g_pRender->DrawTexture(0, G_IM_RGB);
          debuggerPause = true;
          TRACE0("Pause after loading a new texture");
          if( pEntry->ti.Format == G_IM_FMT_YUV )
          {
             TRACE0("This is YUV texture");
          }
          DebuggerAppendMsg("W:%d, H:%d, RealW:%d, RealH:%d, D3DW:%d, D3DH: %d", pEntry->ti.WidthToCreate, pEntry->ti.HeightToCreate,
                pEntry->ti.WidthToLoad, pEntry->ti.HeightToLoad, pEntry->pTexture->m_dwCreatedTextureWidth, pEntry->pTexture->m_dwCreatedTextureHeight);
          DebuggerAppendMsg("ScaledS:%s, ScaledT:%s, CRC=%08X", pEntry->pTexture->m_bScaledS?"T":"F", pEntry->pTexture->m_bScaledT?"T":"F", pEntry->dwCRC);
          DebuggerPause();
          CRender::g_pRender->SetCurrentTexture( 0, NULL, 64, 64, NULL);
       }
#endif
    }

    pEntry->lastEntry = g_lastTextureEntry;
    g_lastTextureEntry = pEntry;
    lastEntryModified = true;
    return pEntry;
}




const char *pszImgFormat[8] = {"RGBA", "YUV", "CI", "IA", "I", "?1", "?2", "?3"};
uint8_t pnImgSize[4]   = {4, 8, 16, 32};
const char *textlutname[4] = {"RGB16", "I16?", "RGBA16", "IA16"};

extern uint16_t g_wRDPTlut[];
extern ConvertFunction  gConvertFunctions_FullTMEM[ 8 ][ 4 ];
extern ConvertFunction  gConvertFunctions[ 8 ][ 4 ];
extern ConvertFunction  gConvertTlutFunctions[ 8 ][ 4 ];
extern ConvertFunction  gConvertFunctions_16[ 8 ][ 4 ];
extern ConvertFunction  gConvertFunctions_16_FullTMEM[ 8 ][ 4 ];
extern ConvertFunction  gConvertTlutFunctions_16[ 8 ][ 4 ];
void CTextureManager::ConvertTexture(TxtrCacheEntry * pEntry, bool fromTMEM)
{
   static uint32_t dwCount = 0;

   ConvertFunction pF;
   if( options.bUseFullTMEM && fromTMEM && status.bAllowLoadFromTMEM )
      pF = gConvertFunctions_FullTMEM[ pEntry->ti.Format ][ pEntry->ti.Size ];
   else
   {
      if( gDP.tiles[7].format == G_IM_FMT_YUV )
      {
         if( gRDP.otherMode.text_tlut>=2 )
            pF = gConvertTlutFunctions[ G_IM_FMT_YUV ][ pEntry->ti.Size ];
         else
            pF = gConvertFunctions[ G_IM_FMT_YUV ][ pEntry->ti.Size ];
      }
      else
      {
         if( gRDP.otherMode.text_tlut>=2 )
            pF = gConvertTlutFunctions[ pEntry->ti.Format ][ pEntry->ti.Size ];
         else
            pF = gConvertFunctions[ pEntry->ti.Format ][ pEntry->ti.Size ];
      }
   }

   if( pF )
   {
      pF( pEntry->pTexture, pEntry->ti );
   }
   else
   {
      TRACE2("ConvertTexture: Unable to decompress %s/%dbpp", pszImgFormat[pEntry->ti.Format], pnImgSize[pEntry->ti.Size]);
   }

   dwCount++;
}

void CTextureManager::ConvertTexture_16(TxtrCacheEntry * pEntry, bool fromTMEM)
{
    static uint32_t dwCount = 0;
    
    ConvertFunction pF;

    if( options.bUseFullTMEM && fromTMEM && status.bAllowLoadFromTMEM )
    {
        pF = gConvertFunctions_16_FullTMEM[ pEntry->ti.Format ][ pEntry->ti.Size ];
    }
    else
    {
        if( gRDP.otherMode.text_tlut>=2 )
            pF = gConvertTlutFunctions_16[ pEntry->ti.Format ][ pEntry->ti.Size ];
        else
            pF = gConvertFunctions_16[ pEntry->ti.Format ][ pEntry->ti.Size ];
    }

    if( pF )
        pF( pEntry->pTexture, pEntry->ti );
    else
    {
        TRACE2("ConvertTexture: Unable to decompress %s/%dbpp", pszImgFormat[pEntry->ti.Format], pnImgSize[pEntry->ti.Size]);
    }

    dwCount++;
}

void CTextureManager::ExpandTexture(TxtrCacheEntry * pEntry, uint32_t sizeToLoad, uint32_t sizeToCreate, uint32_t sizeCreated,
    int arrayWidth, int flag, int mask, int mirror, int clamp, uint32_t otherSize)
{
    if( sizeToLoad >= sizeCreated )
        return;

    uint32_t maskWidth = (1<<mask);
    int size = pEntry->pTexture->GetPixelSize();

#ifdef DEBUGGER
    // Some checks
    if( sizeToLoad > sizeToCreate || sizeToCreate > sizeCreated )   
        TRACE0("Something is wrong, check me here in ExpandTextureS");
#endif

    // Doing Mirror And/Or Wrap in S direction
    // Image has been loaded with width=WidthToLoad, we need to enlarge the image
    // to width = pEntry->ti.WidthToCreate by doing mirroring or wrapping

    DrawInfo di;
    if( !(pEntry->pTexture->StartUpdate(&di)) )
    {
        TRACE0("Can't update the texture");
        return;
    }


    if( mask == 0 )
    {
        // Clamp
        Clamp(di.lpSurface, sizeToLoad, sizeCreated, arrayWidth, otherSize, 
            flag, size);
        pEntry->pTexture->EndUpdate(&di);
        return;
    }

#ifdef DEBUGGER
    if( sizeToLoad > maskWidth )
    {
        TRACE0("Something is wrong, check me here in ExpandTextureS");
        pEntry->pTexture->EndUpdate(&di);
        return;
    }
    if( sizeToLoad == maskWidth && maskWidth == sizeToCreate && sizeToCreate != sizeCreated )
    {
        TRACE0("Something is wrong, check me here in ExpandTextureS");
        pEntry->pTexture->EndUpdate(&di);
        return;
    }
#endif

    if( sizeToLoad == maskWidth )
    {
        uint32_t tempwidth = clamp ? sizeToCreate : sizeCreated;
        if( mirror )
        {
            Mirror(di.lpSurface, sizeToLoad, mask, tempwidth,
                arrayWidth, otherSize, flag, size );
        }
        else
        {
            Wrap(di.lpSurface, sizeToLoad, mask, tempwidth,
                arrayWidth, otherSize, flag, size );
        }

        if( tempwidth < sizeCreated )
        {
            Clamp(di.lpSurface, tempwidth, sizeCreated, arrayWidth, otherSize, 
                flag, size );
        }

        pEntry->pTexture->EndUpdate(&di);
        return;
    }


    if( sizeToLoad < sizeToCreate && sizeToCreate == maskWidth && maskWidth == sizeCreated )
    {
        // widthToLoad < widthToCreate = maskWidth
        Wrap(di.lpSurface, sizeToLoad, mask, sizeCreated, arrayWidth, otherSize, flag, size );

        pEntry->pTexture->EndUpdate(&di);
        return;
    }

    if( sizeToLoad == sizeToCreate && sizeToCreate < maskWidth )
    {
#ifdef DEBUGGER
        if( maskWidth < sizeToCreate )  TRACE0("Incorrect condition, check me");
#endif
        Clamp(di.lpSurface, sizeToLoad, sizeCreated, arrayWidth, otherSize, flag, size );

        pEntry->pTexture->EndUpdate(&di);
        return;
    }

    if( sizeToLoad < sizeToCreate && sizeToCreate < maskWidth )
    {
#ifdef DEBUGGER
        if( clamp ) TRACE0("Incorrect condition, check me");
        if( maskWidth < sizeCreated )   TRACE0("Incorrect condition, check me");
#endif
        Clamp(di.lpSurface, sizeToLoad, sizeCreated, arrayWidth, otherSize, flag, size );
        pEntry->pTexture->EndUpdate(&di);
        return;
    }

    TRACE0("Check me, should not get here");
    pEntry->pTexture->EndUpdate(&di);
}

void CTextureManager::ExpandTextureS(TxtrCacheEntry * pEntry)
{
    TxtrInfo &ti =  pEntry->ti;
    uint32_t textureWidth = pEntry->pTexture->m_dwCreatedTextureWidth;
    ExpandTexture(pEntry, ti.WidthToLoad, ti.WidthToCreate, textureWidth, 
        textureWidth, S_FLAG, ti.maskS, ti.mirrorS, ti.clampS, ti.HeightToLoad);
}

void CTextureManager::ExpandTextureT(TxtrCacheEntry * pEntry)
{
    TxtrInfo &ti =  pEntry->ti;
    uint32_t textureHeight = pEntry->pTexture->m_dwCreatedTextureHeight;
    uint32_t textureWidth = pEntry->pTexture->m_dwCreatedTextureWidth;
    ExpandTexture(pEntry, ti.HeightToLoad, ti.HeightToCreate, textureHeight,
        textureWidth, T_FLAG, ti.maskT, ti.mirrorT, ti.clampT, ti.WidthToLoad);
}


void CTextureManager::ClampS32(uint32_t *array, uint32_t width, uint32_t towidth, uint32_t arrayWidth, uint32_t rows)
{
   if ((int) width <= 0 || (int) towidth < 0)
      return;

   for( uint32_t y = 0; y<rows; y++ )
   {
      uint32_t* line = array+y*arrayWidth;
      uint32_t val = line[width-1];
      for( uint32_t x=width; x<towidth; x++ )
         line[x] = val;
   }
}

void CTextureManager::ClampS16(uint16_t *array, uint32_t width, uint32_t towidth, uint32_t arrayWidth, uint32_t rows)
{
   if ((int) width <= 0 || (int) towidth < 0)
      return;

   for( uint32_t y = 0; y<rows; y++ )
   {
      uint16_t* line = array+y*arrayWidth;
      uint16_t val = line[width-1];
      for( uint32_t x=width; x<towidth; x++ )
         line[x] = val;
   }
}

void CTextureManager::ClampT32(uint32_t *array, uint32_t height, uint32_t toheight, uint32_t arrayWidth, uint32_t cols)
{
   if ((int) height <= 0 || (int) toheight < 0)
      return;

   uint32_t* linesrc = array+arrayWidth*(height-1);
   for( uint32_t y = height; y<toheight; y++ )
   {
      uint32_t* linedst = array+arrayWidth*y;
      for( uint32_t x=0; x<arrayWidth; x++ )
         linedst[x] = linesrc[x];
   }
}

void CTextureManager::ClampT16(uint16_t *array, uint32_t height, uint32_t toheight, uint32_t arrayWidth, uint32_t cols)
{
   if ((int) height <= 0 || (int) toheight < 0)
      return;

   uint16_t* linesrc = array+arrayWidth*(height-1);
   for( uint32_t y = height; y<toheight; y++ )
   {
      uint16_t* linedst = array+arrayWidth*y;
      for( uint32_t x=0; x<arrayWidth; x++ )
         linedst[x] = linesrc[x];
   }
}

void CTextureManager::MirrorS32(uint32_t *array, uint32_t width, uint32_t mask, uint32_t towidth, uint32_t arrayWidth, uint32_t rows)
{
   uint32_t maskval1 = (1<<mask)-1;
   uint32_t maskval2 = (1<<(mask+1))-1;

   for( uint32_t y = 0; y<rows; y++ )
   {
      uint32_t* line = array+y*arrayWidth;
      for( uint32_t x=width; x<towidth; x++ )
         line[x] = (x&maskval2)<=maskval1 ? line[x&maskval1] : line[maskval2-(x&maskval2)];
   }
}

void CTextureManager::MirrorS16(uint16_t *array, uint32_t width, uint32_t mask, uint32_t towidth, uint32_t arrayWidth, uint32_t rows)
{
   uint32_t maskval1 = (1<<mask)-1;
   uint32_t maskval2 = (1<<(mask+1))-1;

   for( uint32_t y = 0; y<rows; y++ )
   {
      uint16_t* line = array+y*arrayWidth;
      for( uint32_t x=width; x<towidth; x++ )
         line[x] = (x&maskval2)<=maskval1 ? line[x&maskval1] : line[maskval2-(x&maskval2)];
   }
}

void CTextureManager::MirrorT32(uint32_t *array, uint32_t height, uint32_t mask, uint32_t toheight, uint32_t arrayWidth, uint32_t cols)
{
   uint32_t maskval1 = (1<<mask)-1;
   uint32_t maskval2 = (1<<(mask+1))-1;

   for( uint32_t y = height; y<toheight; y++ )
   {
      uint32_t srcy = (y&maskval2)<=maskval1 ? y&maskval1 : maskval2-(y&maskval2);
      uint32_t* linesrc = array+arrayWidth*srcy;
      uint32_t* linedst = array+arrayWidth*y;;
      for( uint32_t x=0; x<arrayWidth; x++ )
         linedst[x] = linesrc[x];
   }
}

void CTextureManager::MirrorT16(uint16_t *array, uint32_t height, uint32_t mask, uint32_t toheight, uint32_t arrayWidth, uint32_t cols)
{
   uint32_t maskval1 = (1<<mask)-1;
   uint32_t maskval2 = (1<<(mask+1))-1;

   for( uint32_t y = height; y<toheight; y++ )
   {
      uint32_t srcy = (y&maskval2)<=maskval1 ? y&maskval1 : maskval2-(y&maskval2);
      uint16_t* linesrc = array+arrayWidth*srcy;
      uint16_t* linedst = array+arrayWidth*y;;
      for( uint32_t x=0; x<arrayWidth; x++ )
         linedst[x] = linesrc[x];
   }
}

void CTextureManager::WrapS32(uint32_t *array, uint32_t width, uint32_t mask, uint32_t towidth, uint32_t arrayWidth, uint32_t rows)
{
   uint32_t maskval = (1<<mask)-1;

   for( uint32_t y = 0; y<rows; y++ )
   {
      uint32_t* line = array+y*arrayWidth;
      for( uint32_t x=width; x<towidth; x++ )
         line[x] = line[(x&maskval)<width?(x&maskval):towidth-(x&maskval)];
   }
}

void CTextureManager::WrapS16(uint16_t *array, uint32_t width, uint32_t mask, uint32_t towidth, uint32_t arrayWidth, uint32_t rows)
{
   uint32_t maskval = (1<<mask)-1;

   for( uint32_t y = 0; y<rows; y++ )
   {
      uint16_t* line = array+y*arrayWidth;
      for( uint32_t x=width; x<towidth; x++ )
         line[x] = line[(x&maskval)<width?(x&maskval):towidth-(x&maskval)];
   }
}

void CTextureManager::WrapT32(uint32_t *array, uint32_t height, uint32_t mask, uint32_t toheight, uint32_t arrayWidth, uint32_t cols)
{
   uint32_t maskval = (1<<mask)-1;
   for( uint32_t y = height; y<toheight; y++ )
   {
      uint32_t* linesrc = array+arrayWidth*(y>maskval?y&maskval:y-height);
      uint32_t* linedst = array+arrayWidth*y;;
      for( uint32_t x=0; x<arrayWidth; x++ )
         linedst[x] = linesrc[x];
   }
}

void CTextureManager::WrapT16(uint16_t *array, uint32_t height, uint32_t mask, uint32_t toheight, uint32_t arrayWidth, uint32_t cols)
{
   uint32_t maskval = (1<<mask)-1;
   for( uint32_t y = height; y<toheight; y++ )
   {
      uint16_t* linesrc = array+arrayWidth*(y>maskval?y&maskval:y-height);
      uint16_t* linedst = array+arrayWidth*y;;
      for( uint32_t x=0; x<arrayWidth; x++ )
         linedst[x] = linesrc[x];
   }
}

void CTextureManager::Clamp(void *array, uint32_t width, uint32_t towidth, uint32_t arrayWidth, uint32_t rows, int flag, int size )
{
   if( flag == S_FLAG )    // s
   {
      if( size == 4 ) // 32 bit
         ClampS32((uint32_t*)array, width, towidth, arrayWidth, rows);
      else    // 16 bits
         ClampS16((uint16_t*)array, width, towidth, arrayWidth, rows);
   }
   else    // t
   {
      if( size == 4 ) // 32 bit
         ClampT32((uint32_t*)array, width, towidth, arrayWidth, rows);
      else    // 16 bits
         ClampT16((uint16_t*)array, width, towidth, arrayWidth, rows);
   }
}
void CTextureManager::Wrap(void *array, uint32_t width, uint32_t mask, uint32_t towidth, uint32_t arrayWidth, uint32_t rows, int flag, int size )
{
    if( flag == S_FLAG )    // s
    {
        if( size == 4 ) // 32 bit
            WrapS32((uint32_t*)array, width, mask, towidth, arrayWidth, rows);
        else    // 16 bits
            WrapS16((uint16_t*)array, width, mask, towidth, arrayWidth, rows);
    }
    else    // t
    {
        if( size == 4 ) // 32 bit
            WrapT32((uint32_t*)array, width, mask, towidth, arrayWidth, rows);
        else    // 16 bits
            WrapT16((uint16_t*)array, width, mask, towidth, arrayWidth, rows);
    }
}
void CTextureManager::Mirror(void *array, uint32_t width, uint32_t mask, uint32_t towidth, uint32_t arrayWidth, uint32_t rows, int flag, int size )
{
   if( flag == S_FLAG )    // s
   {
      if( size == 4 ) // 32 bit
         MirrorS32((uint32_t*)array, width, mask, towidth, arrayWidth, rows);
      else    // 16 bits
         MirrorS16((uint16_t*)array, width, mask, towidth, arrayWidth, rows);
   }
   else    // t
   {
      if( size == 4 ) // 32 bit
         MirrorT32((uint32_t*)array, width, mask, towidth, arrayWidth, rows);
      else    // 16 bits
         MirrorT16((uint16_t*)array, width, mask, towidth, arrayWidth, rows);
   }
}


#ifdef DEBUGGER
TxtrCacheEntry * CTextureManager::GetCachedTexture(uint32_t tex)
{
   uint32_t size = 0;
   for( uint32_t i=0; i<m_numOfCachedTxtrList; i++ )
   {
      if( m_pCacheTxtrList[i] == NULL )
         continue;
      else
      {
         TxtrCacheEntry *pEntry;

         for (pEntry = m_pCacheTxtrList[i]; pEntry; pEntry = pEntry->pNext)
         {
            if( size == tex )
               return pEntry;
            else
               size++;
         }
      }
   }
   return NULL;
}

uint32_t CTextureManager::GetNumOfCachedTexture()
{
   uint32_t size = 0;
   for( uint32_t i=0; i<m_numOfCachedTxtrList; i++ )
   {
      if( m_pCacheTxtrList[i] == NULL )
         continue;
      else
      {
         TxtrCacheEntry *pEntry;

         for (pEntry = m_pCacheTxtrList[i]; pEntry; pEntry = pEntry->pNext)
         {
            size++;
         }
      }
   }
   TRACE1("Totally %d texture cached", size);
   return size;
}
#endif


TxtrCacheEntry * CTextureManager::GetBlackTexture(void)
{
    if( m_blackTextureEntry.pTexture == NULL )
    {
        m_blackTextureEntry.pTexture = CDeviceBuilder::GetBuilder()->CreateTexture(4, 4);
        m_blackTextureEntry.ti.WidthToCreate = 4;
        m_blackTextureEntry.ti.HeightToCreate = 4;
        updateColorTexture(m_blackTextureEntry.pTexture,0x00000000);
    }
    return &m_blackTextureEntry;
}
TxtrCacheEntry * CTextureManager::GetPrimColorTexture(uint32_t color)
{
    static uint32_t mcolor = 0;
    if( m_PrimColorTextureEntry.pTexture == NULL )
    {
        m_PrimColorTextureEntry.pTexture = CDeviceBuilder::GetBuilder()->CreateTexture(4, 4);
        m_PrimColorTextureEntry.ti.WidthToCreate = 4;
        m_PrimColorTextureEntry.ti.HeightToCreate = 4;
        updateColorTexture(m_PrimColorTextureEntry.pTexture,color);
        gRDP.texturesAreReloaded = true;
    }
    else if( mcolor != color )
    {
        updateColorTexture(m_PrimColorTextureEntry.pTexture,color);
        gRDP.texturesAreReloaded = true;
    }

    mcolor = color;
    return &m_PrimColorTextureEntry;
}
TxtrCacheEntry * CTextureManager::GetEnvColorTexture(uint32_t color)
{
    static uint32_t mcolor = 0;
    if( m_EnvColorTextureEntry.pTexture == NULL )
    {
        m_EnvColorTextureEntry.pTexture = CDeviceBuilder::GetBuilder()->CreateTexture(4, 4);
        m_EnvColorTextureEntry.ti.WidthToCreate = 4;
        m_EnvColorTextureEntry.ti.HeightToCreate = 4;
        gRDP.texturesAreReloaded = true;

        updateColorTexture(m_EnvColorTextureEntry.pTexture,color);
    }
    else if( mcolor != color )
    {
        updateColorTexture(m_EnvColorTextureEntry.pTexture,color);
        gRDP.texturesAreReloaded = true;
    }

    mcolor = color;
    return &m_EnvColorTextureEntry;
}
TxtrCacheEntry * CTextureManager::GetLODFracTexture(uint8_t fac)
{
    static uint8_t mfac = 0;
    if( m_LODFracTextureEntry.pTexture == NULL )
    {
        m_LODFracTextureEntry.pTexture = CDeviceBuilder::GetBuilder()->CreateTexture(4, 4);
        m_LODFracTextureEntry.ti.WidthToCreate = 4;
        m_LODFracTextureEntry.ti.HeightToCreate = 4;
        uint32_t factor = fac;
        uint32_t color = fac;
        color |= factor << 8;
        color |= color << 16;
        updateColorTexture(m_LODFracTextureEntry.pTexture,color);
        gRDP.texturesAreReloaded = true;
    }
    else if( mfac != fac )
    {
        uint32_t factor = fac;
        uint32_t color = fac;
        color |= factor << 8;
        color |= color << 16;
        updateColorTexture(m_LODFracTextureEntry.pTexture,color);
        gRDP.texturesAreReloaded = true;
    }

    mfac = fac;
    return &m_LODFracTextureEntry;
}

TxtrCacheEntry * CTextureManager::GetPrimLODFracTexture(uint8_t fac)
{
    static uint8_t mfac = 0;
    if( m_PrimLODFracTextureEntry.pTexture == NULL )
    {
        m_PrimLODFracTextureEntry.pTexture = CDeviceBuilder::GetBuilder()->CreateTexture(4, 4);
        m_PrimLODFracTextureEntry.ti.WidthToCreate = 4;
        m_PrimLODFracTextureEntry.ti.HeightToCreate = 4;
        uint32_t factor = fac;
        uint32_t color = fac;
        color |= factor << 8;
        color |= color << 16;
        updateColorTexture(m_PrimLODFracTextureEntry.pTexture,color);
        gRDP.texturesAreReloaded = true;
    }
    else if( mfac != fac )
    {
        uint32_t factor = fac;
        uint32_t color = fac;
        color |= factor << 8;
        color |= color << 16;
        updateColorTexture(m_PrimLODFracTextureEntry.pTexture,color);
        gRDP.texturesAreReloaded = true;
    }

    mfac = fac;
    return &m_PrimLODFracTextureEntry;
}

TxtrCacheEntry * CTextureManager::GetConstantColorTexture(uint32_t constant)
{
   switch( constant )
   {
      case MUX_PRIM:
         return GetPrimColorTexture(gRDP.primitiveColor);
      case MUX_ENV:
         return GetEnvColorTexture(gRDP.envColor);
      case MUX_LODFRAC:
         return GetLODFracTexture((uint8_t)gRDP.LODFrac);
      default:    // MUX_PRIMLODFRAC
         break;
   }

   return GetPrimLODFracTexture((uint8_t)gRDP.primLODFrac);
}

void CTextureManager::updateColorTexture(CTexture *ptexture, uint32_t color)
{
   DrawInfo di;
   if( !(ptexture->StartUpdate(&di)) )
   {
      TRACE0("Cann't update the texture");
      return;
   }

   int size = ptexture->GetPixelSize();
   switch( size )
   {
      case 2: // 16 bits
         {
            uint16_t *buf = (uint16_t*)di.lpSurface;
            uint16_t color16= (uint16_t)((color>>4)&0xF);
            color16 |= ((color>>12)&0xF)<<4;
            color16 |= ((color>>20)&0xF)<<8;
            color16 |= ((color>>28)&0xF)<<12;
            for( int i=0; i<16; i++ )
            {
               buf[i] = color16;
            }
         }
         break;
      case 4: // 32 bits
         {
            uint32_t *buf = (uint32_t*)di.lpSurface;
            for( int i=0; i<16; i++ )
            {
               buf[i] = color;
            }
         }
         break;
   }

   ptexture->EndUpdate(&di);
}

void ConvertTextureRGBAtoI(TxtrCacheEntry* pEntry, bool alpha)
{
   DrawInfo srcInfo;   
   if( pEntry->pTexture->StartUpdate(&srcInfo) )
   {
      uint32_t *buf;
      uint32_t val;
      uint32_t r,g,b,a,i;

      for(int nY = 0; nY < srcInfo.dwCreatedHeight; nY++)
      {
         buf = (uint32_t*)((uint8_t*)srcInfo.lpSurface+nY*srcInfo.lPitch);
         for(int nX = 0; nX < srcInfo.dwCreatedWidth; nX++)
         {
            val = buf[nX];
            b = (val>>0)&0xFF;
            g = (val>>8)&0xFF;
            r = (val>>16)&0xFF;
            i = (r+g+b)/3;
            a = alpha?(val&0xFF000000):(i<<24);
            buf[nX] = (a|(i<<16)|(i<<8)|i);
         }
      }
      pEntry->pTexture->EndUpdate(&srcInfo);
   }
}

