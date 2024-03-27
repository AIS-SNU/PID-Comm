/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dpu_transfer_matrix.h>
#include <dpu_error.h>
#include <dpu_rank.h>
#include <dpu_mask.h>

#include <dpu_management.h>
#include <dpu_internals.h>

#include <ufi_rank_utils.h>
#include <ufi/ufi.h>
#include <ufi/ufi_config.h>

__API_SYMBOL__ dpu_error_t ci_copy_to_irams_rank(
	struct dpu_rank_t *rank, iram_addr_t iram_instruction_index,
	const dpuinstruction_t *source, iram_size_t nb_of_instructions)
{
	dpu_error_t status;
	uint8_t mask = ALL_CIS;
	dpuinstruction_t *iram_array[DPU_MAX_NR_CIS] = {
		[0 ... DPU_MAX_NR_CIS - 1] = (dpuinstruction_t *)source
	};

	FF(ufi_select_all(rank, &mask));
	FF(ufi_iram_write(rank, mask, iram_array, iram_instruction_index,
			  nb_of_instructions));

end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_copy_to_irams_dpu(
	struct dpu_t *dpu, iram_addr_t iram_instruction_index,
	const dpuinstruction_t *source, iram_size_t nb_of_instructions)
{
	struct dpu_rank_t *rank = dpu->rank;

	dpu_error_t status;
	dpu_slice_id_t slice_id = dpu->slice_id;
	uint8_t mask = CI_MASK_ONE(slice_id);
	dpuinstruction_t *iram_array[DPU_MAX_NR_CIS];
	iram_array[slice_id] = (dpuinstruction_t *)source;

	FF(ufi_select_dpu(rank, &mask, dpu->dpu_id));
	FF(ufi_iram_write(rank, mask, iram_array, iram_instruction_index,
			  nb_of_instructions));

end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_copy_to_irams_matrix(
	struct dpu_rank_t *rank, struct dpu_transfer_matrix *matrix)
{
	dpu_error_t status;

	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t nr_dpus_per_ci =
		rank->description->hw.topology.nr_of_dpus_per_control_interface;

	uint32_t address = matrix->offset;
	uint32_t length = matrix->size;
	uint8_t each_ci;

	for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
		uint8_t each_dpu;
		for (each_dpu = 0; each_dpu < nr_dpus_per_ci; ++each_dpu) {
			struct dpu_t *dpu =
				DPU_GET_UNSAFE(rank, each_ci, each_dpu);
			void *buffer;

			if (!dpu->enabled) {
				continue;
			}

			buffer = matrix->ptr[get_transfer_matrix_index(
				rank, each_ci, each_dpu)];

			if (buffer == NULL) {
				continue;
			}

			if ((status = ci_copy_to_irams_dpu(dpu, address, buffer,
							   length)) != DPU_OK) {
				return status;
			}
		}
	}

	return status;
}

__API_SYMBOL__ dpu_error_t ci_copy_from_irams_dpu(
	struct dpu_t *dpu, dpuinstruction_t *destination,
	iram_addr_t iram_instruction_index, iram_size_t nb_of_instructions)
{
	struct dpu_rank_t *rank = dpu->rank;

	dpu_error_t status;
	dpu_slice_id_t slice_id = dpu->slice_id;
	uint8_t mask = CI_MASK_ONE(slice_id);
	dpuinstruction_t *iram_array[DPU_MAX_NR_CIS];
	iram_array[slice_id] = destination;

	FF(ufi_select_dpu(rank, &mask, dpu->dpu_id));
	FF(ufi_iram_read(rank, mask, iram_array, iram_instruction_index,
			 nb_of_instructions));

end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_copy_from_irams_matrix(
	struct dpu_rank_t *rank, struct dpu_transfer_matrix *matrix)
{
	dpu_error_t status;

	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t nr_dpus_per_ci =
		rank->description->hw.topology.nr_of_dpus_per_control_interface;

	uint32_t address = matrix->offset;
	uint32_t length = matrix->size;
	uint8_t each_ci;

	for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
		uint8_t each_dpu;
		for (each_dpu = 0; each_dpu < nr_dpus_per_ci; ++each_dpu) {
			struct dpu_t *dpu =
				DPU_GET_UNSAFE(rank, each_ci, each_dpu);
			void *buffer;

			if (!dpu->enabled) {
				continue;
			}

			buffer = matrix->ptr[get_transfer_matrix_index(
				rank, each_ci, each_dpu)];

			if (buffer == NULL) {
				continue;
			}

			if ((status = ci_copy_from_irams_dpu(
				     dpu, buffer, address, length)) != DPU_OK) {
				return status;
			}
		}
	}

	return status;
}

static bool
duplicate_transfer_matrix(struct dpu_rank_t *rank,
			  const struct dpu_transfer_matrix *transfer_matrix,
			  struct dpu_transfer_matrix *even_transfer_matrix,
			  struct dpu_transfer_matrix *odd_transfer_matrix)
{
	bool is_duplication_needed = false;

	LOG_RANK(VERBOSE, rank, "%p", transfer_matrix);

	if (rank->description->hw.topology.nr_of_dpus_per_control_interface <=
	    1)
		return false;

	/* MRAM mux is set by pair of DPUs: DPU0-DPU1, DPU2-DPU3, DPU4-DPU5, DPU6-DPU7 have the same mux state.
     * In the case all DPUs are stopped and the transfer is authorized, we must take care not overriding MRAMs
     * whose transfer in the matrix is not defined. But with the pair of DPUs as explained above, we must
     * duplicate the transfer if one DPU of a pair has a defined transfer and not the other. Let's take an example
     * (where '1' means there is a defined transfer and '0' no transfer for this DPU should be done):
     *
     *       CI    0     1     2     3     4     5     6     7
     *          -------------------------------------------------
     *  DPU0    |  1  |  1  |  1  |  1  |  1  |  0  |  1  |  1  |
     *  DPU1    |  1  |  1  |  1  |  1  |  1  |  1  |  1  |  1  |
     *                              ....
     *
     *  In this case, we must not override the MRAM of CI5:DPU0, so we must switch the mux DPU-side. But doing so, we
     *  also switch the mux for CI5:DPU1. But CI5:DPU1 has a defined transfer, then we cannot do this at the
     *  same time and hence the duplication of the matrix.
     *  This applies only if it is the job of the API to do that and in that case matrix duplication is MANDATORY since
     *  we don't know how the backend goes through the matrix, so we must prepare all the muxes so that one transfer
     *  matrix is correct.
     *
     *  So the initial transfer matrix must be duplicated at most 2 times, one for even DPUs, one for odd DPUs:
     *
     *       CI    0     1     2     3     4     5     6     7
     *          -------------------------------------------------
     *  DPU0    |  1  |  1  |  1  |  1  |  1  |  0  |  1  |  1  |
     *  DPU1    |  0  |  1  |  1  |  1  |  1  |  1  |  1  |  1  |
     *
     *  For the matrix above, for transfers to be correct, we must duplicate it this way:
     *
     *       CI    0     1     2     3     4     5     6     7
     *          -------------------------------------------------
     *  DPU0    |  1  |  1  |  1  |  1  |  1  |  0  |  1  |  1  |
     *  DPU1    |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |
     *
     *                              +
     *
     *       CI    0     1     2     3     4     5     6     7
     *          -------------------------------------------------
     *  DPU0    |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |
     *  DPU1    |  0  |  1  |  1  |  1  |  1  |  1  |  1  |  1  |
     *
     *  Which amounts to, once such a duplication is detected, to split the initial transfer matrix into 2 matrix,
     *  one containing the odd line and the other the even.
     */
	if (rank->description->configuration.api_must_switch_mram_mux) {
		dpu_member_id_t each_dpu;
		dpu_slice_id_t each_slice;
		/* Let's go through the matrix and search for pairs of DPUs whose one DPU has a defined transfer and the other one
         * has no transfer. If we find one such pair, let's duplicate the matrix.
         */
		for (each_dpu = 0;
		     each_dpu < rank->description->hw.topology
					.nr_of_dpus_per_control_interface;
		     each_dpu += 2) {
			for (each_slice = 0;
			     each_slice < rank->description->hw.topology
						  .nr_of_control_interfaces;
			     ++each_slice) {
				int idx_dpu_first, idx_dpu_second;

				idx_dpu_first = get_transfer_matrix_index(
					rank, each_slice, each_dpu);
				idx_dpu_second = get_transfer_matrix_index(
					rank, each_slice, each_dpu + 1);

				if (dpu_get(rank, each_slice, each_dpu)
					    ->enabled &&
				    dpu_get(rank, each_slice, each_dpu + 1)
					    ->enabled) {
					if ((!transfer_matrix
						      ->ptr[idx_dpu_first] ||
					     !transfer_matrix
						      ->ptr[idx_dpu_second]) &&
					    (transfer_matrix
						     ->ptr[idx_dpu_first] ||
					     transfer_matrix
						     ->ptr[idx_dpu_second])) {
						dpu_member_id_t dpu_id_notnull =
							transfer_matrix->ptr
									[idx_dpu_first] ?
								      each_dpu :
								      (dpu_member_id_t)(each_dpu +
										  1);

						LOG_RANK(
							VERBOSE, rank,
							"Duplicating transfer matrix since DPU (%d, %d) of the pair (%d, %d) has a "
							"defined transfer whereas the other does not.",
							dpu_id_notnull,
							each_slice, each_dpu,
							each_dpu + 1);

						is_duplication_needed = true;
						break;
					}
				}
			}

			if (is_duplication_needed)
				break;
		}

		if (is_duplication_needed) {
			for (each_dpu = 0;
			     each_dpu <
			     rank->description->hw.topology
				     .nr_of_dpus_per_control_interface;
			     each_dpu += 2) {
				for (each_slice = 0;
				     each_slice <
				     rank->description->hw.topology
					     .nr_of_control_interfaces;
				     ++each_slice) {
					struct dpu_t *first_dpu =
						DPU_GET_UNSAFE(rank, each_slice,
							       each_dpu);
					struct dpu_t *second_dpu =
						DPU_GET_UNSAFE(rank, each_slice,
							       each_dpu + 1);

					int first_dpu_idx =
						get_transfer_matrix_index(
							rank, each_slice,
							each_dpu);
					int second_dpu_idx =
						get_transfer_matrix_index(
							rank, each_slice,
							each_dpu + 1);

					dpu_transfer_matrix_add_dpu(
						first_dpu, even_transfer_matrix,
						transfer_matrix
							->ptr[first_dpu_idx]);

					dpu_transfer_matrix_add_dpu(
						second_dpu, odd_transfer_matrix,
						transfer_matrix
							->ptr[second_dpu_idx]);
				}
			}
		}
	}

	return is_duplication_needed;
}

static bool
is_transfer_matrix_full(struct dpu_rank_t *rank,
			const struct dpu_transfer_matrix *transfer_matrix)
{
	uint8_t nr_of_dpus_per_ci =
		rank->description->hw.topology.nr_of_dpus_per_control_interface;
	uint8_t nr_of_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t each_slice;
	uint8_t each_dpu;

	LOG_RANK(VERBOSE, rank, "%p", transfer_matrix);

	for (each_slice = 0; each_slice < nr_of_cis; ++each_slice) {
		dpu_selected_mask_t enabled_dpus =
			rank->runtime.control_interface.slice_info[each_slice]
				.enabled_dpus;

		for (each_dpu = 0; each_dpu < nr_of_dpus_per_ci; ++each_dpu) {
			int idx;

			if (!dpu_mask_is_selected(enabled_dpus, each_dpu))
				continue;

			idx = get_transfer_matrix_index(rank, each_slice,
							each_dpu);

			if (!transfer_matrix->ptr[idx] &&
			    dpu_get(rank, each_slice, each_dpu)->enabled)
				return false;
		}
	}

	return true;
}

/* This function assumes duplication was already done since it does not handle
 * cases where pairs of dpus have incompatible mux positions.
 * Do care about pairs of dpus. See issue #279.
 */
static dpu_error_t host_get_access_for_transfer_matrix(
	struct dpu_rank_t *rank,
	const struct dpu_transfer_matrix *transfer_matrix)
{
	dpu_error_t status = DPU_OK;
	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t nr_dpus_per_ci =
		rank->description->hw.topology.nr_of_dpus_per_control_interface;
	dpu_member_id_t each_dpu;

	for (each_dpu = 0; each_dpu < nr_dpus_per_ci; each_dpu += 2) {
		dpu_slice_id_t each_slice;
		uint8_t mask = 0;

		for (each_slice = 0; each_slice < nr_cis; ++each_slice) {
			int idx_first_dpu = get_transfer_matrix_index(
				rank, each_slice, each_dpu);
			int idx_second_dpu = get_transfer_matrix_index(
				rank, each_slice, each_dpu + 1);

			uint8_t get_mux_first_dpu =
				(transfer_matrix->ptr[idx_first_dpu] != NULL);
			uint8_t get_mux_second_dpu =
				(transfer_matrix->ptr[idx_second_dpu] != NULL);

			if (dpu_get(rank, each_slice, each_dpu)->enabled) {
				mask |= get_mux_first_dpu << each_slice;
			} else if (dpu_get(rank, each_slice, each_dpu + 1)
					   ->enabled) {
				mask |= get_mux_second_dpu << each_slice;
			}
		}

		if (mask) {
			FF(dpu_switch_mux_for_dpu_line(rank, each_dpu, mask));
			FF(dpu_switch_mux_for_dpu_line(rank, each_dpu + 1,
						       mask));
		}
	}

end:
	return status;
}

static dpu_error_t
host_get_access_for_transfer(struct dpu_rank_t *rank,
			     const struct dpu_transfer_matrix *transfer_matrix)
{
	dpu_error_t status;
	bool is_full_matrix = is_transfer_matrix_full(rank, transfer_matrix);

	LOG_RANK(VERBOSE, rank, "");

	if (is_full_matrix)
		FF(dpu_switch_mux_for_rank(rank, true));
	else
		FF(host_get_access_for_transfer_matrix(rank, transfer_matrix));

end:
	return status;
}

static dpu_error_t do_mram_transfer(struct dpu_rank_t *rank,
				    dpu_transfer_type_t type,
				    struct dpu_transfer_matrix *matrix)
{
	dpu_error_t status = DPU_OK;
	dpu_rank_handler_t handler = rank->handler_context->handler;

	uintptr_t host_ptr = 0;
	unsigned each_dpu;

	LOG_RANK(DEBUG, rank, "");

	for (each_dpu = 0; each_dpu < MAX_NR_DPUS_PER_RANK; each_dpu++) {
		host_ptr |= (uintptr_t)matrix->ptr[each_dpu];
	}
	if (!host_ptr) {
		LOG_RANK(
			DEBUG, rank,
			"transfer matrix does not contain any host ptr, do nothing");
		return status;
	}

	switch (type) {
	case DPU_TRANSFER_FROM_MRAM:
		if (handler->copy_from_rank(rank, matrix) != DPU_RANK_SUCCESS) {
			status = DPU_ERR_DRIVER;
		}
		break;
	case DPU_TRANSFER_TO_MRAM:
		if (handler->copy_to_rank(rank, matrix) != DPU_RANK_SUCCESS) {
			status = DPU_ERR_DRIVER;
		}
		break;

	default:
		status = DPU_ERR_INTERNAL;
		break;
	}

	return status;
}

static dpu_error_t
copy_mram_for_dpus(struct dpu_rank_t *rank, dpu_transfer_type_t type,
		   const struct dpu_transfer_matrix *transfer_matrix)
{
	dpu_error_t status;
	struct dpu_transfer_matrix even_transfer_matrix, odd_transfer_matrix;
	bool is_duplication_needed = false;

	LOG_RANK(VERBOSE, rank, "");

	dpu_transfer_matrix_clear_all(rank, &even_transfer_matrix);
	dpu_transfer_matrix_clear_all(rank, &odd_transfer_matrix);

	is_duplication_needed = duplicate_transfer_matrix(rank, transfer_matrix,
							  &even_transfer_matrix,
							  &odd_transfer_matrix);

	if (!is_duplication_needed) {
		FF(host_get_access_for_transfer(rank, transfer_matrix));

		FF(do_mram_transfer(
			rank, type,
			(struct dpu_transfer_matrix *)transfer_matrix));
	} else {
		even_transfer_matrix.size = transfer_matrix->size;
		even_transfer_matrix.offset = transfer_matrix->offset;
		FF(host_get_access_for_transfer_matrix(rank,
						       &even_transfer_matrix));
		FF(do_mram_transfer(rank, type, &even_transfer_matrix));

		odd_transfer_matrix.size = transfer_matrix->size;
		odd_transfer_matrix.offset = transfer_matrix->offset;
		FF(host_get_access_for_transfer_matrix(rank,
						       &odd_transfer_matrix));
		FF(do_mram_transfer(rank, type, &odd_transfer_matrix));
	}
end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_copy_to_mrams(struct dpu_rank_t *rank,
					    struct dpu_transfer_matrix *matrix)
{
	return copy_mram_for_dpus(rank, DPU_TRANSFER_TO_MRAM, matrix);
}

__API_SYMBOL__ dpu_error_t
ci_copy_from_mrams(struct dpu_rank_t *rank, struct dpu_transfer_matrix *matrix)
{
	return copy_mram_for_dpus(rank, DPU_TRANSFER_FROM_MRAM, matrix);
}

__API_SYMBOL__ dpu_error_t ci_copy_to_wrams_rank(struct dpu_rank_t *rank,
						 wram_addr_t wram_word_offset,
						 const dpuword_t *source,
						 wram_size_t nb_of_words)
{
	dpu_error_t status;
	uint8_t mask = ALL_CIS;
	dpuword_t *wram_array[DPU_MAX_NR_CIS] = { [0 ... DPU_MAX_NR_CIS - 1] =
							  (dpuword_t *)source };

	FF(ufi_select_all(rank, &mask));
	FF(ufi_wram_write(rank, mask, wram_array, wram_word_offset,
			  nb_of_words));

end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_copy_to_wrams_dpu(struct dpu_t *dpu,
						wram_addr_t wram_word_offset,
						const dpuword_t *source,
						wram_size_t nb_of_words)
{
	struct dpu_rank_t *rank = dpu->rank;
	dpu_error_t status;
	dpu_slice_id_t slice_id = dpu->slice_id;
	uint8_t mask = CI_MASK_ONE(slice_id);
	dpuword_t *wram_array[DPU_MAX_NR_CIS];
	wram_array[slice_id] = (dpuword_t *)source;

	FF(ufi_select_dpu(rank, &mask, dpu->dpu_id));
	FF(ufi_wram_write(rank, mask, wram_array, wram_word_offset,
			  nb_of_words));

end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_copy_to_wrams_matrix(
	struct dpu_rank_t *rank, struct dpu_transfer_matrix *matrix)
{
	uint32_t wram_word_offset = matrix->offset;
	uint32_t nb_of_words = matrix->size;
	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t nr_dpus_per_ci =
		rank->description->hw.topology.nr_of_dpus_per_control_interface;
	dpu_error_t status;

	dpuword_t *wram_array[DPU_MAX_NR_CIS] = { 0 };
	dpu_member_id_t each_dpu;

	for (each_dpu = 0; each_dpu < nr_dpus_per_ci; ++each_dpu) {
		dpu_slice_id_t each_ci;
		uint8_t mask = 0;

		for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
			dpuword_t *src =
				(dpuword_t *)
					matrix->ptr[get_transfer_matrix_index(
						rank, each_ci, each_dpu)];

			if (src != NULL) {
				wram_array[each_ci] = src;
				mask |= CI_MASK_ONE(each_ci);
			}
		}

		if (mask != 0) {
			FF(ufi_select_dpu(rank, &mask, each_dpu));
			FF(ufi_wram_write(rank, mask, wram_array,
					  wram_word_offset, nb_of_words));
		}
	}

end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_copy_from_wrams_dpu(struct dpu_t *dpu,
						  dpuword_t *destination,
						  wram_addr_t wram_word_offset,
						  wram_size_t nb_of_words)
{
	struct dpu_rank_t *rank = dpu->rank;

	dpu_error_t status;
	dpu_slice_id_t slice_id = dpu->slice_id;
	uint8_t mask = CI_MASK_ONE(slice_id);
	dpuword_t *wram_array[DPU_MAX_NR_CIS];
	wram_array[slice_id] = destination;

	FF(ufi_select_dpu(rank, &mask, dpu->dpu_id));
	FF(ufi_wram_read(rank, mask, wram_array, wram_word_offset,
			 nb_of_words));

end:
	return status;
}

__API_SYMBOL__ dpu_error_t ci_copy_from_wrams_matrix(
	struct dpu_rank_t *rank, struct dpu_transfer_matrix *matrix)
{
	uint32_t wram_word_offset = matrix->offset;
	uint32_t nb_of_words = matrix->size;

	uint8_t nr_cis =
		rank->description->hw.topology.nr_of_control_interfaces;
	uint8_t nr_dpus_per_ci =
		rank->description->hw.topology.nr_of_dpus_per_control_interface;
	dpu_error_t status;

	dpuword_t *wram_array[DPU_MAX_NR_CIS] = { 0 };
	dpu_member_id_t each_dpu;

	for (each_dpu = 0; each_dpu < nr_dpus_per_ci; ++each_dpu) {
		dpu_slice_id_t each_ci;
		uint8_t mask = 0;

		for (each_ci = 0; each_ci < nr_cis; ++each_ci) {
			dpuword_t *dst =
				(dpuword_t *)
					matrix->ptr[get_transfer_matrix_index(
						rank, each_ci, each_dpu)];

			if (dst != NULL) {
				wram_array[each_ci] = dst;
				mask |= CI_MASK_ONE(each_ci);
			}
		}

		if (mask != 0) {
			FF(ufi_select_dpu(rank, &mask, each_dpu));
			FF(ufi_wram_read(rank, mask, wram_array,
					 wram_word_offset, nb_of_words));
		}
	}

end:
	return status;
}
