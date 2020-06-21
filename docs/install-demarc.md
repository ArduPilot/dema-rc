# Dema-rc on SC2

Here you will find information on how to get dema-rc installed and configured
on SkyController2. After ensuring you are connected through adb, you have 2 options:
use pre-built binaries or install from source. See details below.

## Connect to SkyController2 via adb

Turn on the controller and connect it to your computer via usb-ethernet device. Connect to it via adb:

```console
$ adb connect 192.168.53.1:9050
```

## Install prebuilt binaries

Currently there is no released version, so just use latest master branch. Binaries
are available directly from CI:

```console
$ curl -JOL https://gitlab.com/lucas.de.marchi/dema-rc/-/jobs/artifacts/master/raw/bundle.tar.gz?job=bundle-sc2
$ curl -JOL https://raw.githubusercontent.com/lucasdemarchi/dema-rc/master/tools/install-sc2.sh
$ chmod +x install-sc2.sh
$ export ANDROID_SERIAL=192.168.53.1:9050
$ ./install-sc2.sh bundle.tar.gz
```

!!! note
    Setting ANDROID_SERIAL environment var is not strictly necessary if adb is
    connected to only one device. If you have more devices connected like,
    e.g. Disco/Bebop, you need to set it though.

Now you should we an output like this:

```
Space requirements on SkyController 2:
/data:  Free:        4344 KB
        Required:    172 KB
/:      Free:        7736 KB
        Required:    8 KB


Installing these files on SkyController 2:
./data/ftp/internal_000/dema-rc/usr/bin/dema-rc-cm
./data/ftp/internal_000/dema-rc/usr/bin/dema-rc
./data/ftp/internal_000/dema-rc/usr/bin/dema-rc-btn
./etc/boxinit.d/99-demarc.rc
./etc/dema-rc/dema-rc.conf

################
SkyController 2
################

Mounting / as rw...
Removing previous installation under /data/ftp/internal_000/dema-rc/usr
/tmp/demarc-install.iKTnE9PN/./: 5 files pushed. 2.2 MB/s (170483 bytes in 0.073s)

Fixing up file permissions...

Installation finished successfully, please reboot SkyController 2
```

## Build and install dema-rc from source

You can also build dema-rc. For that you will need an armv7 toolchain with some
specific tunables. Since we build dema-rc on CI, we already have a recommended
toolchain based on [buildroot](https://buildroot.org/). You can download the
latest one:
[toolchain](https://gitlab.com/lucas.de.marchi/dema-rc/-/jobs/545230428/artifacts/download).

Extract it to `/opt` and add to PATH:

```console
$ export PATH=/opt/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin/:$PATH
```

Configure dema-rc:

```console
$ meson setup \
    -Dboard=sc2 \
    --cross-file /opt/arm-buildroot-linux-gnueabihf_sdk-buildroot/etc/meson/cross-compilation.conf \
    build
```

Build:
```console
$ ninja -C build
```

Create bundle to install on SkyControler2:
```console
$ ninja -C build bundle
```

Install on SkyController2:
```console
$ export ANDROID_SERIAL=192.168.53.1:9050
$ ./tools/install-sc2.sh build/dema-rc-bundle.tar.gz
```

When developing on dema-rc and only modifying the main program, you can skip
the bundle creation and push only that binary:

```console
$ adb push build/src/dema-rc /data/ftp/internal_000/dema-rc/usr/bin/dema-rc
```

Note that the command above installs only the main binary. Check in the install
log above the other files that need to be sent if you modified it.

## Change SSID dema-rc will connect to

As part of the installed files above, we install a script to start, stop and monitor the WiFi
connections. Since SkyController2 will connect to Disco, you need to configure its SSID. The
file is in `/data/ftp/internal_000/ssid.txt`.

## Automatically start dema-rc

dema-rc already installs the integration files to start dema-rc on boot. However it leaves
the original SW stack as the default. To switch to dema-rc to use with ArduPilot, simply
**press the "settings" button** on SkyController2.

The LED that was red/green will now behave as following:

- Blinking blue when dema-rc is started and until it connects to WiFi
- Solid blue while it remains connected to a WiFi network.
