/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dpu_error.h>
#include <dpu_types.h>

dpu_error_t ci_copy_to_irams_rank(struct dpu_rank_t *rank,
				  iram_addr_t iram_instruction_index,
				  const dpuinstruction_t *source,
				  iram_size_t nb_of_instructions);

dpu_error_t ci_copy_to_irams_dpu(struct dpu_t *dpu,
				 iram_addr_t iram_instruction_index,
				 const dpuinstruction_t *source,
				 iram_size_t nb_of_instructions);

dpu_error_t ci_copy_to_irams_matrix(struct dpu_rank_t *rank,
				    struct dpu_transfer_matrix *matrix);

dpu_error_t ci_copy_from_irams_dpu(struct dpu_t *dpu,
				   dpuinstruction_t *destination,
				   iram_addr_t iram_instruction_index,
				   iram_size_t nb_of_instructions);

dpu_error_t ci_copy_from_irams_matrix(struct dpu_rank_t *rank,
				      struct dpu_transfer_matrix *matrix);

dpu_error_t ci_copy_to_mrams(struct dpu_rank_t *rank,
			     struct dpu_transfer_matrix *matrix);

dpu_error_t ci_copy_from_mrams(struct dpu_rank_t *rank,
			       struct dpu_transfer_matrix *matrix);

dpu_error_t ci_copy_to_wrams_rank(struct dpu_rank_t *rank,
				  wram_addr_t wram_word_offset,
				  const dpuword_t *source,
				  wram_size_t nb_of_words);

dpu_error_t ci_copy_to_wrams_dpu(struct dpu_t *dpu,
				 wram_addr_t wram_word_offset,
				 const dpuword_t *source,
				 wram_size_t nb_of_words);

dpu_error_t ci_copy_to_wrams_matrix(struct dpu_rank_t *rank,
				    struct dpu_transfer_matrix *matrix);

dpu_error_t ci_copy_from_wrams_dpu(struct dpu_t *dpu, dpuword_t *destination,
				   wram_addr_t wram_word_offset,
				   wram_size_t nb_of_words);

dpu_error_t ci_copy_from_wrams_matrix(struct dpu_rank_t *rank,
				      struct dpu_transfer_matrix *matrix);
