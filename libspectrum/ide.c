/* ide.c: Routines for handling HDF hard disk files
   Copyright (c) 2003-2004 Garry Lancaster,
		 2004 Philip Kendall
		
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

   E-mail: Philip Kendall <pak21-fuse@srcf.ucam.org>
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <errno.h>
#include <string.h>

#include "internals.h"

/* Initialise a libspectrum_ide_channel structure */
libspectrum_error
libspectrum_ide_alloc( libspectrum_ide_channel **chn,
		       libspectrum_ide_databus databus )
{
  libspectrum_ide_channel *channel;

  channel = malloc( sizeof( libspectrum_ide_channel ) );
  if( !channel ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_ide_alloc: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  *chn = channel;

  channel->databus = databus;
  channel->drive[ LIBSPECTRUM_IDE_MASTER ].disk = NULL;
  channel->drive[ LIBSPECTRUM_IDE_SLAVE  ].disk = NULL;

  return LIBSPECTRUM_ERROR_NONE;
}

/* Free all memory used by a libspectrum_ide_channel structure */
libspectrum_error
libspectrum_ide_free( libspectrum_ide_channel *chn )
{
  /* Make sure all HDF files are ejected first */
  libspectrum_ide_eject( chn, LIBSPECTRUM_IDE_MASTER );
  libspectrum_ide_eject( chn, LIBSPECTRUM_IDE_SLAVE  );
  
  /* Free the channel structure */
  free( chn );

  return LIBSPECTRUM_ERROR_NONE;
}

/* Insert a hard disk into a drive */
libspectrum_error
libspectrum_ide_insert( libspectrum_ide_channel *chn,
			libspectrum_ide_unit unit,
                        const char *filename )
{
  FILE *f;
  size_t l;
  libspectrum_ide_drive *drv = &chn->drive[unit];

  libspectrum_ide_eject( chn, unit );
  if ( !filename ) return LIBSPECTRUM_ERROR_NONE;
  
  /* Open the file */
  f = fopen( filename, "rb+" );
  if( !f ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "libspectrum_ide_insert: unable to open file '%s': %s", filename,
      strerror( errno )
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }
  
  /* Read the HDF header */
  l = fread( &drv->hdf, 1, sizeof( libspectrum_hdf_header ), f ); 
  if ( l != sizeof( libspectrum_hdf_header ) ) {
    fclose( f );
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "libspectrum_ide_insert: unable to read HDF header from '%s'", filename
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }
  
  /* Verify the validity of the header */
  if( memcmp( &drv->hdf.signature, "RS-IDE", 6 ) ||
      drv->hdf.id != 0x1a                           ) {
    fclose( f );
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_ide_insert: '%s' is not a valid HDF file", filename
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }
  
  /* Extract details from the header */
  drv->disk = f;
  drv->data_offset =
    ( drv->hdf.datastart_hi << 8 ) | ( drv->hdf.datastart_low );
  drv->sector_size = ( drv->hdf.flags & 0x01 ) ? 256 : 512;
  
  /* Extract drive geometry from the drive identity command.

     For reasons best known to RamSoft, this (together with the disk
     data itself) is stored in Intel little-endian format rather than
     the way the ATA spec describes.
  */
  drv->cylinders =
    ( drv->hdf.drive_identity[ 3] << 8 ) | ( drv->hdf.drive_identity[ 2] );
  drv->heads =
    ( drv->hdf.drive_identity[ 7] << 8 ) | ( drv->hdf.drive_identity[ 6] );
  drv->sectors =
    ( drv->hdf.drive_identity[13] << 8 ) | ( drv->hdf.drive_identity[12] );
  
  return LIBSPECTRUM_ERROR_NONE;
}


/* Eject a hard disk from a drive */
libspectrum_error
libspectrum_ide_eject( libspectrum_ide_channel *chn,
                        libspectrum_ide_unit unit )
{
  libspectrum_ide_drive *drv = &chn->drive[unit];
  
  if( drv->disk ) {
    fclose( drv->disk );
    drv->disk = NULL;
  }
  
  return LIBSPECTRUM_ERROR_NONE;
}


/* Reset an IDE channel */
libspectrum_error
libspectrum_ide_reset( libspectrum_ide_channel *chn )
{
  /* Reset channel status */
  chn->selected = LIBSPECTRUM_IDE_MASTER;
  chn->phase = LIBSPECTRUM_IDE_PHASE_READY;
  
  if( chn->drive[LIBSPECTRUM_IDE_MASTER].disk ||
      chn->drive[LIBSPECTRUM_IDE_SLAVE].disk     ) {

    /* ATA reset signature for non-PACKET-supporting drives */
    chn->sector_count = 0x01;
    chn->sector = 0x01;
    chn->cylinder_low = 0x00;
    chn->cylinder_high = 0x00;
    chn->head = 0x00;

    /* Diagnostics passed */
    chn->error = 0x01;
    
    /* Device ready, no PACKET support */
    chn->status = 0x72;

    /* Feature is write-only */
    chn->feature = 0xff;

  } else {

    /* If no drive is present, set all registers to 0xff */
    chn->sector_count = 0xff;
    chn->sector = 0xff;
    chn->cylinder_low = 0xff;
    chn->cylinder_high = 0xff;
    chn->head = 0xff;
    chn->error = 0xff;
    chn->status = 0xff;
    chn->feature = 0xff;

  }
  
  return LIBSPECTRUM_ERROR_NONE;
}


/* Read a sector from the HDF file */
static int
read_hdf( libspectrum_ide_channel *chn )
{
  libspectrum_ide_drive *drv = &chn->drive[ chn->selected ];
  libspectrum_byte packed_buf[512];

  /* Seek to the correct file position */
  chn->sector_position = ftell( drv->disk );
  if( chn->sector_position == -1 ) return 1; /* seek failure */

  /* Read the packed data into a temporary buffer */
  if ( fread( &packed_buf[0], 1, drv->sector_size, drv->disk ) !=
       drv->sector_size                                           )
    return 1;		/* read error */

  /* Unpack or copy the data into the sector buffer */
  if( drv->sector_size == 256 ) {

    int i;
    
    for (i = 0; i < 256; i++ ) {
      chn->buffer[ i*2 ] = packed_buf[ i ];
      chn->buffer[ i*2 + 1 ] = 0xff;
    }

  } else {
    memcpy( &chn->buffer[0], &packed_buf[0], 512 );
  }
  
  return 0;
}


/* Write a sector to the HDF file */
static int
write_hdf( libspectrum_ide_channel *chn )
{
  libspectrum_ide_drive *drv = &chn->drive[ chn->selected ];
  libspectrum_byte packed_buf[512];

  /* Pack or copy the data into a temporary buffer */
  if ( drv->sector_size == 256 ) {
    int i;
    
    for (i = 0; i < 256; i++ ) {
      packed_buf[ i ] = chn->buffer[ i*2 ];
    }

  } else {
    memcpy( &packed_buf[0], &chn->buffer[0], 512 );
  }

  if( fseek( drv->disk, chn->sector_position, SEEK_SET ) )
    return 1;		/* seek failure */

  /* Write the temporary buffer to the HDF file */
  if ( fwrite( &packed_buf[0], 1, drv->sector_size, drv->disk ) !=
       drv->sector_size                                            )
    return 1;		/* write error */
  
  return 0;
}


/* Read the data register */
static libspectrum_byte
read_data( libspectrum_ide_channel *chn )
{
  libspectrum_byte data;
  
  /* Meaningful data is only returned in PIO input phase */
  if( chn->phase != LIBSPECTRUM_IDE_PHASE_PIO_IN ) return 0xff;

  switch( chn->databus ) {

  case LIBSPECTRUM_IDE_DATA8:
    /* 8-bit interfaces just read the low byte */
    data = chn->buffer[ chn->datacounter ];
    chn->datacounter += 2;
    break;

  case LIBSPECTRUM_IDE_DATA16:
    data = chn->buffer[ chn->datacounter++ ];
    break;

  case LIBSPECTRUM_IDE_DATA16_BYTESWAP:
    data = chn->buffer[ chn->datacounter ^ 1 ];
    chn->datacounter++;
    break;
        
  case LIBSPECTRUM_IDE_DATA16_DATA2:
    /* 16-bit interfaces using a secondary data register for the high byte */
    data = chn->buffer[ chn->datacounter++ ];
    chn->data2 = chn->buffer[ chn->datacounter++ ];
    break;

  default:
    data = 0xff; break;

  }

  /* Check for end of phase */
  if( chn->datacounter >= 512 ) {
    chn->phase = LIBSPECTRUM_IDE_PHASE_READY;
    chn->status &= ~LIBSPECTRUM_IDE_STATUS_DRQ;
  }

  return data;
}

/* Read the IDE interface */
libspectrum_byte
libspectrum_ide_read( libspectrum_ide_channel *chn,
		      libspectrum_ide_register reg )
{
  /* Only read if the currently-selected drive exists */
  if( chn->drive[ chn->selected ].disk ) {

    switch( reg ) {

    case LIBSPECTRUM_IDE_REGISTER_DATA:           return read_data( chn );
    case LIBSPECTRUM_IDE_REGISTER_DATA2:          return chn->data2;
    case LIBSPECTRUM_IDE_REGISTER_ERROR_FEATURE:  return chn->error;
    case LIBSPECTRUM_IDE_REGISTER_SECTOR_COUNT:   return chn->sector_count;
    case LIBSPECTRUM_IDE_REGISTER_SECTOR:         return chn->sector;
    case LIBSPECTRUM_IDE_REGISTER_CYLINDER_LOW:   return chn->cylinder_low;
    case LIBSPECTRUM_IDE_REGISTER_CYLINDER_HIGH:  return chn->cylinder_high;
    case LIBSPECTRUM_IDE_REGISTER_HEAD_DRIVE:     return chn->head;
    case LIBSPECTRUM_IDE_REGISTER_COMMAND_STATUS: return chn->status;

    }
  }
  
  return 0xff;
}


/* Write the data register */
static void
write_data( libspectrum_ide_channel *chn, libspectrum_byte data )
{
  /* Data register can only be written in PIO output phase */
  if( chn->phase != LIBSPECTRUM_IDE_PHASE_PIO_OUT ) return;

  switch( chn->databus ) {
    
  case LIBSPECTRUM_IDE_DATA8:
    /* 8-bit interfaces just write the low byte */
    chn->buffer[ chn->datacounter ] = data;
    chn->datacounter += 2;
    break;

  case LIBSPECTRUM_IDE_DATA16:
    chn->buffer[ chn->datacounter++ ] = data;
    break;
    
  case LIBSPECTRUM_IDE_DATA16_BYTESWAP:
    chn->buffer[ chn->datacounter ^ 1 ] = data;
    chn->datacounter++;
    break;

  case LIBSPECTRUM_IDE_DATA16_DATA2:
    /* 16-bit interfaces using a secondary data register for the high byte */
    chn->buffer[ chn->datacounter++ ] = data;
    chn->buffer[ chn->datacounter++ ] = chn->data2;
    break;
  }
    
  /* Check for end of phase */
  if( chn->datacounter >= 512 ) {

    chn->phase = LIBSPECTRUM_IDE_PHASE_READY;
    chn->status &= ~LIBSPECTRUM_IDE_STATUS_DRQ;
      
    /* Write data to disk */
    if ( write_hdf( chn ) ) {
      chn->status |= LIBSPECTRUM_IDE_STATUS_ERR;
      chn->error = LIBSPECTRUM_IDE_ERROR_ABRT | LIBSPECTRUM_IDE_ERROR_UNC;
    }

  }

}

/* Seek to the addressed sector */
static libspectrum_error
seek( libspectrum_ide_channel *chn )
{
  libspectrum_ide_drive *drv = &chn->drive[ chn->selected ];
  int sectornumber;
  
  /* Sector count must be 1 */
  if( chn->sector_count != 1 ) {
    chn->status |= LIBSPECTRUM_IDE_STATUS_ERR;
    chn->error = LIBSPECTRUM_IDE_ERROR_ABRT;
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }
  
  /* Calculate sector number, depending upon LBA/CHS mode. */
  if( chn->head & LIBSPECTRUM_IDE_HEAD_LBA ) {

    sectornumber = ( chn->head & LIBSPECTRUM_IDE_HEAD_HEAD << 24 ) +
                   ( chn->cylinder_high << 16 )			   +
                   ( chn->cylinder_low << 8 )			   +
                   ( chn->sector );

  } else {

    int cylinder = ( chn->cylinder_high << 8 ) | ( chn->cylinder_low );
    int head = ( chn->head & LIBSPECTRUM_IDE_HEAD_HEAD );
    int sector = ( chn->sector - 1 );

    if( cylinder >= drv->cylinders || head >= drv->heads     ||
        sector < 0                 || sector >= drv->sectors    ) {
      sectornumber = -1;
    } else {
      sectornumber =
	( ( ( cylinder * drv->heads ) + head ) * drv->sectors ) + sector;
    }

  }

  /* Seek to the correct position */
  if( sectornumber < 0                                               ||
      sectornumber >= ( drv->cylinders * drv->heads * drv->sectors )    ) {  
    chn->status |= LIBSPECTRUM_IDE_STATUS_ERR;
    chn->error = LIBSPECTRUM_IDE_ERROR_ABRT | LIBSPECTRUM_IDE_ERROR_IDNF;
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }
  
  chn->sector_position =
    drv->data_offset + ( drv->sector_size * sectornumber );
                    
  return LIBSPECTRUM_ERROR_NONE;
}

/* Execute the IDENTIFY DEVICE command */
static void
identifydevice( libspectrum_ide_channel *chn )
{
  /* Clear sector buffer and copy in HDF identity information */
  memset( &chn->buffer[0], 0, 512 );
  memcpy( &chn->buffer[0], &chn->drive[ chn->selected ].hdf.drive_identity[0],
	  0x6a );
    
  /* Initiate the PIO input phase */
  chn->phase = LIBSPECTRUM_IDE_PHASE_PIO_IN;
  chn->status |= LIBSPECTRUM_IDE_STATUS_DRQ;
  chn->datacounter = 0;
}

/* Execute the READ SECTOR command */
static void
readsector( libspectrum_ide_channel *chn )
{
  if( seek( chn ) ) return;

  /* Read data from disk */
  if( read_hdf( chn ) ) {
    
    chn->status |= LIBSPECTRUM_IDE_STATUS_ERR;
    chn->error = LIBSPECTRUM_IDE_ERROR_ABRT | LIBSPECTRUM_IDE_ERROR_UNC;

  } else {

    /* Initiate the PIO input phase */
    chn->phase = LIBSPECTRUM_IDE_PHASE_PIO_IN;
    chn->status |= LIBSPECTRUM_IDE_STATUS_DRQ;
    chn->datacounter = 0;

  }

}

/* Execute the WRITE SECTOR command */
static void
writesector( libspectrum_ide_channel *chn )
{
  if( seek( chn ) ) return;

  /* Initiate the PIO output phase */
  chn->phase = LIBSPECTRUM_IDE_PHASE_PIO_OUT;
  chn->status |= LIBSPECTRUM_IDE_STATUS_DRQ;
  chn->datacounter = 0;
}

/* Execute a command */
static void
execute_command( libspectrum_ide_channel *chn, libspectrum_byte data )
{

  if( !chn->drive[ chn->selected].disk          ||
      chn->phase != LIBSPECTRUM_IDE_PHASE_READY    ) return;

  /* Clear error conditions */
  chn->error = LIBSPECTRUM_IDE_ERROR_OK;
  chn->status &= ~(LIBSPECTRUM_IDE_STATUS_ERR | LIBSPECTRUM_IDE_STATUS_BSY);
  chn->status |= LIBSPECTRUM_IDE_STATUS_DRDY;

  /* Perform command */
  switch( data ) {

  case LIBSPECTRUM_IDE_COMMAND_READ_SECTOR:    readsector( chn );     break;
  case LIBSPECTRUM_IDE_COMMAND_WRITE_SECTOR:   writesector( chn );    break;
  case LIBSPECTRUM_IDE_COMMAND_IDENTIFY_DRIVE: identifydevice( chn ); break;
      
    /* Unknown/unsupported commands */
  default:
    chn->status |= LIBSPECTRUM_IDE_STATUS_ERR;
    chn->error = LIBSPECTRUM_IDE_ERROR_ABRT;
  }

}


/* Write the IDE interface */
void
libspectrum_ide_write( libspectrum_ide_channel *chn,
		       libspectrum_ide_register reg, libspectrum_byte data )
{
  switch( reg ) {
  case LIBSPECTRUM_IDE_REGISTER_DATA:          write_data( chn, data ); break;
  case LIBSPECTRUM_IDE_REGISTER_DATA2:         chn->data2 = data; break;
  case LIBSPECTRUM_IDE_REGISTER_ERROR_FEATURE: chn->feature = data; break;
  case LIBSPECTRUM_IDE_REGISTER_SECTOR_COUNT:  chn->sector_count = data; break;
  case LIBSPECTRUM_IDE_REGISTER_SECTOR:        chn->sector = data; break;
  case LIBSPECTRUM_IDE_REGISTER_CYLINDER_LOW:  chn->cylinder_low = data; break;
  case LIBSPECTRUM_IDE_REGISTER_CYLINDER_HIGH:
    chn->cylinder_high = data; break;
      
  case LIBSPECTRUM_IDE_REGISTER_HEAD_DRIVE:
    chn->head = data;
    chn->selected = chn->head & LIBSPECTRUM_IDE_HEAD_DEV ?
                    LIBSPECTRUM_IDE_SLAVE : LIBSPECTRUM_IDE_MASTER;
    break;

  case LIBSPECTRUM_IDE_REGISTER_COMMAND_STATUS:
    execute_command( chn, data ); break;
  }
}
