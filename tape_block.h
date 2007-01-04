/* tape_block.h: individual tape block types
   Copyright (c) 2003-2007 Philip Kendall

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

#ifndef LIBSPECTRUM_TAPE_BLOCK_H
#define LIBSPECTRUM_TAPE_BLOCK_H

#ifndef LIBSPECTRUM_INTERNALS_H
#include "internals.h"
#endif				/* #ifndef LIBSPECTRUM_INTERNALS_H */

/*
 * The individual block types
 */

/* A standard ROM loading block */
typedef struct libspectrum_tape_rom_block {
  
  size_t length;		/* How long is this block */
  libspectrum_byte *data;	/* The actual data */
  libspectrum_dword pause;	/* Pause after block (milliseconds) */

  /* Private data */

  libspectrum_tape_state_type state;

  size_t edge_count;		/* Number of pilot pulses to go */

  size_t bytes_through_block;
  size_t bits_through_byte;	/* How far through the data are we? */

  libspectrum_byte current_byte; /* The current data byte; gets shifted out
				    as we read bits from it */
  libspectrum_dword bit_tstates; /* How long is an edge for the current bit */

} libspectrum_tape_rom_block;

/* A turbo loading block */
typedef struct libspectrum_tape_turbo_block {

  size_t length;		/* Length of data */
  size_t bits_in_last_byte;	/* How many bits are in the last byte? */
  libspectrum_byte *data;	/* The actual data */
  libspectrum_dword pause;	/* Pause after data (in ms) */

  libspectrum_dword pilot_length; /* Length of pilot pulse (in tstates) */
  size_t pilot_pulses;		/* Number of pilot pulses */

  libspectrum_dword sync1_length, sync2_length; /* Length of the sync pulses */
  libspectrum_dword bit0_length, bit1_length; /* Length of (re)set bits */

  /* Private data */

  libspectrum_tape_state_type state;

  size_t edge_count;		/* Number of pilot pulses to go */

  size_t bytes_through_block;
  size_t bits_through_byte;	/* How far through the data are we? */

  libspectrum_byte current_byte; /* The current data byte; gets shifted out
				    as we read bits from it */
  libspectrum_dword bit_tstates; /* How long is an edge for the current bit */

} libspectrum_tape_turbo_block;

/* A pure tone block */
typedef struct libspectrum_tape_pure_tone_block {

  libspectrum_dword length;
  size_t pulses;

  /* Private data */

  size_t edge_count;

} libspectrum_tape_pure_tone_block;

/* A list of pulses of different lengths */
typedef struct libspectrum_tape_pulses_block {

  size_t count;
  libspectrum_dword *lengths;

  /* Private data */

  size_t edge_count;

} libspectrum_tape_pulses_block;

/* A block of just of data */
typedef struct libspectrum_tape_pure_data_block {

  size_t length;		/* Length of data */
  size_t bits_in_last_byte;	/* How many bits are in the last byte? */
  libspectrum_byte *data;	/* The actual data */
  libspectrum_dword pause;	/* Pause after data (in ms) */

  libspectrum_dword bit0_length, bit1_length; /* Length of (re)set bits */

  /* Private data */

  libspectrum_tape_state_type state;

  size_t bytes_through_block;
  size_t bits_through_byte;	/* How far through the data are we? */

  libspectrum_byte current_byte; /* The current data byte; gets shifted out
				    as we read bits from it */
  libspectrum_dword bit_tstates; /* How long is an edge for the current bit */

} libspectrum_tape_pure_data_block;

/* A raw data block */
typedef struct libspectrum_tape_raw_data_block {

  size_t length;		/* Length of data */
  size_t bits_in_last_byte;	/* How many bits are in the last byte? */
  libspectrum_byte *data;	/* The actual data */
  libspectrum_dword pause;	/* Pause after data (in ms) */

  libspectrum_dword bit_length; /* Bit length. *Not* pulse length! */

  /* Private data */

  libspectrum_tape_state_type state;

  size_t bytes_through_block;
  size_t bits_through_byte;	/* How far through the data are we? */

  libspectrum_byte last_bit;	/* The last bit which was read */
  libspectrum_dword bit_tstates; /* How long is an edge for the current bit */

} libspectrum_tape_raw_data_block;

struct libspectrum_tape_generalised_data_symbol {

  libspectrum_tape_generalised_data_symbol_edge_type edge_type;
  libspectrum_word *lengths;

};

struct libspectrum_tape_generalised_data_symbol_table {

  libspectrum_dword symbols_in_block;
  libspectrum_byte max_pulses;
  libspectrum_word symbols_in_table;

  libspectrum_tape_generalised_data_symbol *symbols;

};

typedef struct libspectrum_tape_generalised_data_block {

  libspectrum_dword pause;	/* Pause after data (in ms) */

  libspectrum_tape_generalised_data_symbol_table pilot_table, data_table;

  libspectrum_byte *pilot_symbols;
  libspectrum_word *pilot_repeats;

} libspectrum_tape_generalised_data_block;

/* A pause block */
typedef struct libspectrum_tape_pause_block {

  libspectrum_dword length;

} libspectrum_tape_pause_block;

/* A group start block */
typedef struct libspectrum_tape_group_start_block {

  char *name;

} libspectrum_tape_group_start_block;

/* No group end block needed as it contains no data */

/* A jump block */
typedef struct libspectrum_tape_jump_block {

  int offset;

} libspectrum_tape_jump_block;

/* A loop start block */
typedef struct libspectrum_tape_loop_start_block {

  int count;

} libspectrum_tape_loop_start_block;

/* No loop end block needed as it contains no data */

/* A select block */
typedef struct libspectrum_tape_select_block {

  /* Number of selections */
  size_t count;

  /* Offset of each selection, and a description of each */
  int *offsets;
  char **descriptions;

} libspectrum_tape_select_block;

/* No `stop tape if in 48K mode' block as it contains no data */

/* A comment block */
typedef struct libspectrum_tape_comment_block {

  char *text;

} libspectrum_tape_comment_block;

/* A message block */
typedef struct libspectrum_tape_message_block {

  int time;
  char *text;

} libspectrum_tape_message_block;

/* An archive info block */
typedef struct libspectrum_tape_archive_info_block {

  /* Number of strings */
  size_t count;

  /* ID for each string */
  int *ids;

  /* Text of each string */
  char **strings;

} libspectrum_tape_archive_info_block;

/* A hardware info block */
typedef struct libspectrum_tape_hardware_block {

  /* Number of entries */
  size_t count;

  /* For each entry, a type, an ID and a value */
  int *types, *ids, *values;

} libspectrum_tape_hardware_block;

/* A custom block */
typedef struct libspectrum_tape_custom_block {

  /* Description of this block */
  char *description;

  /* And the data for it; currently, no attempt is made to interpret
     this data */
  size_t length; libspectrum_byte *data;

} libspectrum_tape_custom_block;

/* No block needed for concatenation block, as it isn't stored */

/* Block types not present in the TZX format follow here */

/* A Z80Em or CSW audio block */
typedef struct libspectrum_tape_rle_pulse_block {

  size_t length;
  size_t index;
  libspectrum_byte *data;
  long scale;

} libspectrum_tape_rle_pulse_block;

/*
 * The generic tape block
 */

struct libspectrum_tape_block {

  libspectrum_tape_type type;

  union {
    libspectrum_tape_rom_block rom;
    libspectrum_tape_turbo_block turbo;
    libspectrum_tape_pure_tone_block pure_tone;
    libspectrum_tape_pulses_block pulses;
    libspectrum_tape_pure_data_block pure_data;
    libspectrum_tape_raw_data_block raw_data;

    libspectrum_tape_generalised_data_block generalised_data;

    libspectrum_tape_pause_block pause;
    libspectrum_tape_group_start_block group_start;
    /* No group end block needed as it contains no data */
    libspectrum_tape_jump_block jump;
    libspectrum_tape_loop_start_block loop_start;
    /* No loop end block needed as it contains no data */

    libspectrum_tape_select_block select;

    /* No `stop tape if in 48K mode' block as it contains no data */

    libspectrum_tape_comment_block comment;
    libspectrum_tape_message_block message;
    libspectrum_tape_archive_info_block archive_info;
    libspectrum_tape_hardware_block hardware;

    libspectrum_tape_custom_block custom;

    libspectrum_tape_rle_pulse_block rle_pulse;
  } types;

};

/* Functions needed by both tape.c and tape_block.c */
libspectrum_error
libspectrum_tape_pure_data_next_bit( libspectrum_tape_pure_data_block *block );
libspectrum_error
libspectrum_tape_raw_data_next_bit( libspectrum_tape_raw_data_block *block );

#endif				/* #ifndef LIBSPECTRUM_TAPE_BLOCK_H */

