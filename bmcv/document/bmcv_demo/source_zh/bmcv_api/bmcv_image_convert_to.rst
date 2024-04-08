bmcv_image_convert_to
======================

该接口用于实现图像像素线性变化，具体数据关系可用如下公式表示：

.. math::
    y=kx+b


**接口形式：**

    .. code-block:: c

      bm_status_t bmcv_image_convert_to (
                  bm_handle_t handle,
                  int input_num,
                  bmcv_convert_to_attr convert_to_attr,
                  bm_image* input,
                  bm_image* output);


**参数说明：**

* bm_handle_t handle

  输入参数。bm_handle 句柄。

* int input_num

  输入参数。输入图片数，如果 input_num > 1, 那么多个输入图像必须是连续存储的（可以使用 bm_image_alloc_contiguous_mem 给多张图申请连续空间）。

* bmcv_convert_to_attr convert_to_attr

  输入参数。每张图片对应的配置参数。

* bm_image\* input

  输入参数。输入 bm_image。每个 bm_image 外部需要调用 bmcv_image_create 创建。image 内存可以使用 bm_image_alloc_dev_mem 或者 bm_image_copy_host_to_device 来开辟新的内存,或者使用 bmcv_image_attach 来 attach 已有的内存。

* bm_image\* output

  输出参数。输出 bm_image。每个 bm_image 外部需要调用 bmcv_image_create 创建。image 内存可以通过 bm_image_alloc_dev_mem 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。如果不主动分配将在 api 内部进行自行分配。


**数据类型说明：**

    .. code-block:: c

        struct bmcv_convert_to_attr_s{
              float alpha_0;
              float beta_0;
              float alpha_1;
              float beta_1;
              float alpha_2;
              float beta_2;
        } bmcv_convert_to_attr;


**参数说明：**

* float alpha_0

  第 0 个 channel 进行线性变换的系数

* float beta_0

  第 0 个 channel 进行线性变换的偏移

* float alpha_1

  第 1 个 channel 进行线性变换的系数

* float beta_1

  第 1 个 channel 进行线性变换的偏移

* float alpha_2

  第 2 个 channel 进行线性变换的系数

* float beta_2

 第 2 个 channel 进行线性变换的偏移


**返回值说明：**

* BM_SUCCESS: 成功

* 其他: 失败


**示例代码**

    .. code-block:: c

        int image_num = 4, image_channel = 3;
        int image_w = 1920, image_h = 1080;
        bm_image input_images[4], output_images[4];
        struct bmcv_convert_to_attr convert_to_attr;
        convert_to_attr.alpha_0 = 1;
        convert_to_attr.beta_0 = 0;
        convert_to_attr.alpha_1 = 1;
        convert_to_attr.beta_1 = 0;
        convert_to_attr.alpha_2 = 1;
        convert_to_attr.beta_2 = 0;
        int img_size = image_w * image_h * image_channel;
        bm_handle_t handle;
        bm_status_t ret = BM_SUCCESS;

        unsigned char* img_data = (unsigned char*)malloc(img_size * image_num * sizeof(unsigned char));
        unsigned char* res_data = (unsigned char*)malloc(img_size * image_num * sizeof(unsigned char));

        ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
            printf("bm_dev_request failed. ret = %d\n", ret);
            exit(-1);
        }

        struct timespec tp;
        clock_gettime(NULL, &tp);
        srand(tp.tv_nsec);

        for (int i = 0; i < img_size * image_num; ++i) {
            img_data[i] = rand() % 256;
        }

        for (int img_idx = 0; img_idx < image_num; ++img_idx) {
            ret = bm_image_create(handle, image_h, image_w, FORMAT_BGR_PLANAR,
                                DATA_TYPE_EXT_1N_BYTE, &input_images[img_idx], NULL);
            if (ret != BM_SUCCESS) {
                printf("bm_image_create failed. ret = %d\n", ret);
                exit(-1);
            }
        }

        ret = bm_image_alloc_contiguous_mem(image_num, input_images, 0);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_contiguous_mem failed. ret = %d\n", ret);
            exit(-1);
        }

        for (int img_idx = 0; img_idx < image_num; img_idx++) {
            unsigned char* input_img_data = img_data + img_size * img_idx;
            ret = bm_image_copy_host_to_device(input_images[img_idx], (void **)&input_img_data);
            if (ret != BM_SUCCESS) {
                printf("bm_image_copy_host_to_device failed. ret = %d\n", ret);
                exit(-1);
            }
        }

        for (int img_idx = 0; img_idx < image_num; img_idx++) {
            ret = bm_image_create(handle, image_h, image_w, FORMAT_BGR_PLANAR,
                                  DATA_TYPE_EXT_1N_BYTE, &output_images[img_idx]);
            if (ret != BM_SUCCESS) {
                printf("bm_image_create failed. ret = %d\n", ret);
                exit(-1);
            }
        }
        ret = bm_image_alloc_contiguous_mem(image_num, output_images, 1);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_contiguous_mem failed. ret = %d\n", ret);
            exit(-1);
        }

        ret = bmcv_image_convert_to(handle, image_num, convert_to_attr, input_images, output_images);
        if (ret != BM_SUCCESS) {
            printf("bmcv_image_convert_to failed. ret = %d\n", ret);
            exit(-1);
        }

        for (int img_idx = 0; img_idx < image_num; img_idx++) {
            unsigned char* res_img_data = res_data + img_size * img_idx;
            ret = bm_image_copy_device_to_host(output_images[img_idx], (void **)&res_img_data);
            if (ret != BM_SUCCESS) {
                printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
                exit(-1);
            }
        }

        bm_image_free_contiguous_mem(image_num, input_images);
        bm_image_free_contiguous_mem(image_num, output_images);
        for(int i = 0; i < image_num; i++) {
            bm_image_destroy(input_images + i);
            bm_image_destroy(output_images + i);
        }
        bm_dev_free(handle);
        free(img_data);
        free(res_data);


**格式支持:**

1. 该接口支持下列 image_format 的转化：

* FORMAT_BGR_PLANAR ——> FORMAT_BGR_PLANAR
* FORMAT_RGB_PLANAR ——> FORMAT_RGB_PLANAR
* FORMAT_GRAY       ——> FORMAT_GRAY

2. 该接口支持下列情形data type之间的转换：

* DATA_TYPE_EXT_1N_BYTE        ——> DATA_TYPE_EXT_FLOAT32
* DATA_TYPE_EXT_1N_BYTE        ——> DATA_TYPE_EXT_1N_BYTE
* DATA_TYPE_EXT_1N_BYTE_SIGNED ——> DATA_TYPE_EXT_1N_BYTE_SIGNED
* DATA_TYPE_EXT_1N_BYTE        ——> DATA_TYPE_EXT_1N_BYTE_SIGNED
* DATA_TYPE_EXT_FLOAT32        ——> DATA_TYPE_EXT_FLOAT32


**注意事项：**

1. 在调用 bmcv_image_convert_to()之前必须确保输入的 image 内存已经申请。

2. 输入的各个 image 的宽、高以及 data_type、image_format 必须相同。

3. 输出的各个 image 的宽、高以及 data_type、image_format 必须相同。

4. 输入 image 宽、高必须等于输出 image 宽高。

5. image_num 必须大于 0。

6. 输出 image 的 stride 必须等于 width。

7. 输入 image 的 stride 必须大于等于 width。

8. 输入图片的最小尺寸为16 * 16，最大尺寸为4096 * 4096，当输入 data type 为 DATA_TYPE_EXT_1N_BYTE 时，最大尺寸可以支持8192 * 8192。

9. 输出图片的最小尺寸为16 * 16，最大尺寸8192 * 8192。