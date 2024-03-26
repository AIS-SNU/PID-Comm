# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


_DECODING_MAPS = {
	'acquire_cc': { 1: 'true', 2: 'z', 3: 'nz' },
	'add_nz_cc': { 1: 'true', 2: 'z', 3: 'nz', 4: 'xz', 5: 'xnz', 8: 'pl', 9: 'mi', 12: 'sz', 13: 'snz', 14: 'spl', 15: 'smi', 18: 'ov', 19: 'nov', 20: 'c', 21: 'nc', 22: 'nc5', 23: 'nc6', 24: 'nc7', 25: 'nc8', 26: 'nc9', 27: 'nc10', 28: 'nc11', 29: 'nc12', 30: 'nc13', 31: 'nc14' },
	'boot_cc': { 0: 'false', 1: 'true', 2: 'z', 3: 'nz', 4: 'xz', 5: 'xnz', 12: 'sz', 13: 'snz', 14: 'spl', 15: 'smi' },
	'const_cc_ge0': { 0: 'pl' },
	'const_cc_geu': { 0: 'geu' },
	'const_cc_zero': { 0: 'z' },
	'count_nz_cc': { 1: 'true', 2: 'z', 3: 'nz', 4: 'xz', 5: 'xnz', 8: 'max', 9: 'nmax', 12: 'sz', 13: 'snz', 14: 'spl', 15: 'smi' },
	'div_cc': { 0: 'false', 1: 'true', 12: 'sz', 13: 'snz', 14: 'spl', 15: 'smi' },
	'div_nz_cc': { 1: 'true', 12: 'sz', 13: 'snz', 14: 'spl', 15: 'smi' },
	'ext_sub_set_cc': { 6: 'z', 7: 'nz', 10: 'xz', 11: 'xnz', 33: 'true', 34: 'z', 35: 'nz', 36: 'xz', 37: 'xnz', 40: 'pl', 41: 'mi', 44: 'sz', 45: 'snz', 46: 'spl', 47: 'smi', 50: 'ov', 51: 'nov', 52: 'ltu', 53: 'geu', 54: 'lts', 55: 'ges', 56: 'les', 57: 'gts', 58: 'leu', 59: 'gtu', 60: 'xles', 61: 'xgts', 62: 'xleu', 63: 'xgtu' },
	'false_cc': { 0: 'false' },
	'imm_shift_nz_cc': { 1: 'true', 2: 'z', 3: 'nz', 4: 'xz', 5: 'xnz', 8: 'pl', 9: 'mi', 12: 'sz', 13: 'snz', 14: 'spl', 15: 'smi', 24: 'e', 25: 'o', 30: 'se', 31: 'so' },
	'log_nz_cc': { 1: 'true', 2: 'z', 3: 'nz', 4: 'xz', 5: 'xnz', 8: 'pl', 9: 'mi', 12: 'sz', 13: 'snz', 14: 'spl', 15: 'smi' },
	'log_set_cc': { 6: 'z', 7: 'nz', 10: 'xz', 11: 'xnz' },
	'mul_nz_cc': { 1: 'true', 2: 'z', 3: 'nz', 4: 'xz', 5: 'xnz', 12: 'sz', 13: 'snz', 14: 'spl', 15: 'smi', 30: 'small', 31: 'large' },
	'no_cc': {  },
	'release_cc': { 0: 'nz' },
	'shift_nz_cc': { 1: 'true', 2: 'z', 3: 'nz', 4: 'xz', 5: 'xnz', 8: 'pl', 9: 'mi', 12: 'sz', 13: 'snz', 14: 'spl', 15: 'smi', 24: 'e', 25: 'o', 28: 'nsh32', 29: 'sh32', 30: 'se', 31: 'so' },
	'sub_nz_cc': { 1: 'true', 2: 'z', 3: 'nz', 4: 'xz', 5: 'xnz', 8: 'pl', 9: 'mi', 12: 'sz', 13: 'snz', 14: 'spl', 15: 'smi', 18: 'ov', 19: 'nov', 20: 'ltu', 21: 'geu', 22: 'lts', 23: 'ges', 24: 'les', 25: 'gts', 26: 'leu', 27: 'gtu', 28: 'xles', 29: 'xgts', 30: 'xleu', 31: 'xgtu' },
	'sub_set_cc': { 6: 'z', 7: 'nz', 10: 'xz', 11: 'xnz' },
	'true_cc': { 1: 'true' },
	'true_false_cc': { 0: 'false', 1: 'true' }
}


class InstructionSyntaxPart:
	pass


class WorkRegister(InstructionSyntaxPart):
	def __init__(self, name, bit_length, can_be_ro, is_safe):
		self.name = name
		self.bit_length = bit_length
		self.can_be_ro = can_be_ro
		self.is_safe = is_safe

	def __str__(self):
		return self.name

	def format(self, value):
		if value is None:
			return '???'
		if (value >= 0) and (value < 24):
			prefix = 'd' if self.bit_length == 64 else ('s' if self.is_safe else 'r')
			return prefix + str(value)
		elif self.can_be_ro:
			if value == 24:
				return 'zero'
			elif value == 25:
				return 'one'
			elif value == 26:
				return 'lneg'
			elif value == 27:
				return 'mneg'
			elif value == 28:
				return 'id'
			elif value == 29:
				return 'id2'
			elif value == 30:
				return 'id4'
			elif value == 31:
				return 'id8'
			else:
				return '???'
		else:
			return '???'


class ConstantRegister(InstructionSyntaxPart):
	def __init__(self, name):
		self.name = name

	def __str__(self):
		return self.name

	def format(self, value):
		return self.name


class Endianess(InstructionSyntaxPart):
	def __init__(self, name):
		self.name = name

	def __str__(self):
		return self.name + ':e'

	def format(self, value):
		if value is None:
			return '???'
		return '!little' if value == 0 else '!big'


class Condition(InstructionSyntaxPart):
	def __init__(self, name):
		self.name = name
		self.decoding = _DECODING_MAPS[name]

	def __str__(self):
		return self.name + ':cc'

	def format(self, value):
		if value is None:
			return '' if self.name == 'false_cc' else '???'
		decoded = self.decoding.get(value)
		return decoded if decoded is not None else '???'


class PcSpec(InstructionSyntaxPart):
	def __init__(self, name, bit_length):
		self.name = name
		self.bit_length = bit_length

	def __str__(self):
		return self.name + ':pc' + str(self.bit_length)

	def format(self, value):
		return hex(value) if value is not None else '???'


class Immediate(InstructionSyntaxPart):
	def __init__(self, name, bit_length, is_signed):
		self.name = name
		self.bit_length = bit_length
		self.is_signed = is_signed

	def __str__(self):
		return self.name + ':' + ('s' if self.is_signed else 'u') + str(self.bit_length)

	def format(self, value):
		if value is None:
			return '???'
		value = value & ((1 << self.bit_length) - 1)
		if self.is_signed:
			sign_mask = 1 << (self.bit_length - 1)
			value = (value ^ sign_mask) - sign_mask
			return str(value)
		else:
			return hex(value)


class InstructionInfo:
	def __init__(self, keyword, signature, syntax, brief, encoding, is_documented, is_syntactic_sugar, syntactic_sugars):
		self.keyword = keyword
		self.signature = signature
		self.syntax = syntax
		self.brief = brief
		self.encoding = encoding
		self.is_documented = is_documented
		self.is_syntactic_sugar = is_syntactic_sugar
		self.syntactic_sugars = syntactic_sugars


INSTRUCTIONS = {
	'acquire:rici': InstructionInfo('acquire', 'acquire:rici', [WorkRegister('ra', 32, True, False), Immediate('imm', 16, True), Condition('acquire_cc'), PcSpec('pc', 16)], '''	cc = (ZF <- acquire((ra + imm)[8:15] ^ (ra + imm)[0:7]))
	if (acquire_cc cc) then
		jump @[pc]''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode1xxxx', 'raAllEnabled', 'immediate16Atomic', 'targetPc', 'acquireCondition'], True, False, {}),
	'add.s:rri': InstructionInfo('add.s', 'add.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), Immediate('imm', 32, False)], '''	dc = (rb + imm):S64
	ZF,CF <- dc''', ['opCode0Or1', 'rbEnabled', 'rcSignedExtendedEnabled', 'raDisabled', 'immediate32DusRb'], True, False, {}),
	'add.s:rric': InstructionInfo('add.s', 'add.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ra + imm
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode0000x', 'unsafeRegister', 'addSetCondition', 'immediate24imm'], True, False, {}),
	'add.s:rrici': InstructionInfo('add.s', 'add.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('add_nz_cc'), PcSpec('pc', 16)], '''	x = ra + imm
	dc = x:S64
	if (add_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode0000x', 'unsafeRegister', 'nzAddCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'add.s:rrif': InstructionInfo('add.s', 'add.s:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra + imm
	dc = x:S64
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode0000x', 'unsafeRegister', 'falseCondition5', 'immediate24imm'], True, False, {}),
	'add.s:rrr': InstructionInfo('add.s', 'add.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra + rb
	dc = x:S64
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode0000x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition5'], True, False, {}),
	'add.s:rrrc': InstructionInfo('add.s', 'add.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra + rb
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode0000x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'addSetCondition'], True, False, {}),
	'add.s:rrrci': InstructionInfo('add.s', 'add.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('add_nz_cc'), PcSpec('pc', 16)], '''	x = ra + rb
	dc = x:S64
	if (add_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode0000x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzAddCondition', 'targetPc'], True, False, {}),
	'add.u:rri': InstructionInfo('add.u', 'add.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), Immediate('imm', 32, False)], '''	dc = (rb + imm):U64
	ZF,CF <- dc''', ['opCode0Or1', 'rbEnabled', 'rcUnsignedExtendedEnabled', 'raDisabled', 'immediate32DusRb'], True, False, {}),
	'add.u:rric': InstructionInfo('add.u', 'add.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ra + imm
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode0000x', 'unsafeRegister', 'addSetCondition', 'immediate24imm'], True, False, {}),
	'add.u:rrici': InstructionInfo('add.u', 'add.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('add_nz_cc'), PcSpec('pc', 16)], '''	x = ra + imm
	dc = x:U64
	if (add_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode0000x', 'unsafeRegister', 'nzAddCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'add.u:rrif': InstructionInfo('add.u', 'add.u:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra + imm
	dc = x:U64
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode0000x', 'unsafeRegister', 'falseCondition5', 'immediate24imm'], True, False, {}),
	'add.u:rrr': InstructionInfo('add.u', 'add.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra + rb
	dc = x:U64
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode0000x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition5'], True, False, {}),
	'add.u:rrrc': InstructionInfo('add.u', 'add.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra + rb
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode0000x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'addSetCondition'], True, False, {}),
	'add.u:rrrci': InstructionInfo('add.u', 'add.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('add_nz_cc'), PcSpec('pc', 16)], '''	x = ra + rb
	dc = x:U64
	if (add_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode0000x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzAddCondition', 'targetPc'], True, False, {}),
	'add:rri': InstructionInfo('add', 'add:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 32, False)], '''	rc = (ra + imm)
	ZF,CF <- rc''', ['opCode0Or1', 'rbDisabled', 'opCode0', 'rcEnabled', 'raAllEnabled', 'immediate32'], True, False, {}),
	'add:rric': InstructionInfo('add', 'add:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ra + imm
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode0000x', 'unsafeRegister', 'addSetCondition', 'immediate24imm'], True, False, {}),
	'add:rrici': InstructionInfo('add', 'add:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('add_nz_cc'), PcSpec('pc', 16)], '''	x = ra + imm
	rc = x
	if (add_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode0000x', 'unsafeRegister', 'nzAddCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'add:rrif': InstructionInfo('add', 'add:rrif', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra + imm
	rc = x
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode0000x', 'unsafeRegister', 'falseCondition5', 'immediate24imm'], True, False, {}),
	'add:rrr': InstructionInfo('add', 'add:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra + rb
	rc = x
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode0000x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition5'], True, False, {}),
	'add:rrrc': InstructionInfo('add', 'add:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra + rb
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode0000x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'addSetCondition'], True, False, {}),
	'add:rrrci': InstructionInfo('add', 'add:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('add_nz_cc'), PcSpec('pc', 16)], '''	x = ra + rb
	rc = x
	if (add_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode0000x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzAddCondition', 'targetPc'], True, False, {}),
	'add:ssi': InstructionInfo('add', 'add:ssi', [WorkRegister('sc', 32, False, True), WorkRegister('sa', 32, True, True), Immediate('imm', 17, True)], '''	cc = (ra & 0xffff) + imm >> 16
	if (const_cc_zero cc) then
		ZF,CF <- rc = (ra + imm)
	else
		ZF,CF <- rc = (ra + imm) & 0xffffff | (id << 25)
		raise exception(_memory_fault)''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode0000x', 'safeRegister', 'immediate17_24imm'], True, False, {'adds:rri': (0, 0)}),
	'add:sss': InstructionInfo('add', 'add:sss', [WorkRegister('sc', 32, False, True), WorkRegister('sa', 32, True, True), WorkRegister('sb', 32, False, True)], '''	cc = (ra & 0xffff) + rb >> 16
	if (const_cc_zero cc) then
		ZF,CF <- rc = (ra + rb)
	else
		ZF,CF <- rc = (ra + rb) & 0xffffff | (id << 25)
		raise exception(_memory_fault)''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode0000x', 'safeRegister', 'raRbArg', 'rbRaArg'], True, False, {'adds:rrr': (0, 0)}),
	'add:zri': InstructionInfo('add', 'add:zri', [ConstantRegister('zero'), WorkRegister('rb', 32, False, False), Immediate('imm', 32, False)], '''	ZF,CF <- rb + imm''', ['opCode6', 'rbEnabled', 'raDisabled', 'immediate32ZeroRb', 'cstRc00000x'], True, False, {}),
	'add:zric': InstructionInfo('add', 'add:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 27, True), Condition('log_set_cc')], '''	ZF,CF <- ra + imm''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode0000x', 'unsafeRegister', 'addSetCondition', 'immediate27imm'], True, False, {}),
	'add:zrici': InstructionInfo('add', 'add:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 11, True), Condition('add_nz_cc'), PcSpec('pc', 16)], '''	x = ra + imm
	if (add_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode0000x', 'unsafeRegister', 'nzAddCondition', 'immediate27Pc', 'targetPc'], True, False, {}),
	'add:zrif': InstructionInfo('add', 'add:zrif', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 27, True), Condition('false_cc')], '''	ZF,CF <- ra + imm''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode0000x', 'unsafeRegister', 'falseCondition5', 'immediate27imm'], True, False, {}),
	'add:zrr': InstructionInfo('add', 'add:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF,CF <- ra + rb''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode0000x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition5'], True, False, {}),
	'add:zrrc': InstructionInfo('add', 'add:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF,CF <- ra + rb''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode0000x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'addSetCondition'], True, False, {}),
	'add:zrrci': InstructionInfo('add', 'add:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('add_nz_cc'), PcSpec('pc', 16)], '''	x = ra + rb
	if (add_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode0000x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzAddCondition', 'targetPc'], True, False, {}),
	'addc.s:rric': InstructionInfo('addc.s', 'addc.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ra + imm + CF
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode0011x', 'unsafeRegister', 'addSetCondition', 'immediate24imm'], True, False, {}),
	'addc.s:rrici': InstructionInfo('addc.s', 'addc.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('add_nz_cc'), PcSpec('pc', 16)], '''	x = ra + imm + CF
	dc = x:S64
	if (add_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode0011x', 'unsafeRegister', 'nzAddCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'addc.s:rrif': InstructionInfo('addc.s', 'addc.s:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra + imm + CF
	dc = x:S64
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode0011x', 'unsafeRegister', 'falseCondition5', 'immediate24imm'], True, False, {}),
	'addc.s:rrr': InstructionInfo('addc.s', 'addc.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra + rb + CF
	dc = x:S64
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode0011x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition5'], True, False, {}),
	'addc.s:rrrc': InstructionInfo('addc.s', 'addc.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra + rb + CF
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode0011x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'addSetCondition'], True, False, {}),
	'addc.s:rrrci': InstructionInfo('addc.s', 'addc.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('add_nz_cc'), PcSpec('pc', 16)], '''	x = ra + rb + CF
	dc = x:S64
	if (add_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode0011x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzAddCondition', 'targetPc'], True, False, {}),
	'addc.u:rric': InstructionInfo('addc.u', 'addc.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ra + imm + CF
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode0011x', 'unsafeRegister', 'addSetCondition', 'immediate24imm'], True, False, {}),
	'addc.u:rrici': InstructionInfo('addc.u', 'addc.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('add_nz_cc'), PcSpec('pc', 16)], '''	x = ra + imm + CF
	dc = x:U64
	if (add_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode0011x', 'unsafeRegister', 'nzAddCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'addc.u:rrif': InstructionInfo('addc.u', 'addc.u:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra + imm + CF
	dc = x:U64
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode0011x', 'unsafeRegister', 'falseCondition5', 'immediate24imm'], True, False, {}),
	'addc.u:rrr': InstructionInfo('addc.u', 'addc.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra + rb + CF
	dc = x:U64
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode0011x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition5'], True, False, {}),
	'addc.u:rrrc': InstructionInfo('addc.u', 'addc.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra + rb + CF
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode0011x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'addSetCondition'], True, False, {}),
	'addc.u:rrrci': InstructionInfo('addc.u', 'addc.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('add_nz_cc'), PcSpec('pc', 16)], '''	x = ra + rb + CF
	dc = x:U64
	if (add_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode0011x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzAddCondition', 'targetPc'], True, False, {}),
	'addc:rri': InstructionInfo('addc', 'addc:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 32, False)], '''	rc = (ra + imm + CF)
	ZF,CF <- rc''', ['opCode0Or1', 'rbDisabled', 'opCode1', 'rcEnabled', 'raAllEnabled', 'immediate32'], True, False, {}),
	'addc:rric': InstructionInfo('addc', 'addc:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ra + imm + CF
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode0011x', 'unsafeRegister', 'addSetCondition', 'immediate24imm'], True, False, {}),
	'addc:rrici': InstructionInfo('addc', 'addc:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('add_nz_cc'), PcSpec('pc', 16)], '''	x = ra + imm + CF
	rc = x
	if (add_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode0011x', 'unsafeRegister', 'nzAddCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'addc:rrif': InstructionInfo('addc', 'addc:rrif', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra + imm + CF
	rc = x
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode0011x', 'unsafeRegister', 'falseCondition5', 'immediate24imm'], True, False, {}),
	'addc:rrr': InstructionInfo('addc', 'addc:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra + rb + CF
	rc = x
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode0011x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition5'], True, False, {}),
	'addc:rrrc': InstructionInfo('addc', 'addc:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra + rb + CF
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode0011x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'addSetCondition'], True, False, {}),
	'addc:rrrci': InstructionInfo('addc', 'addc:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('add_nz_cc'), PcSpec('pc', 16)], '''	x = ra + rb + CF
	rc = x
	if (add_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode0011x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzAddCondition', 'targetPc'], True, False, {}),
	'addc:zri': InstructionInfo('addc', 'addc:zri', [ConstantRegister('zero'), WorkRegister('rb', 32, False, False), Immediate('imm', 32, False)], '''	ZF,CF <- rb + imm + CF''', ['opCode6', 'rbEnabled', 'raDisabled', 'immediate32ZeroRb', 'cstRc00011x'], True, False, {}),
	'addc:zric': InstructionInfo('addc', 'addc:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 27, True), Condition('log_set_cc')], '''	ZF,CF <- ra + imm + CF''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode0011x', 'unsafeRegister', 'addSetCondition', 'immediate27imm'], True, False, {}),
	'addc:zrici': InstructionInfo('addc', 'addc:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 11, True), Condition('add_nz_cc'), PcSpec('pc', 16)], '''	x = ra + imm + CF
	if (add_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode0011x', 'unsafeRegister', 'nzAddCondition', 'immediate27Pc', 'targetPc'], True, False, {}),
	'addc:zrif': InstructionInfo('addc', 'addc:zrif', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 27, True), Condition('false_cc')], '''	ZF,CF <- ra + imm + CF''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode0011x', 'unsafeRegister', 'falseCondition5', 'immediate27imm'], True, False, {}),
	'addc:zrr': InstructionInfo('addc', 'addc:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF,CF <- ra + rb + CF''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode0011x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition5'], True, False, {}),
	'addc:zrrc': InstructionInfo('addc', 'addc:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF,CF <- ra + rb + CF''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode0011x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'addSetCondition'], True, False, {}),
	'addc:zrrci': InstructionInfo('addc', 'addc:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('add_nz_cc'), PcSpec('pc', 16)], '''	x = ra + rb + CF
	if (add_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode0011x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzAddCondition', 'targetPc'], True, False, {}),
	'adds:rri': InstructionInfo('adds', 'adds:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 17, True)], '''	cc = (ra & 0xffff) + imm >> 16
	if (const_cc_zero cc) then
		ZF,CF <- rc = (ra + imm)
	else
		ZF,CF <- rc = (ra + imm) & 0xffffff | (id << 25)
		raise exception(_memory_fault)''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode0000x', 'safeRegister', 'immediate17_24imm', 'syntacticSugar'], True, True, {}),
	'adds:rrr': InstructionInfo('adds', 'adds:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = (ra & 0xffff) + rb >> 16
	if (const_cc_zero cc) then
		ZF,CF <- rc = (ra + rb)
	else
		ZF,CF <- rc = (ra + rb) & 0xffffff | (id << 25)
		raise exception(_memory_fault)''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode0000x', 'safeRegister', 'raRbArg', 'rbRaArg', 'syntacticSugar'], True, True, {}),
	'and.s:rki': InstructionInfo('and.s', 'and.s:rki', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 32, False)], '''	dc = (ra & imm):S64
	ZF <- dc''', ['opCode4Or5', 'rbDisabled', 'opCode5', 'raCstEnabled', 'rcSignedExtendedEnabled', 'immediate32'], True, False, {'move.s:ri': (0x6800000000, 0x7c00000000)}),
	'and.s:rri': InstructionInfo('and.s', 'and.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), Immediate('imm', 32, False)], '''	dc = (rb & imm):S64
	ZF <- dc''', ['opCode2Or3', 'rbEnabled', 'rcSignedExtendedEnabled', 'raDisabled', 'immediate32DusRb'], True, False, {}),
	'and.s:rric': InstructionInfo('and.s', 'and.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ra & imm
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10110', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'and.s:rrici': InstructionInfo('and.s', 'and.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra & imm
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10110', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'and.s:rrif': InstructionInfo('and.s', 'and.s:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra & imm
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10110', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'and.s:rrr': InstructionInfo('and.s', 'and.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra & rb
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10110', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'and.s:rrrc': InstructionInfo('and.s', 'and.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra & rb
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10110', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'and.s:rrrci': InstructionInfo('and.s', 'and.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra & rb
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10110', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'and.u:rki': InstructionInfo('and.u', 'and.u:rki', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 32, False)], '''	dc = (ra & imm):U64
	ZF <- dc''', ['opCode4Or5', 'rbDisabled', 'opCode5', 'raCstEnabled', 'rcUnsignedExtendedEnabled', 'immediate32'], True, False, {'move.u:ri': (0x6800000000, 0x7c00000000)}),
	'and.u:rri': InstructionInfo('and.u', 'and.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), Immediate('imm', 32, False)], '''	dc = (rb & imm):U64
	ZF <- dc''', ['opCode2Or3', 'rbEnabled', 'rcUnsignedExtendedEnabled', 'raDisabled', 'immediate32DusRb'], True, False, {}),
	'and.u:rric': InstructionInfo('and.u', 'and.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ra & imm
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10110', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'and.u:rrici': InstructionInfo('and.u', 'and.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra & imm
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10110', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'and.u:rrif': InstructionInfo('and.u', 'and.u:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra & imm
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10110', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'and.u:rrr': InstructionInfo('and.u', 'and.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra & rb
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10110', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'and.u:rrrc': InstructionInfo('and.u', 'and.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra & rb
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10110', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'and.u:rrrci': InstructionInfo('and.u', 'and.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra & rb
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10110', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'and:rri': InstructionInfo('and', 'and:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, False, False), Immediate('imm', 32, False)], '''	rc = (ra & imm)
	ZF <- rc''', ['opCode4Or5', 'rbDisabled', 'opCode5', 'raEnabled', 'rcEnabled', 'immediate32'], True, False, {}),
	'and:rric': InstructionInfo('and', 'and:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ra & imm
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10110', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'and:rrici': InstructionInfo('and', 'and:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra & imm
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10110', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'and:rrif': InstructionInfo('and', 'and:rrif', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra & imm
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10110', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'and:rrr': InstructionInfo('and', 'and:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra & rb
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10110', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'and:rrrc': InstructionInfo('and', 'and:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra & rb
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10110', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'and:rrrci': InstructionInfo('and', 'and:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra & rb
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10110', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'and:zri': InstructionInfo('and', 'and:zri', [ConstantRegister('zero'), WorkRegister('rb', 32, False, False), Immediate('imm', 32, False)], '''	ZF <- rb & imm''', ['opCode6', 'rbEnabled', 'raDisabled', 'immediate32ZeroRb', 'cstRc01011x'], True, False, {}),
	'and:zric': InstructionInfo('and', 'and:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('log_set_cc')], '''	ZF <- ra & imm''', ['opCode8Or9', 'rbDisabled', 'subOpCode10110', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition', 'immediate28imm'], True, False, {}),
	'and:zrici': InstructionInfo('and', 'and:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 12, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra & imm
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10110', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate28PcOpcode8', 'targetPc'], True, False, {}),
	'and:zrif': InstructionInfo('and', 'and:zrif', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('false_cc')], '''	ZF <- ra & imm''', ['opCode8Or9', 'rbDisabled', 'subOpCode10110', 'rcDisabled', 'raAllEnabled', 'falseCondition4', 'immediate28imm'], True, False, {}),
	'and:zrr': InstructionInfo('and', 'and:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- ra & rb''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10110', 'rcDisabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'and:zrrc': InstructionInfo('and', 'and:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- ra & rb''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10110', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'and:zrrci': InstructionInfo('and', 'and:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra & rb
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10110', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'andn.s:rric': InstructionInfo('andn.s', 'andn.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ~ra & imm
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11000', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'andn.s:rrici': InstructionInfo('andn.s', 'andn.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~ra & imm
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11000', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'andn.s:rrif': InstructionInfo('andn.s', 'andn.s:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ~ra & imm
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11000', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'andn.s:rrr': InstructionInfo('andn.s', 'andn.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ~ra & rb
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11000', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'andn.s:rrrc': InstructionInfo('andn.s', 'andn.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ~ra & rb
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11000', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'andn.s:rrrci': InstructionInfo('andn.s', 'andn.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~ra & rb
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11000', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'andn.u:rric': InstructionInfo('andn.u', 'andn.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ~ra & imm
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11000', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'andn.u:rrici': InstructionInfo('andn.u', 'andn.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~ra & imm
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11000', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'andn.u:rrif': InstructionInfo('andn.u', 'andn.u:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ~ra & imm
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11000', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'andn.u:rrr': InstructionInfo('andn.u', 'andn.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ~ra & rb
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11000', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'andn.u:rrrc': InstructionInfo('andn.u', 'andn.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ~ra & rb
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11000', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'andn.u:rrrci': InstructionInfo('andn.u', 'andn.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~ra & rb
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11000', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'andn:rri': InstructionInfo('andn', 'andn:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True)], '''	x = ~ra & imm
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11000', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm', 'syntacticSugar'], True, True, {}),
	'andn:rric': InstructionInfo('andn', 'andn:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ~ra & imm
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11000', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'andn:rrici': InstructionInfo('andn', 'andn:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~ra & imm
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11000', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'andn:rrif': InstructionInfo('andn', 'andn:rrif', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ~ra & imm
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11000', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {'andn:rri': (0, 0)}),
	'andn:rrr': InstructionInfo('andn', 'andn:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ~ra & rb
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11000', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'andn:rrrc': InstructionInfo('andn', 'andn:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ~ra & rb
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11000', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'andn:rrrci': InstructionInfo('andn', 'andn:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~ra & rb
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11000', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'andn:zric': InstructionInfo('andn', 'andn:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('log_set_cc')], '''	ZF <- ~ra & imm''', ['opCode8Or9', 'rbDisabled', 'subOpCode11000', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition', 'immediate28imm'], True, False, {}),
	'andn:zrici': InstructionInfo('andn', 'andn:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 12, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~ra & imm
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11000', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate28PcOpcode8', 'targetPc'], True, False, {}),
	'andn:zrif': InstructionInfo('andn', 'andn:zrif', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('false_cc')], '''	ZF <- ~ra & imm''', ['opCode8Or9', 'rbDisabled', 'subOpCode11000', 'rcDisabled', 'raAllEnabled', 'falseCondition4', 'immediate28imm'], True, False, {}),
	'andn:zrr': InstructionInfo('andn', 'andn:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- ~ra & rb''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11000', 'rcDisabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'andn:zrrc': InstructionInfo('andn', 'andn:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- ~ra & rb''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11000', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'andn:zrrci': InstructionInfo('andn', 'andn:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~ra & rb
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11000', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'asr.s:rri': InstructionInfo('asr.s', 'asr.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra >>a shift[0:4]
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB10000'], True, False, {}),
	'asr.s:rric': InstructionInfo('asr.s', 'asr.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra >>a shift[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB10000'], True, False, {}),
	'asr.s:rrici': InstructionInfo('asr.s', 'asr.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >>a shift[0:4]
	dc = x:S64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB10000', 'targetPc'], True, False, {}),
	'asr.s:rrr': InstructionInfo('asr.s', 'asr.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra >>a rb[0:4]
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA10000', 'falseConditionShift5'], True, False, {}),
	'asr.s:rrrc': InstructionInfo('asr.s', 'asr.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra >>a rb[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA10000', 'shiftSetCondition'], True, False, {}),
	'asr.s:rrrci': InstructionInfo('asr.s', 'asr.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >>a rb[0:4]
	dc = x:S64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA10000', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'asr.u:rri': InstructionInfo('asr.u', 'asr.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra >>a shift[0:4]
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB10000'], True, False, {}),
	'asr.u:rric': InstructionInfo('asr.u', 'asr.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra >>a shift[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB10000'], True, False, {}),
	'asr.u:rrici': InstructionInfo('asr.u', 'asr.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >>a shift[0:4]
	dc = x:U64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB10000', 'targetPc'], True, False, {}),
	'asr.u:rrr': InstructionInfo('asr.u', 'asr.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra >>a rb[0:4]
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA10000', 'falseConditionShift5'], True, False, {}),
	'asr.u:rrrc': InstructionInfo('asr.u', 'asr.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra >>a rb[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA10000', 'shiftSetCondition'], True, False, {}),
	'asr.u:rrrci': InstructionInfo('asr.u', 'asr.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >>a rb[0:4]
	dc = x:U64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA10000', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'asr:rri': InstructionInfo('asr', 'asr:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra >>a shift[0:4]
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB10000'], True, False, {}),
	'asr:rric': InstructionInfo('asr', 'asr:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra >>a shift[0:4]
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB10000'], True, False, {}),
	'asr:rrici': InstructionInfo('asr', 'asr:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >>a shift[0:4]
	rc = x
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB10000', 'targetPc'], True, False, {}),
	'asr:rrr': InstructionInfo('asr', 'asr:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra >>a rb[0:4]
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA10000', 'falseConditionShift5'], True, False, {}),
	'asr:rrrc': InstructionInfo('asr', 'asr:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra >>a rb[0:4]
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA10000', 'shiftSetCondition'], True, False, {}),
	'asr:rrrci': InstructionInfo('asr', 'asr:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >>a rb[0:4]
	rc = x
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA10000', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'asr:zri': InstructionInfo('asr', 'asr:zri', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra >>a shift[0:4]''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB10000'], True, False, {}),
	'asr:zric': InstructionInfo('asr', 'asr:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	ZF <- ra >>a shift[0:4]''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB10000'], True, False, {}),
	'asr:zrici': InstructionInfo('asr', 'asr:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >>a shift[0:4]
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB10000', 'targetPc'], True, False, {}),
	'asr:zrr': InstructionInfo('asr', 'asr:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- ra >>a rb[0:4]''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA10000', 'falseConditionShift5'], True, False, {}),
	'asr:zrrc': InstructionInfo('asr', 'asr:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- ra >>a rb[0:4]''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA10000', 'shiftSetCondition'], True, False, {}),
	'asr:zrrci': InstructionInfo('asr', 'asr:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >>a rb[0:4]
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA10000', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'bkp:': InstructionInfo('bkp', 'bkp:', [], '''	raise fault''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0011x', 'cstRc111000', 'raIsZero', 'falseCondition', 'syntacticSugar'], True, True, {}),
	'boot:ri': InstructionInfo('boot', 'boot:ri', [WorkRegister('ra', 32, True, False), Immediate('imm', 8, True)], '''	x = boot @[(ra + imm)[8:15] ^ (ra + imm)[0:7]]
	if (boot_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0011x', 'cstRc11x111', 'raAllEnabled', 'bootCondition', 'immediate24Pc', 'syntacticSugar'], True, True, {}),
	'boot:rici': InstructionInfo('boot', 'boot:rici', [WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('boot_cc'), PcSpec('pc', 16)], '''	x = boot @[(ra + imm)[8:15] ^ (ra + imm)[0:7]]
	if (boot_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0011x', 'cstRc11x111', 'raAllEnabled', 'bootCondition', 'immediate24Pc', 'targetPc'], True, False, {'boot:ri': (0x0, 0xf00ffff)}),
	'call:ri': InstructionInfo('call', 'call:ri', [WorkRegister('rc', 32, False, False), PcSpec('off', 24)], '''	rc = call @[ra + off]''', ['opCode8Or9', 'rbDisabled', 'subOpCode00000', 'rcNotExtendedEnabled', 'falseCondition', 'pcImmediate24', 'syntacticSugar'], True, True, {}),
	'call:rr': InstructionInfo('call', 'call:rr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False)], '''	rc = call @[ra + off]''', ['opCode8Or9', 'rbDisabled', 'subOpCode00000', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition', 'syntacticSugar'], True, True, {}),
	'call:rri': InstructionInfo('call', 'call:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), PcSpec('off', 24)], '''	rc = call @[ra + off]''', ['opCode8Or9', 'rbDisabled', 'subOpCode00000', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition', 'pcImmediate24'], True, False, {'call:rr': (0x0, 0xffffff), 'call:ri': (0x6000000000, 0x7c00000000)}),
	'call:rrr': InstructionInfo('call', 'call:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	rc = call @[ra + rb]''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA00000', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition'], True, False, {}),
	'call:zri': InstructionInfo('call', 'call:zri', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), PcSpec('off', 28)], '''	call @[ra + off]''', ['opCode8Or9', 'rbDisabled', 'subOpCode00000', 'rcDisabled', 'raAllEnabled', 'falseCondition', 'pcImmediate28'], True, False, {'jump:ri': (0, 0), 'jump:i': (0x6000000000, 0x7c00000000), 'jump:r': (0x0, 0x138000ffffff)}),
	'call:zrr': InstructionInfo('call', 'call:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	call @[ra + rb]''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA00000', 'rcDisabled', 'raAllEnabled', 'falseCondition'], True, False, {}),
	'cao.s:rr': InstructionInfo('cao.s', 'cao.s:rr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False)], '''	x = count one bits in ra
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0000', 'rcSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'cao.s:rrc': InstructionInfo('cao.s', 'cao.s:rrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = count one bits in ra
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0000', 'rcSignedExtendedEnabled', 'countSetCondition'], True, False, {}),
	'cao.s:rrci': InstructionInfo('cao.s', 'cao.s:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('count_nz_cc'), PcSpec('pc', 16)], '''	x = count one bits in ra
	dc = x:S64
	if (count_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0000', 'rcSignedExtendedEnabled', 'nzCountCondition', 'targetPc'], True, False, {}),
	'cao.u:rr': InstructionInfo('cao.u', 'cao.u:rr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False)], '''	x = count one bits in ra
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0000', 'rcUnsignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'cao.u:rrc': InstructionInfo('cao.u', 'cao.u:rrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = count one bits in ra
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0000', 'rcUnsignedExtendedEnabled', 'countSetCondition'], True, False, {}),
	'cao.u:rrci': InstructionInfo('cao.u', 'cao.u:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('count_nz_cc'), PcSpec('pc', 16)], '''	x = count one bits in ra
	dc = x:U64
	if (count_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0000', 'rcUnsignedExtendedEnabled', 'nzCountCondition', 'targetPc'], True, False, {}),
	'cao:rr': InstructionInfo('cao', 'cao:rr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False)], '''	x = count one bits in ra
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0000', 'rcNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'cao:rrc': InstructionInfo('cao', 'cao:rrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = count one bits in ra
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0000', 'rcNotExtendedEnabled', 'countSetCondition'], True, False, {}),
	'cao:rrci': InstructionInfo('cao', 'cao:rrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('count_nz_cc'), PcSpec('pc', 16)], '''	x = count one bits in ra
	rc = x
	if (count_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0000', 'rcNotExtendedEnabled', 'nzCountCondition', 'targetPc'], True, False, {}),
	'cao:zr': InstructionInfo('cao', 'cao:zr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False)], '''	ZF <- count one bits in ra''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0000', 'rcDisabled', 'falseCondition4'], True, False, {}),
	'cao:zrc': InstructionInfo('cao', 'cao:zrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	ZF <- count one bits in ra''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0000', 'rcDisabled', 'countSetCondition'], True, False, {}),
	'cao:zrci': InstructionInfo('cao', 'cao:zrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('count_nz_cc'), PcSpec('pc', 16)], '''	x = count one bits in ra
	if (count_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0000', 'rcDisabled', 'nzCountCondition', 'targetPc'], True, False, {}),
	'clo.s:rr': InstructionInfo('clo.s', 'clo.s:rr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False)], '''	x = count leading one bits of ra
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0001', 'rcSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'clo.s:rrc': InstructionInfo('clo.s', 'clo.s:rrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = count leading one bits of ra
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0001', 'rcSignedExtendedEnabled', 'countSetCondition'], True, False, {}),
	'clo.s:rrci': InstructionInfo('clo.s', 'clo.s:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('count_nz_cc'), PcSpec('pc', 16)], '''	x = count leading one bits of ra
	dc = x:S64
	if (count_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0001', 'rcSignedExtendedEnabled', 'nzCountCondition', 'targetPc'], True, False, {}),
	'clo.u:rr': InstructionInfo('clo.u', 'clo.u:rr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False)], '''	x = count leading one bits of ra
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0001', 'rcUnsignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'clo.u:rrc': InstructionInfo('clo.u', 'clo.u:rrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = count leading one bits of ra
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0001', 'rcUnsignedExtendedEnabled', 'countSetCondition'], True, False, {}),
	'clo.u:rrci': InstructionInfo('clo.u', 'clo.u:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('count_nz_cc'), PcSpec('pc', 16)], '''	x = count leading one bits of ra
	dc = x:U64
	if (count_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0001', 'rcUnsignedExtendedEnabled', 'nzCountCondition', 'targetPc'], True, False, {}),
	'clo:rr': InstructionInfo('clo', 'clo:rr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False)], '''	x = count leading one bits of ra
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0001', 'rcNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'clo:rrc': InstructionInfo('clo', 'clo:rrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = count leading one bits of ra
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0001', 'rcNotExtendedEnabled', 'countSetCondition'], True, False, {}),
	'clo:rrci': InstructionInfo('clo', 'clo:rrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('count_nz_cc'), PcSpec('pc', 16)], '''	x = count leading one bits of ra
	rc = x
	if (count_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0001', 'rcNotExtendedEnabled', 'nzCountCondition', 'targetPc'], True, False, {}),
	'clo:zr': InstructionInfo('clo', 'clo:zr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False)], '''	ZF <- count leading one bits of ra''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0001', 'rcDisabled', 'falseCondition4'], True, False, {}),
	'clo:zrc': InstructionInfo('clo', 'clo:zrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	ZF <- count leading one bits of ra''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0001', 'rcDisabled', 'countSetCondition'], True, False, {}),
	'clo:zrci': InstructionInfo('clo', 'clo:zrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('count_nz_cc'), PcSpec('pc', 16)], '''	x = count leading one bits of ra
	if (count_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0001', 'rcDisabled', 'nzCountCondition', 'targetPc'], True, False, {}),
	'clr_run:rici': InstructionInfo('clr_run', 'clr_run:rici', [WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('boot_cc'), PcSpec('pc', 16)], '''	x = clear RB((ra + imm)[8:15] ^ (ra + imm)[0:7])
	if (boot_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0011x', 'cstRc110001', 'raAllEnabled', 'bootCondition', 'immediate24Pc', 'targetPc'], False, False, {}),
	'cls.s:rr': InstructionInfo('cls.s', 'cls.s:rr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False)], '''	x = count leading sign bit of ra
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0110', 'rcSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'cls.s:rrc': InstructionInfo('cls.s', 'cls.s:rrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = count leading sign bit of ra
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0110', 'rcSignedExtendedEnabled', 'countSetCondition'], True, False, {}),
	'cls.s:rrci': InstructionInfo('cls.s', 'cls.s:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('count_nz_cc'), PcSpec('pc', 16)], '''	x = count leading sign bit of ra
	dc = x:S64
	if (count_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0110', 'rcSignedExtendedEnabled', 'nzCountCondition', 'targetPc'], True, False, {}),
	'cls.u:rr': InstructionInfo('cls.u', 'cls.u:rr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False)], '''	x = count leading sign bit of ra
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0110', 'rcUnsignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'cls.u:rrc': InstructionInfo('cls.u', 'cls.u:rrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = count leading sign bit of ra
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0110', 'rcUnsignedExtendedEnabled', 'countSetCondition'], True, False, {}),
	'cls.u:rrci': InstructionInfo('cls.u', 'cls.u:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('count_nz_cc'), PcSpec('pc', 16)], '''	x = count leading sign bit of ra
	dc = x:U64
	if (count_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0110', 'rcUnsignedExtendedEnabled', 'nzCountCondition', 'targetPc'], True, False, {}),
	'cls:rr': InstructionInfo('cls', 'cls:rr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False)], '''	x = count leading sign bit of ra
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0110', 'rcNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'cls:rrc': InstructionInfo('cls', 'cls:rrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = count leading sign bit of ra
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0110', 'rcNotExtendedEnabled', 'countSetCondition'], True, False, {}),
	'cls:rrci': InstructionInfo('cls', 'cls:rrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('count_nz_cc'), PcSpec('pc', 16)], '''	x = count leading sign bit of ra
	rc = x
	if (count_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0110', 'rcNotExtendedEnabled', 'nzCountCondition', 'targetPc'], True, False, {}),
	'cls:zr': InstructionInfo('cls', 'cls:zr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False)], '''	ZF <- count leading sign bit of ra''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0110', 'rcDisabled', 'falseCondition4'], True, False, {}),
	'cls:zrc': InstructionInfo('cls', 'cls:zrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	ZF <- count leading sign bit of ra''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0110', 'rcDisabled', 'countSetCondition'], True, False, {}),
	'cls:zrci': InstructionInfo('cls', 'cls:zrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('count_nz_cc'), PcSpec('pc', 16)], '''	x = count leading sign bit of ra
	if (count_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0110', 'rcDisabled', 'nzCountCondition', 'targetPc'], True, False, {}),
	'clz.s:rr': InstructionInfo('clz.s', 'clz.s:rr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False)], '''	x = count leading zero bits of ra
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0111', 'rcSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'clz.s:rrc': InstructionInfo('clz.s', 'clz.s:rrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = count leading zero bits of ra
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0111', 'rcSignedExtendedEnabled', 'countSetCondition'], True, False, {}),
	'clz.s:rrci': InstructionInfo('clz.s', 'clz.s:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('count_nz_cc'), PcSpec('pc', 16)], '''	x = count leading zero bits of ra
	dc = x:S64
	if (count_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0111', 'rcSignedExtendedEnabled', 'nzCountCondition', 'targetPc'], True, False, {}),
	'clz.u:rr': InstructionInfo('clz.u', 'clz.u:rr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False)], '''	x = count leading zero bits of ra
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0111', 'rcUnsignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'clz.u:rrc': InstructionInfo('clz.u', 'clz.u:rrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = count leading zero bits of ra
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0111', 'rcUnsignedExtendedEnabled', 'countSetCondition'], True, False, {}),
	'clz.u:rrci': InstructionInfo('clz.u', 'clz.u:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('count_nz_cc'), PcSpec('pc', 16)], '''	x = count leading zero bits of ra
	dc = x:U64
	if (count_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0111', 'rcUnsignedExtendedEnabled', 'nzCountCondition', 'targetPc'], True, False, {}),
	'clz:rr': InstructionInfo('clz', 'clz:rr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False)], '''	x = count leading zero bits of ra
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0111', 'rcNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'clz:rrc': InstructionInfo('clz', 'clz:rrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = count leading zero bits of ra
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0111', 'rcNotExtendedEnabled', 'countSetCondition'], True, False, {}),
	'clz:rrci': InstructionInfo('clz', 'clz:rrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('count_nz_cc'), PcSpec('pc', 16)], '''	x = count leading zero bits of ra
	rc = x
	if (count_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0111', 'rcNotExtendedEnabled', 'nzCountCondition', 'targetPc'], True, False, {}),
	'clz:zr': InstructionInfo('clz', 'clz:zr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False)], '''	ZF <- count leading zero bits of ra''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0111', 'rcDisabled', 'falseCondition4'], True, False, {}),
	'clz:zrc': InstructionInfo('clz', 'clz:zrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	ZF <- count leading zero bits of ra''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0111', 'rcDisabled', 'countSetCondition'], True, False, {}),
	'clz:zrci': InstructionInfo('clz', 'clz:zrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('count_nz_cc'), PcSpec('pc', 16)], '''	x = count leading zero bits of ra
	if (count_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx0111', 'rcDisabled', 'nzCountCondition', 'targetPc'], True, False, {}),
	'cmpb4.s:rrr': InstructionInfo('cmpb4.s', 'cmpb4.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = For i=[0..3] if byte ra[i] == byte rb[i] then byte i is set to 1, 0 otherwise
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode10xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'cmpb4.s:rrrc': InstructionInfo('cmpb4.s', 'cmpb4.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = For i=[0..3] if byte ra[i] == byte rb[i] then byte i is set to 1, 0 otherwise
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode10xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'cmpb4.s:rrrci': InstructionInfo('cmpb4.s', 'cmpb4.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = For i=[0..3] if byte ra[i] == byte rb[i] then byte i is set to 1, 0 otherwise
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode10xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'cmpb4.u:rrr': InstructionInfo('cmpb4.u', 'cmpb4.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = For i=[0..3] if byte ra[i] == byte rb[i] then byte i is set to 1, 0 otherwise
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode10xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'cmpb4.u:rrrc': InstructionInfo('cmpb4.u', 'cmpb4.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = For i=[0..3] if byte ra[i] == byte rb[i] then byte i is set to 1, 0 otherwise
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode10xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'cmpb4.u:rrrci': InstructionInfo('cmpb4.u', 'cmpb4.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = For i=[0..3] if byte ra[i] == byte rb[i] then byte i is set to 1, 0 otherwise
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode10xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'cmpb4:rrr': InstructionInfo('cmpb4', 'cmpb4:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = For i=[0..3] if byte ra[i] == byte rb[i] then byte i is set to 1, 0 otherwise
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode10xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'cmpb4:rrrc': InstructionInfo('cmpb4', 'cmpb4:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = For i=[0..3] if byte ra[i] == byte rb[i] then byte i is set to 1, 0 otherwise
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode10xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'cmpb4:rrrci': InstructionInfo('cmpb4', 'cmpb4:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = For i=[0..3] if byte ra[i] == byte rb[i] then byte i is set to 1, 0 otherwise
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode10xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'cmpb4:zrr': InstructionInfo('cmpb4', 'cmpb4:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- For i=[0..3] if byte ra[i] == byte rb[i] then byte i is set to 1, 0 otherwise''', ['opCode8Or9', 'rbEnabled', 'subOpCode10xxx', 'rcDisabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'cmpb4:zrrc': InstructionInfo('cmpb4', 'cmpb4:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- For i=[0..3] if byte ra[i] == byte rb[i] then byte i is set to 1, 0 otherwise''', ['opCode8Or9', 'rbEnabled', 'subOpCode10xxx', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'cmpb4:zrrci': InstructionInfo('cmpb4', 'cmpb4:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = For i=[0..3] if byte ra[i] == byte rb[i] then byte i is set to 1, 0 otherwise
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode10xxx', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'div_step:rrri': InstructionInfo('div_step', 'div_step:rrri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('db', 64, False, False), Immediate('shift', 5, False)], '''	cc = (dbo - (ra << shift))
	cc = if (const_cc_geu cc) then
		x = dbe << 1 padded with ones
		y = dbo - (ra << shift)
		dc = (x, y)
		dc
	else
		dce = (dbe << 1)
	if (div_cc cc) then
		jump @[pc]''', ['opCode8Or9', 'dbEnabled', 'subOpCode0111x', 'dcEnabled', 'divCondition', 'raAllEnabled', 'immediate5', 'syntacticSugar'], True, True, {}),
	'div_step:rrrici': InstructionInfo('div_step', 'div_step:rrrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('db', 64, False, False), Immediate('shift', 5, False), Condition('div_cc'), PcSpec('pc', 16)], '''	cc = (dbo - (ra << shift))
	cc = if (const_cc_geu cc) then
		x = dbe << 1 padded with ones
		y = dbo - (ra << shift)
		dc = (x, y)
		dc
	else
		dce = (dbe << 1)
	if (div_cc cc) then
		jump @[pc]''', ['opCode8Or9', 'dbEnabled', 'subOpCode0111x', 'dcEnabled', 'divCondition', 'raAllEnabled', 'immediate5', 'targetPc'], True, False, {'div_step:rrri': (0x0, 0xf00ffff)}),
	'extsb.s:rr': InstructionInfo('extsb.s', 'extsb.s:rr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False)], '''	x = ra[0:7]:S32
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1001', 'rcSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'extsb.s:rrc': InstructionInfo('extsb.s', 'extsb.s:rrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = ra[0:7]:S32
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1001', 'rcSignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'extsb.s:rrci': InstructionInfo('extsb.s', 'extsb.s:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra[0:7]:S32
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1001', 'rcSignedExtendedEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'extsb:rr': InstructionInfo('extsb', 'extsb:rr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False)], '''	x = ra[0:7]:S32
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1001', 'rcNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'extsb:rrc': InstructionInfo('extsb', 'extsb:rrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = ra[0:7]:S32
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1001', 'rcNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'extsb:rrci': InstructionInfo('extsb', 'extsb:rrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra[0:7]:S32
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1001', 'rcNotExtendedEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'extsb:zr': InstructionInfo('extsb', 'extsb:zr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False)], '''	ZF <- ra[0:7]:S32''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1001', 'rcDisabled', 'falseCondition4'], True, False, {}),
	'extsb:zrc': InstructionInfo('extsb', 'extsb:zrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	ZF <- ra[0:7]:S32''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1001', 'rcDisabled', 'logicalSetCondition'], True, False, {}),
	'extsb:zrci': InstructionInfo('extsb', 'extsb:zrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra[0:7]:S32
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1001', 'rcDisabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'extsh.s:rr': InstructionInfo('extsh.s', 'extsh.s:rr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False)], '''	x = ra[0:15]:S32
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1111', 'rcSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'extsh.s:rrc': InstructionInfo('extsh.s', 'extsh.s:rrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = ra[0:15]:S32
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1111', 'rcSignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'extsh.s:rrci': InstructionInfo('extsh.s', 'extsh.s:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra[0:15]:S32
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1111', 'rcSignedExtendedEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'extsh:rr': InstructionInfo('extsh', 'extsh:rr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False)], '''	x = ra[0:15]:S32
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1111', 'rcNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'extsh:rrc': InstructionInfo('extsh', 'extsh:rrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = ra[0:15]:S32
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1111', 'rcNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'extsh:rrci': InstructionInfo('extsh', 'extsh:rrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra[0:15]:S32
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1111', 'rcNotExtendedEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'extsh:zr': InstructionInfo('extsh', 'extsh:zr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False)], '''	ZF <- ra[0:15]:S32''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1111', 'rcDisabled', 'falseCondition4'], True, False, {}),
	'extsh:zrc': InstructionInfo('extsh', 'extsh:zrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	ZF <- ra[0:15]:S32''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1111', 'rcDisabled', 'logicalSetCondition'], True, False, {}),
	'extsh:zrci': InstructionInfo('extsh', 'extsh:zrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra[0:15]:S32
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1111', 'rcDisabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'extub.u:rr': InstructionInfo('extub.u', 'extub.u:rr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False)], '''	x = ra[0:7]:U32
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1000', 'rcUnsignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'extub.u:rrc': InstructionInfo('extub.u', 'extub.u:rrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = ra[0:7]:U32
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1000', 'rcUnsignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'extub.u:rrci': InstructionInfo('extub.u', 'extub.u:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra[0:7]:U32
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1000', 'rcUnsignedExtendedEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'extub:rr': InstructionInfo('extub', 'extub:rr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False)], '''	x = ra[0:7]:U32
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1000', 'rcNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'extub:rrc': InstructionInfo('extub', 'extub:rrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = ra[0:7]:U32
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1000', 'rcNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'extub:rrci': InstructionInfo('extub', 'extub:rrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra[0:7]:U32
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1000', 'rcNotExtendedEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'extub:zr': InstructionInfo('extub', 'extub:zr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False)], '''	ZF <- ra[0:7]:U32''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1000', 'rcDisabled', 'falseCondition4'], True, False, {}),
	'extub:zrc': InstructionInfo('extub', 'extub:zrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	ZF <- ra[0:7]:U32''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1000', 'rcDisabled', 'logicalSetCondition'], True, False, {}),
	'extub:zrci': InstructionInfo('extub', 'extub:zrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra[0:7]:U32
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1000', 'rcDisabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'extuh.u:rr': InstructionInfo('extuh.u', 'extuh.u:rr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False)], '''	x = ra[0:15]:U32
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1110', 'rcUnsignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'extuh.u:rrc': InstructionInfo('extuh.u', 'extuh.u:rrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = ra[0:15]:U32
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1110', 'rcUnsignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'extuh.u:rrci': InstructionInfo('extuh.u', 'extuh.u:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra[0:15]:U32
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1110', 'rcUnsignedExtendedEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'extuh:rr': InstructionInfo('extuh', 'extuh:rr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False)], '''	x = ra[0:15]:U32
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1110', 'rcNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'extuh:rrc': InstructionInfo('extuh', 'extuh:rrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = ra[0:15]:U32
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1110', 'rcNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'extuh:rrci': InstructionInfo('extuh', 'extuh:rrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra[0:15]:U32
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1110', 'rcNotExtendedEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'extuh:zr': InstructionInfo('extuh', 'extuh:zr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False)], '''	ZF <- ra[0:15]:U32''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1110', 'rcDisabled', 'falseCondition4'], True, False, {}),
	'extuh:zrc': InstructionInfo('extuh', 'extuh:zrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	ZF <- ra[0:15]:U32''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1110', 'rcDisabled', 'logicalSetCondition'], True, False, {}),
	'extuh:zrci': InstructionInfo('extuh', 'extuh:zrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra[0:15]:U32
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00111', 'raAllEnabled', 'subAx1110', 'rcDisabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'fault:i': InstructionInfo('fault', 'fault:i', [Immediate('imm', 24, True)], '''	raise fault''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0011x', 'cstRc111000', 'raIsZero', 'falseCondition', 'immediate24imm'], True, False, {'bkp:': (0x0, 0xffffff)}),
	'hash.s:rric': InstructionInfo('hash.s', 'hash.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = hash(ra,imm[0:3])
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'hash.s:rrici': InstructionInfo('hash.s', 'hash.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = hash(ra,imm[0:3])
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'hash.s:rrif': InstructionInfo('hash.s', 'hash.s:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = hash(ra,imm[0:3])
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'hash.s:rrr': InstructionInfo('hash.s', 'hash.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = hash(ra,rb[0:3])
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA00001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'hash.s:rrrc': InstructionInfo('hash.s', 'hash.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = hash(ra,rb[0:3])
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA00001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'hash.s:rrrci': InstructionInfo('hash.s', 'hash.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = hash(ra,rb[0:3])
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA00001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'hash.u:rric': InstructionInfo('hash.u', 'hash.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = hash(ra,imm[0:3])
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'hash.u:rrici': InstructionInfo('hash.u', 'hash.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = hash(ra,imm[0:3])
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'hash.u:rrif': InstructionInfo('hash.u', 'hash.u:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = hash(ra,imm[0:3])
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'hash.u:rrr': InstructionInfo('hash.u', 'hash.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = hash(ra,rb[0:3])
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA00001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'hash.u:rrrc': InstructionInfo('hash.u', 'hash.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = hash(ra,rb[0:3])
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA00001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'hash.u:rrrci': InstructionInfo('hash.u', 'hash.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = hash(ra,rb[0:3])
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA00001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'hash:rri': InstructionInfo('hash', 'hash:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True)], '''	x = hash(ra,imm[0:3])
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00001', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm', 'syntacticSugar'], True, True, {}),
	'hash:rric': InstructionInfo('hash', 'hash:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = hash(ra,imm[0:3])
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00001', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'hash:rrici': InstructionInfo('hash', 'hash:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = hash(ra,imm[0:3])
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00001', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'hash:rrif': InstructionInfo('hash', 'hash:rrif', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = hash(ra,imm[0:3])
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00001', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {'hash:rri': (0, 0)}),
	'hash:rrr': InstructionInfo('hash', 'hash:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = hash(ra,rb[0:3])
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA00001', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'hash:rrrc': InstructionInfo('hash', 'hash:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = hash(ra,rb[0:3])
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA00001', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'hash:rrrci': InstructionInfo('hash', 'hash:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = hash(ra,rb[0:3])
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA00001', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'hash:zric': InstructionInfo('hash', 'hash:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('log_set_cc')], '''	ZF <- hash(ra,imm[0:3])''', ['opCode8Or9', 'rbDisabled', 'subOpCode00001', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition', 'immediate28imm'], True, False, {}),
	'hash:zrici': InstructionInfo('hash', 'hash:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 12, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = hash(ra,imm[0:3])
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00001', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate28PcOpcode8', 'targetPc'], True, False, {}),
	'hash:zrif': InstructionInfo('hash', 'hash:zrif', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('false_cc')], '''	ZF <- hash(ra,imm[0:3])''', ['opCode8Or9', 'rbDisabled', 'subOpCode00001', 'rcDisabled', 'raAllEnabled', 'falseCondition4', 'immediate28imm'], True, False, {}),
	'hash:zrr': InstructionInfo('hash', 'hash:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- hash(ra,rb[0:3])''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA00001', 'rcDisabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'hash:zrrc': InstructionInfo('hash', 'hash:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- hash(ra,rb[0:3])''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA00001', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'hash:zrrci': InstructionInfo('hash', 'hash:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = hash(ra,rb[0:3])
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA00001', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'jeq:rii': InstructionInfo('jeq', 'jeq:rii', [WorkRegister('ra', 32, True, False), Immediate('imm', 11, True), PcSpec('pc', 16)], '''	x = ra - imm
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate27Pc', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jeq:rri': InstructionInfo('jeq', 'jeq:rri', [WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), PcSpec('pc', 16)], '''	x = ra - rb
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jges:rii': InstructionInfo('jges', 'jges:rii', [WorkRegister('ra', 32, True, False), Immediate('imm', 11, True), PcSpec('pc', 16)], '''	x = ra - imm
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate27Pc', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jges:rri': InstructionInfo('jges', 'jges:rri', [WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), PcSpec('pc', 16)], '''	x = ra - rb
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jgeu:rii': InstructionInfo('jgeu', 'jgeu:rii', [WorkRegister('ra', 32, True, False), Immediate('imm', 11, True), PcSpec('pc', 16)], '''	x = ra - imm
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate27Pc', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jgeu:rri': InstructionInfo('jgeu', 'jgeu:rri', [WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), PcSpec('pc', 16)], '''	x = ra - rb
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jgts:rii': InstructionInfo('jgts', 'jgts:rii', [WorkRegister('ra', 32, True, False), Immediate('imm', 11, True), PcSpec('pc', 16)], '''	x = ra - imm
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate27Pc', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jgts:rri': InstructionInfo('jgts', 'jgts:rri', [WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), PcSpec('pc', 16)], '''	x = ra - rb
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jgtu:rii': InstructionInfo('jgtu', 'jgtu:rii', [WorkRegister('ra', 32, True, False), Immediate('imm', 11, True), PcSpec('pc', 16)], '''	x = ra - imm
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate27Pc', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jgtu:rri': InstructionInfo('jgtu', 'jgtu:rri', [WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), PcSpec('pc', 16)], '''	x = ra - rb
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jles:rii': InstructionInfo('jles', 'jles:rii', [WorkRegister('ra', 32, True, False), Immediate('imm', 11, True), PcSpec('pc', 16)], '''	x = ra - imm
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate27Pc', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jles:rri': InstructionInfo('jles', 'jles:rri', [WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), PcSpec('pc', 16)], '''	x = ra - rb
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jleu:rii': InstructionInfo('jleu', 'jleu:rii', [WorkRegister('ra', 32, True, False), Immediate('imm', 11, True), PcSpec('pc', 16)], '''	x = ra - imm
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate27Pc', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jleu:rri': InstructionInfo('jleu', 'jleu:rri', [WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), PcSpec('pc', 16)], '''	x = ra - rb
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jlts:rii': InstructionInfo('jlts', 'jlts:rii', [WorkRegister('ra', 32, True, False), Immediate('imm', 11, True), PcSpec('pc', 16)], '''	x = ra - imm
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate27Pc', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jlts:rri': InstructionInfo('jlts', 'jlts:rri', [WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), PcSpec('pc', 16)], '''	x = ra - rb
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jltu:rii': InstructionInfo('jltu', 'jltu:rii', [WorkRegister('ra', 32, True, False), Immediate('imm', 11, True), PcSpec('pc', 16)], '''	x = ra - imm
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate27Pc', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jltu:rri': InstructionInfo('jltu', 'jltu:rri', [WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), PcSpec('pc', 16)], '''	x = ra - rb
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jneq:rii': InstructionInfo('jneq', 'jneq:rii', [WorkRegister('ra', 32, True, False), Immediate('imm', 11, True), PcSpec('pc', 16)], '''	x = ra - imm
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate27Pc', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jneq:rri': InstructionInfo('jneq', 'jneq:rri', [WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), PcSpec('pc', 16)], '''	x = ra - rb
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jnz:ri': InstructionInfo('jnz', 'jnz:ri', [WorkRegister('ra', 32, True, False), PcSpec('pc', 16)], '''	x = ra - imm
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'nzSubBoolCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'jump:i': InstructionInfo('jump', 'jump:i', [PcSpec('off', 28)], '''	call @[ra + off]''', ['opCode8Or9', 'rbDisabled', 'subOpCode00000', 'rcDisabled', 'falseCondition', 'pcImmediate28', 'syntacticSugar'], True, True, {}),
	'jump:r': InstructionInfo('jump', 'jump:r', [WorkRegister('ra', 32, True, False)], '''	call @[ra + off]''', ['opCode8Or9', 'rbDisabled', 'subOpCode00000', 'rcDisabled', 'raAllEnabled', 'falseCondition', 'syntacticSugar'], True, True, {}),
	'jump:ri': InstructionInfo('jump', 'jump:ri', [WorkRegister('ra', 32, True, False), PcSpec('off', 28)], '''	call @[ra + off]''', ['opCode8Or9', 'rbDisabled', 'subOpCode00000', 'rcDisabled', 'raAllEnabled', 'falseCondition', 'pcImmediate28', 'syntacticSugar'], True, True, {}),
	'jz:ri': InstructionInfo('jz', 'jz:ri', [WorkRegister('ra', 32, True, False), PcSpec('pc', 16)], '''	x = ra - imm
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'nzSubBoolCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'lbs.s:erri': InstructionInfo('lbs.s', 'lbs.s:erri', [Endianess('endian'), WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	dc = (Load 1 byte from WRAM at address @a with endianness endian):S64''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx0001', 'rcSignedExtendedLoadArg'], True, False, {'lbs.s:rri': (0x0, 0x8000000)}),
	'lbs.s:rri': InstructionInfo('lbs.s', 'lbs.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	dc = (Load 1 byte from WRAM at address @a with endianness endian):S64''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx0001', 'rcSignedExtendedLoadArg', 'syntacticSugar'], True, True, {}),
	'lbs:erri': InstructionInfo('lbs', 'lbs:erri', [Endianess('endian'), WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	rc = (Load 1 byte from WRAM at address @a with endianness endian):S32''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx0001', 'rcNotExtendedLoadArg'], True, False, {'lbs:rri': (0x0, 0x8000000)}),
	'lbs:ersi': InstructionInfo('lbs', 'lbs:ersi', [Endianess('endian'), WorkRegister('rc', 32, False, False), WorkRegister('sa', 32, True, True), Immediate('off', 24, True)], '''	cc = (ra & 0xffff) + off - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xffff
		rc = (Load 1 byte from WRAM at address @a with endianness endian):S32
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		rc = (Load 1 byte from WRAM at address @a with endianness endian):S32''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreSafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx0001', 'rcNotExtendedLoadArg'], True, False, {'lbss:rri': (0x0, 0x8000000)}),
	'lbs:rri': InstructionInfo('lbs', 'lbs:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	rc = (Load 1 byte from WRAM at address @a with endianness endian):S32''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx0001', 'rcNotExtendedLoadArg', 'syntacticSugar'], True, True, {}),
	'lbss:rri': InstructionInfo('lbss', 'lbss:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	cc = (ra & 0xffff) + off - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xffff
		rc = (Load 1 byte from WRAM at address @a with endianness endian):S32
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		rc = (Load 1 byte from WRAM at address @a with endianness endian):S32''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreSafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx0001', 'rcNotExtendedLoadArg', 'syntacticSugar'], True, True, {}),
	'lbu.u:erri': InstructionInfo('lbu.u', 'lbu.u:erri', [Endianess('endian'), WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	dc = (Load 1 byte from WRAM at address @a with endianness endian):U64''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx0000', 'rcUnsignedExtendedLoadArg'], True, False, {'lbu.u:rri': (0x0, 0x8000000)}),
	'lbu.u:rri': InstructionInfo('lbu.u', 'lbu.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	dc = (Load 1 byte from WRAM at address @a with endianness endian):U64''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx0000', 'rcUnsignedExtendedLoadArg', 'syntacticSugar'], True, True, {}),
	'lbu:erri': InstructionInfo('lbu', 'lbu:erri', [Endianess('endian'), WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	rc = (Load 1 byte from WRAM at address @a with endianness endian):U32''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx0000', 'rcNotExtendedLoadArg'], True, False, {'lbu:rri': (0x0, 0x8000000)}),
	'lbu:ersi': InstructionInfo('lbu', 'lbu:ersi', [Endianess('endian'), WorkRegister('rc', 32, False, False), WorkRegister('sa', 32, True, True), Immediate('off', 24, True)], '''	cc = (ra & 0xffff) + off - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xffff
		rc = (Load 1 byte from WRAM at address @a with endianness endian):U32
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		rc = (Load 1 byte from WRAM at address @a with endianness endian):U32''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreSafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx0000', 'rcNotExtendedLoadArg'], True, False, {'lbus:rri': (0x0, 0x8000000)}),
	'lbu:rri': InstructionInfo('lbu', 'lbu:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	rc = (Load 1 byte from WRAM at address @a with endianness endian):U32''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx0000', 'rcNotExtendedLoadArg', 'syntacticSugar'], True, True, {}),
	'lbus:rri': InstructionInfo('lbus', 'lbus:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	cc = (ra & 0xffff) + off - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xffff
		rc = (Load 1 byte from WRAM at address @a with endianness endian):U32
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		rc = (Load 1 byte from WRAM at address @a with endianness endian):U32''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreSafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx0000', 'rcNotExtendedLoadArg', 'syntacticSugar'], True, True, {}),
	'ld:erri': InstructionInfo('ld', 'ld:erri', [Endianess('endian'), WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	dc = (Load 8 bytes from WRAM at address @a with endianness endian)''', ['opCode7', 'rbDisabled', 'rcExists', 'dcLoad', 'subOpCode0100x', 'loadStoreUnsafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx111x', 'dcArg'], True, False, {'ld:rri': (0x0, 0x8000000)}),
	'ld:ersi': InstructionInfo('ld', 'ld:ersi', [Endianess('endian'), WorkRegister('dc', 64, False, False), WorkRegister('sa', 32, True, True), Immediate('off', 24, True)], '''	cc = (ra & 0xffff) + off + 8 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfff8
		dc = (Load 8 bytes from WRAM at address @a with endianness endian)
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		dc = (Load 8 bytes from WRAM at address @a with endianness endian)''', ['opCode7', 'rbDisabled', 'rcExists', 'dcLoad', 'subOpCode0100x', 'loadStoreSafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx111x', 'dcArg'], True, False, {'lds:rri': (0x0, 0x8000000)}),
	'ld:rri': InstructionInfo('ld', 'ld:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	dc = (Load 8 bytes from WRAM at address @a with endianness endian)''', ['opCode7', 'rbDisabled', 'rcExists', 'dcLoad', 'subOpCode0100x', 'loadStoreUnsafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx111x', 'dcArg', 'syntacticSugar'], True, True, {}),
	'ldma:rri': InstructionInfo('ldma', 'ldma:rri', [WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Immediate('immDma', 8, False)], '''	let @w = (ra & 0xfffff8)
	let @m = (rb & 0xfffffff8)
	let N = (1 + (immDma:U32 + (ra >> 24) & 0xff) & 0xff) << 3
	Load N bytes from MRAM at address @m into WRAM at address @w''', ['opCode7', 'rbOrDbExists', 'cstRc000000', 'rbArg', 'raAllEnabled', 'immediate8Dma', 'lsb16_0000000000000000'], True, False, {}),
	'ldmai:rri': InstructionInfo('ldmai', 'ldmai:rri', [WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Immediate('immDma', 8, False)], '''	let @i = (ra & 0xfffff8)
	let @m = (rb & 0xfffffff8)
	let N = (1 + (immDma:U32 + (ra >> 24) & 0xff) & 0xff) << 3
	Load N bytes from MRAM at address @m into IRAM at address @w''', ['opCode7', 'rbOrDbExists', 'cstRc000000', 'rbArg', 'raAllEnabled', 'immediate8Dma', 'lsb16_0000000000000001'], True, False, {}),
	'lds:rri': InstructionInfo('lds', 'lds:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	cc = (ra & 0xffff) + off + 8 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfff8
		dc = (Load 8 bytes from WRAM at address @a with endianness endian)
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		dc = (Load 8 bytes from WRAM at address @a with endianness endian)''', ['opCode7', 'rbDisabled', 'rcExists', 'dcLoad', 'subOpCode0100x', 'loadStoreSafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx111x', 'dcArg', 'syntacticSugar'], True, True, {}),
	'lhs.s:erri': InstructionInfo('lhs.s', 'lhs.s:erri', [Endianess('endian'), WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	dc = (Load 2 bytes from WRAM at address @a with endianness endian):S64''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx0111', 'rcSignedExtendedLoadArg'], True, False, {'lhs.s:rri': (0x0, 0x8000000)}),
	'lhs.s:rri': InstructionInfo('lhs.s', 'lhs.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	dc = (Load 2 bytes from WRAM at address @a with endianness endian):S64''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx0111', 'rcSignedExtendedLoadArg', 'syntacticSugar'], True, True, {}),
	'lhs:erri': InstructionInfo('lhs', 'lhs:erri', [Endianess('endian'), WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	rc = (Load 2 bytes from WRAM at address @a with endianness endian):S32''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx0111', 'rcNotExtendedLoadArg'], True, False, {'lhs:rri': (0x0, 0x8000000)}),
	'lhs:ersi': InstructionInfo('lhs', 'lhs:ersi', [Endianess('endian'), WorkRegister('rc', 32, False, False), WorkRegister('sa', 32, True, True), Immediate('off', 24, True)], '''	cc = (ra & 0xffff) + off + 2 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfffe
		rc = (Load 2 bytes from WRAM at address @a with endianness endian):S32
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		rc = (Load 2 bytes from WRAM at address @a with endianness endian):S32''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreSafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx0111', 'rcNotExtendedLoadArg'], True, False, {'lhss:rri': (0x0, 0x8000000)}),
	'lhs:rri': InstructionInfo('lhs', 'lhs:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	rc = (Load 2 bytes from WRAM at address @a with endianness endian):S32''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx0111', 'rcNotExtendedLoadArg', 'syntacticSugar'], True, True, {}),
	'lhss:rri': InstructionInfo('lhss', 'lhss:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	cc = (ra & 0xffff) + off + 2 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfffe
		rc = (Load 2 bytes from WRAM at address @a with endianness endian):S32
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		rc = (Load 2 bytes from WRAM at address @a with endianness endian):S32''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreSafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx0111', 'rcNotExtendedLoadArg', 'syntacticSugar'], True, True, {}),
	'lhu.u:erri': InstructionInfo('lhu.u', 'lhu.u:erri', [Endianess('endian'), WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	dc = (Load 2 bytes from WRAM at address @a with endianness endian):U64''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx0110', 'rcUnsignedExtendedLoadArg'], True, False, {'lhu.u:rri': (0x0, 0x8000000)}),
	'lhu.u:rri': InstructionInfo('lhu.u', 'lhu.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	dc = (Load 2 bytes from WRAM at address @a with endianness endian):U64''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx0110', 'rcUnsignedExtendedLoadArg', 'syntacticSugar'], True, True, {}),
	'lhu:erri': InstructionInfo('lhu', 'lhu:erri', [Endianess('endian'), WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	rc = (Load 2 bytes from WRAM at address @a with endianness endian):U32''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx0110', 'rcNotExtendedLoadArg'], True, False, {'lhu:rri': (0x0, 0x8000000)}),
	'lhu:ersi': InstructionInfo('lhu', 'lhu:ersi', [Endianess('endian'), WorkRegister('rc', 32, False, False), WorkRegister('sa', 32, True, True), Immediate('off', 24, True)], '''	cc = (ra & 0xffff) + off + 2 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfffe
		rc = (Load 2 bytes from WRAM at address @a with endianness endian):U32
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		rc = (Load 2 bytes from WRAM at address @a with endianness endian):U32''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreSafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx0110', 'rcNotExtendedLoadArg'], True, False, {'lhus:rri': (0x0, 0x8000000)}),
	'lhu:rri': InstructionInfo('lhu', 'lhu:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	rc = (Load 2 bytes from WRAM at address @a with endianness endian):U32''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx0110', 'rcNotExtendedLoadArg', 'syntacticSugar'], True, True, {}),
	'lhus:rri': InstructionInfo('lhus', 'lhus:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	cc = (ra & 0xffff) + off + 2 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfffe
		rc = (Load 2 bytes from WRAM at address @a with endianness endian):U32
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		rc = (Load 2 bytes from WRAM at address @a with endianness endian):U32''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreSafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx0110', 'rcNotExtendedLoadArg', 'syntacticSugar'], True, True, {}),
	'lsl.s:rri': InstructionInfo('lsl.s', 'lsl.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra << shift[0:4]
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB01000'], True, False, {}),
	'lsl.s:rric': InstructionInfo('lsl.s', 'lsl.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra << shift[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB01000'], True, False, {}),
	'lsl.s:rrici': InstructionInfo('lsl.s', 'lsl.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra << shift[0:4]
	dc = x:S64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB01000', 'targetPc'], True, False, {}),
	'lsl.s:rrr': InstructionInfo('lsl.s', 'lsl.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra << rb[0:4]
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA01000', 'falseConditionShift5'], True, False, {}),
	'lsl.s:rrrc': InstructionInfo('lsl.s', 'lsl.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra << rb[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA01000', 'shiftSetCondition'], True, False, {}),
	'lsl.s:rrrci': InstructionInfo('lsl.s', 'lsl.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra << rb[0:4]
	dc = x:S64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA01000', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsl.u:rri': InstructionInfo('lsl.u', 'lsl.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra << shift[0:4]
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB01000'], True, False, {}),
	'lsl.u:rric': InstructionInfo('lsl.u', 'lsl.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra << shift[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB01000'], True, False, {}),
	'lsl.u:rrici': InstructionInfo('lsl.u', 'lsl.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra << shift[0:4]
	dc = x:U64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB01000', 'targetPc'], True, False, {}),
	'lsl.u:rrr': InstructionInfo('lsl.u', 'lsl.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra << rb[0:4]
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA01000', 'falseConditionShift5'], True, False, {}),
	'lsl.u:rrrc': InstructionInfo('lsl.u', 'lsl.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra << rb[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA01000', 'shiftSetCondition'], True, False, {}),
	'lsl.u:rrrci': InstructionInfo('lsl.u', 'lsl.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra << rb[0:4]
	dc = x:U64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA01000', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsl1.s:rri': InstructionInfo('lsl1.s', 'lsl1.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra << shift[0:4] padded with ones
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB01110'], True, False, {}),
	'lsl1.s:rric': InstructionInfo('lsl1.s', 'lsl1.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra << shift[0:4] padded with ones
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB01110'], True, False, {}),
	'lsl1.s:rrici': InstructionInfo('lsl1.s', 'lsl1.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra << shift[0:4] padded with ones
	dc = x:S64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB01110', 'targetPc'], True, False, {}),
	'lsl1.s:rrr': InstructionInfo('lsl1.s', 'lsl1.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra << rb[0:4] padded with ones
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA01110', 'falseConditionShift5'], True, False, {}),
	'lsl1.s:rrrc': InstructionInfo('lsl1.s', 'lsl1.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra << rb[0:4] padded with ones
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA01110', 'shiftSetCondition'], True, False, {}),
	'lsl1.s:rrrci': InstructionInfo('lsl1.s', 'lsl1.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra << rb[0:4] padded with ones
	dc = x:S64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA01110', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsl1.u:rri': InstructionInfo('lsl1.u', 'lsl1.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra << shift[0:4] padded with ones
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB01110'], True, False, {}),
	'lsl1.u:rric': InstructionInfo('lsl1.u', 'lsl1.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra << shift[0:4] padded with ones
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB01110'], True, False, {}),
	'lsl1.u:rrici': InstructionInfo('lsl1.u', 'lsl1.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra << shift[0:4] padded with ones
	dc = x:U64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB01110', 'targetPc'], True, False, {}),
	'lsl1.u:rrr': InstructionInfo('lsl1.u', 'lsl1.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra << rb[0:4] padded with ones
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA01110', 'falseConditionShift5'], True, False, {}),
	'lsl1.u:rrrc': InstructionInfo('lsl1.u', 'lsl1.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra << rb[0:4] padded with ones
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA01110', 'shiftSetCondition'], True, False, {}),
	'lsl1.u:rrrci': InstructionInfo('lsl1.u', 'lsl1.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra << rb[0:4] padded with ones
	dc = x:U64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA01110', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsl1:rri': InstructionInfo('lsl1', 'lsl1:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra << shift[0:4] padded with ones
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB01110'], True, False, {}),
	'lsl1:rric': InstructionInfo('lsl1', 'lsl1:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra << shift[0:4] padded with ones
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB01110'], True, False, {}),
	'lsl1:rrici': InstructionInfo('lsl1', 'lsl1:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra << shift[0:4] padded with ones
	rc = x
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB01110', 'targetPc'], True, False, {}),
	'lsl1:rrr': InstructionInfo('lsl1', 'lsl1:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra << rb[0:4] padded with ones
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA01110', 'falseConditionShift5'], True, False, {}),
	'lsl1:rrrc': InstructionInfo('lsl1', 'lsl1:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra << rb[0:4] padded with ones
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA01110', 'shiftSetCondition'], True, False, {}),
	'lsl1:rrrci': InstructionInfo('lsl1', 'lsl1:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra << rb[0:4] padded with ones
	rc = x
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA01110', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsl1:zri': InstructionInfo('lsl1', 'lsl1:zri', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra << shift[0:4] padded with ones''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB01110'], True, False, {}),
	'lsl1:zric': InstructionInfo('lsl1', 'lsl1:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	ZF <- ra << shift[0:4] padded with ones''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB01110'], True, False, {}),
	'lsl1:zrici': InstructionInfo('lsl1', 'lsl1:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra << shift[0:4] padded with ones
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB01110', 'targetPc'], True, False, {}),
	'lsl1:zrr': InstructionInfo('lsl1', 'lsl1:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- ra << rb[0:4] padded with ones''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA01110', 'falseConditionShift5'], True, False, {}),
	'lsl1:zrrc': InstructionInfo('lsl1', 'lsl1:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- ra << rb[0:4] padded with ones''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA01110', 'shiftSetCondition'], True, False, {}),
	'lsl1:zrrci': InstructionInfo('lsl1', 'lsl1:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra << rb[0:4] padded with ones
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA01110', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsl1x.s:rri': InstructionInfo('lsl1x.s', 'lsl1x.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - shift[0:4]) padded with ones
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB01111'], True, False, {}),
	'lsl1x.s:rric': InstructionInfo('lsl1x.s', 'lsl1x.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - shift[0:4]) padded with ones
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB01111'], True, False, {}),
	'lsl1x.s:rrici': InstructionInfo('lsl1x.s', 'lsl1x.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - shift[0:4]) padded with ones
	dc = x:S64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB01111', 'targetPc'], True, False, {}),
	'lsl1x.s:rrr': InstructionInfo('lsl1x.s', 'lsl1x.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - rb[0:4]) padded with ones
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA01111', 'falseConditionShift5'], True, False, {}),
	'lsl1x.s:rrrc': InstructionInfo('lsl1x.s', 'lsl1x.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - rb[0:4]) padded with ones
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA01111', 'shiftSetCondition'], True, False, {}),
	'lsl1x.s:rrrci': InstructionInfo('lsl1x.s', 'lsl1x.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - rb[0:4]) padded with ones
	dc = x:S64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA01111', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsl1x.u:rri': InstructionInfo('lsl1x.u', 'lsl1x.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - shift[0:4]) padded with ones
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB01111'], True, False, {}),
	'lsl1x.u:rric': InstructionInfo('lsl1x.u', 'lsl1x.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - shift[0:4]) padded with ones
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB01111'], True, False, {}),
	'lsl1x.u:rrici': InstructionInfo('lsl1x.u', 'lsl1x.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - shift[0:4]) padded with ones
	dc = x:U64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB01111', 'targetPc'], True, False, {}),
	'lsl1x.u:rrr': InstructionInfo('lsl1x.u', 'lsl1x.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - rb[0:4]) padded with ones
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA01111', 'falseConditionShift5'], True, False, {}),
	'lsl1x.u:rrrc': InstructionInfo('lsl1x.u', 'lsl1x.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - rb[0:4]) padded with ones
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA01111', 'shiftSetCondition'], True, False, {}),
	'lsl1x.u:rrrci': InstructionInfo('lsl1x.u', 'lsl1x.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - rb[0:4]) padded with ones
	dc = x:U64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA01111', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsl1x:rri': InstructionInfo('lsl1x', 'lsl1x:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - shift[0:4]) padded with ones
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB01111'], True, False, {}),
	'lsl1x:rric': InstructionInfo('lsl1x', 'lsl1x:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - shift[0:4]) padded with ones
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB01111'], True, False, {}),
	'lsl1x:rrici': InstructionInfo('lsl1x', 'lsl1x:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - shift[0:4]) padded with ones
	rc = x
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB01111', 'targetPc'], True, False, {}),
	'lsl1x:rrr': InstructionInfo('lsl1x', 'lsl1x:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - rb[0:4]) padded with ones
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA01111', 'falseConditionShift5'], True, False, {}),
	'lsl1x:rrrc': InstructionInfo('lsl1x', 'lsl1x:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - rb[0:4]) padded with ones
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA01111', 'shiftSetCondition'], True, False, {}),
	'lsl1x:rrrci': InstructionInfo('lsl1x', 'lsl1x:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - rb[0:4]) padded with ones
	rc = x
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA01111', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsl1x:zri': InstructionInfo('lsl1x', 'lsl1x:zri', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	cc = shift[0:4]
	if (const_cc_zero cc) then
		-1
	else
	ZF <- 	(ra >> (32 - shift[0:4]) padded with ones)''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB01111'], True, False, {}),
	'lsl1x:zric': InstructionInfo('lsl1x', 'lsl1x:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	cc = shift[0:4]
	if (const_cc_zero cc) then
		-1
	else
	ZF <- 	(ra >> (32 - shift[0:4]) padded with ones)''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB01111'], True, False, {}),
	'lsl1x:zrici': InstructionInfo('lsl1x', 'lsl1x:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - shift[0:4]) padded with ones
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB01111', 'targetPc'], True, False, {}),
	'lsl1x:zrr': InstructionInfo('lsl1x', 'lsl1x:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = rb[0:4]
	if (const_cc_zero cc) then
		-1
	else
	ZF <- 	(ra >> (32 - rb[0:4]) padded with ones)''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA01111', 'falseConditionShift5'], True, False, {}),
	'lsl1x:zrrc': InstructionInfo('lsl1x', 'lsl1x:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	cc = rb[0:4]
	if (const_cc_zero cc) then
		-1
	else
	ZF <- 	(ra >> (32 - rb[0:4]) padded with ones)''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA01111', 'shiftSetCondition'], True, False, {}),
	'lsl1x:zrrci': InstructionInfo('lsl1x', 'lsl1x:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra >> (32 - rb[0:4]) padded with ones
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA01111', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsl:rri': InstructionInfo('lsl', 'lsl:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra << shift[0:4]
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB01000'], True, False, {}),
	'lsl:rric': InstructionInfo('lsl', 'lsl:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra << shift[0:4]
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB01000'], True, False, {}),
	'lsl:rrici': InstructionInfo('lsl', 'lsl:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra << shift[0:4]
	rc = x
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB01000', 'targetPc'], True, False, {}),
	'lsl:rrr': InstructionInfo('lsl', 'lsl:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra << rb[0:4]
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA01000', 'falseConditionShift5'], True, False, {}),
	'lsl:rrrc': InstructionInfo('lsl', 'lsl:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra << rb[0:4]
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA01000', 'shiftSetCondition'], True, False, {}),
	'lsl:rrrci': InstructionInfo('lsl', 'lsl:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra << rb[0:4]
	rc = x
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA01000', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsl:zri': InstructionInfo('lsl', 'lsl:zri', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra << shift[0:4]''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB01000'], True, False, {}),
	'lsl:zric': InstructionInfo('lsl', 'lsl:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	ZF <- ra << shift[0:4]''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB01000'], True, False, {}),
	'lsl:zrici': InstructionInfo('lsl', 'lsl:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra << shift[0:4]
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB01000', 'targetPc'], True, False, {}),
	'lsl:zrr': InstructionInfo('lsl', 'lsl:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- ra << rb[0:4]''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA01000', 'falseConditionShift5'], True, False, {}),
	'lsl:zrrc': InstructionInfo('lsl', 'lsl:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- ra << rb[0:4]''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA01000', 'shiftSetCondition'], True, False, {}),
	'lsl:zrrci': InstructionInfo('lsl', 'lsl:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra << rb[0:4]
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA01000', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsl_add.s:rrri': InstructionInfo('lsl_add.s', 'lsl_add.s:rrri', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra << shift
	x = rb + (ra << shift[0:4])
	dc = x:S64''', ['opCode8Or9', 'rbEnabled', 'subOpCode0100x', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate5'], True, False, {}),
	'lsl_add.s:rrrici': InstructionInfo('lsl_add.s', 'lsl_add.s:rrrici', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('div_nz_cc'), PcSpec('pc', 16)], '''	ZF <- ra << shift
	x = rb + (ra << shift[0:4])
	dc = x:S64
	if (div_nz_cc x) then
		jump @[pc]''', ['opCode8Or9', 'rbEnabled', 'subOpCode0100x', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzDivCondition', 'immediate5', 'targetPc'], True, False, {}),
	'lsl_add.u:rrri': InstructionInfo('lsl_add.u', 'lsl_add.u:rrri', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra << shift
	x = rb + (ra << shift[0:4])
	dc = x:U64''', ['opCode8Or9', 'rbEnabled', 'subOpCode0100x', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate5'], True, False, {}),
	'lsl_add.u:rrrici': InstructionInfo('lsl_add.u', 'lsl_add.u:rrrici', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('div_nz_cc'), PcSpec('pc', 16)], '''	ZF <- ra << shift
	x = rb + (ra << shift[0:4])
	dc = x:U64
	if (div_nz_cc x) then
		jump @[pc]''', ['opCode8Or9', 'rbEnabled', 'subOpCode0100x', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzDivCondition', 'immediate5', 'targetPc'], True, False, {}),
	'lsl_add:rrri': InstructionInfo('lsl_add', 'lsl_add:rrri', [WorkRegister('rc', 32, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra << shift
	x = rb + (ra << shift[0:4])
	rc = x''', ['opCode8Or9', 'rbEnabled', 'subOpCode0100x', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate5'], True, False, {}),
	'lsl_add:rrrici': InstructionInfo('lsl_add', 'lsl_add:rrrici', [WorkRegister('rc', 32, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('div_nz_cc'), PcSpec('pc', 16)], '''	ZF <- ra << shift
	x = rb + (ra << shift[0:4])
	rc = x
	if (div_nz_cc x) then
		jump @[pc]''', ['opCode8Or9', 'rbEnabled', 'subOpCode0100x', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzDivCondition', 'immediate5', 'targetPc'], True, False, {}),
	'lsl_add:zrri': InstructionInfo('lsl_add', 'lsl_add:zrri', [ConstantRegister('zero'), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	instruction with no effect''', ['opCode8Or9', 'rbEnabled', 'subOpCode0100x', 'rcDisabled', 'raAllEnabled', 'falseCondition4', 'immediate5'], True, False, {}),
	'lsl_add:zrrici': InstructionInfo('lsl_add', 'lsl_add:zrrici', [ConstantRegister('zero'), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('div_nz_cc'), PcSpec('pc', 16)], '''	ZF <- ra << shift
	x = rb + (ra << shift[0:4])
	if (div_nz_cc x) then
		jump @[pc]''', ['opCode8Or9', 'rbEnabled', 'subOpCode0100x', 'rcDisabled', 'raAllEnabled', 'nzDivCondition', 'immediate5', 'targetPc'], True, False, {}),
	'lsl_sub.s:rrri': InstructionInfo('lsl_sub.s', 'lsl_sub.s:rrri', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra << shift
	x = rb - (ra << shift[0:4])
	dc = x:S64''', ['opCode8Or9', 'rbEnabled', 'subOpCode0111x', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate5'], True, False, {}),
	'lsl_sub.s:rrrici': InstructionInfo('lsl_sub.s', 'lsl_sub.s:rrrici', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('div_nz_cc'), PcSpec('pc', 16)], '''	ZF <- ra << shift
	x = rb - (ra << shift[0:4])
	dc = x:S64
	if (div_nz_cc x) then
		jump @[pc]''', ['opCode8Or9', 'rbEnabled', 'subOpCode0111x', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzDivCondition', 'immediate5', 'targetPc'], True, False, {}),
	'lsl_sub.u:rrri': InstructionInfo('lsl_sub.u', 'lsl_sub.u:rrri', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra << shift
	x = rb - (ra << shift[0:4])
	dc = x:U64''', ['opCode8Or9', 'rbEnabled', 'subOpCode0111x', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate5'], True, False, {}),
	'lsl_sub.u:rrrici': InstructionInfo('lsl_sub.u', 'lsl_sub.u:rrrici', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('div_nz_cc'), PcSpec('pc', 16)], '''	ZF <- ra << shift
	x = rb - (ra << shift[0:4])
	dc = x:U64
	if (div_nz_cc x) then
		jump @[pc]''', ['opCode8Or9', 'rbEnabled', 'subOpCode0111x', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzDivCondition', 'immediate5', 'targetPc'], True, False, {}),
	'lsl_sub:rrri': InstructionInfo('lsl_sub', 'lsl_sub:rrri', [WorkRegister('rc', 32, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra << shift
	x = rb - (ra << shift[0:4])
	rc = x''', ['opCode8Or9', 'rbEnabled', 'subOpCode0111x', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate5'], True, False, {}),
	'lsl_sub:rrrici': InstructionInfo('lsl_sub', 'lsl_sub:rrrici', [WorkRegister('rc', 32, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('div_nz_cc'), PcSpec('pc', 16)], '''	ZF <- ra << shift
	x = rb - (ra << shift[0:4])
	rc = x
	if (div_nz_cc x) then
		jump @[pc]''', ['opCode8Or9', 'rbEnabled', 'subOpCode0111x', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzDivCondition', 'immediate5', 'targetPc'], True, False, {}),
	'lsl_sub:zrri': InstructionInfo('lsl_sub', 'lsl_sub:zrri', [ConstantRegister('zero'), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	instruction with no effect''', ['opCode8Or9', 'rbEnabled', 'subOpCode0111x', 'rcDisabled', 'raAllEnabled', 'falseCondition4', 'immediate5'], True, False, {}),
	'lsl_sub:zrrici': InstructionInfo('lsl_sub', 'lsl_sub:zrrici', [ConstantRegister('zero'), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('div_nz_cc'), PcSpec('pc', 16)], '''	ZF <- ra << shift
	x = rb - (ra << shift[0:4])
	if (div_nz_cc x) then
		jump @[pc]''', ['opCode8Or9', 'rbEnabled', 'subOpCode0111x', 'rcDisabled', 'raAllEnabled', 'nzDivCondition', 'immediate5', 'targetPc'], True, False, {}),
	'lslx.s:rri': InstructionInfo('lslx.s', 'lslx.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - shift[0:4])
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB01001'], True, False, {}),
	'lslx.s:rric': InstructionInfo('lslx.s', 'lslx.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - shift[0:4])
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB01001'], True, False, {}),
	'lslx.s:rrici': InstructionInfo('lslx.s', 'lslx.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - shift[0:4])
	dc = x:S64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB01001', 'targetPc'], True, False, {}),
	'lslx.s:rrr': InstructionInfo('lslx.s', 'lslx.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - rb[0:4])
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA01001', 'falseConditionShift5'], True, False, {}),
	'lslx.s:rrrc': InstructionInfo('lslx.s', 'lslx.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - rb[0:4])
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA01001', 'shiftSetCondition'], True, False, {}),
	'lslx.s:rrrci': InstructionInfo('lslx.s', 'lslx.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - rb[0:4])
	dc = x:S64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA01001', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lslx.u:rri': InstructionInfo('lslx.u', 'lslx.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - shift[0:4])
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB01001'], True, False, {}),
	'lslx.u:rric': InstructionInfo('lslx.u', 'lslx.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - shift[0:4])
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB01001'], True, False, {}),
	'lslx.u:rrici': InstructionInfo('lslx.u', 'lslx.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - shift[0:4])
	dc = x:U64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB01001', 'targetPc'], True, False, {}),
	'lslx.u:rrr': InstructionInfo('lslx.u', 'lslx.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - rb[0:4])
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA01001', 'falseConditionShift5'], True, False, {}),
	'lslx.u:rrrc': InstructionInfo('lslx.u', 'lslx.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - rb[0:4])
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA01001', 'shiftSetCondition'], True, False, {}),
	'lslx.u:rrrci': InstructionInfo('lslx.u', 'lslx.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - rb[0:4])
	dc = x:U64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA01001', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lslx:rri': InstructionInfo('lslx', 'lslx:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - shift[0:4])
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB01001'], True, False, {}),
	'lslx:rric': InstructionInfo('lslx', 'lslx:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - shift[0:4])
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB01001'], True, False, {}),
	'lslx:rrici': InstructionInfo('lslx', 'lslx:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - shift[0:4])
	rc = x
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB01001', 'targetPc'], True, False, {}),
	'lslx:rrr': InstructionInfo('lslx', 'lslx:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - rb[0:4])
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA01001', 'falseConditionShift5'], True, False, {}),
	'lslx:rrrc': InstructionInfo('lslx', 'lslx:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - rb[0:4])
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA01001', 'shiftSetCondition'], True, False, {}),
	'lslx:rrrci': InstructionInfo('lslx', 'lslx:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - rb[0:4])
	rc = x
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA01001', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lslx:zri': InstructionInfo('lslx', 'lslx:zri', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	cc = shift[0:4]
	if (const_cc_zero cc) then
		0
	else
	ZF <- 	(ra >> (32 - shift[0:4]))''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB01001'], True, False, {}),
	'lslx:zric': InstructionInfo('lslx', 'lslx:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	cc = shift[0:4]
	if (const_cc_zero cc) then
		0
	else
	ZF <- 	(ra >> (32 - shift[0:4]))''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB01001'], True, False, {}),
	'lslx:zrici': InstructionInfo('lslx', 'lslx:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - shift[0:4])
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB01001', 'targetPc'], True, False, {}),
	'lslx:zrr': InstructionInfo('lslx', 'lslx:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = rb[0:4]
	if (const_cc_zero cc) then
		0
	else
	ZF <- 	(ra >> (32 - rb[0:4]))''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA01001', 'falseConditionShift5'], True, False, {}),
	'lslx:zrrc': InstructionInfo('lslx', 'lslx:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	cc = rb[0:4]
	if (const_cc_zero cc) then
		0
	else
	ZF <- 	(ra >> (32 - rb[0:4]))''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA01001', 'shiftSetCondition'], True, False, {}),
	'lslx:zrrci': InstructionInfo('lslx', 'lslx:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra >> (32 - rb[0:4])
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA01001', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsr.s:rri': InstructionInfo('lsr.s', 'lsr.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra >> shift[0:4]
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB11000'], True, False, {}),
	'lsr.s:rric': InstructionInfo('lsr.s', 'lsr.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra >> shift[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB11000'], True, False, {}),
	'lsr.s:rrici': InstructionInfo('lsr.s', 'lsr.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >> shift[0:4]
	dc = x:S64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB11000', 'targetPc'], True, False, {}),
	'lsr.s:rrr': InstructionInfo('lsr.s', 'lsr.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra >> rb[0:4]
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA11000', 'falseConditionShift5'], True, False, {}),
	'lsr.s:rrrc': InstructionInfo('lsr.s', 'lsr.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra >> rb[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA11000', 'shiftSetCondition'], True, False, {}),
	'lsr.s:rrrci': InstructionInfo('lsr.s', 'lsr.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >> rb[0:4]
	dc = x:S64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA11000', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsr.u:rri': InstructionInfo('lsr.u', 'lsr.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra >> shift[0:4]
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB11000'], True, False, {}),
	'lsr.u:rric': InstructionInfo('lsr.u', 'lsr.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra >> shift[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB11000'], True, False, {}),
	'lsr.u:rrici': InstructionInfo('lsr.u', 'lsr.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >> shift[0:4]
	dc = x:U64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB11000', 'targetPc'], True, False, {}),
	'lsr.u:rrr': InstructionInfo('lsr.u', 'lsr.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra >> rb[0:4]
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA11000', 'falseConditionShift5'], True, False, {}),
	'lsr.u:rrrc': InstructionInfo('lsr.u', 'lsr.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra >> rb[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA11000', 'shiftSetCondition'], True, False, {}),
	'lsr.u:rrrci': InstructionInfo('lsr.u', 'lsr.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >> rb[0:4]
	dc = x:U64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA11000', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsr1.s:rri': InstructionInfo('lsr1.s', 'lsr1.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra >> shift[0:4] padded with ones
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB11110'], True, False, {}),
	'lsr1.s:rric': InstructionInfo('lsr1.s', 'lsr1.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra >> shift[0:4] padded with ones
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB11110'], True, False, {}),
	'lsr1.s:rrici': InstructionInfo('lsr1.s', 'lsr1.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >> shift[0:4] padded with ones
	dc = x:S64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB11110', 'targetPc'], True, False, {}),
	'lsr1.s:rrr': InstructionInfo('lsr1.s', 'lsr1.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra >> rb[0:4] padded with ones
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA11110', 'falseConditionShift5'], True, False, {}),
	'lsr1.s:rrrc': InstructionInfo('lsr1.s', 'lsr1.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra >> rb[0:4] padded with ones
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA11110', 'shiftSetCondition'], True, False, {}),
	'lsr1.s:rrrci': InstructionInfo('lsr1.s', 'lsr1.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >> rb[0:4] padded with ones
	dc = x:S64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA11110', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsr1.u:rri': InstructionInfo('lsr1.u', 'lsr1.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra >> shift[0:4] padded with ones
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB11110'], True, False, {}),
	'lsr1.u:rric': InstructionInfo('lsr1.u', 'lsr1.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra >> shift[0:4] padded with ones
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB11110'], True, False, {}),
	'lsr1.u:rrici': InstructionInfo('lsr1.u', 'lsr1.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >> shift[0:4] padded with ones
	dc = x:U64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB11110', 'targetPc'], True, False, {}),
	'lsr1.u:rrr': InstructionInfo('lsr1.u', 'lsr1.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra >> rb[0:4] padded with ones
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA11110', 'falseConditionShift5'], True, False, {}),
	'lsr1.u:rrrc': InstructionInfo('lsr1.u', 'lsr1.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra >> rb[0:4] padded with ones
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA11110', 'shiftSetCondition'], True, False, {}),
	'lsr1.u:rrrci': InstructionInfo('lsr1.u', 'lsr1.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >> rb[0:4] padded with ones
	dc = x:U64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA11110', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsr1:rri': InstructionInfo('lsr1', 'lsr1:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra >> shift[0:4] padded with ones
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB11110'], True, False, {}),
	'lsr1:rric': InstructionInfo('lsr1', 'lsr1:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra >> shift[0:4] padded with ones
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB11110'], True, False, {}),
	'lsr1:rrici': InstructionInfo('lsr1', 'lsr1:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >> shift[0:4] padded with ones
	rc = x
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB11110', 'targetPc'], True, False, {}),
	'lsr1:rrr': InstructionInfo('lsr1', 'lsr1:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra >> rb[0:4] padded with ones
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA11110', 'falseConditionShift5'], True, False, {}),
	'lsr1:rrrc': InstructionInfo('lsr1', 'lsr1:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra >> rb[0:4] padded with ones
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA11110', 'shiftSetCondition'], True, False, {}),
	'lsr1:rrrci': InstructionInfo('lsr1', 'lsr1:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >> rb[0:4] padded with ones
	rc = x
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA11110', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsr1:zri': InstructionInfo('lsr1', 'lsr1:zri', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra >> shift[0:4] padded with ones''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB11110'], True, False, {}),
	'lsr1:zric': InstructionInfo('lsr1', 'lsr1:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	ZF <- ra >> shift[0:4] padded with ones''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB11110'], True, False, {}),
	'lsr1:zrici': InstructionInfo('lsr1', 'lsr1:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >> shift[0:4] padded with ones
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB11110', 'targetPc'], True, False, {}),
	'lsr1:zrr': InstructionInfo('lsr1', 'lsr1:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- ra >> rb[0:4] padded with ones''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA11110', 'falseConditionShift5'], True, False, {}),
	'lsr1:zrrc': InstructionInfo('lsr1', 'lsr1:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- ra >> rb[0:4] padded with ones''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA11110', 'shiftSetCondition'], True, False, {}),
	'lsr1:zrrci': InstructionInfo('lsr1', 'lsr1:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >> rb[0:4] padded with ones
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA11110', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsr1x.s:rri': InstructionInfo('lsr1x.s', 'lsr1x.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - shift[0:4]) padded with ones
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB11111'], True, False, {}),
	'lsr1x.s:rric': InstructionInfo('lsr1x.s', 'lsr1x.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - shift[0:4]) padded with ones
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB11111'], True, False, {}),
	'lsr1x.s:rrici': InstructionInfo('lsr1x.s', 'lsr1x.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - shift[0:4]) padded with ones
	dc = x:S64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB11111', 'targetPc'], True, False, {}),
	'lsr1x.s:rrr': InstructionInfo('lsr1x.s', 'lsr1x.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - rb[0:4]) padded with ones
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA11111', 'falseConditionShift5'], True, False, {}),
	'lsr1x.s:rrrc': InstructionInfo('lsr1x.s', 'lsr1x.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - rb[0:4]) padded with ones
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA11111', 'shiftSetCondition'], True, False, {}),
	'lsr1x.s:rrrci': InstructionInfo('lsr1x.s', 'lsr1x.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - rb[0:4]) padded with ones
	dc = x:S64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA11111', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsr1x.u:rri': InstructionInfo('lsr1x.u', 'lsr1x.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - shift[0:4]) padded with ones
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB11111'], True, False, {}),
	'lsr1x.u:rric': InstructionInfo('lsr1x.u', 'lsr1x.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - shift[0:4]) padded with ones
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB11111'], True, False, {}),
	'lsr1x.u:rrici': InstructionInfo('lsr1x.u', 'lsr1x.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - shift[0:4]) padded with ones
	dc = x:U64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB11111', 'targetPc'], True, False, {}),
	'lsr1x.u:rrr': InstructionInfo('lsr1x.u', 'lsr1x.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - rb[0:4]) padded with ones
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA11111', 'falseConditionShift5'], True, False, {}),
	'lsr1x.u:rrrc': InstructionInfo('lsr1x.u', 'lsr1x.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - rb[0:4]) padded with ones
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA11111', 'shiftSetCondition'], True, False, {}),
	'lsr1x.u:rrrci': InstructionInfo('lsr1x.u', 'lsr1x.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - rb[0:4]) padded with ones
	dc = x:U64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA11111', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsr1x:rri': InstructionInfo('lsr1x', 'lsr1x:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - shift[0:4]) padded with ones
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB11111'], True, False, {}),
	'lsr1x:rric': InstructionInfo('lsr1x', 'lsr1x:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - shift[0:4]) padded with ones
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB11111'], True, False, {}),
	'lsr1x:rrici': InstructionInfo('lsr1x', 'lsr1x:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - shift[0:4]) padded with ones
	rc = x
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB11111', 'targetPc'], True, False, {}),
	'lsr1x:rrr': InstructionInfo('lsr1x', 'lsr1x:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - rb[0:4]) padded with ones
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA11111', 'falseConditionShift5'], True, False, {}),
	'lsr1x:rrrc': InstructionInfo('lsr1x', 'lsr1x:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - rb[0:4]) padded with ones
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA11111', 'shiftSetCondition'], True, False, {}),
	'lsr1x:rrrci': InstructionInfo('lsr1x', 'lsr1x:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - rb[0:4]) padded with ones
	rc = x
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA11111', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsr1x:zri': InstructionInfo('lsr1x', 'lsr1x:zri', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	cc = shift[0:4]
	if (const_cc_zero cc) then
		-1
	else
	ZF <- 	(ra << (32 - shift[0:4]) padded with ones)''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB11111'], True, False, {}),
	'lsr1x:zric': InstructionInfo('lsr1x', 'lsr1x:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	cc = shift[0:4]
	if (const_cc_zero cc) then
		-1
	else
	ZF <- 	(ra << (32 - shift[0:4]) padded with ones)''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB11111'], True, False, {}),
	'lsr1x:zrici': InstructionInfo('lsr1x', 'lsr1x:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - shift[0:4]) padded with ones
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB11111', 'targetPc'], True, False, {}),
	'lsr1x:zrr': InstructionInfo('lsr1x', 'lsr1x:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = rb[0:4]
	if (const_cc_zero cc) then
		-1
	else
	ZF <- 	(ra << (32 - rb[0:4]) padded with ones)''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA11111', 'falseConditionShift5'], True, False, {}),
	'lsr1x:zrrc': InstructionInfo('lsr1x', 'lsr1x:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	cc = rb[0:4]
	if (const_cc_zero cc) then
		-1
	else
	ZF <- 	(ra << (32 - rb[0:4]) padded with ones)''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA11111', 'shiftSetCondition'], True, False, {}),
	'lsr1x:zrrci': InstructionInfo('lsr1x', 'lsr1x:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		-1
	else
		ra << (32 - rb[0:4]) padded with ones
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA11111', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsr:rri': InstructionInfo('lsr', 'lsr:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra >> shift[0:4]
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB11000'], True, False, {}),
	'lsr:rric': InstructionInfo('lsr', 'lsr:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra >> shift[0:4]
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB11000'], True, False, {}),
	'lsr:rrici': InstructionInfo('lsr', 'lsr:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >> shift[0:4]
	rc = x
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB11000', 'targetPc'], True, False, {}),
	'lsr:rrr': InstructionInfo('lsr', 'lsr:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra >> rb[0:4]
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA11000', 'falseConditionShift5'], True, False, {}),
	'lsr:rrrc': InstructionInfo('lsr', 'lsr:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra >> rb[0:4]
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA11000', 'shiftSetCondition'], True, False, {}),
	'lsr:rrrci': InstructionInfo('lsr', 'lsr:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >> rb[0:4]
	rc = x
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA11000', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsr:zri': InstructionInfo('lsr', 'lsr:zri', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra >> shift[0:4]''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB11000'], True, False, {}),
	'lsr:zric': InstructionInfo('lsr', 'lsr:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	ZF <- ra >> shift[0:4]''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB11000'], True, False, {}),
	'lsr:zrici': InstructionInfo('lsr', 'lsr:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >> shift[0:4]
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB11000', 'targetPc'], True, False, {}),
	'lsr:zrr': InstructionInfo('lsr', 'lsr:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- ra >> rb[0:4]''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA11000', 'falseConditionShift5'], True, False, {}),
	'lsr:zrrc': InstructionInfo('lsr', 'lsr:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- ra >> rb[0:4]''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA11000', 'shiftSetCondition'], True, False, {}),
	'lsr:zrrci': InstructionInfo('lsr', 'lsr:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >> rb[0:4]
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA11000', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsr_add.s:rrri': InstructionInfo('lsr_add.s', 'lsr_add.s:rrri', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra >> shift
	x = rb + (ra >> shift[0:4])
	dc = x:S64''', ['opCode8Or9', 'rbEnabled', 'subOpCode0011x', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate5'], True, False, {}),
	'lsr_add.s:rrrici': InstructionInfo('lsr_add.s', 'lsr_add.s:rrrici', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('div_nz_cc'), PcSpec('pc', 16)], '''	ZF <- ra >> shift
	x = rb + (ra >> shift[0:4])
	dc = x:S64
	if (div_nz_cc x) then
		jump @[pc]''', ['opCode8Or9', 'rbEnabled', 'subOpCode0011x', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzDivCondition', 'immediate5', 'targetPc'], True, False, {}),
	'lsr_add.u:rrri': InstructionInfo('lsr_add.u', 'lsr_add.u:rrri', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra >> shift
	x = rb + (ra >> shift[0:4])
	dc = x:U64''', ['opCode8Or9', 'rbEnabled', 'subOpCode0011x', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate5'], True, False, {}),
	'lsr_add.u:rrrici': InstructionInfo('lsr_add.u', 'lsr_add.u:rrrici', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('div_nz_cc'), PcSpec('pc', 16)], '''	ZF <- ra >> shift
	x = rb + (ra >> shift[0:4])
	dc = x:U64
	if (div_nz_cc x) then
		jump @[pc]''', ['opCode8Or9', 'rbEnabled', 'subOpCode0011x', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzDivCondition', 'immediate5', 'targetPc'], True, False, {}),
	'lsr_add:rrri': InstructionInfo('lsr_add', 'lsr_add:rrri', [WorkRegister('rc', 32, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra >> shift
	x = rb + (ra >> shift[0:4])
	rc = x''', ['opCode8Or9', 'rbEnabled', 'subOpCode0011x', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate5'], True, False, {}),
	'lsr_add:rrrici': InstructionInfo('lsr_add', 'lsr_add:rrrici', [WorkRegister('rc', 32, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('div_nz_cc'), PcSpec('pc', 16)], '''	ZF <- ra >> shift
	x = rb + (ra >> shift[0:4])
	rc = x
	if (div_nz_cc x) then
		jump @[pc]''', ['opCode8Or9', 'rbEnabled', 'subOpCode0011x', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzDivCondition', 'immediate5', 'targetPc'], True, False, {}),
	'lsr_add:zrri': InstructionInfo('lsr_add', 'lsr_add:zrri', [ConstantRegister('zero'), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	instruction with no effect''', ['opCode8Or9', 'rbEnabled', 'subOpCode0011x', 'rcDisabled', 'raAllEnabled', 'falseCondition4', 'immediate5'], True, False, {}),
	'lsr_add:zrrici': InstructionInfo('lsr_add', 'lsr_add:zrrici', [ConstantRegister('zero'), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('div_nz_cc'), PcSpec('pc', 16)], '''	ZF <- ra >> shift
	x = rb + (ra >> shift[0:4])
	if (div_nz_cc x) then
		jump @[pc]''', ['opCode8Or9', 'rbEnabled', 'subOpCode0011x', 'rcDisabled', 'raAllEnabled', 'nzDivCondition', 'immediate5', 'targetPc'], True, False, {}),
	'lsrx.s:rri': InstructionInfo('lsrx.s', 'lsrx.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - shift[0:4])
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB11001'], True, False, {}),
	'lsrx.s:rric': InstructionInfo('lsrx.s', 'lsrx.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - shift[0:4])
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB11001'], True, False, {}),
	'lsrx.s:rrici': InstructionInfo('lsrx.s', 'lsrx.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - shift[0:4])
	dc = x:S64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB11001', 'targetPc'], True, False, {}),
	'lsrx.s:rrr': InstructionInfo('lsrx.s', 'lsrx.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - rb[0:4])
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA11001', 'falseConditionShift5'], True, False, {}),
	'lsrx.s:rrrc': InstructionInfo('lsrx.s', 'lsrx.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - rb[0:4])
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA11001', 'shiftSetCondition'], True, False, {}),
	'lsrx.s:rrrci': InstructionInfo('lsrx.s', 'lsrx.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - rb[0:4])
	dc = x:S64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA11001', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsrx.u:rri': InstructionInfo('lsrx.u', 'lsrx.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - shift[0:4])
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB11001'], True, False, {}),
	'lsrx.u:rric': InstructionInfo('lsrx.u', 'lsrx.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - shift[0:4])
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB11001'], True, False, {}),
	'lsrx.u:rrici': InstructionInfo('lsrx.u', 'lsrx.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - shift[0:4])
	dc = x:U64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB11001', 'targetPc'], True, False, {}),
	'lsrx.u:rrr': InstructionInfo('lsrx.u', 'lsrx.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - rb[0:4])
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA11001', 'falseConditionShift5'], True, False, {}),
	'lsrx.u:rrrc': InstructionInfo('lsrx.u', 'lsrx.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - rb[0:4])
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA11001', 'shiftSetCondition'], True, False, {}),
	'lsrx.u:rrrci': InstructionInfo('lsrx.u', 'lsrx.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - rb[0:4])
	dc = x:U64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA11001', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsrx:rri': InstructionInfo('lsrx', 'lsrx:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - shift[0:4])
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB11001'], True, False, {}),
	'lsrx:rric': InstructionInfo('lsrx', 'lsrx:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - shift[0:4])
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB11001'], True, False, {}),
	'lsrx:rrici': InstructionInfo('lsrx', 'lsrx:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - shift[0:4])
	rc = x
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB11001', 'targetPc'], True, False, {}),
	'lsrx:rrr': InstructionInfo('lsrx', 'lsrx:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - rb[0:4])
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA11001', 'falseConditionShift5'], True, False, {}),
	'lsrx:rrrc': InstructionInfo('lsrx', 'lsrx:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - rb[0:4])
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA11001', 'shiftSetCondition'], True, False, {}),
	'lsrx:rrrci': InstructionInfo('lsrx', 'lsrx:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - rb[0:4])
	rc = x
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA11001', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lsrx:zri': InstructionInfo('lsrx', 'lsrx:zri', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	cc = shift[0:4]
	if (const_cc_zero cc) then
		0
	else
	ZF <- 	(ra << (32 - shift[0:4]))''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB11001'], True, False, {}),
	'lsrx:zric': InstructionInfo('lsrx', 'lsrx:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	cc = shift[0:4]
	if (const_cc_zero cc) then
		0
	else
	ZF <- 	(ra << (32 - shift[0:4]))''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB11001'], True, False, {}),
	'lsrx:zrici': InstructionInfo('lsrx', 'lsrx:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	cc = shift[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - shift[0:4])
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB11001', 'targetPc'], True, False, {}),
	'lsrx:zrr': InstructionInfo('lsrx', 'lsrx:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = rb[0:4]
	if (const_cc_zero cc) then
		0
	else
	ZF <- 	(ra << (32 - rb[0:4]))''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA11001', 'falseConditionShift5'], True, False, {}),
	'lsrx:zrrc': InstructionInfo('lsrx', 'lsrx:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	cc = rb[0:4]
	if (const_cc_zero cc) then
		0
	else
	ZF <- 	(ra << (32 - rb[0:4]))''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA11001', 'shiftSetCondition'], True, False, {}),
	'lsrx:zrrci': InstructionInfo('lsrx', 'lsrx:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	cc = rb[0:4]
	x = if (const_cc_zero cc) then
		0
	else
		ra << (32 - rb[0:4])
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA11001', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'lw.s:erri': InstructionInfo('lw.s', 'lw.s:erri', [Endianess('endian'), WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	dc = (Load 4 bytes from WRAM at address @a with endianness endian):S64''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx100x', 'rcSignedExtendedLoadArg'], True, False, {'lw.s:rri': (0x0, 0x8000000)}),
	'lw.s:rri': InstructionInfo('lw.s', 'lw.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	dc = (Load 4 bytes from WRAM at address @a with endianness endian):S64''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx100x', 'rcSignedExtendedLoadArg', 'syntacticSugar'], True, True, {}),
	'lw.u:erri': InstructionInfo('lw.u', 'lw.u:erri', [Endianess('endian'), WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	dc = (Load 4 bytes from WRAM at address @a with endianness endian):U64''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx100x', 'rcUnsignedExtendedLoadArg'], True, False, {'lw.u:rri': (0x0, 0x8000000)}),
	'lw.u:rri': InstructionInfo('lw.u', 'lw.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	dc = (Load 4 bytes from WRAM at address @a with endianness endian):U64''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx100x', 'rcUnsignedExtendedLoadArg', 'syntacticSugar'], True, True, {}),
	'lw:erri': InstructionInfo('lw', 'lw:erri', [Endianess('endian'), WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	rc = (Load 4 bytes from WRAM at address @a with endianness endian)''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx100x', 'rcNotExtendedLoadArg'], True, False, {'lw:rri': (0x0, 0x8000000)}),
	'lw:ersi': InstructionInfo('lw', 'lw:ersi', [Endianess('endian'), WorkRegister('rc', 32, False, False), WorkRegister('sa', 32, True, True), Immediate('off', 24, True)], '''	cc = (ra & 0xffff) + off + 4 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfffc
		rc = (Load 4 bytes from WRAM at address @a with endianness endian)
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		rc = (Load 4 bytes from WRAM at address @a with endianness endian)''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreSafeRegister', 'raAllEnabled', 'endianess', 'immediate24', 'subAuxx100x', 'rcNotExtendedLoadArg'], True, False, {'lws:rri': (0x0, 0x8000000)}),
	'lw:rri': InstructionInfo('lw', 'lw:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	let @a = (ra + off)
	rc = (Load 4 bytes from WRAM at address @a with endianness endian)''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx100x', 'rcNotExtendedLoadArg', 'syntacticSugar'], True, True, {}),
	'lws:rri': InstructionInfo('lws', 'lws:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('off', 24, True)], '''	cc = (ra & 0xffff) + off + 4 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfffc
		rc = (Load 4 bytes from WRAM at address @a with endianness endian)
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		rc = (Load 4 bytes from WRAM at address @a with endianness endian)''', ['opCode7', 'rbDisabled', 'rcExists', 'notDcLoad', 'subOpCode01xxx', 'loadStoreSafeRegister', 'raAllEnabled', 'immediate24', 'subAuxx100x', 'rcNotExtendedLoadArg', 'syntacticSugar'], True, True, {}),
	'movd:rr': InstructionInfo('movd', 'movd:rr', [WorkRegister('dc', 64, False, False), WorkRegister('db', 64, False, False)], '''	cc = dc = (dbe, dbo)
	if (true_false_cc cc) then
		jump @[pc]''', ['opCode8Or9', 'dbEnabled', 'subOpCode00110', 'dcEnabled', 'raIsZero', 'bootCondition', 'syntacticSugar'], True, True, {}),
	'movd:rrci': InstructionInfo('movd', 'movd:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('db', 64, False, False), Condition('true_false_cc'), PcSpec('pc', 16)], '''	cc = dc = (dbe, dbo)
	if (true_false_cc cc) then
		jump @[pc]''', ['opCode8Or9', 'dbEnabled', 'subOpCode00110', 'dcEnabled', 'raIsZero', 'bootCondition', 'targetPc'], True, False, {'movd:rr': (0x0, 0xf00ffff)}),
	'move.s:ri': InstructionInfo('move.s', 'move.s:ri', [WorkRegister('dc', 64, False, False), Immediate('imm', 32, False)], '''	dc = (ra & imm):S64
	ZF <- dc''', ['opCode4Or5', 'rbDisabled', 'opCode5', 'raCstEnabled', 'rcSignedExtendedEnabled', 'immediate32', 'syntacticSugar'], True, True, {}),
	'move.s:rici': InstructionInfo('move.s', 'move.s:rici', [WorkRegister('dc', 64, False, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra | imm
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcSignedExtendedEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc', 'syntacticSugar'], True, True, {}),
	'move.s:rr': InstructionInfo('move.s', 'move.s:rr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False)], '''	x = ra | imm
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'syntacticSugar'], True, True, {}),
	'move.s:rrci': InstructionInfo('move.s', 'move.s:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra | imm
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'move.u:ri': InstructionInfo('move.u', 'move.u:ri', [WorkRegister('dc', 64, False, False), Immediate('imm', 32, False)], '''	dc = (ra & imm):U64
	ZF <- dc''', ['opCode4Or5', 'rbDisabled', 'opCode5', 'raCstEnabled', 'rcUnsignedExtendedEnabled', 'immediate32', 'syntacticSugar'], True, True, {}),
	'move.u:rici': InstructionInfo('move.u', 'move.u:rici', [WorkRegister('dc', 64, False, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra | imm
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcUnsignedExtendedEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc', 'syntacticSugar'], True, True, {}),
	'move.u:rr': InstructionInfo('move.u', 'move.u:rr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False)], '''	x = ra | imm
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'syntacticSugar'], True, True, {}),
	'move.u:rrci': InstructionInfo('move.u', 'move.u:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra | imm
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'move:ri': InstructionInfo('move', 'move:ri', [WorkRegister('rc', 32, False, False), Immediate('imm', 32, False)], '''	rc = (ra | imm)
	ZF <- rc''', ['opCode6', 'rbDisabled', 'immediate32', 'rcEnabled', 'syntacticSugar'], True, True, {}),
	'move:rici': InstructionInfo('move', 'move:rici', [WorkRegister('rc', 32, False, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra | imm
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcNotExtendedEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc', 'syntacticSugar'], True, True, {}),
	'move:rr': InstructionInfo('move', 'move:rr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False)], '''	x = ra | imm
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'syntacticSugar'], True, True, {}),
	'move:rrci': InstructionInfo('move', 'move:rrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra | imm
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'mul_sh_sh.s:rrr': InstructionInfo('mul_sh_sh.s', 'mul_sh_sh.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[8:15] * rb[8:15]):S32
	dc = x:S64
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01111', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_sh_sh.s:rrrc': InstructionInfo('mul_sh_sh.s', 'mul_sh_sh.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[8:15] * rb[8:15]):S32
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01111', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_sh_sh.s:rrrci': InstructionInfo('mul_sh_sh.s', 'mul_sh_sh.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[8:15]):S32
	dc = x:S64
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01111', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sh_sh:rrr': InstructionInfo('mul_sh_sh', 'mul_sh_sh:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[8:15] * rb[8:15]):S32
	rc = x
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01111', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_sh_sh:rrrc': InstructionInfo('mul_sh_sh', 'mul_sh_sh:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[8:15] * rb[8:15]):S32
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01111', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_sh_sh:rrrci': InstructionInfo('mul_sh_sh', 'mul_sh_sh:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[8:15]):S32
	rc = x
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01111', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sh_sh:zrr': InstructionInfo('mul_sh_sh', 'mul_sh_sh:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- (ra[8:15] * rb[8:15]):S32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01111', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'falseCondition4'], True, False, {}),
	'mul_sh_sh:zrrc': InstructionInfo('mul_sh_sh', 'mul_sh_sh:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- (ra[8:15] * rb[8:15]):S32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01111', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'logicalSetCondition'], True, False, {}),
	'mul_sh_sh:zrrci': InstructionInfo('mul_sh_sh', 'mul_sh_sh:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[8:15]):S32
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01111', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sh_sl.s:rrr': InstructionInfo('mul_sh_sl.s', 'mul_sh_sl.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[8:15] * rb[0:7]):S32
	dc = x:S64
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01110', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_sh_sl.s:rrrc': InstructionInfo('mul_sh_sl.s', 'mul_sh_sl.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[8:15] * rb[0:7]):S32
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01110', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_sh_sl.s:rrrci': InstructionInfo('mul_sh_sl.s', 'mul_sh_sl.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[0:7]):S32
	dc = x:S64
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01110', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sh_sl:rrr': InstructionInfo('mul_sh_sl', 'mul_sh_sl:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[8:15] * rb[0:7]):S32
	rc = x
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01110', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_sh_sl:rrrc': InstructionInfo('mul_sh_sl', 'mul_sh_sl:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[8:15] * rb[0:7]):S32
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01110', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_sh_sl:rrrci': InstructionInfo('mul_sh_sl', 'mul_sh_sl:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[0:7]):S32
	rc = x
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01110', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sh_sl:zrr': InstructionInfo('mul_sh_sl', 'mul_sh_sl:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- (ra[8:15] * rb[0:7]):S32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01110', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'falseCondition4'], True, False, {}),
	'mul_sh_sl:zrrc': InstructionInfo('mul_sh_sl', 'mul_sh_sl:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- (ra[8:15] * rb[0:7]):S32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01110', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'logicalSetCondition'], True, False, {}),
	'mul_sh_sl:zrrci': InstructionInfo('mul_sh_sl', 'mul_sh_sl:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[0:7]):S32
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01110', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sh_uh.s:rrr': InstructionInfo('mul_sh_uh.s', 'mul_sh_uh.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[8:15] * rb[8:15]):S32
	dc = x:S64
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10111', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_sh_uh.s:rrrc': InstructionInfo('mul_sh_uh.s', 'mul_sh_uh.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[8:15] * rb[8:15]):S32
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10111', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_sh_uh.s:rrrci': InstructionInfo('mul_sh_uh.s', 'mul_sh_uh.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[8:15]):S32
	dc = x:S64
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10111', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sh_uh:rrr': InstructionInfo('mul_sh_uh', 'mul_sh_uh:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[8:15] * rb[8:15]):S32
	rc = x
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10111', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_sh_uh:rrrc': InstructionInfo('mul_sh_uh', 'mul_sh_uh:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[8:15] * rb[8:15]):S32
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10111', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_sh_uh:rrrci': InstructionInfo('mul_sh_uh', 'mul_sh_uh:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[8:15]):S32
	rc = x
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10111', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sh_uh:zrr': InstructionInfo('mul_sh_uh', 'mul_sh_uh:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- (ra[8:15] * rb[8:15]):S32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10111', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'falseCondition4'], True, False, {}),
	'mul_sh_uh:zrrc': InstructionInfo('mul_sh_uh', 'mul_sh_uh:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- (ra[8:15] * rb[8:15]):S32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10111', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'logicalSetCondition'], True, False, {}),
	'mul_sh_uh:zrrci': InstructionInfo('mul_sh_uh', 'mul_sh_uh:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[8:15]):S32
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10111', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sh_ul.s:rrr': InstructionInfo('mul_sh_ul.s', 'mul_sh_ul.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[8:15] * rb[0:7]):S32
	dc = x:S64
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10110', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_sh_ul.s:rrrc': InstructionInfo('mul_sh_ul.s', 'mul_sh_ul.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[8:15] * rb[0:7]):S32
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10110', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_sh_ul.s:rrrci': InstructionInfo('mul_sh_ul.s', 'mul_sh_ul.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[0:7]):S32
	dc = x:S64
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10110', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sh_ul:rrr': InstructionInfo('mul_sh_ul', 'mul_sh_ul:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[8:15] * rb[0:7]):S32
	rc = x
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10110', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_sh_ul:rrrc': InstructionInfo('mul_sh_ul', 'mul_sh_ul:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[8:15] * rb[0:7]):S32
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10110', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_sh_ul:rrrci': InstructionInfo('mul_sh_ul', 'mul_sh_ul:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[0:7]):S32
	rc = x
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10110', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sh_ul:zrr': InstructionInfo('mul_sh_ul', 'mul_sh_ul:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- (ra[8:15] * rb[0:7]):S32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10110', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'falseCondition4'], True, False, {}),
	'mul_sh_ul:zrrc': InstructionInfo('mul_sh_ul', 'mul_sh_ul:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- (ra[8:15] * rb[0:7]):S32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10110', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'logicalSetCondition'], True, False, {}),
	'mul_sh_ul:zrrci': InstructionInfo('mul_sh_ul', 'mul_sh_ul:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[0:7]):S32
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10110', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sl_sh.s:rrr': InstructionInfo('mul_sl_sh.s', 'mul_sl_sh.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[0:7] * rb[8:15]):S32
	dc = x:S64
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01001', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_sl_sh.s:rrrc': InstructionInfo('mul_sl_sh.s', 'mul_sl_sh.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[0:7] * rb[8:15]):S32
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01001', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_sl_sh.s:rrrci': InstructionInfo('mul_sl_sh.s', 'mul_sl_sh.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[8:15]):S32
	dc = x:S64
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01001', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sl_sh:rrr': InstructionInfo('mul_sl_sh', 'mul_sl_sh:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[0:7] * rb[8:15]):S32
	rc = x
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01001', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_sl_sh:rrrc': InstructionInfo('mul_sl_sh', 'mul_sl_sh:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[0:7] * rb[8:15]):S32
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01001', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_sl_sh:rrrci': InstructionInfo('mul_sl_sh', 'mul_sl_sh:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[8:15]):S32
	rc = x
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01001', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sl_sh:zrr': InstructionInfo('mul_sl_sh', 'mul_sl_sh:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- (ra[0:7] * rb[8:15]):S32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01001', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'falseCondition4'], True, False, {}),
	'mul_sl_sh:zrrc': InstructionInfo('mul_sl_sh', 'mul_sl_sh:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- (ra[0:7] * rb[8:15]):S32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01001', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'logicalSetCondition'], True, False, {}),
	'mul_sl_sh:zrrci': InstructionInfo('mul_sl_sh', 'mul_sl_sh:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[8:15]):S32
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01001', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sl_sl.s:rrr': InstructionInfo('mul_sl_sl.s', 'mul_sl_sl.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[0:7] * rb[0:7]):S32
	dc = x:S64
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01000', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_sl_sl.s:rrrc': InstructionInfo('mul_sl_sl.s', 'mul_sl_sl.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[0:7] * rb[0:7]):S32
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01000', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_sl_sl.s:rrrci': InstructionInfo('mul_sl_sl.s', 'mul_sl_sl.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[0:7]):S32
	dc = x:S64
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01000', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sl_sl:rrr': InstructionInfo('mul_sl_sl', 'mul_sl_sl:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[0:7] * rb[0:7]):S32
	rc = x
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01000', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_sl_sl:rrrc': InstructionInfo('mul_sl_sl', 'mul_sl_sl:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[0:7] * rb[0:7]):S32
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01000', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_sl_sl:rrrci': InstructionInfo('mul_sl_sl', 'mul_sl_sl:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[0:7]):S32
	rc = x
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01000', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sl_sl:zrr': InstructionInfo('mul_sl_sl', 'mul_sl_sl:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- (ra[0:7] * rb[0:7]):S32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01000', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'falseCondition4'], True, False, {}),
	'mul_sl_sl:zrrc': InstructionInfo('mul_sl_sl', 'mul_sl_sl:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- (ra[0:7] * rb[0:7]):S32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01000', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'logicalSetCondition'], True, False, {}),
	'mul_sl_sl:zrrci': InstructionInfo('mul_sl_sl', 'mul_sl_sl:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[0:7]):S32
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA01000', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sl_uh.s:rrr': InstructionInfo('mul_sl_uh.s', 'mul_sl_uh.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[0:7] * rb[8:15]):S32
	dc = x:S64
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10001', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_sl_uh.s:rrrc': InstructionInfo('mul_sl_uh.s', 'mul_sl_uh.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[0:7] * rb[8:15]):S32
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10001', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_sl_uh.s:rrrci': InstructionInfo('mul_sl_uh.s', 'mul_sl_uh.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[8:15]):S32
	dc = x:S64
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10001', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sl_uh:rrr': InstructionInfo('mul_sl_uh', 'mul_sl_uh:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[0:7] * rb[8:15]):S32
	rc = x
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10001', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_sl_uh:rrrc': InstructionInfo('mul_sl_uh', 'mul_sl_uh:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[0:7] * rb[8:15]):S32
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10001', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_sl_uh:rrrci': InstructionInfo('mul_sl_uh', 'mul_sl_uh:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[8:15]):S32
	rc = x
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10001', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sl_uh:zrr': InstructionInfo('mul_sl_uh', 'mul_sl_uh:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- (ra[0:7] * rb[8:15]):S32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10001', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'falseCondition4'], True, False, {}),
	'mul_sl_uh:zrrc': InstructionInfo('mul_sl_uh', 'mul_sl_uh:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- (ra[0:7] * rb[8:15]):S32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10001', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'logicalSetCondition'], True, False, {}),
	'mul_sl_uh:zrrci': InstructionInfo('mul_sl_uh', 'mul_sl_uh:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[8:15]):S32
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10001', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sl_ul.s:rrr': InstructionInfo('mul_sl_ul.s', 'mul_sl_ul.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[0:7] * rb[0:7]):S32
	dc = x:S64
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10000', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_sl_ul.s:rrrc': InstructionInfo('mul_sl_ul.s', 'mul_sl_ul.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[0:7] * rb[0:7]):S32
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10000', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_sl_ul.s:rrrci': InstructionInfo('mul_sl_ul.s', 'mul_sl_ul.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[0:7]):S32
	dc = x:S64
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10000', 'rbArg', 'raAllEnabled', 'rcXSignedExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sl_ul:rrr': InstructionInfo('mul_sl_ul', 'mul_sl_ul:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[0:7] * rb[0:7]):S32
	rc = x
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10000', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_sl_ul:rrrc': InstructionInfo('mul_sl_ul', 'mul_sl_ul:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[0:7] * rb[0:7]):S32
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10000', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_sl_ul:rrrci': InstructionInfo('mul_sl_ul', 'mul_sl_ul:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[0:7]):S32
	rc = x
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10000', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_sl_ul:zrr': InstructionInfo('mul_sl_ul', 'mul_sl_ul:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- (ra[0:7] * rb[0:7]):S32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10000', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'falseCondition4'], True, False, {}),
	'mul_sl_ul:zrrc': InstructionInfo('mul_sl_ul', 'mul_sl_ul:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- (ra[0:7] * rb[0:7]):S32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10000', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'logicalSetCondition'], True, False, {}),
	'mul_sl_ul:zrrci': InstructionInfo('mul_sl_ul', 'mul_sl_ul:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[0:7]):S32
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA10000', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_step:rrri': InstructionInfo('mul_step', 'mul_step:rrri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('db', 64, False, False), Immediate('shift', 5, False)], '''	cc = x = dbe >> 1
	cc = (dbe & 0x1) - 1
	if (const_cc_zero cc) then
		dco = (dbo + (ra << shift))
	dce = x
	if (boot_cc cc) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'dbEnabled', 'subOpCode0100x', 'dcEnabled', 'raIsNotZero', 'bootCondition', 'immediate5', 'syntacticSugar'], True, True, {}),
	'mul_step:rrrici': InstructionInfo('mul_step', 'mul_step:rrrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('db', 64, False, False), Immediate('shift', 5, False), Condition('boot_cc'), PcSpec('pc', 16)], '''	cc = x = dbe >> 1
	cc = (dbe & 0x1) - 1
	if (const_cc_zero cc) then
		dco = (dbo + (ra << shift))
	dce = x
	if (boot_cc cc) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'dbEnabled', 'subOpCode0100x', 'dcEnabled', 'raIsNotZero', 'bootCondition', 'immediate5', 'targetPc'], True, False, {'mul_step:rrri': (0x0, 0xf00ffff)}),
	'mul_uh_uh.u:rrr': InstructionInfo('mul_uh_uh.u', 'mul_uh_uh.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[8:15] * rb[8:15]):U32
	dc = x:U64
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00111', 'rbArg', 'raAllEnabled', 'rcXUnsignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_uh_uh.u:rrrc': InstructionInfo('mul_uh_uh.u', 'mul_uh_uh.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[8:15] * rb[8:15]):U32
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00111', 'rbArg', 'raAllEnabled', 'rcXUnsignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_uh_uh.u:rrrci': InstructionInfo('mul_uh_uh.u', 'mul_uh_uh.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[8:15]):U32
	dc = x:U64
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00111', 'rbArg', 'raAllEnabled', 'rcXUnsignedExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_uh_uh:rrr': InstructionInfo('mul_uh_uh', 'mul_uh_uh:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[8:15] * rb[8:15]):U32
	rc = x
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00111', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_uh_uh:rrrc': InstructionInfo('mul_uh_uh', 'mul_uh_uh:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[8:15] * rb[8:15]):U32
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00111', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_uh_uh:rrrci': InstructionInfo('mul_uh_uh', 'mul_uh_uh:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[8:15]):U32
	rc = x
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00111', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_uh_uh:zrr': InstructionInfo('mul_uh_uh', 'mul_uh_uh:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- (ra[8:15] * rb[8:15]):U32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00111', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'falseCondition4'], True, False, {}),
	'mul_uh_uh:zrrc': InstructionInfo('mul_uh_uh', 'mul_uh_uh:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- (ra[8:15] * rb[8:15]):U32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00111', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'logicalSetCondition'], True, False, {}),
	'mul_uh_uh:zrrci': InstructionInfo('mul_uh_uh', 'mul_uh_uh:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[8:15]):U32
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00111', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_uh_ul.u:rrr': InstructionInfo('mul_uh_ul.u', 'mul_uh_ul.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[8:15] * rb[0:7]):U32
	dc = x:U64
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00110', 'rbArg', 'raAllEnabled', 'rcXUnsignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_uh_ul.u:rrrc': InstructionInfo('mul_uh_ul.u', 'mul_uh_ul.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[8:15] * rb[0:7]):U32
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00110', 'rbArg', 'raAllEnabled', 'rcXUnsignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_uh_ul.u:rrrci': InstructionInfo('mul_uh_ul.u', 'mul_uh_ul.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[0:7]):U32
	dc = x:U64
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00110', 'rbArg', 'raAllEnabled', 'rcXUnsignedExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_uh_ul:rrr': InstructionInfo('mul_uh_ul', 'mul_uh_ul:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[8:15] * rb[0:7]):U32
	rc = x
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00110', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_uh_ul:rrrc': InstructionInfo('mul_uh_ul', 'mul_uh_ul:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[8:15] * rb[0:7]):U32
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00110', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_uh_ul:rrrci': InstructionInfo('mul_uh_ul', 'mul_uh_ul:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[0:7]):U32
	rc = x
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00110', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_uh_ul:zrr': InstructionInfo('mul_uh_ul', 'mul_uh_ul:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- (ra[8:15] * rb[0:7]):U32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00110', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'falseCondition4'], True, False, {}),
	'mul_uh_ul:zrrc': InstructionInfo('mul_uh_ul', 'mul_uh_ul:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- (ra[8:15] * rb[0:7]):U32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00110', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'logicalSetCondition'], True, False, {}),
	'mul_uh_ul:zrrci': InstructionInfo('mul_uh_ul', 'mul_uh_ul:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[8:15] * rb[0:7]):U32
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00110', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_ul_uh.u:rrr': InstructionInfo('mul_ul_uh.u', 'mul_ul_uh.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[0:7] * rb[8:15]):U32
	dc = x:U64
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00001', 'rbArg', 'raAllEnabled', 'rcXUnsignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_ul_uh.u:rrrc': InstructionInfo('mul_ul_uh.u', 'mul_ul_uh.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[0:7] * rb[8:15]):U32
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00001', 'rbArg', 'raAllEnabled', 'rcXUnsignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_ul_uh.u:rrrci': InstructionInfo('mul_ul_uh.u', 'mul_ul_uh.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[8:15]):U32
	dc = x:U64
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00001', 'rbArg', 'raAllEnabled', 'rcXUnsignedExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_ul_uh:rrr': InstructionInfo('mul_ul_uh', 'mul_ul_uh:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[0:7] * rb[8:15]):U32
	rc = x
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00001', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_ul_uh:rrrc': InstructionInfo('mul_ul_uh', 'mul_ul_uh:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[0:7] * rb[8:15]):U32
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00001', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_ul_uh:rrrci': InstructionInfo('mul_ul_uh', 'mul_ul_uh:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[8:15]):U32
	rc = x
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00001', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_ul_uh:zrr': InstructionInfo('mul_ul_uh', 'mul_ul_uh:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- (ra[0:7] * rb[8:15]):U32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00001', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'falseCondition4'], True, False, {}),
	'mul_ul_uh:zrrc': InstructionInfo('mul_ul_uh', 'mul_ul_uh:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- (ra[0:7] * rb[8:15]):U32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00001', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'logicalSetCondition'], True, False, {}),
	'mul_ul_uh:zrrci': InstructionInfo('mul_ul_uh', 'mul_ul_uh:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[8:15]):U32
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00001', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_ul_ul.u:rrr': InstructionInfo('mul_ul_ul.u', 'mul_ul_ul.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[0:7] * rb[0:7]):U32
	dc = x:U64
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00000', 'rbArg', 'raAllEnabled', 'rcXUnsignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_ul_ul.u:rrrc': InstructionInfo('mul_ul_ul.u', 'mul_ul_ul.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[0:7] * rb[0:7]):U32
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00000', 'rbArg', 'raAllEnabled', 'rcXUnsignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_ul_ul.u:rrrci': InstructionInfo('mul_ul_ul.u', 'mul_ul_ul.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[0:7]):U32
	dc = x:U64
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00000', 'rbArg', 'raAllEnabled', 'rcXUnsignedExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_ul_ul:rrr': InstructionInfo('mul_ul_ul', 'mul_ul_ul:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = (ra[0:7] * rb[0:7]):U32
	rc = x
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00000', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'mul_ul_ul:rrrc': InstructionInfo('mul_ul_ul', 'mul_ul_ul:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = (ra[0:7] * rb[0:7]):U32
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00000', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'mul_ul_ul:rrrci': InstructionInfo('mul_ul_ul', 'mul_ul_ul:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[0:7]):U32
	rc = x
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00000', 'rbArg', 'raAllEnabled', 'rcXNotExtendedEnabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'mul_ul_ul:zrr': InstructionInfo('mul_ul_ul', 'mul_ul_ul:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- (ra[0:7] * rb[0:7]):U32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00000', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'falseCondition4'], True, False, {}),
	'mul_ul_ul:zrrc': InstructionInfo('mul_ul_ul', 'mul_ul_ul:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- (ra[0:7] * rb[0:7]):U32''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00000', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'logicalSetCondition'], True, False, {}),
	'mul_ul_ul:zrrci': InstructionInfo('mul_ul_ul', 'mul_ul_ul:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('mul_nz_cc'), PcSpec('pc', 16)], '''	x = (ra[0:7] * rb[0:7]):U32
	if (mul_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx00x', 'subA00000', 'rbArg', 'raAllEnabled', 'rcXDisabled', 'nzMulCondition', 'targetPc'], True, False, {}),
	'nand.s:rric': InstructionInfo('nand.s', 'nand.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ~(ra & imm)
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11111', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'nand.s:rrici': InstructionInfo('nand.s', 'nand.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra & imm)
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11111', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'nand.s:rrif': InstructionInfo('nand.s', 'nand.s:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ~(ra & imm)
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11111', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'nand.s:rrr': InstructionInfo('nand.s', 'nand.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ~(ra & rb)
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11111', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'nand.s:rrrc': InstructionInfo('nand.s', 'nand.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ~(ra & rb)
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11111', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'nand.s:rrrci': InstructionInfo('nand.s', 'nand.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra & rb)
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11111', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'nand.u:rric': InstructionInfo('nand.u', 'nand.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ~(ra & imm)
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11111', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'nand.u:rrici': InstructionInfo('nand.u', 'nand.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra & imm)
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11111', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'nand.u:rrif': InstructionInfo('nand.u', 'nand.u:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ~(ra & imm)
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11111', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'nand.u:rrr': InstructionInfo('nand.u', 'nand.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ~(ra & rb)
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11111', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'nand.u:rrrc': InstructionInfo('nand.u', 'nand.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ~(ra & rb)
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11111', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'nand.u:rrrci': InstructionInfo('nand.u', 'nand.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra & rb)
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11111', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'nand:rri': InstructionInfo('nand', 'nand:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True)], '''	x = ~(ra & imm)
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11111', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm', 'syntacticSugar'], True, True, {}),
	'nand:rric': InstructionInfo('nand', 'nand:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ~(ra & imm)
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11111', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'nand:rrici': InstructionInfo('nand', 'nand:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra & imm)
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11111', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'nand:rrif': InstructionInfo('nand', 'nand:rrif', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ~(ra & imm)
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11111', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {'nand:rri': (0, 0)}),
	'nand:rrr': InstructionInfo('nand', 'nand:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ~(ra & rb)
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11111', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'nand:rrrc': InstructionInfo('nand', 'nand:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ~(ra & rb)
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11111', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'nand:rrrci': InstructionInfo('nand', 'nand:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra & rb)
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11111', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'nand:zric': InstructionInfo('nand', 'nand:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('log_set_cc')], '''	ZF <- ~(ra & imm)''', ['opCode8Or9', 'rbDisabled', 'subOpCode11111', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition', 'immediate28imm'], True, False, {}),
	'nand:zrici': InstructionInfo('nand', 'nand:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 12, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra & imm)
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11111', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate28PcOpcode8', 'targetPc'], True, False, {}),
	'nand:zrif': InstructionInfo('nand', 'nand:zrif', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('false_cc')], '''	ZF <- ~(ra & imm)''', ['opCode8Or9', 'rbDisabled', 'subOpCode11111', 'rcDisabled', 'raAllEnabled', 'falseCondition4', 'immediate28imm'], True, False, {}),
	'nand:zrr': InstructionInfo('nand', 'nand:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- ~(ra & rb)''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11111', 'rcDisabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'nand:zrrc': InstructionInfo('nand', 'nand:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- ~(ra & rb)''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11111', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'nand:zrrci': InstructionInfo('nand', 'nand:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra & rb)
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11111', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'neg:rr': InstructionInfo('neg', 'neg:rr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False)], '''	rc = (imm - ra)
	ZF,CF <- rc''', ['opCode2Or3', 'rbDisabled', 'opCode2', 'rcEnabled', 'raAllEnabled', 'syntacticSugar'], True, True, {}),
	'neg:rrci': InstructionInfo('neg', 'neg:rrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = imm - ra
	rc = x
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode0100x', 'nzSubCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'nop:': InstructionInfo('nop', 'nop:', [], '''	instruction with no effect''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode00000', 'cstRc110000', 'raIsZero', 'falseCondition', 'subB00000', 'lsb16_0000000000000000'], True, False, {}),
	'nor.s:rric': InstructionInfo('nor.s', 'nor.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ~(ra | imm)
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11110', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'nor.s:rrici': InstructionInfo('nor.s', 'nor.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra | imm)
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11110', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'nor.s:rrif': InstructionInfo('nor.s', 'nor.s:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ~(ra | imm)
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11110', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'nor.s:rrr': InstructionInfo('nor.s', 'nor.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ~(ra | rb)
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11110', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'nor.s:rrrc': InstructionInfo('nor.s', 'nor.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ~(ra | rb)
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11110', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'nor.s:rrrci': InstructionInfo('nor.s', 'nor.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra | rb)
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11110', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'nor.u:rric': InstructionInfo('nor.u', 'nor.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ~(ra | imm)
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11110', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'nor.u:rrici': InstructionInfo('nor.u', 'nor.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra | imm)
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11110', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'nor.u:rrif': InstructionInfo('nor.u', 'nor.u:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ~(ra | imm)
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11110', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'nor.u:rrr': InstructionInfo('nor.u', 'nor.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ~(ra | rb)
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11110', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'nor.u:rrrc': InstructionInfo('nor.u', 'nor.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ~(ra | rb)
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11110', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'nor.u:rrrci': InstructionInfo('nor.u', 'nor.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra | rb)
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11110', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'nor:rri': InstructionInfo('nor', 'nor:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True)], '''	x = ~(ra | imm)
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11110', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm', 'syntacticSugar'], True, True, {}),
	'nor:rric': InstructionInfo('nor', 'nor:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ~(ra | imm)
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11110', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'nor:rrici': InstructionInfo('nor', 'nor:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra | imm)
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11110', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'nor:rrif': InstructionInfo('nor', 'nor:rrif', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ~(ra | imm)
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11110', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {'nor:rri': (0, 0)}),
	'nor:rrr': InstructionInfo('nor', 'nor:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ~(ra | rb)
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11110', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'nor:rrrc': InstructionInfo('nor', 'nor:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ~(ra | rb)
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11110', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'nor:rrrci': InstructionInfo('nor', 'nor:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra | rb)
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11110', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'nor:zric': InstructionInfo('nor', 'nor:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('log_set_cc')], '''	ZF <- ~(ra | imm)''', ['opCode8Or9', 'rbDisabled', 'subOpCode11110', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition', 'immediate28imm'], True, False, {}),
	'nor:zrici': InstructionInfo('nor', 'nor:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 12, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra | imm)
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11110', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate28PcOpcode8', 'targetPc'], True, False, {}),
	'nor:zrif': InstructionInfo('nor', 'nor:zrif', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('false_cc')], '''	ZF <- ~(ra | imm)''', ['opCode8Or9', 'rbDisabled', 'subOpCode11110', 'rcDisabled', 'raAllEnabled', 'falseCondition4', 'immediate28imm'], True, False, {}),
	'nor:zrr': InstructionInfo('nor', 'nor:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- ~(ra | rb)''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11110', 'rcDisabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'nor:zrrc': InstructionInfo('nor', 'nor:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- ~(ra | rb)''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11110', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'nor:zrrci': InstructionInfo('nor', 'nor:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra | rb)
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11110', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'not:rci': InstructionInfo('not', 'not:rci', [WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra ^ imm
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10000', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'not:rr': InstructionInfo('not', 'not:rr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False)], '''	rc = (ra ^ imm)
	ZF <- rc''', ['opCode4Or5', 'rbDisabled', 'opCode4', 'rcEnabled', 'raAllEnabled', 'syntacticSugar'], True, True, {}),
	'not:rrci': InstructionInfo('not', 'not:rrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra ^ imm
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10000', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc', 'syntacticSugar'], True, True, {}),
	'nxor.s:rric': InstructionInfo('nxor.s', 'nxor.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ~(ra ^ imm)
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'nxor.s:rrici': InstructionInfo('nxor.s', 'nxor.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra ^ imm)
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'nxor.s:rrif': InstructionInfo('nxor.s', 'nxor.s:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ~(ra ^ imm)
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'nxor.s:rrr': InstructionInfo('nxor.s', 'nxor.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ~(ra ^ rb)
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'nxor.s:rrrc': InstructionInfo('nxor.s', 'nxor.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ~(ra ^ rb)
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'nxor.s:rrrci': InstructionInfo('nxor.s', 'nxor.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra ^ rb)
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'nxor.u:rric': InstructionInfo('nxor.u', 'nxor.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ~(ra ^ imm)
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'nxor.u:rrici': InstructionInfo('nxor.u', 'nxor.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra ^ imm)
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'nxor.u:rrif': InstructionInfo('nxor.u', 'nxor.u:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ~(ra ^ imm)
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'nxor.u:rrr': InstructionInfo('nxor.u', 'nxor.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ~(ra ^ rb)
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'nxor.u:rrrc': InstructionInfo('nxor.u', 'nxor.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ~(ra ^ rb)
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'nxor.u:rrrci': InstructionInfo('nxor.u', 'nxor.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra ^ rb)
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'nxor:rri': InstructionInfo('nxor', 'nxor:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True)], '''	x = ~(ra ^ imm)
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10001', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm', 'syntacticSugar'], True, True, {}),
	'nxor:rric': InstructionInfo('nxor', 'nxor:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ~(ra ^ imm)
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10001', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'nxor:rrici': InstructionInfo('nxor', 'nxor:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra ^ imm)
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10001', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'nxor:rrif': InstructionInfo('nxor', 'nxor:rrif', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ~(ra ^ imm)
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10001', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {'nxor:rri': (0, 0)}),
	'nxor:rrr': InstructionInfo('nxor', 'nxor:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ~(ra ^ rb)
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10001', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'nxor:rrrc': InstructionInfo('nxor', 'nxor:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ~(ra ^ rb)
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10001', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'nxor:rrrci': InstructionInfo('nxor', 'nxor:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra ^ rb)
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10001', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'nxor:zric': InstructionInfo('nxor', 'nxor:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('log_set_cc')], '''	ZF <- ~(ra ^ imm)''', ['opCode8Or9', 'rbDisabled', 'subOpCode10001', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition', 'immediate28imm'], True, False, {}),
	'nxor:zrici': InstructionInfo('nxor', 'nxor:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 12, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra ^ imm)
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10001', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate28PcOpcode8', 'targetPc'], True, False, {}),
	'nxor:zrif': InstructionInfo('nxor', 'nxor:zrif', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('false_cc')], '''	ZF <- ~(ra ^ imm)''', ['opCode8Or9', 'rbDisabled', 'subOpCode10001', 'rcDisabled', 'raAllEnabled', 'falseCondition4', 'immediate28imm'], True, False, {}),
	'nxor:zrr': InstructionInfo('nxor', 'nxor:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- ~(ra ^ rb)''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10001', 'rcDisabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'nxor:zrrc': InstructionInfo('nxor', 'nxor:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- ~(ra ^ rb)''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10001', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'nxor:zrrci': InstructionInfo('nxor', 'nxor:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~(ra ^ rb)
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10001', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'or.s:rri': InstructionInfo('or.s', 'or.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), Immediate('imm', 32, False)], '''	dc = (rb | imm):S64
	ZF <- dc''', ['opCode4Or5', 'rbEnabled', 'rcSignedExtendedEnabled', 'raDisabled', 'immediate32DusRb'], True, False, {}),
	'or.s:rric': InstructionInfo('or.s', 'or.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ra | imm
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'or.s:rrici': InstructionInfo('or.s', 'or.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra | imm
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {'move.s:rrci': (0x0, 0xff0000), 'move.s:rici': (0x6000000000, 0x7c00000000)}),
	'or.s:rrif': InstructionInfo('or.s', 'or.s:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra | imm
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {'move.s:rr': (0x0, 0xffffff)}),
	'or.s:rrr': InstructionInfo('or.s', 'or.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra | rb
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10111', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'or.s:rrrc': InstructionInfo('or.s', 'or.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra | rb
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10111', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'or.s:rrrci': InstructionInfo('or.s', 'or.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra | rb
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10111', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'or.u:rri': InstructionInfo('or.u', 'or.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), Immediate('imm', 32, False)], '''	dc = (rb | imm):U64
	ZF <- dc''', ['opCode4Or5', 'rbEnabled', 'rcUnsignedExtendedEnabled', 'raDisabled', 'immediate32DusRb'], True, False, {}),
	'or.u:rric': InstructionInfo('or.u', 'or.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ra | imm
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'or.u:rrici': InstructionInfo('or.u', 'or.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra | imm
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {'move.u:rici': (0x6000000000, 0x7c00000000), 'move.u:rrci': (0x0, 0xff0000)}),
	'or.u:rrif': InstructionInfo('or.u', 'or.u:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra | imm
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {'move.u:rr': (0x0, 0xffffff)}),
	'or.u:rrr': InstructionInfo('or.u', 'or.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra | rb
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10111', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'or.u:rrrc': InstructionInfo('or.u', 'or.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra | rb
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10111', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'or.u:rrrci': InstructionInfo('or.u', 'or.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra | rb
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10111', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'or:rri': InstructionInfo('or', 'or:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 32, False)], '''	rc = (ra | imm)
	ZF <- rc''', ['opCode6', 'rbDisabled', 'raAllEnabled', 'immediate32', 'rcEnabled'], True, False, {'move:ri': (0x6000000000, 0x7c00000000)}),
	'or:rric': InstructionInfo('or', 'or:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ra | imm
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'or:rrici': InstructionInfo('or', 'or:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra | imm
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {'move:rrci': (0x0, 0xff0000), 'move:rici': (0x6000000000, 0x7c00000000)}),
	'or:rrif': InstructionInfo('or', 'or:rrif', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra | imm
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {'move:rr': (0x0, 0xffffff)}),
	'or:rrr': InstructionInfo('or', 'or:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra | rb
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10111', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'or:rrrc': InstructionInfo('or', 'or:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra | rb
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10111', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'or:rrrci': InstructionInfo('or', 'or:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra | rb
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10111', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'or:zri': InstructionInfo('or', 'or:zri', [ConstantRegister('zero'), WorkRegister('rb', 32, False, False), Immediate('imm', 32, False)], '''	ZF <- rb | imm''', ['opCode6', 'rbEnabled', 'raDisabled', 'immediate32ZeroRb', 'cstRc01100x'], True, False, {}),
	'or:zric': InstructionInfo('or', 'or:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('log_set_cc')], '''	ZF <- ra | imm''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition', 'immediate28imm'], True, False, {}),
	'or:zrici': InstructionInfo('or', 'or:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 12, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra | imm
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate28PcOpcode8', 'targetPc'], True, False, {}),
	'or:zrif': InstructionInfo('or', 'or:zrif', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('false_cc')], '''	ZF <- ra | imm''', ['opCode8Or9', 'rbDisabled', 'subOpCode10111', 'rcDisabled', 'raAllEnabled', 'falseCondition4', 'immediate28imm'], True, False, {}),
	'or:zrr': InstructionInfo('or', 'or:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- ra | rb''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10111', 'rcDisabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'or:zrrc': InstructionInfo('or', 'or:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- ra | rb''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10111', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'or:zrrci': InstructionInfo('or', 'or:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra | rb
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10111', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'orn.s:rric': InstructionInfo('orn.s', 'orn.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ~ra | imm
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'orn.s:rrici': InstructionInfo('orn.s', 'orn.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~ra | imm
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'orn.s:rrif': InstructionInfo('orn.s', 'orn.s:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ~ra | imm
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'orn.s:rrr': InstructionInfo('orn.s', 'orn.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ~ra | rb
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'orn.s:rrrc': InstructionInfo('orn.s', 'orn.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ~ra | rb
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'orn.s:rrrci': InstructionInfo('orn.s', 'orn.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~ra | rb
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11001', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'orn.u:rric': InstructionInfo('orn.u', 'orn.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ~ra | imm
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'orn.u:rrici': InstructionInfo('orn.u', 'orn.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~ra | imm
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'orn.u:rrif': InstructionInfo('orn.u', 'orn.u:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ~ra | imm
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'orn.u:rrr': InstructionInfo('orn.u', 'orn.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ~ra | rb
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'orn.u:rrrc': InstructionInfo('orn.u', 'orn.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ~ra | rb
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'orn.u:rrrci': InstructionInfo('orn.u', 'orn.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~ra | rb
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11001', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'orn:rri': InstructionInfo('orn', 'orn:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True)], '''	x = ~ra | imm
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11001', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm', 'syntacticSugar'], True, True, {}),
	'orn:rric': InstructionInfo('orn', 'orn:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ~ra | imm
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11001', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'orn:rrici': InstructionInfo('orn', 'orn:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~ra | imm
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11001', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'orn:rrif': InstructionInfo('orn', 'orn:rrif', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ~ra | imm
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11001', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {'orn:rri': (0, 0)}),
	'orn:rrr': InstructionInfo('orn', 'orn:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ~ra | rb
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11001', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'orn:rrrc': InstructionInfo('orn', 'orn:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ~ra | rb
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11001', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'orn:rrrci': InstructionInfo('orn', 'orn:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~ra | rb
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11001', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'orn:zric': InstructionInfo('orn', 'orn:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('log_set_cc')], '''	ZF <- ~ra | imm''', ['opCode8Or9', 'rbDisabled', 'subOpCode11001', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition', 'immediate28imm'], True, False, {}),
	'orn:zrici': InstructionInfo('orn', 'orn:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 12, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~ra | imm
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode11001', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate28PcOpcode8', 'targetPc'], True, False, {}),
	'orn:zrif': InstructionInfo('orn', 'orn:zrif', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('false_cc')], '''	ZF <- ~ra | imm''', ['opCode8Or9', 'rbDisabled', 'subOpCode11001', 'rcDisabled', 'raAllEnabled', 'falseCondition4', 'immediate28imm'], True, False, {}),
	'orn:zrr': InstructionInfo('orn', 'orn:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- ~ra | rb''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11001', 'rcDisabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'orn:zrrc': InstructionInfo('orn', 'orn:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- ~ra | rb''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11001', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'orn:zrrci': InstructionInfo('orn', 'orn:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ~ra | rb
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA11001', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'read_run:rici': InstructionInfo('read_run', 'read_run:rici', [WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('boot_cc'), PcSpec('pc', 16)], '''	x = read RB((ra + imm)[8:15] ^ (ra + imm)[0:7])
	if (boot_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0011x', 'cstRc110000', 'subOpCode00111', 'raAllEnabled', 'bootCondition', 'immediate24Pc', 'targetPc'], False, False, {}),
	'release:rici': InstructionInfo('release', 'release:rici', [WorkRegister('ra', 32, True, False), Immediate('imm', 16, True), Condition('release_cc'), PcSpec('pc', 16)], '''	cc = (ZF <- release((ra + imm)[8:15] ^ (ra + imm)[0:7]))
	if (release_cc cc) then
		jump @[pc]''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode1xxxx', 'raAllEnabled', 'immediate16Atomic', 'targetPc', 'releaseCondition'], True, False, {}),
	'resume:ri': InstructionInfo('resume', 'resume:ri', [WorkRegister('ra', 32, True, False), Immediate('imm', 8, True)], '''	x = resume @[(ra + imm)[8:15] ^ (ra + imm)[0:7]]
	if (boot_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0011x', 'cstRc11x110', 'raAllEnabled', 'bootCondition', 'immediate24Pc', 'syntacticSugar'], True, True, {}),
	'resume:rici': InstructionInfo('resume', 'resume:rici', [WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('boot_cc'), PcSpec('pc', 16)], '''	x = resume @[(ra + imm)[8:15] ^ (ra + imm)[0:7]]
	if (boot_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0011x', 'cstRc11x110', 'raAllEnabled', 'bootCondition', 'immediate24Pc', 'targetPc'], True, False, {'resume:ri': (0x0, 0xf00ffff)}),
	'rol.s:rri': InstructionInfo('rol.s', 'rol.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra <<r shift[0:4]
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB00110'], True, False, {}),
	'rol.s:rric': InstructionInfo('rol.s', 'rol.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra <<r shift[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB00110'], True, False, {}),
	'rol.s:rrici': InstructionInfo('rol.s', 'rol.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra <<r shift[0:4]
	dc = x:S64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB00110', 'targetPc'], True, False, {}),
	'rol.s:rrr': InstructionInfo('rol.s', 'rol.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra <<r rb[0:4]
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA00110', 'falseConditionShift5'], True, False, {}),
	'rol.s:rrrc': InstructionInfo('rol.s', 'rol.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra <<r rb[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA00110', 'shiftSetCondition'], True, False, {}),
	'rol.s:rrrci': InstructionInfo('rol.s', 'rol.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra <<r rb[0:4]
	dc = x:S64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA00110', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'rol.u:rri': InstructionInfo('rol.u', 'rol.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra <<r shift[0:4]
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB00110'], True, False, {}),
	'rol.u:rric': InstructionInfo('rol.u', 'rol.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra <<r shift[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB00110'], True, False, {}),
	'rol.u:rrici': InstructionInfo('rol.u', 'rol.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra <<r shift[0:4]
	dc = x:U64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB00110', 'targetPc'], True, False, {}),
	'rol.u:rrr': InstructionInfo('rol.u', 'rol.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra <<r rb[0:4]
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA00110', 'falseConditionShift5'], True, False, {}),
	'rol.u:rrrc': InstructionInfo('rol.u', 'rol.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra <<r rb[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA00110', 'shiftSetCondition'], True, False, {}),
	'rol.u:rrrci': InstructionInfo('rol.u', 'rol.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra <<r rb[0:4]
	dc = x:U64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA00110', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'rol:rri': InstructionInfo('rol', 'rol:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra <<r shift[0:4]
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB00110'], True, False, {}),
	'rol:rric': InstructionInfo('rol', 'rol:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra <<r shift[0:4]
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB00110'], True, False, {}),
	'rol:rrici': InstructionInfo('rol', 'rol:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra <<r shift[0:4]
	rc = x
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB00110', 'targetPc'], True, False, {}),
	'rol:rrr': InstructionInfo('rol', 'rol:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra <<r rb[0:4]
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA00110', 'falseConditionShift5'], True, False, {}),
	'rol:rrrc': InstructionInfo('rol', 'rol:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra <<r rb[0:4]
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA00110', 'shiftSetCondition'], True, False, {}),
	'rol:rrrci': InstructionInfo('rol', 'rol:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra <<r rb[0:4]
	rc = x
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA00110', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'rol:zri': InstructionInfo('rol', 'rol:zri', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra <<r shift[0:4]''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB00110'], True, False, {}),
	'rol:zric': InstructionInfo('rol', 'rol:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	ZF <- ra <<r shift[0:4]''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB00110'], True, False, {}),
	'rol:zrici': InstructionInfo('rol', 'rol:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra <<r shift[0:4]
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB00110', 'targetPc'], True, False, {}),
	'rol:zrr': InstructionInfo('rol', 'rol:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- ra <<r rb[0:4]''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA00110', 'falseConditionShift5'], True, False, {}),
	'rol:zrrc': InstructionInfo('rol', 'rol:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- ra <<r rb[0:4]''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA00110', 'shiftSetCondition'], True, False, {}),
	'rol:zrrci': InstructionInfo('rol', 'rol:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra <<r rb[0:4]
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA00110', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'rol_add.s:rrri': InstructionInfo('rol_add.s', 'rol_add.s:rrri', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra <<r shift
	x = rb + (ra <<r shift[0:4])
	dc = x:S64''', ['opCode8Or9', 'rbEnabled', 'subOpCode0000x', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate5'], True, False, {}),
	'rol_add.s:rrrici': InstructionInfo('rol_add.s', 'rol_add.s:rrrici', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('div_nz_cc'), PcSpec('pc', 16)], '''	ZF <- ra <<r shift
	x = rb + (ra <<r shift[0:4])
	dc = x:S64
	if (div_nz_cc x) then
		jump @[pc]''', ['opCode8Or9', 'rbEnabled', 'subOpCode0000x', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzDivCondition', 'immediate5', 'targetPc'], True, False, {}),
	'rol_add.u:rrri': InstructionInfo('rol_add.u', 'rol_add.u:rrri', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra <<r shift
	x = rb + (ra <<r shift[0:4])
	dc = x:U64''', ['opCode8Or9', 'rbEnabled', 'subOpCode0000x', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate5'], True, False, {}),
	'rol_add.u:rrrici': InstructionInfo('rol_add.u', 'rol_add.u:rrrici', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('div_nz_cc'), PcSpec('pc', 16)], '''	ZF <- ra <<r shift
	x = rb + (ra <<r shift[0:4])
	dc = x:U64
	if (div_nz_cc x) then
		jump @[pc]''', ['opCode8Or9', 'rbEnabled', 'subOpCode0000x', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzDivCondition', 'immediate5', 'targetPc'], True, False, {}),
	'rol_add:rrri': InstructionInfo('rol_add', 'rol_add:rrri', [WorkRegister('rc', 32, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra <<r shift
	x = rb + (ra <<r shift[0:4])
	rc = x''', ['opCode8Or9', 'rbEnabled', 'subOpCode0000x', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate5'], True, False, {}),
	'rol_add:rrrici': InstructionInfo('rol_add', 'rol_add:rrrici', [WorkRegister('rc', 32, False, False), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('div_nz_cc'), PcSpec('pc', 16)], '''	ZF <- ra <<r shift
	x = rb + (ra <<r shift[0:4])
	rc = x
	if (div_nz_cc x) then
		jump @[pc]''', ['opCode8Or9', 'rbEnabled', 'subOpCode0000x', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzDivCondition', 'immediate5', 'targetPc'], True, False, {}),
	'rol_add:zrri': InstructionInfo('rol_add', 'rol_add:zrri', [ConstantRegister('zero'), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	instruction with no effect''', ['opCode8Or9', 'rbEnabled', 'subOpCode0000x', 'rcDisabled', 'raAllEnabled', 'falseCondition4', 'immediate5'], True, False, {}),
	'rol_add:zrrici': InstructionInfo('rol_add', 'rol_add:zrrici', [ConstantRegister('zero'), WorkRegister('rb', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('div_nz_cc'), PcSpec('pc', 16)], '''	ZF <- ra <<r shift
	x = rb + (ra <<r shift[0:4])
	if (div_nz_cc x) then
		jump @[pc]''', ['opCode8Or9', 'rbEnabled', 'subOpCode0000x', 'rcDisabled', 'raAllEnabled', 'nzDivCondition', 'immediate5', 'targetPc'], True, False, {}),
	'ror.s:rri': InstructionInfo('ror.s', 'ror.s:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra >>r shift[0:4]
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB10110'], True, False, {}),
	'ror.s:rric': InstructionInfo('ror.s', 'ror.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra >>r shift[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB10110'], True, False, {}),
	'ror.s:rrici': InstructionInfo('ror.s', 'ror.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >>r shift[0:4]
	dc = x:S64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcSignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB10110', 'targetPc'], True, False, {}),
	'ror.s:rrr': InstructionInfo('ror.s', 'ror.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra >>r rb[0:4]
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA10110', 'falseConditionShift5'], True, False, {}),
	'ror.s:rrrc': InstructionInfo('ror.s', 'ror.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra >>r rb[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA10110', 'shiftSetCondition'], True, False, {}),
	'ror.s:rrrci': InstructionInfo('ror.s', 'ror.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >>r rb[0:4]
	dc = x:S64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcSignedExtendedEnabled', 'raAllEnabled', 'subA10110', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'ror.u:rri': InstructionInfo('ror.u', 'ror.u:rri', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra >>r shift[0:4]
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB10110'], True, False, {}),
	'ror.u:rric': InstructionInfo('ror.u', 'ror.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra >>r shift[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB10110'], True, False, {}),
	'ror.u:rrici': InstructionInfo('ror.u', 'ror.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >>r shift[0:4]
	dc = x:U64
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB10110', 'targetPc'], True, False, {}),
	'ror.u:rrr': InstructionInfo('ror.u', 'ror.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra >>r rb[0:4]
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA10110', 'falseConditionShift5'], True, False, {}),
	'ror.u:rrrc': InstructionInfo('ror.u', 'ror.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra >>r rb[0:4]
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA10110', 'shiftSetCondition'], True, False, {}),
	'ror.u:rrrci': InstructionInfo('ror.u', 'ror.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >>r rb[0:4]
	dc = x:U64
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'subA10110', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'ror:rri': InstructionInfo('ror', 'ror:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	x = ra >>r shift[0:4]
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB10110'], True, False, {}),
	'ror:rric': InstructionInfo('ror', 'ror:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	x = ra >>r shift[0:4]
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB10110'], True, False, {}),
	'ror:rrici': InstructionInfo('ror', 'ror:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >>r shift[0:4]
	rc = x
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcNotExtendedEnabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB10110', 'targetPc'], True, False, {}),
	'ror:rrr': InstructionInfo('ror', 'ror:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra >>r rb[0:4]
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA10110', 'falseConditionShift5'], True, False, {}),
	'ror:rrrc': InstructionInfo('ror', 'ror:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra >>r rb[0:4]
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA10110', 'shiftSetCondition'], True, False, {}),
	'ror:rrrci': InstructionInfo('ror', 'ror:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >>r rb[0:4]
	rc = x
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcNotExtendedEnabled', 'raAllEnabled', 'subA10110', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'ror:zri': InstructionInfo('ror', 'ror:zri', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False)], '''	ZF <- ra >>r shift[0:4]''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'falseConditionShift5', 'immediate5', 'subB10110'], True, False, {}),
	'ror:zric': InstructionInfo('ror', 'ror:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('log_set_cc')], '''	ZF <- ra >>r shift[0:4]''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'shiftSetCondition', 'immediate5', 'subB10110'], True, False, {}),
	'ror:zrici': InstructionInfo('ror', 'ror:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('shift', 5, False), Condition('imm_shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >>r shift[0:4]
	if (imm_shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode01xxx', 'rcDisabled', 'raAllEnabled', 'immNzShiftCondition', 'immediate5', 'subB10110', 'targetPc'], True, False, {}),
	'ror:zrr': InstructionInfo('ror', 'ror:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- ra >>r rb[0:4]''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA10110', 'falseConditionShift5'], True, False, {}),
	'ror:zrrc': InstructionInfo('ror', 'ror:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- ra >>r rb[0:4]''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA10110', 'shiftSetCondition'], True, False, {}),
	'ror:zrrci': InstructionInfo('ror', 'ror:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('shift_nz_cc'), PcSpec('pc', 16)], '''	x = ra >>r rb[0:4]
	if (shift_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11xx1', 'rcDisabled', 'raAllEnabled', 'subA10110', 'nzShiftCondition', 'targetPc'], True, False, {}),
	'rsub.s:rrr': InstructionInfo('rsub.s', 'rsub.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = rb - ra
	dc = x:S64
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode0100x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition5'], True, False, {}),
	'rsub.s:rrrc': InstructionInfo('rsub.s', 'rsub.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_set_cc')], '''	x = rb - ra
	if (sub_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode0100x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'subSetCondition'], True, False, {}),
	'rsub.s:rrrci': InstructionInfo('rsub.s', 'rsub.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = rb - ra
	dc = x:S64
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode0100x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubCondition', 'targetPc'], True, False, {}),
	'rsub.u:rrr': InstructionInfo('rsub.u', 'rsub.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = rb - ra
	dc = x:U64
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode0100x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition5'], True, False, {}),
	'rsub.u:rrrc': InstructionInfo('rsub.u', 'rsub.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_set_cc')], '''	x = rb - ra
	if (sub_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode0100x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'subSetCondition'], True, False, {}),
	'rsub.u:rrrci': InstructionInfo('rsub.u', 'rsub.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = rb - ra
	dc = x:U64
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode0100x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubCondition', 'targetPc'], True, False, {}),
	'rsub:rrr': InstructionInfo('rsub', 'rsub:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = rb - ra
	rc = x
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode0100x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition5'], True, False, {}),
	'rsub:rrrc': InstructionInfo('rsub', 'rsub:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_set_cc')], '''	x = rb - ra
	if (sub_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode0100x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'subSetCondition'], True, False, {}),
	'rsub:rrrci': InstructionInfo('rsub', 'rsub:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = rb - ra
	rc = x
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode0100x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubCondition', 'targetPc'], True, False, {}),
	'rsub:zrr': InstructionInfo('rsub', 'rsub:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF,CF <- rb - ra''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode0100x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition5'], True, False, {}),
	'rsub:zrrc': InstructionInfo('rsub', 'rsub:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_set_cc')], '''	ZF,CF <- rb - ra''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode0100x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'subSetCondition'], True, False, {}),
	'rsub:zrrci': InstructionInfo('rsub', 'rsub:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = rb - ra
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode0100x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubCondition', 'targetPc'], True, False, {}),
	'rsubc.s:rrr': InstructionInfo('rsubc.s', 'rsubc.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = rb + ~ra + CF
	dc = x:S64
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode0111x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition5'], True, False, {}),
	'rsubc.s:rrrc': InstructionInfo('rsubc.s', 'rsubc.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_set_cc')], '''	x = rb + ~ra + CF
	if (sub_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode0111x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'subSetCondition'], True, False, {}),
	'rsubc.s:rrrci': InstructionInfo('rsubc.s', 'rsubc.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = rb + ~ra + CF
	dc = x:S64
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode0111x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubCondition', 'targetPc'], True, False, {}),
	'rsubc.u:rrr': InstructionInfo('rsubc.u', 'rsubc.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = rb + ~ra + CF
	dc = x:U64
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode0111x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition5'], True, False, {}),
	'rsubc.u:rrrc': InstructionInfo('rsubc.u', 'rsubc.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_set_cc')], '''	x = rb + ~ra + CF
	if (sub_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode0111x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'subSetCondition'], True, False, {}),
	'rsubc.u:rrrci': InstructionInfo('rsubc.u', 'rsubc.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = rb + ~ra + CF
	dc = x:U64
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode0111x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubCondition', 'targetPc'], True, False, {}),
	'rsubc:rrr': InstructionInfo('rsubc', 'rsubc:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = rb + ~ra + CF
	rc = x
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode0111x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition5'], True, False, {}),
	'rsubc:rrrc': InstructionInfo('rsubc', 'rsubc:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_set_cc')], '''	x = rb + ~ra + CF
	if (sub_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode0111x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'subSetCondition'], True, False, {}),
	'rsubc:rrrci': InstructionInfo('rsubc', 'rsubc:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = rb + ~ra + CF
	rc = x
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode0111x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubCondition', 'targetPc'], True, False, {}),
	'rsubc:zrr': InstructionInfo('rsubc', 'rsubc:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF,CF <- rb + ~ra + CF''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode0111x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition5'], True, False, {}),
	'rsubc:zrrc': InstructionInfo('rsubc', 'rsubc:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_set_cc')], '''	ZF,CF <- rb + ~ra + CF''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode0111x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'subSetCondition'], True, False, {}),
	'rsubc:zrrci': InstructionInfo('rsubc', 'rsubc:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = rb + ~ra + CF
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode0111x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubCondition', 'targetPc'], True, False, {}),
	'sats.s:rr': InstructionInfo('sats.s', 'sats.s:rr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False)], '''	x = sats(ra)
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00110', 'raAllEnabled', 'subA00xxx', 'rcSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'sats.s:rrc': InstructionInfo('sats.s', 'sats.s:rrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = sats(ra)
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00110', 'raAllEnabled', 'subA00xxx', 'rcSignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'sats.s:rrci': InstructionInfo('sats.s', 'sats.s:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = sats(ra)
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00110', 'raAllEnabled', 'subA00xxx', 'rcSignedExtendedEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'sats.u:rr': InstructionInfo('sats.u', 'sats.u:rr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False)], '''	x = sats(ra)
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00110', 'raAllEnabled', 'subA00xxx', 'rcUnsignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'sats.u:rrc': InstructionInfo('sats.u', 'sats.u:rrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = sats(ra)
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00110', 'raAllEnabled', 'subA00xxx', 'rcUnsignedExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'sats.u:rrci': InstructionInfo('sats.u', 'sats.u:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = sats(ra)
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00110', 'raAllEnabled', 'subA00xxx', 'rcUnsignedExtendedEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'sats:rr': InstructionInfo('sats', 'sats:rr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False)], '''	x = sats(ra)
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00110', 'raAllEnabled', 'subA00xxx', 'rcNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'sats:rrc': InstructionInfo('sats', 'sats:rrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	x = sats(ra)
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00110', 'raAllEnabled', 'subA00xxx', 'rcNotExtendedEnabled', 'logicalSetCondition'], True, False, {}),
	'sats:rrci': InstructionInfo('sats', 'sats:rrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = sats(ra)
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00110', 'raAllEnabled', 'subA00xxx', 'rcNotExtendedEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'sats:zr': InstructionInfo('sats', 'sats:zr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False)], '''	ZF <- sats(ra)''', ['opCode8Or9', 'rbDisabled', 'subOpCode00110', 'raAllEnabled', 'subA00xxx', 'rcDisabled', 'falseCondition4'], True, False, {}),
	'sats:zrc': InstructionInfo('sats', 'sats:zrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('log_set_cc')], '''	ZF <- sats(ra)''', ['opCode8Or9', 'rbDisabled', 'subOpCode00110', 'raAllEnabled', 'subA00xxx', 'rcDisabled', 'logicalSetCondition'], True, False, {}),
	'sats:zrci': InstructionInfo('sats', 'sats:zrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = sats(ra)
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode00110', 'raAllEnabled', 'subA00xxx', 'rcDisabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'sb:erii': InstructionInfo('sb', 'sb:erii', [Endianess('endian'), WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 8, True)], '''	let @a = (ra + off)
	Store imm:8 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0100x', 'loadStoreUnsafeRegister', 'raAllEnabled', 'endianess', 'immediate12Store', 'subAuxx000x', 'immediate8Store'], True, False, {'sb:rii': (0x0, 0x8000000)}),
	'sb:erir': InstructionInfo('sb', 'sb:erir', [Endianess('endian'), WorkRegister('ra', 32, True, False), Immediate('off', 24, True), WorkRegister('rb', 32, False, False)], '''	let @a = (ra + off)
	Store rb:8 into WRAM at address @a with endianness endian''', ['opCode7', 'rbOrDbExists', 'rcDisabled', 'rbOrDbIsRbArg', 'raAllEnabled', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'endianess', 'immediate24WithRb', 'subAuxx000x'], True, False, {'sb:rir': (0x0, 0x8000000)}),
	'sb:esii': InstructionInfo('sb', 'sb:esii', [Endianess('endian'), WorkRegister('sa', 32, True, True), Immediate('off', 12, True), Immediate('imm', 8, True)], '''	cc = (ra & 0xffff) + off - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xffff
		Store imm:8 into WRAM at address @a with endianness endian
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		Store imm:8 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0100x', 'loadStoreSafeRegister', 'raAllEnabled', 'endianess', 'immediate12Store', 'subAuxx000x', 'immediate8Store'], True, False, {'sbs:rii': (0x0, 0x8000000)}),
	'sb:esir': InstructionInfo('sb', 'sb:esir', [Endianess('endian'), WorkRegister('sa', 32, True, True), Immediate('off', 24, True), WorkRegister('rb', 32, False, False)], '''	cc = (ra & 0xffff) + off - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xffff
		Store rb:8 into WRAM at address @a with endianness endian
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		Store rb:8 into WRAM at address @a with endianness endian''', ['opCode7', 'rbOrDbExists', 'rcDisabled', 'rbOrDbIsRbArg', 'raAllEnabled', 'subOpCode01xxx', 'loadStoreSafeRegister', 'endianess', 'immediate24WithRb', 'subAuxx000x'], True, False, {'sbs:rir': (0x0, 0x8000000)}),
	'sb:rii': InstructionInfo('sb', 'sb:rii', [WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 8, True)], '''	let @a = (ra + off)
	Store imm:8 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0100x', 'loadStoreUnsafeRegister', 'raAllEnabled', 'immediate12Store', 'subAuxx000x', 'immediate8Store', 'syntacticSugar'], True, True, {}),
	'sb:rir': InstructionInfo('sb', 'sb:rir', [WorkRegister('ra', 32, True, False), Immediate('off', 24, True), WorkRegister('rb', 32, False, False)], '''	let @a = (ra + off)
	Store rb:8 into WRAM at address @a with endianness endian''', ['opCode7', 'rbOrDbExists', 'rcDisabled', 'rbOrDbIsRbArg', 'raAllEnabled', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'immediate24WithRb', 'subAuxx000x', 'syntacticSugar'], True, True, {}),
	'sb_id:erii': InstructionInfo('sb_id', 'sb_id:erii', [Endianess('endian'), WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 8, True)], '''	let @a = (ra + off)
	Store (id | imm):8 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0111x', 'raAllEnabled', 'endianess', 'immediate12Store', 'subAuxx000x', 'immediate8Store'], True, False, {'sb_id:rii': (0x0, 0x8000000), 'sb_id:ri': (0x0, 0x80f000f)}),
	'sb_id:ri': InstructionInfo('sb_id', 'sb_id:ri', [WorkRegister('ra', 32, True, False), Immediate('off', 12, True)], '''	let @a = (ra + off)
	Store (id | imm):8 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0111x', 'raAllEnabled', 'immediate12Store', 'subAuxx000x', 'syntacticSugar'], True, True, {}),
	'sb_id:rii': InstructionInfo('sb_id', 'sb_id:rii', [WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 8, True)], '''	let @a = (ra + off)
	Store (id | imm):8 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0111x', 'raAllEnabled', 'immediate12Store', 'subAuxx000x', 'immediate8Store', 'syntacticSugar'], True, True, {}),
	'sbs:rii': InstructionInfo('sbs', 'sbs:rii', [WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 8, True)], '''	cc = (ra & 0xffff) + off - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xffff
		Store imm:8 into WRAM at address @a with endianness endian
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		Store imm:8 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0100x', 'loadStoreSafeRegister', 'raAllEnabled', 'immediate12Store', 'subAuxx000x', 'immediate8Store', 'syntacticSugar'], True, True, {}),
	'sbs:rir': InstructionInfo('sbs', 'sbs:rir', [WorkRegister('ra', 32, True, False), Immediate('off', 24, True), WorkRegister('rb', 32, False, False)], '''	cc = (ra & 0xffff) + off - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xffff
		Store rb:8 into WRAM at address @a with endianness endian
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		Store rb:8 into WRAM at address @a with endianness endian''', ['opCode7', 'rbOrDbExists', 'rcDisabled', 'rbOrDbIsRbArg', 'raAllEnabled', 'subOpCode01xxx', 'loadStoreSafeRegister', 'immediate24WithRb', 'subAuxx000x', 'syntacticSugar'], True, True, {}),
	'sd:erii': InstructionInfo('sd', 'sd:erii', [Endianess('endian'), WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	let @a = (ra + off)
	Store imm:S64:64 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0100x', 'loadStoreUnsafeRegister', 'raAllEnabled', 'endianess', 'immediate12Store', 'subAuxx111x', 'immediate16Store'], True, False, {'sd:rii': (0x0, 0x8000000)}),
	'sd:erir': InstructionInfo('sd', 'sd:erir', [Endianess('endian'), WorkRegister('ra', 32, True, False), Immediate('off', 24, True), WorkRegister('db', 64, False, False)], '''	let @a = (ra + off)
	Store db:64 into WRAM at address @a with endianness endian''', ['opCode7', 'rbOrDbExists', 'rcDisabled', 'rbOrDbIsDbArg', 'raAllEnabled', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'endianess', 'immediate24WithRb', 'subAuxx111x'], True, False, {'sd:rir': (0x0, 0x8000000)}),
	'sd:esii': InstructionInfo('sd', 'sd:esii', [Endianess('endian'), WorkRegister('sa', 32, True, True), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	cc = (ra & 0xffff) + off + 8 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfff8
		Store imm:S64:64 into WRAM at address @a with endianness endian
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		Store imm:S64:64 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0100x', 'loadStoreSafeRegister', 'raAllEnabled', 'endianess', 'immediate12Store', 'subAuxx111x', 'immediate16Store'], True, False, {'sds:rii': (0x0, 0x8000000)}),
	'sd:esir': InstructionInfo('sd', 'sd:esir', [Endianess('endian'), WorkRegister('sa', 32, True, True), Immediate('off', 24, True), WorkRegister('db', 64, False, False)], '''	cc = (ra & 0xffff) + off + 8 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfff8
		Store db:64 into WRAM at address @a with endianness endian
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		Store db:64 into WRAM at address @a with endianness endian''', ['opCode7', 'rbOrDbExists', 'rcDisabled', 'rbOrDbIsDbArg', 'raAllEnabled', 'subOpCode01xxx', 'loadStoreSafeRegister', 'endianess', 'immediate24WithRb', 'subAuxx111x'], True, False, {'sds:rir': (0x0, 0x8000000)}),
	'sd:rii': InstructionInfo('sd', 'sd:rii', [WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	let @a = (ra + off)
	Store imm:S64:64 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0100x', 'loadStoreUnsafeRegister', 'raAllEnabled', 'immediate12Store', 'subAuxx111x', 'immediate16Store', 'syntacticSugar'], True, True, {}),
	'sd:rir': InstructionInfo('sd', 'sd:rir', [WorkRegister('ra', 32, True, False), Immediate('off', 24, True), WorkRegister('db', 64, False, False)], '''	let @a = (ra + off)
	Store db:64 into WRAM at address @a with endianness endian''', ['opCode7', 'rbOrDbExists', 'rcDisabled', 'rbOrDbIsDbArg', 'raAllEnabled', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'immediate24WithRb', 'subAuxx111x', 'syntacticSugar'], True, True, {}),
	'sd_id:erii': InstructionInfo('sd_id', 'sd_id:erii', [Endianess('endian'), WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	let @a = (ra + off)
	Store (id | imm):S64:64 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0111x', 'raAllEnabled', 'endianess', 'immediate12Store', 'subAuxx111x', 'immediate16Store'], True, False, {'sd_id:rii': (0x0, 0x8000000), 'sd_id:ri': (0x0, 0x80f0fff)}),
	'sd_id:ri': InstructionInfo('sd_id', 'sd_id:ri', [WorkRegister('ra', 32, True, False), Immediate('off', 12, True)], '''	let @a = (ra + off)
	Store (id | imm):S64:64 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0111x', 'raAllEnabled', 'immediate12Store', 'subAuxx111x', 'syntacticSugar'], True, True, {}),
	'sd_id:rii': InstructionInfo('sd_id', 'sd_id:rii', [WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	let @a = (ra + off)
	Store (id | imm):S64:64 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0111x', 'raAllEnabled', 'immediate12Store', 'subAuxx111x', 'immediate16Store', 'syntacticSugar'], True, True, {}),
	'sdma:rri': InstructionInfo('sdma', 'sdma:rri', [WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Immediate('immDma', 8, False)], '''	let @w = (ra & 0xfffff8)
	let @m = (rb & 0xfffffff8)
	let N = (1 + (immDma:U32 + (ra >> 24) & 0xff) & 0xff) << 3
	Store N bytes from WRAM at address @w into MRAM at address @m''', ['opCode7', 'rbOrDbExists', 'cstRc000000', 'rbArg', 'raAllEnabled', 'immediate8Dma', 'lsb16_0000000000000010'], True, False, {}),
	'sds:rii': InstructionInfo('sds', 'sds:rii', [WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	cc = (ra & 0xffff) + off + 8 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfff8
		Store imm:S64:64 into WRAM at address @a with endianness endian
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		Store imm:S64:64 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0100x', 'loadStoreSafeRegister', 'raAllEnabled', 'immediate12Store', 'subAuxx111x', 'immediate16Store', 'syntacticSugar'], True, True, {}),
	'sds:rir': InstructionInfo('sds', 'sds:rir', [WorkRegister('ra', 32, True, False), Immediate('off', 24, True), WorkRegister('db', 64, False, False)], '''	cc = (ra & 0xffff) + off + 8 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfff8
		Store db:64 into WRAM at address @a with endianness endian
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		Store db:64 into WRAM at address @a with endianness endian''', ['opCode7', 'rbOrDbExists', 'rcDisabled', 'rbOrDbIsDbArg', 'raAllEnabled', 'subOpCode01xxx', 'loadStoreSafeRegister', 'immediate24WithRb', 'subAuxx111x', 'syntacticSugar'], True, True, {}),
	'sh:erii': InstructionInfo('sh', 'sh:erii', [Endianess('endian'), WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	let @a = (ra + off)
	Store imm:16 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0100x', 'loadStoreUnsafeRegister', 'raAllEnabled', 'endianess', 'immediate12Store', 'subAuxx011x', 'immediate16Store'], True, False, {'sh:rii': (0x0, 0x8000000)}),
	'sh:erir': InstructionInfo('sh', 'sh:erir', [Endianess('endian'), WorkRegister('ra', 32, True, False), Immediate('off', 24, True), WorkRegister('rb', 32, False, False)], '''	let @a = (ra + off)
	Store rb:16 into WRAM at address @a with endianness endian''', ['opCode7', 'rbOrDbExists', 'rcDisabled', 'rbOrDbIsRbArg', 'raAllEnabled', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'endianess', 'immediate24WithRb', 'subAuxx011x'], True, False, {'sh:rir': (0x0, 0x8000000)}),
	'sh:esii': InstructionInfo('sh', 'sh:esii', [Endianess('endian'), WorkRegister('sa', 32, True, True), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	cc = (ra & 0xffff) + off + 2 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfffe
		Store imm:16 into WRAM at address @a with endianness endian
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		Store imm:16 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0100x', 'loadStoreSafeRegister', 'raAllEnabled', 'endianess', 'immediate12Store', 'subAuxx011x', 'immediate16Store'], True, False, {'shs:rii': (0x0, 0x8000000)}),
	'sh:esir': InstructionInfo('sh', 'sh:esir', [Endianess('endian'), WorkRegister('sa', 32, True, True), Immediate('off', 24, True), WorkRegister('rb', 32, False, False)], '''	cc = (ra & 0xffff) + off + 2 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfffe
		Store rb:16 into WRAM at address @a with endianness endian
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		Store rb:16 into WRAM at address @a with endianness endian''', ['opCode7', 'rbOrDbExists', 'rcDisabled', 'rbOrDbIsRbArg', 'raAllEnabled', 'subOpCode01xxx', 'loadStoreSafeRegister', 'endianess', 'immediate24WithRb', 'subAuxx011x'], True, False, {'shs:rir': (0x0, 0x8000000)}),
	'sh:rii': InstructionInfo('sh', 'sh:rii', [WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	let @a = (ra + off)
	Store imm:16 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0100x', 'loadStoreUnsafeRegister', 'raAllEnabled', 'immediate12Store', 'subAuxx011x', 'immediate16Store', 'syntacticSugar'], True, True, {}),
	'sh:rir': InstructionInfo('sh', 'sh:rir', [WorkRegister('ra', 32, True, False), Immediate('off', 24, True), WorkRegister('rb', 32, False, False)], '''	let @a = (ra + off)
	Store rb:16 into WRAM at address @a with endianness endian''', ['opCode7', 'rbOrDbExists', 'rcDisabled', 'rbOrDbIsRbArg', 'raAllEnabled', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'immediate24WithRb', 'subAuxx011x', 'syntacticSugar'], True, True, {}),
	'sh_id:erii': InstructionInfo('sh_id', 'sh_id:erii', [Endianess('endian'), WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	let @a = (ra + off)
	Store (id | imm):16 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0111x', 'raAllEnabled', 'endianess', 'immediate12Store', 'subAuxx011x', 'immediate16Store'], True, False, {'sh_id:rii': (0x0, 0x8000000), 'sh_id:ri': (0x0, 0x80f0fff)}),
	'sh_id:ri': InstructionInfo('sh_id', 'sh_id:ri', [WorkRegister('ra', 32, True, False), Immediate('off', 12, True)], '''	let @a = (ra + off)
	Store (id | imm):16 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0111x', 'raAllEnabled', 'immediate12Store', 'subAuxx011x', 'syntacticSugar'], True, True, {}),
	'sh_id:rii': InstructionInfo('sh_id', 'sh_id:rii', [WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	let @a = (ra + off)
	Store (id | imm):16 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0111x', 'raAllEnabled', 'immediate12Store', 'subAuxx011x', 'immediate16Store', 'syntacticSugar'], True, True, {}),
	'shs:rii': InstructionInfo('shs', 'shs:rii', [WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	cc = (ra & 0xffff) + off + 2 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfffe
		Store imm:16 into WRAM at address @a with endianness endian
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		Store imm:16 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0100x', 'loadStoreSafeRegister', 'raAllEnabled', 'immediate12Store', 'subAuxx011x', 'immediate16Store', 'syntacticSugar'], True, True, {}),
	'shs:rir': InstructionInfo('shs', 'shs:rir', [WorkRegister('ra', 32, True, False), Immediate('off', 24, True), WorkRegister('rb', 32, False, False)], '''	cc = (ra & 0xffff) + off + 2 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfffe
		Store rb:16 into WRAM at address @a with endianness endian
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		Store rb:16 into WRAM at address @a with endianness endian''', ['opCode7', 'rbOrDbExists', 'rcDisabled', 'rbOrDbIsRbArg', 'raAllEnabled', 'subOpCode01xxx', 'loadStoreSafeRegister', 'immediate24WithRb', 'subAuxx011x', 'syntacticSugar'], True, True, {}),
	'stop:': InstructionInfo('stop', 'stop:', [], '''	if (boot_cc 1) then
		jump @[pc]
	stop current thread''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0011x', 'cstRc111001', 'raIsId', 'bootCondition', 'syntacticSugar'], True, True, {}),
	'stop:ci': InstructionInfo('stop', 'stop:ci', [Condition('boot_cc'), PcSpec('pc', 16)], '''	if (boot_cc 1) then
		jump @[pc]
	stop current thread''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0011x', 'cstRc111001', 'raIsId', 'bootCondition', 'targetPc'], True, False, {'stop:': (0x0, 0xf00ffff)}),
	'sub.s:rirc': InstructionInfo('sub.s', 'sub.s:rirc', [WorkRegister('dc', 64, False, False), Immediate('imm', 24, True), WorkRegister('ra', 32, True, False), Condition('sub_set_cc')], '''	x = imm - ra
	if (sub_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode0100x', 'subSetCondition', 'immediate24imm'], True, False, {}),
	'sub.s:rirci': InstructionInfo('sub.s', 'sub.s:rirci', [WorkRegister('dc', 64, False, False), Immediate('imm', 8, True), WorkRegister('ra', 32, True, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = imm - ra
	dc = x:S64
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode0100x', 'nzSubCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'sub.s:rirf': InstructionInfo('sub.s', 'sub.s:rirf', [WorkRegister('dc', 64, False, False), Immediate('imm', 24, True), WorkRegister('ra', 32, True, False), Condition('false_cc')], '''	x = imm - ra
	dc = x:S64
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode0100x', 'falseCondition5', 'immediate24imm'], True, False, {}),
	'sub.s:rric': InstructionInfo('sub.s', 'sub.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('ext_sub_set_cc')], '''	x = ra - imm
	if (ext_sub_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'subBoolSetCondition', 'immediate24imm'], True, False, {}),
	'sub.s:rrici': InstructionInfo('sub.s', 'sub.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = ra - imm
	dc = x:S64
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'sub.s:rrif': InstructionInfo('sub.s', 'sub.s:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra - imm
	dc = x:S64
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'falseCondition6', 'immediate24imm'], True, False, {}),
	'sub.s:rrr': InstructionInfo('sub.s', 'sub.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra - rb
	dc = x:S64
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition6'], True, False, {}),
	'sub.s:rrrc': InstructionInfo('sub.s', 'sub.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('ext_sub_set_cc')], '''	x = ra - rb
	if (ext_sub_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'subBoolSetCondition'], True, False, {}),
	'sub.s:rrrci': InstructionInfo('sub.s', 'sub.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = ra - rb
	dc = x:S64
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc'], True, False, {}),
	'sub.u:rirc': InstructionInfo('sub.u', 'sub.u:rirc', [WorkRegister('dc', 64, False, False), Immediate('imm', 24, True), WorkRegister('ra', 32, True, False), Condition('sub_set_cc')], '''	x = imm - ra
	if (sub_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode0100x', 'subSetCondition', 'immediate24imm'], True, False, {}),
	'sub.u:rirci': InstructionInfo('sub.u', 'sub.u:rirci', [WorkRegister('dc', 64, False, False), Immediate('imm', 8, True), WorkRegister('ra', 32, True, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = imm - ra
	dc = x:U64
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode0100x', 'nzSubCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'sub.u:rirf': InstructionInfo('sub.u', 'sub.u:rirf', [WorkRegister('dc', 64, False, False), Immediate('imm', 24, True), WorkRegister('ra', 32, True, False), Condition('false_cc')], '''	x = imm - ra
	dc = x:U64
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode0100x', 'falseCondition5', 'immediate24imm'], True, False, {}),
	'sub.u:rric': InstructionInfo('sub.u', 'sub.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('ext_sub_set_cc')], '''	x = ra - imm
	if (ext_sub_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'subBoolSetCondition', 'immediate24imm'], True, False, {}),
	'sub.u:rrici': InstructionInfo('sub.u', 'sub.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = ra - imm
	dc = x:U64
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'sub.u:rrif': InstructionInfo('sub.u', 'sub.u:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra - imm
	dc = x:U64
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'falseCondition6', 'immediate24imm'], True, False, {}),
	'sub.u:rrr': InstructionInfo('sub.u', 'sub.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra - rb
	dc = x:U64
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition6'], True, False, {}),
	'sub.u:rrrc': InstructionInfo('sub.u', 'sub.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('ext_sub_set_cc')], '''	x = ra - rb
	if (ext_sub_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'subBoolSetCondition'], True, False, {}),
	'sub.u:rrrci': InstructionInfo('sub.u', 'sub.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = ra - rb
	dc = x:U64
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc'], True, False, {}),
	'sub:rir': InstructionInfo('sub', 'sub:rir', [WorkRegister('rc', 32, False, False), Immediate('imm', 32, False), WorkRegister('ra', 32, True, False)], '''	rc = (imm - ra)
	ZF,CF <- rc''', ['opCode2Or3', 'rbDisabled', 'opCode2', 'rcEnabled', 'raAllEnabled', 'immediate32'], True, False, {'neg:rr': (0x0, 0xffffffff)}),
	'sub:rirc': InstructionInfo('sub', 'sub:rirc', [WorkRegister('rc', 32, False, False), Immediate('imm', 24, True), WorkRegister('ra', 32, True, False), Condition('sub_set_cc')], '''	x = imm - ra
	if (sub_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode0100x', 'subSetCondition', 'immediate24imm'], True, False, {}),
	'sub:rirci': InstructionInfo('sub', 'sub:rirci', [WorkRegister('rc', 32, False, False), Immediate('imm', 8, True), WorkRegister('ra', 32, True, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = imm - ra
	rc = x
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode0100x', 'nzSubCondition', 'immediate24Pc', 'targetPc'], True, False, {'neg:rrci': (0x0, 0xff0000)}),
	'sub:rirf': InstructionInfo('sub', 'sub:rirf', [WorkRegister('rc', 32, False, False), Immediate('imm', 24, True), WorkRegister('ra', 32, True, False), Condition('false_cc')], '''	x = imm - ra
	rc = x
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode0100x', 'falseCondition5', 'immediate24imm'], True, False, {}),
	'sub:rric': InstructionInfo('sub', 'sub:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('ext_sub_set_cc')], '''	x = ra - imm
	if (ext_sub_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'subBoolSetCondition', 'immediate24imm'], True, False, {}),
	'sub:rrici': InstructionInfo('sub', 'sub:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = ra - imm
	rc = x
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'sub:rrif': InstructionInfo('sub', 'sub:rrif', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra - imm
	rc = x
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'falseCondition6', 'immediate24imm'], True, False, {}),
	'sub:rrr': InstructionInfo('sub', 'sub:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra - rb
	rc = x
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition6'], True, False, {}),
	'sub:rrrc': InstructionInfo('sub', 'sub:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('ext_sub_set_cc')], '''	x = ra - rb
	if (ext_sub_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'subBoolSetCondition'], True, False, {}),
	'sub:rrrci': InstructionInfo('sub', 'sub:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = ra - rb
	rc = x
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc'], True, False, {}),
	'sub:ssi': InstructionInfo('sub', 'sub:ssi', [WorkRegister('sc', 32, False, True), WorkRegister('sa', 32, True, True), Immediate('imm', 17, True)], '''	cc = (ra & 0xffff) - imm >> 16
	if (const_cc_zero cc) then
		ZF,CF <- rc = (ra - imm)
	else
		ZF,CF <- rc = (ra - imm) & 0xffffff | (id << 25)
		raise exception(_memory_fault)''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode1x00x', 'safeRegister', 'immediate17_24imm'], True, False, {'subs:rri': (0, 0)}),
	'sub:sss': InstructionInfo('sub', 'sub:sss', [WorkRegister('sc', 32, False, True), WorkRegister('sa', 32, True, True), WorkRegister('sb', 32, False, True)], '''	cc = (ra & 0xffff) - rb >> 16
	if (const_cc_zero cc) then
		ZF,CF <- rc = (ra - rb)
	else
		ZF,CF <- rc = (ra - rb) & 0xffffff | (id << 25)
		raise exception(_memory_fault)''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode0100x', 'safeRegister', 'raRbArg', 'rbRaArg'], True, False, {'subs:rrr': (0, 0)}),
	'sub:zir': InstructionInfo('sub', 'sub:zir', [ConstantRegister('zero'), Immediate('imm', 32, False), WorkRegister('rb', 32, False, False)], '''	ZF,CF <- imm - rb''', ['opCode6', 'rbEnabled', 'raDisabled', 'immediate32ZeroRb', 'cstRc00100x'], True, False, {}),
	'sub:zirc': InstructionInfo('sub', 'sub:zirc', [ConstantRegister('zero'), Immediate('imm', 27, True), WorkRegister('ra', 32, True, False), Condition('sub_set_cc')], '''	ZF,CF <- imm - ra''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode0100x', 'subSetCondition', 'immediate27imm'], True, False, {}),
	'sub:zirci': InstructionInfo('sub', 'sub:zirci', [ConstantRegister('zero'), Immediate('imm', 11, True), WorkRegister('ra', 32, True, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = imm - ra
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode0100x', 'nzSubCondition', 'immediate27Pc', 'targetPc'], True, False, {}),
	'sub:zirf': InstructionInfo('sub', 'sub:zirf', [ConstantRegister('zero'), Immediate('imm', 27, True), WorkRegister('ra', 32, True, False), Condition('false_cc')], '''	ZF,CF <- imm - ra''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode0100x', 'falseCondition5', 'immediate27imm'], True, False, {}),
	'sub:zric': InstructionInfo('sub', 'sub:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 27, True), Condition('ext_sub_set_cc')], '''	ZF,CF <- ra - imm''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'subBoolSetCondition', 'immediate27imm'], True, False, {}),
	'sub:zrici': InstructionInfo('sub', 'sub:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 11, True), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = ra - imm
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate27Pc', 'targetPc'], True, False, {'jeq:rii': (0x2000000, 0x5f000000), 'jneq:rii': (0x3000000, 0x5f000000), 'jz:ri': (0x2000000, 0x3805fff0000), 'jnz:ri': (0x3000000, 0x3805fff0000), 'jltu:rii': (0x14000000, 0x5f000000), 'jgtu:rii': (0x1b000000, 0x5f000000), 'jleu:rii': (0x1a000000, 0x5f000000), 'jgeu:rii': (0x15000000, 0x5f000000), 'jlts:rii': (0x16000000, 0x5f000000), 'jgts:rii': (0x19000000, 0x5f000000), 'jles:rii': (0x18000000, 0x5f000000), 'jges:rii': (0x17000000, 0x5f000000)}),
	'sub:zrif': InstructionInfo('sub', 'sub:zrif', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 27, True), Condition('false_cc')], '''	ZF,CF <- ra - imm''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x00x', 'unsafeRegister', 'falseCondition6', 'immediate27imm'], True, False, {}),
	'sub:zrr': InstructionInfo('sub', 'sub:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF,CF <- ra - rb''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition6'], True, False, {}),
	'sub:zrrc': InstructionInfo('sub', 'sub:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('ext_sub_set_cc')], '''	ZF,CF <- ra - rb''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'subBoolSetCondition'], True, False, {}),
	'sub:zrrci': InstructionInfo('sub', 'sub:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = ra - rb
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode1x00x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc'], True, False, {'jeq:rri': (0x2000000, 0x5f000000), 'jneq:rri': (0x3000000, 0x5f000000), 'jltu:rri': (0x14000000, 0x5f000000), 'jgtu:rri': (0x1b000000, 0x5f000000), 'jleu:rri': (0x1a000000, 0x5f000000), 'jgeu:rri': (0x15000000, 0x5f000000), 'jlts:rri': (0x16000000, 0x5f000000), 'jgts:rri': (0x19000000, 0x5f000000), 'jles:rri': (0x18000000, 0x5f000000), 'jges:rri': (0x17000000, 0x5f000000)}),
	'subc.s:rirc': InstructionInfo('subc.s', 'subc.s:rirc', [WorkRegister('dc', 64, False, False), Immediate('imm', 24, True), WorkRegister('ra', 32, True, False), Condition('sub_set_cc')], '''	x = imm + ~ra + CF
	if (sub_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode0111x', 'subSetCondition', 'immediate24imm'], True, False, {}),
	'subc.s:rirci': InstructionInfo('subc.s', 'subc.s:rirci', [WorkRegister('dc', 64, False, False), Immediate('imm', 8, True), WorkRegister('ra', 32, True, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = imm + ~ra + CF
	dc = x:S64
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode0111x', 'nzSubCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'subc.s:rirf': InstructionInfo('subc.s', 'subc.s:rirf', [WorkRegister('dc', 64, False, False), Immediate('imm', 24, True), WorkRegister('ra', 32, True, False), Condition('false_cc')], '''	x = imm + ~ra + CF
	dc = x:S64
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode0111x', 'falseCondition5', 'immediate24imm'], True, False, {}),
	'subc.s:rric': InstructionInfo('subc.s', 'subc.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('ext_sub_set_cc')], '''	x = ra + ~imm + CF
	if (ext_sub_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode1x11x', 'unsafeRegister', 'subBoolSetCondition', 'immediate24imm'], True, False, {}),
	'subc.s:rrici': InstructionInfo('subc.s', 'subc.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = ra + ~imm + CF
	dc = x:S64
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode1x11x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'subc.s:rrif': InstructionInfo('subc.s', 'subc.s:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra + ~imm + CF
	dc = x:S64
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXSignedExtendedEnabled', 'raAllEnabled', 'subOpCode1x11x', 'unsafeRegister', 'falseCondition6', 'immediate24imm'], True, False, {}),
	'subc.s:rrr': InstructionInfo('subc.s', 'subc.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra + ~rb + CF
	dc = x:S64
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode1x11x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition6'], True, False, {}),
	'subc.s:rrrc': InstructionInfo('subc.s', 'subc.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('ext_sub_set_cc')], '''	x = ra + ~rb + CF
	if (ext_sub_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode1x11x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'subBoolSetCondition'], True, False, {}),
	'subc.s:rrrci': InstructionInfo('subc.s', 'subc.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = ra + ~rb + CF
	dc = x:S64
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXSignedExtendedEnabled', 'subOpCode1x11x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc'], True, False, {}),
	'subc.u:rirc': InstructionInfo('subc.u', 'subc.u:rirc', [WorkRegister('dc', 64, False, False), Immediate('imm', 24, True), WorkRegister('ra', 32, True, False), Condition('sub_set_cc')], '''	x = imm + ~ra + CF
	if (sub_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode0111x', 'subSetCondition', 'immediate24imm'], True, False, {}),
	'subc.u:rirci': InstructionInfo('subc.u', 'subc.u:rirci', [WorkRegister('dc', 64, False, False), Immediate('imm', 8, True), WorkRegister('ra', 32, True, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = imm + ~ra + CF
	dc = x:U64
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode0111x', 'nzSubCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'subc.u:rirf': InstructionInfo('subc.u', 'subc.u:rirf', [WorkRegister('dc', 64, False, False), Immediate('imm', 24, True), WorkRegister('ra', 32, True, False), Condition('false_cc')], '''	x = imm + ~ra + CF
	dc = x:U64
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode0111x', 'falseCondition5', 'immediate24imm'], True, False, {}),
	'subc.u:rric': InstructionInfo('subc.u', 'subc.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('ext_sub_set_cc')], '''	x = ra + ~imm + CF
	if (ext_sub_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode1x11x', 'unsafeRegister', 'subBoolSetCondition', 'immediate24imm'], True, False, {}),
	'subc.u:rrici': InstructionInfo('subc.u', 'subc.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = ra + ~imm + CF
	dc = x:U64
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode1x11x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'subc.u:rrif': InstructionInfo('subc.u', 'subc.u:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra + ~imm + CF
	dc = x:U64
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXUnsignedExtendedEnabled', 'raAllEnabled', 'subOpCode1x11x', 'unsafeRegister', 'falseCondition6', 'immediate24imm'], True, False, {}),
	'subc.u:rrr': InstructionInfo('subc.u', 'subc.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra + ~rb + CF
	dc = x:U64
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode1x11x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition6'], True, False, {}),
	'subc.u:rrrc': InstructionInfo('subc.u', 'subc.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('ext_sub_set_cc')], '''	x = ra + ~rb + CF
	if (ext_sub_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode1x11x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'subBoolSetCondition'], True, False, {}),
	'subc.u:rrrci': InstructionInfo('subc.u', 'subc.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = ra + ~rb + CF
	dc = x:U64
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXUnsignedExtendedEnabled', 'subOpCode1x11x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc'], True, False, {}),
	'subc:rir': InstructionInfo('subc', 'subc:rir', [WorkRegister('rc', 32, False, False), Immediate('imm', 32, False), WorkRegister('ra', 32, True, False)], '''	rc = (imm + ~ra + CF)
	ZF,CF <- rc''', ['opCode2Or3', 'rbDisabled', 'opCode3', 'rcEnabled', 'raAllEnabled', 'immediate32'], True, False, {}),
	'subc:rirc': InstructionInfo('subc', 'subc:rirc', [WorkRegister('rc', 32, False, False), Immediate('imm', 24, True), WorkRegister('ra', 32, True, False), Condition('sub_set_cc')], '''	x = imm + ~ra + CF
	if (sub_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode0111x', 'subSetCondition', 'immediate24imm'], True, False, {}),
	'subc:rirci': InstructionInfo('subc', 'subc:rirci', [WorkRegister('rc', 32, False, False), Immediate('imm', 8, True), WorkRegister('ra', 32, True, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = imm + ~ra + CF
	rc = x
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode0111x', 'nzSubCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'subc:rirf': InstructionInfo('subc', 'subc:rirf', [WorkRegister('rc', 32, False, False), Immediate('imm', 24, True), WorkRegister('ra', 32, True, False), Condition('false_cc')], '''	x = imm + ~ra + CF
	rc = x
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode0111x', 'falseCondition5', 'immediate24imm'], True, False, {}),
	'subc:rric': InstructionInfo('subc', 'subc:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('ext_sub_set_cc')], '''	x = ra + ~imm + CF
	if (ext_sub_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode1x11x', 'unsafeRegister', 'subBoolSetCondition', 'immediate24imm'], True, False, {}),
	'subc:rrici': InstructionInfo('subc', 'subc:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = ra + ~imm + CF
	rc = x
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode1x11x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'subc:rrif': InstructionInfo('subc', 'subc:rrif', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra + ~imm + CF
	rc = x
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode1x11x', 'unsafeRegister', 'falseCondition6', 'immediate24imm'], True, False, {}),
	'subc:rrr': InstructionInfo('subc', 'subc:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra + ~rb + CF
	rc = x
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode1x11x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition6'], True, False, {}),
	'subc:rrrc': InstructionInfo('subc', 'subc:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('ext_sub_set_cc')], '''	x = ra + ~rb + CF
	if (ext_sub_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode1x11x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'subBoolSetCondition'], True, False, {}),
	'subc:rrrci': InstructionInfo('subc', 'subc:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = ra + ~rb + CF
	rc = x
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode1x11x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc'], True, False, {}),
	'subc:zir': InstructionInfo('subc', 'subc:zir', [ConstantRegister('zero'), Immediate('imm', 32, False), WorkRegister('rb', 32, False, False)], '''	ZF,CF <- imm + ~rb + CF''', ['opCode6', 'rbEnabled', 'raDisabled', 'immediate32ZeroRb', 'cstRc00111x'], True, False, {}),
	'subc:zirc': InstructionInfo('subc', 'subc:zirc', [ConstantRegister('zero'), Immediate('imm', 27, True), WorkRegister('ra', 32, True, False), Condition('sub_set_cc')], '''	ZF,CF <- imm + ~ra + CF''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode0111x', 'subSetCondition', 'immediate27imm'], True, False, {}),
	'subc:zirci': InstructionInfo('subc', 'subc:zirci', [ConstantRegister('zero'), Immediate('imm', 11, True), WorkRegister('ra', 32, True, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = imm + ~ra + CF
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode0111x', 'nzSubCondition', 'immediate27Pc', 'targetPc'], True, False, {}),
	'subc:zirf': InstructionInfo('subc', 'subc:zirf', [ConstantRegister('zero'), Immediate('imm', 27, True), WorkRegister('ra', 32, True, False), Condition('false_cc')], '''	ZF,CF <- imm + ~ra + CF''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode0111x', 'falseCondition5', 'immediate27imm'], True, False, {}),
	'subc:zric': InstructionInfo('subc', 'subc:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 27, True), Condition('ext_sub_set_cc')], '''	ZF,CF <- ra + ~imm + CF''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x11x', 'unsafeRegister', 'subBoolSetCondition', 'immediate27imm'], True, False, {}),
	'subc:zrici': InstructionInfo('subc', 'subc:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 11, True), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = ra + ~imm + CF
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x11x', 'unsafeRegister', 'nzSubBoolCondition', 'immediate27Pc', 'targetPc'], True, False, {}),
	'subc:zrif': InstructionInfo('subc', 'subc:zrif', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 27, True), Condition('false_cc')], '''	ZF,CF <- ra + ~imm + CF''', ['opCodeXOrY', 'rbDisabled', 'rcXDisabled', 'raAllEnabled', 'subOpCode1x11x', 'unsafeRegister', 'falseCondition6', 'immediate27imm'], True, False, {}),
	'subc:zrr': InstructionInfo('subc', 'subc:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF,CF <- ra + ~rb + CF''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode1x11x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'falseCondition6'], True, False, {}),
	'subc:zrrc': InstructionInfo('subc', 'subc:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('ext_sub_set_cc')], '''	ZF,CF <- ra + ~rb + CF''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode1x11x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'subBoolSetCondition'], True, False, {}),
	'subc:zrrci': InstructionInfo('subc', 'subc:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('sub_nz_cc'), PcSpec('pc', 16)], '''	x = ra + ~rb + CF
	if (sub_nz_cc x) then
		jump @[pc]
	ZF,CF <- x''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXDisabled', 'subOpCode1x11x', 'unsafeRegister', 'rbArg', 'raAllEnabled', 'nzSubBoolCondition', 'targetPc'], True, False, {}),
	'subs:rri': InstructionInfo('subs', 'subs:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 17, True)], '''	cc = (ra & 0xffff) - imm >> 16
	if (const_cc_zero cc) then
		ZF,CF <- rc = (ra - imm)
	else
		ZF,CF <- rc = (ra - imm) & 0xffffff | (id << 25)
		raise exception(_memory_fault)''', ['opCodeXOrY', 'rbDisabled', 'rcXNotExtendedEnabled', 'raAllEnabled', 'subOpCode1x00x', 'safeRegister', 'immediate17_24imm', 'syntacticSugar'], True, True, {}),
	'subs:rrr': InstructionInfo('subs', 'subs:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	cc = (ra & 0xffff) - rb >> 16
	if (const_cc_zero cc) then
		ZF,CF <- rc = (ra - rb)
	else
		ZF,CF <- rc = (ra - rb) & 0xffffff | (id << 25)
		raise exception(_memory_fault)''', ['opCodeXOrY', 'rbExists', 'subA11xxx', 'rcXNotExtendedEnabled', 'subOpCode0100x', 'safeRegister', 'raRbArg', 'rbRaArg', 'syntacticSugar'], True, True, {}),
	'sw:erii': InstructionInfo('sw', 'sw:erii', [Endianess('endian'), WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	let @a = (ra + off)
	Store imm:S32:32 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0100x', 'loadStoreUnsafeRegister', 'raAllEnabled', 'endianess', 'immediate12Store', 'subAuxx100x', 'immediate16Store'], True, False, {'sw:rii': (0x0, 0x8000000)}),
	'sw:erir': InstructionInfo('sw', 'sw:erir', [Endianess('endian'), WorkRegister('ra', 32, True, False), Immediate('off', 24, True), WorkRegister('rb', 32, False, False)], '''	let @a = (ra + off)
	Store rb:32 into WRAM at address @a with endianness endian''', ['opCode7', 'rbOrDbExists', 'rcDisabled', 'rbOrDbIsRbArg', 'raAllEnabled', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'endianess', 'immediate24WithRb', 'subAuxx100x'], True, False, {'sw:rir': (0x0, 0x8000000)}),
	'sw:esii': InstructionInfo('sw', 'sw:esii', [Endianess('endian'), WorkRegister('sa', 32, True, True), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	cc = (ra & 0xffff) + off + 4 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfffc
		Store imm:S32:32 into WRAM at address @a with endianness endian
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		Store imm:S32:32 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0100x', 'loadStoreSafeRegister', 'raAllEnabled', 'endianess', 'immediate12Store', 'subAuxx100x', 'immediate16Store'], True, False, {'sws:rii': (0x0, 0x8000000)}),
	'sw:esir': InstructionInfo('sw', 'sw:esir', [Endianess('endian'), WorkRegister('sa', 32, True, True), Immediate('off', 24, True), WorkRegister('rb', 32, False, False)], '''	cc = (ra & 0xffff) + off + 4 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfffc
		Store rb:32 into WRAM at address @a with endianness endian
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		Store rb:32 into WRAM at address @a with endianness endian''', ['opCode7', 'rbOrDbExists', 'rcDisabled', 'rbOrDbIsRbArg', 'raAllEnabled', 'subOpCode01xxx', 'loadStoreSafeRegister', 'endianess', 'immediate24WithRb', 'subAuxx100x'], True, False, {'sws:rir': (0x0, 0x8000000)}),
	'sw:rii': InstructionInfo('sw', 'sw:rii', [WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	let @a = (ra + off)
	Store imm:S32:32 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0100x', 'loadStoreUnsafeRegister', 'raAllEnabled', 'immediate12Store', 'subAuxx100x', 'immediate16Store', 'syntacticSugar'], True, True, {}),
	'sw:rir': InstructionInfo('sw', 'sw:rir', [WorkRegister('ra', 32, True, False), Immediate('off', 24, True), WorkRegister('rb', 32, False, False)], '''	let @a = (ra + off)
	Store rb:32 into WRAM at address @a with endianness endian''', ['opCode7', 'rbOrDbExists', 'rcDisabled', 'rbOrDbIsRbArg', 'raAllEnabled', 'subOpCode01xxx', 'loadStoreUnsafeRegister', 'immediate24WithRb', 'subAuxx100x', 'syntacticSugar'], True, True, {}),
	'sw_id:erii': InstructionInfo('sw_id', 'sw_id:erii', [Endianess('endian'), WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	let @a = (ra + off)
	Store (id | imm):32 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0111x', 'raAllEnabled', 'endianess', 'immediate12Store', 'subAuxx100x', 'immediate16Store'], True, False, {'sw_id:rii': (0x0, 0x8000000), 'sw_id:ri': (0x0, 0x80f0fff)}),
	'sw_id:ri': InstructionInfo('sw_id', 'sw_id:ri', [WorkRegister('ra', 32, True, False), Immediate('off', 12, True)], '''	let @a = (ra + off)
	Store (id | imm):32 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0111x', 'raAllEnabled', 'immediate12Store', 'subAuxx100x', 'syntacticSugar'], True, True, {}),
	'sw_id:rii': InstructionInfo('sw_id', 'sw_id:rii', [WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	let @a = (ra + off)
	Store (id | imm):32 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0111x', 'raAllEnabled', 'immediate12Store', 'subAuxx100x', 'immediate16Store', 'syntacticSugar'], True, True, {}),
	'swapd:rr': InstructionInfo('swapd', 'swapd:rr', [WorkRegister('dc', 64, False, False), WorkRegister('db', 64, False, False)], '''	cc = x = dbe
	y = dbo
	dc = (y, x)
	if (true_false_cc cc) then
		jump @[pc]''', ['opCode8Or9', 'dbEnabled', 'subOpCode10110', 'dcEnabled', 'raIsZero', 'bootCondition', 'syntacticSugar'], True, True, {}),
	'swapd:rrci': InstructionInfo('swapd', 'swapd:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('db', 64, False, False), Condition('true_false_cc'), PcSpec('pc', 16)], '''	cc = x = dbe
	y = dbo
	dc = (y, x)
	if (true_false_cc cc) then
		jump @[pc]''', ['opCode8Or9', 'dbEnabled', 'subOpCode10110', 'dcEnabled', 'raIsZero', 'bootCondition', 'targetPc'], True, False, {'swapd:rr': (0x0, 0xf00ffff)}),
	'sws:rii': InstructionInfo('sws', 'sws:rii', [WorkRegister('ra', 32, True, False), Immediate('off', 12, True), Immediate('imm', 16, True)], '''	cc = (ra & 0xffff) + off + 4 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfffc
		Store imm:S32:32 into WRAM at address @a with endianness endian
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		Store imm:S32:32 into WRAM at address @a with endianness endian''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode0100x', 'loadStoreSafeRegister', 'raAllEnabled', 'immediate12Store', 'subAuxx100x', 'immediate16Store', 'syntacticSugar'], True, True, {}),
	'sws:rir': InstructionInfo('sws', 'sws:rir', [WorkRegister('ra', 32, True, False), Immediate('off', 24, True), WorkRegister('rb', 32, False, False)], '''	cc = (ra & 0xffff) + off + 4 - (ra >> 16)
	if (const_cc_ge0 cc) then
		let @a = (ra & 0xffff) + off & 0xfffc
		Store rb:32 into WRAM at address @a with endianness endian
		raise exception(_memory_fault)
	else
		let @a = (ra & 0xffff) + off
		Store rb:32 into WRAM at address @a with endianness endian''', ['opCode7', 'rbOrDbExists', 'rcDisabled', 'rbOrDbIsRbArg', 'raAllEnabled', 'subOpCode01xxx', 'loadStoreSafeRegister', 'immediate24WithRb', 'subAuxx100x', 'syntacticSugar'], True, True, {}),
	'tell:ri': InstructionInfo('tell', 'tell:ri', [WorkRegister('ra', 32, False, False), Immediate('off', 24, True)], '''	instruction with no effect''', ['opCode7', 'rbDisabled', 'rcDisabled', 'subOpCode00000', 'cstRc110000', 'raEnabled', 'falseCondition', 'immediate24'], False, False, {}),
	'time.s:r': InstructionInfo('time.s', 'time.s:r', [WorkRegister('dc', 64, False, False)], '''	x = (T:read)[4:35]
	dc = x:S64''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsZero', 'rcXSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'time.s:rci': InstructionInfo('time.s', 'time.s:rci', [WorkRegister('dc', 64, False, False), Condition('true_cc'), PcSpec('pc', 16)], '''	x = (T:read)[4:35]
	dc = x:S64
	if (true_cc x) then
		jump @[pc]''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsZero', 'rcXSignedExtendedEnabled', 'nzBootCondition', 'targetPc'], True, False, {}),
	'time.u:r': InstructionInfo('time.u', 'time.u:r', [WorkRegister('dc', 64, False, False)], '''	x = (T:read)[4:35]
	dc = x:U64''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsZero', 'rcXUnsignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'time.u:rci': InstructionInfo('time.u', 'time.u:rci', [WorkRegister('dc', 64, False, False), Condition('true_cc'), PcSpec('pc', 16)], '''	x = (T:read)[4:35]
	dc = x:U64
	if (true_cc x) then
		jump @[pc]''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsZero', 'rcXUnsignedExtendedEnabled', 'nzBootCondition', 'targetPc'], True, False, {}),
	'time:r': InstructionInfo('time', 'time:r', [WorkRegister('rc', 32, False, False)], '''	x = (T:read)[4:35]
	rc = x''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsZero', 'rcXNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'time:rci': InstructionInfo('time', 'time:rci', [WorkRegister('rc', 32, False, False), Condition('true_cc'), PcSpec('pc', 16)], '''	x = (T:read)[4:35]
	rc = x
	if (true_cc x) then
		jump @[pc]''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsZero', 'rcXNotExtendedEnabled', 'nzBootCondition', 'targetPc'], True, False, {}),
	'time:z': InstructionInfo('time', 'time:z', [ConstantRegister('zero')], '''	instruction with no effect''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsZero', 'rcXDisabled', 'falseCondition4'], True, False, {}),
	'time:zci': InstructionInfo('time', 'time:zci', [ConstantRegister('zero'), Condition('true_cc'), PcSpec('pc', 16)], '''	x = (T:read)[4:35]
	if (true_cc x) then
		jump @[pc]''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsZero', 'rcXDisabled', 'nzBootCondition', 'targetPc'], True, False, {}),
	'time_cfg.s:rr': InstructionInfo('time_cfg.s', 'time_cfg.s:rr', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False)], '''	x = T:read
	T:config(rb[0:2])
	y = x[4:35]
	dc = y:S64''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsOne', 'rbArg', 'rcXSignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'time_cfg.s:rrci': InstructionInfo('time_cfg.s', 'time_cfg.s:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), Condition('true_cc'), PcSpec('pc', 16)], '''	x = T:read
	T:config(rb[0:2])
	y = x[4:35]
	dc = y:S64
	if (true_cc y) then
		jump @[pc]''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsOne', 'rbArg', 'rcXSignedExtendedEnabled', 'nzBootCondition', 'targetPc'], True, False, {}),
	'time_cfg.u:rr': InstructionInfo('time_cfg.u', 'time_cfg.u:rr', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False)], '''	x = T:read
	T:config(rb[0:2])
	y = x[4:35]
	dc = y:U64''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsOne', 'rbArg', 'rcXUnsignedExtendedEnabled', 'falseCondition4'], True, False, {}),
	'time_cfg.u:rrci': InstructionInfo('time_cfg.u', 'time_cfg.u:rrci', [WorkRegister('dc', 64, False, False), WorkRegister('rb', 32, False, False), Condition('true_cc'), PcSpec('pc', 16)], '''	x = T:read
	T:config(rb[0:2])
	y = x[4:35]
	dc = y:U64
	if (true_cc y) then
		jump @[pc]''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsOne', 'rbArg', 'rcXUnsignedExtendedEnabled', 'nzBootCondition', 'targetPc'], True, False, {}),
	'time_cfg:r': InstructionInfo('time_cfg', 'time_cfg:r', [WorkRegister('rb', 32, False, False)], '''	instruction with no effect''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsOne', 'rbArg', 'rcXDisabled', 'falseCondition4', 'syntacticSugar'], True, True, {}),
	'time_cfg:rr': InstructionInfo('time_cfg', 'time_cfg:rr', [WorkRegister('rc', 32, False, False), WorkRegister('rb', 32, False, False)], '''	x = T:read
	T:config(rb[0:2])
	y = x[4:35]
	rc = y''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsOne', 'rbArg', 'rcXNotExtendedEnabled', 'falseCondition4'], True, False, {}),
	'time_cfg:rrci': InstructionInfo('time_cfg', 'time_cfg:rrci', [WorkRegister('rc', 32, False, False), WorkRegister('rb', 32, False, False), Condition('true_cc'), PcSpec('pc', 16)], '''	x = T:read
	T:config(rb[0:2])
	y = x[4:35]
	rc = y
	if (true_cc y) then
		jump @[pc]''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsOne', 'rbArg', 'rcXNotExtendedEnabled', 'nzBootCondition', 'targetPc'], True, False, {}),
	'time_cfg:zr': InstructionInfo('time_cfg', 'time_cfg:zr', [ConstantRegister('zero'), WorkRegister('rb', 32, False, False)], '''	instruction with no effect''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsOne', 'rbArg', 'rcXDisabled', 'falseCondition4'], True, False, {'time_cfg:r': (0, 0)}),
	'time_cfg:zrci': InstructionInfo('time_cfg', 'time_cfg:zrci', [ConstantRegister('zero'), WorkRegister('rb', 32, False, False), Condition('true_cc'), PcSpec('pc', 16)], '''	x = T:read
	T:config(rb[0:2])
	y = x[4:35]
	if (true_cc y) then
		jump @[pc]''', ['opCodeXOrY', 'rbExists', 'subANot11xx', 'subOpCodexx110', 'subA10000', 'raIsOne', 'rbArg', 'rcXDisabled', 'nzBootCondition', 'targetPc'], True, False, {}),
	'xor.s:rric': InstructionInfo('xor.s', 'xor.s:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ra ^ imm
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10000', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'xor.s:rrici': InstructionInfo('xor.s', 'xor.s:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra ^ imm
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10000', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'xor.s:rrif': InstructionInfo('xor.s', 'xor.s:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra ^ imm
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10000', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'xor.s:rrr': InstructionInfo('xor.s', 'xor.s:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra ^ rb
	dc = x:S64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10000', 'rcSignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'xor.s:rrrc': InstructionInfo('xor.s', 'xor.s:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra ^ rb
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10000', 'rcSignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'xor.s:rrrci': InstructionInfo('xor.s', 'xor.s:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra ^ rb
	dc = x:S64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10000', 'rcSignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'xor.u:rric': InstructionInfo('xor.u', 'xor.u:rric', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ra ^ imm
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10000', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'xor.u:rrici': InstructionInfo('xor.u', 'xor.u:rrici', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra ^ imm
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10000', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {}),
	'xor.u:rrif': InstructionInfo('xor.u', 'xor.u:rrif', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra ^ imm
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10000', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'xor.u:rrr': InstructionInfo('xor.u', 'xor.u:rrr', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra ^ rb
	dc = x:U64
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10000', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'xor.u:rrrc': InstructionInfo('xor.u', 'xor.u:rrrc', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra ^ rb
	if (log_set_cc x) then
		dc = 1
	else
		dc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10000', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'xor.u:rrrci': InstructionInfo('xor.u', 'xor.u:rrrci', [WorkRegister('dc', 64, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra ^ rb
	dc = x:U64
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10000', 'rcUnsignedExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'xor:rri': InstructionInfo('xor', 'xor:rri', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 32, False)], '''	rc = (ra ^ imm)
	ZF <- rc''', ['opCode4Or5', 'rbDisabled', 'opCode4', 'rcEnabled', 'raAllEnabled', 'immediate32'], True, False, {'not:rr': (0xffffffff, 0xffffffff)}),
	'xor:rric': InstructionInfo('xor', 'xor:rric', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('log_set_cc')], '''	x = ra ^ imm
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10000', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition', 'immediate24imm'], True, False, {}),
	'xor:rrici': InstructionInfo('xor', 'xor:rrici', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 8, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra ^ imm
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10000', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate24Pc', 'targetPc'], True, False, {'not:rrci': (0xff0000, 0xff0000)}),
	'xor:rrif': InstructionInfo('xor', 'xor:rrif', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), Immediate('imm', 24, True), Condition('false_cc')], '''	x = ra ^ imm
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10000', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4', 'immediate24imm'], True, False, {}),
	'xor:rrr': InstructionInfo('xor', 'xor:rrr', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	x = ra ^ rb
	rc = x
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10000', 'rcNotExtendedEnabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'xor:rrrc': InstructionInfo('xor', 'xor:rrrc', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	x = ra ^ rb
	if (log_set_cc x) then
		rc = 1
	else
		rc = 0
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10000', 'rcNotExtendedEnabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'xor:rrrci': InstructionInfo('xor', 'xor:rrrci', [WorkRegister('rc', 32, False, False), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra ^ rb
	rc = x
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10000', 'rcNotExtendedEnabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
	'xor:zri': InstructionInfo('xor', 'xor:zri', [ConstantRegister('zero'), WorkRegister('rb', 32, False, False), Immediate('imm', 32, False)], '''	ZF <- rb ^ imm''', ['opCode6', 'rbEnabled', 'raDisabled', 'immediate32ZeroRb', 'cstRc01000x'], True, False, {}),
	'xor:zric': InstructionInfo('xor', 'xor:zric', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('log_set_cc')], '''	ZF <- ra ^ imm''', ['opCode8Or9', 'rbDisabled', 'subOpCode10000', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition', 'immediate28imm'], True, False, {}),
	'xor:zrici': InstructionInfo('xor', 'xor:zrici', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 12, True), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra ^ imm
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbDisabled', 'subOpCode10000', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'immediate28PcOpcode8', 'targetPc'], True, False, {'not:rci': (0x138000ff0000, 0x138000ff0000)}),
	'xor:zrif': InstructionInfo('xor', 'xor:zrif', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), Immediate('imm', 28, True), Condition('false_cc')], '''	ZF <- ra ^ imm''', ['opCode8Or9', 'rbDisabled', 'subOpCode10000', 'rcDisabled', 'raAllEnabled', 'falseCondition4', 'immediate28imm'], True, False, {}),
	'xor:zrr': InstructionInfo('xor', 'xor:zrr', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False)], '''	ZF <- ra ^ rb''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10000', 'rcDisabled', 'raAllEnabled', 'falseCondition4'], True, False, {}),
	'xor:zrrc': InstructionInfo('xor', 'xor:zrrc', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_set_cc')], '''	ZF <- ra ^ rb''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10000', 'rcDisabled', 'raAllEnabled', 'logicalSetCondition'], True, False, {}),
	'xor:zrrci': InstructionInfo('xor', 'xor:zrrci', [ConstantRegister('zero'), WorkRegister('ra', 32, True, False), WorkRegister('rb', 32, False, False), Condition('log_nz_cc'), PcSpec('pc', 16)], '''	x = ra ^ rb
	if (log_nz_cc x) then
		jump @[pc]
	ZF <- x''', ['opCode8Or9', 'rbEnabled', 'subOpCode11000', 'subA10000', 'rcDisabled', 'raAllEnabled', 'nzLogicalCondition', 'targetPc'], True, False, {}),
}
