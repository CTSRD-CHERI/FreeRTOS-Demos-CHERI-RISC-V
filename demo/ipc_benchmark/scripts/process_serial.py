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

import sys
import argparse
import os.path
from os import path
import json
import csv
from json2html import *

# Args
parser = argparse.ArgumentParser(description='Process serial output from a main_ipc_benchmark run.')
parser.add_argument("--input-logfile", help="The file path to read serial output from", default='')
parser.add_argument("--output-csv", help="The file path to write output csv data to", default='output.csv')
parser.add_argument("--output-json", help="The file path to write output json data to", default='output.json')
parser.add_argument("--output-html", help="The file path to write output json data to", default='output.html')

args = parser.parse_args()

if not path.exists(args.input_logfile):
  print('Log file ', args.input_logfile, 'does not exists')
  sys.exit(-1)

hpm_counters = {
  "CYCLE": 0.0,
  "INSTRET": 0.0,
  "": 0.0,
  "REDIRECT": 0.0,
  "BRANCH": 0.0,
  "JAL": 0.0,
  "JALR": 0.0,
  "TRAP": 0.0,
  "LOAD_WAIT": 0.0,
  "CAP_LOAD": 0.0,
  "CAP_STORE": 0.0,
  "ITLB_MISS": 0.0,
  "ICACHE_LOAD": 0.0,
  "ICACHE_LOAD_MISS": 0.0,
  "ICACHE_LOAD_MISS_WAIT": 0.0,
  "DTLB_ACCESS": 0.0,
  "DTLB_MISS": 0.0,
  "DTLB_MISS_WAIT": 0.0,
  "DCACHE_LOAD": 0.0,
  "DCACHE_LOAD_MISS": 0.0,
  "DCACHE_LOAD_MISS_WAIT": 0.0,
  "DCACHE_STORE": 0.0,
  "DCACHE_STORE_MISS": 0.0,
  "LLCACHE_FILL": 0.0,
  "LLCACHE_LLCACHE_FILL_WAIT": 0.0,
  "TAGCACHE_LOAD": 0.0,
  "TAGCACHE_LOAD_MISS": 0.0,
  "LLCACHE_EVICT": 0.0,
  "TAGCACHE_STORE_MISS": 0.0,
  "TAGCACHE_EVICT": 0.0
}

IPC_TYPES = ["TSKNOTIF", "QUEUES"]
QUEUES_SIZES = [pow(2, size) for size in range(11)]
QUEUES_DICT = dict.fromkeys(QUEUES_SIZES, [])
NOTIF_DICT = dict.fromkeys(hpm_counters, [])
IPC_RESULTS = dict.fromkeys(IPC_TYPES, [])

print(QUEUES_SIZES)

for qsize in QUEUES_SIZES:
    QUEUES_DICT[qsize] = dict.fromkeys(hpm_counters, [])
    with open(args.input_logfile) as logfile:
        f = logfile.read().splitlines()
        lines = [line.rstrip('\n') for line in f]
        for line_idx in range(len(lines)):
            line = lines[line_idx]
            if "queue size: " + str(qsize) in line:
                line_idx = line_idx + 1
                line = lines[line_idx]
                for counter in hpm_counters:
                    QUEUES_DICT[qsize][counter] = float(0)
                    line = lines[line_idx]
                    while "HPM" not in line:
                        line_idx = line_idx + 1
                        line = lines[line_idx]
                    if counter+':' in line:
                        QUEUES_DICT[qsize][counter] = float(line.split(':')[1])
                        line_idx = line_idx + 1
            elif "task notif" in line:
                line_idx = line_idx + 1
                line = lines[line_idx]
                for counter in hpm_counters:
                    NOTIF_DICT[counter] = float(0)
                    line = lines[line_idx]
                    while "HPM" not in line:
                        line_idx = line_idx + 1
                        line = lines[line_idx]
                    if counter+':' in line:
                        NOTIF_DICT[counter] = float(line.split(':')[1])
                        line_idx = line_idx + 1
            line_idx = line_idx + 1

IPC_RESULTS["QUEUES"] = QUEUES_DICT
IPC_RESULTS["TSKNOTIF"] = NOTIF_DICT

json_object = json.dumps(IPC_RESULTS, indent = 4)
print(json_object)

with open(args.output_csv,'w') as f:
    w = csv.writer(f)
    w.writerows(hpm_counters.items())

with open(args.output_json, 'w') as outfile:
    json.dump(hpm_counters, outfile, indent = 4)

with open(args.output_html,'w') as f:
    output_html = json2html.convert(json = json_object)
    f.write(output_html)
