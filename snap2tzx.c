/* snap2tzx.c: Snapshot to .tzx tape image converter
   Copyright (c) 1997-2001 ThunderWare Research Center, written by
                           Martijn van der Heide,
		 2003 Tomaz Kac,
		 2003 Philip Kendall

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

#include <config.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <libspectrum.h>

#include "utils.h"

char *progname;				/* argv[0] */

libspectrum_snap *snap;
char filename[512];
char out_filename[512];
char loader_name[9] = "";

char game_name[33]="                                ";
char info1[33]="                                ";
char info2[33]="                                ";

int snap_type;				// -1 = Not recognised
							//  0 = Z80
int snap_len;
libspectrum_byte snap_bin[256000];
libspectrum_byte WorkBuffer[65535];

int verbose = 0;
int speed_value = 3;
int load_colour = -1;
int external = 0;
char external_filename[512];
libspectrum_byte bright = 0;

void print_error(char * text)
{
	printf("\n-- ERROR : %s\n",text);
}

void print_verbose(char * text)
{
	if (verbose)
	{
		printf("%s\n",text);
	}
}

static void crunch_z80 (libspectrum_byte *BufferIn, libspectrum_word BlLength, libspectrum_byte *BufferOut, libspectrum_word *CrunchedLength)

/**********************************************************************************************************************************/
/* Pre   : `BufferIn' points to the uncrunched spectrum block, `BlLength' holds the length, `BufferOut' points to the result.     */
/*         `CrunchedLength' is the length after crunching.                                                                        */
/*         The version of Z80 file must have been determined.                                                                     */
/* Post  : The block has been crunched from `BufferIn' to `BufferOut'.                                                            */
/*         If the crunched size is larger than (or equal to) the non-crunched size, the block is returned directly with           */
/*         `CrunchedLength' set to 0x0000.                                                                                        */
/*         The crunched block is returned reversed (for backward loading).                                                        */
/* Import: None.                                                                                                                  */
/**********************************************************************************************************************************/

{
  libspectrum_word register IndexIn;
  libspectrum_word register IndexOut       = 0;
  libspectrum_word          LengthOut;
  libspectrum_word register RunTimeLength;
  int          CrunchIt       = 0;
  libspectrum_byte          RepeatedCode;
  libspectrum_byte          PrevByte       = 0x00;
  libspectrum_byte         *BufferTemp;

  

  BufferTemp	= WorkBuffer + (libspectrum_word)32768;                                                        /* Use the top 16Kb of Workbuffer */
  for (IndexIn = 0 ; IndexIn < BlLength - 4 ; IndexIn ++)                                     /* Crunch it into the second buffer */
  {
    if (*(BufferIn + IndexIn) == 0xED &&                                                                     /* Exceptional cases */
        *(BufferIn + IndexIn + 1) == 0xED)
      CrunchIt = 1;
    else if (*(BufferIn + IndexIn) == *(BufferIn + IndexIn + 1) &&
             *(BufferIn + IndexIn) == *(BufferIn + IndexIn + 2) &&
             *(BufferIn + IndexIn) == *(BufferIn + IndexIn + 3) &&
             *(BufferIn + IndexIn) == *(BufferIn + IndexIn + 4) &&                                        /* At least 5 repeats ? */
             PrevByte != 0xED)                                                                         /* Not right after a 0xED! */
      CrunchIt = 1;
    if (CrunchIt)                                                                                             /* Crunch a block ? */
    {
      CrunchIt = 0;
      RunTimeLength = 1;
      RepeatedCode = *(BufferIn + IndexIn);
      while (RunTimeLength < 255 && RunTimeLength + IndexIn < BlLength && *(BufferIn + RunTimeLength + IndexIn) == RepeatedCode)
        RunTimeLength ++;
      *(BufferTemp + (IndexOut ++)) = 0xED;
      *(BufferTemp + (IndexOut ++)) = 0xED;
      *(BufferTemp + (IndexOut ++)) = (libspectrum_byte) RunTimeLength;
      *(BufferTemp + (IndexOut ++)) = RepeatedCode;
      IndexIn += RunTimeLength - 1;
      PrevByte = 0x00;
    }
    else
      PrevByte = *(BufferTemp + (IndexOut ++)) = *(BufferIn + IndexIn);                                          /* No, just copy */
  }
  while (IndexIn < BlLength)
    *(BufferTemp + (IndexOut ++)) = *(BufferIn + (IndexIn ++));                                                 /* Copy last ones */
  if (IndexOut >= BlLength)                                                              /* Compressed larger than uncompressed ? */
  {
    memcpy (BufferTemp, BufferIn, BlLength);                                                /* Return the inputblock uncompressed */
    LengthOut = BlLength;
    IndexOut = 0x0000;                                                                                  /* Signal: not compressed */
  }
  else
    LengthOut = IndexOut;
  BufferTemp = WorkBuffer + (libspectrum_word)32768 + LengthOut - 1;                                       /* Point to the last crunched byte */
  for (IndexIn = 0 ; IndexIn < LengthOut ; IndexIn ++)                             /* Now reverse the result for backward loading */
    *(BufferOut + IndexIn) = *(BufferTemp - IndexIn);
  *CrunchedLength = IndexOut;
}

/*
int TestDecZ80(libspectrum_byte *s, libspectrum_byte *d, int l, int ll)
{
// Decompresses a portion of .Z80 file pointed to by s to d, the destination
// length is in l , and return 1 if it can stay decompressed (it never overlaps
// with the source) ll is the length of source
int n=0;
int m=0;
int o;
int over;

over=1;
while (n<l)
	{
	if (s[m]==0xED && s[m+1]==0xED)
		{
		for (o=0; o<s[m+2]; o++)
			d[n++]=s[m+3];
		m+=4;
		}
	else
		d[n++]=s[m++];
	if (((l-ll)+m)<n) over=0;
	}
return(over);
}
*/

int test_decz80(libspectrum_byte *source, int final_len, int source_len)
{
	// source is not reversed !!!

	int dst_pnt = 0;
	int src_pnt = final_len-source_len;

	int overwrite = 0;

	libspectrum_byte b1;
	libspectrum_byte b2;
	libspectrum_byte times;
	libspectrum_byte value;

	int off = src_pnt;

	while (dst_pnt < final_len && !overwrite)
	{
		b1 = source[src_pnt-off];
		src_pnt++;
		b2 = source[src_pnt-off];

		if (b1 == 0xED && b2 == 0xED)
		{
			// Repeat
			times = source[(src_pnt-off)+1];
			value = source[(src_pnt-off)+2];
			dst_pnt += times;
			src_pnt+=3;
		}
		else
		{
			dst_pnt++;
		}

		if (dst_pnt > src_pnt)
		{
			overwrite = 1;
		}
	}
	return overwrite;
}

int test_rev_decz80(libspectrum_byte *source, int final_len, int source_len)
{
	// source is reversed !!!

	int dst_pnt = final_len-1;
	int src_pnt = source_len-1;

	int overwrite = 0;

	libspectrum_byte b1;
	libspectrum_byte b2;
	libspectrum_byte times;
	libspectrum_byte value;

	while (dst_pnt >=0 && !overwrite)
	{
		b1 = source[src_pnt];
		src_pnt--;
		b2 = source[src_pnt];

		if (b1 == 0xED && b2 == 0xED)
		{
			// Repeat
			times = source[src_pnt-1];
			value = source[src_pnt-2];
			dst_pnt -= times;
			src_pnt-=3;
		}
		else
		{
			dst_pnt--;
		}

		if (dst_pnt < src_pnt)
		{
			overwrite = 1;
		}
	}
	return overwrite;
}

static void reverse_block (libspectrum_byte *BufferOut, libspectrum_byte *BufferIn)
{
  libspectrum_word register  Cnt;
  libspectrum_byte          *BIn;
  libspectrum_byte          *BOut;

  for (BIn = BufferIn, BOut = BufferOut + 16383, Cnt = 0 ; Cnt < 16384 ; Cnt ++)
    *(BOut --) = *(BIn ++);
}
 
void create_out_filename()
{
	strcpy(out_filename, filename);
	int last = strlen(out_filename)-1;
	while (out_filename[last] != '.' && last > 0)
	{
		last--;
	}
	if (last == 0)
	{
		// No extension ???
		return;
	}
	out_filename[last+1]='t';
	out_filename[last+2]='z';
	out_filename[last+3]='x';
	out_filename[last+4]=0;
}

char * get_file_only(char *path)
{
	int pos = strlen(path)-1;
	while (pos>=0 && path[pos] != '\\' && path[pos] != '/')
	{
		pos--;
	}
	if (pos == 0)
	{
		return path;
	}
	else
	{
		return (&path[pos+1]);
	}
}

void create_loader_name()
{
	char * skip = get_file_only(filename);
	int i;

	int len = strlen(skip);
	if (len > 8)
	{
		len = 8;
	}
	for (i=0; i < len && skip[i] != '.'; i++)
	{
		loader_name[i] = skip[i];
	}
	loader_name[len]=0;
}

void center_name(char name[])
{
        int i;

	for (i=31; i >=0 && name[i]==' '; i--);
	int num = 31-i;

	if (num>1)
	{
		for (i=31; i >= num/2; i--)
		{
			name[i] = name[i-(num/2)];
		}
		for (i=0; i < num/2; i++)
		{
			name[i]=' ';
		}
	}
}

static void
create_game_name( void )
{
  char *skip = get_file_only( filename );

  int fl = strlen( skip );

  int pos = 0;
  int posf = 0;

  while( pos < 32 && posf < fl && skip[posf] != '.' ) {

    if( skip[posf] == '_' ) {

      game_name[pos] = ' ';

    } else {

      game_name[pos] = skip[posf];
      if( islower( skip[posf] ) && 
	  ( isdigit( skip[posf+1] ) || isupper( skip[posf+1] ) ||
	    skip[posf+1] == '('     || skip[posf+1] == '+'       
          )
        ) {
	pos++;
	game_name[pos]=' ';
      }

    }
		
    pos++;
    posf++;

  }
}

void print_usage(int title)
{
	if (title)
	{
		printf("\nZ80 Snapshot to TZX Tape Converter  v1.0\n");
		printf(  "\n  ->by Tom-Cat<-\n\n");
	}
	printf("Usage:\n\n");
	printf("  Z802TZX Filename.z80 [Options]\n\n");
	printf("  Options:\n");
	printf("  -v    Verbose Output (Info on conversion)\n");
	printf("  -s n  Loading Speed (n: 0=1500  1=2250  2=3000  3=6000 bps) Default: 3\n");
	printf("  -b n  Border (0=Black 1=Blue 2=Red 3=Magenta 4=Green 5=Cyan 6=Yellow 7=White)\n");
	printf("  -r    Use Bright Colour when filling in the last attribute line\n");
	printf("  -$ f  Use External Loading Screen in file f (.scr 6912 bytes long file)\n");
	printf("  -o f  Use f as the Output File (Default: Input File with .tzx extension)\n");
	printf("  -l s  Use String s as the ZX Loader Name (Loading: name) Up To 8 Chars!\n");
	printf("  -g s  Use String s as the Game Name (Shown when Loading starts)\n");
	printf("  -i1 s Show a line of info when loading (first line)\n");
	printf("  -i2 s Show another line of info when loading (second line)\n");
	printf("\nStrings (s) can be Up To 32 chars long. Use '~' as (C) char.If no Border Colour\n"); 
	printf("is selected then it will be gathered from the snapshot file. Loading and Game\n");
	printf("Name will be taken from the Filename if you don't use the -l or -g parameters!\n");
}

void change_copyright(char * st)
{
	int len = strlen(st), jj;
	for (jj=0; jj < len; jj++)
	{
		if (st[jj] == '~')
		{
			st[jj] = 0x7f;
		}
	}
}

static int
parse_args( int argc, char **argv )
{
  int c, got_game_name, got_loader_name, got_out_filename;

  got_game_name = got_loader_name = got_out_filename = 0;

  while( ( c = getopt( argc, argv, "1:2:b:g:l:o:rs:v$:" ) ) != EOF ) {
    switch (c) {

    case '1':
      snprintf( info1, 33, "%-32s", optarg );
      change_copyright( info1 );
      center_name( info1 );
      break;

    case '2':
      snprintf( info2, 33, "%-32s", optarg );
      change_copyright( info2 );
      center_name( info2 );
      break;

      /* Border Colour when loading */
    case 'b':
      load_colour = atoi( optarg );
      if( load_colour < 0 || load_colour > 7 ) {
	print_error( "Invalid Border Colour!" );
	return 1;
      }
      break;

      /* Game Name (When Loader is loaded) */
    case 'g':
      snprintf( game_name, 33, "%-32s", optarg );
      change_copyright( game_name );
      center_name( game_name );
      got_game_name = 1;
      break;

      /* Loader Name (Loading: Name) */
    case 'l':
      strncpy( loader_name, optarg, 8 );
      loader_name[8] = 0;
      got_loader_name = 1;
      break;
      
      /* Output Filename */
    case 'o':
      strncpy( out_filename, optarg, 511 );
      out_filename[511] = 0;
      got_out_filename = 1;
      break;

    case 'r': bright = 0x40; break;

      /* Speed value  */
    case 's':
      speed_value=atoi( optarg );
      if( speed_value < 0 || speed_value > 3 ) {
	print_error( "Invalid Speed Value!" );
	return 1;
      }
      break;

      /* Verbose output */
    case 'v': verbose = 1; break;

      /* External Loading Screen */
    case '$':
      external = 1;
      strncpy( external_filename, optarg, 511 );
      external_filename[511] = 0;
      break;

    case '?':
      print_error( "Unknown Option\n" );
      print_usage( 0 );
      return 1;

    }
  }

  if( argv[optind] == NULL ) {
    print_usage( 1 );
    return 1;
  }

  // Snapshot filename
  strcpy( filename, argv[optind] );
  if( !got_out_filename ) create_out_filename();
  if( !got_loader_name ) create_loader_name();
  if( !got_game_name ) create_game_name();
  
  return 0;
}

static int
load_snap( void )
{
  int error;
  unsigned char *buffer; size_t length;

  error = libspectrum_snap_alloc( &snap ); if( error ) return error;

  if( mmap_file( filename, &buffer, &length ) ) {
    libspectrum_snap_free( snap );
    return 1;
  }

  error = libspectrum_snap_read( snap, buffer, length, LIBSPECTRUM_ID_UNKNOWN,
				 filename );
  if( error ) {
    libspectrum_snap_free( snap ); munmap( buffer, length );
    return error;
  }

  if( munmap( buffer, length ) == -1 ) {
    fprintf( stderr, "%s: couldn't munmap '%s': %s\n", progname, filename,
	     strerror( errno ) );
    libspectrum_snap_free( snap );
    return 1;
  }

  return 0;
}

#define LOADERPREPIECE  (224+1)-87+12-1+41+41                                                  /* Length of the BASIC part before the loader code */
static libspectrum_byte SpectrumBASICData[LOADERPREPIECE] = {

	/* 0 {INK 255}{PAPER 255}BORDER PI-PI : PAPER PI-PI : INK PI-PI : CLS
		PRINT "{AT 6,0}{INK 5}
		{AT 12,9}{INK 6}{PAPER 2}{FLASH 1} NOW LOADING {AT 0,0}{FLASH 0}{PAPER 0}{INK 0}":
		RANDOMIZE USR (PEEK VAL "23627"+VAL "256"*PEEK VAL "23628") */
	"\x00\x00\xFF\xFF\x10\xFF\x11\xFF\xE7\xA7\x2D\xA7\x3A\xDA\xA7\x2D\xA7\x3A\xD9\xA7\x2D\xA7\x3A\xFB\x3A"
	"\xF5\"\x16\x06\x00\x10\x05""                                "
	"\x16\x0B\x0A\x10\x06\x11\x02\x12\x01 IS LOADING \x16\x00\x00\x12\x00\x11\x00\x10\x00\""
	"\x3A\xF5\"\x16\x13\x00\x10\x04""                                ""\""
	"\x3A\xF5\"\x16\x15\x00\x10\x04""                                ""\""
	"\x3A\xF9\xC0(\xBE\xB0\"23627\"+\xB0\"256\"*\xBE\xB0\"23628\")\x0D"

	/* Variables area - the 768 byte loader block is	appended after this piece */
	"\xCD\x52\x00"         /* VARSTART  CALL 0052,RET_INSTR   ; Determine on which address we are now             */
	"\x3B"                 /* RETRET    DEC  SP                                                                   */
	"\x3B"                 /*           DEC  SP                                                                   */
	"\xE1"                 /*           POP  HL                                                                   */
	"\x01\x12\x00"         /*           LD   BC,0012                                                              */
	"\x09"                 /*           ADD  HL,BC            ; Find start of the 768 byte loader                 */
	"\x11\x00\xBD"         /*           LD   DE,+BD00                                                             */
	"\x01\x00\x03"         /*           LD   BC,+0300                                                             */
	"\xED\xB0"             /*           LDIR                  ; Move it into place                                */
	"\xC3\x00\xBD" };      /*           JP   BD00,LDSTART     ; And run it!                                       */

/**********************************************************************************************************************************/
/* The custom loader. Any speed can be handled by it.                                                                             */
/* Notice that the EAR bit value is NOT rotated as opposed to the ROM and is kept at 0x40!                                        */
/* ROM speed: a = 22, b = 25 (bd = 27) => delay = 32*22 = 704, sampler = 59*25 = 1475 => delay/sampler = 1/2                      */
/* Max speed: a = 1, b = 0 (bd = 2) => 279 + 32 = 311 (hb0 = 156 T) => 468 average bit => 7479 bps                                */
/* (see below) The pilot pulse must be between 1453 + 16a T and 3130 + 16a T (a=known)                                            */
/* (see below) The first sync pulse must be 840 + 16a T maximum (a=known)                                                         */
/* Average bit length = 3,500,000 / speed : (a=x, b=2x - so delay/sampler is kept at 1/2)                                         */
/*  1364 bps => 2566 T => hb0 = 855 T, hb1 = 1710 T, avg = 1283 (2565) T       <-- ROM speed                                      */
/*              279 + 32a + 43b = 2565 => 32x + 86x = 2286 => 118x = 2286 => x = 19.4                                             */
/*              279 + 32*20 + 43*39 = 2596 (2565 needed), so a = 20, b = 39, bd = 41                                              */
/*              PilotMin = 1453 + 16a = 1453 + 16*20 = 1773 T                                                                     */
/*              PilotMax = 3130 + 16a = 3130 + 16*20 = 3450 T => 2168 T                                                           */
/*              Sync0 = 840 + 16a = 840 + 16*20 = 1160 T => 667 T                                                                 */
/*  2250 bps => 1556 T => hb0 = 518 T, hb1 = 1036 T, avg = 777 (1554) T                                                           */
/*              279 + 32a + 43b = 1554 => 32x + 86x = 1275 => 118x = 1275 => x = 10.8                                             */
/*              279 + 32*11 + 43*22 = 1577 (1554 needed), so a = 11, b = 22, bd = 24                                              */
/*              PilotMin = 1453 + 16a = 1453 + 16*11 = 1629 T                                                                     */
/*              PilotMax = 3130 + 16a = 3130 + 16*11 = 3306 T => 2000 T                                                           */
/*              Sync0 = 840 + 16a = 840 + 16*11 = 1016 T => 600 T                                                                 */
/*  3000 bps => 1167 T => hb0 = 389 T, hb1 = 778 T, avg = 584 (1167) T                                                            */
/*              279 + 32a + 43b = 1167 => 32x + 86x = 888 => 118x = 888 => x = 7.5                                                */
/*              279 + 32*7 + 43*16 = 1191 (1167 needed), so a = 7, b = 16, bd = 18                                                */
/*              PilotMin = 1453 + 16a = 1453 + 16*7 = 1565 T                                                                      */
/*              PilotMax = 3130 + 16a = 3130 + 16*7 = 3242 T => 1900 T                                                            */
/*              Sync0 = 840 + 16a = 840 + 16*7 = 952 T => 550 T                                                                   */
/*  6000 bps => 584 T => hb0 = 195 T, hb1 = 390 T, avg = 293 (585) T                                                              */
/*              279 + 32a + 43b = 585 => 32x + 86x = 306 => 118x = 306 => x = 2.6                                                 */
/*              279 + 32*3 + 43*5 = 590 (585 needed), so a = 3, b = 5, bd = 7                                                     */
/*              PilotMin = 1453 + 16a = 1453 + 16*3 = 1501 T                                                                      */
/*              PilotMax = 3130 + 16a = 3130 + 16*3 = 3178 T => 1700 T                                                            */
/*              Sync0 = 840 + 16a = 840 + 16*3 = 888 T => 450 T                                                                   */
/**********************************************************************************************************************************/

static struct TurboLoadVars
{
  /* Values as stored inside the TurboLoader code */
  libspectrum_byte      _Compare;    /* Variable 'bd' + starting value (+80) */
  libspectrum_byte      _Delay;      /* Variable 'a' */
  /* Timing values as stored in the TZX block header (unless ROM speed) */
  libspectrum_word      _LenPilot;   /* Number of pilot pulses is calculated to make the pilot take exactly 1 second */
                         /* This is just enough for the largest possible block to decompress and sync again */
  libspectrum_word      _LenSync0;   /* Both sync values are made equal */
  libspectrum_word      _Len0;       /* A '1' bit gets twice this value */
} turbo_vars[4] = {{ 0x80 + 41, 20, 2168, 667, 855 },   /*  1364 bps - uses the normal ROM timing values! */
                   { 0x80 + 24, 11, 2000, 600, 518 },   /*  2250 bps */
                   { 0x80 + 18,  7, 1900, 550, 389 },   /*  3000 bps */
                   { 0x80 +  7,  3, 1700, 450, 195 }};  /*  6000 bps */

/*
  COMPARE is on 94
  DELAY   is on 118
  XOR COL is on 134
*/

libspectrum_byte turbo_loader[144+1]   = 
  { "\xCD\x5B\xBF"        /* LD_BLOCK  CALL LD_BYTES         */
    "\xD8"                /*           RET  C                */
    "\xCF"                /*           RST  0008,ERROR_1     */
    "\x1A"                /*           DEFB +1A ; R Tape Loading Error */
    "\x14"                /* LD_BYTES  INC  D                */
    "\x08"                /*           EX   AF,AF'           */
    "\x15"                /*           DEC  D                */
    "\x3E\x08"            /*           LD   A,+08            (Border BLACK + MIC off) */
    "\xD3\xFE"            /*           OUT  (+FE),A          */
    "\xDB\xFE"            /*           IN   A,(+FE)          */
    "\xE6\x40"            /*           AND  +40              */
    "\x4F"                /*           LD   C,A              (BLACK/chosen colour) */
    "\xBF"                /*           CP   A                */
    "\xC0"                /* LD_BREAK  RET  NZ               */
    "\xCD\xCA\xBF"        /* LD_START  CALL LD_EDGE_1        */
    "\x30\xFA"            /*           JR   NC,LD_BREAK      */
    "\x26\x00"            /*           LD   H,+80            */

    /* Each subsequent 2 edges take:   65 + LD_EDGE_2 T = 283 + 32a + 43b T, bd = b + 2                      */
    /* Minimum:                                                                                              */
    /*   ROM => b = (+C6 - +9C - 2) + 1 = 41, a=22 => 427 + 32a + 59b T => 427 + 32*22 + 59*41 T = 3550 T    */
    /*          283 + 32a + 43b = 3432, a=20 => 283 + 32*20 + 43b = 3550 => 43b = 2627 => b = 61, bd = 63    */
    /*   b = 61, so each 2 leader pulses must fall within:                                                   */
    /*   283 + 32a + 43*61 T = 2906 + 32a T                                                                  */
    /*   One leader pulse must therefore be (2906 + 32a) / 2 = 1453 + 16a T minimum                          */
    /* Maximum, at timeout:                                                                                  */
    /*   ROM => b = +100 - +9C - 2 = 98, a=22 => 427 + 32a + 59b T => 427 + 32*22 + 59*98 T = 6913 T         */
    /*          283 + 32a + 43b = 6913, a=20 => 283 + 32*20 + 43b = 6913 => 43b = 6002 => b = 139, bd = 141  */
    /*   b = 139, so each 2 leader pulses must fall within:                                                  */
    /*   283 + 32a + 43*139 T = 6260 + 32a T                                                                 */
    /*   One leader pulse must therefore be (6260 + 32a) / 2 = 3130 + 16a T maximum                          */
    /* Notice that, as opposed to the ROM, we don't need the leader to be over a second. Only 256 pulses are */
    /* required (128 * 2 edges, counted in H).                                                               */

    "\x06\x75"            /* LD_LEADER LD   B,+73            7       */
    "\xCD\xC6\xBF"        /*           CALL LD_EDGE_2        17      */
    "\x30\xF1"            /*           JR   NC,LD_BREAK      7 (/12) */
    "\x3E\xB0"            /*           LD   A,+B2            7       */
    "\xB8"                /*           CP   B                4       */
    "\x30\xED"            /*           JR   NC,LD_START      7 (/12) */
    "\x24"                /*           INC  H                4       */
    "\x20\xF1"            /*           JR   NZ,LD_LEADER     7/12    */

    /* Each subsequent edge takes:   54 + LD_EDGE_1 T = 152 + 16a + 43b T, bd = b + 1                        */
    /* Sync0 maximum:                                                                                        */
    /*   ROM => b = +D4 - +C9 - 1 = 10, a=22 => 224 + 16a + 59b T => 224 + 16*22 + 59*10 T = 1166 T          */
    /*          152 + 16a + 43b = 1166, a=20 => 152 + 16*20 + 43b = 1166 => 43b = 694 => b = 16, bd = 17     */
    /*   b = 16, so the (first) sync pulse must be less than:                                                */
    /*   152 + 16a + 43*16 T = 840 + 16a T                                                                   */
    /* Leader maximum (at timeout):                                                                          */
    /*   ROM => b = +100 - +C9 - 1 = 56, a=22 => 224 + 16a + 59b T => 224 + 16*22 + 59*56 T = 3880 T         */
    /*          152 + 16a + 43b = 3880, a=20 => 152 + 16*20 + 43b = 3880 => 43b = 3408 => b = 79, bd = 80    */
    /*   Notice that this leader maximum is higher than above, so is not needed to be considered             */

    "\x06\xB0"            /* LD_SYNC   LD   B,+B0            7       */
    "\xCD\xCA\xBF"        /*           CALL LD_EDGE_1        17      */
    "\x30\xE2"            /*           JR   NC,LD_BREAK      7 (/12) */
    "\x78"                /*           LD   A,B              4       */
    "\xFE\xC1"            /*           CP   +C1              7       */
    "\x30\xF4"            /*           JR   NC,LD_SYNC       7/12    */

    "\xCD\xCA\xBF"        /*           CALL LD_EDGE_1        17      */
    "\xD0"                /*           RET  NC               5 (/11) */
    "\x26\x00"            /*           LD   H,+00            7       */
    "\x06\x80"            /*           LD   B,+80            7       */
    "\x18\x17"            /*           JR   LD_MARKER        12   = 48S */

    "\x08"                /* LD_LOOP   EX   AF,AF'           4       */
    "\x20\x05"            /*           JR   NZ,LD_FLAG       7/12 = 16F */
    "\xDD\x75\x00"        /*           LD   (IX+00),L        19      */
    "\x18\x09"            /*           JR   LD_NEXT          12   = 42D */

    "\xCB\x11"            /* LD_FLAG   RL   C                4       */
    "\xAD"                /*           XOR  L                4       */
    "\xC0"                /*           RET  NZ               5 (/11) */
    "\x79"                /*           LD   A,C              4       */
    "\x1F"                /*           RRA                   4       */
    "\x4F"                /*           LD   C,A              4       */
    "\x18\x03"            /*           JR   LD_FLNEXT        12   = 37F */

    "\xDD\x2B"            /* LD_NEXT   DEC  IX               10      (We're loading BACKWARD!) */
    "\x1B"                /*           DEC  DE               6       */
    "\x08"                /* LD_FLNEXT EX   AF,AF'           4       */
    "\x06\x82"            /*           LD   B,+82            7       */
    "\x2E\x01"            /* LD_MARKER LD   L,+01            7    = 34D/18F/7S */

    /* The very first (flag) bit takes (S) = 48 + 7 = 55 + LD_EDGE_2 T = 273 + 32a + 43b T                       */
    /* (D) 55 + 32 + 42 + 34 = 163, so                                                                           */
    /* Each first (data) bit takes: 163 + LD_EDGE_2 - 2b T = 295 + 32a + 43b T (ROM = 405 + 32a + 59b T)         */
    /* Each subsequent bit takes:   60 + LD_EDGE_2       T = 278 + 32a + 43b T (ROM = 422 + 32a + 59b T)         */

    "\xCD\xC6\xBF"        /* LD_8_BITS CALL LD_EDGE_2        17      */
    "\xD0"                /*           RET  NC               5 (/11) */
    "\x3E"                /*           LD   A,COMPARE        7    (variable 'bd') */

    "\x00"				// COMPARE !!!

    "\xB8"                /*           CP   B                4       */
    "\xCB\x15"            /*           RL   L                8       */
    "\x06\x80"            /*           LD   B,+80            7       */
    "\x30\xF3"            /*           JR   NC,LD_8_BITS     7/12 = 55D/60N */

    "\x7C"                /*           LD   A,H              4       */
    "\xAD"                /*           XOR  L                4       */
    "\x67"                /*           LD   H,A              4       */
    "\x7A"                /*           LD   A,D              4       */
    "\xB3"                /*           OR   E                4       */
    "\x20\xD3"            /*           JR   NZ,LD_LOOP       (7/) 12 = 32D */
    "\x7C"                /*           LD   A,H                      */
    "\xFE\x01"            /*           CP   +01                      */
    "\xC9"                /*           RET                           */

    /* The LD_EDGE_2 routine takes:  22 + 2 * (98 + 16a) + 43b T = 218 + 32a + 43b T (ROM = 362 + 32a + 59b T), bd = b + 2 */

    "\xCD\xCA\xBF"        /* LD_EDGE_2 CALL LD_EDGE_1        17      */
    "\xD0"                /*           RET  NC               5 (/11) */

    /* The LD_EDGE_1 routine takes:  98 + 16a + 43b T (ROM = 170 + 16a + 59b T, a=22), bd = b + 1 */

    "\x3E"                /* LD_EDGE_1 LD   A,DELAY          7    (variable 'a') */

    "\x00" // DELAY !!!

    "\x3D"                /* LD_DELAY  DEC  A                4       */
    "\x20\xFD"            /*           JR   NZ,LD_DELAY      7/12    */
    "\xA7"                /*           AND  A                4       */

    "\x04"                /* LD_SAMPLE INC  B                4    (variable 'b') */
    "\xC8"                /*           RET  Z                5 (/11) */
    "\xDB\xFE"            /*           IN   A,(+FE)          11      */
    "\xA9"                /*           XOR  C                4       */
    "\xE6\x40"            /*           AND  +40              7       */
    "\x28\xF7"            /*           JR   Z,LD_SAMPLE      7/12 = 43 */

    "\x79"                /*           LD   A,C              4       */
    "\xEE"                /*           XOR  COLOUR           7       */

    "\x00" // XOR COLOUR !!!

    "\x4F"                /*           LD   C,A              4       */
    "\xE6\x07"            /*           AND  +07              7       */
    "\xF6\x08"            /*           OR   +08              7       */
    "\xD3\xFE"            /*           OUT  (+FE),A          11      */
    "\x37"                /*           SCF                   4       */
    "\xC9" };            /*           RET                   10      */

libspectrum_byte tzx_start[10]={'Z','X','T','a','p','e','!',0x1a, 1, 13};

#define tzx_header_size  5+1+17+1
libspectrum_byte tzx_header[tzx_header_size] =
							{	0x10, 0x00, 0x00, 0x13, 0x00,
								0x00, 0x00,			// Basic
								0x11, 0x05, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,		// Filename
								0xff, 0xff,		// Length of Data block
								0x00, 0x00,		// Autostart line number
								0xCB, 0x00,		// Variable Area
								0xff};			// Checksum

libspectrum_byte tzx_header_data[6] = { 0x10, 0x00, 0x00, 0xff, 0xff, 0xff };

struct tzx_turbo_head_str
{
	libspectrum_byte id;
	libspectrum_word pilot;
	libspectrum_word sync1;
	libspectrum_word sync2;
	libspectrum_word zero;
	libspectrum_word one;
	libspectrum_word pilotlen;
	libspectrum_byte usedbits;
	libspectrum_word pause;
	libspectrum_word len1;
	libspectrum_byte len2;	// High byte of len is 0 !
	libspectrum_byte flag;
};

  typedef struct PageOrder_s     /* Each page as found in a Z80 file */
  {
    libspectrum_byte PageNumber;     /* Spectrum value, not Z80 value! (= +3) */
    libspectrum_word PageStart;      /* Normal (non-paged) start address of this 16Kb chunk */
  } PageOrder_s;	/* Notice that the 3 blocks for an 48K machine have different block numbers for a 48K and 128K snapshot! */

  struct PageOrder_s      PageOrder48S[4]           = {{255, 0xC000 },	// Loading screen (external)
														{ 0, 0x8000 },  // + With alternate loading screen
                                                        { 2, 0xC000 },  // +
                                                        { 5, 0x4000 }}; // + Must be decompressed !

  struct PageOrder_s      PageOrder128S[9]          = {{255, 0xC000 },	// Loading screen (external)
														{ 2, 0x8000 },  // +
                                                        { 1, 0xC000 },
                                                        { 3, 0xC000 },
                                                        { 4, 0xC000 },
                                                        { 6, 0xC000 },
                                                        { 7, 0xC000 },
                                                        { 0, 0xC000 },  // +
                                                        { 5, 0x4000 }}; // + Must be decompressed

  struct PageOrder_s      PageOrder48N[3]            = {{ 5, 0xC000 },  // + Loading screen from memory
                                                        { 2, 0x8000 },  // +
                                                        { 0, 0xC000 }}; // +

  struct PageOrder_s      PageOrder128N[8]           = {{ 5, 0xC000 },  // + Loading screen from memory
                                                        { 2, 0x8000 },  // +
                                                        { 1, 0xC000 },
                                                        { 3, 0xC000 },
                                                        { 4, 0xC000 },
                                                        { 6, 0xC000 },
                                                        { 7, 0xC000 },
                                                        { 0, 0xC000 }}; // +


libspectrum_byte tzx_turbo_head[20];

int load_page[8];
int load_768 = 0;

libspectrum_byte loader_data[768] = /* 768 */
{0xF3,0x31,0x00,0xC0,0xCD,0x91,0xBE,0xDD,0x21,0x6C,0xBE,0x21,0xBB,0xBB,0x11
,0xCC,0xCC,0x01,0xDD,0xDD,0xD9,0xFD,0x21,0xFF,0xFF,0xDD,0x7E,0x00,0xA7,0x28
,0x50,0xDD,0x66,0x01,0x2E,0xFF,0xDD,0x5E,0x02,0xDD,0x56,0x03,0x01,0xFD,0x7F
,0xED,0x79,0x01,0x04,0x00,0xDD,0x09,0xDD,0xE5,0xD5,0x7A,0xB3,0x20,0x0D,0x11
,0x00,0x40,0xDD,0x7E,0xFC,0xFE,0x12,0x20,0x03,0x11,0x00,0x3D,0xE5,0xDD,0xE1
,0x3E,0xAA,0x37,0xCD,0x55,0xBF,0xC1,0xDD,0xE5,0xE1,0x23,0xCD,0x44,0xBE,0x11
,0x01,0x00,0xFD,0x19,0x30,0x0B,0x21,0x00,0xC0,0x11,0x00,0x40,0x01,0x00,0x40
,0xED,0xB0,0xDD,0xE1,0x18,0xAA,0xFD,0x21,0xAA,0xAA,0x21,0xE0,0x5A,0x06,0x20
,0x3E,0xFF,0x77,0x23,0x10,0xFC,0x21,0xC8,0xBD,0x11,0xE0,0x57,0x0E,0x20,0xED
,0xB0,0x11,0xE0,0x56,0x0E,0x1F,0xED,0xB0,0x11,0xE0,0x55,0x0E,0x09,0xED,0xB0
,0x11,0xE0,0x54,0x0E,0x20,0xED,0xB0,0x11,0xE0,0x53,0x0E,0x14,0xED,0xB0,0x3E
,0xCC,0x3C,0x20,0x11,0x31,0xEA,0x52,0xDD,0x21,0x00,0xBD,0x11,0x00,0x03,0x3E
,0x55,0xBC,0x08,0xC3,0xE0,0x57,0x21,0x00,0xBD,0x11,0x01,0xBD,0x01,0xFF,0x02
,0x36,0x00,0xC3,0xE0,0x57,0xCD,0x62,0x05,0x31,0xE0,0x54,0x16,0x10,0x01,0xFD
,0xFF,0x15,0xED,0x51,0xF1,0x06,0xBF,0xED,0x79,0x7A,0xA7,0x20,0xF1,0x06,0xFF
,0x3E,0xBB,0xED,0x79,0xC3,0xE3,0x56,0xCD,0x62,0x05,0x31,0xE0,0x53,0xF1,0xD3
,0xFE,0xC1,0xF1,0xED,0x79,0xDD,0xE1,0xF1,0x08,0xE1,0x7C,0xED,0x47,0x7D,0xED
,0x4F,0xE1,0xD1,0xC1,0xF1,0xC3,0xE0,0x55,0x31,0xBB,0xAA,0xED,0x56,0xF3,0xC3
,0xDD,0xCC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x01,0xFD,0x7F,0x00,0x02,0x03,0x04,0x05,0x06,0x07
,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x7C,0xE6,0xC0,0x57,0x1E,0x00
,0x78,0xB1,0xC8,0x7E,0xFE,0xED,0x20,0x05,0x23,0xBE,0x28,0x05,0x2B,0xED,0xA0
,0x18,0xEF,0x23,0xC5,0x46,0x23,0x7E,0x23,0x12,0x13,0x10,0xFC,0xC1,0x0B,0x0B
,0x0B,0x0B,0x18,0xDE,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x21,0xFE,0x5A,0x36
,0x0D,0x23,0x36,0x0D,0x06,0x08,0x21,0xB4,0xBE,0xDD,0x21,0xFE,0x50,0x56,0xDD
,0x72,0x00,0x23,0x56,0xDD,0x72,0x01,0x23,0x11,0x00,0x01,0xDD,0x19,0x10,0xEF
,0xC9,0x00,0x00,0x7F,0xFE,0x48,0x02,0x08,0xF8,0x08,0x80,0x08,0x82,0x3E,0xFE
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00};
/* end binary data. size = 768 bytes */

int data_pos = 0;

void add_data(void * dat, int len)
{
	memcpy(snap_bin+data_pos, dat, len);
	data_pos+=len;
}

libspectrum_byte calc_checksum(libspectrum_byte data[], int len)
{
	libspectrum_byte xx = 0;
	int i;

	for (i=0; i < len; i++)
	{
		xx ^= data[i];
	}
	return xx;
}

void create_main_header()
{
  int len = ( LOADERPREPIECE - 1 ) + 768, i;

  // Fill in the name
  int tlen = strlen( loader_name );
  for( i=0; i < tlen; i++ )
    tzx_header[ 9 + i ] = loader_name[ i ];

  // Fill in the length of data
  tzx_header[17] = len & 0xff;
  tzx_header[18] = len >> 8;

  int var = ( LOADERPREPIECE - 1 ) - 21;	// Variables start here
  // Fill in proper variable area
  tzx_header[21] = var & 0xff;
  tzx_header[22] = var >> 8;

  // Calc checksum
  tzx_header[23] = calc_checksum( tzx_header + 5, 18 );
}

void set_loader_speed()
{
	turbo_loader[94] = turbo_vars[speed_value]._Compare;
	turbo_loader[118]= turbo_vars[speed_value]._Delay;
	libspectrum_byte xor_colour;
	if (load_colour == -1)
	{
		load_colour = libspectrum_snap_out_ula( snap ) & 0x07;
	}
    if (load_colour == 0x00)      
	{
		/* Border is going to be black ? */
		xor_colour = 0x41;                           /* Then use blue as counter colour (black/black cannot be seen....) */
	}
	else
	{
		xor_colour = 0x40 | load_colour;           /* Use the ultimate colour as counter colour for the loading stripes */
	}
	turbo_loader[134]= xor_colour;
}

#define POS_TABLE			0x16C		//  Loader table
#define POS_HL2				0x00C		//2 H'L'
#define POS_DE2				0x00F		//2 D'E'
#define POS_BC2				0x012		//2 B'C'
#define	POS_IY				0x071		//2 IY
#define POS_LAST_ATTR		0x079		//  Last Line Attribute value
#define POS_768_LOAD		0x0A5		//  FF = YES , 00 = NO
#define POS_48K_1			0x0B9		//  0x56 = 48k , 0x57 = 128k
#define POS_48K_2			0x0C7		//  -||-
#define POS_CLEAN_1			0x0C8		//3 ED,B0,00 = LDIR for Clean 768 !
#define POS_CLEAN_2			0x0E8		//3 -||-
#define POS_LAST_AY			0x0E2		//  Last AY out byte
#define POS_SP				0x108		//2 SP
#define POS_IM				0x10B		//  IM: 0=0x46 , 1=0x56 , 2=0x5E
#define POS_DIEI			0x10C		//  DI=0xF3 , EI=0xFB
#define POS_PC				0x10E		//2 PC
#define POS_AYREG			0x110		//2 16 AY registers, first byte = 00 !
#define POS_BORDER			0x131		//  Border Colour
#define POS_PAGE			0x135		//  Current 128 page (if 48k then 0x10)
#define POS_IX				0x136		//2 IX
#define POS_F2				0x138		//  F'
#define POS_A2				0x139		//  A'
#define POS_R				0x13A		//  R
#define POS_I				0x13B		//  I
#define POS_HL				0x13C		//2 HL
#define POS_DE				0x13E		//2 DE
#define POS_BC				0x140		//2 BC
#define POS_F				0x142		//  F
#define POS_A				0x143		//  A
		
void create_main_data()
{
  char tempc[256];

  int len = (LOADERPREPIECE-1) + 768 + 2, i, j;

  // Fill in the length of data
  tzx_header_data[3] = len & 0xff;
  tzx_header_data[4] = len >> 8;

  // Fill the loader with appropriate speed values and colour
  set_loader_speed();

  // Copy the loader to its position in the data
  memcpy(loader_data+341+256, turbo_loader, 144);

  data_pos = 0;
  add_data( tzx_header_data, 6 );

  // Copy the Game Name here and the info lines
  memcpy( SpectrumBASICData+32, game_name, 32 );
  memcpy( SpectrumBASICData+103, info1, 32 );
  memcpy( SpectrumBASICData+144, info2, 32 );

  add_data( SpectrumBASICData, LOADERPREPIECE-1 );

  int loader_start_pos = data_pos;	// Remember where the main loader
					// starts so we can fill the table
					// later

  int loader_table_pos = loader_start_pos + POS_TABLE;

  int pp = loader_start_pos;

  add_data( loader_data, 768 );

  int main_checksum = data_pos;

  data_pos++;

  // setup turbo header
  tzx_turbo_head[0] = 0x11;
  tzx_turbo_head[1] = turbo_vars[speed_value]._LenPilot&255;
  tzx_turbo_head[2] = turbo_vars[speed_value]._LenPilot>>8;
  tzx_turbo_head[3] = turbo_vars[speed_value]._LenSync0&255;
  tzx_turbo_head[4] = turbo_vars[speed_value]._LenSync0>>8;
  tzx_turbo_head[5] = turbo_vars[speed_value]._LenSync0&255;
  tzx_turbo_head[6] = turbo_vars[speed_value]._LenSync0>>8;
  tzx_turbo_head[7] = turbo_vars[speed_value]._Len0&255;
  tzx_turbo_head[8] = turbo_vars[speed_value]._Len0>>8;
  tzx_turbo_head[9] = (turbo_vars[speed_value]._Len0*2)&255;
  tzx_turbo_head[10] = (turbo_vars[speed_value]._Len0*2)>>8;
  tzx_turbo_head[11] = (libspectrum_word)((libspectrum_dword)3500000 / turbo_vars[speed_value]._LenPilot)&255;
  tzx_turbo_head[12] = (libspectrum_word)((libspectrum_dword)3500000 / turbo_vars[speed_value]._LenPilot)>>8;
  tzx_turbo_head[13] = 8;
  tzx_turbo_head[14] = 0;
  tzx_turbo_head[15] = 0;
  tzx_turbo_head[16] = 0;	// Lowest two filled in later
  tzx_turbo_head[17] = 0;
  tzx_turbo_head[18] = 0;	// Highest byte of length is 0 !
  tzx_turbo_head[19] = 0xaa;

  // We only need to write proper lenght later !

  int num_pages;
  PageOrder_s *page_order;

  int capabilities;

  libspectrum_machine machine = libspectrum_snap_machine( snap );
  capabilities = libspectrum_machine_capabilities( machine );

  int mode_128 = capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY;

  if( mode_128 ) {

    if( external ) {
      num_pages = 9;
      page_order = PageOrder128S;
    } else {
      num_pages = 8;
      page_order = PageOrder128N;
    }

  } else {
		
    if( external ) {
      num_pages = 4;
      page_order = PageOrder48S;
    } else {
      num_pages = 3;
      page_order = PageOrder48N;
    }

  }

	
  int loader_table_entry = 0;
  libspectrum_word add;
  libspectrum_byte num;
  libspectrum_word pagelen;
  libspectrum_word loadlen;
  int smallpage;

  libspectrum_byte external_screen[6912];

  int shortpage = 2;	// The page which contains loader - 1 on 48k and
			// 2 on 128k !

  for( i=0; i < num_pages; i++ )
    if( page_order[i].PageStart == 0x8000 )
      shortpage = page_order[i].PageNumber;

  // Get the information on which pages need to be loaded in !
  // Pages that are not loaded in are filled with 0 !

  int load;

  for( i=0; i < 8; i++ ) {

    if( !libspectrum_snap_pages( snap, i ) ) {
      load_page[i] = 0;
      continue;
    }

    load = 0;

    if( i != shortpage ) {

      // Pages 0, 2-8 are loaded in FULL
      for (j=0; j < 16384 && !load; j++)
	if( ( libspectrum_snap_pages( snap, i ) )[j] != 0 )
	  load = 1;

    } else {

      // Page 1 (48k) or 2 (128k) contains loader, so it is loaded in 2 parts
      for (j=0; j < 16384-768 && !load; j++) {
	if( (libspectrum_snap_pages( snap, i ))[j] != 0 ) {
	  load = 1;
	}
      }

      for (j=16384-768; j < 16384 && !load_768; j++) {
	if( (libspectrum_snap_pages( snap, i ))[j] != 0 )
	  load_768 = 1;
      }

    }

    load_page[i] = load;

  }

  for( i=0; i < num_pages; i++ ) {

    num = page_order[i].PageNumber;

    if( num == 255 ) {

      // External Loading screen
      FILE * exfile = NULL;
      exfile = fopen(external_filename, "rb");
      if (exfile == NULL) {
	print_error("Could not read the Loading Screen!");
	return;
      }

      fread(external_screen, 1, 6912, exfile);
      fclose(exfile);
      add = page_order[i].PageStart;

      pagelen = 6912;

      crunch_z80(external_screen, pagelen, WorkBuffer, &loadlen);

      // Fill in the table data
      add = add+(pagelen-1);
      libspectrum_byte addhi = (add>>8)&255;
      
      snap_bin[loader_table_pos+(loader_table_entry*4)] = 0x10;
      snap_bin[loader_table_pos+(loader_table_entry*4)+1] = addhi;
      snap_bin[loader_table_pos+(loader_table_entry*4)+2] = loadlen&255;
      snap_bin[loader_table_pos+(loader_table_entry*4)+3] = loadlen>>8;

      if (loadlen == 0) {
	sprintf(tempc,"- Adding Separate Loading Screen (Uncompressed) Len: %04x", 6912);
      } else {
	sprintf(tempc,"- Adding Separate Loading Screen  (Compressed)  Len: %04x", loadlen);
      }
      print_verbose(tempc);

      if( loadlen == 0 ) {
	loadlen = pagelen;	// The crunch_z80 returns 0 if the block could not be crunched
      }

      // Update the TZX Turbo header with the page length
      tzx_turbo_head[16] = (libspectrum_byte) ((loadlen+2)&255);
      tzx_turbo_head[17] = (libspectrum_byte) ((loadlen+2)>>8);

      add_data(tzx_turbo_head, 20);	// Add the turbo header to the file
      add_data(WorkBuffer, loadlen);	// Add the actual data
      snap_bin[data_pos]= calc_checksum((snap_bin+data_pos)-(loadlen+1), loadlen+1);
      data_pos++;				
      
      loader_table_entry++;

    } else {

      if( load_page[num] ) {

	// This page needs to be loaded
	add = page_order[i].PageStart;
	pagelen = 16384;

	libspectrum_byte realnum;

	if( mode_128 ) {

	  if (loader_table_entry == 0) {
	    realnum = 0x10;
	  } else {
	    realnum = num | 0x10;
	  }

	} else {
	  realnum = 0x10;
	}			
				
	if( add == 0x8000) {
	  // Special case - load 768 bytes less
	  pagelen-=768;

	  realnum = 0x12;	// Always assume 32768 page when loading short !

	  smallpage = num;	// Remember which page it was for later !
	}

	// Now crunch the block
	crunch_z80( libspectrum_snap_pages( snap, num ), pagelen, WorkBuffer,
		    &loadlen );

	if( external && num == 5 ) {

	  // Check if external screen is loading and last page selected
	  // If so then check if it would overwrite loading screen - if so
	  // then load decrunched
	  if( loadlen > (16384-6912) ) {
	    reverse_block(WorkBuffer, libspectrum_snap_pages( snap, num ) );
	    loadlen = 0;	// Not compressed !
	  }

	}

	int reverse_off = 0;

	if( loadlen != 0 ) {
	  // Check if it would overwrite itself ?
	  if( test_rev_decz80(WorkBuffer, pagelen, loadlen) ) {
	    reverse_block( WorkBuffer, libspectrum_snap_pages( snap, num ) );
	    if( pagelen != 16384 ) {
	      reverse_off = 768;	// We flipped the whole block, need
					// only lower part !
	    }

	    loadlen = 0;	// Not compressed !
	  }
	}

	// Fill in the table data
	add = add+(pagelen-1);
	libspectrum_byte addhi = (add>>8)&255;
				
	snap_bin[loader_table_pos+(loader_table_entry*4)] = realnum;
	snap_bin[loader_table_pos+(loader_table_entry*4)+1] = addhi;
	snap_bin[loader_table_pos+(loader_table_entry*4)+2] = loadlen&255;
	snap_bin[loader_table_pos+(loader_table_entry*4)+3] = loadlen>>8;

	if( loadlen == 0 ) {
	  sprintf(tempc,"- Adding Memory Page %i           (Uncompressed) Len: %04x", num, pagelen);
	} else {
	  sprintf(tempc,"- Adding Memory Page %i            (Compressed)  Len: %04x", num, loadlen);
	}
	print_verbose(tempc);

	if( loadlen == 0 ) {
	  loadlen = pagelen;	// The crunch_z80 returns 0 if the block could not be crunched
	}

	// Update the TZX Turbo header with the page length
	tzx_turbo_head[16] = (libspectrum_byte) ((loadlen+2)&255);
	tzx_turbo_head[17] = (libspectrum_byte) ((loadlen+2)>>8);

	add_data(tzx_turbo_head, 20);	// Add the turbo header to the file
	add_data(WorkBuffer+reverse_off, loadlen);	// Add the actual data
	snap_bin[data_pos]= calc_checksum((snap_bin+data_pos)-(loadlen+1), loadlen+1);
	data_pos++;				

	loader_table_entry++;
      
      }
    }
  }

  if( load_768 ) {

    // Area where the loader is was not empty - need to load it in
    print_verbose("- Adding Extra ROM Loading Block");
		
    tzx_header_data[3] = (768+2)&255;	// Length of extra block
    tzx_header_data[4] = (768+2)>>8;
    tzx_header_data[5] = 0x55;		// Flag is 0x55

    add_data(tzx_header_data, 6);
    int cstart = data_pos-1;
    add_data( libspectrum_snap_pages( snap, smallpage ) + 16384 - 768, 768 );
    snap_bin[data_pos]= calc_checksum(snap_bin+cstart, 769);
    data_pos++;

    // Set the 768 load flag to FF !
    snap_bin[pp+POS_768_LOAD] = 0xFF;

  } else {

    // Set the 768 load flag to 01 !
    snap_bin[pp+POS_768_LOAD] = 0x00;

    // Change the load stuff to LDIR stuff !
    snap_bin[pp+POS_CLEAN_1]   = 0xed;
    snap_bin[pp+POS_CLEAN_1+1] = 0xb0;
    snap_bin[pp+POS_CLEAN_1+2] = 0x00;
    snap_bin[pp+POS_CLEAN_2]   = 0xed;
    snap_bin[pp+POS_CLEAN_2+1] = 0xb0;
    snap_bin[pp+POS_CLEAN_2+2] = 0x00;

  }

  // Now lets fill the registers and stuff
	
  snap_bin[pp+POS_HL2]   = libspectrum_snap_hl_( snap ) & 0xff;
  snap_bin[pp+POS_HL2+1] = libspectrum_snap_hl_( snap ) >> 8;

  snap_bin[pp+POS_DE2]   = libspectrum_snap_de_( snap ) & 0xff;
  snap_bin[pp+POS_DE2+1] = libspectrum_snap_de_( snap ) >> 8;

  snap_bin[pp+POS_BC2]   = libspectrum_snap_bc_( snap ) & 0xff;
  snap_bin[pp+POS_BC2+1] = libspectrum_snap_bc_( snap ) >> 8;

  snap_bin[pp+POS_IY]    = libspectrum_snap_iy( snap ) & 0xff;
  snap_bin[pp+POS_IY+1]  = libspectrum_snap_iy( snap ) >> 8;

  snap_bin[pp+POS_LAST_ATTR] = load_colour|(load_colour<<3)|bright;

  if( mode_128 ) {
    snap_bin[pp+POS_48K_1] = 0x57;
    snap_bin[pp+POS_48K_2] = 0x57;
  } else {
    snap_bin[pp+POS_48K_1] = 0x56;
    snap_bin[pp+POS_48K_2] = 0x56;
  }

  snap_bin[pp+POS_LAST_AY] = libspectrum_snap_out_ay_registerport( snap );

  snap_bin[pp+POS_SP]   = libspectrum_snap_sp( snap ) & 0xff;
  snap_bin[pp+POS_SP+1] = libspectrum_snap_sp( snap ) >> 8;

  switch( libspectrum_snap_im( snap ) ) {
  case 0: snap_bin[pp+POS_IM] = 0x46; break;
  case 1: snap_bin[pp+POS_IM] = 0x56; break;
  case 2: snap_bin[pp+POS_IM] = 0x5E; break;
  }

  if( libspectrum_snap_iff1( snap ) ) {
    snap_bin[pp+POS_DIEI] = 0xfb;
  } else {
    snap_bin[pp+POS_DIEI] = 0xf3;		
  }

  snap_bin[pp+POS_PC]   = libspectrum_snap_pc( snap ) & 0xff;
  snap_bin[pp+POS_PC+1] = libspectrum_snap_pc( snap ) >> 8;

  snap_bin[pp+POS_BORDER] = load_colour;

  if( mode_128 ) {
    snap_bin[pp+POS_PAGE] = libspectrum_snap_out_128_memoryport( snap );
  } else {
    snap_bin[pp+POS_PAGE] = 0x10;
  }

  snap_bin[pp+POS_IX]   = libspectrum_snap_ix( snap ) & 0xff;
  snap_bin[pp+POS_IX+1] = libspectrum_snap_ix( snap ) >> 8;

  snap_bin[pp+POS_F2] = libspectrum_snap_f_( snap );
  snap_bin[pp+POS_A2] = libspectrum_snap_a_( snap );

  libspectrum_word rr = (libspectrum_word) libspectrum_snap_r( snap );
  rr-=0x0a;	// Compensate for the instructions after R is loaded !

  snap_bin[pp+POS_R] = rr & 0xff;
  snap_bin[pp+POS_I] = libspectrum_snap_i( snap );

  snap_bin[pp+POS_HL]   = libspectrum_snap_hl( snap ) & 0xff;
  snap_bin[pp+POS_HL+1] = libspectrum_snap_hl( snap ) >> 8;
	
  snap_bin[pp+POS_DE]   = libspectrum_snap_de( snap ) & 0xff;
  snap_bin[pp+POS_DE+1] = libspectrum_snap_de( snap ) >> 8;

  snap_bin[pp+POS_BC]   = libspectrum_snap_bc( snap ) & 0xff;
  snap_bin[pp+POS_BC+1] = libspectrum_snap_bc( snap ) >> 8;

  snap_bin[pp+POS_F] = libspectrum_snap_f( snap );
  snap_bin[pp+POS_A] = libspectrum_snap_a( snap );

  int ppay = POS_AYREG+1;
  for( i=0; i < 16; i++ ) {
    snap_bin[pp+ppay] = libspectrum_snap_ay_registers( snap, 15 - i );
    ppay+=2;
  }

  snap_bin[main_checksum] = calc_checksum( snap_bin + 5, main_checksum - 5 );  // need to recalc this in the end!!!
}

static void
convert_snap( void )
{
  FILE *file;

  print_verbose( "\nCreating TZX File :" );

  file = fopen( out_filename, "wb" );
  if( !file ) {
    print_error( "Output File could not be Opened!" );
    return;
  }

  data_pos = 0;
  add_data( tzx_start, 10 );	/* TZX start - ZXTape!... */

  create_main_header();
  add_data( tzx_header, tzx_header_size );

  fwrite( snap_bin, 1, data_pos, file );

  print_verbose( "- Adding Loader" );

  create_main_data();
  fwrite( snap_bin, 1, data_pos, file );

  fclose( file );
}

int
main( int argc, char **argv )
{
  int error;

  progname = argv[0];

  error = init_libspectrum(); if( error ) return error;
  error = parse_args( argc, argv ); if( error ) return error;
  error = load_snap(); if( error ) return error;

  convert_snap();

  print_verbose("\nDone!");

  return 0;
}
