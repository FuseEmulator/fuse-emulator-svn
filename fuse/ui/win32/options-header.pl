#!/usr/bin/perl -w

# options-header.pl: generate options dialog boxes
# $Id$

# Copyright (c) 2001-2007 Philip Kendall, Stuart Brady, Marek Januszewski

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

# Author contact information:

# E-mail: philip-fuse@shadowmagic.org.uk

use strict;

use Fuse;
use Fuse::Dialog;

die "No data file specified" unless @ARGV;

my @dialogs = Fuse::Dialog::read( shift @ARGV );
my $internal = 1;

$internal = 0 if( @ARGV && shift( @ARGV ) eq 'public' );

if( $internal ) {
    print Fuse::GPL( 'options_internals.h: options dialog boxes',
		 '2001-2002 Philip Kendall' );
} else {
    print Fuse::GPL( 'options.h: options dialog boxes public declarations',
		 '2001-2002 Philip Kendall' );
}
print <<"CODE";

/* This file is autogenerated from options.dat by options-header.pl.
   Do not edit unless you know what you\'re doing! */

CODE

if( $internal ) {
  print << "CODE";
#ifndef FUSE_WIN32OPTIONS_INTERNALS_H
#define FUSE_WIN32OPTIONS_INTERNALS_H

CODE
} else {
  print << "CODE";
#ifndef FUSE_OPTIONS_H
#define FUSE_OPTIONS_H

CODE
}

if( $internal ) {
    my $optnum = 3000;

    foreach( @dialogs ) {

	my $optname = uc( "OPT_$_->{name}" );
	printf "#define IDD_%s %s\n", $optname, $optnum++;

        foreach my $widget ( @{ $_->{widgets} } ) {

	    if( $widget->{type} eq "Checkbox" ) {
		printf "#define IDC_%s_%s %s\n",
		    $optname, uc( $widget->{value} ), $optnum++;
	    } elsif( $widget->{type} eq "Entry" ) {
		printf "#define IDC_%s_%s %s\n",
		    $optname, uc( $widget->{value} ), $optnum++;
		printf "#define IDC_%s_LABEL_%s %s\n",
		    $optname, uc( $widget->{value} ), $optnum++;
	    } elsif( $widget->{type} eq "Combo" ) {
		printf "#define IDC_%s_%s %s\n",
		    $optname, uc( $widget->{value} ), $optnum++;
		printf "#define IDC_%s_LABEL_%s %s\n",
		    $optname, uc( $widget->{value} ), $optnum++;
	    } else {
		die "Unknown type '$widget->{type}'";
	    }

	}
    }
} else {
    foreach( @dialogs ) {
	foreach my $widget ( @{ $_->{widgets} } ) {
	    if( $widget->{type} eq "Combo" ) {
		print <<"CODE";
int option_enumerate_$_->{name}_$widget->{value}( void );

CODE
	    }
	}
    }
}

if( $internal ) {
    print << "CODE";
#endif				/* #ifndef FUSE_WIN32OPTIONS_INTERNALS_H */
CODE
} else {
    print << "CODE";
#endif				/* #ifndef FUSE_OPTIONS_H */
CODE
}

