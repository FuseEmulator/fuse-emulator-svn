#!/usr/bin/perl -w

# keysyms.pl: generate keysyms.c from keysyms.dat
# Copyright (c) 2000-2004 Philip Kendall, Matan Ziv-Av, Russell Marks,
#			  Fredrick Meunier

# $Id$

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 49 Temple Place, Suite 330, Boston, MA 02111-1307 USA

# Author contact information:

# E-mail: pak21-fuse@srcf.ucam.org
# Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

use strict;

use lib '../../perl';

use Fuse;

my $ui = shift;
$ui = 'gtk' unless defined $ui;

die "$0: unrecognised user interface: $ui\n"
  unless 0 < grep { $ui eq $_ } ( 'ggi', 'gtk', 'x', 'svga', 'fb', 'sdl' );

sub fb_keysym ($) {

    my $keysym = shift;

    $keysym =~ tr/a-z/A-Z/;
    substr( $keysym, 0, 4 ) = 'WIN' if substr( $keysym, 0, 5 ) eq 'META_';

    return $keysym;
}

# The GGI keysyms which start with `GIIK', not `GIIUC'
my %ggi_giik_keysyms = map { $_ => 1 } qw(

    AltL
    AltR
    CapsLock
    CtrlL
    CtrlR
    Down
    End
    Home
    HyperL
    HyperR
    Left
    Menu
    MetaL
    MetaR
    PageUp
    PageDown
    Right
    ShiftL
    ShiftR
    SuperL
    SuperR
    Up

);

sub ggi_keysym ($) {

    my $keysym = shift;

    # GGI 'long name' keysyms start with a capitial letter
    $keysym = ucfirst $keysym if length $keysym > 1;

    # and have no underscores
    $keysym =~ s/_//g;

    if( $ggi_giik_keysyms{ $keysym } ) {
	$keysym = "GIIK_$keysym";
    } else {
	$keysym = "GIIUC_$keysym";
    }

    return $keysym;
}

sub sdl_keysym ($) {

    my $keysym = shift;

    if ( $keysym =~ /[a-zA-Z][a-z]+/ ) {
	$keysym =~ tr/a-z/A-Z/;
    }
    $keysym =~ s/(.*)_L$/L$1/;
    $keysym =~ s/(.*)_R$/R$1/;
    
    # All the magic #defines start with `SDLK_'
    $keysym = "SDLK_$keysym";

    return $keysym;
}

sub svga_keysym ($) {
    
    my $keysym = shift;

    $keysym =~ tr/a-z/A-Z/;
    $keysym =~ s/(.*)_L$/LEFT$1/;
    $keysym =~ s/(.*)_R$/RIGHT$1/;
    $keysym =~ s/META$/WIN/;		# Fairly sensible mapping
    $keysym =~ s/^PAGE_/PAGE/;

    # All the magic #defines start with `SCANCODE_'
    $keysym = "SCANCODE_$keysym";
    
    # Apart from this one :-)
    $keysym = "127" if $keysym eq 'SCANCODE_MODE_SWITCH';

    return $keysym;
}

# Parameters for each UI
my %ui_data = (

    fb   => { headers => [ ],
	      # max_length not used
	      skips => { },
	      translations => { 
	          Mode_switch => 'MENU',
	      },
	      function => \&fb_keysym
	    },

    ggi  => { headers => [ 'ggi/ggi.h' ],
	      max_length => 18,
	      skips => { },
	      translations => {
	          Control_L   => 'CtrlL',
		  Control_R   => 'CtrlR',
		  Mode_switch => 'Menu',
		  apostrophe  => 'DoubleQuote',
		  numbersign  => 'Hash',
	      },
	      function => \&ggi_keysym,
	    },

    gtk  => { headers => [ 'gdk/gdkkeysyms.h' ],
	      max_length => 16,
	      skips => { },
	      translations => { },
	      function => sub ($) { "GDK_$_[0]" },
    	    },

    sdl  => { headers => [ 'SDL.h' ],
	      max_length => 15,
	      skips => { map { $_ => 1 } qw( Hyper_L Hyper_R ) },
	      translations => {
		  apostrophe  => 'QUOTE',
		  Caps_Lock   => 'CAPSLOCK',
		  Control_L   => 'LCTRL',	 
		  Control_R   => 'RCTRL',	 
		  equal       => 'EQUALS',
		  numbersign  => 'HASH',
		  Mode_switch => 'MENU',
		  Page_Up     => 'PAGEUP',
		  Page_Down   => 'PAGEDOWN',
	      },
	      function => \&sdl_keysym,
	    },

    svga => { headers => [ 'vgakeyboard.h' ],
	      max_length => 26,
	      skips => { map { $_ => 1 } qw( Hyper_L Hyper_R
					     Super_L Super_R ) },
	      translations => {
		  Caps_Lock  => 'CAPSLOCK',
		  numbersign => 'BACKSLASH',
		  Return     => 'ENTER',
		  Left       => 'CURSORBLOCKLEFT',
		  Down       => 'CURSORBLOCKDOWN',
		  Up         => 'CURSORBLOCKUP',
		  Right      => 'CURSORBLOCKRIGHT',
	      },
	      function => \&svga_keysym,
	    },

    x    => { headers => [ 'X11/keysym.h' ],
	      max_length => 15,
	      skips => { },
	      translations => { },
	      function => sub ($) { "XK_$_[0]" },
	    },
);

# Translation table for any UI which uses keyboard mode K_MEDIUMRAW
my @cooked_keysyms = (
    # 0x00
    undef, 'ESCAPE', '1', '2', '3', '4', '5', '6',
    '7', '8', '9', '0', 'MINUS', 'EQUAL', 'BACKSPACE', 'TAB',
    # 0x10
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', 'BRACKETLEFT', 'BRACKETRIGHT', 'RETURN', 'CONTROL_L', 'A', 'S',
    # 0x20
    'D', 'F', 'G', 'H', 'J', 'K', 'L', 'SEMICOLON',
    'APOSTROPHE', 'GRAVE', 'SHIFT_L', 'NUMBERSIGN', 'Z', 'X', 'C', 'V',
    # 0x30
    'B', 'N', 'M', 'COMMA', 'PERIOD', 'SLASH', 'SHIFT_R', 'KB_MULTIPLY',
    'ALT_L', 'SPACE', 'CAPS_LOCK', 'F1', 'F2', 'F3', 'F4', 'F5',
    # 0x40
    'F6', 'F7', 'F8', 'F9', 'F10', 'NUM_LOCK', 'SCROLL_LOCK', 'KP_7',
    'KP_8', 'KP_9', 'KP_MINUS', 'KP_4', 'KP_5', 'KP_6', 'KP_PLUS', 'KP_1',
    # 0x50
    'KP_2', 'KP_3', 'KP_0', 'KP_DECIMAL', undef, undef, 'BACKSLASH', 'F11',
    'F12', undef, undef, undef, undef, undef, undef, undef,
    # 0x60
    'KP_ENTER', 'CONTROL_R', 'KP_DIVIDE', 'PRINT', 'ALT_R', undef, 'HOME','UP',
    'PAGE_UP', 'LEFT', 'RIGHT', 'END', 'DOWN', 'PAGE_DOWN', 'INSERT', 'DELETE',
    # 0x70
    undef, undef, undef, undef, undef, undef, undef, 'BREAK',
    undef, undef, undef, undef, undef, 'WIN_L', 'WIN_R', 'MENU'
);

my @keys;
while(<>) {

    next if /^\s*$/;
    next if /^\s*\#/;

    chomp;

    my( $keysym, $key1, $key2 ) = split /\s*,\s*/;

    push @keys, [ $keysym, $key1, $key2 ]

}

my $define = uc $ui;

print Fuse::GPL(
    'keysyms.c: UI keysym to Fuse input layer keysym mappings',
    "2000-2004 Philip Kendall, Matan Ziv-Av, Russell Marks,\n" .
    "                           Fredrick Meunier, Catalin Mihaila"  ),
    << "CODE";

/* This file is autogenerated from keysyms.dat by keysyms.pl.
   Do not edit unless you know what you're doing! */

#include <config.h>

#ifdef UI_$define

#include "input.h"
#include "keyboard.h"

CODE

# Comment to unbreak Emacs' perl mode

foreach my $header ( @{ $ui_data{$ui}{headers} } ) {
    print "#include <$header>\n";
}

print "\nkeysyms_map_t keysyms_map[] = {\n\n";

KEY:
foreach( @keys ) {

    my( $keysym ) = @$_;

    next if $ui_data{$ui}{skips}{$keysym};

    my $ui_keysym = $keysym;

    $ui_keysym = $ui_data{$ui}{translations}{$keysym} if
	$ui_data{$ui}{translations}{$keysym};

    $ui_keysym = $ui_data{$ui}{function}->( $ui_keysym );

    if( $ui eq 'svga' and $ui_keysym =~ /WIN$/ ) {
	print "#ifdef $ui_keysym\n";
    }

    if( $ui eq 'fb' ) {

	for( my $i = 0; $i <= $#cooked_keysyms; $i++ ) {
	    next unless defined $cooked_keysyms[$i] and
			$cooked_keysyms[$i] eq $ui_keysym;
	    printf "  { %3i, INPUT_KEY_%-11s },\n", $i, $keysym;
	    last;
	}

    } else {

	printf "  { %-$ui_data{$ui}{max_length}s INPUT_KEY_%-11s },\n",
            "$ui_keysym,", $keysym;

	if( $ui eq 'ggi' and $ui_keysym =~ /GIIUC_[a-z]/ ) {
#	    printf "  { %-$ui_data{$ui}{max_length}s KEYBOARD_%-9s KEYBOARD_%-6s },\n",
#	    uc $keysym . ",", "$key1,", $key2;
	}

    }

    if( $ui eq 'svga' and $ui_keysym =~ /WIN$/ ) {
	print "#endif                          /* #ifdef $keysym */\n";
    }

}

print << "CODE";

  { 0, 0 }			/* End marker: DO NOT MOVE! */

};

#endif
CODE
