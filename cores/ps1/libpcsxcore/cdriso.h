/***************************************************************************
 *   Copyright (C) 2007 PCSX-df Team                                       *
 *   Copyright (C) 2009 Wei Mingzhi                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

#ifndef CDRISO_H
#define CDRISO_H

#ifdef __cplusplus
extern "C" {
#endif

struct CdrStat;

int ISOinit(void);
int ISOshutdown(void);
int ISOopen(const char *fname);
int ISOclose(void);
int ISOgetTN(unsigned char *buffer);
int ISOgetTD(int track, unsigned char *buffer);
int ISOreadTrack(const unsigned char *time, void *buf);
int ISOreadCDDA(const unsigned char *time, void *buffer);
int ISOreadSub(const unsigned char *time, void *buffer);
int ISOgetStatus(struct CdrStat *stat);

extern void * (*ISOgetBuffer)(void);

extern unsigned int cdrIsoMultidiskCount;
extern unsigned int cdrIsoMultidiskSelect;

#ifdef __cplusplus
}
#endif
#endif
