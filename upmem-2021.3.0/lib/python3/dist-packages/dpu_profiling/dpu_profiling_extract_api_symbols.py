#!/usr/bin/env python3
# sudo pip install pyelftools
import sys
import argparse
from collections import defaultdict
from collections import namedtuple
import os

from elftools.common.py3compat import maxint, bytes2str
from elftools.dwarf.descriptions import describe_form_class
from elftools.elf.elffile import ELFFile

def extract_api_symbols_from_file(filename, internal_symbols, output):
	with open(filename, 'rb') as f:
		elffile = ELFFile(f)

		if not elffile.has_dwarf_info():
			print('file has no DWARF info')
			return

		api_symbols = get_api_symbols_from_dynsym(elffile)
		if not api_symbols:
			return

		dwarfinfo = elffile.get_dwarf_info()
		elf_name = os.path.basename(filename).split(".")[0]
		return get_symbols_infos_from_dwarf(dwarfinfo, elf_name, api_symbols, internal_symbols, output)


def get_file_name(lp_header, DIE):
	file_index = DIE.attributes['DW_AT_decl_file'].value
	file_entries = lp_header["file_entry"]

	# File and directory indices are 1-indexed.
	file_entry = file_entries[file_index - 1]
	dir_index = file_entry["dir_index"]

	# A dir_index of 0 indicates that no absolute directory was recorded during
	# compilation; return just the basename.
	if dir_index == 0:
		return file_entry.name.decode()

	directory = lp_header["include_directory"][dir_index - 1]
	return os.path.join(directory, file_entry.name).decode()


def get_api_symbols_from_dynsym(elffile):
	dynsym = elffile.get_section_by_name(".symtab")

	list_api_symbols = []
	try:
		for sym in dynsym.iter_symbols():
			if sym.entry["st_info"]["type"] == "STT_FUNC" and sym.entry["st_info"]["bind"] == "STB_GLOBAL":
				list_api_symbols.append(sym.name)
	except AttributeError as e:
		# There are no list of symbols, return
		print('file has no symbols')
		return

	return list_api_symbols


def get_symbols_infos_from_dwarf(dwarfinfo, elf_name, api_symbols, internal_symbols, output):
	func_library = dict()
	funcInfos = namedtuple('funcInfos', 'filename parameters')

	for CU in dwarfinfo.iter_CUs():
		# Every compilation unit in the DWARF information may or may not
		# have a corresponding line program in .debug_line.
		line_program = dwarfinfo.line_program_for_CU(CU)
		if line_program is None:
			print('  DWARF info is missing a line program for this CU')
			continue

		lp_header = line_program.header
		cur_func_name = ""

		for DIE in CU.iter_DIEs():
			try:
				if DIE.tag == 'DW_TAG_subprogram':
					cur_func_name = ""
					filename = get_file_name(lp_header, DIE)
					# Profile only our functions
					#if not "backends/" in filename:
					#	continue;
					# Do not try to profile inlined functions
					if 'DW_AT_inline' in DIE.attributes:
						continue
					# We only want to track internal functions
					if 'DW_AT_prototyped' not in DIE.attributes:
						continue
					if internal_symbols or DIE.attributes['DW_AT_external'].value == True:
						cur_func_name = bytes.decode(DIE.attributes['DW_AT_name'].value, encoding = "UTF-8")
						if not internal_symbols and cur_func_name not in api_symbols:
							continue

						if cur_func_name not in func_library:
							func_library[cur_func_name] = funcInfos(filename, [])
				elif cur_func_name != "" and DIE.tag == 'DW_TAG_formal_parameter' and 'DW_AT_name' in DIE.attributes:
					param_name = DIE.attributes['DW_AT_name'].value
					func_library[cur_func_name].parameters.append(param_name)

			except KeyError:
				continue
	if output is None:
		return func_library
	else:
		with open(output, "w") as f:
			for func_name, func_info in func_library.items():
				# elf_name must prefix event name otherwise 2 functions with identical names
				# could not be probed with same event name.
				f.write("{};{}_{}__entry={} {}\n".format(args.dwarf, elf_name, func_name, func_name, bytes.decode(b" ".join(func_info.parameters), encoding = "UTF-8")))
				f.write("{};{}_{}__exit={}%return\n".format(args.dwarf, elf_name, func_name, func_name))


if __name__ == '__main__':
	parser = argparse.ArgumentParser(description = 'Add probes on dwarf executable')
	parser.add_argument('--internal-symbols', action = 'store_true',
			    help = 'Add probes on non-exposed symbols too')
	parser.add_argument('--output', default = "symbols_event",
			    help = 'Output file perf library event')
	parser.add_argument("dwarf", help = "Path to the dwarf executable")

	args = parser.parse_args()
	extract_api_symbols_from_file(args.dwarf, args.internal_symbols, args.output)
