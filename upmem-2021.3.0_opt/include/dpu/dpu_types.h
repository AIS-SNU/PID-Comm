/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DPU_TYPES_H
#define DPU_TYPES_H

/**
 * @file dpu_types.h
 * @brief Base DPU types in the C API.
 */

#include <stdint.h>
#include <pthread.h>

#include <dpu_target.h>

/**
 * @brief Maximum number of Control Interfaces in a DPU rank.
 */
#define DPU_MAX_NR_CIS 8

/**
 * @brief Maximum number of DPUs in a Control Interface.
 */
#define DPU_MAX_NR_DPUS_PER_CI 8

/**
 * @brief Number of the DPU thread which is launched when booting the DPU.
 */
#define DPU_BOOT_THREAD 0

/**
 * @brief Maximum number of groups in a Control Interface.
 */
#define DPU_MAX_NR_GROUPS 8

/**
 * @brief ID of a DPU rank.
 */
typedef uint16_t dpu_rank_id_t;

/**
 * @brief ID of a DPU.
 */
typedef uint32_t dpu_id_t;

/**
 * @brief ID of a DPU rank slice.
 */
typedef uint8_t dpu_slice_id_t;

/**
 * @brief ID of a DPU rank slice member.
 */
typedef uint8_t dpu_member_id_t;

/**
 * @brief ID of a DPU rank slice group.
 */
typedef uint8_t dpu_group_id_t;

/**
 * @brief Index of a DPU thread.
 */
typedef uint8_t dpu_thread_t;

/**
 * @brief Index of a DPU notify bit.
 */
typedef uint8_t dpu_notify_bit_id_t;

/**
 * @brief Address in IRAM.
 */
typedef uint16_t iram_addr_t;

/**
 * @brief Address in WRAM.
 */
typedef uint32_t wram_addr_t;

/**
 * @brief Address in MRAM.
 */
typedef uint32_t mram_addr_t;

/**
 * @brief Bigger storage for a DPU memory address.
 */
typedef mram_addr_t dpu_mem_max_addr_t;

/**
 * @brief Size in IRAM.
 */
typedef uint16_t iram_size_t;

/**
 * @brief Size in WRAM.
 */
typedef uint32_t wram_size_t;

/**
 * @brief Size in MRAM.
 */
typedef uint32_t mram_size_t;

/**
 * @brief Bigger storage for a DPU memory size.
 */
typedef mram_size_t dpu_mem_max_size_t;

/**
 * @brief DPU instruction.
 */
typedef uint64_t dpuinstruction_t;

/**
 * @brief DPU word in WRAM.
 */
typedef uint32_t dpuword_t;

/**
 * @brief Bitfield of DPUs in a CI.
 */
typedef uint8_t dpu_bitfield_t;

/**
 * @brief Bitfield representing all DPUs of a CI.
 * @private
 */
#define DPU_BITFIELD_ALL ((dpu_bitfield_t)-1)

/**
 * @brief Bitfield of CIs in a rank.
 */
typedef uint8_t dpu_ci_bitfield_t;

/**
 * @brief Clock divisor for the DPU clock.
 */
typedef enum _dpu_clock_division_t {
    DPU_CLOCK_DIV8 = 0x0,
    DPU_CLOCK_DIV4 = 0x4,
    DPU_CLOCK_DIV3 = 0x3,
    DPU_CLOCK_DIV2 = 0x8,
} dpu_clock_division_t;

/**
 * @brief Target type for a CI Select command.
 */
enum dpu_slice_target_type {
    DPU_SLICE_TARGET_NONE,
    DPU_SLICE_TARGET_DPU,
    DPU_SLICE_TARGET_ALL,
    DPU_SLICE_TARGET_GROUP,
    NR_OF_DPU_SLICE_TARGETS
};

/**
 * @brief Target for a CI Select command.
 */
struct dpu_slice_target {
    /** The type of target. */
    enum dpu_slice_target_type type;
    /** Specific context depending on the slice target type. */
    union {
        /** The selected DPU ID, if any. */
        dpu_member_id_t dpu_id;
        /** The selected group ID, if any. */
        dpu_group_id_t group_id;
    };
};

/**
 * @brief Transform a slice target type to its associated string.
 * @private
 * @hideinitializer
 */
#define DPU_SLICE_TARGET_TYPE_NAME(target_type)                                                                                  \
    ((((uint32_t)(target_type)) < NR_OF_DPU_SLICE_TARGETS) ? dpu_slice_target_names[target_type]                                 \
                                                           : "DPU_SLICE_TARGET_TYPE_UNKNOWN")
/**
 * @brief Strings associated to the slice target types.
 * @private
 */
extern const char *dpu_slice_target_names[NR_OF_DPU_SLICE_TARGETS];

/**
 * @brief High-level event that may be passed to the backends.
 */
typedef enum __attribute__((packed)) _dpu_event_kind_t {
    DPU_EVENT_RESET,
    DPU_EVENT_EXTRACT_CONTEXT,
    DPU_EVENT_RESTORE_CONTEXT,
    DPU_EVENT_MRAM_ACCESS_PROGRAM,
    DPU_EVENT_LOAD_PROGRAM,
    DPU_EVENT_DEBUG_ACTION,
} dpu_event_kind_t;

/**
 * @brief Attribute used on deprecated functions.
 * @hideinitializer
 */
#define DPU_API_DEPRECATED __attribute__((deprecated))

/**
 * @brief Name of the symbol pointing at the start of the unused MRAM in a DPU program.
 * @hideinitializer
 */
#define DPU_MRAM_HEAP_POINTER_NAME "__sys_used_mram_end"

/**
 * @brief DPU rank context.
 */
struct dpu_rank_t;

/**
 * @brief DPU context.
 */
struct dpu_t;

/**
 * @brief DPU memory transfer context.
 */
struct dpu_transfer_matrix;

/**
 * @brief DPU device type of a DPU set.
 */
typedef enum _dpu_set_kind_t {
    /** Set is composed of DPU ranks. */
    DPU_SET_RANKS,
    /** Set is composed of one DPU. */
    DPU_SET_DPU,
} dpu_set_kind_t;


typedef struct rotate_n_stream_job_t
{
    float job_priority; // deprecate
    int src_rank;
    int dst_rank;
    struct dpu_set_t* src_rank_dpu_set;
    struct dpu_set_t* dst_rank_dpu_set;
    int mram_src_offset;
    int mram_dst_offset; // address of MRAM
    int job_type;
    float src_packet_num;
    void* host_buffer;
    int64_t xfer_bytes; // host<->PIM tranfer size per rank
    void* idp_handler;
    void* job_queue_ptr;
    struct rotate_n_stream_job_t* next_job;
    int packet_size; // 8  16 32 64 128
    bool cache_bypass;
    int prefetch_distance;
    int bankchunk_8_offset; // num_bankchunk / 8, data with same amount of rotation. Also used as interleave offset at max BW test -JH
    int num_job_per_rank;
    int num_repeat; // to test rnc xfer size per DPU bigger than DPU WRAM size - Comment
    // deprecate
    int* src_histogram; // contains 64 
    int* dst_histogram; // contains 8

    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    int *running_jobs_per_rank; // to wait until all xfer jobs sent produced by a rank thread
} rotate_n_stream_job_t;

#ifndef NUM_MAX_RANKS
#define NUM_MAX_RANKS 40
#endif

#define  DPU_TRANSFER_JOB_TYPE_NO_OP 0
#define  DPU_TRANSFER_JOB_TYPE_ROTATE_AND_BYPASS_PACKET_SIZE_128B 1
#define  DPU_TRANSFER_JOB_TYPE_ROTATE_AND_BYPASS_PACKET_SIZE_64B 2
#define  DPU_TRANSFER_JOB_TYPE_ROTATE_AND_BYPASS_PACKET_SIZE_32B 3
#define  DPU_TRANSFER_JOB_TYPE_ROTATE_AND_BYPASS_PACKET_SIZE_16B 4
#define  DPU_TRANSFER_JOB_TYPE_ROTATE_AND_BYPASS_PACKET_SIZE_8B 5
#define  DPU_TRANSFER_JOB_TYPE_LINEAR_ACCESS_TEST_NORMAL 6
#define  DPU_TRANSFER_JOB_TYPE_LINEAR_ACCESS_TEST_RANDOM 7
#define  DPU_TRANSFER_JOB_TYPE_LINEAR_ACCESS_TEST_SKIP 8
#define  DPU_TRANSFER_JOB_TYPE_ROTATE_AND_BYPASS_MT 9
#define  DPU_TRANSFER_JOB_TYPE_LINEAR_ACCESS_TEST_INTERLEAVE 10
#define  DPU_TRANSFER_JOB_TYPE_UNORDERED_SCATTER 11
#define  DPU_TRANSFER_JOB_TYPE_UNORDERED_GATHER 12
#define  DPU_TRANSFER_JOB_TYPE_CONVENTIONAL_SHUFFLE_ST 13

#define DPU_TRANSFER_JOB_TYPE_ROTATE_AND_BYPASS_UNROLL 14
#define DPU_TRANSFER_JOB_TYPE_TRANSPOSE_SHUFFLE_TRANSPOSE_UNROLL 15
#define DPU_TRANSFER_JOB_TYPE_ROTATE_AND_PREFETCH_UNROLL 16

#define DPU_TRANSFER_JOB_TYPE_RANDOM_ACCESS_TEST 17
/**
 * @brief A set of DPU devices.
 */
struct dpu_set_t {
    /** DPU device type for the DPU set. */
    dpu_set_kind_t kind;
    /** Specific context depending on the DPU set kind. */
    union {
        /** Structure when the DPU set is composed of ranks. */
        struct {
            /** Number of ranks in the DPU set. */
            uint32_t nr_ranks;
            /** Ranks in the DPU set. */
            struct dpu_rank_t **ranks;
        } list;
        /** Structure when the DPU set is composed of one DPU. */
        struct dpu_t *dpu;
    };
};


typedef struct RNS_Job_Queue_t
{
    struct dpu_rank_t* dpu_rank_strcts[NUM_MAX_RANKS];
    struct dpu_region_address_translation* trans[NUM_MAX_RANKS];
    uint8_t* ptr_regions[NUM_MAX_RANKS];
    rotate_n_stream_job_t* head_job;
    rotate_n_stream_job_t* tail_job;
    int curr_job_num;
    int num_ranks;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int job_done;
    pthread_t worker_threads_numa[NUM_MAX_RANKS];
    int worker_num; // -JH
    struct dpu_set_t* rank_sets[NUM_MAX_RANKS];
    void* join_instance;
    // Functions
    rotate_n_stream_job_t* (*rnc_queue_next_job)(struct RNS_Job_Queue_t* job_queue); // function pointer allocation at IDPHandler::InitRNS - Comment
    void (*rnc_nqueue_add_job)(struct RNS_Job_Queue_t* job_queue, const rotate_n_stream_job_t* new_job);
    void (*return_job)(struct RNS_Job_Queue_t* job_queue, const rotate_n_stream_job_t* ended_job);
    // Thread Controlling vars
    int total__;
    int ROTATE_AND_STREAM_JOBS;
    int curr_priority__;
    // 
    void* job_queue_stl_ptr; // ptr to RNS_job_priority_queue_t, where jobs are really queued - Comment

    uint8_t channel_ids[NUM_MAX_RANKS]; // -JH
    int numa_node_ids[NUM_MAX_RANKS]; // -JH
} RNS_Job_Queue_t;

/**
 * @brief DPU program information.
 * */
struct dpu_program_t;

/**
 * @brief Information for a symbol from a DPU program.
 */
struct dpu_symbol_t {
    /** Memory address of the symbol. */
    dpu_mem_max_addr_t address;
    /** Size in bytes of the symbol. */
    dpu_mem_max_size_t size;
};

/**
 * @brief Information on a binary embedded in the program with "DPU_INCBIN"
 */
struct dpu_incbin_t {
    /** Program bytes. */
    uint8_t *buffer;
    /** Number of bytes in the program. */
    size_t size;
    /** Underlying file name for the program, if any. */
    const char *path;
};

/**
 * @brief Configuration used to handle bit shuffling.
 */
struct dpu_bit_config {
    /** Configuration when writing data. */
    uint16_t cpu2dpu;
    /** Configuration when reading data. */
    uint16_t dpu2cpu;
    /** Whether the nibbles of a byte are swapped. */
    uint8_t nibble_swap;
    /** Whether the stutter mechanism is enabled. */
    uint8_t stutter;
};

/**
 * @brief Timing configuration for CI <-> DPU communication.
 */
struct dpu_carousel_config {
    /** Number of cycles during which a command is valid. */
    uint8_t cmd_duration;
    /** Number of cycles between each command sampling. */
    uint8_t cmd_sampling;
    /** Number of cycles during which a result is valid. */
    uint8_t res_duration;
    /** Number of cycles between each result sampling. */
    uint8_t res_sampling;
};

/**
 * @brief Memory bank reparation configuration.
 */
struct dpu_repair_config {
    /** MSBs for the repaired addresses A & B. */
    uint8_t AB_msbs;
    /** MSBs for the repaired addresses C & D. */
    uint8_t CD_msbs;
    /** LSBs for the repaired address A. */
    uint8_t A_lsbs;
    /** LSBs for the repaired address B. */
    uint8_t B_lsbs;
    /** LSBs for the repaired address C. */
    uint8_t C_lsbs;
    /** LSBs for the repaired address D. */
    uint8_t D_lsbs;
    /** Even bit index repaired. */
    uint8_t even_index;
    /** Odd bit index repaired. */
    uint8_t odd_index;
};

/**
 * @brief Temperature measure, in degree Celsius, in a DPU.
 */
enum dpu_temperature {
    DPU_TEMPERATURE_LESS_THAN_50 = 0,
    DPU_TEMPERATURE_BETWEEN_50_AND_60 = 1,
    DPU_TEMPERATURE_BETWEEN_60_AND_70 = 2,
    DPU_TEMPERATURE_BETWEEN_70_AND_80 = 3,
    DPU_TEMPERATURE_BETWEEN_80_AND_90 = 4,
    DPU_TEMPERATURE_BETWEEN_90_AND_100 = 5,
    DPU_TEMPERATURE_BETWEEN_100_AND_110 = 6,
    DPU_TEMPERATURE_GREATER_THAN_110 = 7,
};

/**
 * @brief Configuration for the instruction address size on a DPU.
 */
enum dpu_pc_mode {
    /** Instruction addresses are 12-bit long. */
    DPU_PC_MODE_12 = 0,
    /** Instruction addresses are 13-bit long. */
    DPU_PC_MODE_13 = 1,
    /** Instruction addresses are 14-bit long. */
    DPU_PC_MODE_14 = 2,
    /** Instruction addresses are 15-bit long. */
    DPU_PC_MODE_15 = 3,
    /** Instruction addresses are 16-bit long. */
    DPU_PC_MODE_16 = 4,
};

/**
 * @brief Debugging information.
 */
struct dpu_context_t {
    /** Hardware characteristics of the DPU. */
    struct {
        /** Number of threads per DPU. */
        uint32_t nr_threads;
        /** Number of work registers per DPU thread. */
        uint32_t nr_registers;
        /** Number of atomic bits per DPU. */
        uint32_t nr_atomic_bits;

        /** Instruction RAM size in instructions. */
        iram_size_t iram_size;
        /** Main RAM size in bytes. */
        mram_size_t mram_size;
        /** Work RANK size in 32-bit words. */
        wram_size_t wram_size;
    } info;

    /** Instruction RAM snapshot. */
    dpuinstruction_t *iram;
    /** Main RAM snapshot. */
    uint8_t *mram;
    /** Work RAM snapshot. */
    dpuword_t *wram;

    /** Storage for the working register values of each DPU thread. */
    uint32_t *registers;
    /** Storage for the PC value of each DPU thread. */
    iram_addr_t *pcs;
    /** Storage for the atomic register value. */
    bool *atomic_register;
    /** Storage for the zero flag value of each DPU thread. */
    bool *zero_flags;
    /** Storage for the carry flag value of each DPU thread. */
    bool *carry_flags;
    /** Number of running threads in the current debugging session. */
    uint8_t nr_of_running_threads;
    /** Scheduling of the DPU threads in the current debugging session. */
    uint8_t *scheduling;
    /** Whether a BKP fault was triggered. */
    bool bkp_fault;
    /** Whether a DMA fault was triggered. */
    bool dma_fault;
    /** Whether a MEM fault was triggered. */
    bool mem_fault;
    /** The DPU thread index that triggered the BKP fault (if any). */
    dpu_thread_t bkp_fault_thread_index;
    /** The DPU thread index that triggered the DMA fault (if any). */
    dpu_thread_t dma_fault_thread_index;
    /** The DPU thread index that triggered the MEM fault (if any). */
    dpu_thread_t mem_fault_thread_index;
    /** The BKP fault ID associated to the current BKP fault (if any). */
    uint32_t bkp_fault_id;
};

#endif // DPU_TYPES_H
