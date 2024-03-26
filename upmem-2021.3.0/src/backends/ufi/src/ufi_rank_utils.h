/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __UFI_RANK_UTILS_H__
#define __UFI_RANK_UTILS_H__

#include <dpu_attributes.h>
#include <dpu_internals.h>
#include <dpu_rank.h>
#include <dpu_log_utils.h>
#include <static_verbose.h>
#include <time.h>

#define GET_CMDS(r) ((r)->cmds)
#define GET_DESC_HW(r) (&(r)->description->hw)
#define GET_CI_CONTEXT(r) (&(r)->runtime.control_interface)
#define GET_HANDLER(r) ((r)->handler_context->handler)
#define GET_DEBUG(r) (&(r)->debug)

#define _LOGD_RANK(rank, fmt, ...) LOG_RANK(DEBUG, rank, fmt, __VA_ARGS__)
#define _LOGV_RANK(rank, fmt, ...) LOG_RANK(VERBOSE, rank, fmt, __VA_ARGS__)

static struct verbose_control *this_vc;
static inline struct verbose_control *__vc(void)
{
	if (this_vc == NULL) {
		this_vc = get_verbose_control_for("ufi");
	}
	return this_vc;
}
static struct verbose_control *temp_vc;
static inline struct verbose_control *__temp_vc(void)
{
	if (temp_vc == NULL) {
		temp_vc = get_verbose_control_for("temp");
	}
	return temp_vc;
}

static inline const char *temperature_to_string(enum dpu_temperature temp)
{
	switch (temp) {
	case DPU_TEMPERATURE_LESS_THAN_50:
		return "< 50C";
	case DPU_TEMPERATURE_BETWEEN_50_AND_60:
		return "[50-60C]";
	case DPU_TEMPERATURE_BETWEEN_60_AND_70:
		return "[60-70C]";
	case DPU_TEMPERATURE_BETWEEN_70_AND_80:
		return "[70-80C]";
	case DPU_TEMPERATURE_BETWEEN_80_AND_90:
		return "[80-90C]";
	case DPU_TEMPERATURE_BETWEEN_90_AND_100:
		return "[90-100C]";
	case DPU_TEMPERATURE_BETWEEN_100_AND_110:
		return "[100-110C]";
	case DPU_TEMPERATURE_GREATER_THAN_110:
		return "> 110C";
	default:
		return "UNSTABLE";
	}
}

static inline bool update_rank_temperature_sample_time(struct dpu_rank_t *rank)
{
	struct timespec latest = rank->temperature_sample_time;
	struct timespec now;
	double diff;

	clock_gettime(CLOCK_MONOTONIC, &now);

	diff = (now.tv_sec - latest.tv_sec) +
	       ((now.tv_nsec - latest.tv_nsec) * 1E-9);

	if (diff >= 1.0) {
		rank->temperature_sample_time = now;
		return true;
	}

	return false;
}

#define WRITE_DIR 'W'
#define READ_DIR 'R'

#define PACKET_LOG_TEMPLATE                                                    \
	"XXXXXXXXXXXXXXXX XXXXXXXXXXXXXXXX XXXXXXXXXXXXXXXX XXXXXXXXXXXXXXXX " \
	"XXXXXXXXXXXXXXXX XXXXXXXXXXXXXXXX XXXXXXXXXXXXXXXX XXXXXXXXXXXXXXXX"

#define LOGV_PACKET(rank, data, type) LOG_PACKET(rank, data, type, LOGV)

#define LOGD_PACKET(rank, data, type) LOG_PACKET(rank, data, type, LOGD)

#define LOG_PACKET(rank, data, type, log)                                      \
	do {                                                                   \
		char packet_str[strlen(PACKET_LOG_TEMPLATE) + 1];              \
		u8 nr_cis =                                                    \
			GET_DESC_HW(rank)->topology.nr_of_control_interfaces;  \
		u8 each_ci;                                                    \
		if (_CONCAT(log, _ENABLED)(__vc())) {                          \
			for (each_ci = 0; each_ci < nr_cis; ++each_ci) {       \
				sprintf(packet_str + each_ci * 17, "%016lx%s", \
					data[each_ci],                         \
					(each_ci != (nr_cis - 1)) ? " " : ""); \
			}                                                      \
			_CONCAT(_, _CONCAT(log, _RANK))                        \
			(rank, "[%c] %s", type, packet_str);                   \
		}                                                              \
	} while (0)

#define LOGD_LAST_COMMANDS(r)                                                  \
	do {                                                                   \
		struct dpu_debug_context_t *debug = GET_DEBUG(r);              \
		u32 idx = debug->cmds_buffer.has_wrapped ?                     \
					debug->cmds_buffer.idx_last :                \
					0;                                           \
		u32 nb_cmd = debug->cmds_buffer.nb, nb;                        \
		u32 size = debug->cmds_buffer.size;                            \
		LOG_RANK(DEBUG, r, "==================================");      \
		LOG_RANK(DEBUG, r, "Timeout, dumping commands buffer :");      \
		for (nb = 0; nb < nb_cmd; idx = (idx + 1) % size, ++nb) {      \
			LOGD_PACKET(r, debug->cmds_buffer.cmds[idx].data,      \
				    debug->cmds_buffer.cmds[idx].direction);   \
		}                                                              \
		LOG_RANK(DEBUG, r, "==================================");      \
	} while (0)

#define TEMP_LOG_TEMPLATE                                                      \
	"XXXXXXXXXX XXXXXXXXXX XXXXXXXXXX XXXXXXXXXX "                         \
	"XXXXXXXXXX XXXXXXXXXX XXXXXXXXXX XXXXXXXXXX"
#define UNAVAILABLE_TEMPERATURE_STR "----------"

#define LOG_TEMPERATURE_ENABLED() (LOGI_ENABLED(__temp_vc()))

#define LOG_TEMPERATURE_TRIGGERED(rank)                                        \
	update_rank_temperature_sample_time(rank)

#define LOG_TEMPERATURE(rank, data)                                            \
	do {                                                                   \
		char packet_str[strlen(TEMP_LOG_TEMPLATE) + 1];                \
		u8 _nr_cis =                                                   \
			GET_DESC_HW(rank)->topology.nr_of_control_interfaces;  \
		u8 _each_ci;                                                   \
		for (_each_ci = 0; _each_ci < _nr_cis; ++_each_ci) {           \
			if (data[_each_ci] != CI_EMPTY) {                      \
				enum dpu_temperature _temp =                   \
					__builtin_popcount(                    \
						(data[_each_ci] >> 40) &       \
						0xff);                         \
				sprintf(packet_str + _each_ci * 11, "%-16s%s", \
					temperature_to_string(_temp),          \
					(_each_ci != (_nr_cis - 1)) ? " " :    \
									    "");     \
			} else {                                               \
				sprintf(packet_str + _each_ci * 11,            \
					UNAVAILABLE_TEMPERATURE_STR "%s",      \
					(_each_ci != (_nr_cis - 1)) ? " " :    \
									    "");     \
			}                                                      \
		}                                                              \
		LOG(INFO)                                                      \
		(__temp_vc(), "[" LOG_FMT_RANK "] %s: %s", rank->rank_id,      \
		 __func__, packet_str);                                        \
	} while (0)

static inline uint32_t debug_record_last_cmd(struct dpu_rank_t *rank,
					     char direction, uint64_t *commands)
{
	struct dpu_debug_context_t *debug = GET_DEBUG(rank);
	uint8_t nr_cis = GET_DESC_HW(rank)->topology.nr_of_control_interfaces;
	uint32_t idx_last;

	if (!LOGD_ENABLED(__vc()))
		return DPU_OK;

	idx_last = debug->cmds_buffer.idx_last;

	memcpy(debug->cmds_buffer.cmds[idx_last].data, commands,
	       nr_cis * sizeof(uint64_t));
	debug->cmds_buffer.cmds[idx_last].direction = direction;

	debug->cmds_buffer.idx_last++;

	if (!debug->cmds_buffer.has_wrapped) {
		if (debug->cmds_buffer.idx_last >= debug->cmds_buffer.size) {
			debug->cmds_buffer.has_wrapped = true;
			debug->cmds_buffer.nb = debug->cmds_buffer.size;
		} else {
			debug->cmds_buffer.nb++;
		}
	}

	debug->cmds_buffer.idx_last %= debug->cmds_buffer.size;

	return DPU_OK;
}

#define do_div(n, base)                                                        \
	({                                                                     \
		uint32_t __base = (base);                                      \
		uint32_t __rem;                                                \
		__rem = ((uint64_t)(n)) % __base;                              \
		(n) = ((uint64_t)(n)) / __base;                                \
		__rem;                                                         \
	})

#define DIV_ROUND_DOWN_ULL(ll, d)                                              \
	({                                                                     \
		unsigned long long _tmp = (ll);                                \
		do_div(_tmp, d);                                               \
		_tmp;                                                          \
	})

#define DIV_ROUND_UP_ULL(ll, d)                                                \
	DIV_ROUND_DOWN_ULL((unsigned long long)(ll) + (d)-1, (d))

#endif // __UFI_RANK_UTILS_H__
