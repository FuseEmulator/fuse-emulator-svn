<?php

/* fuse.php: the main Fuse homepage
   Copyright (c) 1999-2005 Darren Salt, Philip Kendall

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
   <li>A "buggy snapshot" of a Win32 port is available; see the <a href="#What_s_new">What's New</a> section for more details.</li>
   <li>Fuse 0.4.0 was ported to the PocketPC by Anders Holmberg as <a href="http://pocketclive.emuunlim.com/">PocketClive</a>.</li>
   <li>And PocketClive has been ported to the both the <a href="http://www.surrealservices.dsl.pipex.com/PocketClive.html">Nokia Smartphone 2002</a> and <a href="http://www.aooa27.dsl.pipex.com/FuseSP.htm">Smartphone 2003 SE</a>.</li>
  </ul>

END_SECTION
);

#<!-- ======================================================= -->

$img = mk_image('screens/f-large.png', 650, 532, 4);
fuse_section ("Features", "What features does it have?", <<<END_SECTION
  $img
  <ul>
   <li>Working 16K, 48K, 128K, +2, +2A, +3, +3e, SE, TC2048, TC2068, Pentagon 128 and Scorpion ZS 256 emulation, running at true Speccy speed on any computer you're likely to try it on.</li>
   <li>Support for loading from .tzx files.</li>
   <li>Sound (on systems supporting the Open Sound System, SDL or OpenBSD/Solaris's <tt>/dev/audio</tt>).</li>
   <li>Kempston joystick emulation.</li>
   <li>Emulation of the various printers you could attach to the Spectrum.</li>
   <li>Support for the RZX input recording file format, including 'competition mode'.</li>
   <li>Emulation of the Spectrum +3e, ZXATASP and ZXCF IDE interfaces.</li>
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
     <li><a href="http://www.gnu.org/directory/security/libgcrypt.html">libgcrypt</a>: the ability to digitally sign RZX files (note that Fuse requires version 1.1.42 or later).</li>
     <li><a href="http://www.libpng.org/pub/png/libpng.html">libpng</a>: the ability to save screenshots.</li>
     <li><a href="http://xmlsoft.org/">libxml2</a>: the ability to load and save Fuse's current configuration.</li>
     <li><a href="http://www.gzip.org/zlib/">zlib</a>: support for compressed RZX files.</li>
    </ul>
   </dd>
  </dl>

  <h3>Mac OS X</h3>
  <p>A native port to OS X by <a href="mailto:fredm@spamcop.net">Fredrick Meunier</a> is available below, as well as a Spotlight importer for Mac OS X 10.4 Tiger users. Alternatively, the original version of Fuse will compile on OS X 10.3 (Panther) or later.</p>

END_SECTION
);

#<!-- ======================================================= -->

$img = mk_image('screens/f-nongtk.png', 330, 266);
fuse_section ("Download", "Where can I get it from?", <<<END_SECTION
  <p>Fuse is licensed under the <a href="COPYING">GNU General Public License</a>. Please read this before downloading Fuse if you're not already familiar with it.</p>

  <h3>Source</h3>
  $img
  <ul>
   <li>Get the <a href="http://prdownloads.sourceforge.net/fuse-emulator/fuse-0.7.0.tar.gz?download">source code</a> (<a href="http://prdownloads.sourceforge.net/fuse-emulator/fuse-0.7.0.tar.gz.sig?download">PGP signature</a>).</li>
   <li>The utilities which were previously packaged with Fuse are now available in their <a href="http://prdownloads.sourceforge.net/fuse-emulator/fuse-utils-0.7.0.tar.gz?download">own package</a> (<a href="http://prdownloads.sourceforge.net/fuse-emulator/fuse-utils-0.7.0.tar.gz.sig?download">PGP signature</a>). Note that
you'll still need <a href="libspectrum.php">libspectrum</a> installed to run these.</li>
   <li>The above are also mirrored at <a href="http://www.worldofspectrum.org/">World of Spectrum</a>: <a
href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/unix/fuse-0.7.0.tar.gz">Fuse source</a> (<a
href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/unix/fuse-0.7.0.tar.gz.sig">signature</a>), <a href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/unix/fuse-utils-0.7.0.tar.gz">utils source</a> (<a href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/unix/fuse-utils-0.7.0.tar.gz.sig">signature</a>).</li>
   <li>The source code releases above are signed with the <a href="http://wwwkeys.pgp.net:11371/pks/lookup?op=get&amp;search=0xD0767AB9">Fuse Release Key, ID <tt>D0767AB9</tt></a>, which has fingerprint <tt>064E 0BA9 688F 2699 3267 B1E5 1043 EEEB D076 7AB9</tt>. This is different from the key used to sign the 0.6.0(.1) releases as I forgot the passphrase for that key <tt>:-(</tt>.</li>
  </ul>

  <h3>Binaries</h3>
  <p>Packages are available for some Unix distributions; in general, any problems which are specific to the packages should be sent to the package maintainer.</p>
  <ul>
   <li>Unofficial packages for <a href="http://www.debian.org/">Debian</a>, by Darren Salt: <a href="http://www.youmustbejoking.demon.co.uk/progs.stable.html">stable</a> (Debian 3.1, "Sarge"), <a href="http://www.youmustbejoking.demon.co.uk/progs.testing.html">testing</a>, <a href="http://www.youmustbejoking.demon.co.uk/progs.unstable.html">unstable</a> (all 0.7.0) or <a href="http://www.youmustbejoking.demon.co.uk/progs.woody.html">old stable</a> (Debian 3.0, "Woody") (0.6.1.1).</li>
   <li><a href="http://www.freebsd.org/">FreeBSD</a> has a port of 0.7.0 available as <a href="http://www.FreeBSD.org/cgi/cvsweb.cgi/ports/emulators/fuse/">emulators/fuse</a> (but the utilities aren't available?).</li>
   <li><a href="http://www.gentoo.org/">Gentoo</a> users have an <a href="http://packages.gentoo.org/packages/?category=app-emulation;name=fuse">ebuild</a> of 0.7.0 available.</li>
   <li><a href="http://www.mandrake.org/">Mandrake</a> 10.0 packages of 0.7.0 are available from the <a href="http://plf.zarb.org/">PLF</a>, or Miguel Barrio Orsikowsky has some <a href="http://www.speccy.org/sinclairmania/arch/emu/mandrake/">older versions</a> available.</li>
   <li>Mac OS X: <a href="http://prdownloads.sourceforge.net/fuse-emulator/Fuse-0.7.0.1.dmg?download">Fuse binaries</a>, the slightly <a href="http://prdownloads.sourceforge.net/fuse-emulator/fuse-macosx-0.7.0.1.tar.bz2?download">modified Fuse source</a>, or the <a href="http://prdownloads.sourceforge.net/fuse-emulator/fuse-utils-macosx-0.7.0.tar.bz2?download">utilities binaries</a>. These are also mirrored at WoS: <a href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/mac/osx/Fuse-0.7.0.1.dmg">Fuse</a>, <a href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/mac/osx/Fuse-0.7.0.1-src.tar.bz2">Fuse source</a> or <a href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/mac/osx/fuse-utils-0.7.0.tar.bz2">utilities</a>. These require OS X 10.3 or later; if you're using an earlier version of OS X, Fuse 0.6.1.1 will work there: <a href="http://prdownloads.sourceforge.net/fuse-emulator/Fuse-0.6.1.1.dmg?download">Fuse binary</a>, <a href="http://prdownloads.sourceforge.net/fuse-emulator/fuse-macosx-0.6.1.1.tar.gz?download">Fuse source</a> and <a href="http://prdownloads.sourceforge.net/fuse-emulator/fuse-utils-macosx-0.6.1.tar.gz?download">utilities binaries</a>. Mac OS X 10.4 users can also use the <a href="http://prdownloads.sourceforge.net/fuse-emulator/FuseImporter1.0.dmg?download">Fuse Spotlight Importer</a> which allows Spotlight to find ZX Spectrum emulation related files based on metadata in the files. The <a href="http://prdownloads.sourceforge.net/fuse-emulator/FuseImporter1.0.tar.bz2?download">source</a> is also available.</li>
   <li><a href="http://www.netbsd.org/">NetBSD</a> users can get version 0.7.0 from <a href="http://www.pkgsrc.org/">pkgsrc</a> (the NetBSD Packages Collection) as <a href="http://cvsweb.netbsd.org/bsdweb.cgi/pkgsrc/emulators/fuse/">emulators/fuse</a> and <a href="http://cvsweb.netbsd.org/bsdweb.cgi/pkgsrc/emulators/fuse-utils/">emulators/fuse-utils</a>.</li>
   <li><a href="http://www.openbsd.org/">OpenBSD</a> users have version 0.6.2.1 available from ports as <a href="http://www.openbsd.org/cgi-bin/cvsweb/ports/emulators/fuse/">ports/emulators/fuse</a>, and <a
href="http://www.openbsd.org/cgi-bin/cvsweb/ports/emulators/fuse-utils/">ports/emulators/fuse-utils/</a>, with thanks to Alexander Yurchenko.</li>
   <li>The <a href="http://en.docs.pld-linux.org/">Polish Linux Distribution</a> has packages of Fuse 0.6.2.1 and the utilities.</li>
   <li>Ian Chapman has made a <a href="http://packages.amiga-hardware.com/">Fedora Core 2 package</a> of 0.7.0.</li>
   <li><a href="mailto:spec(at)webtech(dot)pl">Marek Januszewski</a> has produced a <a href="http://www.slackware.com/">Slackware</a> 9.1 packages of <a href="fuse-0.6.1.1-i386-1spec.tgz">0.6.1.1</a> (<a href="fuse-0.6.1.1-i386-1spec.tgz.md5">md5 sum</a>) and <a href="fuse-utils-0.6.1-i386-1spec.tgz">the utilities</a> (<a href="fuse-utils-0.6.1-i386-1spec.tgz.md5">md5 sum</a>).</li>
   <li>And Cristi&aacute;n Gonz&aacute;lez has made <a href="http://www.suse.com">SuSE</a> 9.1 packages of 0.7.0 with either the <a href="fuse-0.7.0-2.i386.gtk2.rpm">GTK+ 2.x</a> or <a href="fuse-0.7.0-2.i386.sdl.rpm">SDL</a> user interfaces.</li>
  </ul>

END_SECTION
);

#<!-- ======================================================= -->

fuse_section (NULL, "What's new?", <<<END_SECTION
  <h3>Fuse Spotlight Importer for Mac OS X 10.4 Tiger</h3>
  <p>The <a href="http://prdownloads.sourceforge.net/fuse-emulator/FuseImporter1.0.dmg?download">Fuse Spotlight Importer</a> allows Spotlight to find ZX Spectrum emulation related files based on metadata in the files. The <a href="http://prdownloads.sourceforge.net/fuse-emulator/FuseImporter1.0.tar.bz2?download">source</a> is also available.</p>
   <h3>0.7.0.1 for Mac OS X</h3>
  <p>This is a minor revision that resolves issues with saving .sna snapshots, TR-DOS disk ejection and autoloading TR-DOS and +3 disks.</p>
  <h3>0.7.0</h3>
  <ul>
    <li>Scorpion ZS 256, Spectrum +3e and Spectrum SE support.</li>
    <li>Interface II, ZXATASP and ZXCF support.</li>
    <li>Much improved contended memory and floating bus emulation.</li>
    <li>Improved display timings.</li>
    <li>(fuse-utils) New createhdf utility for creating blank .hdf IDE hard disk images.</li>
  </ul>
  <p>See the <a href="http://fuse-emulator.sourceforge.net/fuse.ChangeLog">ChangeLog</a> for full details.</p>
  <p>A "buggy Win32 snapshot missing a lot of functionality" is also <a href="http://www.castlesofpoland.com/fuse-win32-snapshot.zip">available</a>, with thanks to Marek Januszewski.</p>

END_SECTION
);

#<!-- ======================================================= -->

fuse_section (NULL, "Development", <<<END_SECTION
  <p>If you're just want news of new versions and the like, the (low volume) <a href="http://lists.sourceforge.net/lists/listinfo/fuse-emulator-announce">fuse-emulator-announce</a> list is available. If you're interested in the development of Fuse, this is coordinated via the <a href="http://lists.sourceforge.net/lists/listinfo/fuse-emulator-devel">fuse-emulator-devel</a> list and the <a href="http://sourceforge.net/projects/fuse-emulator/">project page</a> on SourceForge.</p>

  <p>The latest version of Fuse is always available by checking out the 'fuse' module from the <a href="http://sourceforge.net/cvs/?group_id=91293">CVS repository</a> on SourceForge. Note that this isn't guaranteed to compile, let alone work properly. Also, don't expect any support for this version! (You'll also need libspectrum from CVS; this is in the 'libspectrum' module). Similarly, the utilities are available in the 'fuse-utils' module.</p>

  <p>One thing which isn't in the SourceForge tracking system:</p>

  <ul>
   <li>David Gardner has produced a patch to give <a href="fuse-xvideo.patch.bz2">XVideo support</a> for the Xlib UI, allowing arbitrary sized windows.</li>
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
