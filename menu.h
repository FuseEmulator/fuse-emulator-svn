#ifndef FUSE_MENU_H
#define FUSE_MENU_H

#include <libspectrum.h>

#include "ui/scaler/scaler.h"

#ifdef USE_WIDGET

#define MENU_CALLBACK( name ) void name( int action )
#define MENU_CALLBACK_WITH_ACTION( name ) void name( int action )

#else			/* #ifdef USE_WIDGET */

#include <gtk/gtk.h>

#define MENU_CALLBACK( name ) void name( GtkWidget *widget, gpointer data )
#define MENU_CALLBACK_WITH_ACTION( name ) \
  void name( gpointer data, guint action, GtkWidget *widget )

#endif			/* #ifdef USE_WIDGET */

/*
 * Things defined in menu.c
 */

MENU_CALLBACK( menu_file_open );
MENU_CALLBACK( menu_file_recording_play );
MENU_CALLBACK( menu_file_aylogging_stop );
MENU_CALLBACK( menu_file_openscrscreenshot );

MENU_CALLBACK_WITH_ACTION( menu_options_selectroms_select );
MENU_CALLBACK( menu_options_filter );
MENU_CALLBACK( menu_options_save );

MENU_CALLBACK( menu_machine_nmi );

MENU_CALLBACK( menu_media_tape_browse );
MENU_CALLBACK( menu_media_tape_open );
MENU_CALLBACK( menu_media_tape_play );
MENU_CALLBACK( menu_media_tape_rewind );
MENU_CALLBACK( menu_media_tape_clear );
MENU_CALLBACK( menu_media_tape_write );

MENU_CALLBACK_WITH_ACTION( menu_media_disk_eject );

MENU_CALLBACK( menu_media_cartridge_insert );
MENU_CALLBACK( menu_media_cartridge_eject );

MENU_CALLBACK_WITH_ACTION( menu_media_ide_simple8bit_insert );
MENU_CALLBACK_WITH_ACTION( menu_media_ide_simple8bit_commit );
MENU_CALLBACK_WITH_ACTION( menu_media_ide_simple8bit_eject );

int menu_open_snap( void );

/*
 * Things to be defined elsewhere
 */

/* Direct menu callbacks */

MENU_CALLBACK( menu_file_savesnapshot );
MENU_CALLBACK( menu_file_recording_record );
MENU_CALLBACK( menu_file_recording_recordfromsnapshot );
MENU_CALLBACK( menu_file_recording_stop );
MENU_CALLBACK( menu_file_loadbinarydata );
MENU_CALLBACK( menu_file_savebinarydata );
MENU_CALLBACK( menu_file_exit );

MENU_CALLBACK( menu_file_aylogging_record );

MENU_CALLBACK( menu_file_savescreenasscr );
MENU_CALLBACK( menu_file_savescreenaspng );

MENU_CALLBACK( menu_options_general );
MENU_CALLBACK( menu_options_sound );
MENU_CALLBACK( menu_options_rzx );
MENU_CALLBACK( menu_options_joysticks_select );

MENU_CALLBACK( menu_machine_pause );
MENU_CALLBACK( menu_machine_reset );
MENU_CALLBACK( menu_machine_select );
MENU_CALLBACK( menu_machine_debugger );
MENU_CALLBACK( menu_machine_pokefinder );
MENU_CALLBACK( menu_machine_memorybrowser );

MENU_CALLBACK_WITH_ACTION( menu_media_disk_insert );

MENU_CALLBACK( menu_help_keyboard );

/* Called from elsewhere (generally from one of the routines defined
   in menu.c) */

int menu_select_roms( libspectrum_machine machine, size_t start,
		      size_t count );
char *menu_get_filename( const char *title );
scaler_type menu_get_scaler( scaler_available_fn selector );

#endif				/* #ifndef FUSE_MENU_H */
