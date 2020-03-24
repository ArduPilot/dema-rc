![Gitlab pipeline status (branch)](https://gitlab.com/lucas.de.marchi/dema-rc/badges/master/pipeline.svg)

dema-rc
=======

Linux Drone Remote Controller

Binaries
--------

The latest binaries for master branch can be downloaded directly from CI
for SkyController 2:

- [dema-rc bundle](https://gitlab.com/lucas.de.marchi/dema-rc/-/jobs/artifacts/master/raw/bundle.tar.gz?job=bundle-sk2):
  this includes all dependencies and configuration files
- [dema-rc](https://gitlab.com/lucas.de.marchi/dema-rc/-/jobs/artifacts/master/raw/dema-rc.tar.gz?job=pkg-sk2)

First file contains everything needed to be uploaded to SkyController2.
The second one contains only the dema-rc binary (that can be used to
speed up deployment when changing only the main source code). If in
doubt, use the first.

GIT
---

ssh: `git@github.com:lucasdemarchi/dema-rc.git`

https/gitweb: https://github.com/lucasdemarchi/dema-rc.git


Requeriments
------------

**Build**:

- meson >= 0.44
- ninja
- gcc

**Runtime**:

- libc: glibc, musl or uClibc
- linux kernel >= 3.10 with config:
  *

Build instructions
------------------

Make sure to have all the submodules in place:

```console
$ git submodule update --init --recursive
```

Native:

```console
$ mkdir build
$ meson setup build
$ ninja -C build
```

Cross-compile:

```console
$ mkdir build-armv7
$ meson setup --cross-file /path/to/cross-compilation.conf build
$ ninja -C build-armv7
```

We maintain a Dockerfile that has all the required dependencies for building
for Parrot Disco drone. See `ci/Dockerfile.debian` and `.gitlab-ci.yml` in
order to use it. Basically it sets up a toolchain under `/opt` so the setup
command above can be replaced with:

```console
$ meson setup --cross-file /opt/arm-buildroot-linux-gnueabihf_sdk-buildroot/etc/meson/cross-compilation.conf build-armv7
```

License
-------

LGPL v2.1+
