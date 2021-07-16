#!/usr/bin/python3

#-
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2021 Hesham Almatary
#
# This software was developed by SRI International and the University of
# Cambridge Computer Laboratory (Department of Computer Science and
# Technology) under DARPA contract HR0011-18-C-0016 ("ECATS"), as part of the
# DARPA SSITH research programme.
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

import pexpect
import sys, os, getopt
import socket, ipaddress
import argparse
from threading import Thread
from time import sleep

def capture_print_udp(print_server, port):
    sock = socket.socket(socket.AF_INET, # Internet
                  socket.SOCK_DGRAM) # UDP
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((print_server, int(port)))
    while True:
        data, addr = sock.recvfrom(4096)
        print("freertos: ", data.decode('utf-8'))
        if "Ended" in data.decode('utf-8'):
            return

def main(argv):
    server = '127.0.0.1'
    print_server = '127.0.0.1'
    ftp_port   = '10021'
    cli_port   = '10023'
    print_port = '45000'
    file_path = ''
    file_name = ''
    buffersize = ''
    totalsize = ''
    iterations = ''

    try:
       opts, args = getopt.getopt(argv,"hf:s:n:b:t:",["ftp-port=","cli-port=,print-server="])
    except getopt.GetoptError:
       print('servers_load.py -f <filetoload> -s <server> [--ftp-port, --cli-port]')
       sys.exit(2)

    for opt, arg in opts:
       if opt == '-h':
           print('servers_load.py -f <filetoload> -s <server> [--ftp-port, --cli-port]')
           sys.exit()
       elif opt == "-f":
           if os.path.exists(arg):
               file_path = arg
               file_name = file_path.split('/')[-1]
           else:
               print("File" + arg + " does not exist")
               sys.exit(2)
       elif opt == "-t":
           totalsize = arg
       elif opt == "-b":
           buffersize = arg
       elif opt == "-n":
           iterations = arg
       elif opt == "-s":
           server = arg
       elif opt == "--cli-port":
           cli_port = arg
       elif opt == "--ftp-port":
           ftp_port = arg
       elif opt == "--print-port":
           print_port = arg
       elif opt == "--print-server":
           print_server = arg

    ftp = pexpect.spawn ('ftp ' + server + ' ' + ftp_port)

    with open("libdl.conf", "w") as conf_file:
        conf_libs = "/lib/" + file_name
        conf_file.write(conf_libs)

    ftp.expect('Name .*: ')
    ftp.sendline('anonymous')
    ftp.expect ('Password:')
    ftp.sendline('')
    ftp.expect('ftp>')
    ftp.sendline('put ' + file_path + ' /lib/'+ file_path.split('/')[-1])
    ftp.expect('ftp>')
    ftp.sendline('put ./libdl.conf /etc/libdl.conf')
    ftp.expect('ftp>')
    ftp.sendline('bye')
    print(ftp.before.decode("utf-8") )

    #print_thread = Thread(target = capture_print_udp, args = (print_server, print_port))
    #print_thread.start()
    #thread.join()

    cli = pexpect.spawn ('telnet ' + server + ' ' + cli_port)
    cli.expect('>')
    cli.sendline('exe /lib/' + file_name + ':main_ipc_benchmark.c.1.o ' + file_name.split('.')[0][3:] + \
                 ' -n ' + iterations + \
                 ' -b ' + buffersize + \
                 ' -t ' + totalsize + ' -q')
    cli.expect('>', timeout= -1)
    print(cli.before.decode("utf-8"))
    #print_thread.join();

    sys.exit(0)

if __name__ == "__main__":
    main(sys.argv[1:])
