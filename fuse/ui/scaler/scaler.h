/* ScummVM - Scumm Interpreter
 * Copyright (C) 2002-2003 The ScummVM project and Fredrick Meunier
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 */

#ifndef SCALER_H
#define SCALER_H

typedef enum scaler_type {
  GFX_HALF = 0,
  GFX_NORMAL,
  GFX_DOUBLESIZE,
  GFX_TRIPLESIZE,
  GFX_2XSAI,
  GFX_SUPER2XSAI,
  GFX_SUPEREAGLE,
  GFX_ADVMAME2X,
  GFX_TV2X,
  GFX_TIMEXTV,
  GFX_NUM
} scaler_type;

extern scaler_type current_scaler;
extern int scalers_registered;

int scaler_select_id( const char *scaler_mode );
void scaler_register_clear( void );
int scaler_select_scaler( scaler_type scaler );
void scaler_register( scaler_type scaler );
int scaler_is_supported( scaler_type scaler );
const char *scaler_name( scaler_type scaler );

typedef void ScalerProc(BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr,
	                BYTE *dstPtr, DWORD dstPitch, int width, int height);

extern int Init_2xSaI(DWORD BitFormat);
extern void _2xSaI(BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr, BYTE *dstPtr,
                   DWORD dstPitch, int width, int height);
extern void Super2xSaI(BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr,
	               BYTE *dstPtr, DWORD dstPitch, int width, int height);
extern void SuperEagle(BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr,
	               BYTE *dstPtr, DWORD dstPitch, int width, int height);
extern void AdvMame2x(BYTE *srcPtr, DWORD srcPitch, BYTE *null,
	             BYTE *dstPtr, DWORD dstPitch, int width, int height);
extern void Half(BYTE *srcPtr, DWORD srcPitch, BYTE *null,
	             BYTE *dstPtr, DWORD dstPitch, int width, int height);
extern void Normal1x(BYTE *srcPtr, DWORD srcPitch, BYTE *null,
	             BYTE *dstPtr, DWORD dstPitch, int width, int height);
extern void Normal2x(BYTE *srcPtr, DWORD srcPitch, BYTE *null,
	             BYTE *dstPtr, DWORD dstPitch, int width, int height);
extern void Normal3x(BYTE *srcPtr, DWORD srcPitch, BYTE *null,
	             BYTE *dstPtr, DWORD dstPitch, int width, int height);
extern void TV2x(BYTE *srcPtr, DWORD srcPitch, BYTE *null,
               BYTE *dstPtr, DWORD dstPitch, int width, int height);
extern void TimexTV(BYTE *srcPtr, DWORD srcPitch, BYTE *null, BYTE *dstPtr,
               DWORD dstPitch, int width, int height);

#endif
