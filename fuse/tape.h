/* tape.h: tape handling routines
   Copyright (c) 1999-2004 Philip Kendall

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
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_TAPE_H
#define FUSE_TAPE_H

#include <libspectrum.h>

int tape_init( void );

int tape_open( const char *filename, int autoload );
int tape_open_default_autoload( const char *filename );

int
tape_read_buffer( unsigned char *buffer, size_t length, libspectrum_id_t type,
		  const char *filename, int autoload );

int tape_close( void );
int tape_select_block( size_t n );
int tape_select_block_no_update( size_t n );
int tape_get_current_block( void );
int tape_write( const char *filename );

int tape_load_trap( void );
int tape_save_trap( void );

int tape_play( void );
int tape_toggle_play( void );

int tape_next_edge( libspectrum_dword last_tstates );

int tape_stop( void );

/* Call a user-supplied function for every block in the current tape */
int
tape_foreach( void (*function)( libspectrum_tape_block *block,
				void *user_data),
	      void *user_data );

/* Fill 'buffer' with up a maximum of 'length' characters of
   information about 'block' */
int tape_block_details( char *buffer, size_t length,
			libspectrum_tape_block *block );

extern int tape_microphone;
extern int tape_modified;

#endif
