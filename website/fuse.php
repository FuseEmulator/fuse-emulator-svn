<?php

/* fuse.php: the main Fuse homepage
   Copyright (c) 1999-2003 Darren Salt, Philip Kendall

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

include ".inc/fuse.inc";

fuse_title ("Fuse - the Free Unix Spectrum Emulator", true);
fuse_menu_heading ("Fuse");

#<!-- ======================================================= -->

$img = mk_image('screens/f-awm.png', 330, 292);
fuse_section (NULL, "What is it?", <<<END_SECTION
  $img
  <p>Fuse (the Free Unix Spectrum Emulator) was originally, and somewhat unsurprisingly, a Spectrum emulator for Unix. However, it has now also been ported to Mac OS X, which may or may not count as a Unix variant depending on your advocacy position.</p>

  <ul>
   <li>There's also been at least <a href="http://groups.google.com/groups?selm=9372C2F8Agasmanrawworg%40127.0.0.1">one report</a> of a very slightly modified version of Fuse compiling under <a href="http://www.cygwin.com/">Cygwin</a>.</li>
   <li>Fuse 0.4.0 was ported to the PocketPC by Anders Holmberg as <a href="http://pocketclive.emuunlim.com/">PocketClive</a>.</li>
   <li>And PocketClive has been ported to the <a href="http://www.davemoller-nz.demon.co.uk/PocketClive.html">Nokia Smartphone</a>.</li>
  </ul>

END_SECTION
);

#<!-- ======================================================= -->

$img = mk_image('screens/f-large.png', 650, 532, 4);
fuse_section ("Features", "What features does it have?", <<<END_SECTION
  $img
  <ul>
   <li>Working 16K, 48K, 128K, +2, +2A, +3, TC2048, TC2068 and Pentagon emulation, running at true Speccy speed on any computer you're likely to try it on.</li>
   <li>Support for loading from .tzx files.</li>
   <li>Sound (on systems supporting the Open Sound System, SDL or OpenBSD/Solaris's <tt>/dev/audio</tt>).</li>
   <li>Kempston joystick emulation.</li>
   <li>Emulation of the various printers you could attach to the Spectrum.</li>
   <li>Support for the RZX input recording file format, including 'competition mode'.</li>
  </ul>

END_SECTION
);

#<!-- ======================================================= -->

fuse_section ("What's lacking?", "What is it lacking?", <<<END_SECTION
  <ul>
   <li>Quite a lot! However, it's a lot better than it used to be...</li>
  </ul>

END_SECTION
);

#<!-- ======================================================= -->

$img = mk_image('screens/f-rzx.png', 330, 266);
fuse_section ("Requirements", "What do I need to run Fuse?", <<<END_SECTION
  <h3> Unix, Linux, BSD etc.</h3>
  $img
  <dl>
   <dt>Required:</dt>
   <dd>
    <ul>
     <li>X, <a href="http://www.libsdl.org/">SDL</a>, <a href="http://www.svgalib.org/">svgalib</a> or framebuffer support. If you have <a href="http://www.gtk.org/">GTK+</a> installed, you'll get a (much) nicer user interface under X.</li>
     <li><a href="libspectrum.php">libspectrum</a>: the Spectrum emulator file format and information library.</li>
    </ul>
   </dd>
   <dt>Optional:</dt>
   <dd>
    <ul>
     <li>If you want +3 support, you'll need John Elliott's lib765 installed; this is available from the bottom of the <a href="http://www.seasip.demon.co.uk/Unix/LibDsk/">libdsk homepage</a>; if you also have libdsk installed, you'll also get support for extended .dsk files.</li>
     <li><a href="http://www.gnu.org/directory/security/libgcrypt.html">libgcrypt</a>: the ability to digitally sign RZX files (note that Fuse requires version <a href="ftp://ftp.gnupg.org/gcrypt/alpha/libgcrypt/libgcrypt-1.1.12.tar.gz">1.1.12</a>, not 1.1.42 or later).</li>
     <li><a href="http://www.libpng.org/pub/png/libpng.html">libpng</a>: the ability to save screenshots.</li>
     <li><a href="http://xmlsoft.org/">libxml2</a>: the ability to load and save Fuse's current configuration.</li>
     <li><a href="http://www.gzip.org/zlib/">zlib</a>: support for compressed RZX files.</li>
    </ul>
   </dd>
  </dl>

  <h3>Mac OS X</h3>
  <p>A port to OS X by <a href="mailto:fredm@spamcop.net">Fredrick Meunier</a> is available below. All comments, bug reports, etc on this version should go to Fred in the first instance.</p>

END_SECTION
);

#<!-- ======================================================= -->

$img = mk_image('screens/f-nongtk.png', 330, 266);
fuse_section ("Download", "Where can I get it from?", <<<END_SECTION
  <p>Fuse is licensed under the <a href="COPYING">GNU General Public License</a>. Please read this before downloading Fuse if you're not already familiar with it.</p>

  <h3>Source</h3>
  $img
  <ul>
   <li>Get the <a href="http://prdownloads.sourceforge.net/fuse-emulator/fuse-0.6.1.1.tar.gz?download">source code</a> (<a href="http://prdownloads.sourceforge.net/fuse-emulator/fuse-0.6.1.1.tar.gz.sig?download">PGP signature</a>).</li>
   <li>The utilities which were previously packaged with Fuse are now available in their <a href="http://prdownloads.sourceforge.net/fuse-emulator/fuse-utils-0.6.1.tar.gz?download">own package</a> (<a href="http://prdownloads.sourceforge.net/fuse-emulator/fuse-utils-0.6.1.tar.gz.sig?download">PGP signature</a>). Note that
you'll still need <a href="libspectrum.php">libspectrum</a> installed to run these.</li>
   <li>The above are also mirrored at <a href="http://www.worldofspectrum.org/">World of Spectrum</a>: <a
href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/unix/fuse-0.6.1.1.tar.gz">Fuse source</a> (<a
href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/unix/fuse-0.6.1.1.tar.gz.sig">signature</a>), <a href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/unix/fuse-utils-0.6.1.tar.gz">utils source</a> (<a href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/unix/fuse-utils-0.6.1.tar.gz.sig">signature</a>).</li>
   <li>The source code releases above are signed with the <a href="http://wwwkeys.pgp.net:11371/pks/lookup?op=get&amp;search=0xD0767AB9">Fuse Release Key, ID <tt>D0767AB9</tt></a>, which has fingerprint <tt>064E 0BA9 688F 2699 3267 B1E5 1043 EEEB D076 7AB9</tt>. This is different from the key used to sign the 0.6.0(.1) releases as I forgot the passphrase for that key <tt>:-(</tt>.</li>
  </ul>

  <h3>Binaries</h3>
  <p>Packages are available for some Unix distributions; in general, any problems which are specific to the packages should be sent to the package maintainer.</p>
  <ul>
   <li>Unofficial packages for <a href="http://www.debian.org/">Debian</a>, by Darren Salt: either <a href="http://www.youmustbejoking.demon.co.uk/progs.stable.html">stable</a> or <a href="http://www.youmustbejoking.demon.co.uk/progs.unstable.html">unstable</a> (both 0.6.1.1).</li>
   <li><a href="http://www.freebsd.org/">FreeBSD</a> has a port of 0.6.1.1 available as <a href="http://www.FreeBSD.org/cgi/cvsweb.cgi/ports/emulators/fuse/">emulators/fuse</a> (but the utilities aren't available?).</li>
   <li><a href="http://www.gentoo.org/">Gentoo</a> users have an <a href="http://www.gentoo.org/cgi-bin/viewcvs.cgi/app-emulation/fuse/">ebuild</a> of 0.6.0.1 available.</li>
   <li>Miguel Barrio Orsikowsky has made <a href="http://www.speccy.org/sinclairmania/arch/emu/mandrake/">Mandrake 9.1 and 9.2</a> packages of version 0.6.1.1 available, as well as some of the libraries needed by Fuse such as lib765 and libdsk.</li>
   <li>Mac OS X: <a href="http://prdownloads.sourceforge.net/fuse-emulator/Fuse-0.6.1.1.dmg?download">Fuse binaries</a>, the slightly <a href="http://prdownloads.sourceforge.net/fuse-emulator/fuse-macosx-0.6.1.1.tar.g
z?download">modified Fuse source</a>, or the <a href="http://prdownloads.sourceforge.net/fuse-emulator/fuse-utils-macosx-0.6.1.t
ar.gz?download">utilities binaries</a>. These are also mirrored at WoS: <a href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/mac/osx/Fuse-0.6.1.1.dmg">Fuse</a>, <a href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/mac/osx/Fuse-0.6.1.1-src.tar.gz">Fuse source</a> or <a href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/mac/osx/fuse-utils-0.6.1.tar.gz">utilities</a>.</li>
   <li><a href="http://www.netbsd.org/">NetBSD</a> has version 0.6.1.1 in <a href="http://cvsweb.netbsd.org/bsdweb.cgi/pkgsrc/emulators/fuse/">emulators/fuse</a> and <a href="http://cvsweb.netbsd.org/bsdweb.cgi/pkgsrc/emulators/fuse-utils/">emulators/fuse-utils</a>.</li>
   <li><a href="http://www.openbsd.org/">OpenBSD</a> users have version 0.6.1.1 available from ports as <a href="http://www.openbsd.org/cgi-bin/cvsweb/ports/emulators/fuse/">ports/emulators/fuse</a>, and <a
href="http://www.openbsd.org/cgi-bin/cvsweb/ports/emulators/fuse-utils/">ports/emulators/fuse-utils/</a>, with thanks to Alexander Yurchenko.</li>
   <li>The <a href="http://www.pld.net.pl/">Polish Linux Distribution</a> has packages of Fuse and the utilities.</li>
   <li>There is a <a href="http://www.unix-city.co.uk/rh9_x86/index.html">RedHat 9 package</a> of 0.6.1.1 (as well as lib765 and libdsk) available, with thanks to Ian Chapman.</li>
   <li>And <a href="mailto:spec(at)webtech(dot)pl">Marek Januszewski</a> has produced a <a href="http://www.slackware.com/">Slackware</a> 9.1 packages of <a href="fuse-0.6.1.1-i386-1spec.tgz">0.6.1.1</a> (<a href="fuse-0.6.1.1-i386-1spec.tgz.md5">md5 sum</a>) and <a href="fuse-utils-0.6.1-i386-1spec.tgz">the utilities</a> (<a href="fuse-utils-0.6.1-i386-1spec.tgz.md5">md5 sum</a>).</li>
  </ul>


END_SECTION
);

#<!-- ======================================================= -->

fuse_section (NULL, "What's new?", <<<END_SECTION
  <h3>0.6.1</h3>
  <ul>
   <li>RZX competition mode.</li>
   <li>Conditional breakpoints in the debugger.</li>
   <li>AY logging.</li>
   <li>Fuse will now use read() and malloc() if mmap() fails; allows Fuse to open files on filesystems which don't support mmap() (e.g. NTFS).</li>
  </ul>

  <h3>0.6.1.1</h3>
  <ul>
   <li>Bugfixes to the RZX code and to allow the SVGAlib user interface to compile.</li>
  </ul>

END_SECTION
);

#<!-- ======================================================= -->

fuse_section (NULL, "Development", <<<END_SECTION
  <p>If you're just want news of new versions and the like, the (low volume) <a href="http://lists.sourceforge.net/lists/listinfo/fuse-emulator-announce">fuse-emulator-announce</a> list is available. If you're interested in the development of Fuse, this is coordinated via the <a href="http://lists.sourceforge.net/lists/listinfo/fuse-emulator-devel">fuse-emulator-devel</a> list and the <a href="http://sourceforge.net/projects/fuse-emulator/">project page</a> on SourceForge.</p>

  <p>A <a href="fuse-cvs.tar.gz">daily snapshot</a> of the current work in progress should always be available. Note that this isn't guaranteed to compile, let alone work properly. Also, don't expect any support for this version! (You'll also need the <a href="libspectrum-cvs.tar.gz">libspectrum CVS snapshot</a>).</p>

  <p>Similarly, get the <a href="fuse-utils-cvs.tar.gz">utilities CVS snapshot</a>.</p>

  <p>A couple of patches which aren't in the SourceForge tracking system:</p>

  <ul>
   <li>David Gardner has produce a patch to give <a href="fuse-xvideo.patch.bz2">XVideo support</a> for the Xlib UI, allowing arbitrary sized windows.</li>
   <li>Catalin Mihaila has sent some code to give Fuse <a href="fuse-allegro.tar.gz">Allegro</a> and <a href="fuse-opengl.tar.gz">OpenGL</a> user interfaces.</li>
  </ul>

END_SECTION
);

#<!-- ======================================================= -->

fuse_section ("Related projects", "Are there any related projects?", <<<END_SECTION
  <ul>
   <li><a href="libspectrum.php">libspectrum</a> is the library used by Fuse to handle various file formats.</li>
   <li>Alexander Shabarshin is working on <a href="http://robots.ural.net/nedopc/sprinter/">SPRINT</a>, an emulator of the <a href="http://www.petersplus.com/">Peters Plus</a> super-Speccy, the <a href="http://www.petersplus.com/sprinter/">Sprinter</a>. SPRINT is using Fuse's Z80 core for its CPU emulation.</li>
   <li>Mike Wynne's ZX81 emulator, <a href="http://www.chuntey.com/eightyone/">EightyOne</a> is also using
Fuse's Z80 core.</li>
  </ul>

END_SECTION
);

#<!-- ======================================================= -->

fuse_related ("libspectrum", "libspectrum.php");

fuse_footer ();
?>
