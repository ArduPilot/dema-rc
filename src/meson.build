# SPDX-License-Identifier: LGPL-2.1+
# Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com>

exe_dema_rc = executable(
    'dema-rc',
    [
      'array.c',
      'controller.c',
      'event_loop.c',
      'log.c',
      'main.c',
      'remote.c',
      'signal.c',
      'util.c',
    ],
    dependencies: [
      dep_cini,
    ],
    install: true
)

bundle_tgt = custom_target('bundle',
    command: [ '../tools/bundle-sc2.sh', '@OUTPUT@' ],
    output:  'dema-rc-bundle.tar.gz',
    input:   exe_dema_rc)
# meson >= 0.52
# alias_target('bundle', bundle_tgt)
run_target('bundle',
    command: ['echo', 'Creating bundle...'], depends: bundle_tgt)
