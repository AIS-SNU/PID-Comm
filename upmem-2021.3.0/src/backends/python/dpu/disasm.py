# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilities to disassemble DPU code"""

from dpu.isa import INSTRUCTIONS
from typing import Dict, Optional


class Instruction:
	def __init__(self, signature: str, variables: Dict[str, int]):
		self.signature = signature
		self.variables = variables

	def __str__(self):
		info = INSTRUCTIONS[self.signature]
		disassembled = info.keyword
		parts = []
		for part in info.syntax:
			name = part.name
			if name.endswith('_cc'):
				name = 'cc'
			else:
				name = name.replace('sa', 'ra').replace('sb', 'rb').replace('sc', 'rc')
			parts.append(part.format(self.variables.get(name)))
		if len(parts) != 0:
			disassembled += ' '
		disassembled += ', '.join(parts)
		return disassembled


def disassemble(instruction: int) -> Optional[Instruction]:
	decoded = disassemble_raw(instruction)
	if decoded is not None:
		nr_args = None
		sugar = None
		info = INSTRUCTIONS[decoded.signature]
		for signature, (value, mask) in info.syntactic_sugars.items():
			if (instruction & mask) == value:
				candidate = INSTRUCTIONS[signature]
				candidate_nr_args = len(candidate.syntax)
				if nr_args is None or nr_args > candidate_nr_args:
					nr_args = candidate_nr_args
					sugar = candidate
		if sugar is not None:
			decoded = Instruction(sugar.signature, decoded.variables)
	return decoded


def disassemble_raw(instruction: int) -> Optional[Instruction]:
	variables = {}

	if (((instruction >> 44) & 0xf))==(0x7):
		if (((instruction >> 32) & 0x3))==(0x3):
			if (((instruction >> 42) & 0x3))==(0x3):
				if ((((instruction >> 28) & 0xf))&(0x8))==(0x8):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					variables['pc'] = (((instruction >> 0) & 65535) << 0)
					variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 26) & 31) << 8) | (((instruction >> 39) & 7) << 13)
					if (((instruction >> 24) & 0x3))!=(0x0):
						variables['cc'] = (((instruction >> 24) & 3) << 0)
						return Instruction('acquire:rici', variables)
					if (((instruction >> 24) & 0x3))==(0x0):
						variables['cc'] = (((instruction >> 24) & 3) << 0)
						return Instruction('release:rici', variables)
					return None
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x2):
					if ((((instruction >> 39) & 0x1f))&(0x1b))==(0x1b):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if ((((instruction >> 24) & 0xf))<(0x6)) or ((((instruction >> 24) & 0xf))>(0xb)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
							return Instruction('boot:rici', variables)
						return None
					if ((((instruction >> 39) & 0x1f))&(0x1f))==(0x19):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if ((((instruction >> 24) & 0xf))<(0x6)) or ((((instruction >> 24) & 0xf))>(0xb)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
							return Instruction('clr_run:rici', variables)
						return None
					if ((((instruction >> 39) & 0x1f))&(0x1f))==(0x1c):
						if (((instruction >> 34) & 0x1f))==(0x18):
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0xf))==(0x0):
								variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
								return Instruction('fault:i', variables)
							return None
						return None
					if ((((instruction >> 39) & 0x1f))&(0x1f))==(0x18):
						if ((((instruction >> 28) & 0xf))&(0xf))==(0x3):
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if ((((instruction >> 24) & 0xf))<(0x6)) or ((((instruction >> 24) & 0xf))>(0xb)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
								return Instruction('read_run:rici', variables)
							return None
						return None
					if ((((instruction >> 39) & 0x1f))&(0x1b))==(0x1a):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if ((((instruction >> 24) & 0xf))<(0x6)) or ((((instruction >> 24) & 0xf))>(0xb)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
							return Instruction('resume:rici', variables)
						return None
					if ((((instruction >> 39) & 0x1f))&(0x1f))==(0x1d):
						if (((instruction >> 34) & 0x1f))==(0x1c):
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if ((((instruction >> 24) & 0xf))<(0x6)) or ((((instruction >> 24) & 0xf))>(0xb)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('stop:ci', variables)
							return None
						return None
					return None
				if ((((instruction >> 28) & 0xf))&(0xf))==(0x0):
					if ((((instruction >> 39) & 0x1f))&(0x1f))==(0x18):
						if (((instruction >> 34) & 0x1f))==(0x18):
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0xf))==(0x0):
								if ((((instruction >> 16) & 0xf))&(0xf))==(0x0):
									if ((((instruction >> 0) & 0xffff))&(0xffff))==(0x0):
										return Instruction('nop:', variables)
									return None
								return None
							return None
						if (((instruction >> 37) & 0x3))!=(0x3):
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0xf))==(0x0):
								variables['off'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
								return Instruction('tell:ri', variables)
							return None
						return None
					return None
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x4):
					if (((instruction >> 28) & 0x1))==(0x0):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						variables['off'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 39) & 7) << 4) | (((instruction >> 24) & 1) << 7) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11)
						variables['endian'] = (((instruction >> 27) & 1) << 0)
						if ((((instruction >> 24) & 0xf))&(0x6))==(0x0):
							variables['imm'] = (((instruction >> 16) & 15) << 0) | (((instruction >> 0) & 15) << 4)
							return Instruction('sb:erii', variables)
						if ((((instruction >> 24) & 0xf))&(0x6))==(0x6):
							variables['imm'] = (((instruction >> 16) & 15) << 0) | (((instruction >> 0) & 4095) << 4)
							return Instruction('sd:erii', variables)
						if ((((instruction >> 24) & 0xf))&(0x6))==(0x2):
							variables['imm'] = (((instruction >> 16) & 15) << 0) | (((instruction >> 0) & 4095) << 4)
							return Instruction('sh:erii', variables)
						if ((((instruction >> 24) & 0xf))&(0x6))==(0x4):
							variables['imm'] = (((instruction >> 16) & 15) << 0) | (((instruction >> 0) & 4095) << 4)
							return Instruction('sw:erii', variables)
						return None
					if (((instruction >> 28) & 0x1))==(0x1):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						variables['off'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 39) & 7) << 4) | (((instruction >> 24) & 1) << 7) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11)
						variables['endian'] = (((instruction >> 27) & 1) << 0)
						if ((((instruction >> 24) & 0xf))&(0x6))==(0x0):
							variables['imm'] = (((instruction >> 16) & 15) << 0) | (((instruction >> 0) & 15) << 4)
							return Instruction('sb:esii', variables)
						if ((((instruction >> 24) & 0xf))&(0x6))==(0x6):
							variables['imm'] = (((instruction >> 16) & 15) << 0) | (((instruction >> 0) & 4095) << 4)
							return Instruction('sd:esii', variables)
						if ((((instruction >> 24) & 0xf))&(0x6))==(0x2):
							variables['imm'] = (((instruction >> 16) & 15) << 0) | (((instruction >> 0) & 4095) << 4)
							return Instruction('sh:esii', variables)
						if ((((instruction >> 24) & 0xf))&(0x6))==(0x4):
							variables['imm'] = (((instruction >> 16) & 15) << 0) | (((instruction >> 0) & 4095) << 4)
							return Instruction('sw:esii', variables)
						return None
					return None
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x6):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					variables['off'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 39) & 7) << 4) | (((instruction >> 24) & 1) << 7) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11)
					variables['endian'] = (((instruction >> 27) & 1) << 0)
					if ((((instruction >> 24) & 0xf))&(0x6))==(0x0):
						variables['imm'] = (((instruction >> 16) & 15) << 0) | (((instruction >> 0) & 15) << 4)
						return Instruction('sb_id:erii', variables)
					if ((((instruction >> 24) & 0xf))&(0x6))==(0x6):
						variables['imm'] = (((instruction >> 16) & 15) << 0) | (((instruction >> 0) & 4095) << 4)
						return Instruction('sd_id:erii', variables)
					if ((((instruction >> 24) & 0xf))&(0x6))==(0x2):
						variables['imm'] = (((instruction >> 16) & 15) << 0) | (((instruction >> 0) & 4095) << 4)
						return Instruction('sh_id:erii', variables)
					if ((((instruction >> 24) & 0xf))&(0x6))==(0x4):
						variables['imm'] = (((instruction >> 16) & 15) << 0) | (((instruction >> 0) & 4095) << 4)
						return Instruction('sw_id:erii', variables)
					return None
				return None
			if (((instruction >> 42) & 0x3))!=(0x3):
				if (((instruction >> 25) & 0x3))!=(0x3):
					if ((((instruction >> 28) & 0xf))&(0xc))==(0x4):
						if (((instruction >> 28) & 0x1))==(0x0):
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							variables['off'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							variables['endian'] = (((instruction >> 27) & 1) << 0)
							if ((((instruction >> 24) & 0xf))&(0x7))==(0x1):
								if ((((instruction >> 29) & 0x1))==(0x1)) and ((((instruction >> 39) & 0x1))==(0x1)):
									variables['dc'] = (((instruction >> 40) & 15) << 1)
									return Instruction('lbs.s:erri', variables)
								if (((instruction >> 29) & 0x1))==(0x0):
									variables['rc'] = (((instruction >> 39) & 31) << 0)
									return Instruction('lbs:erri', variables)
								return None
							if ((((instruction >> 24) & 0xf))&(0x7))==(0x0):
								if ((((instruction >> 29) & 0x1))==(0x1)) and ((((instruction >> 39) & 0x1))==(0x0)):
									variables['dc'] = (((instruction >> 40) & 15) << 1)
									return Instruction('lbu.u:erri', variables)
								if (((instruction >> 29) & 0x1))==(0x0):
									variables['rc'] = (((instruction >> 39) & 31) << 0)
									return Instruction('lbu:erri', variables)
								return None
							if ((((instruction >> 24) & 0xf))&(0x7))==(0x3):
								if ((((instruction >> 29) & 0x1))==(0x1)) and ((((instruction >> 39) & 0x1))==(0x1)):
									variables['dc'] = (((instruction >> 40) & 15) << 1)
									return Instruction('lhs.s:erri', variables)
								if (((instruction >> 29) & 0x1))==(0x0):
									variables['rc'] = (((instruction >> 39) & 31) << 0)
									return Instruction('lhs:erri', variables)
								return None
							if ((((instruction >> 24) & 0xf))&(0x7))==(0x2):
								if ((((instruction >> 29) & 0x1))==(0x1)) and ((((instruction >> 39) & 0x1))==(0x0)):
									variables['dc'] = (((instruction >> 40) & 15) << 1)
									return Instruction('lhu.u:erri', variables)
								if (((instruction >> 29) & 0x1))==(0x0):
									variables['rc'] = (((instruction >> 39) & 31) << 0)
									return Instruction('lhu:erri', variables)
								return None
							if ((((instruction >> 24) & 0xf))&(0x6))==(0x4):
								if ((((instruction >> 29) & 0x1))==(0x1)) and ((((instruction >> 39) & 0x1))==(0x1)):
									variables['dc'] = (((instruction >> 40) & 15) << 1)
									return Instruction('lw.s:erri', variables)
								if ((((instruction >> 29) & 0x1))==(0x1)) and ((((instruction >> 39) & 0x1))==(0x0)):
									variables['dc'] = (((instruction >> 40) & 15) << 1)
									return Instruction('lw.u:erri', variables)
								if (((instruction >> 29) & 0x1))==(0x0):
									variables['rc'] = (((instruction >> 39) & 31) << 0)
									return Instruction('lw:erri', variables)
								return None
							return None
						if (((instruction >> 28) & 0x1))==(0x1):
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							variables['off'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							variables['endian'] = (((instruction >> 27) & 1) << 0)
							if ((((instruction >> 24) & 0xf))&(0x7))==(0x1):
								if (((instruction >> 29) & 0x1))==(0x0):
									variables['rc'] = (((instruction >> 39) & 31) << 0)
									return Instruction('lbs:ersi', variables)
								return None
							if ((((instruction >> 24) & 0xf))&(0x7))==(0x0):
								if (((instruction >> 29) & 0x1))==(0x0):
									variables['rc'] = (((instruction >> 39) & 31) << 0)
									return Instruction('lbu:ersi', variables)
								return None
							if ((((instruction >> 24) & 0xf))&(0x7))==(0x3):
								if (((instruction >> 29) & 0x1))==(0x0):
									variables['rc'] = (((instruction >> 39) & 31) << 0)
									return Instruction('lhs:ersi', variables)
								return None
							if ((((instruction >> 24) & 0xf))&(0x7))==(0x2):
								if (((instruction >> 29) & 0x1))==(0x0):
									variables['rc'] = (((instruction >> 39) & 31) << 0)
									return Instruction('lhu:ersi', variables)
								return None
							if ((((instruction >> 24) & 0xf))&(0x6))==(0x4):
								if (((instruction >> 29) & 0x1))==(0x0):
									variables['rc'] = (((instruction >> 39) & 31) << 0)
									return Instruction('lw:ersi', variables)
								return None
							return None
						return None
					return None
				if (((instruction >> 25) & 0x3))==(0x3):
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x4):
						if (((instruction >> 28) & 0x1))==(0x0):
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							variables['off'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							variables['endian'] = (((instruction >> 27) & 1) << 0)
							if ((((instruction >> 24) & 0xf))&(0x6))==(0x6):
								variables['dc'] = (((instruction >> 39) & 31) << 0)
								return Instruction('ld:erri', variables)
							return None
						if (((instruction >> 28) & 0x1))==(0x1):
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							variables['off'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							variables['endian'] = (((instruction >> 27) & 1) << 0)
							if ((((instruction >> 24) & 0xf))&(0x6))==(0x6):
								variables['dc'] = (((instruction >> 39) & 31) << 0)
								return Instruction('ld:ersi', variables)
							return None
						return None
					return None
				return None
			return None
		if (((instruction >> 32) & 0x3))!=(0x3):
			if ((((instruction >> 39) & 0x1f))&(0x1f))==(0x0):
				variables['immDma'] = (((instruction >> 24) & 255) << 0)
				variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
				variables['ra'] = (((instruction >> 34) & 31) << 0)
				if ((((instruction >> 0) & 0xffff))&(0xffff))==(0x0):
					return Instruction('ldma:rri', variables)
				if ((((instruction >> 0) & 0xffff))&(0xffff))==(0x1):
					return Instruction('ldmai:rri', variables)
				if ((((instruction >> 0) & 0xffff))&(0xffff))==(0x2):
					return Instruction('sdma:rri', variables)
				return None
			if (((instruction >> 42) & 0x3))==(0x3):
				if (((instruction >> 16) & 0x1))==(0x0):
					variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if ((((instruction >> 28) & 0xf))&(0xc))==(0x4):
						if (((instruction >> 28) & 0x1))==(0x0):
							variables['off'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 39) & 7) << 4) | (((instruction >> 24) & 1) << 7) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							variables['endian'] = (((instruction >> 27) & 1) << 0)
							if ((((instruction >> 24) & 0xf))&(0x6))==(0x0):
								return Instruction('sb:erir', variables)
							if ((((instruction >> 24) & 0xf))&(0x6))==(0x2):
								return Instruction('sh:erir', variables)
							if ((((instruction >> 24) & 0xf))&(0x6))==(0x4):
								return Instruction('sw:erir', variables)
							return None
						if (((instruction >> 28) & 0x1))==(0x1):
							variables['off'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 39) & 7) << 4) | (((instruction >> 24) & 1) << 7) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							variables['endian'] = (((instruction >> 27) & 1) << 0)
							if ((((instruction >> 24) & 0xf))&(0x6))==(0x0):
								return Instruction('sb:esir', variables)
							if ((((instruction >> 24) & 0xf))&(0x6))==(0x2):
								return Instruction('sh:esir', variables)
							if ((((instruction >> 24) & 0xf))&(0x6))==(0x4):
								return Instruction('sw:esir', variables)
							return None
						return None
					return None
				if (((instruction >> 16) & 0x1))==(0x1):
					variables['db'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if ((((instruction >> 28) & 0xf))&(0xc))==(0x4):
						if (((instruction >> 28) & 0x1))==(0x0):
							variables['off'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 39) & 7) << 4) | (((instruction >> 24) & 1) << 7) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							variables['endian'] = (((instruction >> 27) & 1) << 0)
							if ((((instruction >> 24) & 0xf))&(0x6))==(0x6):
								return Instruction('sd:erir', variables)
							return None
						if (((instruction >> 28) & 0x1))==(0x1):
							variables['off'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 39) & 7) << 4) | (((instruction >> 24) & 1) << 7) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							variables['endian'] = (((instruction >> 27) & 1) << 0)
							if ((((instruction >> 24) & 0xf))&(0x6))==(0x6):
								return Instruction('sd:esir', variables)
							return None
						return None
					return None
				return None
			return None
		return None
	if ((((instruction >> 45) & 0x7))==(0x0)) and ((((instruction >> 42) & 0x3))!=(0x3)):
		if ((((instruction >> 32) & 0x3))!=(0x3)) and ((((instruction >> 16) & 0x1))==(0x0)):
			variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
			if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 39) & 0x1))==(0x1)):
				variables['dc'] = (((instruction >> 40) & 15) << 1)
				if (((instruction >> 37) & 0x3))==(0x3):
					variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 34) & 7) << 4) | (((instruction >> 44) & 1) << 7) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 24) & 255) << 24)
					return Instruction('add.s:rri', variables)
				return None
			if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 39) & 0x1))==(0x0)):
				variables['dc'] = (((instruction >> 40) & 15) << 1)
				if (((instruction >> 37) & 0x3))==(0x3):
					variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 34) & 7) << 4) | (((instruction >> 44) & 1) << 7) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 24) & 255) << 24)
					return Instruction('add.u:rri', variables)
				return None
			return None
		if (((instruction >> 32) & 0x3))==(0x3):
			if ((((instruction >> 44) & 0xf))==(0x0)) and ((((instruction >> 42) & 0x3))!=(0x3)):
				if (((instruction >> 42) & 0x3))!=(0x3):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 24) & 255) << 24)
					return Instruction('add:rri', variables)
				return None
			if ((((instruction >> 44) & 0xf))==(0x1)) and ((((instruction >> 42) & 0x3))!=(0x3)):
				if (((instruction >> 42) & 0x3))!=(0x3):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 24) & 255) << 24)
					return Instruction('addc:rri', variables)
				return None
			return None
		return None
	if (((((instruction >> 47) & 0x1))==(0x0)) and ((((instruction >> 44) & 0x7))!=(0x7))) and ((((instruction >> 42) & 0x3))==(0x3)):
		if (((instruction >> 32) & 0x3))==(0x3):
			if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
				variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
				variables['ra'] = (((instruction >> 34) & 31) << 0)
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x0):
					if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('add.s:rric', variables)
						if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 31) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
							return Instruction('add.s:rrici', variables)
						if (((instruction >> 24) & 0x1f))==(0x0):
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('add.s:rrif', variables)
						return None
					return None
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x2):
					if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('addc.s:rric', variables)
						if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 31) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
							return Instruction('addc.s:rrici', variables)
						if (((instruction >> 24) & 0x1f))==(0x0):
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('addc.s:rrif', variables)
						return None
					return None
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x4):
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
						variables['cc'] = (((instruction >> 24) & 31) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('sub.s:rirc', variables)
					if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 31) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('sub.s:rirci', variables)
					if (((instruction >> 24) & 0x1f))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('sub.s:rirf', variables)
					return None
				if ((((instruction >> 28) & 0xf))&(0xa))==(0x8):
					if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
						if (((((instruction >> 30) & 0x1))==(0x1)) or ((((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))))) and (((((instruction >> 30) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('sub.s:rric', variables)
						if (((((instruction >> 30) & 0x1))==(0x0)) and (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8))))) and (((((instruction >> 30) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
							return Instruction('sub.s:rrici', variables)
						if ((((instruction >> 24) & 0x1f))==(0x0)) and ((((instruction >> 30) & 0x1))==(0x0)):
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('sub.s:rrif', variables)
						return None
					return None
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x6):
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
						variables['cc'] = (((instruction >> 24) & 31) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('subc.s:rirc', variables)
					if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 31) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('subc.s:rirci', variables)
					if (((instruction >> 24) & 0x1f))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('subc.s:rirf', variables)
					return None
				if ((((instruction >> 28) & 0xf))&(0xa))==(0xa):
					if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
						if (((((instruction >> 30) & 0x1))==(0x1)) or ((((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))))) and (((((instruction >> 30) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('subc.s:rric', variables)
						if (((((instruction >> 30) & 0x1))==(0x0)) and (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8))))) and (((((instruction >> 30) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
							return Instruction('subc.s:rrici', variables)
						if ((((instruction >> 24) & 0x1f))==(0x0)) and ((((instruction >> 30) & 0x1))==(0x0)):
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('subc.s:rrif', variables)
						return None
					return None
				return None
			if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
				variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
				variables['ra'] = (((instruction >> 34) & 31) << 0)
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x0):
					if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('add.u:rric', variables)
						if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 31) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
							return Instruction('add.u:rrici', variables)
						if (((instruction >> 24) & 0x1f))==(0x0):
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('add.u:rrif', variables)
						return None
					return None
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x2):
					if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('addc.u:rric', variables)
						if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 31) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
							return Instruction('addc.u:rrici', variables)
						if (((instruction >> 24) & 0x1f))==(0x0):
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('addc.u:rrif', variables)
						return None
					return None
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x4):
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
						variables['cc'] = (((instruction >> 24) & 31) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('sub.u:rirc', variables)
					if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 31) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('sub.u:rirci', variables)
					if (((instruction >> 24) & 0x1f))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('sub.u:rirf', variables)
					return None
				if ((((instruction >> 28) & 0xf))&(0xa))==(0x8):
					if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
						if (((((instruction >> 30) & 0x1))==(0x1)) or ((((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))))) and (((((instruction >> 30) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('sub.u:rric', variables)
						if (((((instruction >> 30) & 0x1))==(0x0)) and (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8))))) and (((((instruction >> 30) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
							return Instruction('sub.u:rrici', variables)
						if ((((instruction >> 24) & 0x1f))==(0x0)) and ((((instruction >> 30) & 0x1))==(0x0)):
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('sub.u:rrif', variables)
						return None
					return None
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x6):
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
						variables['cc'] = (((instruction >> 24) & 31) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('subc.u:rirc', variables)
					if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 31) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('subc.u:rirci', variables)
					if (((instruction >> 24) & 0x1f))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('subc.u:rirf', variables)
					return None
				if ((((instruction >> 28) & 0xf))&(0xa))==(0xa):
					if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
						if (((((instruction >> 30) & 0x1))==(0x1)) or ((((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))))) and (((((instruction >> 30) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('subc.u:rric', variables)
						if (((((instruction >> 30) & 0x1))==(0x0)) and (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8))))) and (((((instruction >> 30) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
							return Instruction('subc.u:rrici', variables)
						if ((((instruction >> 24) & 0x1f))==(0x0)) and ((((instruction >> 30) & 0x1))==(0x0)):
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('subc.u:rrif', variables)
						return None
					return None
				return None
			if ((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x0)):
				variables['rc'] = (((instruction >> 39) & 7) << 0) | (((instruction >> 44) & 3) << 3)
				variables['ra'] = (((instruction >> 34) & 31) << 0)
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x0):
					if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('add:rric', variables)
						if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 31) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
							return Instruction('add:rrici', variables)
						if (((instruction >> 24) & 0x1f))==(0x0):
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('add:rrif', variables)
						return None
					if ((((instruction >> 28) & 0x1))==(0x1)) and ((((instruction >> 24) & 0xf))==(0x0)):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 31) << 12) | (((instruction >> 5) & 1) << 16) | (((instruction >> 6) & 1) << 16) | (((instruction >> 7) & 1) << 16) | (((instruction >> 8) & 1) << 16) | (((instruction >> 9) & 1) << 16) | (((instruction >> 10) & 1) << 16) | (((instruction >> 11) & 1) << 16)
						return Instruction('add:ssi', variables)
					return None
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x2):
					if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('addc:rric', variables)
						if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 31) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
							return Instruction('addc:rrici', variables)
						if (((instruction >> 24) & 0x1f))==(0x0):
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('addc:rrif', variables)
						return None
					return None
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x4):
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
						variables['cc'] = (((instruction >> 24) & 31) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('sub:rirc', variables)
					if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 31) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('sub:rirci', variables)
					if (((instruction >> 24) & 0x1f))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('sub:rirf', variables)
					return None
				if ((((instruction >> 28) & 0xf))&(0xa))==(0x8):
					if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
						if (((((instruction >> 30) & 0x1))==(0x1)) or ((((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))))) and (((((instruction >> 30) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('sub:rric', variables)
						if (((((instruction >> 30) & 0x1))==(0x0)) and (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8))))) and (((((instruction >> 30) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
							return Instruction('sub:rrici', variables)
						if ((((instruction >> 24) & 0x1f))==(0x0)) and ((((instruction >> 30) & 0x1))==(0x0)):
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('sub:rrif', variables)
						return None
					if ((((instruction >> 28) & 0x1))==(0x1)) and ((((instruction >> 24) & 0xf))==(0x0)):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 31) << 12) | (((instruction >> 5) & 1) << 16) | (((instruction >> 6) & 1) << 16) | (((instruction >> 7) & 1) << 16) | (((instruction >> 8) & 1) << 16) | (((instruction >> 9) & 1) << 16) | (((instruction >> 10) & 1) << 16) | (((instruction >> 11) & 1) << 16)
						return Instruction('sub:ssi', variables)
					return None
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x6):
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
						variables['cc'] = (((instruction >> 24) & 31) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('subc:rirc', variables)
					if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 31) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('subc:rirci', variables)
					if (((instruction >> 24) & 0x1f))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('subc:rirf', variables)
					return None
				if ((((instruction >> 28) & 0xf))&(0xa))==(0xa):
					if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
						if (((((instruction >> 30) & 0x1))==(0x1)) or ((((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))))) and (((((instruction >> 30) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('subc:rric', variables)
						if (((((instruction >> 30) & 0x1))==(0x0)) and (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8))))) and (((((instruction >> 30) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
							return Instruction('subc:rrici', variables)
						if ((((instruction >> 24) & 0x1f))==(0x0)) and ((((instruction >> 30) & 0x1))==(0x0)):
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
							return Instruction('subc:rrif', variables)
						return None
					return None
				return None
			if (((instruction >> 44) & 0x3))==(0x3):
				variables['ra'] = (((instruction >> 34) & 31) << 0)
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x0):
					if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24)
							return Instruction('add:zric', variables)
						if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 31) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 39) & 7) << 8)
							return Instruction('add:zrici', variables)
						if (((instruction >> 24) & 0x1f))==(0x0):
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24)
							return Instruction('add:zrif', variables)
						return None
					return None
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x2):
					if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24)
							return Instruction('addc:zric', variables)
						if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 31) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 39) & 7) << 8)
							return Instruction('addc:zrici', variables)
						if (((instruction >> 24) & 0x1f))==(0x0):
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24)
							return Instruction('addc:zrif', variables)
						return None
					return None
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x4):
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
						variables['cc'] = (((instruction >> 24) & 31) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24)
						return Instruction('sub:zirc', variables)
					if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 31) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 39) & 7) << 8)
						return Instruction('sub:zirci', variables)
					if (((instruction >> 24) & 0x1f))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24)
						return Instruction('sub:zirf', variables)
					return None
				if ((((instruction >> 28) & 0xf))&(0xa))==(0x8):
					if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
						if (((((instruction >> 30) & 0x1))==(0x1)) or ((((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))))) and (((((instruction >> 30) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24)
							return Instruction('sub:zric', variables)
						if (((((instruction >> 30) & 0x1))==(0x0)) and (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8))))) and (((((instruction >> 30) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 39) & 7) << 8)
							return Instruction('sub:zrici', variables)
						if ((((instruction >> 24) & 0x1f))==(0x0)) and ((((instruction >> 30) & 0x1))==(0x0)):
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24)
							return Instruction('sub:zrif', variables)
						return None
					return None
				if ((((instruction >> 28) & 0xf))&(0xe))==(0x6):
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
						variables['cc'] = (((instruction >> 24) & 31) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24)
						return Instruction('subc:zirc', variables)
					if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 31) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 39) & 7) << 8)
						return Instruction('subc:zirci', variables)
					if (((instruction >> 24) & 0x1f))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24)
						return Instruction('subc:zirf', variables)
					return None
				if ((((instruction >> 28) & 0xf))&(0xa))==(0xa):
					if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
						if (((((instruction >> 30) & 0x1))==(0x1)) or ((((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))))) and (((((instruction >> 30) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24)
							return Instruction('subc:zric', variables)
						if (((((instruction >> 30) & 0x1))==(0x0)) and (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8))))) and (((((instruction >> 30) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 39) & 7) << 8)
							return Instruction('subc:zrici', variables)
						if ((((instruction >> 24) & 0x1f))==(0x0)) and ((((instruction >> 30) & 0x1))==(0x0)):
							variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24)
							return Instruction('subc:zrif', variables)
						return None
					return None
				return None
			return None
		if ((((instruction >> 32) & 0x3))!=(0x3)) and ((((instruction >> 16) & 0x1))==(0x0)):
			if ((((instruction >> 20) & 0xf))&(0xc))==(0xc):
				if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x0):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0x1f))==(0x0):
								return Instruction('add.s:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								return Instruction('add.s:rrrc', variables)
							if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('add.s:rrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x2):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0x1f))==(0x0):
								return Instruction('addc.s:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								return Instruction('addc.s:rrrc', variables)
							if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('addc.s:rrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x4):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0x1f))==(0x0):
								return Instruction('rsub.s:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								return Instruction('rsub.s:rrrc', variables)
							if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('rsub.s:rrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x6):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0x1f))==(0x0):
								return Instruction('rsubc.s:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								return Instruction('rsubc.s:rrrc', variables)
							if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('rsubc.s:rrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xa))==(0x8):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if ((((instruction >> 24) & 0x1f))==(0x0)) and ((((instruction >> 30) & 0x1))==(0x0)):
								return Instruction('sub.s:rrr', variables)
							if (((((instruction >> 30) & 0x1))==(0x1)) or ((((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))))) and (((((instruction >> 30) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
								return Instruction('sub.s:rrrc', variables)
							if (((((instruction >> 30) & 0x1))==(0x0)) and (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8))))) and (((((instruction >> 30) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('sub.s:rrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xa))==(0xa):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if ((((instruction >> 24) & 0x1f))==(0x0)) and ((((instruction >> 30) & 0x1))==(0x0)):
								return Instruction('subc.s:rrr', variables)
							if (((((instruction >> 30) & 0x1))==(0x1)) or ((((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))))) and (((((instruction >> 30) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
								return Instruction('subc.s:rrrc', variables)
							if (((((instruction >> 30) & 0x1))==(0x0)) and (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8))))) and (((((instruction >> 30) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('subc.s:rrrci', variables)
							return None
						return None
					return None
				if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x0):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0x1f))==(0x0):
								return Instruction('add.u:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								return Instruction('add.u:rrrc', variables)
							if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('add.u:rrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x2):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0x1f))==(0x0):
								return Instruction('addc.u:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								return Instruction('addc.u:rrrc', variables)
							if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('addc.u:rrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x4):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0x1f))==(0x0):
								return Instruction('rsub.u:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								return Instruction('rsub.u:rrrc', variables)
							if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('rsub.u:rrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x6):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0x1f))==(0x0):
								return Instruction('rsubc.u:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								return Instruction('rsubc.u:rrrc', variables)
							if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('rsubc.u:rrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xa))==(0x8):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if ((((instruction >> 24) & 0x1f))==(0x0)) and ((((instruction >> 30) & 0x1))==(0x0)):
								return Instruction('sub.u:rrr', variables)
							if (((((instruction >> 30) & 0x1))==(0x1)) or ((((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))))) and (((((instruction >> 30) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
								return Instruction('sub.u:rrrc', variables)
							if (((((instruction >> 30) & 0x1))==(0x0)) and (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8))))) and (((((instruction >> 30) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('sub.u:rrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xa))==(0xa):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if ((((instruction >> 24) & 0x1f))==(0x0)) and ((((instruction >> 30) & 0x1))==(0x0)):
								return Instruction('subc.u:rrr', variables)
							if (((((instruction >> 30) & 0x1))==(0x1)) or ((((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))))) and (((((instruction >> 30) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
								return Instruction('subc.u:rrrc', variables)
							if (((((instruction >> 30) & 0x1))==(0x0)) and (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8))))) and (((((instruction >> 30) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('subc.u:rrrci', variables)
							return None
						return None
					return None
				if ((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 7) << 0) | (((instruction >> 44) & 3) << 3)
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x0):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0x1f))==(0x0):
								return Instruction('add:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								return Instruction('add:rrrc', variables)
							if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('add:rrrci', variables)
							return None
						if ((((instruction >> 28) & 0x1))==(0x1)) and ((((instruction >> 24) & 0xf))==(0x0)):
							if (((instruction >> 37) & 0x3))!=(0x3):
								variables['rb'] = (((instruction >> 34) & 31) << 0)
								variables['ra'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
								return Instruction('add:sss', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x2):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0x1f))==(0x0):
								return Instruction('addc:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								return Instruction('addc:rrrc', variables)
							if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('addc:rrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x4):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0x1f))==(0x0):
								return Instruction('rsub:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								return Instruction('rsub:rrrc', variables)
							if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('rsub:rrrci', variables)
							return None
						if ((((instruction >> 28) & 0x1))==(0x1)) and ((((instruction >> 24) & 0xf))==(0x0)):
							if (((instruction >> 37) & 0x3))!=(0x3):
								variables['rb'] = (((instruction >> 34) & 31) << 0)
								variables['ra'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
								return Instruction('sub:sss', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x6):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0x1f))==(0x0):
								return Instruction('rsubc:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								return Instruction('rsubc:rrrc', variables)
							if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('rsubc:rrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xa))==(0x8):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if ((((instruction >> 24) & 0x1f))==(0x0)) and ((((instruction >> 30) & 0x1))==(0x0)):
								return Instruction('sub:rrr', variables)
							if (((((instruction >> 30) & 0x1))==(0x1)) or ((((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))))) and (((((instruction >> 30) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
								return Instruction('sub:rrrc', variables)
							if (((((instruction >> 30) & 0x1))==(0x0)) and (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8))))) and (((((instruction >> 30) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('sub:rrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xa))==(0xa):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if ((((instruction >> 24) & 0x1f))==(0x0)) and ((((instruction >> 30) & 0x1))==(0x0)):
								return Instruction('subc:rrr', variables)
							if (((((instruction >> 30) & 0x1))==(0x1)) or ((((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))))) and (((((instruction >> 30) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
								return Instruction('subc:rrrc', variables)
							if (((((instruction >> 30) & 0x1))==(0x0)) and (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8))))) and (((((instruction >> 30) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('subc:rrrci', variables)
							return None
						return None
					return None
				if (((instruction >> 44) & 0x3))==(0x3):
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x0):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0x1f))==(0x0):
								return Instruction('add:zrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								return Instruction('add:zrrc', variables)
							if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('add:zrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x2):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0x1f))==(0x0):
								return Instruction('addc:zrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								return Instruction('addc:zrrc', variables)
							if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('addc:zrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x4):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0x1f))==(0x0):
								return Instruction('rsub:zrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								return Instruction('rsub:zrrc', variables)
							if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('rsub:zrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xe))==(0x6):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((instruction >> 24) & 0x1f))==(0x0):
								return Instruction('rsubc:zrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								return Instruction('rsubc:zrrc', variables)
							if (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('rsubc:zrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xa))==(0x8):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if ((((instruction >> 24) & 0x1f))==(0x0)) and ((((instruction >> 30) & 0x1))==(0x0)):
								return Instruction('sub:zrr', variables)
							if (((((instruction >> 30) & 0x1))==(0x1)) or ((((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))))) and (((((instruction >> 30) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
								return Instruction('sub:zrrc', variables)
							if (((((instruction >> 30) & 0x1))==(0x0)) and (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8))))) and (((((instruction >> 30) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('sub:zrrci', variables)
							return None
						return None
					if ((((instruction >> 28) & 0xf))&(0xa))==(0xa):
						if ((((instruction >> 28) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if ((((instruction >> 24) & 0x1f))==(0x0)) and ((((instruction >> 30) & 0x1))==(0x0)):
								return Instruction('subc:zrr', variables)
							if (((((instruction >> 30) & 0x1))==(0x1)) or ((((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and (((((instruction >> 25) & 0x1))==(0x1)) and ((((instruction >> 28) & 0x1))==(0x0))))) and (((((instruction >> 30) & 0x1))!=(0x1)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
								return Instruction('subc:zrrc', variables)
							if (((((instruction >> 30) & 0x1))==(0x0)) and (((((instruction >> 24) & 0x1f))<(0x6)) or (((((instruction >> 24) & 0x1f))>(0xb)) or (((((instruction >> 24) & 0x1f))&(0x1e))==(0x8))))) and (((((instruction >> 30) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0x1f))!=(0x0))):
								variables['cc'] = (((instruction >> 24) & 31) << 0) | (((instruction >> 30) & 1) << 5)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('subc:zrrci', variables)
							return None
						return None
					return None
				return None
			if ((((instruction >> 20) & 0xf))&(0xc))!=(0xc):
				if ((((instruction >> 28) & 0xf))&(0x2))==(0x0):
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x7):
						variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
							variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sh_sh.s:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sh_sh.s:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sh_sh.s:rrrci', variables)
							return None
						if ((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x0)):
							variables['rc'] = (((instruction >> 39) & 7) << 0) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sh_sh:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sh_sh:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sh_sh:rrrci', variables)
							return None
						if (((instruction >> 44) & 0x3))==(0x3):
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sh_sh:zrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sh_sh:zrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sh_sh:zrrci', variables)
							return None
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x6):
						variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
							variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sh_sl.s:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sh_sl.s:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sh_sl.s:rrrci', variables)
							return None
						if ((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x0)):
							variables['rc'] = (((instruction >> 39) & 7) << 0) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sh_sl:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sh_sl:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sh_sl:rrrci', variables)
							return None
						if (((instruction >> 44) & 0x3))==(0x3):
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sh_sl:zrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sh_sl:zrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sh_sl:zrrci', variables)
							return None
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xb):
						variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
							variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sh_uh.s:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sh_uh.s:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sh_uh.s:rrrci', variables)
							return None
						if ((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x0)):
							variables['rc'] = (((instruction >> 39) & 7) << 0) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sh_uh:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sh_uh:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sh_uh:rrrci', variables)
							return None
						if (((instruction >> 44) & 0x3))==(0x3):
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sh_uh:zrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sh_uh:zrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sh_uh:zrrci', variables)
							return None
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xa):
						variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
							variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sh_ul.s:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sh_ul.s:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sh_ul.s:rrrci', variables)
							return None
						if ((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x0)):
							variables['rc'] = (((instruction >> 39) & 7) << 0) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sh_ul:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sh_ul:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sh_ul:rrrci', variables)
							return None
						if (((instruction >> 44) & 0x3))==(0x3):
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sh_ul:zrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sh_ul:zrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sh_ul:zrrci', variables)
							return None
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x5):
						variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
							variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sl_sh.s:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sl_sh.s:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sl_sh.s:rrrci', variables)
							return None
						if ((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x0)):
							variables['rc'] = (((instruction >> 39) & 7) << 0) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sl_sh:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sl_sh:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sl_sh:rrrci', variables)
							return None
						if (((instruction >> 44) & 0x3))==(0x3):
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sl_sh:zrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sl_sh:zrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sl_sh:zrrci', variables)
							return None
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x4):
						variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
							variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sl_sl.s:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sl_sl.s:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sl_sl.s:rrrci', variables)
							return None
						if ((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x0)):
							variables['rc'] = (((instruction >> 39) & 7) << 0) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sl_sl:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sl_sl:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sl_sl:rrrci', variables)
							return None
						if (((instruction >> 44) & 0x3))==(0x3):
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sl_sl:zrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sl_sl:zrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sl_sl:zrrci', variables)
							return None
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x9):
						variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
							variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sl_uh.s:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sl_uh.s:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sl_uh.s:rrrci', variables)
							return None
						if ((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x0)):
							variables['rc'] = (((instruction >> 39) & 7) << 0) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sl_uh:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sl_uh:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sl_uh:rrrci', variables)
							return None
						if (((instruction >> 44) & 0x3))==(0x3):
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sl_uh:zrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sl_uh:zrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sl_uh:zrrci', variables)
							return None
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x8):
						variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
							variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sl_ul.s:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sl_ul.s:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sl_ul.s:rrrci', variables)
							return None
						if ((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x0)):
							variables['rc'] = (((instruction >> 39) & 7) << 0) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sl_ul:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sl_ul:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sl_ul:rrrci', variables)
							return None
						if (((instruction >> 44) & 0x3))==(0x3):
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_sl_ul:zrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_sl_ul:zrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_sl_ul:zrrci', variables)
							return None
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x3):
						variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
							variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_uh_uh.u:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_uh_uh.u:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_uh_uh.u:rrrci', variables)
							return None
						if ((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x0)):
							variables['rc'] = (((instruction >> 39) & 7) << 0) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_uh_uh:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_uh_uh:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_uh_uh:rrrci', variables)
							return None
						if (((instruction >> 44) & 0x3))==(0x3):
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_uh_uh:zrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_uh_uh:zrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_uh_uh:zrrci', variables)
							return None
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x2):
						variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
							variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_uh_ul.u:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_uh_ul.u:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_uh_ul.u:rrrci', variables)
							return None
						if ((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x0)):
							variables['rc'] = (((instruction >> 39) & 7) << 0) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_uh_ul:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_uh_ul:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_uh_ul:rrrci', variables)
							return None
						if (((instruction >> 44) & 0x3))==(0x3):
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_uh_ul:zrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_uh_ul:zrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_uh_ul:zrrci', variables)
							return None
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x1):
						variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
							variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_ul_uh.u:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_ul_uh.u:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_ul_uh.u:rrrci', variables)
							return None
						if ((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x0)):
							variables['rc'] = (((instruction >> 39) & 7) << 0) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_ul_uh:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_ul_uh:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_ul_uh:rrrci', variables)
							return None
						if (((instruction >> 44) & 0x3))==(0x3):
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_ul_uh:zrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_ul_uh:zrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_ul_uh:zrrci', variables)
							return None
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x0):
						variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
							variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_ul_ul.u:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_ul_ul.u:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_ul_ul.u:rrrci', variables)
							return None
						if ((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x0)):
							variables['rc'] = (((instruction >> 39) & 7) << 0) | (((instruction >> 44) & 3) << 3)
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_ul_ul:rrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_ul_ul:rrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_ul_ul:rrrci', variables)
							return None
						if (((instruction >> 44) & 0x3))==(0x3):
							if (((instruction >> 24) & 0xf))==(0x0):
								return Instruction('mul_ul_ul:zrr', variables)
							if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
								variables['cc'] = (((instruction >> 24) & 15) << 0)
								return Instruction('mul_ul_ul:zrrc', variables)
							if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0x1f))!=(0x0)):
								variables['cc'] = (((instruction >> 24) & 31) << 0)
								variables['pc'] = (((instruction >> 0) & 65535) << 0)
								return Instruction('mul_ul_ul:zrrci', variables)
							return None
						return None
					return None
				if ((((instruction >> 28) & 0xf))&(0x3))==(0x2):
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x8):
						if (((instruction >> 34) & 0x1f))==(0x18):
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
								variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
								if (((instruction >> 24) & 0xf))==(0x0):
									return Instruction('time.s:r', variables)
								if (((((instruction >> 24) & 0xf))<(0x6)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
									variables['cc'] = (((instruction >> 24) & 15) << 0)
									variables['pc'] = (((instruction >> 0) & 65535) << 0)
									return Instruction('time.s:rci', variables)
								return None
							if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
								variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
								if (((instruction >> 24) & 0xf))==(0x0):
									return Instruction('time.u:r', variables)
								if (((((instruction >> 24) & 0xf))<(0x6)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
									variables['cc'] = (((instruction >> 24) & 15) << 0)
									variables['pc'] = (((instruction >> 0) & 65535) << 0)
									return Instruction('time.u:rci', variables)
								return None
							if ((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x0)):
								variables['rc'] = (((instruction >> 39) & 7) << 0) | (((instruction >> 44) & 3) << 3)
								if (((instruction >> 24) & 0xf))==(0x0):
									return Instruction('time:r', variables)
								if (((((instruction >> 24) & 0xf))<(0x6)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
									variables['cc'] = (((instruction >> 24) & 15) << 0)
									variables['pc'] = (((instruction >> 0) & 65535) << 0)
									return Instruction('time:rci', variables)
								return None
							if (((instruction >> 44) & 0x3))==(0x3):
								if (((instruction >> 24) & 0xf))==(0x0):
									return Instruction('time:z', variables)
								if (((((instruction >> 24) & 0xf))<(0x6)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
									variables['cc'] = (((instruction >> 24) & 15) << 0)
									variables['pc'] = (((instruction >> 0) & 65535) << 0)
									return Instruction('time:zci', variables)
								return None
							return None
						if (((instruction >> 34) & 0x1f))==(0x19):
							variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
							variables['ra'] = (((instruction >> 34) & 31) << 0)
							if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
								variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
								if (((instruction >> 24) & 0xf))==(0x0):
									return Instruction('time_cfg.s:rr', variables)
								if (((((instruction >> 24) & 0xf))<(0x6)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
									variables['cc'] = (((instruction >> 24) & 15) << 0)
									variables['pc'] = (((instruction >> 0) & 65535) << 0)
									return Instruction('time_cfg.s:rrci', variables)
								return None
							if (((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
								variables['dc'] = (((instruction >> 40) & 3) << 1) | (((instruction >> 44) & 3) << 3)
								if (((instruction >> 24) & 0xf))==(0x0):
									return Instruction('time_cfg.u:rr', variables)
								if (((((instruction >> 24) & 0xf))<(0x6)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
									variables['cc'] = (((instruction >> 24) & 15) << 0)
									variables['pc'] = (((instruction >> 0) & 65535) << 0)
									return Instruction('time_cfg.u:rrci', variables)
								return None
							if ((((instruction >> 44) & 0x3))!=(0x3)) and ((((instruction >> 46) & 0x1))==(0x0)):
								variables['rc'] = (((instruction >> 39) & 7) << 0) | (((instruction >> 44) & 3) << 3)
								if (((instruction >> 24) & 0xf))==(0x0):
									return Instruction('time_cfg:rr', variables)
								if (((((instruction >> 24) & 0xf))<(0x6)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
									variables['cc'] = (((instruction >> 24) & 15) << 0)
									variables['pc'] = (((instruction >> 0) & 65535) << 0)
									return Instruction('time_cfg:rrci', variables)
								return None
							if (((instruction >> 44) & 0x3))==(0x3):
								if (((instruction >> 24) & 0xf))==(0x0):
									return Instruction('time_cfg:zr', variables)
								if (((((instruction >> 24) & 0xf))<(0x6)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
									variables['cc'] = (((instruction >> 24) & 15) << 0)
									variables['pc'] = (((instruction >> 0) & 65535) << 0)
									return Instruction('time_cfg:zrci', variables)
								return None
							return None
						return None
					return None
				return None
			return None
		return None
	if ((((instruction >> 44) & 0xf))==(0x6)) and ((((instruction >> 42) & 0x3))!=(0x3)):
		if ((((instruction >> 32) & 0x3))!=(0x3)) and ((((instruction >> 16) & 0x1))==(0x0)):
			variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
			if (((instruction >> 37) & 0x3))==(0x3):
				variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 34) & 7) << 4) | (((instruction >> 39) & 1) << 7) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 24) & 255) << 24)
				if ((((instruction >> 39) & 0x1f))&(0x1e))==(0x0):
					return Instruction('add:zri', variables)
				if ((((instruction >> 39) & 0x1f))&(0x1e))==(0x2):
					return Instruction('addc:zri', variables)
				if ((((instruction >> 39) & 0x1f))&(0x1e))==(0xa):
					return Instruction('and:zri', variables)
				if ((((instruction >> 39) & 0x1f))&(0x1e))==(0xc):
					return Instruction('or:zri', variables)
				if ((((instruction >> 39) & 0x1f))&(0x1e))==(0x4):
					return Instruction('sub:zir', variables)
				if ((((instruction >> 39) & 0x1f))&(0x1e))==(0x6):
					return Instruction('subc:zir', variables)
				if ((((instruction >> 39) & 0x1f))&(0x1e))==(0x8):
					return Instruction('xor:zri', variables)
				return None
			return None
		if (((instruction >> 32) & 0x3))==(0x3):
			variables['ra'] = (((instruction >> 34) & 31) << 0)
			variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 24) & 255) << 24)
			if (((instruction >> 42) & 0x3))!=(0x3):
				variables['rc'] = (((instruction >> 39) & 31) << 0)
				return Instruction('or:rri', variables)
			return None
		return None
	if ((((instruction >> 45) & 0x7))==(0x2)) and ((((instruction >> 42) & 0x3))!=(0x3)):
		if (((instruction >> 32) & 0x3))==(0x3):
			if ((((instruction >> 44) & 0xf))==(0x5)) and ((((instruction >> 42) & 0x3))!=(0x3)):
				if (((instruction >> 37) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 24) & 255) << 24)
						return Instruction('and.s:rki', variables)
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 24) & 255) << 24)
						return Instruction('and.u:rki', variables)
					return None
				if (((instruction >> 37) & 0x3))!=(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 42) & 0x3))!=(0x3):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 24) & 255) << 24)
						return Instruction('and:rri', variables)
					return None
				return None
			if ((((instruction >> 44) & 0xf))==(0x4)) and ((((instruction >> 42) & 0x3))!=(0x3)):
				if (((instruction >> 42) & 0x3))!=(0x3):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 24) & 255) << 24)
					return Instruction('xor:rri', variables)
				return None
			return None
		if ((((instruction >> 32) & 0x3))!=(0x3)) and ((((instruction >> 16) & 0x1))==(0x0)):
			variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
			if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 39) & 0x1))==(0x1)):
				variables['dc'] = (((instruction >> 40) & 15) << 1)
				if (((instruction >> 37) & 0x3))==(0x3):
					variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 34) & 7) << 4) | (((instruction >> 44) & 1) << 7) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 24) & 255) << 24)
					return Instruction('or.s:rri', variables)
				return None
			if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 39) & 0x1))==(0x0)):
				variables['dc'] = (((instruction >> 40) & 15) << 1)
				if (((instruction >> 37) & 0x3))==(0x3):
					variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 34) & 7) << 4) | (((instruction >> 44) & 1) << 7) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 24) & 255) << 24)
					return Instruction('or.u:rri', variables)
				return None
			return None
		return None
	if ((((instruction >> 45) & 0x7))==(0x1)) and ((((instruction >> 42) & 0x3))!=(0x3)):
		if ((((instruction >> 32) & 0x3))!=(0x3)) and ((((instruction >> 16) & 0x1))==(0x0)):
			variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
			if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 39) & 0x1))==(0x1)):
				variables['dc'] = (((instruction >> 40) & 15) << 1)
				if (((instruction >> 37) & 0x3))==(0x3):
					variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 34) & 7) << 4) | (((instruction >> 44) & 1) << 7) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 24) & 255) << 24)
					return Instruction('and.s:rri', variables)
				return None
			if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 39) & 0x1))==(0x0)):
				variables['dc'] = (((instruction >> 40) & 15) << 1)
				if (((instruction >> 37) & 0x3))==(0x3):
					variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 34) & 7) << 4) | (((instruction >> 44) & 1) << 7) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 24) & 255) << 24)
					return Instruction('and.u:rri', variables)
				return None
			return None
		if (((instruction >> 32) & 0x3))==(0x3):
			if ((((instruction >> 44) & 0xf))==(0x2)) and ((((instruction >> 42) & 0x3))!=(0x3)):
				if (((instruction >> 42) & 0x3))!=(0x3):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 24) & 255) << 24)
					return Instruction('sub:rir', variables)
				return None
			if ((((instruction >> 44) & 0xf))==(0x3)) and ((((instruction >> 42) & 0x3))!=(0x3)):
				if (((instruction >> 42) & 0x3))!=(0x3):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 24) & 255) << 24)
					return Instruction('subc:rir', variables)
				return None
			return None
		return None
	if (((instruction >> 45) & 0x7))==(0x4):
		if (((instruction >> 32) & 0x3))==(0x3):
			if ((((instruction >> 28) & 0xf))&(0xf))==(0xa):
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('and.s:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('and.s:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('and.s:rrif', variables)
					return None
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('and.u:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('and.u:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('and.u:rrif', variables)
					return None
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('and:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('and:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('and:rrif', variables)
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('and:zric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 39) & 7) << 8) | (((instruction >> 44) & 1) << 11)
						return Instruction('and:zrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('and:zrif', variables)
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xf))==(0xc):
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('andn.s:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('andn.s:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('andn.s:rrif', variables)
					return None
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('andn.u:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('andn.u:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('andn.u:rrif', variables)
					return None
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('andn:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('andn:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('andn:rrif', variables)
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('andn:zric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 39) & 7) << 8) | (((instruction >> 44) & 1) << 11)
						return Instruction('andn:zrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('andn:zrif', variables)
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xc))==(0x4):
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x8):
							return Instruction('asr.s:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x4):
							return Instruction('lsl.s:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x6):
							return Instruction('lsl1.s:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x7):
							return Instruction('lsl1x.s:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x5):
							return Instruction('lslx.s:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xc):
							return Instruction('lsr.s:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xe):
							return Instruction('lsr1.s:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xf):
							return Instruction('lsr1x.s:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xd):
							return Instruction('lsrx.s:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x2):
							return Instruction('rol.s:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xa):
							return Instruction('ror.s:rri', variables)
						return None
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x8):
							return Instruction('asr.s:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x4):
							return Instruction('lsl.s:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x6):
							return Instruction('lsl1.s:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x7):
							return Instruction('lsl1x.s:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x5):
							return Instruction('lslx.s:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xc):
							return Instruction('lsr.s:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xe):
							return Instruction('lsr1.s:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xf):
							return Instruction('lsr1x.s:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xd):
							return Instruction('lsrx.s:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x2):
							return Instruction('rol.s:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xa):
							return Instruction('ror.s:rric', variables)
						return None
					if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xd)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
						variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x8):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('asr.s:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x4):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl.s:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x6):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl1.s:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x7):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl1x.s:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x5):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lslx.s:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xc):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr.s:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xe):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr1.s:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xf):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr1x.s:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xd):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsrx.s:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x2):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('rol.s:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xa):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('ror.s:rrici', variables)
						return None
					return None
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x8):
							return Instruction('asr.u:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x4):
							return Instruction('lsl.u:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x6):
							return Instruction('lsl1.u:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x7):
							return Instruction('lsl1x.u:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x5):
							return Instruction('lslx.u:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xc):
							return Instruction('lsr.u:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xe):
							return Instruction('lsr1.u:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xf):
							return Instruction('lsr1x.u:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xd):
							return Instruction('lsrx.u:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x2):
							return Instruction('rol.u:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xa):
							return Instruction('ror.u:rri', variables)
						return None
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x8):
							return Instruction('asr.u:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x4):
							return Instruction('lsl.u:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x6):
							return Instruction('lsl1.u:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x7):
							return Instruction('lsl1x.u:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x5):
							return Instruction('lslx.u:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xc):
							return Instruction('lsr.u:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xe):
							return Instruction('lsr1.u:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xf):
							return Instruction('lsr1x.u:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xd):
							return Instruction('lsrx.u:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x2):
							return Instruction('rol.u:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xa):
							return Instruction('ror.u:rric', variables)
						return None
					if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xd)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
						variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x8):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('asr.u:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x4):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl.u:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x6):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl1.u:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x7):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl1x.u:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x5):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lslx.u:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xc):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr.u:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xe):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr1.u:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xf):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr1x.u:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xd):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsrx.u:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x2):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('rol.u:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xa):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('ror.u:rrici', variables)
						return None
					return None
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x8):
							return Instruction('asr:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x6):
							return Instruction('lsl1:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x7):
							return Instruction('lsl1x:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x4):
							return Instruction('lsl:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x5):
							return Instruction('lslx:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xe):
							return Instruction('lsr1:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xf):
							return Instruction('lsr1x:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xc):
							return Instruction('lsr:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xd):
							return Instruction('lsrx:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x2):
							return Instruction('rol:rri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xa):
							return Instruction('ror:rri', variables)
						return None
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x8):
							return Instruction('asr:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x6):
							return Instruction('lsl1:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x7):
							return Instruction('lsl1x:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x4):
							return Instruction('lsl:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x5):
							return Instruction('lslx:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xe):
							return Instruction('lsr1:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xf):
							return Instruction('lsr1x:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xc):
							return Instruction('lsr:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xd):
							return Instruction('lsrx:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x2):
							return Instruction('rol:rric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xa):
							return Instruction('ror:rric', variables)
						return None
					if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xd)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
						variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x8):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('asr:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x6):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl1:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x7):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl1x:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x4):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x5):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lslx:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xe):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr1:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xf):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr1x:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xc):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xd):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsrx:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x2):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('rol:rrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xa):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('ror:rrici', variables)
						return None
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x8):
							return Instruction('asr:zri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x6):
							return Instruction('lsl1:zri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x7):
							return Instruction('lsl1x:zri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x4):
							return Instruction('lsl:zri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x5):
							return Instruction('lslx:zri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xe):
							return Instruction('lsr1:zri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xf):
							return Instruction('lsr1x:zri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xc):
							return Instruction('lsr:zri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xd):
							return Instruction('lsrx:zri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x2):
							return Instruction('rol:zri', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xa):
							return Instruction('ror:zri', variables)
						return None
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x8):
							return Instruction('asr:zric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x6):
							return Instruction('lsl1:zric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x7):
							return Instruction('lsl1x:zric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x4):
							return Instruction('lsl:zric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x5):
							return Instruction('lslx:zric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xe):
							return Instruction('lsr1:zric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xf):
							return Instruction('lsr1x:zric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xc):
							return Instruction('lsr:zric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xd):
							return Instruction('lsrx:zric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x2):
							return Instruction('rol:zric', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xa):
							return Instruction('ror:zric', variables)
						return None
					if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xd)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
						variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x8):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('asr:zrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x6):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl1:zrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x7):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl1x:zrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x4):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl:zrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x5):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lslx:zrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xe):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr1:zrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xf):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr1x:zrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xc):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr:zrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xd):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsrx:zrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0x2):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('rol:zrici', variables)
						if ((((instruction >> 16) & 0xf))&(0xf))==(0xa):
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('ror:zrici', variables)
						return None
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xf))==(0x0):
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['off'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('call:rri', variables)
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['off'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('call:zri', variables)
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xf))==(0x3):
				variables['ra'] = (((instruction >> 34) & 31) << 0)
				if ((((instruction >> 20) & 0xf))&(0x7))==(0x0):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('cao.s:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('cao.s:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('cao.s:rrci', variables)
						return None
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('cao.u:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('cao.u:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('cao.u:rrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('cao:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('cao:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('cao:rrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('cao:zr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('cao:zrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('cao:zrci', variables)
						return None
					return None
				if ((((instruction >> 20) & 0xf))&(0x7))==(0x1):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('clo.s:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('clo.s:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('clo.s:rrci', variables)
						return None
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('clo.u:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('clo.u:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('clo.u:rrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('clo:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('clo:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('clo:rrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('clo:zr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('clo:zrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('clo:zrci', variables)
						return None
					return None
				if ((((instruction >> 20) & 0xf))&(0x7))==(0x2):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('cls.s:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('cls.s:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('cls.s:rrci', variables)
						return None
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('cls.u:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('cls.u:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('cls.u:rrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('cls:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('cls:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('cls:rrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('cls:zr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('cls:zrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('cls:zrci', variables)
						return None
					return None
				if ((((instruction >> 20) & 0xf))&(0x7))==(0x3):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('clz.s:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('clz.s:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('clz.s:rrci', variables)
						return None
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('clz.u:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('clz.u:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('clz.u:rrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('clz:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('clz:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('clz:rrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('clz:zr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('clz:zrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('clz:zrci', variables)
						return None
					return None
				if ((((instruction >> 20) & 0xf))&(0x7))==(0x5):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('extsb.s:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('extsb.s:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('extsb.s:rrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('extsb:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('extsb:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('extsb:rrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('extsb:zr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('extsb:zrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('extsb:zrci', variables)
						return None
					return None
				if ((((instruction >> 20) & 0xf))&(0x7))==(0x7):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('extsh.s:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('extsh.s:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('extsh.s:rrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('extsh:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('extsh:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('extsh:rrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('extsh:zr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('extsh:zrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('extsh:zrci', variables)
						return None
					return None
				if ((((instruction >> 20) & 0xf))&(0x7))==(0x4):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('extub.u:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('extub.u:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('extub.u:rrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('extub:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('extub:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('extub:rrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('extub:zr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('extub:zrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('extub:zrci', variables)
						return None
					return None
				if ((((instruction >> 20) & 0xf))&(0x7))==(0x6):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('extuh.u:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('extuh.u:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('extuh.u:rrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('extuh:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('extuh:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('extuh:rrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('extuh:zr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('extuh:zrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('extuh:zrci', variables)
						return None
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xf))==(0x1):
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('hash.s:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('hash.s:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('hash.s:rrif', variables)
					return None
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('hash.u:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('hash.u:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('hash.u:rrif', variables)
					return None
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('hash:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('hash:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('hash:rrif', variables)
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('hash:zric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 39) & 7) << 8) | (((instruction >> 44) & 1) << 11)
						return Instruction('hash:zrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('hash:zrif', variables)
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xf))==(0xf):
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nand.s:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('nand.s:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nand.s:rrif', variables)
					return None
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nand.u:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('nand.u:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nand.u:rrif', variables)
					return None
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nand:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('nand:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nand:rrif', variables)
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('nand:zric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 39) & 7) << 8) | (((instruction >> 44) & 1) << 11)
						return Instruction('nand:zrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('nand:zrif', variables)
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xf))==(0xe):
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nor.s:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('nor.s:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nor.s:rrif', variables)
					return None
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nor.u:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('nor.u:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nor.u:rrif', variables)
					return None
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nor:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('nor:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nor:rrif', variables)
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('nor:zric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 39) & 7) << 8) | (((instruction >> 44) & 1) << 11)
						return Instruction('nor:zrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('nor:zrif', variables)
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xf))==(0x9):
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nxor.s:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('nxor.s:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nxor.s:rrif', variables)
					return None
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nxor.u:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('nxor.u:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nxor.u:rrif', variables)
					return None
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nxor:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('nxor:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('nxor:rrif', variables)
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('nxor:zric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 39) & 7) << 8) | (((instruction >> 44) & 1) << 11)
						return Instruction('nxor:zrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('nxor:zrif', variables)
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xf))==(0xb):
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('or.s:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('or.s:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('or.s:rrif', variables)
					return None
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('or.u:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('or.u:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('or.u:rrif', variables)
					return None
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('or:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('or:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('or:rrif', variables)
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('or:zric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 39) & 7) << 8) | (((instruction >> 44) & 1) << 11)
						return Instruction('or:zrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('or:zrif', variables)
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xf))==(0xd):
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('orn.s:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('orn.s:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('orn.s:rrif', variables)
					return None
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('orn.u:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('orn.u:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('orn.u:rrif', variables)
					return None
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('orn:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('orn:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('orn:rrif', variables)
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('orn:zric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 39) & 7) << 8) | (((instruction >> 44) & 1) << 11)
						return Instruction('orn:zrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('orn:zrif', variables)
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xf))==(0x2):
				variables['ra'] = (((instruction >> 34) & 31) << 0)
				if ((((instruction >> 20) & 0xf))&(0xc))==(0x0):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('sats.s:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('sats.s:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('sats.s:rrci', variables)
						return None
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('sats.u:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('sats.u:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('sats.u:rrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('sats:rr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('sats:rrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('sats:rrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('sats:zr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('sats:zrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('sats:zrci', variables)
						return None
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xf))==(0x8):
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('xor.s:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('xor.s:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('xor.s:rrif', variables)
					return None
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('xor.u:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('xor.u:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('xor.u:rrif', variables)
					return None
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('xor:rric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4)
						return Instruction('xor:rrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12)
						return Instruction('xor:rrif', variables)
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('xor:zric', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 39) & 7) << 8) | (((instruction >> 44) & 1) << 11)
						return Instruction('xor:zrici', variables)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['imm'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 16) & 15) << 4) | (((instruction >> 15) & 1) << 8) | (((instruction >> 14) & 1) << 9) | (((instruction >> 13) & 1) << 10) | (((instruction >> 12) & 1) << 11) | (((instruction >> 0) & 4095) << 12) | (((instruction >> 39) & 7) << 24) | (((instruction >> 44) & 1) << 27)
						return Instruction('xor:zrif', variables)
					return None
				return None
			return None
		if ((((instruction >> 32) & 0x3))!=(0x3)) and ((((instruction >> 16) & 0x1))==(0x0)):
			variables['rb'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
			if ((((instruction >> 28) & 0xf))&(0xf))==(0xc):
				if ((((instruction >> 20) & 0xf))&(0xf))==(0xa):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('and.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('and.s:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('and.s:rrrci', variables)
						return None
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('and.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('and.u:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('and.u:rrrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('and:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('and:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('and:rrrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('and:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('and:zrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('and:zrrci', variables)
						return None
					return None
				if ((((instruction >> 20) & 0xf))&(0xf))==(0xc):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('andn.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('andn.s:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('andn.s:rrrci', variables)
						return None
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('andn.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('andn.u:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('andn.u:rrrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('andn:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('andn:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('andn:rrrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('andn:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('andn:zrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('andn:zrrci', variables)
						return None
					return None
				if ((((instruction >> 20) & 0xf))&(0xf))==(0x0):
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('call:rrr', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('call:zrr', variables)
						return None
					return None
				if ((((instruction >> 20) & 0xf))&(0xf))==(0x1):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('hash.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('hash.s:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('hash.s:rrrci', variables)
						return None
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('hash.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('hash.u:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('hash.u:rrrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('hash:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('hash:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('hash:rrrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('hash:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('hash:zrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('hash:zrrci', variables)
						return None
					return None
				if ((((instruction >> 20) & 0xf))&(0xf))==(0xf):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('nand.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('nand.s:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('nand.s:rrrci', variables)
						return None
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('nand.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('nand.u:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('nand.u:rrrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('nand:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('nand:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('nand:rrrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('nand:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('nand:zrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('nand:zrrci', variables)
						return None
					return None
				if ((((instruction >> 20) & 0xf))&(0xf))==(0xe):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('nor.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('nor.s:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('nor.s:rrrci', variables)
						return None
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('nor.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('nor.u:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('nor.u:rrrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('nor:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('nor:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('nor:rrrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('nor:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('nor:zrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('nor:zrrci', variables)
						return None
					return None
				if ((((instruction >> 20) & 0xf))&(0xf))==(0x9):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('nxor.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('nxor.s:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('nxor.s:rrrci', variables)
						return None
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('nxor.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('nxor.u:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('nxor.u:rrrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('nxor:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('nxor:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('nxor:rrrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('nxor:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('nxor:zrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('nxor:zrrci', variables)
						return None
					return None
				if ((((instruction >> 20) & 0xf))&(0xf))==(0xb):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('or.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('or.s:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('or.s:rrrci', variables)
						return None
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('or.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('or.u:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('or.u:rrrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('or:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('or:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('or:rrrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('or:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('or:zrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('or:zrrci', variables)
						return None
					return None
				if ((((instruction >> 20) & 0xf))&(0xf))==(0xd):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('orn.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('orn.s:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('orn.s:rrrci', variables)
						return None
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('orn.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('orn.u:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('orn.u:rrrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('orn:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('orn:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('orn:rrrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('orn:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('orn:zrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('orn:zrrci', variables)
						return None
					return None
				if ((((instruction >> 20) & 0xf))&(0xf))==(0x8):
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('xor.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('xor.s:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('xor.s:rrrci', variables)
						return None
					if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
						variables['dc'] = (((instruction >> 40) & 15) << 1)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('xor.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('xor.u:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('xor.u:rrrci', variables)
						return None
					if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
						variables['rc'] = (((instruction >> 39) & 31) << 0)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('xor:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('xor:rrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('xor:rrrci', variables)
						return None
					if (((instruction >> 42) & 0x3))==(0x3):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if (((instruction >> 24) & 0xf))==(0x0):
							return Instruction('xor:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('xor:zrrc', variables)
						if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('xor:zrrci', variables)
						return None
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xd))==(0xd):
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x8):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('asr.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('asr.s:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('asr.s:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x4):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsl.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsl.s:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl.s:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x6):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsl1.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsl1.s:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl1.s:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x7):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsl1x.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsl1x.s:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl1x.s:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x5):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lslx.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lslx.s:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lslx.s:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xc):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsr.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsr.s:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr.s:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xe):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsr1.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsr1.s:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr1.s:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xf):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsr1x.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsr1x.s:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr1x.s:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xd):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsrx.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsrx.s:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsrx.s:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x2):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('rol.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('rol.s:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('rol.s:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xa):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('ror.s:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('ror.s:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('ror.s:rrrci', variables)
						return None
					return None
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x8):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('asr.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('asr.u:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('asr.u:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x4):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsl.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsl.u:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl.u:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x6):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsl1.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsl1.u:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl1.u:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x7):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsl1x.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsl1x.u:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl1x.u:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x5):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lslx.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lslx.u:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lslx.u:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xc):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsr.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsr.u:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr.u:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xe):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsr1.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsr1.u:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr1.u:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xf):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsr1x.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsr1x.u:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr1x.u:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xd):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsrx.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsrx.u:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsrx.u:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x2):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('rol.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('rol.u:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('rol.u:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xa):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('ror.u:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('ror.u:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('ror.u:rrrci', variables)
						return None
					return None
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x8):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('asr:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('asr:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('asr:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x6):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsl1:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsl1:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl1:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x7):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsl1x:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsl1x:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl1x:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x4):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsl:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsl:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x5):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lslx:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lslx:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lslx:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xe):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsr1:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsr1:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr1:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xf):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsr1x:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsr1x:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr1x:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xc):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsr:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsr:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xd):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsrx:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsrx:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsrx:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x2):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('rol:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('rol:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('rol:rrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xa):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('ror:rrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('ror:rrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('ror:rrrci', variables)
						return None
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x8):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('asr:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('asr:zrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('asr:zrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x6):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsl1:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsl1:zrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl1:zrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x7):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsl1x:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsl1x:zrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl1x:zrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x4):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsl:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsl:zrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsl:zrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x5):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lslx:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lslx:zrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lslx:zrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xe):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsr1:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsr1:zrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr1:zrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xf):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsr1x:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsr1x:zrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr1x:zrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xc):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsr:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsr:zrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsr:zrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xd):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('lsrx:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('lsrx:zrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('lsrx:zrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0x2):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('rol:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('rol:zrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('rol:zrrci', variables)
						return None
					if ((((instruction >> 20) & 0xf))&(0xf))==(0xa):
						if ((((instruction >> 24) & 0xf))==(0x0)) and ((((instruction >> 29) & 0x1))==(0x0)):
							return Instruction('ror:zrr', variables)
						if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							return Instruction('ror:zrrc', variables)
						if ((((((instruction >> 29) & 0x1))==(0x0)) and (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) or (((((instruction >> 29) & 0x1))==(0x1)) and (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8))))) and (((((instruction >> 29) & 0x1))!=(0x0)) or ((((instruction >> 24) & 0xf))!=(0x0))):
							variables['cc'] = (((instruction >> 24) & 15) << 0) | (((instruction >> 29) & 1) << 4)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('ror:zrrci', variables)
						return None
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xc))==(0x8):
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						return Instruction('cmpb4.s:rrr', variables)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						return Instruction('cmpb4.s:rrrc', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						return Instruction('cmpb4.s:rrrci', variables)
					return None
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						return Instruction('cmpb4.u:rrr', variables)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						return Instruction('cmpb4.u:rrrc', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						return Instruction('cmpb4.u:rrrci', variables)
					return None
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						return Instruction('cmpb4:rrr', variables)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						return Instruction('cmpb4:rrrc', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						return Instruction('cmpb4:rrrci', variables)
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						return Instruction('cmpb4:zrr', variables)
					if (((((instruction >> 26) & 0x1))^(((instruction >> 27) & 0x1)))==(0x1)) and ((((instruction >> 25) & 0x1))==(0x1)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						return Instruction('cmpb4:zrrc', variables)
					if (((((instruction >> 24) & 0xf))<(0x6)) or (((((instruction >> 24) & 0xf))>(0xb)) or (((((instruction >> 24) & 0xf))&(0xe))==(0x8)))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						return Instruction('cmpb4:zrrci', variables)
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xe))==(0x4):
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsl_add.s:rrri', variables)
					if (((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsl_add.s:rrrici', variables)
					return None
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsl_add.u:rrri', variables)
					if (((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsl_add.u:rrrici', variables)
					return None
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsl_add:rrri', variables)
					if (((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsl_add:rrrici', variables)
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsl_add:zrri', variables)
					if (((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsl_add:zrrici', variables)
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xe))==(0x6):
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsl_sub.s:rrri', variables)
					if (((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsl_sub.s:rrrici', variables)
					return None
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsl_sub.u:rrri', variables)
					if (((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsl_sub.u:rrrici', variables)
					return None
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsl_sub:rrri', variables)
					if (((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsl_sub:rrrici', variables)
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsl_sub:zrri', variables)
					if (((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsl_sub:zrrici', variables)
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xe))==(0x2):
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsr_add.s:rrri', variables)
					if (((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsr_add.s:rrrici', variables)
					return None
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsr_add.u:rrri', variables)
					if (((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsr_add.u:rrrici', variables)
					return None
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsr_add:rrri', variables)
					if (((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsr_add:rrrici', variables)
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsr_add:zrri', variables)
					if (((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('lsr_add:zrrici', variables)
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xe))==(0x0):
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x1)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('rol_add.s:rrri', variables)
					if (((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('rol_add.s:rrrici', variables)
					return None
				if (((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x1))) and ((((instruction >> 39) & 0x1))==(0x0)):
					variables['dc'] = (((instruction >> 40) & 15) << 1)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('rol_add.u:rrri', variables)
					if (((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('rol_add.u:rrrici', variables)
					return None
				if ((((instruction >> 42) & 0x3))!=(0x3)) and ((((instruction >> 44) & 0x1))==(0x0)):
					variables['rc'] = (((instruction >> 39) & 31) << 0)
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('rol_add:rrri', variables)
					if (((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('rol_add:rrrici', variables)
					return None
				if (((instruction >> 42) & 0x3))==(0x3):
					variables['ra'] = (((instruction >> 34) & 31) << 0)
					if (((instruction >> 24) & 0xf))==(0x0):
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('rol_add:zrri', variables)
					if (((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb))) and ((((instruction >> 24) & 0xf))!=(0x0)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('rol_add:zrrici', variables)
					return None
				return None
			return None
		if ((((instruction >> 32) & 0x3))!=(0x3)) and ((((instruction >> 16) & 0x1))==(0x1)):
			variables['db'] = (((instruction >> 17) & 7) << 0) | (((instruction >> 32) & 3) << 3)
			if ((((instruction >> 28) & 0xf))&(0xe))==(0x6):
				if (((instruction >> 42) & 0x3))!=(0x3):
					variables['dc'] = (((instruction >> 39) & 31) << 0)
					if ((((instruction >> 24) & 0xf))<(0x2)) or ((((instruction >> 24) & 0xf))>(0xb)):
						variables['cc'] = (((instruction >> 24) & 15) << 0)
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						variables['pc'] = (((instruction >> 0) & 65535) << 0)
						variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
						return Instruction('div_step:rrrici', variables)
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xf))==(0x2):
				if (((instruction >> 42) & 0x3))!=(0x3):
					variables['dc'] = (((instruction >> 39) & 31) << 0)
					if (((instruction >> 34) & 0x1f))==(0x18):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if ((((instruction >> 24) & 0xf))<(0x6)) or ((((instruction >> 24) & 0xf))>(0xb)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('movd:rrci', variables)
						return None
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xe))==(0x4):
				if (((instruction >> 42) & 0x3))!=(0x3):
					variables['dc'] = (((instruction >> 39) & 31) << 0)
					if (((instruction >> 34) & 0x1f))!=(0x18):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if ((((instruction >> 24) & 0xf))<(0x6)) or ((((instruction >> 24) & 0xf))>(0xb)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							variables['shift'] = (((instruction >> 20) & 15) << 0) | (((instruction >> 28) & 1) << 4)
							return Instruction('mul_step:rrrici', variables)
						return None
					return None
				return None
			if ((((instruction >> 28) & 0xf))&(0xf))==(0xa):
				if (((instruction >> 42) & 0x3))!=(0x3):
					variables['dc'] = (((instruction >> 39) & 31) << 0)
					if (((instruction >> 34) & 0x1f))==(0x18):
						variables['ra'] = (((instruction >> 34) & 31) << 0)
						if ((((instruction >> 24) & 0xf))<(0x6)) or ((((instruction >> 24) & 0xf))>(0xb)):
							variables['cc'] = (((instruction >> 24) & 15) << 0)
							variables['pc'] = (((instruction >> 0) & 65535) << 0)
							return Instruction('swapd:rrci', variables)
						return None
					return None
				return None
			return None
		return None
	return None
