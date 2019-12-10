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

Native:

```console
$ mkdir build
$ meson wrap promote subprojects/c-ini/subprojects/c-utf8
$ meson wrap promote subprojects/c-ini/subprojects/c-list
$ meson wrap promote subprojects/c-ini/subprojects/c-rbtree
$ meson setup build
$ ninja -C build
```

Cross-compile:

```console
$ mkdir build-armv7
$ meson wrap promote subprojects/c-ini/subprojects/c-utf8
$ meson wrap promote subprojects/c-ini/subprojects/c-list
$ meson wrap promote subprojects/c-ini/subprojects/c-rbtree
$ meson setup --cross-file /path/to/cross-compilation.conf build
$ ninja -C build-armv7
```

For both native and cross-compiling the wrap commands and setup are needed only
once. We maintain a Dockerfile that has all the required dependencies for
building for Parrot Disco drone. See `ci/Dockerfile.debian` and `.gitlab-ci.yml`
in order to use it. Basically it sets up a toolchain under `/opt` so the setup
command above can be replaced with:

```console
$ meson setup --cross-file /opt/arm-buildroot-linux-gnueabihf_sdk-buildroot/etc/meson/cross-compilation.conf build
```

License
-------

LGPL v2.1+
