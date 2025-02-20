bmcv_image_vpp_csc_matrix_convert
---------------------------------

| 【描述】

| 默认情况下，bmcv_image_vpp_convert 使用的是 BT_601 标准进行色域转换。有些情况下需要使用其他标准，或者用户自定义 csc 参数。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_vpp_csc_matrix_convert(
        bm_handle_t  handle,
        int output_num,
        bm_image input,
        bm_image *output,
        csc_type_t csc,
        csc_matrix_t * matrix = NULL,
        bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR,
        bmcv_rect_t *crop_rect = NULL);

| 【参数】

.. list-table:: bmcv_image_vpp_csc_matrix_convert 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - image_num
      - 输入
      - 输入 bm_image 数量。
    * - input
      - 输入
      - 输入 bm_image 对象。
    * - \*output
      - 输出
      - 输出 bm_image 对象指针。
    * - csc
      - 输入
      - 色域转换枚举类型。
    * - \*matrix
      - 输入
      - 色域转换自定义矩阵，当且仅当 csc 为 CSC_USER_DEFINED_MATRIX 时这个值才生效。
    * - algorithm
      - 输入
      - resize 算法选择，包括 BMCV_INTER_NEAREST、BMCV_INTER_LINEAR 和 BMCV_INTER_BICUBIC 三种，默认情况下是双线性差值。

| 【注意】

该接口的参数数据类型与注意事项与 bmcv_image_vpp_basic 接口相同。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【代码示例】

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    #include <limits.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    #include "bmcv_api_ext_c.h"

    int main() {
        char* filename_src = "path/to/src_file";
        char* filename_dst = "path/to/dst_file";
        int in_width = 1920;
        int in_height = 1080;
        int out_width = 1920;
        int out_height = 1080;
        // int use_real_img = 1;
        bm_image_format_ext src_format = 0;
        bm_image_format_ext dst_format = 0;
        bmcv_rect_t crop_rect = {
            .start_x = 0,
            .start_y = 0,
            .crop_w = 200,
            .crop_h = 200};
        bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR;
        bm_image_data_format_ext data_format = 1;

        bm_status_t ret = BM_SUCCESS;

        int src_size = in_width * in_height * 3 / 2;
        int dst_size = out_width * out_height * 3 / 2;
        unsigned char *src_data = (unsigned char *)malloc(src_size);
        unsigned char *dst_data = (unsigned char *)malloc(dst_size);

        FILE *file;
        file = fopen(filename_src, "rb");
        fread(src_data, sizeof(unsigned char), src_size, file);
        fclose(file);

        bm_handle_t handle = NULL;
        int dev_id = 0;
        bm_image src, dst;

        ret = bm_dev_request(&handle, dev_id);
        if (ret != BM_SUCCESS) {
            printf("Create bm handle failed. ret = %d\n", ret);
            return ret;
        }

        bm_image_create(handle, in_height, in_width, src_format, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
        bm_image_create(handle, out_height, out_width, dst_format, data_format, &dst, NULL);
        bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
        bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

        int src_image_byte_size[4] = {0};
        bm_image_get_byte_size(src, src_image_byte_size);

        void *src_in_ptr[4] = {(void *)src_data,
                              (void *)((char *)src_data + src_image_byte_size[0]),
                              (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1]),
                              (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1] + src_image_byte_size[2])};

        bm_image_copy_host_to_device(src, (void **)src_in_ptr);

        // ret = bmcv_image_vpp_convert(handle, 1, src, &dst, crop_rect, algorithm);
        ret = bmcv_image_vpp_csc_matrix_convert(handle, 1, src, &dst, CSC_MAX_ENUM, NULL, algorithm, &crop_rect);

        int dst_image_byte_size[4] = {0};
        bm_image_get_byte_size(dst, dst_image_byte_size);

        void *dst_in_ptr[4] = {(void *)dst_data,
                              (void *)((char *)dst_data + dst_image_byte_size[0]),
                              (void *)((char *)dst_data + dst_image_byte_size[0] + dst_image_byte_size[1]),
                              (void *)((char *)dst_data + dst_image_byte_size[0] + dst_image_byte_size[1] + dst_image_byte_size[2])};

        bm_image_copy_device_to_host(dst, (void **)dst_in_ptr);

        bm_image_destroy(&src);
        bm_image_destroy(&dst);
        bm_dev_free(handle);

        file = fopen(filename_dst, "wb");
        fwrite(dst_data, sizeof(unsigned char), dst_size, file);
        fclose(file);


        free(src_data);
        free(dst_data);

        return ret;
    }
