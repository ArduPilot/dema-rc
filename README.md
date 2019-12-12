dema-rc
=======

Linux Drone Remote Controller


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
