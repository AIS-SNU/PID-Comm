#!/usr/bin/env python3

import argparse
import subprocess
import operator

NR_THREADS  = 24
IRAM_OFFSET = 0x80000000

def iram_index_to_addr(index):
	return hex(int(index, 16) * 8 | IRAM_OFFSET)

def addr_to_iram_index(addr):
	return hex((int(addr, 16) & ~IRAM_OFFSET) / 8)

class DpuProfiling:
	def __init__(self, nb_threads, dpu_path, profiling_log_path):
		self.total_occurence = 0
		self.total_occurence_per_thread = [ x for x in range(nb_threads) ]
		self.dict_addr_to_function = dict()
		self.dict_function_to_occurence = dict()
		self.dict_function_to_occurence_per_thread = [ dict() for x in range(nb_threads) ]
		self.dpu_path = dpu_path
		self.profiling_log_path = profiling_log_path

	def print_profiling(self, dict_func_occ, total_occ):
		for name, occ in sorted(dict_func_occ.items(), key = operator.itemgetter(1)):
			print("%45s (#occ = %8u) = \t%f" % (name, occ, float(occ) / float(total_occ) * 100))

		print("%45s = \t%u" % ("Total samples", total_occ))


	def print_profiling_per_thread(self):
		for th_id in range(NR_THREADS):
			print("******************")
			print("*** Thread #%2u ***" % th_id)
			print("******************")

			if self.total_occurence_per_thread[th_id] == 0:
				print("No trace available\n")
				continue

			self.print_profiling(self.dict_function_to_occurence_per_thread[th_id], self.total_occurence_per_thread[th_id])
			print("\n")


	def print_profiling_global(self):
		print("**************")
		print("*** Global ***")
		print("**************")

		if self.total_occurence == 0:
			print("No trace available\n")
		else:
			self.print_profiling(self.dict_function_to_occurence, self.total_occurence)


	def print_call_sites(self):
		print("******************")
		print("*** Call sites ***")
		print("******************")

		for name, occ in self.dict_function_to_occurence.items():
			str_idx = ""
			for addr, func_name in self.dict_addr_to_function.items():
				if name == func_name:
					str_idx = str_idx + " " + addr_to_iram_index(addr)
			print("%45s: %s" % (name, str_idx))


	def extract_profiling(self):
		# Extract profiling output
		dict_occurence = [ dict() for x in range(NR_THREADS) ]
		dict_total_occurence = dict()
		is_total = False

		with open(self.profiling_log_path, "r") as f:
			for line in f:
				if "dpu_load" in line:
					if self.dpu_path is None:
						self.dpu_path = line.split(" ")[-1].strip(" \n\"")
				if "profiling_result" not in line:
					continue
				th_id = line.split(': ')[3].rstrip()
				iram_index = line.split(': ')[4].rstrip()
				occurence = line.split(': ')[5].rstrip()
				occurence_int = int(occurence)
				if th_id == "total":
					is_total = True
					dict_total_occurence[iram_index] = occurence_int
				else:
					dict_occurence[int(th_id)][iram_index] = occurence_int

		if is_total:
			self.total_occurence = 0
			for index, occ in dict_total_occurence.items():
				self.total_occurence += occ

			# Sort the dictionary
			sorted_dict_occurence = sorted(dict_total_occurence.items(), key=operator.itemgetter(1))

			# Prepare addr2line
			str_addresses = [ str(iram_index_to_addr(x[0])) for x in sorted_dict_occurence ]
			addr2line_params = [ "addr2line", "-e", "{0}".format(self.dpu_path), "-f" ] + str_addresses
			res = subprocess.Popen(addr2line_params, stdout = subprocess.PIPE)

			# Parse the results in the same order
			result = res.stdout.read().splitlines()
			idx_func_in_str_addresses = 0
			for index, occ in sorted_dict_occurence:
				addr = iram_index_to_addr(index)
				function_name = result[idx_func_in_str_addresses].decode("utf-8")
				idx_func_in_str_addresses += 2
				self.dict_addr_to_function[addr] = function_name
				self.dict_function_to_occurence[function_name] = 0

			for index, occ in dict_total_occurence.items():
				addr = iram_index_to_addr(index)
				name = self.dict_addr_to_function[addr]
				self.dict_function_to_occurence[name] += occ

			return

		for th_id in range(NR_THREADS):
			self.total_occurence_per_thread[th_id] = 0
			for index, occ in dict_occurence[th_id].items():
				self.total_occurence_per_thread[th_id] += occ
			self.total_occurence += self.total_occurence_per_thread[th_id]

		# Map addresses to function names
		for th_id in range(NR_THREADS):
			# Sort the dictionary
			sorted_dict_occurence = sorted(dict_occurence[th_id].items(), key=operator.itemgetter(1))

			# Prepare addr2line
			str_addresses = [ str(iram_index_to_addr(x[0])) for x in sorted_dict_occurence ]
			addr2line_params = [ "addr2line", "-e", "{0}".format(self.dpu_path), "-f" ] + str_addresses
			res = subprocess.Popen(addr2line_params, stdout = subprocess.PIPE)

			# Parse the results in the same order
			result = res.stdout.read().splitlines()
			idx_func_in_str_addresses = 0
			for index, occ in sorted_dict_occurence:
				addr = iram_index_to_addr(index)
				function_name = result[idx_func_in_str_addresses].decode("utf-8")
				idx_func_in_str_addresses += 2
				self.dict_addr_to_function[addr] = function_name
				self.dict_function_to_occurence[function_name] = 0
				self.dict_function_to_occurence_per_thread[th_id][function_name] = 0

		for th_id in range(NR_THREADS):
			for index, occ in dict_occurence[th_id].items():
				addr = iram_index_to_addr(index)
				name = self.dict_addr_to_function[addr]
				if name in self.dict_function_to_occurence_per_thread[th_id]:
					self.dict_function_to_occurence_per_thread[th_id][name] += occ
				self.dict_function_to_occurence[name] += occ


def print_dpu_statistics(dpu_binary_path, profiling_log_path, per_thread, call_sites):
	dpu_profiling = DpuProfiling(NR_THREADS, dpu_binary_path, profiling_log_path)
	dpu_profiling.extract_profiling()
	if per_thread:
		dpu_profiling.print_profiling_per_thread()
	dpu_profiling.print_profiling_global()
	if call_sites:
		dpu_profiling.print_call_sites()

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description = 'Extract profiling data')
	parser.add_argument('--dpu-binary-path', default = "dpu.bin",
			    help = 'Path to dpu elf')
	parser.add_argument('--profiling-log-path', default = "dpu.log",
			    help = 'Path to the log path')
	parser.add_argument('--per-thread', action = 'store_true',
			    help = 'Print per-thread stats in addition to global')
	parser.add_argument('--call-sites', action = 'store_true',
			    help = 'Print call sites (ie calls inside each function)')
	args = parser.parse_args()

	print_dpu_statistics(args.dpu_binary_path, args.profiling_log_path, args.per_thread, args.call_sites)
