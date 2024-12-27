#include "bmcv_api_ext_c.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

int main()
{
    int batch_size = rand() % 10 + 1;
    int feature_size = rand() % 1000 + 1;
    int rshiftbits = rand() % 3;
    int sort_cnt = rand() % 30 + 1;
    int db_size = (rand() % 90 + 1) * 1000;
    bm_handle_t handle;
    int ret = 0;

    ret = (int)bm_dev_request(&handle, 0);

    int8_t* input_data = (int8_t*)malloc(sizeof(int8_t) * batch_size * feature_size);
    int8_t* db_data = (int8_t*)malloc(sizeof(int8_t) * db_size * feature_size);
    short* output_similarity = (short*)malloc(sizeof(short) * batch_size * db_size);
    int* output_index = (int*)malloc(sizeof(int) * batch_size * db_size);
    int i, j;
    int8_t temp_val;

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

    ret = bmcv_feature_match(handle, bm_mem_from_system(input_data), bm_mem_from_system(db_data),
                            bm_mem_from_system(output_similarity), bm_mem_from_system(output_index),
                            batch_size, feature_size, db_size, sort_cnt, rshiftbits);

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

    bm_dev_free(handle);
    return ret;
}
