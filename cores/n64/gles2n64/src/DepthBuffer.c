#include <stdint.h>
#include <stdlib.h>
#include <boolean.h>
#include "DepthBuffer.h"

DepthBufferInfo depthBuffer;

void DepthBuffer_Init(void)
{
   depthBuffer.current = NULL;
   depthBuffer.top = NULL;
   depthBuffer.bottom = NULL;
   depthBuffer.numBuffers = 0;
}

void DepthBuffer_RemoveBottom(void)
{
   DepthBuffer *newBottom = depthBuffer.bottom->higher;

   if (depthBuffer.bottom == depthBuffer.top)
      depthBuffer.top = NULL;

   free( depthBuffer.bottom );

   depthBuffer.bottom = newBottom;

   if (depthBuffer.bottom != NULL)
      depthBuffer.bottom->lower = NULL;

   depthBuffer.numBuffers--;
}

void DepthBuffer_Remove( DepthBuffer *buffer )
{
   if ((buffer == depthBuffer.bottom) &&
         (buffer == depthBuffer.top))
   {
      depthBuffer.top = NULL;
      depthBuffer.bottom = NULL;
   }
   else if (buffer == depthBuffer.bottom)
   {
      depthBuffer.bottom = buffer->higher;

      if (depthBuffer.bottom)
         depthBuffer.bottom->lower = NULL;
   }
   else if (buffer == depthBuffer.top)
   {
      depthBuffer.top = buffer->lower;

      if (depthBuffer.top)
         depthBuffer.top->higher = NULL;
   }
   else
   {
      buffer->higher->lower = buffer->lower;
      buffer->lower->higher = buffer->higher;
   }

   free( buffer );
   depthBuffer.numBuffers--;
}

void DepthBuffer_RemoveBuffer( uint32_t address )
{
    DepthBuffer *current = depthBuffer.bottom;
    while (current != NULL)
    {
        if (current->address == address)
        {
            DepthBuffer_Remove( current );
            return;
        }
        current = current->higher;
    }
}

DepthBuffer *DepthBuffer_AddTop(void)
{
   DepthBuffer *newtop = (DepthBuffer*)malloc( sizeof( DepthBuffer ) );

   newtop->lower = depthBuffer.top;
   newtop->higher = NULL;

   if (depthBuffer.top)
      depthBuffer.top->higher = newtop;

   if (!depthBuffer.bottom)
      depthBuffer.bottom = newtop;

   depthBuffer.top = newtop;

   depthBuffer.numBuffers++;

   return newtop;
}

void DepthBuffer_MoveToTop( DepthBuffer *newtop )
{
    if (newtop == depthBuffer.top)
        return;

    if (newtop == depthBuffer.bottom)
    {
        depthBuffer.bottom = newtop->higher;
        depthBuffer.bottom->lower = NULL;
    }
    else
    {
        newtop->higher->lower = newtop->lower;
        newtop->lower->higher = newtop->higher;
    }

    newtop->higher = NULL;
    newtop->lower = depthBuffer.top;
    depthBuffer.top->higher = newtop;
    depthBuffer.top = newtop;
}

void DepthBuffer_Destroy(void)
{
   while (depthBuffer.bottom)
      DepthBuffer_RemoveBottom();

   depthBuffer.top = NULL;
}

void DepthBuffer_SetBuffer( uint32_t address )
{
   DepthBuffer *current = depthBuffer.top;

   // Search through saved depth buffers
   while (current != NULL)
   {
      if (current->address == address)
      {
         DepthBuffer_MoveToTop( current );
         depthBuffer.current = current;
         return;
      }
      current = current->lower;
   }

   current = DepthBuffer_AddTop();

   current->address    = address;
   current->cleared    = true;
   depthBuffer.current = current;
}

DepthBuffer *DepthBuffer_FindBuffer( uint32_t address )
{
   DepthBuffer *current = depthBuffer.top;

   while (current)
   {
      if (current->address == address)
         return current;
      current = current->lower;
   }

   return NULL;
}
