#!/usr/bin/env bash
insn=$1
dst_dir=$2
mkdir -p ${dst_dir}
opcode=$(grep ^DECLARE_INSN\s*\(\s*${insn}\s*\, ./encoding.h | sed "s/DECLARE_INSN(.*,\(.*\),.*)/\1/")
sed "s/NAME/${insn}/" ./insn_template.cc | sed "s/OPCODE/${opcode}/" > ${dst_dir}/${insn}.cc