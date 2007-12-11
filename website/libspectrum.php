<?php

/* libspectrum.php: the libspectrum 'home' page
   Copyright (c) 1999-2007 Darren Salt, Philip Kendall

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

include ".inc/fuse.inc";

fuse_title ("libspectrum - emulator support library");
fuse_menu_heading ("libspectrum");

#<!-- ======================================================= -->

fuse_section (NULL, "What is it?", <<<END_SECTION
  <p>libspectrum is a library designed to make the input and output of some ZX Spectrum emulator files slightly easier. It is intended to be usable on Unix variants, Win32 and Mac OS X. Currently, it is mainly (only?) used by <a href="fuse.php">Fuse</a>, but other Spectrum emulator and utility authors are encouraged to use it.</p>

END_SECTION
);

#<!-- ======================================================= -->

fuse_section ("Supported formats", "Which formats does it support?", <<<END_SECTION
  <ul>
   <li>Snapshots:
    <ul>
     <li>read/write: .z80, .szx, .sna</li>
     <li>read only: .zxs, .sp., .snp, +D snapshots</li>
    </ul>
   </li>
   <li>Tape images:
    <ul>
     <li>read/write: .tzx, .tap</li>
     <li>read only: Warajevo .tap, Z80Em, CSW version 1</li>
    </ul>
   </li>
   <li>Input recordings:
    <ul><li>read/write: .rzx</li></ul>
   </li>
   <li>Timex cartridges:
    <ul><li>read only: .dck</li></ul>
   </li>
   <li>IDE hard disk images:
    <ul><li>read/write: .hdf</li></ul>
   </li>
   <li>Microdrive cartridge images:
    <ul><li>read/write: .mdr</li></ul>
   </li>
   <li>Compressed files: reading of gzipped and bzipped files is transparently supported.</li>
  </ul>

  <p>There are also some useful utility routines to do things like identify Spectrum files, or give the timings for various machines. For more details, see the <a href="libspectrum.txt">supplied documentation</a>.</p>

END_SECTION
);

#<!-- ======================================================= -->

fuse_section ("Download", "Downloads", <<<END_SECTION

  <p>libspectrum is licensed under the <a href="COPYING">GNU General Public License</a>. Please read this before downloading libspectrum if you're not already familiar with it.</p>

  <h3>Source</h3>
  <ul>
   <li><a href="http://prdownloads.sourceforge.net/fuse-emulator/libspectrum-0.3.0.1.tar.gz?download">libspectrum-0.3.0.1.tar.gz</a> (<a href="http://prdownloads.sourceforge.net/fuse-emulator/libspectrum-0.3.0.1.tar.gz.sig?download">PGP signature</a>) or from <a href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/unix/libspectrum-0.3.0.1.tar.gz">World of Spectrum</a> (<a href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/unix/libspectrum-0.3.0.1.tar.gz.sig">signature</a>).</li>
   <li>The source code release above is signed with the <a href="http://wwwkeys.pgp.net:11371/pks/lookup?op=get&amp;search=0xD0767AB9">Fuse Release Key, ID <tt>D0767AB9</tt></a>, which has fingerprint <tt>064E 0BA9 688F 2699 3267 B1E5 1043 EEEB D076 7AB9</tt>. This is different from the key used to sign the 0.1.1 release as I forgot the passphrase for that key <tt>:-(</tt>.</li>
  </ul>

  <h3>Binaries</h3>
  <p>Packages are available for some Unix distributions; in general, any problems which are specific to the packages should be sent to the package maintainer.</p>
  <ul>
   <li><a href="http://www.youmustbejoking.demon.co.uk/progs.stable.html">Debian stable</a> (Debian 4.0, "Etch"), <a href="http://www.youmustbejoking.demon.co.uk/progs.testing.html">testing</a>, <a href="http://www.youmustbejoking.demon.co.uk/progs.unstable.html">unstable</a>, <a href="http://www.youmustbejoking.demon.co.uk/progs.oldstable.html">Debian 3.1 ("Sarge")</a> (all 0.2.2) or <a href="http://www.youmustbejoking.demon.co.uk/progs.woody.html">Debian 3.0 ("Woody")</a> (0.2.0.1) by Darren Salt.</li>
   <li><a href="https://admin.fedoraproject.org/pkgdb/packages/name/libspectrum">Fedora</a> (0.3.0.1)</li>
   <li><a href="http://www.FreeBSD.org/cgi/cvsweb.cgi/ports/emulators/libspectrum/">FreeBSD</a> (0.3.0.1)</li>
   <li><a href="http://packages.gentoo.org/packages/?category=app-emulation;name=libspectrum">Gentoo</a> (0.2.2)</li>
   <li><a href="http://www.mandriva.com/">Mandriva</a> 10.0 packages of 0.3.0.1 are available from the <a href="http://plf.zarb.org/">PLF</a>, or some <a href="http://www.speccy.org/sinclairmania/arch/emu/mandrake/">older packages</a> from Miguel Barrio Orsikowsky.</li>
   <li><a href="http://www.netbsd.org/">NetBSD</a> has 0.3.0.1 available as <a href="http://cvsweb.netbsd.org/bsdweb.cgi/pkgsrc/emulators/libspectrum/">emulators/libspectrum</a> in <a href="http://www.pkgsrc.org/">pkgsrc</a>.</li>
   <li><a href="http://www.openbsd.org/">OpenBSD</a> users can find a port of 0.2.2 in <a href="http://www.openbsd.org/cgi-bin/cvsweb/ports/devel/libspectrum/">ports/devel/libspectrum</a>.</li>
   <li><a href="http://en.docs.pld-linux.org/">PLD</a> users have a package of 0.2.1 available.</li>
   <li><a href="mailto:spec(at)webtech(dot)pl">Marek Januszewski</a> has made a <a href="libspectrum-0.2.0.1-i386-1spec.tgz">Slackware 9.1</a> (<a href="libspectrum-0.2.0.1-i386-1spec.tgz.md5">md5 sum</a>) package of 0.2.0.1.</li>
   <li><a href="libspectrum-0.2.2-1.i386.rpm">0.2.2</a> for <a href="http://www.suse.com">SuSE</a> 9.1, with thanks to Cristi&aacute;n Gonz&aacute;lez.</li>
  </ul>

END_SECTION
);

#<!-- ======================================================= -->

fuse_section (NULL, "Development", <<<END_SECTION
  <p>If you're interested in working on libspectrum, subscribe to the <a href="http://lists.sourceforge.net/lists/listinfo/fuse-emulator-devel">fuse-emulator-devel</a> mailing list and/or see the <a href="http://sourceforge.net/projects/fuse-emulator/">project page</a> on SourceForge. If you just want news of new versions, you probably want the (low volume) <a href="http://lists.sourceforge.net/lists/listinfo/fuse-emulator-announce">fuse-emulator-announce</a> list instead.</p>

  <p>Finally, the latest development version should always be available as the 'trunk/libspectrum' directory from the <a href="http://sourceforge.net/svn/?group_id=91293">Subversion repository</a> on SourceForge. Don't expect any support for this, or even that it can be compiled...</p>

END_SECTION
);

#<!-- ======================================================= -->

fuse_related ("Fuse", "fuse.php");

fuse_footer ();
?>
