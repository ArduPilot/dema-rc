# SPDX-License-Identifier: LGPL-2.1+

project(
        'dema-rc',
        'c',
        version: '1',
        license: 'LGPLv2+',
        default_options: [
                'c_std=gnu11',
        ],
)
project_description = 'Linux Drone Remote Controller'

check_compilation_sh = find_program('tools/meson-check-compilation.sh')
cc = meson.get_compiler('c')

conf = configuration_data()
possible_cc_flags = [
        '-Wextra',
        '-Werror=undef',
        '-Wlogical-op',
        '-Wmissing-include-dirs',
        '-Wold-style-definition',
        '-Wpointer-arith',
        '-Winit-self',
        '-Wfloat-equal',
        '-Wsuggest-attribute=noreturn',
        '-Werror=missing-prototypes',
        '-Werror=implicit-function-declaration',
        '-Werror=missing-declarations',
        '-Werror=return-type',
        '-Werror=incompatible-pointer-types',
        '-Werror=format=2',
        '-Wstrict-prototypes',
        '-Wredundant-decls',
        '-Wmissing-noreturn',
        '-Wimplicit-fallthrough=5',
        '-Wshadow',
        '-Wendif-labels',
        '-Wstrict-aliasing=2',
        '-Wwrite-strings',
        '-Werror=overflow',
        '-Werror=shift-count-overflow',
        '-Werror=shift-overflow=2',
        '-Wdate-time',
        '-Wnested-externs',
        '-ffast-math',
        '-fno-common',
        '-fdiagnostics-show-option',
        '-fno-strict-aliasing',
        '-fvisibility=hidden',
        '-fstack-protector',
        '-fstack-protector-strong',
        '--param=ssp-buffer-size=4',
        '-ffunction-sections',
        '-fdata-sections',
]

# --as-needed and --no-undefined are provided by meson by default,
# run mesonconf to see what is enabled
possible_link_flags = [
        '-Wl,-z,relro',
        '-Wl,-z,now',
	'-Wl,--gc-sections',
]

foreach arg : ['unused-parameter',
               'missing-field-initializers',
               'unused-result',
               'format-signedness',
               'error=nonnull', # work-around for gcc 7.1 turning this on on its own

               # Disable -Wmaybe-uninitialized, since it's noisy on gcc 8 with
               # optimizations enabled, producing essentially false positives.
               'maybe-uninitialized',
              ]
        if cc.has_argument('-W' + arg)
                add_project_arguments('-Wno-' + arg, language : 'c')
        endif
endforeach

add_project_arguments(cc.get_supported_arguments(possible_cc_flags), language: 'c')

link_test_c = files('tools/meson-link-test.c')
foreach arg : possible_link_flags
        have = run_command(check_compilation_sh,
                           cc.cmd_array(), '-x', 'c', arg,
                           '-include', link_test_c).returncode() == 0
        message('Linking with @0@ supported: @1@'.format(arg, have ? 'yes' : 'no'))
        if have
                add_project_link_arguments(arg, language : 'c')
        endif
endforeach

conf.set('_GNU_SOURCE', true)
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('PACKAGE', meson.project_name())

config_h = configure_file(
    output: 'config.h',
    configuration: conf)
add_project_arguments('-include', 'config.h', language : 'c')

subdir('src')
