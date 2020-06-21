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

board = get_option('board')
bundle_tgt = custom_target('bundle_tgt',
    command: [ '../tools/bundle.sh', '-b', board, '@OUTPUT@' ],
    output:  'dema-rc-bundle.tar.gz',
    input:   exe_dema_rc)

# TODO: meson >= 0.52 supports an alias in place of run_target
# alias_target('bundle', bundle_tgt)
run_target('bundle',
    command: 'true', depends: bundle_tgt)
