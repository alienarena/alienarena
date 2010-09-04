
*** Notes for GNU standard distribution and other documents ***

=== README ===
TODO: reference the doc/README.txt
      Might just copy README.txt instead of having all standard doc files 
TODO: add information for packager maintainers
      
=== INSTALL ===
TODO: this usually has the generic documentation for install-sh.
TODO: add dedicated server only build and install
TODO: !!! research install-sh -C option for updating existing installation


--- Linux ---

NOTE: This version does not have a dynamically loadable game.so module
      like previous versions. The game module is statically linked into
      crx and crx-ded.
NOTE: The program name for the dedicated server is now "crx-ded" not "crded"
      as in previous versions.

--- For STANDARD installation:

In the directory where you copied the compressed archive (tarball):
Do:

$ tar -xzf alienarena-7.45.tar.gz
$ cd alienarena-7.45
$ ./configure                  <- configures the make for your system
$ make                         <- builds the programs
$ sudo make install            <- installs files (see below)
$ cd                           <- back to your home directory
$ crx                          <- starts the game

To install in another place, instead of /usr/local, change
the configure step to:

$ ./configure --prefix=/full-path-to-other-place


TODO: add instructions for renaming the executables

This directory may not be in your PATH environment variable, so
to run the game, you may have to:

$ cd /full-path-to-other-place/bin
$ ./crx


Files will installed in these locations:
 
/usr/local/bin/
    crx                       <- client executable
    crx-ded                   <- dedicated server executable

/usr/local/share/alienarena/
    alienarena.png            <- the official Alien Arena icon 
    arena/                    <- 2 files (motd.txt, server.cfg)
    botinfo/                  <- bot data files
    data1                     <- official map and model data (about 700 MB)
    
/usr/local/share/doc/alienarena/
    license.txt   <- existing
    README.txt    <- existing (with minor mods)
    COPYING       <- GPL 2

This directory will be created when the game is first run. Files here have
priority over the read-only files in /usr/local/share/alienarena.

$HOME/.codered                 <- user writeable configuration and data
    arena/                     <- configuration files and downloaded game data
    botinfo/                   <- custom bot data
    data1/                     <- normally empty


--- For ALTERNATE installation:

  Alternate installation is the traditional, single directory in-place install.
  It is probably required for map creation tools to work. It may be the way
  to go, if you update from the Subversion Repository frequently.
  
  The compressed archive should be extracted somewhere in your home directory.
  Let us assume, for example, that the compressed archive is in $HOME/games,
  then files will be installed like this:

$HOME/games/alienarena-7.45
    crx                       <- client executable
    crx-ded                   <- dedicated server executable
    ...everything else...     <- all the extracted files

This directory will be created when the game is first run:

$HOME/.codered                 <- user writeable configuration and data
    arena/                     <- downloaded game data
    botinfo/                   <- custom bot data
    data1/                     <- normally empty

In $HOME/games
Do:  

$ tar -xzf alienarena-7.45.tar.gz
$ cd alienarena-7.45
$ ./configure --enable-alternate-install
$ make
$ make install-alternate

Note: make install-alternate only copies the executables to the top directory.
      The other files remain in-place.

=== COPYING ===
TODO: just a copy of the GNU GPL.
TODO: probably add a note at the top referencing license.txt, README.txt

=== NEWS ===
TODO: this one is probably not useful.

=== CHANGELOG ===
TODO: the release change log should go here. (Subversion can generate a GNU
  style change log, but it would still have to be hand edited).

=== AUTHORS ===
TODO: could reference README.txt, but probably just do not use this one.

=== LICENSE ===
TODO: reference license.txt and README.txt
TODO: append or reference licenses for statically linked packages


=== The official README.txt ===

TODO: Change the Linux install to reference a separate install document.

