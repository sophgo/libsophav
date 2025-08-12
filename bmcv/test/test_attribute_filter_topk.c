#include <math.h>
#include "stdio.h"
#include "stdlib.h"
#include <time.h>
#include "bmcv_api_ext_c.h"
#include <pthread.h>
#include <stdint.h>
#ifdef __linux__
#include <sys/time.h>
#else
#include <windows.h>
#include "time.h"
#endif
#include <assert.h>

#include <float.h>
#include <unistd.h>

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))
#define MAX_BITS 128
#define MAX_HD MAX_BITS
#define TYPICAL_HD 30

typedef struct {
    int loop_num;
    int random;
    int data_type;
    int data_count;
    int max_space_points;
    int time_range_min;
    int time_range_max;
    int attr3_flag;
    int topk;
    int8_t attr_mask;
    int64_t* space_bits;
    int32_t* time_points;
    int32_t* space_points;
    int8_t* attributes;
    float* similarity_data_fp32;
    uint32_t* similarity_data_u32;
    float threshold;
    bm_handle_t handle;
} attribute_filter_topk_thread_arg_t;

void generate_test_data(size_t data_count, int max_space_points, int64_t** space_bits, int32_t** time_points, int32_t** space_points,
                        int8_t** attributes, int active_points_num, float** similarity_data_fp32, uint32_t** similarity_data_u32) {
    const size_t bitmap_size = (max_space_points + 64 - 1) / 64;
    *space_bits = (int64_t*)calloc(bitmap_size, sizeof(int64_t));
    *time_points = (int32_t*)malloc(data_count * sizeof(int32_t));
    *space_points = (int32_t*)malloc(data_count * sizeof(int32_t));
    *attributes = (int8_t*)malloc(data_count * sizeof(int8_t));
    *similarity_data_fp32 = (float*)malloc(data_count * sizeof(float));
    *similarity_data_u32 = (uint32_t*)malloc(data_count * sizeof(uint32_t));

    //Bitmap initialization
    int32_t* valid_points = (int32_t*)malloc(active_points_num * sizeof(int32_t));
    for (size_t i = 0; i < active_points_num; ++i) {
        valid_points[i] = rand() % max_space_points;
    }

    // Fisher-Yates Shuffle
    for (int i = active_points_num-1; i > 0; --i) {
        int j = rand() % (i + 1);
        int32_t temp = valid_points[i];
        valid_points[i] = valid_points[j];
        valid_points[j] = temp;
    }

    for (size_t i = 0; i < active_points_num; ++i) {
        int32_t point = valid_points[i];
        int group = point / 64;
        int offset = point % 64;
        (*space_bits)[group] |= (1ULL << offset);
    }
    free(valid_points);
    // generate_hamming_data(*similarity_data_u32, data_count);
    //Fill in the attributes of each feature
    for (size_t i = 0; i < data_count; ++i) {
        (*time_points)[i] = rand() % 315360001;
        (*space_points)[i] = rand() % 300001;
        (*attributes)[i] = (int8_t)rand();
        (*similarity_data_fp32)[i] = (float)rand() / RAND_MAX * 3000000.0f - 1500000.0f;
        (*similarity_data_u32)[i] = (uint32_t)rand() % 1025;
    }
}

typedef struct {
    int index;
    float distance;
} Candidate_fp32;

typedef struct {
    int index;
    uint32_t distance;
} Candidate_u32;

static float* gemm_data_global;
static int compare_candidates_qsort_stable_fp32(const void *a, const void *b) {
    const Candidate_fp32 *ca = (const Candidate_fp32 *)a;
    const Candidate_fp32 *cb = (const Candidate_fp32 *)b;

    float distance_a = gemm_data_global[ca->index];
    float distance_b = gemm_data_global[cb->index];

    if (distance_a < distance_b) return 1;
    if (distance_a > distance_b) return -1;

    if (ca->index < cb->index) return -1;
    if (ca->index > cb->index) return 1;

    return 0;
}

static uint32_t* hm_distance_data_global;
static int compare_candidates_qsort_stable_u32(const void *a, const void *b) {
    const Candidate_u32 *ca = (const Candidate_u32 *)a;
    const Candidate_u32 *cb = (const Candidate_u32 *)b;

    uint32_t distance_a = hm_distance_data_global[ca->index];
    uint32_t distance_b = hm_distance_data_global[cb->index];

    if (distance_a < distance_b) return -1;
    if (distance_a > distance_b) return 1;

    if (ca->index < cb->index) return -1;
    if (ca->index > cb->index) return 1;

    return 0;
}

static void attribute_filter_topk_cpu(
        int data_type,
        int data_count,
        int max_space_points,
        int time_range_min,
        int time_range_max,
        int attr3_flag,
        int topk,
        float threshold,
        int8_t attr_mask,
        int64_t* space_bits,
        int32_t* time_points,
        int32_t* space_points,
        int8_t* attributes,
        float* similarity_data_fp32,
        uint32_t* similarity_data_u32,
        void* topk_data_cpu,
        int* topk_idx_cpu,
        int* filter_count) {
    float float_thres = 0.0f;
    uint32_t uint32_thres = 0;
    Candidate_fp32* candidates_fp32 = NULL;
    Candidate_u32* candidates_u32 = NULL;
    float* float_topk = NULL;
    uint32_t* uint32_topk = NULL;
    if (data_type == 5) {
        float_thres = threshold;
        candidates_fp32 = (Candidate_fp32*)malloc(data_count * sizeof(Candidate_fp32));
        float_topk = (float*)topk_data_cpu;
    } else if (data_type == 8) {
        uint32_thres = (uint32_t)threshold;
        candidates_u32 = (Candidate_u32*)malloc(data_count * sizeof(Candidate_u32));
        uint32_topk = (uint32_t*)topk_data_cpu;
    } else {
        printf("Unsupported data type: %d\n", data_type);
        return;
    }

    for (int i = 0; i < topk; i++) {
        topk_idx_cpu[i] = -1;
        if (data_type == 5) {
            float_topk[i] = -FLT_MAX;
        } else {
            uint32_topk[i] = UINT32_MAX;
        }
    }

    int candidate_count = 0;
    // 1. Attribute filter
    for (int i = 0; i < data_count; ++i) {
        // Space point match
        if (max_space_points != -1) {
            int32_t space_point = space_points[i];
            if (space_point < 0 || space_point >= max_space_points) {
                continue;
            }
            int group = space_point / 64;
            int remained = space_point % 64;
            if ((space_bits[group] & (1ULL << remained)) == 0) {
                continue;
            }
        }
        // Attribute match
        if (attr3_flag) {
            if ((attributes[i] & attr_mask) <= 0) {
                continue;
            }
        } else {
            if ((attributes[i] & attr_mask) != attr_mask) {
                continue;
            }
        }
        // Time range match
        int32_t time_point = time_points[i];
        if (time_point < time_range_min || time_point > time_range_max) {
            continue;
        }
        // Similarity match
        if (data_type == 5) {  //gemm_output_data
            if (similarity_data_fp32[i] <= float_thres) {
                continue;
            }
        } else {  //hm_distance_output_data
            if (similarity_data_u32[i] >= uint32_thres) {
                continue;
            }
        }
        if (data_type == 5) {
            candidates_fp32[candidate_count].index = i;
            candidates_fp32[candidate_count].distance = similarity_data_fp32[i];
        } else {
            candidates_u32[candidate_count].index = i;
            candidates_u32[candidate_count].distance = similarity_data_u32[i];
        }
        candidate_count++;
    }
    *filter_count = candidate_count;
    printf("CPU Candidate count: %d\n", *filter_count);
    // 2. Sort by distance ascending
    if (data_type == 5) {
        gemm_data_global = similarity_data_fp32;
        qsort(candidates_fp32, candidate_count, sizeof(Candidate_fp32), compare_candidates_qsort_stable_fp32);
    } else {
        hm_distance_data_global = similarity_data_u32;
        qsort(candidates_u32, candidate_count, sizeof(Candidate_u32), compare_candidates_qsort_stable_u32);
    }
    // 3. Get the topk results
    int result_count = (candidate_count < topk) ? candidate_count : topk;
    for (int i = 0; i < result_count; ++i) {
        if (data_type == 5) {
            topk_idx_cpu[i] = candidates_fp32[i].index;
            float_topk[i] = candidates_fp32[i].distance;
        } else {
            topk_idx_cpu[i] = candidates_u32[i].index;
            uint32_topk[i] = candidates_u32[i].distance;
        }
    }

    if(data_type == 5) {
        free(candidates_fp32);
    } else {
        free(candidates_u32);
    }
}

void convert_int64_to_int32(const int64_t* input, int32_t* output, size_t n) {
    for (size_t i = 0; i < n; i++) {
        uint64_t value = (uint64_t)input[i]; //Use unsigned to ensure correct shifting
        output[2 * i] = (int32_t)(value & 0xFFFFFFFF);
        output[2 * i + 1] = (int32_t)((value >> 32) & 0xFFFFFFFF);
    }
}

static int attribute_filter_topk_tpu(
        bm_handle_t handle,
        int data_type,
        int data_count,
        int max_space_points,
        int time_range_min,
        int time_range_max,
        int attr3_flag,
        int topk,
        float threshold,
        int8_t attr_mask,
        int64_t* space_bits,
        int32_t* time_points,
        int32_t* space_points,
        int8_t* attributes,
        float* similarity_data_fp32,
        uint32_t* similarity_data_u32,
        void* topk_data_tpu,
        int* topk_idx_tpu) {
    bm_status_t bm_ret;
    bm_device_mem_t space_bits_dev_mem;
    bm_device_mem_t space_points_dev_mem;
    bm_device_mem_t time_points_dev_mem;
    bm_device_mem_t attributes_dev_mem;
    bm_device_mem_t similarity_data_dev_mem;
    bm_device_mem_t filtered_idx_dev_mem;
    bm_device_mem_t filtered_similarity_data_dev_mem;
    bm_device_mem_t topk_data_dev_mem;
    bm_device_mem_t topk_idx_dev_mem;

    size_t type_bytes = (data_type == 5 ? sizeof(float) : sizeof(uint32_t));
    const size_t bitmap_size = (max_space_points + 64 - 1) / 64;
    struct timeval t1, t2;
    int per_row_num = 1024;
    int per_row_num_bits_map = per_row_num / 32;
    int space_bits_num = ((bitmap_size * 2 + per_row_num_bits_map - 1) / per_row_num_bits_map) * per_row_num_bits_map;
    int32_t* space_bits_int32 = (int32_t*)calloc(space_bits_num, sizeof(int32_t));
    convert_int64_to_int32(space_bits, space_bits_int32, bitmap_size);

    bm_malloc_device_byte(handle, &space_bits_dev_mem, space_bits_num * sizeof(int32_t));
    bm_malloc_device_byte(handle, &space_points_dev_mem, data_count * sizeof(int32_t));
    bm_malloc_device_byte(handle, &time_points_dev_mem, data_count * sizeof(int32_t));
    bm_malloc_device_byte(handle, &attributes_dev_mem, data_count * sizeof(int8_t));
    bm_malloc_device_byte(handle, &similarity_data_dev_mem, data_count * type_bytes);
    bm_malloc_device_byte(handle, &filtered_similarity_data_dev_mem, data_count * type_bytes);
    bm_malloc_device_byte(handle, &filtered_idx_dev_mem, data_count * sizeof(int));
    bm_malloc_device_byte(handle, &topk_idx_dev_mem, topk * sizeof(unsigned int));
    bm_malloc_device_byte(handle, &topk_data_dev_mem, topk * type_bytes);
    bm_ret = bm_memcpy_s2d(handle, space_bits_dev_mem, space_bits_int32);
    if(bm_ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d space_bits_dev_mem failed\n");
    }
    bm_ret = bm_memcpy_s2d(handle, space_points_dev_mem, space_points);
    if(bm_ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d space_points_dev_mem failed\n");
    }
    bm_ret = bm_memcpy_s2d(handle, time_points_dev_mem, time_points);
    if(bm_ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d time_points_dev_mem failed\n");
    }
    bm_ret = bm_memcpy_s2d(handle, attributes_dev_mem, attributes);
    if(bm_ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d attributes_dev_mem failed\n");
    }
    if (data_type == 5) {
        bm_ret = bm_memcpy_s2d(handle, similarity_data_dev_mem, similarity_data_fp32);
    } else {
        bm_ret = bm_memcpy_s2d(handle, similarity_data_dev_mem, similarity_data_u32);
    }
    if(bm_ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d similarity_data_dev_mem failed\n");
    }
    gettimeofday(&t1, NULL);
    bm_ret = bmcv_attribute_filter_topk(handle, data_type, data_count, max_space_points, time_range_min,
                                        time_range_max, attr3_flag, topk, threshold, attr_mask, space_bits_dev_mem,
                                        space_points_dev_mem, time_points_dev_mem, attributes_dev_mem,
                                        similarity_data_dev_mem, filtered_idx_dev_mem, filtered_similarity_data_dev_mem,
                                        topk_idx_dev_mem, topk_data_dev_mem);
    if(bm_ret != BM_SUCCESS) {
        printf("bmcv_attribute_filter_topk failed\n");
    }
    gettimeofday(&t2, NULL);
    printf("attribute_filter_topk TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    bm_memcpy_d2s(handle, topk_data_tpu, topk_data_dev_mem);
    bm_memcpy_d2s(handle, topk_idx_tpu, topk_idx_dev_mem);
    bm_free_device(handle, space_bits_dev_mem);
    bm_free_device(handle, space_points_dev_mem);
    bm_free_device(handle, time_points_dev_mem);
    bm_free_device(handle, attributes_dev_mem);
    bm_free_device(handle, similarity_data_dev_mem);
    bm_free_device(handle, topk_data_dev_mem);
    bm_free_device(handle, topk_idx_dev_mem);
    bm_free_device(handle, filtered_idx_dev_mem);
    bm_free_device(handle, filtered_similarity_data_dev_mem);
    free(space_bits_int32);
    return (int)bm_ret;
}

static int cmp_topk_fp32(const float* topk_data_cpu, const float* topk_data_tpu, int* topk_idx_cpu, int* topk_idx_tpu, int size) {
    const float EPS = 1e-6f;
    for (int i = 0; i < size; i++) {
        if ((topk_idx_cpu[i] == -1) && (topk_idx_tpu[i] == -1)) {
            continue;
        }
        if (fabsf(topk_data_cpu[i] - topk_data_tpu[i]) > EPS) {
            printf("cmp error[%d]: idx_cpu:%d, similarity_cpu:%f, idx_tpu:%d, similarity_tpu:%f\n",
                    i, topk_idx_cpu[i], topk_data_cpu[i], topk_idx_tpu[i], topk_data_tpu[i]);
            for (int d = i+1; d < i+5; d++) {
                printf("idx:[%d], idx_cpu:%d, similarity_cpu:%f, idx_tpu:%d, similarity_tpu:%f\n",
                       d, topk_idx_cpu[d], topk_data_cpu[d], topk_idx_tpu[d], topk_data_tpu[d]);
            }
            return -1;
        }
    }
    return 0;
}

static int cmp_topk_u32(const uint32_t* topk_data_cpu, const uint32_t* topk_data_tpu, int* topk_idx_cpu, int* topk_idx_tpu, int size) {
    for (int i = 0; i < size; i++) {
        if ((topk_idx_cpu[i] == -1) && (topk_idx_tpu[i] == -1)) {
            continue;
        }
        if (topk_data_cpu[i] != topk_data_tpu[i]) {
            printf("cmp error[%d]: idx_cpu:%d, similarity_cpu:%u, idx_tpu:%d, similarity_tpu:%u\n",
                i, topk_idx_cpu[i], topk_data_cpu[i], topk_idx_tpu[i], topk_data_tpu[i]);
            for (int d = i+1; d < i+5; d++) {
                printf("idx:[%d], idx_cpu:%d, similarity_cpu:%u, idx_tpu:%d, similarity_tpu:%u\n",
                    d, topk_idx_cpu[d], topk_data_cpu[d], topk_idx_tpu[d], topk_data_tpu[d]);
            }
            return -1;
        }
    }
    return 0;
}

static int test_attribute_filter_topk_single_test(
        bm_handle_t handle,
        int random,
        int data_type,
        int data_count,
        int max_space_points,
        int time_range_min,
        int time_range_max,
        int attr3_flag,
        int topk,
        float threshold,
        int8_t attr_mask,
        int64_t* space_bits,
        int32_t* time_points,
        int32_t* space_points,
        int8_t* attributes,
        float* similarity_data_fp32,
        uint32_t* similarity_data_u32) {
    printf("data_type = %d, data_count = %d, max_space_points = %d, time_range_min = %d, time_range_max = %d, attr3_flag = %d, topk = %d, attr_mask = %d, threshold = %f\n",
            data_type, data_count, max_space_points, time_range_min, time_range_max, attr3_flag, topk, attr_mask, threshold);
    int ret = 0;
    struct timeval t1, t2;
    void* topk_data_cpu = (void*)malloc(topk * 4);
    void* topk_data_tpu = (void*)malloc(topk * 4);
    int* topk_idx_cpu = (void*)malloc(topk * sizeof(int));
    int* topk_idx_tpu = (void*)malloc(topk * sizeof(int));
    // cpu ref
    int filter_count = 0;
    gettimeofday(&t1, NULL);
    attribute_filter_topk_cpu(data_type, data_count, max_space_points, time_range_min, time_range_max, attr3_flag,
                              topk, threshold, attr_mask, space_bits, time_points, space_points,
                              attributes, similarity_data_fp32, similarity_data_u32, topk_data_cpu, topk_idx_cpu, &filter_count);
    gettimeofday(&t2, NULL);
    printf("attribute_filter_topk CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    // tpu cal
    ret = attribute_filter_topk_tpu(handle, data_type, data_count, max_space_points, time_range_min, time_range_max, attr3_flag, topk,
                               threshold, attr_mask, space_bits, time_points, space_points,
                               attributes, similarity_data_fp32, similarity_data_u32, topk_data_tpu, topk_idx_tpu);
    if (ret != 0) {
        printf("attribute_filter_topk_tpu failed!\n");
        free(topk_data_cpu);
        free(topk_data_tpu);
        free(topk_idx_cpu);
        free(topk_idx_tpu);
        return ret;
    }
    // cmp
    if (data_type == 5) {
        ret = cmp_topk_fp32((float*)topk_data_cpu, (float*)topk_data_tpu, topk_idx_cpu, topk_idx_tpu, topk);
    } else {
        ret = cmp_topk_u32((uint32_t*)topk_data_cpu, (uint32_t*)topk_data_tpu, topk_idx_cpu, topk_idx_tpu, topk);
    }

    if(ret != 0) {
        printf("attribute_filter_topk cmp failed!\n");
    }
    free(topk_data_cpu);
    free(topk_data_tpu);
    free(topk_idx_cpu);
    free(topk_idx_tpu);
    return ret;
}

void* test_attribute_filter_topk(void* args) {
    attribute_filter_topk_thread_arg_t* attribute_filter_topk_thread_arg = (attribute_filter_topk_thread_arg_t*)args;
    int random = attribute_filter_topk_thread_arg->random;
    int loop_num = attribute_filter_topk_thread_arg->loop_num;
    int data_type = attribute_filter_topk_thread_arg->data_type;
    int data_count = attribute_filter_topk_thread_arg->data_count;
    int max_space_points = attribute_filter_topk_thread_arg->max_space_points;
    int time_range_min = attribute_filter_topk_thread_arg->time_range_min;
    int time_range_max = attribute_filter_topk_thread_arg->time_range_max;
    int topk = attribute_filter_topk_thread_arg->topk;
    int attr3_flag = attribute_filter_topk_thread_arg->attr3_flag;
    float threshold = attribute_filter_topk_thread_arg->threshold;
    int8_t attr_mask = attribute_filter_topk_thread_arg->attr_mask;
    int64_t* space_bits = attribute_filter_topk_thread_arg->space_bits;
    int32_t* time_points = attribute_filter_topk_thread_arg->time_points;
    int32_t* space_points = attribute_filter_topk_thread_arg->space_points;
    int8_t* attributes = attribute_filter_topk_thread_arg->attributes;
    float* similarity_data_fp32 = attribute_filter_topk_thread_arg->similarity_data_fp32;
    uint32_t* similarity_data_u32 = attribute_filter_topk_thread_arg->similarity_data_u32;
    bm_handle_t handle = attribute_filter_topk_thread_arg->handle;

    int64_t* local_space_bits = NULL;
    int32_t* local_time_points = NULL;
    int32_t* local_space_points = NULL;
    int8_t* local_attributes = NULL;
    float* local_similarity_data_fp32 = NULL;
    uint32_t* local_similarity_data_u32 = NULL;

    for (int i = 0; i < loop_num; i++) {
        if (random) {
            struct timespec tp;
            clock_gettime(CLOCK_MONOTONIC, &tp);
            unsigned int seed = (unsigned int)(tp.tv_nsec ^ tp.tv_sec ^ i ^ getpid());
            srand(seed);
            printf("loop seed = %u\n", seed);
            // data_type = 5 + 3 * (rand() % 2);
            // data_count = 1024 * (1 + rand() % 4000);
            // int topk_num[] = {5, 10, 100, 1000, 10000, 50000};
            // int size = sizeof(topk_num) / sizeof(topk_num[0]);
            // int rand_num = rand() % size;
            // topk = topk_num[rand_num];
            // // topk = 1 + rand() % 50000;
            // time_range_max = rand() % 315360000;
            // attr_mask = (int8_t)rand();
            // attr3_flag = (int)rand() % 2;
            int active_points_num = 5000;
            generate_test_data(data_count, max_space_points, &local_space_bits, &local_time_points,
                               &local_space_points, &local_attributes, active_points_num,
                               &local_similarity_data_fp32, &local_similarity_data_u32);
            space_bits = local_space_bits;
            time_points = local_time_points;
            space_points = local_space_points;
            attributes = local_attributes;
            similarity_data_fp32 = local_similarity_data_fp32;
            similarity_data_u32 = local_similarity_data_u32;
        }
        if (0 != test_attribute_filter_topk_single_test(handle, random, data_type, data_count, max_space_points, time_range_min, time_range_max, attr3_flag,
            topk, threshold, attr_mask, space_bits, time_points, space_points, attributes, similarity_data_fp32, similarity_data_u32)) {
            printf("------TEST ATTRIBUTE_FILTER_TOPK FAILED------\n");
            exit(-1);
        }
        printf("------TEST ATTRIBUTE_FILTER_TOPK PASSED!------\n");
        if(random) {
            free(local_space_bits);
            free(local_time_points);
            free(local_space_points);
            free(local_attributes);
            free(local_similarity_data_fp32);
            free(local_similarity_data_u32);
        }
    }
    return (void*)0;
}

int main(int argc, char *argv[]) {
    struct timespec tp;
    clock_gettime(0, &tp);
    unsigned int seed = tp.tv_nsec;
    srand(seed);
    int thread_num = 1;
    int loop = 1;
    int random = 1;
    int data_type = 5 + 3 * (rand() % 2);
    int topk = 10;
    int data_count = 1024 * (1 + rand() % 2000);
    int max_space_points = 300000;
    float threshold = 500.0;
    int time_range_min = 0;
    int time_range_max = rand() % 315360000;
    int attr3_flag = (int)rand() % 2;
    int8_t attr_mask = (int8_t)rand();
    int64_t* space_bits = NULL;
    int active_points_num = 5000;
    int32_t* time_points = NULL;
    int32_t* space_points = NULL;
    int8_t* attributes = NULL;
    float* similarity_data_fp32 = NULL;
    uint32_t* similarity_data_u32 = NULL;
    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }

    if (argc == 2 && atoi(argv[1]) == -1) {
        printf("usage: \n");
        printf("%s thread_num loop random data_type topk data_count seed time_range_max attr_mask attr3_flag\n", argv[0]);
        printf("example:\n");
        printf("%s \n", argv[0]);
        printf("%s 2\n", argv[0]);
        printf("%s 2 1\n", argv[0]);
        return 0;
    }

    if(argc > 1) thread_num = atoi(argv[1]);
    if(argc > 2) loop = atoi(argv[2]);
    if(argc > 3) random = atoi(argv[3]);
    if(argc > 4) data_type = atoi(argv[4]);
    if(argc > 5) topk = atoi(argv[5]);
    if(argc > 6) data_count = atoi(argv[6]);
    if(argc > 7) seed = atoi(argv[7]);
    if(argc > 8) time_range_max = atoi(argv[8]);
    if(argc > 9) attr_mask = atoi(argv[9]);
    if(argc > 10) attr3_flag = atoi(argv[10]);
    printf("seed = %d\n", seed);
    srand(seed);
    if (!random) {
        generate_test_data(data_count, max_space_points, &space_bits, &time_points, &space_points,
                           &attributes, active_points_num, &similarity_data_fp32, &similarity_data_u32);
    }
    // test for multi-thread
    pthread_t pid[thread_num];
    attribute_filter_topk_thread_arg_t attribute_filter_topk_thread_arg[thread_num];
    for (int i = 0; i < thread_num; i++) {
        attribute_filter_topk_thread_arg[i].loop_num = loop;
        attribute_filter_topk_thread_arg[i].data_type = data_type;
        attribute_filter_topk_thread_arg[i].data_count = data_count;
        attribute_filter_topk_thread_arg[i].max_space_points = max_space_points;
        attribute_filter_topk_thread_arg[i].threshold = threshold;
        attribute_filter_topk_thread_arg[i].time_range_min = time_range_min;
        attribute_filter_topk_thread_arg[i].time_range_max = time_range_max;
        attribute_filter_topk_thread_arg[i].attr3_flag = attr3_flag;
        attribute_filter_topk_thread_arg[i].attr_mask = attr_mask;
        attribute_filter_topk_thread_arg[i].time_points = time_points;
        attribute_filter_topk_thread_arg[i].space_points = space_points;
        attribute_filter_topk_thread_arg[i].space_bits = space_bits;
        attribute_filter_topk_thread_arg[i].attributes = attributes;
        attribute_filter_topk_thread_arg[i].similarity_data_fp32 = similarity_data_fp32;
        attribute_filter_topk_thread_arg[i].similarity_data_u32 = similarity_data_u32;
        attribute_filter_topk_thread_arg[i].topk = topk;
        attribute_filter_topk_thread_arg[i].random = random;
        attribute_filter_topk_thread_arg[i].handle = handle;
        if (pthread_create(pid + i, NULL, test_attribute_filter_topk, attribute_filter_topk_thread_arg + i) != 0) {
            printf("create thread failed\n");
            return -1;
        }
    }
    for (int i = 0; i < thread_num; i++) {
        int ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            printf("Thread join failed\n");
            exit(-1);
        }
    }
    if (!random) {
        free(space_bits);
        free(time_points);
        free(space_points);
        free(attributes);
        free(similarity_data_fp32);
        free(similarity_data_u32);
    }
    bm_dev_free(handle);
    return ret;
}