#include "bmcv_api_ext_c.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

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


int main()
{
    int batch_size = rand() % 8 + 1;
    int feature_size = rand() % 1000 + 1;
    int db_size = (rand() % 90 + 1) * 1000;
    bm_handle_t handle;
    int ret = 0;

    ret = (int)bm_dev_request(&handle, 0);
    if (ret) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return ret;
    }

    float* input_data = (float*)malloc(sizeof(float) * batch_size * feature_size);
    float* db_data = (float*)malloc(sizeof(float) * db_size * feature_size);
    float* db_feature = (float*)malloc(sizeof(float) * db_size);
    float* output_similarity = (float*)malloc(sizeof(float) * batch_size); /*float*/
    int* output_index = (int*)malloc(sizeof(int) * batch_size);
    int i, j;

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

    ret = bmcv_feature_match_normalized(handle, bm_mem_from_system(input_data), bm_mem_from_system(db_data),
                                bm_mem_from_system(db_feature), bm_mem_from_system(output_similarity),
                                bm_mem_from_system(output_index), batch_size, feature_size, db_size);


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

    bm_dev_free(handle);
    return ret;
}
