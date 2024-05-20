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

static float calc_sqrt(const float* feature, int feature_size)
{
    float a = 0.f;
    int i;

    for (i = 0; i < feature_size; ++i) {
        a += feature[i] * feature[i];
    }

    return 1.f / sqrt(a);
}

static int calc_sqrt_transposed(float** feature, int rows, int cols, float* db_feature)
{
    int i, j;
    float tmp;
    float result;

    for (i = 0; i < cols; ++i) {
        tmp = 0.f;
        for (j = 0; j < rows; ++j) {
            tmp += feature[j][i] * feature[j][i];
        }
        result = 1.f / sqrt(tmp);
        db_feature[i] = result;
    }

    return 0;
}

static int transpose(float** origin, int rows, int cols, float** trans)
{
    if (rows == 0 || cols == 0) {
        return BM_ERR_FAILURE;
    }

    int i, j;

    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            trans[j][i] = origin[i][j];
        }
    }

    return 0;
}

static int compare_float(const void *a, const void *b)
{
    const float epsilon = CMP_ERR_THRESHOLD;
    float diff = *(float *)b - *(float *)a;
    return (diff > epsilon) ? 1 : ((diff < -epsilon) ? -1 : 0);
}

static void sort_cpu_float(float** match_res, int batch_size, int db_size)
{
    int i;

    for (i = 0; i < batch_size; ++i) {
        qsort(match_res[i], db_size, sizeof(float), compare_float);
    }
}

static int cpu_feature_match(float** input_data, int batch_size, int feature_size, float** db_data,
                            int db_size, float** match_res)
{
    int i, j, k;
    float a, b;
    int ret = 0;
    float** trans_vec = (float**)malloc(db_size * sizeof(float*));
    for (i = 0; i < db_size; i++) {
        trans_vec[i] = (float*)malloc(feature_size * sizeof(float));
    }

    ret = transpose(db_data, feature_size, db_size, trans_vec); /*trans_vec row = db_size col = feature_size*/

    for (i = 0; i < batch_size; ++i) {
        for (j = 0; j < db_size; ++j) {
            a = 0.f;
            for (k = 0; k < feature_size; ++k) {
                a += input_data[i][k] * trans_vec[j][k];
            }
            b = calc_sqrt(input_data[i], feature_size) * calc_sqrt(trans_vec[j], feature_size);
            match_res[i][j] = a * b;
        }
    }

    sort_cpu_float(match_res, batch_size, db_size);

    for (i = 0; i < db_size; ++i) {
        free(trans_vec[i]);
    }
    free(trans_vec);

    return ret;
}

static int res_compare_float(float* tpu_res_similarity, int* tpu_res_index,
                            float** ref_res, int batch_size,int db_size)
{
    int i;
    float tpu_res, cpu_res;

    for (i = 0; i < batch_size; i++) {
            tpu_res = tpu_res_similarity[i];
            cpu_res = ref_res[i][0];
            if (tpu_res - cpu_res > ERR_THRESHOLD) {
                printf("the batch = %d, the tpu_res = %f, the cpu_res = %f\n",
                        i, tpu_res_similarity[i], ref_res[i][0]);
                return -1;
            }
        }

    return 0;
}

static int tpu_feature_match(bm_handle_t handle, float* input_data, float* db_data, float* db_feature,
                            float* output_similarity, int* output_index, int batch_size,int feature_size,
                            int db_size, long int *tpu_time)
{
    struct timeval t1, t2;
    bm_status_t ret = BM_SUCCESS;

    gettimeofday(&t1, NULL);
    ret = bmcv_feature_match_normalized(handle, bm_mem_from_system(input_data), bm_mem_from_system(db_data),
                                bm_mem_from_system(db_feature), bm_mem_from_system(output_similarity),
                                bm_mem_from_system(output_index), batch_size, feature_size, db_size);

    if (ret != BM_SUCCESS) {
        printf("bmcv_feature_match_normalized failed!\n");
        return -1;
    }
    gettimeofday(&t2, NULL);

    printf("Feature Match TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *tpu_time += TIME_COST_US(t1, t2);
    return 0;
}


static int cv_fm_test_rand(int batch_size, int feature_size, int db_size, bm_handle_t handle,
                            long int *cpu_time, long int *tpu_time)
{
    float* input_data = (float*)malloc(sizeof(float) * batch_size * feature_size);
    float* db_data = (float*)malloc(sizeof(float) * db_size * feature_size);
    float* db_feature = (float*)malloc(sizeof(float) * db_size);
    float* output_similarity = (float*)malloc(sizeof(float) * batch_size); /*float*/
    int* output_index = (int*)malloc(sizeof(int) * batch_size);
    int i, j;
    int ret = 0;
    struct timeval t1, t2;

    float** db_content_vec = (float**)malloc(feature_size * sizeof(float*)); /*row = feature_size col = db_size*/
    for (i = 0; i < feature_size; ++i) {
        db_content_vec[i] = (float*)malloc(db_size * sizeof(float));
        for (j = 0; j < db_size; ++j) {
            db_content_vec[i][j] = rand() % 20 -10;
        }
    }

    float** input_content_vec = (float**)malloc(batch_size * sizeof(float*)); /*row = batch_size col = feature_size*/
    for (i = 0; i < batch_size; ++i) {
        input_content_vec[i] = (float*)malloc(feature_size * sizeof(float));
        for (j = 0; j < feature_size; ++j) {
            input_content_vec[i][j] = rand() % 20 -10;
        }
    }

    float** ref_res = (float**)malloc(sizeof(float*) * batch_size); /* row = batch_size col = db_size */
    for (i = 0; i < batch_size; ++i) {
        ref_res[i] = (float*)malloc(db_size * sizeof(float));
    }

    for (i = 0; i < feature_size; ++i) {
        for (j = 0; j < db_size; ++j) {
            db_data[i * db_size + j] = db_content_vec[i][j];
        }
    }

    ret = calc_sqrt_transposed(db_content_vec, feature_size, db_size, db_feature);

    for (i = 0; i < batch_size; i++) {
        for (j = 0; j < feature_size; j++) {
            input_data[i * feature_size + j] = input_content_vec[i][j];
        }
    }

    ret = tpu_feature_match(handle, input_data, db_data, db_feature, output_similarity, output_index,
                            batch_size, feature_size, db_size, tpu_time);
    if (ret) {
        printf("tpu_feature_match failed!\n");
        goto exit;
    }

    gettimeofday(&t1, NULL);
    ret = cpu_feature_match(input_content_vec, batch_size, feature_size, db_content_vec, db_size, ref_res);
    if (ret) {
        printf("cpu_feature_match failed!\n");
        goto exit;
    }
    gettimeofday(&t2, NULL);
    printf("Feature Match CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *cpu_time += TIME_COST_US(t1, t2);

    ret = res_compare_float(output_similarity, output_index, ref_res, batch_size, db_size);
    if (ret) {
        printf("FEATURE MATCHING COMPARE ERROR\n");
        goto exit;
    }

exit:
    free(input_data);
    free(db_data);
    free(db_feature);
    free(output_similarity);
    free(output_index);
    for(i = 0; i < batch_size; i++) {
        free(input_content_vec[i]);
        free(ref_res[i]);
    }
    for(i = 0; i < feature_size; i++) {
        free(db_content_vec[i]);
    }
    free(input_content_vec);
    free(db_content_vec);
    free(ref_res);
    return ret;
}

int main(int argc, char* args[])
{
    printf("log will be saved in special_size_test_cv_feature_match_float.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_feature_match_float.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return 1;
    }
    struct timespec tp;
    clock_gettime(0, &tp);
    srand(tp.tv_nsec);

    int batch_size_num[] = {1, 5, 10};
    int feature_size_num[] = {1, 10, 100, 1000};
    int db_size_num[] = {1000, 10000};
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
    cv_fm_test_rand(1, 1, 1000, handle, test_cputime, test_tputime);
    for(i = 0; i < 3; i++) {
        int batch_size = batch_size_num[i];
        for(j = 0;j < 4; j++) {
            int feature_size = feature_size_num[j];
            for(n = 0; n < 2; n++) {
                int db_size = db_size_num[n];
                long int cputime = 0;
                long int tputime = 0;
                long int *cpu_time = &cputime;
                long int *tpu_time = &tputime;
                printf("batch_size = %d, feature_size = %d, db_size = %d\n",
                        batch_size, feature_size, db_size);
                for(int loop = 0; loop < 3; loop++) {
                    ret = cv_fm_test_rand(batch_size, feature_size, db_size, handle, cpu_time, tpu_time);
                    if (ret) {
                        printf("------Test Feature Match Failed!------\n");
                        return -1;
                    }
                }
                printf("------float average CPU time = %ldus------\n", cputime / 3);
                printf("------float average TPU time = %ldus------\n", tputime / 3);
            }

        }
    }

    bm_dev_free(handle);
    fclose(stdout);
    stdout = original_stdout;
    return ret;
}