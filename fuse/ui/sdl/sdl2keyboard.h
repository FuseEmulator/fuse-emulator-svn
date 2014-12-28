/* sdl2keyboard.h: macros for dealing with SDL-SDL2 SDLK_.. difference
   Copyright (c) 2014 Gergely Szasz

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

   E-mail: szaszg@hu.inter.net

*/

#ifndef FUSE_SDL2KEYBOARD_H
#define FUSE_SDL2KEYBOARD_H

#include <config.h>

#ifdef UI_SDL2
#define SDLK_LMETA SDLK_LGUI
#define SDLK_RMETA SDLK_RGUI

#define SDLK_LSUPER -1
#define SDLK_RSUPER -1
#endif

#endif                  /* #ifndef FUSE_SDL2KEYBOARD_H */
