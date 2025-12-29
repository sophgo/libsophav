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
    bm_handle_t handle;
};

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
                            int db_size)
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
    return 0;
}


static int cv_fm_test_rand(int batch_size, int feature_size, int db_size, bm_handle_t handle)
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
                            batch_size, feature_size, db_size);
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

void* test_fm_all(void* args)
{
    struct cv_fm_thread_arg_t* cv_fm_thread_arg = (struct cv_fm_thread_arg_t*)args;

    int loop = cv_fm_thread_arg->loop_num;
    int batch_size = cv_fm_thread_arg->batch_size;
    int feature_size = cv_fm_thread_arg->feature_size;
    int db_size = cv_fm_thread_arg->db_size;
    bm_handle_t handle = cv_fm_thread_arg->handle;
    int i;
    int ret = 0;

    for (i = 0; i < loop; ++i) {
        ret = cv_fm_test_rand(batch_size, feature_size, db_size, handle);
        if (ret) {
            printf("------Test Feature Match Failed!------\n");
            exit(-1);
        }
        printf("------Test Feature Match Done!------\n");
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
        printf("%s thread_num loop batch_size feature_size db_size\n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 1 1 2 100 1000\n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) batch_size = atoi(args[3]);
    if (argc > 4) feature_size = atoi(args[4]);
    if (argc > 5) db_size  = atoi(args[5]);


    pthread_t pid[thread_num];
    struct cv_fm_thread_arg_t fm_thread_arg[thread_num];
    for (i = 0; i < thread_num; i++) {
        fm_thread_arg[i].loop_num = loop;
        fm_thread_arg[i].batch_size = batch_size;
        fm_thread_arg[i].feature_size = feature_size;
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