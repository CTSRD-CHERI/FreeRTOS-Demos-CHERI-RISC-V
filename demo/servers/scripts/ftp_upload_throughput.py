#!/usr/bin/python3

#-
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2022 Hesham Almatary
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

import sys
import argparse
import os.path
from os import path
from ftplib import FTP

# Args
parser = argparse.ArgumentParser(description='Process FTP script command.')
parser.add_argument("--server", help="Server/host IP address", default='127.0.0.1')
parser.add_argument("--port", help="FTP port in the server", default=21)
parser.add_argument("--file-size", help="The size to create a file of and upload it to FTP", default=4096)

args = parser.parse_args()

with open('upload_file', 'w') as f:
    num_chars = int(args.file_size)
    f.write('0' * num_chars)
    f.flush()

with open('upload_file', 'rb') as f:
    ftp = FTP(args.server, 'anonymous', '')
    ftp.set_debuglevel(2)
    ftp.set_pasv(True)
    ftp.connect(args.server, int(args.port))
    ftp.login()
    print("Sending the file over")

    ftp.storbinary('STOR /ram/upload_file', f, int(args.file_size))     # send the file
    print("File sent")
    f.close()                                                           # close file and FTP
    ftp.quit()
