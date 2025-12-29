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
                                    int feature_size,int db_size,int sort_cnt, int rshiftbits, long int *int_tpu_time)
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
    gettimeofday(&t2, NULL);
    printf("Feature Match Fix8b TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *int_tpu_time += TIME_COST_US(t1, t2);
    return 0;
}

static int cv_fm_fix8b_test_rand(int batch_size, int feature_size, int rshiftbits, int sort_cnt,
                                int db_size, bm_handle_t handle, long int *int_cpu_time, long int *int_tpu_time)
{
    int8_t* input_data = (int8_t*)malloc(sizeof(int8_t) * batch_size * feature_size);
    int8_t* db_data = (int8_t*)malloc(sizeof(int8_t) * db_size * feature_size);
    short* output_similarity = (short*)malloc(sizeof(short) * batch_size * db_size);
    int* output_index = (int*)malloc(sizeof(int) * batch_size * db_size);
    int i, j;
    int8_t temp_val;
    int ret = 0;
    struct timeval t1, t2;

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
                                feature_size, db_size, sort_cnt, rshiftbits, int_tpu_time);
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
    gettimeofday(&t2, NULL);
    printf("Feature Match Fix8b CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *int_cpu_time += TIME_COST_US(t1, t2);
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

int main(int argc, char* args[])
{
    printf("log will be saved in special_size_test_cv_feature_match_int8.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_feature_match_int8.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return 1;
    }
    struct timespec tp;
    clock_gettime(0, &tp);
    srand(tp.tv_nsec);


    int rshiftbits = 0;
    int sort_cnt = 10;
    int batch_size_num[] = {1, 5, 10};
    int feature_size_num[] = {1, 10, 100, 1000, 2000, 3000};
    int db_size_num[] = {1000, 10000, 100000};
    bm_handle_t handle;
    int ret = 0;
    int i, j, n;

    ret = (int)bm_dev_request(&handle, 0);
    if (ret) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return ret;
    }
    long int first_cpu = 0;
    long int first_tpu = 0;
    long int *test_cputime = &first_cpu;
    long int *test_tputime = &first_tpu;
    cv_fm_fix8b_test_rand(1, 1, 0, 10, 1000, handle, test_cputime, test_tputime);
    for(i = 0; i < 3; i++) {
        int batch_size = batch_size_num[i];
        for(j = 0;j < 6; j++) {
            int feature_size = feature_size_num[j];
            for(n = 0; n < 3; n++) {
                int db_size = db_size_num[n];
                long int int_cputime = 0;
                long int int_tputime = 0;
                long int *int_cpu_time = &int_cputime;
                long int *int_tpu_time = &int_tputime;
                printf("batch_size = %d, feature_size = %d, db_size = %d, sort_cnt = %d,\n",
                        batch_size, feature_size, db_size, sort_cnt);
                for(int loop = 0; loop < 3; loop++) {
                    ret = cv_fm_fix8b_test_rand(batch_size, feature_size, rshiftbits, sort_cnt, db_size, handle, int_cpu_time, int_tpu_time);
                    if (ret) {
                        printf("------Test Feature Match Fix8b Failed!------\n");
                        return -1;
                    }
                }
                printf("------int average CPU time = %ldus------\n", int_cputime / 3);
                printf("------int average TPU time = %ldus------\n", int_tputime / 3);
            }

        }
    }

    bm_dev_free(handle);
    fclose(stdout);
    stdout = original_stdout;
    return ret;
}