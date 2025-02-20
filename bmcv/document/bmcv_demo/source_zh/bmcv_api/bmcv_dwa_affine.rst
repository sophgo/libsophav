bmcv_dwa_affine
----------------

| 【描述】
| 去畸变仿射(DWA)模块的仿射校正功能，通过线性组合和平移操作，来保持图像中的平行线仍然平行，并保持源图像两点之间的距离比例，从而对图像进行旋转、缩放、平移和倾斜等变换。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_dwa_affine(bm_handle_t          handle,
                                bm_image             input_image,
                                bm_image             output_image,
                                bmcv_affine_attr_s   affine_attr);

| 【参数】

.. list-table:: bmcv_dwa_affine 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取
    * - input_image
      - 输入
      - 输入的源图像，通过调用 bm_image_create 创建
    * - output_image
      - 输出
      - 输出的仿射变化校正后的图像，通过调用 bm_image_create 创建
    * - affine_attr
      - 输入
      - affine功能的参数配置列表，包括u32RegionNum astRegionAttr stDestSize

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【数据类型说明】

.. list-table:: bmcv_affine_attr_s 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - u32RegionNum
      - 输入
      - 配置范围:[1, 32]，在源图像中进行 Affine 操作的区域数量
    * - astRegionAttr
      - 输入
      - 结构体数组，存储源图像中每个 Affine 区域的四个顶点坐标
    * - stDestSize
      - 输入
      - 结构体，存储 Affine 操作后的目标区域尺寸

| 【格式支持】

1. 输入和输出的数据类型：

+-----+-------------------------------+
| num | data_type                     |
+=====+===============================+
|  1  | DATA_TYPE_EXT_1N_BYTE         |
+-----+-------------------------------+

2. 输入和输出的色彩格式必须保持一致，可支持：

+-----+-------------------------------+
| num | image_format                  |
+=====+===============================+
|  1  | FORMAT_RGB_PLANAR             |
+-----+-------------------------------+
|  2  | FORMAT_YUV420P                |
+-----+-------------------------------+
|  3  | FORMAT_YUV444P                |
+-----+-------------------------------+
|  4  | FORMAT_GRAY                   |
+-----+-------------------------------+

| 【注意】

1. 输入输出所有 bm_image 结构必须提前创建，否则返回失败；

2. 支持图像的分辨率为32x32~4096x4096，且要求32对齐；

3. 用户需根据源图像中想要进行 Affine 操作的位置及目标区域尺寸自行输入配置参数列表affine_attr；

4. astRegionAttr数组中，原图中某区域四个顶点的坐标顺序分别为左上、右上、左下、右下；

5. 推荐stDestSize的width与输出图像output_image的width相同，否则结果需要裁剪。

| 【代码示例】

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "bmcv_api_ext_c.h"
    #include <unistd.h>

    int main() {
        int src_h = 1080, src_w = 1920, dst_w = 1920, dst_h = 1080, dev_id = 0;
        bm_image_format_ext fmt = FORMAT_YUV420P;
        char *src_name = "path/to/src", *dst_name = "path/to/dst";
        bm_handle_t handle = NULL;
        bmcv_affine_attr_s affine_attr = {0};
        affine_attr.u32RegionNum = 4;
        affine_attr.stDestSize.u32Width = 400;
        affine_attr.stDestSize.u32Height = 400;
        // bmcv_point2f_s faces[9][4] = {0};
        bmcv_point2f_s faces[9][4] = {
            { {.x = 722.755, .y = 65.7575}, {.x = 828.402, .y = 80.6858}, {.x = 707.827, .y = 171.405}, {.x = 813.474, .y = 186.333} },
            { {.x = 494.919, .y = 117.918}, {.x = 605.38,  .y = 109.453}, {.x = 503.384, .y = 228.378}, {.x = 613.845, .y = 219.913} },
            { {.x = 1509.06, .y = 147.139}, {.x = 1592.4,  .y = 193.044}, {.x = 1463.15, .y = 230.48 }, {.x = 1546.5,  .y = 276.383} },
            { {.x = 1580.21, .y = 66.7939}, {.x = 1694.1,  .y = 70.356 }, {.x = 1576.65, .y = 180.682}, {.x = 1690.54, .y = 184.243} },
            { {.x = 178.76,  .y = 90.4814}, {.x = 286.234, .y = 80.799 }, {.x = 188.442, .y = 197.955}, {.x = 295.916, .y = 188.273} },
            { {.x = 1195.57, .y = 139.226}, {.x = 1292.69, .y = 104.122}, {.x = 1230.68, .y = 236.34}, {.x = 1327.79, .y = 201.236}, },
            { {.x = 398.669, .y = 109.872}, {.x = 501.93, .y = 133.357}, {.x = 375.184, .y = 213.133}, {.x = 478.445, .y = 236.618}, },
            { {.x = 845.989, .y = 94.591}, {.x = 949.411, .y = 63.6143}, {.x = 876.966, .y = 198.013}, {.x = 980.388, .y = 167.036}, },
            { {.x = 1060.19, .y = 58.7882}, {.x = 1170.61, .y = 61.9105}, {.x = 1057.07, .y = 169.203}, {.x = 1167.48, .y = 172.325}, },
        };
        memcpy(affine_attr.astRegionAttr, faces, sizeof(faces));

        int ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            exit(-1);
        }

        bm_image src, dst;

        bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
        bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

        int image_byte_size[4] = {0};
        bm_image_get_byte_size(src, image_byte_size);
        int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
        unsigned char *input_data = (unsigned char*)malloc(byte_size);
        FILE *fp_src = fopen(src_name, "rb");
        if (fread((void*)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
            printf("file size is less than required bytes%d\n", byte_size);
        }
        fclose(fp_src);
        void* in_ptr[4] = {(void *)input_data,
                            (void *)((unsigned char*)input_data + image_byte_size[0]),
                            (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1]),
                            (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
        bm_image_copy_host_to_device(src, in_ptr);

        bmcv_dwa_affine(handle, src, dst, affine_attr);

        bm_image_get_byte_size(dst, image_byte_size);
        byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
        unsigned char* output_ptr = (unsigned char*)malloc(byte_size);
        void* out_ptr[4] = {(void*)output_ptr,
                            (void*)((unsigned char*)output_ptr + image_byte_size[0]),
                            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
        bm_image_copy_device_to_host(dst, (void **)out_ptr);

        FILE *fp_dst = fopen(dst_name, "wb");
        if (fwrite((void *)output_ptr, 1, byte_size, fp_dst) < (unsigned int)byte_size){
            printf("file size is less than %d required bytes\n", byte_size);
        };
        fclose(fp_dst);

        bm_image_destroy(&src);
        bm_image_destroy(&dst);

        free(input_data);
        free(output_ptr);

        bm_dev_free(handle);

        return 0;
    }

