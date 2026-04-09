#pragma once

#define TEST_DPU

// Full PQ code width. For MSMARCO10M 4096/128/8, keep this at 128.
#define MY_PQ_M 128
// LUT staging width per transfer / per DPU tile accumulation step.
#define LUT_TILE_M 32

#define DIM 1024

// u8bin uint8 query: 0
// signed int8 query: 1
// generic fvecs query: 2
#define QUERY_TYPE 2

#define SLOT_L 100000

#define COPY_RATE 10

#define MAX_COROUTINE 4

// set to 1 to enable replica
#define ENABLE_REPLICA 1

#define CHANGE_MAX_COROUTINE 0
#define CHANGE_COPY_RATE 0
#define CHANGE_ENABLE_REPLICA 0

#define ENABLE_DPU_LOAD 0



// below is not necessary to change

#define NR_DPU 2558

#define FIG_BREAKDOWN


// SPACE 1B20M4096 MAX_SIZE = 22923525
// MAX_SHARD = MAX_CLUSTER_LEN / SLOT_L = 22923525 / 100000 = 229
#define MAX_SHARD 300

#define NO_QUERY -1
#define MAX_K 10


#define BACK_THREAD 1
#define DynamicBalance_THREAD 1
#define FRONT_THREAD 40

//set to 1 to enable detect dynamic balance
#define DYNAMIC_BALANCE 0


#define MAX_DPU 2560
#define MAX_PAIR 1280
#define MAX_LINE 320
#define MAX_PAIR_LINE 160
#define MAX_RANK 40

#define PERDPU_LOG_SIZE 128
#define MAX_NPROBE 1024


// when DIST_TYPE change, MAX_VALUE should be changed accordingly
#define DIST_TYPE int32_t
// #define DIST_TYPE float # 개느림
#define ID_TYPE int64_t


//MY_PQ_CLUSTER need to be changed according to DATA_TYPE
#define MY_PQ_CLUSTER 256
#define DATA_TYPE uint8_t

#define SAMPLE_INTERVAL_MS 10


#define LUT_SIZE (MY_PQ_M * MY_PQ_CLUSTER)
#define TILE_LUT_SIZE (LUT_TILE_M * MY_PQ_CLUSTER)
#define LUT_TILE_NUM (((MY_PQ_M) + (LUT_TILE_M) - 1) / (LUT_TILE_M))

#define SLOT_DATA_SIZE (SLOT_L * MY_PQ_M * sizeof(DATA_TYPE))
#define SLOT_ID_SIZE (SLOT_L * sizeof(ID_TYPE))
#define SLOT_NUM ((MRAM_SIZE) / (SLOT_DATA_SIZE + SLOT_ID_SIZE))

#if LUT_TILE_M <= 0
#error "LUT_TILE_M must be positive"
#endif

#if MY_PQ_M % LUT_TILE_M != 0
#error "MY_PQ_M must be divisible by LUT_TILE_M for LUT tiling"
#endif


// because batch dpu need more mram to store lut, so there is less mram for load balance
#if defined(TEST_BATCH_DPU)
#define MRAM_SIZE (40 * 1024 * 1024)
#define MAX_DPUBATCH 300
#else
#define MRAM_SIZE (55 * 1024 * 1024)
#define MAX_DPUBATCH 100
#endif
