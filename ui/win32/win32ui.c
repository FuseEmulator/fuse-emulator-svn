/* win32ui.c: Win32 routines for dealing with the user interface
   Copyright (c) 2003-2007 Marek Januszewski, Philip Kendall, Stuart Brady

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include "debugger/debugger.h"
#include "display.h"
#include "fuse.h"
#include "menu.h"
#include "menu_data.h"
#include "psg.h"
#include "rzx.h"
#include "screenshot.h"
#include "select_template.h"
#include "settings.h"
#include "snapshot.h"
#include "tape.h"
#include "timer/timer.h"
#include "ui/ui.h"
#include "ui/uijoystick.h"
#include "utils.h"
#include "win32internals.h"

/* fuse_hPrevInstance is needed only to register window class */
static HINSTANCE fuse_hPrevInstance;

/* specifies how the main window should be shown: minimized, maximized, etc */
static int fuse_nCmdShow;

/* handle to accelartors/keyboard shortcuts */
static HACCEL hAccels;

/* machine select dialog's font object */
static HFONT h_ms_font = NULL;

/* monospaced font to use with debugger and memory browser */
static HFONT monospaced_font = NULL;

/* True if we were paused via the Machine/Pause menu item */
static int paused = 0;

/* this helps pause fuse while the main window is minimized */
static int size_paused = 0;

/* Structure used by the radio button selection widgets (eg the
   graphics filter selectors and Machine/Select) */
typedef struct win32ui_select_info {

  int length;
  int selected;
  const char **labels;
  TCHAR *dialog_title;

} win32ui_select_info;

static BOOL win32ui_make_menu( void );
static int win32ui_window_paint( HWND hWnd, WPARAM wParam, LPARAM lParam );
static int win32ui_window_resize( HWND hWnd, WPARAM wParam, LPARAM lParam );
static int win32ui_window_resizing( HWND hWnd, WPARAM wParam, LPARAM lParam );

static int
selector_dialog( win32ui_select_info *items );

#define STUB do { printf("STUB: %s()\n", __func__); fflush(stdout); } while(0)

static void
handle_drop( HDROP hDrop )
{
  size_t bufsize;
  char *namebuf;

  /* Check that only one file was dropped */
  if( DragQueryFile( hDrop, ~0UL, NULL, 0 ) == 1) {
    bufsize = DragQueryFile( hDrop, 0, NULL, 0 ) + 1;
    if( ( namebuf = malloc( bufsize ) ) ) {
      DragQueryFile( hDrop, 0, namebuf, bufsize );

      fuse_emulation_pause();

      utils_open_file( namebuf, tape_can_autoload(), NULL );

      free( namebuf );

      display_refresh_all();

      fuse_emulation_unpause();
    }
  }
  DragFinish( hDrop );
}

static LRESULT WINAPI
fuse_window_proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch( msg ) {
    case WM_COMMAND:
      handle_menu( LOWORD( wParam ), hWnd );
      break;

    case WM_DROPFILES:
      handle_drop( ( HDROP )wParam );
      break;

    case WM_CLOSE:
      menu_file_exit( 0 );
      break;

    case WM_KEYDOWN:
      win32keyboard_keypress( wParam, lParam );
      break;

    case WM_KEYUP:
      win32keyboard_keyrelease( wParam, lParam );
      break;

    case WM_PAINT:
      return win32ui_window_paint( hWnd, wParam, lParam ); 

    case WM_SIZING:
      return win32ui_window_resizing( hWnd, wParam, lParam );

    case WM_SIZE:
      return win32ui_window_resize( hWnd, wParam, lParam );

    case WM_DRAWITEM:
      if( wParam == ID_STATUSBAR ) {
        win32statusbar_redraw( hWnd, lParam );
        return TRUE;
      }

    case WM_DESTROY:
      fuse_exiting = 1;
      PostQuitMessage( 0 );

      /* Stop the paused state to allow us to exit (occurs from main
         emulation loop) */
      if( paused ) menu_machine_pause( 0 );
      break;

    case WM_ENTERMENULOOP:
    case WM_ENTERSIZEMOVE:
    {
      fuse_emulation_pause();
      break;
    }

    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE:
    {
      fuse_emulation_unpause();
      break;
    }
    
    case WM_LBUTTONUP:
      win32mouse_button( 1, 0 );
      return 0;
      
    case WM_LBUTTONDOWN:
      win32mouse_button( 1, 1 );
      return 0;

    case WM_MBUTTONUP:
      win32mouse_button( 2, 0 );
      return 0;

    case WM_MBUTTONDOWN:
      win32mouse_button( 2, 1 );
      return 0;

    case WM_RBUTTONUP:
      win32mouse_button( 3, 0 );
      return 0;

    case WM_RBUTTONDOWN:
      win32mouse_button( 3, 1 );
      return 0;
      
    case WM_MOUSEMOVE:
      win32mouse_position( lParam );
      return 0;
      
    case WM_SETCURSOR:
    /* prevent the cursor from being redrawn if fuse has grabbed the mouse */
      if( ui_mouse_grabbed )
        return TRUE;

    default:
      return( DefWindowProc( hWnd, msg, wParam, lParam ) );
  }

  return 0;
}

/* this is where windows program begins */
int WINAPI
WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
         int nCmdShow )
{
  /* remember those values for window creation in ui_init */
  fuse_hInstance = hInstance;
  fuse_nCmdShow = nCmdShow;
  fuse_hPrevInstance = hPrevInstance;

  return fuse_main(__argc, __argv);
  /* FIXME: how do deal with returning wParam */
}

int
ui_init( int *argc, char ***argv )
{
  /* register window class */
  WNDCLASS wc;

  if( !fuse_hPrevInstance ) {
    wc.lpszClassName = "Fuse";
    wc.lpfnWndProc = fuse_window_proc;
    wc.style = CS_OWNDC;
    wc.hInstance = fuse_hInstance;
    wc.hIcon = LoadIcon( fuse_hInstance, "win32_icon" );
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)( COLOR_WINDOW+1 );
    wc.lpszMenuName = "win32_menu";
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;

    if( !RegisterClass( &wc ) )
      return 0;
  }

  /* create the window */
  fuse_hWnd = CreateWindow( "Fuse", "Fuse", WS_OVERLAPPED | WS_CAPTION |
    WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    NULL, NULL, fuse_hInstance, NULL );

  /* init windows controls such as the status bar */
  InitCommonControls();

  /* menu is created in rc file, but we still need to set initial state */
  win32ui_make_menu();

  /* load keyboard shortcuts */
  hAccels = LoadAccelerators( fuse_hInstance, "win32_accel" );

  /* status bar */
  win32statusbar_create( fuse_hWnd );

  /* set the initial size of the drawing area */
  RECT wr, cr, statr;
  int w_ofs, h_ofs;
  
  GetWindowRect( fuse_hWnd, &wr );
  GetClientRect( fuse_hWnd, &cr );
  GetClientRect( fuse_hStatusWindow, &statr );

  w_ofs = ( wr.right - wr.left ) - ( cr.right - cr.left );
  h_ofs = ( wr.bottom - wr.top ) - ( cr.bottom - cr.top )
          + ( statr.bottom - statr.top );
  MoveWindow( fuse_hWnd, wr.left, wr.top,
              DISPLAY_ASPECT_WIDTH + w_ofs,
              DISPLAY_SCREEN_HEIGHT + h_ofs,
              FALSE );

  /* init the display area */
  if( win32display_init() ) return 1;

  /* show the window finally */
  ShowWindow( fuse_hWnd, fuse_nCmdShow );
  UpdateWindow( fuse_hWnd );

  /* window will accept dragging and dropping */
  DragAcceptFiles( fuse_hWnd, TRUE );

  win32statusbar_set_visibility( settings_current.statusbar );
  
  ui_mouse_present = 1;

  return 0;
}

static BOOL
win32ui_make_menu( void )
{
  /* Start various menus in the 'off' state */
  ui_menu_activate( UI_MENU_ITEM_AY_LOGGING, 0 );
  ui_menu_activate( UI_MENU_ITEM_FILE_MOVIES_RECORDING, 0 );
  ui_menu_activate( UI_MENU_ITEM_MACHINE_PROFILER, 0 );
  ui_menu_activate( UI_MENU_ITEM_RECORDING, 0 );
  ui_menu_activate( UI_MENU_ITEM_RECORDING_ROLLBACK, 0 );
  ui_menu_activate( UI_MENU_ITEM_TAPE_RECORDING, 0 );

  return FALSE;
}

int
ui_event( void )
{
  MSG msg;

  do {

    /* Process messages */
    while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
      if( !IsDialogMessage( fuse_hPFWnd, &msg  ) &&
          !IsDialogMessage( fuse_hDBGWnd, &msg ) ) {
        if( !TranslateAccelerator( fuse_hWnd, hAccels, &msg ) ) {
          if( msg.message == WM_QUIT ) break;
          /* finish - set exit flag somewhere */
          TranslateMessage( &msg );
          DispatchMessage( &msg );
        }
      }
    }

    if( fuse_emulation_paused )
      Sleep( 25 );

  } while( fuse_emulation_paused );

  return 0;
  /* finish - somwhere there should be return msg.wParam */
}

int
ui_end( void )
{
  int error;

  error = win32display_end(); if( error ) return error;
   
  /* close the monospaced font handle */     
  if( monospaced_font ) {
    DeleteObject( monospaced_font );
    monospaced_font = NULL;
  }
        
  return 0;
}

/* Create a dialog box with the given error message */
int
ui_error_specific( ui_error_level severity, const char *message )
{
  /* finish - can ui be not initialized? */
  HWND hWnd;

  fuse_emulation_pause();

  hWnd = GetActiveWindow();

  switch( severity ) {

  case UI_ERROR_INFO:
    MessageBox( hWnd, message, "Fuse - Info", MB_ICONINFORMATION | MB_OK );
    break;
  case UI_ERROR_WARNING:
    MessageBox( hWnd, message, "Fuse - Warning", MB_ICONWARNING | MB_OK );
    break;
  case UI_ERROR_ERROR:
    MessageBox( hWnd, message, "Fuse - Error", MB_ICONERROR | MB_OK );
    break;
  default:
    MessageBox( hWnd, message, "Fuse - (Unknown Error Level)",
                MB_ICONINFORMATION | MB_OK );
    break;

  }

  fuse_emulation_unpause();

  return 0;
}

/* Called by the menu when File/Exit selected */
void
menu_file_exit( int action )
{
 /* FIXME: this should really be sending WM_CLOSE, not duplicate code */
  if( win32ui_confirm( "Exit Fuse?" ) ) {

    if( menu_check_media_changed() ) return;

    DestroyWindow(fuse_hWnd);
  }
}

/* Select a graphics filter from those for which `available' returns
   true */
scaler_type
menu_get_scaler( scaler_available_fn selector )
{
	/* FIXME if fuse hasn't been resized prior to opening, Filters... dialog
	         shows all scalers independently of current window size */

  scaler_type selected_scaler = SCALER_NUM;
  win32ui_select_info items;
  int count, i, selection;
  scaler_type scaler;

  /* Get count of currently applicable scalars first */
  count = 0; 
  for( scaler = 0; scaler < SCALER_NUM; scaler++ ) {
    if( selector( scaler ) ) count++;
  }

  /* Populate win32ui_select_info */
  items.dialog_title = TEXT( "Fuse - Select Scaler" );
  items.labels = malloc( count * sizeof( char * ) );
  items.length = count; 

  /* Populate the labels with currently applicable scalars */
  count = 0;
  
  for( scaler = 0; scaler < SCALER_NUM; scaler++ ) {

    if( !selector( scaler ) ) continue;

    items.labels[ count ] = scaler_name( scaler );

    if( current_scaler == scaler ) {
      items.selected = count;
    }

    count++;
  }

  /* Start the selection dialog box */
  selection = selector_dialog( &items );
  
  if( selection >= 0 ) {
    /* Apply the selected scalar */
    count = 0;
    
    for( i = 0; i < SCALER_NUM; i++ ) {
      if( !selector( i ) ) continue;
      	
      if( selection == count ) {
      	selected_scaler = i;
      }
  
      count++;
    }
  }
	
  free( items.labels );

  return selected_scaler;
}

/* Machine/Pause */
void
menu_machine_pause( int action )
{
  int error;

  if( paused ) {
    paused = 0;
    ui_statusbar_update( UI_STATUSBAR_ITEM_PAUSED,
                         UI_STATUSBAR_STATE_INACTIVE );
    timer_estimate_reset();
    fuse_emulation_unpause();
  } else {

    /* Stop recording any competition mode RZX file */
    if( rzx_recording && rzx_competition_mode ) {
      ui_error( UI_ERROR_INFO, "Stopping competition mode RZX recording" );
      error = rzx_stop_recording(); if( error ) return;
    }

    paused = 1;
    ui_statusbar_update( UI_STATUSBAR_ITEM_PAUSED, UI_STATUSBAR_STATE_ACTIVE );
    fuse_emulation_pause();
  }
}

/* Called by the menu when Machine/Reset selected */
void
menu_machine_reset( int action )
{
  int hard_reset = action;
  
  if( win32ui_confirm( "Reset?" ) && machine_reset( hard_reset ) ) {
    ui_error( UI_ERROR_ERROR, "couldn't reset machine: giving up!" );

    /* FIXME: abort() seems a bit extreme here, but it'll do for now */
    fuse_abort();
  }
}

/* Called by the menu when Machine/Select selected */
void
menu_machine_select( int action )
{
  /* FIXME: choosing spectrum SE crashes Fuse sound_frame () at sound.c:477 "ay_change[f].ofs = ( ay_change[f].tstates * sfreq ) / cpufreq;" */
  /* FIXME: choosing some Timexes crashes (win32) fuse as well */

  int selected_machine;
  win32ui_select_info items;
  int i;

  /* Stop emulation */
  fuse_emulation_pause();

  /* Populate win32ui_select_info */
  items.dialog_title = TEXT( "Fuse - Select Machine" );
  items.labels = malloc( machine_count * sizeof( char * ) );
  items.length = machine_count; 

  for( i=0; i<machine_count; i++ ) {

    items.labels[i] = libspectrum_machine_name( machine_types[i]->machine );

    if( machine_current == machine_types[i] ) {
      items.selected = i;
    }
  }

  /* start the machine select dialog box */
  selected_machine = selector_dialog( &items );
  
  if( machine_types[ selected_machine ] != machine_current ) {
    machine_select( machine_types[ selected_machine ]->machine );
  }

  free( items.labels );

  /* Resume emulation */
  fuse_emulation_unpause();
}

void
menu_machine_debugger( int action )
{
  debugger_mode = DEBUGGER_MODE_HALTED;
  if( paused ) ui_debugger_activate();
}

/* Called on machine selection */
int
ui_widgets_reset( void )
{
  win32ui_pokefinder_clear();
  return 0;
}

void
menu_help_keyboard( int action )
{
  win32ui_picture( "keyboard.scr", 0 );
}

/* Functions to activate and deactivate certain menu items */
static int
set_active( HMENU menu, const char *path, int active )
{
  int i, menu_count;
  char menu_text[255];
  MENUITEMINFO mii;
  
  if( *path == '/' ) path++;

  menu_count = GetMenuItemCount( menu );
  for( i = 0; i < menu_count; i++ ) {

    if( GetMenuString( menu, i, menu_text, 255, MF_BYPOSITION ) == 0 ) return 1;

    const char *p = menu_text, *q = path;

    /* Compare the two strings, but skip hotkey-delimiter characters */
    /* Anything after \t is a shortcut key on Win32 */
    do {
      if( *p == '&' ) p++;
      if( ! *p || *p == '\t' || *p != *q ) break;
      p++; q++;
    } while( 1 );

    if( *p && *p != '\t' ) continue;		/* not matched */

    /* match, but with a submenu */
    if( *q == '/' ) return set_active( GetSubMenu( menu, i ), q, active );

    if( *q ) continue;		/* not matched */

    /* we have a match */
    mii.fState = active ? MFS_ENABLED : MFS_DISABLED;
    mii.fMask = MIIM_STATE;
    mii.cbSize = sizeof( MENUITEMINFO );
    SetMenuItemInfo( menu, i, TRUE, &mii );

    return 0;
  }

  return 1;
}

int
ui_menu_item_set_active( const char *path, int active )
{
  return set_active( GetMenu( fuse_hWnd ), path, active );
}

ui_confirm_joystick_t
ui_confirm_joystick( libspectrum_joystick libspectrum_type, int inputs )
{
  STUB;
  return UI_CONFIRM_JOYSTICK_NONE;
}

int
ui_joystick_init( void )
{
  STUB;
  return 0;
}

void
ui_joystick_end( void )
{
  STUB;
}

void
ui_joystick_poll( void )
{
  /* STUB; */
}

/*
 * Font code
 */

int
win32ui_get_monospaced_font( HFONT *font )
{
  if( ! monospaced_font ) {
    *font = CreateFont( -11, 0, 0, 0, 400, FALSE, FALSE, FALSE, 0, 400, 2,
                            1, 1, TEXT( "Courier New" ) );
    if( !font ) {
      ui_error( UI_ERROR_ERROR, "couldn't find a monospaced font" );
      return 1;
    }
    monospaced_font = *font;
  }
  else {
    *font = monospaced_font;
  }

  return 0;
}

void
win32ui_set_font( HWND hDlg, int nIDDlgItem, HFONT font )
{
  SendDlgItemMessage( hDlg, nIDDlgItem , WM_SETFONT, (WPARAM) font, FALSE );
}  

static INT_PTR CALLBACK
selector_dialog_proc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  int i, pos_y;
  win32ui_select_info *items;
  HWND next_item = NULL;

  switch( uMsg )
  {
    case WM_INITDIALOG: 
    {
      /* items are passed to WM_INITDIALOG as lParam */
      items = ( win32ui_select_info * ) lParam;
      
      /* set the title of the window */
      SendMessage( hwndDlg, WM_SETTEXT, 0, (LPARAM) items->dialog_title );
      
      /* create radio buttons */
      pos_y = 8;
      for( i=0; i< items->length; i++ ) {

        CreateWindow( TEXT( "BUTTON" ), items->labels[i],
                      /* no need for WS_GROUP since they're all in the same group */  
                      WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTORADIOBUTTON,
                      8, pos_y, 155, 16,
                      hwndDlg, (HMENU) ( IDC_SELECT_OFFSET + i ), fuse_hInstance, 0 );
        SendDlgItemMessage( hwndDlg, ( IDC_SELECT_OFFSET + i ), WM_SETFONT,
                            (WPARAM) h_ms_font, FALSE );

        /* check the radiobutton corresponding to current label */
        if( i == items->selected ) {
          SendDlgItemMessage( hwndDlg, ( IDC_SELECT_OFFSET + i ), BM_SETCHECK,
                              BST_CHECKED, 0 );
        }

        pos_y += 16;
      }

      /* create OK and Cancel buttons */
      pos_y += 6;
      CreateWindow( TEXT( "BUTTON" ), TEXT( "&OK" ),
                    WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON,
                    6, pos_y, 75, 23,
                    hwndDlg, (HMENU) IDOK, fuse_hInstance, 0 );
      SendDlgItemMessage( hwndDlg, IDOK, WM_SETFONT,
                          (WPARAM) h_ms_font, FALSE );

      CreateWindow( TEXT( "BUTTON" ), TEXT( "&Cancel" ),
                    WS_VISIBLE | WS_CHILD | WS_TABSTOP,
                    90, pos_y, 75, 23,
                    hwndDlg, (HMENU) IDCANCEL, fuse_hInstance, 0 );
      SendDlgItemMessage( hwndDlg, IDCANCEL, WM_SETFONT,
                          (WPARAM) h_ms_font, FALSE );
      
      pos_y += 54;
      /* the following will only change the size of the window */
      SetWindowPos( hwndDlg, NULL, 0, 0, 177, pos_y,
                    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
      
      return TRUE;
    }
    case WM_SETFONT:
    {
      h_ms_font = (HFONT) wParam;
      return TRUE;
    }
    case WM_COMMAND:
    {
      if ( HIWORD( wParam ) != BN_CLICKED ) return 0;

      /* service OK and Cancel buttons */
      switch LOWORD( wParam )
      {
        case IDOK:
        {
          /* check which radiobutton is selected and return the selection */
          i = 0;
          while( ( next_item = GetNextDlgGroupItem( hwndDlg, next_item,
                                                    FALSE ) ) != NULL ) {
            if( SendDlgItemMessage( hwndDlg, ( IDC_SELECT_OFFSET + i ), 
                                    BM_GETCHECK, 0, 0 ) == BST_CHECKED ) {
              EndDialog( hwndDlg, i );
              return TRUE;
            }
            i++;
          }
          return FALSE; /* program should never reach here */
        }
        case IDCANCEL:
        {
          EndDialog( hwndDlg, 0 );
          return TRUE;
        }
      }
      /* service clicking radiobuttons */
      /* FIXME should also be checking if wParam < offset + radio count */
      if( LOWORD( wParam ) >= IDC_SELECT_OFFSET ) {
        return TRUE;
      }
    }
    case WM_DESTROY:
    {
      EndDialog( hwndDlg, 0 );
      return TRUE;
    }
  }
  return FALSE;
}

static int
selector_dialog( win32ui_select_info *items )
{
  /* selector_dialog will display a modal dialog, with a list of grouped
     radiobuttons, OK and Cancel buttons. The radiobuttons' labels, their count,
     current selection and dialog title are provided via win32ui_select_info.
     The function returns an int corresponding to the selected radiobutton */
  
  /* FIXME: fix accelerators for this window */
  /* FIXME: switching machines changes filter (typically to 1x) */

  return DialogBoxParam( fuse_hInstance, MAKEINTRESOURCE( IDD_SELECT_DIALOG ),
                         fuse_hWnd, selector_dialog_proc, ( LPARAM )items );
}

void
win32_verror( int is_error )
{
  if( !is_error ) return;

  DWORD last_error;
  static LPVOID err_msg;
  last_error = GetLastError();
  FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                 FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL, last_error, LANG_USER_DEFAULT,
                 (LPTSTR) &err_msg, 0, NULL );
  MessageBox( fuse_hWnd, err_msg, "Error", MB_OK );
}

/* Handler for the main window's WM_PAINT notification.
   The handler is an equivalent of GTK's expose_event */
static int
win32ui_window_paint( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
  blit();
  return 0;
}

/* Handler for the main window's WM_SIZE notification */
static int
win32ui_window_resize( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
  RECT cr;
  GetClientRect( fuse_hWnd, &cr );
  win32display_drawing_area_resize( cr.right - cr.left, cr.bottom - cr.top );

  /* FIXME: statusbar needs to be accounted for in size */
  SendMessage( fuse_hStatusWindow, WM_SIZE, wParam, lParam );

  if( wParam == SIZE_MINIMIZED ) {
    if( !size_paused ) {
      size_paused = 1;
      fuse_emulation_pause();
    }
  } else {
    if( size_paused ) {
      size_paused = 0;
      fuse_emulation_unpause();
    }
  }

  win32statusbar_resize( hWnd );
  return 0;
}

/* Handler for the main window's WM_SIZING notification.
   The handler is an equivalent of setting window geometry in GTK */
static int
win32ui_window_resizing( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
  RECT *selr, wr, cr , statr;
  int width, height, size, w_ofs, h_ofs;

  selr = (RECT *)lParam;
  GetWindowRect( fuse_hWnd, &wr );
  GetClientRect( fuse_hWnd, &cr );
  GetClientRect( fuse_hStatusWindow, &statr );

  w_ofs = ( wr.right - wr.left ) - ( cr.right - cr.left );
  h_ofs = ( wr.bottom - wr.top ) - ( cr.bottom - cr.top )
          + ( statr.bottom - statr.top );

  width = selr->right - selr->left + DISPLAY_ASPECT_WIDTH / 2;
  height = selr->bottom - selr->top + DISPLAY_SCREEN_HEIGHT / 2;

  width -= w_ofs; height -= h_ofs;
  width /= DISPLAY_ASPECT_WIDTH; height /= DISPLAY_SCREEN_HEIGHT;

  if( wParam == WMSZ_LEFT || wParam == WMSZ_RIGHT ) {
    height = width;
  } else if( wParam == WMSZ_TOP || wParam == WMSZ_BOTTOM ) {
    width = height;
  } else if( width < 1 || height < 1 ) {
    width = 1; height = 1;
  }

  if( width > 3 ) {
    width = 3;
  }
  if( height > 3 ) {
    height = 3;
  }

  if( width < height ) {
    height = width;
  } else {
    width = height;
  }
  size = width;

  width *= DISPLAY_ASPECT_WIDTH; height *= DISPLAY_SCREEN_HEIGHT;
  width += w_ofs; height += h_ofs;

  if( wParam == WMSZ_TOP ||
      wParam == WMSZ_TOPLEFT ||
      wParam == WMSZ_TOPRIGHT ) {
    selr->top = selr->bottom - height;
  } else {
    selr->bottom = selr->top + height;
  }
  if( wParam == WMSZ_LEFT ||
      wParam == WMSZ_TOPLEFT ||
      wParam == WMSZ_BOTTOMLEFT ) {
    selr->left = selr->right - width;
  } else {
    selr->right = selr->left + width;
  }

  /* FIXME: function is defined to return int,
            check which one does window proc expect */
  return TRUE;
}
