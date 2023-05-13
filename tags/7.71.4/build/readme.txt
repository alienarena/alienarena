Steps to create an alien arena build for Steam on linux, using the steam runtime.

Remarks about the ODE folder:
To be able to build AND ragdolls to work correctly, I had to copy all header files to a local folder.
The following did NOT work:

a. Environment variable for include path C_INCLUDE_PATH="/usr/include".
It compiles, but ragdoll animations aren't shown, instead, the screen freezes.
b. -I/usr/include in makefile, it just can't find the ode header files and it doesn't compile
c. Creating a symbolic link in build called "ode", pointing to "/usr/include/ode". Same result as a.


Installation steps
---------------------------------------------------------------------------------------------------------

1. Install steamworks sdk from here:
https://partner.steamgames.com/doc/sdk

2. Download steam runtime from here:
https://github.com/ValveSoftware/steam-runtime


Build preparation steps, probably needed every session
---------------------------------------------------------------------------------------------------------

3. Open a terminal

4. Run 64-bit shell (assuming you installed it in ~/sdk):

> ~/sdk/tools/linux/shell-amd64.sh

5. Setup Steam Runtime chroot (assuming you downloaded it in ~/steam-runtime-master):

> ~/steam-runtime-master/steam-runtime-master/setup-chroot.sh --amd64

6. Install libode-dev under steam runtime
(if this is not in your home folder but on another partition you might need to add it to /etc/schroot/default/fstab)

> sudo schroot --chroot steamrt_scout_amd64 -- apt-get install libode-dev

7. If needed, copy any changed header files from /usr/include/ode to build/ode

8. If needed, create a new config.h in the build folder.
Create it by running configure again for a normal non-steam build, and copy the new config.h to the build folder.
If you recreated it, make sure to add the following to it:

/* Steam version */
#define STEAM_VARIANT 1

9. Make sure debug_build=no inside the makefile.
If not, there will be issues with the console making you look down, and exclusive fullscreen not working.


Build steps
---------------------------------------------------------------------------------------------------------

10. cd into the build folder

11. Do a make clean
> make clean

12. Make
> schroot --chroot steamrt_scout_amd64 -- make

13. Copy alienarena.x86_64 to the steam folder to test it