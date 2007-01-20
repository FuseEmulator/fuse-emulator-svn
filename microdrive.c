/* microdrive.c: Routines for handling microdrive images
   Copyright (c) 2004-2005 Philip Kendall

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
     Post: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England
*/

#include <config.h>

#include <string.h>

#include "internals.h"

/* The type itself */

struct libspectrum_microdrive {

  libspectrum_byte data[ LIBSPECTRUM_MICRODRIVE_CARTRIDGE_LENGTH ];
  int write_protect;
  libspectrum_byte cartridge_len;

};

typedef struct libspectrum_microdrive_block {

  libspectrum_byte hdflag;		/* bit0 = 1-head, ( 0 - data ) */
  libspectrum_byte hdbnum;		/* block num 1 -- 254 */
  libspectrum_word hdblen;		/* not used */
  libspectrum_byte hdbnam[11];		/* cartridge label + \0 */
  libspectrum_byte hdchks;		/* header checksum */
 
  libspectrum_byte recflg;		/* bit0 = 0-data, bit1, bit2 */
  libspectrum_byte recnum;		/* data block num  */
  libspectrum_word reclen;		/* block length 0 -- 512 */
  libspectrum_byte recnam[11];		/* record (file) name + \0 */
  libspectrum_byte rechks;		/* descriptor checksum */

  libspectrum_byte data[512];		/* data bytes */
  libspectrum_byte datchk;		/* data checksum */

} libspectrum_microdrive_block;

const static size_t MDR_LENGTH = LIBSPECTRUM_MICRODRIVE_CARTRIDGE_LENGTH + 1;

/* Constructor/destructor */

/* Allocate a microdrive image */
libspectrum_error
libspectrum_microdrive_alloc( libspectrum_microdrive **microdrive )
{
  *microdrive = malloc( sizeof( **microdrive ) );
  if( !*microdrive ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_MEMORY,
      "libspectrum_microdrive_alloc: out of memory at %s:%d", __FILE__,
      __LINE__
    );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

/* Free a microdrive image */
libspectrum_error
libspectrum_microdrive_free( libspectrum_microdrive *microdrive )
{
  free( microdrive );

  return LIBSPECTRUM_ERROR_NONE;
}

/* Accessors */

libspectrum_byte
libspectrum_microdrive_data( const libspectrum_microdrive *microdrive,
			     size_t which )
{
  return microdrive->data[ which ];
}

void
libspectrum_microdrive_set_data( libspectrum_microdrive *microdrive,
				 size_t which, libspectrum_byte data )
{
  microdrive->data[ which ] = data;
}

int
libspectrum_microdrive_write_protect( const libspectrum_microdrive *microdrive )
{
  return microdrive->write_protect;
}

void
libspectrum_microdrive_set_write_protect( libspectrum_microdrive *microdrive,
					  int write_protect )
{
  microdrive->write_protect = write_protect;
}

libspectrum_byte
libspectrum_microdrive_cartridge_len( const libspectrum_microdrive *microdrive )
{
  return microdrive->cartridge_len;
}

void
libspectrum_microdrive_set_cartridge_len( libspectrum_microdrive *microdrive,
			     libspectrum_byte len )
{
  microdrive->cartridge_len = len;
}

void
libspectrum_microdrive_get_block( const libspectrum_microdrive *microdrive,
				  libspectrum_byte which,
				  libspectrum_microdrive_block *block )
{
  const libspectrum_byte *d;
  
  d = microdrive->data + which * LIBSPECTRUM_MICRODRIVE_BLOCK_LEN;

  /* The block header */
  block->hdflag = *(d++);
  block->hdbnum = *(d++);
  block->hdblen = *d + ( *( d + 1 ) << 8 ); d += 2;
  memcpy( block->hdbnam, d, 10 ); d += 10; block->hdbnam[10] = '\0';
  block->hdchks = *(d++);

  /* The data header */
  block->recflg = *(d++);
  block->recnum = *(d++);
  block->reclen = *d + ( *( d + 1 ) << 8 ); d += 2;
  memcpy( block->recnam, d, 10 ); d += 10; block->recnam[10] = '\0';
  block->rechks = *(d++);

  /* The data itself */
  memcpy( block->data, d, 512 ); d += 512;
  block->datchk = *(d++);

}

void
libspectrum_microdrive_set_block( libspectrum_microdrive *microdrive,
				  libspectrum_byte which,
				  libspectrum_microdrive_block *block )
{
  libspectrum_byte *d = &microdrive->data[ which * 
					   LIBSPECTRUM_MICRODRIVE_BLOCK_LEN ];
  /* The block header */
  *(d++) = block->hdflag;
  *(d++) = block->hdbnum;
  *(d++) = block->hdblen & 0xff; *(d++) = block->hdblen >> 8;
  memcpy( d, block->hdbnam, 10 ); d += 10;
  *(d++) = block->hdchks;

  /* The data header */
  *(d++) = block->recflg;
  *(d++) = block->recnum;
  *(d++) = block->reclen & 0xff; *(d++) = block->reclen >> 8;
  memcpy( d, block->recnam, 10 ); d += 10;
  *(d++) = block->rechks;

  /* The data itself */
  memcpy( d, block->data, 512 ); d += 512;
  *(d++) = block->datchk;

}

int
libspectrum_microdrive_checksum( libspectrum_microdrive *microdrive,
				 libspectrum_byte what )
{
  libspectrum_byte *data;
  unsigned int checksum, carry;
  int i;

#define DO_CHECK \
    checksum += *data;		/* LD    A,E */ \
				/* ADD A, (HL) */ \
    if( checksum > 255 ) {	/* ... ADD A, (HL) --> CY*/ \
      carry = 1;		\
      checksum -= 256;		\
    } else {			\
      carry = 0;		\
    }				\
    data++;			/* INC HL */ \
    checksum += carry + 1;	/* ADC A, #01 */ \
    if( checksum == 256 ) {	/* JR Z,LSTCHK */ \
      checksum -= 256;		/* LD E,A */\
    } else {			\
      checksum--;		/* DEC A */ \
    }

/*

LOOP-C:  LD      A,E             ; fetch running sum
         ADD     A,(HL)          ; add to current location.
         INC     HL              ; point to next location.


         ADC     A,#01           ; avoid the value #FF.
         JR      Z,LSTCHK        ; forward to STCHK

         DEC     A               ; decrement.

LSTCHK   LD      E,A             ; update the 8-bit sum.

*/  
  data = &(microdrive->data[ what * LIBSPECTRUM_MICRODRIVE_BLOCK_LEN ]);
  
  if( ( *( data + 15 ) & 2 ) && 
        ( *( data + 17 ) == 0 ) && ( *( data + 18 ) == 0 ) ) {
    return -1;		/* PRESET BAD BLOCK */
  }

  checksum = 0;			/* Block header */
  for( i = LIBSPECTRUM_MICRODRIVE_HEAD_LEN; i > 1; i-- ) {
    DO_CHECK;
  }
  if( *(data++) != checksum ) 
    return 1;

  checksum = 0;			/* Record header */
  for( i = LIBSPECTRUM_MICRODRIVE_HEAD_LEN; i > 1; i-- ) {
    DO_CHECK;
  }
  if( *(data++) != checksum ) 
    return 2;

  if( ( *( data - 13 ) == 0 ) && ( *( data - 12 ) == 0 ) ) {
    return 0;		/* Erased / empty block: data checksum irrelevant */
  }

  checksum = 0;			/* Data */
  for( i = LIBSPECTRUM_MICRODRIVE_DATA_LEN; i > 0; i-- ) {
    DO_CHECK;
  }
  if( *data != checksum )
    return 3;

  return 0;
}

/* .mdr format routines */

libspectrum_error
libspectrum_microdrive_mdr_read( libspectrum_microdrive *microdrive,
				 libspectrum_byte *buffer, size_t length )
{
  libspectrum_microdrive_block b;
  libspectrum_byte label[10];
  libspectrum_byte n;
  int e, nolabel;
  
  if( length < LIBSPECTRUM_MICRODRIVE_BLOCK_LEN * 10 ||
     ( length % LIBSPECTRUM_MICRODRIVE_BLOCK_LEN ) > 1 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_microdrive_mdr_read: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  length = length > MDR_LENGTH ? MDR_LENGTH : length;
  
  memcpy( microdrive->data, buffer, length ); buffer += length;

  if( ( length % LIBSPECTRUM_MICRODRIVE_BLOCK_LEN ) == 1 )
    libspectrum_microdrive_set_write_protect( microdrive, *buffer );
  else
    libspectrum_microdrive_set_write_protect( microdrive, 0 );

  libspectrum_microdrive_set_cartridge_len( microdrive,
				length / LIBSPECTRUM_MICRODRIVE_BLOCK_LEN );
 
  n = libspectrum_microdrive_cartridge_len( microdrive );
  nolabel = 1;	/* No real label ! */
  
  while( n > 0 ) {
    n--;
    if( ( e = libspectrum_microdrive_checksum( microdrive, n ) ) > 0 ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
        "libspectrum_microdrive_mdr_read: %s checksum error in #%d record",
	e == 1 ? "record header" : e == 2 ? "data header" : "data",
	n
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    libspectrum_microdrive_get_block( microdrive, 0, &b );

    if( !nolabel && memcmp( label, b.hdbnam, 10 ) ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
        "libspectrum_microdrive_mdr_read: inconsistent labels in #%d record",
	n
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    if( e == 0 && nolabel ) {
      memcpy( label, b.hdbnam, 10 );
      nolabel = 0;
    }
  }
  
  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_microdrive_mdr_write( const libspectrum_microdrive *microdrive,
				  libspectrum_byte **buffer, size_t *length )
{
  *buffer = malloc( *length = microdrive->cartridge_len * 
				    LIBSPECTRUM_MICRODRIVE_BLOCK_LEN + 1 );
  if( !*buffer ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_MEMORY,
      "libspectrum_microdrive_mdr_write: out of memory at %s:%d", __FILE__,
      __LINE__
    );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  memcpy( *buffer, microdrive->data, *length );

  
  (*buffer)[ *length ] = microdrive->write_protect;

  return LIBSPECTRUM_ERROR_NONE;
}
