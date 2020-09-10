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

  def __init__(self, ymlfile, linkerscript, makefile):
    self.ymlfile = ymlfile
    self.linkerscript = linkerscript
    self.makefile = makefile
    self.CFLAGS = ""
    self.num_comps = 0
    self.max_comp_namelen = 32

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
    comp_idx = 0
    for compartment in compartments:
      phdre_string += '\t' + compartment + " PT_LOAD FLAGS(" +\
                        str(hex(0xf800 + comp_idx)) + ");\n"
      phdre_string += '\t' + compartment+".symtab" + " PT_LOAD FLAGS(" +\
                        str(hex(0xfc00 + comp_idx)) + ");\n"
      comp_idx += 1

    return phdre_string

  def comps_gen_c_table(self, compartments):
    c_array = "char comp_strtab["+str(len(compartments))+"]["+str(self.max_comp_namelen)+"] = {\n"\

    for compartment in compartments:
      c_array += '\"' + compartment + '\",\n' \

    c_array += "};\n"\

    with open("comp_strtab_generated.c", "w") as file:
      file.write(c_array)

  def comp_gen_assembly_file(self, compartment):
    assembly_string = ""
    assembly_string += ".section " + compartment + "\n" \
                       ".incbin \"" + compartment + ".o\"\n"

    with open(compartment + ".S", "w") as file:
      file.write(assembly_string)

  def linkcmd_add_comp_sections(self, compartments):
    section_string = ""
    for compartment in compartments:
      section_string += '.' + compartment + ' : ALIGN(16) {\n' \
                       "\t" + compartment + "*\n" \
                       "\t} > dmem :" + compartment + "\n"
      #section_string += '.' + compartment+".symtab" + ' : {\n' \
      #                 "\t. = ALIGN(16);\n" \
      #                 "\t(" + compartment + ".symtab)\n" \
      #                 "\t} > dmem :" + compartment+".symtab" + "\n"
                       #"\t_" + compartment + "_strtab_start = .;\n" \
                       #"\t (" + compartment + ".strtab)\n" \
                       #"\t_" + compartment + "_strtab_end = .;\n"
    return section_string

  def linkcmd_add_compartments_libs(self, compartments):
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

    new_output = ""
    with open(self.makefile, "r") as file:
      make =  file.readlines()
      for line in make:
        if "COMPARTMENTS ?=" in line:
          new_output += "COMPARTMENTS = " + " ".join([compartment+".a" for compartment in compartments])
          #new_output += "COMPARTMENTS_NUM = " + str(self.num_comps) + "\n"
          new_output += "\nCFLAGS = -DconfigCOMPARTMENTS_NUM=" + str(self.num_comps) + "\n"
          new_output += "\nCFLAGS += -DconfigMAXLEN_COMPNAME=" + str(self.max_comp_namelen) + "\n"
          new_output += "\nCFLAGS += -D__freertos__=1\n"
        else:
          new_output += line

    with open("Makefile", "w") as file:
      file.write(new_output)

  def linkcmd_add_compartments_objs(self, compartments):
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

    new_output = ""
    with open(self.makefile, "r") as file:
      make =  file.readlines()
      for line in make:
        if "COMPARTMENTS ?=" in line:
          #new_output += "COMPARTMENTS = " + " ".join([compartment+".a" for compartment in compartments])
          new_output += "COMPARTMENTS = " + " ".join([compartment+".wrapped.o" for compartment in compartments])
          #new_output += "COMPARTMENTS_NUM = " + str(self.num_comps) + "\n"
          new_output += "\nCFLAGS = -DconfigCOMPARTMENTS_NUM=" + str(self.num_comps) + "\n"
          new_output += "\nCFLAGS += -DconfigMAXLEN_COMPNAME=" + str(self.max_comp_namelen) + "\n"
          new_output += "\nCFLAGS += -D__freertos__=1\n"
        else:
          new_output += line

    with open("Makefile", "w") as file:
      file.write(new_output)

  def llvm_compile_file(self, source_file):
      with open(source_file, "r") as file:
        logging.debug("CFLAG= %s\n", self.CFLAGS)
        logging.debug("Compiling %s\n", source_file)
        output = subprocess.call(["clang" + " -c " + source_file + " -target " + "riscv64-unknown-elf " +  self.CFLAGS],
                      stdin =subprocess.PIPE,
                      #stdout=subprocess.PIPE,
                      #stderr=subprocess.PIPE,
                      shell=True,
                      #universal_newlines=True,
                      bufsize=0)
        #logging.debug("The commandline is {}".format(output.args))

  def llvm_compile_files(self, source_files):
        logging.debug("CFLAG= %s\n", self.CFLAGS)
        logging.debug("Compiling %s\n", source_files)
        output = subprocess.call(["clang" + " -c " + ' '.join(source_files) + " -target " + "riscv64-unknown-elf -fpic -fno-pie " +  self.CFLAGS],
                      stdin =subprocess.PIPE,
                      shell=True,
                      bufsize=0)

  def llvm_wrap_obj_in_elf(self, output_obj):

      self.comp_gen_assembly_file(output_obj)

      output = subprocess.call(["clang" + " -o " + output_obj + ".wrapped.o" " -c " + output_obj + ".S" + " -target " + "riscv64-unknown-elf " +  self.CFLAGS],
                      shell=True,
                      bufsize=0)

      #output = subprocess.call(["ld.lld" + " -relocatable -o" + output_obj + ".o.bin " + output_obj + ".o"],
      #              stdin =subprocess.PIPE,
                    #stdout=subprocess.PIPE,
                    #stderr=subprocess.PIPE,
      #              shell=True,
                    #universal_newlines=True,
      #              bufsize=0)

  def llvm_create_obj_from_sources(self, output_obj, srcs):
      logging.debug("CFLAG= %s\n", self.CFLAGS)

      self.llvm_compile_files(srcs)

      objs = [source.replace(".c", ".o") for source in srcs]
      objs = [obj.split("/")[-1] for obj in objs]

      output = subprocess.call(["ld.lld" + " -relocatable -o " + output_obj + ".o " + " ".join(objs)],
                    stdin =subprocess.PIPE,
                    #stdout=subprocess.PIPE,
                    #stderr=subprocess.PIPE,
                    shell=True,
                    #universal_newlines=True,
                    bufsize=0)

  def llvm_create_lib(self, libname, objs):
    logging.debug(objs)

    output = subprocess.call(["llvm-ar" + " rcs " + libname +".a " + ' '.join(objs)],
             stdin =subprocess.PIPE,
             shell=True,
             bufsize=0)
    #logging.debug("The commandline is {}".format(output.args))

    output = subprocess.call(["llvm-objcopy" + " --remove-section=.riscv.attributes " + libname + ".a"],
             stdin =subprocess.PIPE,
             shell=True,
             bufsize=0)

    output = subprocess.call(["llvm-objcopy" + " --set-section-flags .symtab=alloc,load " + libname + ".a"],
             stdin =subprocess.PIPE,
             shell=True,
             bufsize=0)

    #output = subprocess.Popen(["llvm-objcopy" + " --set-section-flags .strtab=alloc,load " + libname + ".a"],
    #         stdin =subprocess.PIPE,
    #         shell=True,
    #         bufsize=0)

    output = subprocess.call(["llvm-objcopy" + " --prefix-alloc-sections=" + libname + " " + libname + ".a "],
             stdin =subprocess.PIPE,
             shell=True,
             bufsize=0)

    output = subprocess.call(["llvm-objcopy" + " --rename-section .strtab=" + libname + ".strtab " + libname + ".a "],
             stdin =subprocess.PIPE,
             shell=True,
             bufsize=0)

  def process(self, sys_desc):
    with open("comp.cflags", "r") as file:
      self.CFLAGS += file.read().replace('\n', '')

    # Get components
    comp_list = []

    self.num_comps += len(sys_desc["compartments"])

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

        self.llvm_create_obj_from_sources(comp_name, source_files)
        self.llvm_wrap_obj_in_elf(comp_name)

    #self.linkcmd_add_compartments_libs(comp_list);
    self.linkcmd_add_compartments_objs(comp_list);

    self.comps_gen_c_table(comp_list)

  def Usage(self):
    pass

  def main(self):
    logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)

    sys_desc = self.open_yml()
    self.process(sys_desc)

c = Compartmentalize("system.yml", "link.ld.in", "Makefile.in")
c.main()
