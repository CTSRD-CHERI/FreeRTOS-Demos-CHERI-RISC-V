#!/usr/bin/python3
# -*- coding: utf-8 -*-

#-
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2020 Hesham Almatary
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

import yaml
import os
import subprocess
import logging, sys

from pathlib import Path

class Compartmentalize:
  yml_files = []

  def __init__(self, ymlfile, linkerscript):
    self.ymlfile = ymlfile
    self.linkerscript = linkerscript
    pass

  def open_yml(self):
    with open(self.ymlfile, "r") as file:
      # The FullLoader parameter handles the conversion from YAML
      # scalar values to Python the dictionary format
      read_file = yaml.load(file, Loader=yaml.FullLoader)
      logging.debug(read_file)
      return read_file

  def open_linkcmd(self):
    with open(self.linkerscript, "r") as file:
      # The FullLoader parameter handles the conversion from YAML
      # scalar values to Python the dictionary format
      linkcmd = file.readlines()
      logging.debug(linkcmd)
      return linkcmd

  def linkcmd_add_comp_phdrs(self, compartments):
    phdre_string = ""
    comp_idx = 1
    for compartment in compartments:
      phdre_string += '\t' + compartment + " PT_LOAD FLAGS(" +\
                        str(hex(0xf800 + comp_idx)) + ");\n"
      comp_idx += 1

    return phdre_string

  def linkcmd_add_comp_sections(self, compartments):
    section_string = ""
    for compartment in compartments:
      section_string += '.' + compartment + ' : {\n' \
                       "\t. = ALIGN(16);\n" \
                       "\t" + compartment + ".a:(*)\n" \
                       "\t} > dmem :" + compartment + "\n"
    return section_string

  def linkcmd_add_compartments(self, compartments):
    logging.debug("Opening a file %s", self.linkerscript)
    new_output = ""
    with open(self.linkerscript, "r") as file:
      linkcmd =  file.readlines()
      for line in linkcmd:
        if "start_compartments" in line:
          new_output += self.linkcmd_add_comp_phdrs(compartments)
          new_output += line
        elif "_compartments_end" in line:
          new_output += self.linkcmd_add_comp_sections(compartments)
          new_output += line
        else:
          new_output += line

    with open("link.ld.generated", "w") as file:
      file.write(new_output)

  def llvm_compile_file(self, source_file):
      with open(source_file, "r") as file:
        with open("comp.cflags", "r") as file:
          CFLAGS = file.read().replace('\n', '')
          logging.debug("CFLAG= %s\n", CFLAGS)
          logging.debug("Compiling %s\n", source_file)
          output = subprocess.Popen(["clang" + " -c " + source_file + " -target " + "riscv64-unknown-elf " +  CFLAGS],
                        stdin =subprocess.PIPE,
                        #stdout=subprocess.PIPE,
                        #stderr=subprocess.PIPE,
                        shell=True,
                        #universal_newlines=True,
                        bufsize=0)
          logging.debug("The commandline is {}".format(output.args))

  def llvm_create_lib(self, libname, objs):
    logging.debug(objs)

    output = subprocess.Popen(["llvm-ar" + " rcs " + libname +".a " + ' '.join(objs)],
             stdin =subprocess.PIPE,
             shell=True,
             bufsize=0)
    logging.debug("The commandline is {}".format(output.args))

  def process(self, sys_desc):
    # Get components
    comp_list = []

    for compartment in sys_desc["compartments"]:
      logging.debug(compartment)

      for comp_name in compartment.keys():
        logging.debug(comp_name)
        #logging.debug("Comp %s sources -> {}", comp_name, compartment[comp_name]["input"])
        comp_list.append(comp_name)

        source_files =  compartment[comp_name]["input"]

        for source_file in source_files:
          self.llvm_compile_file(source_file)

        objs = [source.replace(".c", ".o") for source in source_files]
        objs = [obj.split("/")[-1] for obj in objs]

        logging.debug("Objects -> %s", objs)

        self.llvm_create_lib(comp_name, objs)

    self.linkcmd_add_compartments(comp_list);

  def Usage(self):
    pass

  def main(self):
    logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)

    sys_desc = self.open_yml()
    self.process(sys_desc)

c = Compartmentalize("system.yml", "link.ld.in")
c.main()
