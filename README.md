![Gitlab pipeline status (branch)](https://gitlab.com/lucas.de.marchi/dema-rc/badges/master/pipeline.svg)

# dema-rc


Linux Drone Remote Controller

## Binaries


The latest binaries for master branch can be downloaded directly from CI
for SkyController 2: [dema-rc bundle](https://gitlab.com/lucas.de.marchi/dema-rc/-/jobs/artifacts/master/raw/bundle.tar.gz?job=bundle-sc2).
This includes all dependencies and configuration files


## GIT

ssh: `git@github.com:lucasdemarchi/dema-rc.git`

https/gitweb: https://github.com/lucasdemarchi/dema-rc.git


## Requeriments


**Build**:

- meson >= 0.57
- ninja
- gcc

**Runtime**:

- libc: glibc, musl or uClibc
- linux kernel >= 3.10 with config:
  *

## Build instructions


Make sure to have all the submodules in place:

```console
$ git submodule update --init --recursive
```

### Native

```console
$ mkdir build
$ meson setup build
$ ninja -C build
```

### Cross-compile

We maintain a Dockerfile that has all the required dependencies for building
for Parrot Disco drone. It basically it sets up a toolchain under `/opt` in the container.
To build using a container you can use the commands below. Create toolchain:

```console
$ docker build -t disco-builder -f ci/Dockerfile.debian ci/
```

```console
$ mkdir build-armv7
$ docker run -it -v .:/src disco-builder meson setup --cross-file /opt/arm-buildroot-linux-gnueabihf_sdk-buildroot/etc/meson/cross-compilation.conf build-armv7
$ docker run -it -v .:/src ninja -C build-armv7
```

If you prefer, you can also copy the toolchain off the container and just use it in your
own host. Then you don't have to ever use the container again and can delete it.

```console
$ docker create --name dummy -t disco-builder bash
$ docker cp dummy:/opt/arm-buildroot-linux-gnueabihf_sdk-buildroot /opt/
$ docker rm -f dummy
```

And then just build as you would on *native*, but passing the additional `--cross-file` to the setup:

```console
$ mkdir build-armv7
$ meson setup --cross-file /opt/arm-buildroot-linux-gnueabihf_sdk-buildroot/etc/meson/cross-compilation.conf build-armv7
$ ninja -C build-armv7
```

## License

LGPL v2.1+
