#include "bmcv_api_ext_c.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#define ERR_THRESHOLD 1e-2
#define CMP_ERR_THRESHOLD 1e-6
#define ALIGN(x, align) (((x) + (align - 1)) & ~(align - 1))
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

struct cv_fm_thread_arg_t {
    int loop_num;
    int batch_size;
    int feature_size;
    int db_size;
    float sort_cnt;
    float rshiftbits;
    bm_handle_t handle;
};

static int compare_short(const void *a, const void *b) {
    return (*(short*)b - *(short*)a);
}

static void sort_cpu_fix8b(short** match_res, int batch_size, int db_size) {
    int i;

    for (i = 0; i < batch_size; ++i) {
        qsort(match_res[i], db_size, sizeof(short), compare_short);
    }
}

static int cpu_feature_match_fix8b(int8_t** input_data, int batch_size, int feature_size,
                                    int8_t** db_data, int db_size, int rshiftbits,
                                    short** match_res)
{
    int tmp = 0;
    int i, j, k;

    for (i = 0; i < batch_size; ++i) {
        for (j = 0; j < db_size; ++j) {
            tmp = 0;
            for (k = 0; k < feature_size; ++k) {
                tmp += db_data[k][j] * input_data[i][k];
            }
            tmp = tmp > 0x7fff ? 0x7fff : tmp;
            tmp = tmp < -0x8000 ? -0x8000 : tmp;
            if (rshiftbits != 0) {
                tmp = ((tmp >> (rshiftbits - 1)) + 1) >> 1;
            }
            match_res[i][j] = (short)tmp;
        }
    }
    sort_cpu_fix8b(match_res, batch_size, db_size);

    return 0;
}

static int res_compare_short(short* tpu_res_similarity, int* tpu_res_index, short** ref_res,
                            int batch_size, int db_size, int sort_cnt)
{
    int i, j;
    float tpu_res, cpu_res;

    for (i = 0; i < batch_size; i++) {
        for (j = 0; j < sort_cnt; j++) {
            tpu_res = (float)tpu_res_similarity[i * sort_cnt + j];
            cpu_res = (float)ref_res[i][j];
            if (tpu_res - cpu_res > ERR_THRESHOLD) {
                printf("the batch = %d, the sort = %d, the tpu_res = %hd, the cpu_res = %hd\n",
                        i, j, tpu_res_similarity[i * sort_cnt + j], ref_res[i][j]);
                return -1;
            }
        }
    }

    return 0;
}

static int tpu_feature_match_fix8b(bm_handle_t handle, int8_t* input_data, int8_t* db_data,
                                    short* output_similarity, int* output_index, int batch_size,
                                    int feature_size,int db_size,int sort_cnt, int rshiftbits)
{
    struct timeval t1, t2;
    bm_status_t ret = BM_SUCCESS;

    gettimeofday(&t1, NULL);
    ret = bmcv_feature_match(handle, bm_mem_from_system(input_data), bm_mem_from_system(db_data),
                            bm_mem_from_system(output_similarity), bm_mem_from_system(output_index),
                            batch_size, feature_size, db_size, sort_cnt, rshiftbits);
    if (ret != BM_SUCCESS) {
        printf("bmcv_feature_match_fix8_bm1684X failed!\n");
        return -1;
    }
    gettimeofday(&t2, NULL);;
    printf("Feature Match Fix8b TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    return 0;
}

static int cv_fm_fix8b_test_rand(int batch_size, int feature_size, int rshiftbits, int sort_cnt,
                                int db_size, bm_handle_t handle)
{
    int8_t* input_data = (int8_t*)malloc(sizeof(int8_t) * batch_size * feature_size);
    int8_t* db_data = (int8_t*)malloc(sizeof(int8_t) * db_size * feature_size);
    short* output_similarity = (short*)malloc(sizeof(short) * batch_size * db_size);
    int* output_index = (int*)malloc(sizeof(int) * batch_size * db_size);
    int i, j;
    int8_t temp_val;
    int ret = 0;
    struct timeval t1, t2;

    printf("db_size:%d, batch_size:%d, feature_size:%d, sort_cnt:%d, rshiftbits:%d\n",
            db_size, batch_size, feature_size, sort_cnt, rshiftbits);

    int8_t** db_content_vec = (int8_t**)malloc(sizeof(int8_t*) * feature_size);
    for (i = 0; i < feature_size; ++i) {
        db_content_vec[i] = (int8_t*)malloc(sizeof(int8_t) * db_size);
    }
    int8_t** input_content_vec = (int8_t**)malloc(sizeof(int8_t*) * batch_size);
    for (i = 0; i < batch_size; ++i) {
        input_content_vec[i] = (int8_t*)malloc(sizeof(int8_t) * feature_size);
    }

    short** ref_res = (short**)malloc(sizeof(short*) * batch_size);
    for (i = 0; i < batch_size; ++i) {
        ref_res[i] = (short*)malloc(sizeof(short) * db_size);
    }

    for (i = 0; i < feature_size; ++i) {
        for (j = 0; j < db_size; ++j) {
            temp_val = rand() % 20 - 10;
            db_content_vec[i][j] = temp_val;
        }
    }

    for (i = 0; i < batch_size; ++i) {
        for (j = 0; j < feature_size; ++j) {
            temp_val = rand() % 20 - 10;
            input_content_vec[i][j] = temp_val;
        }
    }

    for (i = 0; i < feature_size; ++i) {
        for (j = 0; j < db_size; ++j) {
            db_data[i * db_size + j] = db_content_vec[i][j];
        }
    }

    for (i = 0; i < batch_size; ++i) {
        for (j = 0; j < feature_size; ++j) {
            input_data[i * feature_size + j] = input_content_vec[i][j];
        }
    }

    ret = tpu_feature_match_fix8b(handle, input_data, db_data, output_similarity, output_index, batch_size,
                                feature_size, db_size, sort_cnt, rshiftbits);
    if (ret) {
        printf("tpu_feature_match_fix8b failed!\n");
        goto exit;
    }

    gettimeofday(&t1, NULL);
    ret = cpu_feature_match_fix8b(input_content_vec, batch_size, feature_size, db_content_vec, db_size,
                                 rshiftbits, ref_res);
    if (ret) {
        printf("cpu_feature_match_fix8b failed!\n");
        goto exit;
    }
    gettimeofday(&t2, NULL);;
    printf("Feature Match Fix8b CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    ret = res_compare_short(output_similarity, output_index, ref_res, batch_size, db_size, sort_cnt);
    if (ret) {
        printf("FEATURE MATCHING FIX8B COMPARE ERROR\n");
        goto exit;
    }

exit:
    free(input_data);
    free(db_data);
    free(output_similarity);
    free(output_index);
    for(i = 0; i < batch_size; ++i) {
        free(input_content_vec[i]);
        free(ref_res[i]);
    }
    for(i = 0; i < feature_size; ++i) {
        free(db_content_vec[i]);
    }
    free(input_content_vec);
    free(db_content_vec);
    free(ref_res);
    return ret;
}

void* test_fm_all(void* args)
{
    struct cv_fm_thread_arg_t* cv_fm_thread_arg = (struct cv_fm_thread_arg_t*)args;

    int loop = cv_fm_thread_arg->loop_num;
    int batch_size = cv_fm_thread_arg->batch_size;
    int feature_size = cv_fm_thread_arg->feature_size;
    int rshiftbits = cv_fm_thread_arg->rshiftbits;
    int sort_cnt = cv_fm_thread_arg->sort_cnt;
    int db_size = cv_fm_thread_arg->db_size;
    bm_handle_t handle = cv_fm_thread_arg->handle;
    int i;
    int ret = 0;

    for (i = 0; i < loop; ++i) {
        ret = cv_fm_fix8b_test_rand(batch_size, feature_size, rshiftbits, sort_cnt, db_size, handle);
        if (ret) {
            printf("------Test Feature Match Fix8b Failed!------\n");
            exit(-1);
        }
        printf("------Test Feature Match Fix8b Done!------\n");
    }
    printf("------Test Feature Match All Done!------\n");
    return (void*)0;
}

int main(int argc, char* args[])
{
    struct timespec tp;
    clock_gettime(0, &tp);
    srand(tp.tv_nsec);

    int thread_num = 1;
    int loop = 1;
    int batch_size = rand() % 8 + 1;
    int feature_size = rand() % 1000 + 1;
    int rshiftbits = rand() % 3;
    int sort_cnt = rand() % 30 + 1;
    int db_size = (rand() % 90 + 1) * 1000;
    bm_handle_t handle;
    int ret = 0;
    int i;

    ret = (int)bm_dev_request(&handle, 0);
    if (ret) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return ret;
    }

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("%s thread_num loop batch_size feature_size db_size sort_cnt rshiftbits\n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 1 1 2 100 1000 5 0\n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) batch_size = atoi(args[3]);
    if (argc > 4) feature_size = atoi(args[4]);
    if (argc > 5) db_size  = atoi(args[5]);
    if (argc > 6) sort_cnt  = atoi(args[6]);
    if (argc > 7) rshiftbits = atoi(args[7]);

    pthread_t pid[thread_num];
    struct cv_fm_thread_arg_t fm_thread_arg[thread_num];
    for (i = 0; i < thread_num; i++) {
        fm_thread_arg[i].loop_num = loop;
        fm_thread_arg[i].batch_size = batch_size;
        fm_thread_arg[i].feature_size = feature_size;
        fm_thread_arg[i].rshiftbits = rshiftbits;
        fm_thread_arg[i].sort_cnt = sort_cnt;
        fm_thread_arg[i].db_size = db_size;
        fm_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_fm_all, &fm_thread_arg[i]) != 0) {
            printf("create thread failed\n");
            return -1;
        }
    }
    for (i = 0; i < thread_num; i++) {
        ret = pthread_join(pid[i], NULL);
        if (ret) {
            printf("Thread join failed\n");
            exit(-1);
        }
    }

    bm_dev_free(handle);
    return ret;
}