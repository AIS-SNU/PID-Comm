//
// Copyright 2020 UPMEM. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Software reset
// Clears all the registers of all the threads.
// Fills in the WRAM with a reset value.

#ifndef LAST_WRAM_LWORD_ADDRESS
#error "please define LAST_WRAM_LWORD_ADDRESS = numberOfWordsInWram * 4 - 8"
#endif
#ifndef NUMBER_OF_THREADS_M1
#error "please define NUMBER_OF_THREADS_M1 = numberOfDpuThreads - 1"
#endif
#ifndef NUMBER_OF_ATOMIC_BITS
#error "please define NUMBER_OF_ATOMIC_BITS"
#endif

#define RESET_WORD_VALUE 0xfabddbaf

.text
.globl __bootstrap
__bootstrap:

sub zero, id, NUMBER_OF_THREADS_M1, z, clear_atomic_bits
boot_thread:
    boot id, 1, true, clear_atomic_bits

clear_atomic_bits:
    add r0, id, NUMBER_OF_ATOMIC_BITS - (NUMBER_OF_THREADS_M1 + 1)
    clear_atomic_bit_loop:
        release r0, 0, nz, .+1
        add r0, r0, -(NUMBER_OF_THREADS_M1 + 1), pl, clear_atomic_bit_loop

#ifndef __SWRESET_NO_WRAM_INIT__
populate_wram:
	or r0, zero, RESET_WORD_VALUE
	or r1, zero, RESET_WORD_VALUE
	add r2, id8, (LAST_WRAM_LWORD_ADDRESS + 8) - (NUMBER_OF_THREADS_M1 + 1) * 8
	or r3, zero, (NUMBER_OF_THREADS_M1 + 1) * 8
	for_loop:
		sd !little, r2, 0, d0
		sub r2, r2, r3, pl, for_loop
#endif
#ifdef __SWRESET_ZERO_MRAM_1K__
sub zero, id, 0, nz, populate_work_registers
zero_wram:
    or r0, zero, 0
    or r1, zero, 0
    or r2, zero, 1024
    zero_wram_loop:
        sd !little, r2, 0, d0
        add r2, r2, -8, pl, zero_wram_loop

    sdma r0, r0, 127
#endif

populate_work_registers:
	or r0, zero, RESET_WORD_VALUE
	or r1, zero, RESET_WORD_VALUE
	movd d2, d0, false, 0
	movd d4, d0, false, 0
	movd d6, d0, false, 0
	movd d8, d0, false, 0
	movd d10, d0, false, 0
	movd d12, d0, false, 0
	movd d14, d0, false, 0
	movd d16, d0, false, 0
	movd d18, d0, false, 0
	movd d20, d0, false, 0
	movd d22, d0, false, 0

	stop true, 0
