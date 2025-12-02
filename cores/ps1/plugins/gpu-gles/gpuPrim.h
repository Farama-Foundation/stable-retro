/***************************************************************************
                          prim.h  -  description
                             -------------------
    begin                : Sun Mar 08 2009
    copyright            : (C) 1999-2009 by Pete Bernert
    web                  : www.pbernert.com   
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************// 
// History of changes:
//
// 2009/03/08 - Pete  
// - generic cleanup for the Peops release
//
//*************************************************************************// 

#ifndef _PRIMDRAW_H_
#define _PRIMDRAW_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "gpuExternals.h"
#include "gpuStdafx.h"

#ifndef _WINDOWS
extern EGLSurface surface;
extern EGLDisplay display;
#endif

void UploadScreen (int Position);
void PrepareFullScreenUpload (int Position);
BOOL CheckAgainstScreen(short imageX0,short imageY0,short imageX1,short imageY1);
BOOL CheckAgainstFrontScreen(short imageX0,short imageY0,short imageX1,short imageY1);
BOOL FastCheckAgainstScreen(short imageX0,short imageY0,short imageX1,short imageY1);
BOOL FastCheckAgainstFrontScreen(short imageX0,short imageY0,short imageX1,short imageY1);
BOOL bCheckFF9G4(unsigned char * baseAddr);
void SetScanTrans(void);
void SetScanTexTrans(void);
void DrawMultiBlur(void);
void CheckWriteUpdate();

#ifdef __cplusplus
}
#endif

#endif // _PRIMDRAW_H_
