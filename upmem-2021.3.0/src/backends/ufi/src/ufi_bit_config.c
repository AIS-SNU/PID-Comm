/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <ufi/ufi_ci_types.h>
#include <dpu_types.h>
#include <ufi_rank_utils.h>

#define NR_NIBBLES_PER_BYTE 2
#define NR_BITS_PER_NIBBLE 4

static u8 get_reciprocal(u8 code);

static const u8 index_in_nibble[16] = {
	0, //  N/A  0000
	0, //       0001
	1, //       0010
	0, //  N/A  0011
	2, //       0100
	0, //  N/A  0101
	0, //  N/A  0110
	0, //  N/A  0111
	3, //       1000
	0, //  N/A  1001
	0, //  N/A  1010
	0, //  N/A  1011
	0, //  N/A  1100
	0, //  N/A  1101
	0, //  N/A  1110
	0, //  N/A  1111
};

__API_SYMBOL__ void dpu_bit_config_compute(u32 bit_order_result,
					   struct dpu_bit_config *config)
{
	/* 0/ Byte order matrix */
	// TODO

	/* 1/ Determine nibble swap if any */

	/*  The "0F" part of the pattern's 32-lsb is used to determine if there is a nibble swapping or not */
	u8 nibble_swap =
		(((bit_order_result >> 28) & 0b1111) != 0) ? 0xFF : 0x00;

	/* 2/ Determine the bit shufflingS */

	/* m_syndrome is the function that describes the shuffling from DPU to CPU of the MSB */
	u8 m_syndrome = // (0b00 << (index_in_nibble[ (bit_order_result >>   ) & 0b1111 ] << 1)) |  // see note *
		(0b01
		 << (index_in_nibble[(bit_order_result >> 4) & 0b1111] << 1)) |
		(0b10
		 << (index_in_nibble[(bit_order_result >> 12) & 0b1111] << 1)) |
		(0b11
		 << (index_in_nibble[(bit_order_result >> 20) & 0b1111] << 1));

	/* l_syndrome is the function that describes the shuffling from DPU to CPU of the LSB */
	u8 l_syndrome = // (0b00 << (index_in_nibble[ (bit_order_result >>   ) & 0b1111 ] << 1)) |  // see note *
		(0b01
		 << (index_in_nibble[(bit_order_result >> 0) & 0b1111] << 1)) |
		(0b10
		 << (index_in_nibble[(bit_order_result >> 8) & 0b1111] << 1)) |
		(0b11
		 << (index_in_nibble[(bit_order_result >> 16) & 0b1111] << 1));

	/* mm_syndrome and ll_syndrome are the inverse functions (ie from CPU to DPU) */
	u8 mm_syndrome = nibble_swap ? get_reciprocal(l_syndrome) :
					     get_reciprocal(m_syndrome);
	u8 ll_syndrome = nibble_swap ? get_reciprocal(m_syndrome) :
					     get_reciprocal(l_syndrome);

	/* 3/ Determine dpu to cpu bit shuffling */

	/* From the above syndromes, "compute" the bitfield for the BIT_ORDER command */
	u8 config_3124 = nibble_swap ? get_reciprocal(ll_syndrome) :
					     get_reciprocal(mm_syndrome);
	u8 config_2316 = nibble_swap ? get_reciprocal(mm_syndrome) :
					     get_reciprocal(ll_syndrome);

	u8 config_4740 = nibble_swap ? get_reciprocal(l_syndrome) :
					     get_reciprocal(m_syndrome);
	u8 config_3932 = nibble_swap ? get_reciprocal(m_syndrome) :
					     get_reciprocal(l_syndrome);

	config->nibble_swap = nibble_swap;
	config->cpu2dpu = config_2316 | (config_3124 << 8);
	config->dpu2cpu = config_3932 | (config_4740 << 8);
}

__API_SYMBOL__ u32 dpu_bit_config_dpu2cpu(struct dpu_bit_config *config,
					  u32 value)
{
	u32 cpu_value = 0;
	u8 byte_id, nibble_id, transformation_id;

	// TODO Not that sure about nibble_swap : should it done after computation or before computation (when retrieving
	// nibble_value) ?

	for (byte_id = 0; byte_id < sizeof(u32); ++byte_id) {
		u8 byte_value = (value >> (byte_id << 3)) & 0xFF;
		u8 byte_transformed = 0;

		for (nibble_id = 0; nibble_id < NR_NIBBLES_PER_BYTE;
		     ++nibble_id) {
			/* Get the nibble to 'transform' */
			u8 nibble_value =
				(byte_value >> (nibble_id << 2)) & 0xF;
			u8 nibble_transformed = 0;
			/* Get the nibble transformation to apply to nibble_value */
			u8 nibble_transformation =
				(config->dpu2cpu >> (nibble_id << 3)) & 0xFF;

			for (transformation_id = 0;
			     transformation_id < NR_BITS_PER_NIBBLE;
			     ++transformation_id) {
				/* Get the index of the bit 'transformation_id' into nibble_transformed */
				u8 id_bit = (nibble_transformation >>
					     (transformation_id << 1)) &
					    0x3;
				u8 bit_value =
					(nibble_value >> transformation_id) &
					0x1;

				nibble_transformed |= bit_value << id_bit;
			}

			byte_transformed |= nibble_transformed
					    << ((config->nibble_swap ?
							       ((nibble_id + 1) % 2) :
							       nibble_id)
						<< 2);
		}

		cpu_value |= byte_transformed << (byte_id << 3);
	}

	return cpu_value;
}

__API_SYMBOL__ u32 dpu_bit_config_cpu2dpu(struct dpu_bit_config *config,
					  u32 value)
{
	u32 dpu_value = 0;
	u8 byte_id, nibble_id, transformation_id;

	for (byte_id = 0; byte_id < sizeof(u32); ++byte_id) {
		u8 byte_value = (value >> (byte_id << 3)) & 0xFF;
		u8 byte_transformed = 0;

		for (nibble_id = 0; nibble_id < NR_NIBBLES_PER_BYTE;
		     ++nibble_id) {
			/* Get the nibble to 'transform' */
			u8 nibble_value =
				(byte_value >> (nibble_id << 2)) & 0xF;
			u8 nibble_transformed = 0;
			/* Get the nibble transformation to apply to nibble_value */
			u8 nibble_transformation =
				(config->cpu2dpu >> (nibble_id << 3)) & 0xFF;

			for (transformation_id = 0;
			     transformation_id < NR_BITS_PER_NIBBLE;
			     ++transformation_id) {
				/* Get the index of the bit 'transformation_id' into nibble_transformed */
				u8 id_bit = (nibble_transformation >>
					     (transformation_id << 1)) &
					    0x3;
				u8 bit_value =
					(nibble_value >> transformation_id) &
					0x1;

				nibble_transformed |= bit_value << id_bit;
			}

			byte_transformed |= nibble_transformed
					    << ((config->nibble_swap ?
							       ((nibble_id + 1) % 2) :
							       nibble_id)
						<< 2);
		}

		dpu_value |= byte_transformed << (byte_id << 3);
	}

	return dpu_value;
}

/* Permits to transform m/l syndrome into its reciprocal
 * function mm/ll.
 */
static u8 get_reciprocal(u8 code)
{
	u8 reciproc = 0;
	u8 i;

	for (i = 0; i < 4; i++) {
		u8 index = code & 0b11;
		code = code >> 2;
		reciproc |= i << (index << 1);
	}

	return reciproc;
}
