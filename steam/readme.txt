Windows:

Uncomment this line in vs2010/include:
#define STEAM_VARIANT 1

Then build the release version in visual studio.

Linux:

z_crc_t was removed from zlib (now just uint32_t) source:https://github.com/zlib-ng/zlib-ng/commit/4db4cfdb5badc8860f7410732b12c45216d709b3
change line 160 of unix/minizip/zip.c: "z_crc_t" to "unsigned long"

METROIDHunter_ used distrobox here, it might work with docker as well.

distrobox create --image registry.gitlab.steamos.cloud/steamrt/scout/sdk scout
distrobox enter scout
./configure
echo "#define STEAM_VARIANT 1" >> config/config.h
sed -i 's/z_crc_t/unsigned\ long/g' source/unix/minizip/zip.c
make


Remarks about alienarena.zip:
It's the last uploaded zip to steam, it doesn't contain all changes of 7.71.7 combined.