/* listbasic: extract the BASIC listing from a snapshot or tape file
   Copyright (c) 2002 Chris Cowley
                 2003 Philip Kendall

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

   Philip: pak21-fuse@srcf.ucam.org
     Post: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

    Chris: ccowley@grok.co.uk

*/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <libspectrum.h>

#include "utils.h"

/* argv[0] */
char *progname;

/* A function used to read memory */
typedef
  libspectrum_byte (*memory_read_fn)( libspectrum_word offset, void *data );

int parse_snapshot_file( const unsigned char *buffer, size_t length,
			 libspectrum_id_t type );
libspectrum_byte read_snap_memory( libspectrum_word address, void *data );

int parse_tape_file( const unsigned char *buffer, size_t length,
		     libspectrum_id_t type );
libspectrum_byte read_tape_block( libspectrum_word offset, void *data );

int extract_basic( libspectrum_word offset,
		   memory_read_fn get_byte, void *data );
int detokenize( libspectrum_word offset, int length,
		memory_read_fn get_byte, void *data );

int main(int argc, char* argv[])
{
  int error;
  unsigned char *buffer; size_t length;
  libspectrum_id_t type;

  progname = argv[0];

  if( argc != 2 ) {
    fprintf( stderr, "%s: usage: %s <file>\n", progname, progname );
    return 1;
  }

  error = mmap_file( argv[1], &buffer, &length ); if( error ) return error;

  error = libspectrum_identify_file( &type, argv[1], buffer, length );
  if( error ) { munmap( buffer, length ); return error; }

  switch( type ) {

  case LIBSPECTRUM_ID_SNAPSHOT_SNA:
  case LIBSPECTRUM_ID_SNAPSHOT_Z80:
    error = parse_snapshot_file( buffer, length, type );
    if( error ) { munmap( buffer, length ); return error; }
    break;

  case LIBSPECTRUM_ID_TAPE_TAP:
  case LIBSPECTRUM_ID_TAPE_TZX:
    error = parse_tape_file( buffer, length, type );
    if( error ) { munmap( buffer, length ); return error; }
    break;

  case LIBSPECTRUM_ID_UNKNOWN:
    fprintf( stderr, "%s: couldn't identify the file type of `%s'\n",
	     progname, argv[1] );
    munmap( buffer, length );
    return 1;

  default:
    fprintf( stderr, "%s: `%s' is an unsupported file type\n",
	     progname, argv[1] );
    munmap( buffer, length );
    return 1;

  }

  error = munmap( buffer, length );
  if( error ) {
    fprintf( stderr, "%s: couldn't munmap `%s': %s\n", progname, argv[1],
	     strerror( errno ) );
    return 1;
  }

  return 0;
}

int
parse_snapshot_file( const unsigned char *buffer, size_t length,
		     libspectrum_id_t type )
{
  libspectrum_snap *snap;
  libspectrum_word program_address;

  int error;

  error = libspectrum_snap_alloc( &snap ); if( error ) return error;

  error = libspectrum_snap_read( snap, buffer, length, type, NULL );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  program_address =
    read_snap_memory( 23635, snap ) | read_snap_memory( 23636, snap ) << 8;

  if( program_address < 23296 || program_address > 65530 ) {
    fprintf( stderr, "%s: invalid program start address 0x%04x\n", progname,
	     program_address );
    libspectrum_snap_free( snap );
    return 1;
  }

  error = extract_basic( program_address, read_snap_memory, snap );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  error = libspectrum_snap_free( snap ); if( error ) return error;

  return 0;
}

libspectrum_byte
read_snap_memory( libspectrum_word address, void *data )
{
  libspectrum_snap *snap = data;

  /* FIXME: assumes a 48K memory model */

  switch( address >> 14 ) {
    
  case 0: /* ROM; can't handle this */
    fprintf( stderr, "%s: attempt to read from ROM\n", progname );
    /* FIXME: find a better way to handle the error return */
    exit( 1 );

  case 1:
    return snap->pages[5][ address & 0x3fff ];

  case 2:
    return snap->pages[2][ address & 0x3fff ];

  case 3:
    return snap->pages[0][ address & 0x3fff ];

  default: /* Should never happen */
    fprintf( stderr, "%s: attempt to read from address %x\n", progname,
	     address );
    abort();

  }
}

int
parse_tape_file( const unsigned char *buffer, size_t length,
		 libspectrum_id_t type )
{
  libspectrum_tape *tape;
  GSList *block;
  libspectrum_tape_block *tape_block;
  libspectrum_tape_rom_block *rom_block;
  libspectrum_word program_length;

  int error;

  error = libspectrum_tape_alloc( &tape ); if( error ) return error;

  error = libspectrum_tape_read( tape, buffer, length, type, NULL );
  if( error ) { libspectrum_tape_free( tape ); return error; }

  for( block = tape->blocks; block; block = block->next ) {

    /* Find a ROM block */
    tape_block = block->data;
    if( tape_block->type != LIBSPECTRUM_TAPE_BLOCK_ROM ) continue;

    /* Start assuming this block is a BASIC header; firstly, check
       there is another block after this one to hold the data, and
       if there's not, just finish */
    if( !block->next ) break;

    rom_block = &tape_block->types.rom;

    /* If it's a header, it must be 19 bytes long */
    if( rom_block->length != 19 ) continue;

    /* The first byte should be zero to indicate a header */
    if( rom_block->data[0] != 0 ) continue;

    /* The second byte should be zero to indicate a BASIC program */
    if( rom_block->data[1] != 0 ) continue;

    /* The program length is stored at offset 16 */
    program_length = rom_block->data[16] | rom_block->data[17] << 8;

    /* Now have a look at the next block */
    tape_block = block->next->data;

    /* Must be a ROM block */
    if( tape_block->type != LIBSPECTRUM_TAPE_BLOCK_ROM ) continue;

    rom_block = &tape_block->types.rom;

    /* Must be at least as long as the program */
    if( rom_block->length < program_length ) continue;

    /* Must be a data block */
    if( rom_block->data[0] != 0xff ) continue;

    /* Now, just read the BASIC out */
    error = extract_basic( 1, read_tape_block, rom_block );
    if( error ) { libspectrum_tape_free( tape ); return error; }

    /* Don't parse this block again */
    block = block->next;
  }

  error = libspectrum_tape_free( tape ); if( error ) return error;

  return 0;
}

libspectrum_byte
read_tape_block( libspectrum_word offset, void *data )
{
  libspectrum_tape_rom_block *block = data;

  if( offset > block->length ) {
    fprintf( stderr, "%s: attempt to read past end of block\n", progname );
    exit( 1 );
  }

  return block->data[ offset ];
}
  
int
extract_basic( libspectrum_word offset, memory_read_fn get_byte, void *data )
{
  int line_number, line_length;
  int error;

  while( 1 ) {

    line_number = get_byte( offset, data ) << 8 | get_byte( offset + 1, data );
    offset += 2;
    if( line_number > 16384 ) break;

    printf( "%d", line_number );

    line_length = get_byte( offset, data ) | get_byte( offset + 1, data ) << 8;
    offset += 2;

    error = detokenize( offset, line_length, get_byte, data );
    if( error ) return error;

    printf( "\n" );

    offset += line_length;
  }

  return 0;
}

int
detokenize( libspectrum_word offset, int length,
	    memory_read_fn get_byte, void *data )
{
  int i;
  libspectrum_byte b;

  for( i = 0; i < length; i++ ) {

    b = get_byte( offset + i, data );

    switch( b ) {

    case 14: i += 5; break;		/* Skip encoded number */

    /* Skip encoded INK, PAPER, FLASH, BRIGHT, INVERSE< OVER */
    case 16: case 17: case 18: case 19: case 20: case 21:
      i++; break;

    case 22: case 23:			/* Skip encoded AT, TAB */
      i += 2; break;

    case  92: printf( "\\\\" ); break;

    case 127: printf( "\\*" ); break;	/* (c) symbol */

    case 128: printf( "\\  " ); break;	/* Graphics characters */
    case 129: printf( "\\ '" ); break;
    case 130: printf( "\\' " ); break;
    case 131: printf( "\\''" ); break;
    case 132: printf( "\\ ." ); break;
    case 133: printf( "\\ :" ); break;
    case 134: printf( "\\'." ); break;
    case 135: printf( "\\':" ); break;
    case 136: printf( "\\. " ); break;
    case 137: printf( "\\.'" ); break;
    case 138: printf( "\\: " ); break;
    case 139: printf( "\\:'" ); break;
    case 140: printf( "\\.." ); break;
    case 141: printf( "\\.:" ); break;
    case 142: printf( "\\:." ); break;
    case 143: printf( "\\::" ); break;

    case 144: printf( "\\a" ); break;	/* UDGs */
    case 145: printf( "\\b" ); break;
    case 146: printf( "\\c" ); break;
    case 147: printf( "\\d" ); break;
    case 148: printf( "\\e" ); break;
    case 149: printf( "\\f" ); break;
    case 150: printf( "\\g" ); break;
    case 151: printf( "\\h" ); break;
    case 152: printf( "\\i" ); break;
    case 153: printf( "\\j" ); break;
    case 154: printf( "\\k" ); break;
    case 155: printf( "\\l" ); break;
    case 156: printf( "\\m" ); break;
    case 157: printf( "\\n" ); break;
    case 158: printf( "\\o" ); break;
    case 159: printf( "\\p" ); break;
    case 160: printf( "\\q" ); break;
    case 161: printf( "\\r" ); break;
    case 162: printf( "\\s" ); break;
    case 163: printf( "\\t" ); break;
    case 164: printf( "\\u" ); break;

    case 165: printf( "RND" ); break;
    case 166: printf( "INKEY$" ); break;
    case 167: printf( "PI" ); break;
    case 168: printf( "FN " ); break;
    case 169: printf( "POINT " ); break;
    case 170: printf( "SCREEN$ " ); break;
    case 171: printf( "ATTR " ); break;
    case 172: printf( "AT " ); break;
    case 173: printf( "TAB " ); break;
    case 174: printf( "VAL$ " ); break;
    case 175: printf( "CODE " ); break;
    case 176: printf( "VAL " ); break;
    case 177: printf( "LEN " ); break;
    case 178: printf( "SIN " ); break;
    case 179: printf( "COS " ); break;
    case 180: printf( "TAN " ); break;
    case 181: printf( "ASN " ); break;
    case 182: printf( "ACS " ); break;
    case 183: printf( "ATN " ); break;
    case 184: printf( "LN " ); break;
    case 185: printf( "EXP " ); break;
    case 186: printf( "INT " ); break;
    case 187: printf( "SQR " ); break;
    case 188: printf( "SGN " ); break;
    case 189: printf( "ABS " ); break;
    case 190: printf( "PEEK " ); break;
    case 191: printf( "IN " ); break;
    case 192: printf( "USR " ); break;
    case 193: printf( "STR$ " ); break;
    case 194: printf( "CHR$ " ); break;
    case 195: printf( "NOT " ); break;
    case 196: printf( "BIN " ); break;
    case 197: printf( " OR " ); break;
    case 198: printf( " AND " ); break;
    case 199: printf( "<=" ); break;
    case 200: printf( ">=" ); break;
    case 201: printf( "<>" ); break;
    case 202: printf( " LINE " ); break;
    case 203: printf( " THEN " ); break;
    case 204: printf( " TO " ); break;
    case 205: printf( " STEP " ); break;
    case 206: printf( " DEF FN " ); break;
    case 207: printf( " CAT " ); break;
    case 208: printf( " FORMAT " ); break;
    case 209: printf( " MOVE " ); break;
    case 210: printf( " ERASE " ); break;
    case 211: printf( " OPEN #" ); break;
    case 212: printf( " CLOSE #" ); break;
    case 213: printf( " MERGE " ); break;
    case 214: printf( " VERIFY " ); break;
    case 215: printf( " BEEP " ); break;
    case 216: printf( " CIRCLE " ); break;
    case 217: printf( " INK " ); break;
    case 218: printf( " PAPER " ); break;
    case 219: printf( " FLASH " ); break;
    case 220: printf( " BRIGHT " ); break;
    case 221: printf( " INVERSE " ); break;
    case 222: printf( " OVER " ); break;
    case 223: printf( " OUT " ); break;
    case 224: printf( " LPRINT " ); break;
    case 225: printf( " LLIST " ); break;
    case 226: printf( " STOP " ); break;
    case 227: printf( " READ " ); break;
    case 228: printf( " DATA " ); break;
    case 229: printf( " RESTORE " ); break;
    case 230: printf( " NEW " ); break;
    case 231: printf( " BORDER " ); break;
    case 232: printf( " CONTINUE " ); break;
    case 233: printf( " DIM " ); break;
    case 234: printf( " REM " ); break;
    case 235: printf( " FOR " ); break;
    case 236: printf( " GO TO " ); break;
    case 237: printf( " GO SUB " ); break;
    case 238: printf( " INPUT " ); break;
    case 239: printf( " LOAD " ); break;
    case 240: printf( " LIST " ); break;
    case 241: printf( " LET " ); break;
    case 242: printf( " PAUSE " ); break;
    case 243: printf( " NEXT " ); break;
    case 244: printf( " POKE " ); break;
    case 245: printf( " PRINT " ); break;
    case 246: printf( " PLOT " ); break;
    case 247: printf( " RUN " ); break;
    case 248: printf( " SAVE " ); break;
    case 249: printf( " RANDOMIZE " ); break;
    case 250: printf( " IF " ); break;
    case 251: printf( " CLS " ); break;
    case 252: printf( " DRAW " ); break;
    case 253: printf( " CLEAR " ); break;
    case 254: printf( " RETURN " ); break;
    case 255: printf( " COPY " ); break;

    default:
      if( b >= 32 ) printf( "%c", b ); break;

    }
  }

  return 0;
}
