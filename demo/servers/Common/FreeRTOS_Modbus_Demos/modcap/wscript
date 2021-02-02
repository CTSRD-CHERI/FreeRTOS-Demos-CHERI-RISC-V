#-
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2020 Michael Dodson
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

def options(ctx):
    ctx.load('compiler_c');

    ctx.add_option('--target',
                    action='store',
                    default='freertos',
                    help='Target OS for the build (supported: linux/freertos, default: freertos)')

    ctx.add_option('--endpoint',
                    action='store',
                    default='server',
                    help='Build a Modbus client or server (supported: client/server, default: server)')

def configure(ctx):
    print("Configuring modcap @", ctx.path.abspath())

    ctx.load('compiler_c');

    # ENV - Save options for build stage
    try:
        ctx.env.TARGET = ctx.options.target
    except:
        ctx.env.TARGET = 'freertos'

    # If this wscript is being consumed by a larger project, options() is never called,
    # so we need to set suitable defaults, since ctx.options.target/endpoint will
    # cause an exception.
    try:
        ctx.env.ENDPOINT = ctx.options.endpoint
    except:
        ctx.env.ENDPOINT = 'server'

    # Check for a supported target/endpoint combination
    if ctx.env.TARGET == 'freertos':
        if ctx.env.ENDPOINT != 'server':
            ctx.fatal('Only Modbus servers are supported for FreeRTOS')
    elif ctx.env.TARGET == 'linux':
        if ctx.env.ENDPOINT != 'client':
            ctx.fatal('Only Modbus clients are supported for Linux')
    else:
        ctx.fatal('Unsupported target (only freertos and linux are supported)')

    ctx.env.append_value('INCLUDES', [
        ctx.path.abspath(),
        ctx.path.abspath() + '/include',
        ctx.path.abspath() + '/libmodbus/',
        ctx.path.abspath() + '/libmodbus/src/',
        ctx.path.abspath() + '/libmodbus/include/',
        ctx.path.abspath() + '/libmacaroons/include/',
        ctx.path.abspath() + '/libmodbus_object_caps/include/',
        ctx.path.abspath() + '/libmodbus_network_caps/include/',
        ctx.path.abspath() + '/modbus_benchmarks/include/',
    ])

    # Additional library dependencies if we're targeting freertos
    if ctx.env.TARGET == 'freertos':
        ctx.env.append_value('LIB_DEPS', ['freertos_tcpip', 'virtio'])

def build(bld):
    print("Building modcap")

    MODBUS_CLIENT_DIR = 'modbus_client/'
    LIBMACAROONS_DIR = 'libmacaroons/'
    LIBMODBUS_DIR = 'libmodbus/'
    LIBMODBUS_OBJECT_CAPS_DIR = 'libmodbus_object_caps/'
    LIBMODBUS_NETWORK_CAPS_DIR = 'libmodbus_network_caps/'
    MODBUS_BENCHMARKS_DIR = 'modbus_benchmarks/'

    if bld.env.TARGET == 'linux' and bld.env.ENDPOINT == 'client':
        bld.stlib(features=['c'],
                      source=[
                          LIBMODBUS_DIR + 'src/modbus.c',
                          LIBMODBUS_DIR + 'src/modbus-data.c',
                          LIBMODBUS_DIR + 'src/modbus-helpers.c',
                          LIBMODBUS_DIR + 'src/modbus-tcp.c',
                          LIBMODBUS_DIR + 'src/modbus-rtu.c',
                      ],
                      use=[],
                      target='modbus')

        bld.stlib(
            features=['c'],
            source=[
                LIBMACAROONS_DIR + 'src/base64.c',
                LIBMACAROONS_DIR + 'src/explicit_bzero.c',
                LIBMACAROONS_DIR + 'src/macaroons.c',
                LIBMACAROONS_DIR + 'src/packet.c',
                LIBMACAROONS_DIR + 'src/port.c',
                LIBMACAROONS_DIR + 'src/sha256.c',
                LIBMACAROONS_DIR + 'src/shim.c',
                LIBMACAROONS_DIR + 'src/slice.c',
                LIBMACAROONS_DIR + 'src/timingsafe_bcmp.c',
                LIBMACAROONS_DIR + 'src/tweetnacl.c',
                LIBMACAROONS_DIR + 'src/v1.c',
                LIBMACAROONS_DIR + 'src/v2.c',
                LIBMACAROONS_DIR + 'src/varint.c'
            ],
            use=[],
            target="macaroons")

        bld.stlib(features=['c'],
                  source=[LIBMODBUS_NETWORK_CAPS_DIR + 'src/modbus_network_caps.c'],
                  use=[
                    "macaroons",
                    "modbus"],
                  target="modbus_network_caps")

        bld.stlib(features=['c'],
                  source=[MODBUS_BENCHMARKS_DIR + 'src/microbenchmark.c'],
                  use=["modbus"],
                  target="modbus_benchmarks")

        # build a basic modbus client to test a modbus server
        bld.program(features=['c'],
                      source=[MODBUS_CLIENT_DIR + 'modbus_test_client.c'],
                      use=['modbus'],
                      target='modbus_test_client')

        # build a modbus client to benchmark a modbus server
        bld.program(features=['c'],
                      source=[MODBUS_CLIENT_DIR + 'modbus_test_client.c'],
                      use=[
                        'modbus',
                        'modbus_benchmarks',
                        ],
                      defines=bld.env.DEFINES + ['MODBUS_BENCHMARK=1'],
                      target='modbus_test_client_bench')

        # build a modbus client to test a modbus server with network capabiliies
        bld.program(features=['c'],
                      source=[MODBUS_CLIENT_DIR + 'modbus_test_client.c'],
                      use=[
                        'modbus',
                        'modbus_network_caps'
                        ],
                      defines=bld.env.DEFINES + ['MODBUS_NETWORK_CAPS=1'],
                      target='modbus_test_client_network_caps')

        # build a modbus client to benchmark a modbus server with network capabiliies
        bld.program(features=['c'],
                      source=[MODBUS_CLIENT_DIR + 'modbus_test_client.c'],
                      use=[
                        'modbus',
                        'modbus_benchmarks',
                        'modbus_network_caps'
                        ],
                      defines=bld.env.DEFINES + [
                        'MODBUS_NETWORK_CAPS=1',
                        'MODBUS_BENCHMARK=1'
                        ],
                      target='modbus_test_client_network_caps_bench')

    if bld.env.TARGET == 'freertos' and bld.env.ENDPOINT == 'server':
        bld.stlib(features=['c'],
                  source=[
                      LIBMODBUS_DIR + 'src/modbus.c',
                      LIBMODBUS_DIR + 'src/modbus-data.c',
                      LIBMODBUS_DIR + 'src/modbus-tcp.c',
                      LIBMODBUS_DIR + 'src/modbus-helpers.c'
                  ],
                  use=[
                      "freertos_core",
                      "freertos_bsp",
                      "freertos_tcpip"
                  ],
                  target="modbus")

        bld.stlib(
            features=['c'],
            source=[
                LIBMACAROONS_DIR + 'src/base64.c',
                LIBMACAROONS_DIR + 'src/explicit_bzero.c',
                LIBMACAROONS_DIR + 'src/macaroons.c',
                LIBMACAROONS_DIR + 'src/packet.c', LIBMACAROONS_DIR + 'src/port.c',
                LIBMACAROONS_DIR + 'src/sha256.c', LIBMACAROONS_DIR + 'src/shim.c',
                LIBMACAROONS_DIR + 'src/slice.c',
                LIBMACAROONS_DIR + 'src/timingsafe_bcmp.c',
                LIBMACAROONS_DIR + 'src/tweetnacl.c',
                LIBMACAROONS_DIR + 'src/v1.c', LIBMACAROONS_DIR + 'src/v2.c',
                LIBMACAROONS_DIR + 'src/varint.c'
            ],
            use=[
                "freertos_core",
                "freertos_bsp"
            ],
            target="macaroons")

        if bld.env.PURECAP:
            bld.stlib(features=['c'],
                      source=[LIBMODBUS_OBJECT_CAPS_DIR + 'src/modbus_object_caps.c'],
                      use=[
                          "freertos_core",
                          "freertos_bsp",
                          "freertos_tcpip"
                      ],
                      defines=bld.env.DEFINES + ['MODBUS_OBJECT_CAPS=1'],
                      target="modbus_object_caps")

        bld.stlib(features=['c'],
                  source=[LIBMODBUS_NETWORK_CAPS_DIR + 'src/modbus_network_caps.c'],
                  use=[
                      "freertos_core",
                      "freertos_bsp",
                      "freertos_tcpip",
                      "macaroons"
                  ],
                  target="modbus_network_caps")

        bld.stlib(features=['c'],
                  source=[MODBUS_BENCHMARKS_DIR + 'src/microbenchmark.c'],
                  use=[
                      "modbus",
                      "freertos_core",
                      "freertos_bsp",
                      "freertos_tcpip",
                  ],
                  target="modbus_benchmarks")
