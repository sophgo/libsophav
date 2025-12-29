bmcv_attribute_filter_topk
------------------------------

**描述：**

海康-属性过滤排序算子。输入数据具有空间，时间，属性值和相似度这四个维度的属性，每个维度数据先按照特定的标准进行属性过滤，然后按照相似度值进行排序。

**语法：**

.. code-block:: c
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_attribute_filter_topk(
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
            bm_device_mem_t space_bits_dev_mem,
            bm_device_mem_t space_points_dev_mem,
            bm_device_mem_t time_points_dev_mem,
            bm_device_mem_t attributes_dev_mem,
            bm_device_mem_t similarity_data_dev_mem,
            bm_device_mem_t filtered_idx_dev_mem,
            bm_device_mem_t filtered_similarity_data_dev_mem,
            bm_device_mem_t topk_idx_dev_mem,
            bm_device_mem_t topk_data_dev_mem);

**参数：**

.. list-table:: bmcv_attribute_filter_topk 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用bm_dev_request获取。
    * - data_type
      - 输入
      - 输入数据类型。
    * - data_count
      - 输入
      - 输入数据数量。
    * - max_space_points
      - 输入
      - 最大空间点个数上限。
    * - time_range_min
      - 输入
      - 时间范围下限。
    * - time_range_max
      - 输入
      - 时间范围上限。
    * - attr3_flag
      - 输入
      - 属性值过滤标志位。
    * - topk
      - 输入
      - 排序并获取到的最大或最小输出数据的个数。
    * - threshold
      - 输入
      - 相似度过滤阈值。
    * - attr_mask
      - 输入
      - 属性掩码。
    * - space_bits_dev_mem
      - 输入
      - 空间位图数据的设备地址。
    * - space_points_dev_mem
      - 输入
      - 空间点数据的设备地址。
    * - time_points_dev_mem
      - 输入
      - 时间点数据的设备地址。
    * - attributes_dev_mem
      - 输入
      - 属性值数据的设备地址。
    * - similarity_data_dev_mem
      - 输入
      - 相似度数据的设备地址。
    * - filtered_idx_dev_mem
      - 输入
      - 用于暂存过滤后索引数据的设备地址。
    * - filtered_similarity_data_dev_mem
      - 输入
      - 用于暂存过滤后相似度数据的设备地址。
    * - topk_idx_dev_mem
      - 输出
      - 最大或最小topk个输出索引数据的设备地址。
    * - topk_data_dev_mem
      - 输出
      - 最大或最小topk个输出相似度数据的设备地址。


**返回值：**

该函数成功调用时, 返回BM_SUCCESS，调用失败时，返回BM_ERR_PARAM等错误码。

**注意事项：**

1. data_type支持fp32和uint32两种数据类型，其中fp32数据将进行降序排序，uint32数据类型将进行升序排序。

2. data_count需是1024的倍数，理论最大支持数据量受内存容量限制，目前压测过的范围为1024-4096000。

3. max_space_points设定范围为-1和(0,300000]，其中设定为-1时表示空间属性不进行过滤。

4. time_range_min和time_range_max的支持范围为(0,315360000)，其中time_range_min小于等于time_range_max。

5. attr3_flag支持0和1两种模式，0表示属性值和属性掩码按位与操作后，数值不等于属性掩码，该数据被过滤；1表示属性值和属性掩码按位与操作后，数值小于等于0，该数据被过滤。

6. topk值支持5，10，100，1000，10000，20000，50000，且topk值须小于等于data_count。

7. threshold相似度阈值默认为float类型，如进行uint32类型数据过滤，接口内会将threshold数值强制转为uint32类型。

8. attr_mask支持范围为[-128,127]。


**代码示例：**

.. code-block:: c
    :linenos:
    :lineno-start: 1
    :force:

    #include "stdio.h"
    #include "stdlib.h"
    #include <time.h>
    #include "bmcv_api_ext_c.h"
    #include <stdint.h>
    #ifdef __linux__
    #include <sys/time.h>
    #else
    #include <windows.h>
    #include "time.h"
    #endif

    #define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))
    #define MAX_BITS 128
    #define MAX_HD MAX_BITS
    #define TYPICAL_HD 30
    uint32_t generate_hamming_distance(int max_bits) {
        uint32_t hd = 0;
        for(int i = 0; i < max_bits; i++) {
            if((float)rand()/RAND_MAX < 0.3) {
                hd++;
            }
        }
        return hd;
    }

    void generate_hamming_data(uint32_t* out, size_t count) {
        for(size_t i = 0; i < count; i++) {
            out[i] = generate_hamming_distance(MAX_BITS);
            if(i == 0) out[i] = 0;
            if(i == 1) out[i] = MAX_HD;
            if(i == 2) out[i] = TYPICAL_HD-1;
            if(i == 3) out[i] = TYPICAL_HD+1;
            if(i % 10000 == 0) out[i] = UINT32_MAX;
        }
    }

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
        generate_hamming_data(*similarity_data_u32, data_count);
        //Fill in the attributes of each feature
        for (size_t i = 0; i < data_count; ++i) {
            (*time_points)[i] = rand() % 315360001;
            (*space_points)[i] = rand() % 300001;
            (*attributes)[i] = (int8_t)(rand() % 256 - 128);
            (*similarity_data_fp32)[i] = (float)rand() / RAND_MAX * 3000000.0f - 1500000.0f;
        }
    }

    void convert_int64_to_int32(const int64_t* input, int32_t* output, size_t n) {
        for (size_t i = 0; i < n; i++) {
            uint64_t value = (uint64_t)input[i]; //Use unsigned to ensure correct shifting
            output[2 * i] = (int32_t)(value & 0xFFFFFFFF);
            output[2 * i + 1] = (int32_t)((value >> 32) & 0xFFFFFFFF);
        }
    }

    int main(int argc, char *argv[]) {
        int data_type = 5 + 3 * (rand() % 2);
        int topk_num[] = {5, 10, 100, 1000, 10000, 50000};
        int size = sizeof(topk_num) / sizeof(topk_num[0]);
        int rand_num = rand() % size;
        int topk = topk_num[rand_num];
        int data_count = 1024 * (1 + rand() % 4000);
        int max_space_points = 300000;
        float threshold = 3.0;
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

        generate_test_data(data_count, max_space_points, &space_bits, &time_points, &space_points,
                            &attributes, active_points_num, &similarity_data_fp32, &similarity_data_u32);
        void* topk_data_tpu = (void*)malloc(topk * 4);
        int* topk_idx_tpu = (int*)malloc(topk * sizeof(int));
        bm_status_t bm_ret;
        bm_device_mem_t space_bits_dev_mem;
        bm_device_mem_t space_points_dev_mem;
        bm_device_mem_t time_points_dev_mem;
        bm_device_mem_t attributes_dev_mem;
        bm_device_mem_t similarity_data_dev_mem;
        bm_device_mem_t filtered_similarity_data_dev_mem;
        bm_device_mem_t filtered_idx_dev_mem;
        bm_device_mem_t topk_data_dev_mem;
        bm_device_mem_t topk_idx_dev_mem;
        size_t type_bytes = (data_type == 5 ? sizeof(float) : sizeof(uint32_t));
        const size_t bitmap_size = (max_space_points + 64 - 1) / 64;
        struct timeval t1, t2;
        int per_row_num = 1024;
        int per_row_num_bits_map = per_row_num / 32;
        int space_bits_num = ((bitmap_size * 2 + per_row_num_bits_map - 1) / per_row_num_bits_map) * per_row_num_bits_map;
        int32_t* space_bits_int32 = (int32_t*)calloc(space_bits_num * sizeof(int32_t), sizeof(int32_t));
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
        bm_memcpy_s2d(handle, space_bits_dev_mem, space_bits_int32);
        bm_memcpy_s2d(handle, space_points_dev_mem, space_points);
        bm_memcpy_s2d(handle, time_points_dev_mem, time_points);
        bm_memcpy_s2d(handle, attributes_dev_mem, attributes);
        if (data_type == 5) {
            bm_memcpy_s2d(handle, similarity_data_dev_mem, similarity_data_fp32);
        } else {
            bm_memcpy_s2d(handle, similarity_data_dev_mem, similarity_data_u32);
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
        free(space_bits_int32);
        free(space_bits);
        free(time_points);
        free(space_points);
        free(attributes);
        free(similarity_data_fp32);
        free(similarity_data_u32);
        bm_dev_free(handle);
        return ret;
    }