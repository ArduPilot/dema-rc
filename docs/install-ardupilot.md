# ArduPilot on Disco

Installing ArduPilot on Disco can be done by following their
[documentation](https://ardupilot.org/plane/docs/airframe-disco.html).
Here we deviate a little bit, particularly to use our toolchain that allows
doesn't require ArduPilot to be statically built, which allows using libiio
to read the sonar and us of Parrot's telemetry module to integrate with the
camera.

## Build ArduPilot

Use the same toolchain used for dema-rc, see [previous chapter](install-demarc.md#build-dema-rc-from-source).
Assuming that the toolchain is setup, you already cloned ArduPilot's repo and fetched submodules:

```console
$ ./waf configure --board disco
$ ./waf plane
```

## Connect to Disco

You have 2 options to connect to Disco: either via WiFi or via USB (using rndis, i.e. it will
create an ethernet link over USB):

Method | Command
------ | -------------
WiFi   | `adb connect 192.168.42.1:9050`
USB    | `adb connect 192.168.43.1:9050`

If you choose USB, make sure to activate it first: double press the power
button.

## Copy some additional libraries:

```console
$ adb shell mkdir -p /data/ftp/internal_000/ardupilot/lib
$ adb push \
    /opt/arm-buildroot-linux-gnueabihf_sdk-buildroot/arm-buildroot-linux-gnueabihf/sysroot/usr/lib/libserialport.so.0 \
    /data/ftp/internal_000/ardupilot/lib/libserialport.so.0
$ adb push \
    /opt/arm-buildroot-linux-gnueabihf_sdk-buildroot/arm-buildroot-linux-gnueabihf/sysroot/usr/lib/libiio.so.0 \
    /data/ftp/internal_000/ardupilot/lib/libiio.so.0
```

## Copy arduplane

```console
$ adb push build/disco/bin/arduplane /data/ftp/internal_000/ardupilot/arduplane
```

## Default parameters

To make it easier for first setup, copy
[pre-configured parameters from ArduPilot's repo](https://github.com/ArduPilot/ardupilot/blob/master/Tools/Frame_params/Parrot_Disco/Parrot_Disco.param):

```console
$ adb push Parrot_Disco.param /data/ftp/internal_000/ardupilot/disco.parm
```

Another option is to simply load them from your preferred GCS.


## Manually check if ArduPilot can be executed

Before automating the startup, make sure to run it once on the terminal to
check if everything is working:

```console
$ adb shell
$ kk
$ LD_LIBRARY_PATH=/data/ftp/internal_000/ardupilot/lib \
    /data/ftp/internal_000/ardupilot/arduplane  \
        -A udp:192.168.42.255:14550:bcast \
        -B /dev/ttyPA1 \
        -C udp:192.168.43.255:14550:bcast
```

This should start ArduPilot (so you will hear the buzzer) and telemetry will
be sent via both USB and WiFi. Now you should be able to start your GCS and
communicate with the drone, go through calibration procedures, etc.

## Automatically start ArduPilot

Although possible, it's not provided a way in this documentation to start only
arduplane in Disco. Otherwise switching back and forth becomes troublesome and
error prone. So we keep boot sequences as they are and provide a way to switch
to ardupilot.

On Disco, edit this file: `/usr/bin/apm-plane-disco.sh` and change:

```diff
- exec ${rw_apm} $@
+ exec LD_LIBRARY_PATH=/data/ftp/internal_000/ardupilot/lib ${rw_apm} $@
```

**Press the power button 3 times rapidly**. This will switch from the stock flight
stack to ArduPilot.
