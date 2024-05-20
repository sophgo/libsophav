bmcv_image_bayer2rgb
------------------------------

**描述：**

该接口可以将bayerBG8或bayerRG8格式图像转化为rgb plannar格式图像。

**语法：**

.. code-block:: c
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_bayer2rgb(
        bm_handle_t handle,
        unsigned char* convd_kernel,
        bm_image input,
        bm_image output);

**参数：**

.. list-table:: bmcv_image_bayer2rgb 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用bm_dev_request获取。
    * - convd_kernel
      - 输入
      - 卷积核数值。
    * - input
      - 输入
      - 输入参数。输入bayer格式图像的 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以使用 bm_image_alloc_dev_mem 或者 bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。
    * - output
      - 输出
      - 输出图像的bm_image，bm_image需要外部调用bmcv_image_create创建。image内存可以使用bm_image_alloc_dev_mem或者bm_image_copy_host_to_device来开辟新的内存，或者使用bmcv_image_attach来attach已有的内存。

**返回值：**

该函数成功调用时, 返回BM_SUCCESS。

**注意事项：**

1. input的格式目前支持bayerBG8或bayerRG8，bm_image_create步骤中bayerBG8创建为FORMAT_BAYER格式，bayerRG8创建为FORMAT_BAYER_RG8格式。

2. output的格式是rgb plannar， data_type均为uint8类型。

3. 输入图像的宽高需为偶数，且最大宽高均为4096。

4. 如调用该接口的程序为多线程程序，需要在创建bm_image前和销毁bm_image后加线程锁。

**代码示例：**

.. code-block:: c
    :linenos:
    :lineno-start: 1
    :force:

        #define KERNEL_SIZE 3 * 3 * 3 * 4 * 32
        #define CONVD_MATRIX 12 * 9
        const unsigned char convd_kernel_bg8[CONVD_MATRIX] = {1, 0, 1, 0, 0, 0, 1, 0, 1, //Rb
                                                              0, 0, 2, 0, 0, 0, 0, 0, 2, //Rg1
                                                              0, 0, 0, 0, 0, 0, 2, 0, 2, //Rg2
                                                              0, 0, 0, 0, 0, 0, 0, 0, 4, //Rr
                                                              4, 0, 0, 0, 0, 0, 0, 0, 0, //Bb
                                                              2, 0, 2, 0, 0, 0, 0, 0, 0, //Bg1
                                                              2, 0, 0, 0, 0, 0, 2, 0, 0, //Bg2
                                                              1, 0, 1, 0, 0, 0, 1, 0, 1, //Br
                                                              0, 1, 0, 1, 0, 1, 0, 1, 0, //Gb
                                                              0, 0, 0, 0, 0, 4, 0, 0, 0, //Gg1
                                                              0, 0, 0, 0, 0, 0, 0, 4, 0, //Gg2
                                                              0, 1, 0, 1, 0, 1, 0, 1, 0};//Gr

        const unsigned char convd_kernel_rg8[CONVD_MATRIX] = {4, 0, 0, 0, 0, 0, 0, 0, 0, //Rr
                                                              2, 0, 2, 0, 0, 0, 0, 0, 0, //Rg1
                                                              2, 0, 0, 0, 0, 0, 2, 0, 0, //Rg2
                                                              1, 0, 1, 0, 0, 0, 1, 0, 1, //Rb
                                                              1, 0, 1, 0, 0, 0, 1, 0, 1, //Br
                                                              0, 0, 2, 0, 0, 0, 0, 0, 2, //Bg1
                                                              0, 0, 0, 2, 0, 2, 0, 0, 0, //Bg2
                                                              0, 0, 0, 0, 0, 0, 0, 0, 4, //Bb
                                                              1, 0, 1, 0, 0, 0, 1, 0, 1, //Gr
                                                              0, 0, 0, 0, 0, 4, 0, 0, 0, //Gg1
                                                              0, 0, 0, 0, 0, 0, 0, 4, 0, //Gg2
                                                              0, 1, 0, 1, 0, 1, 0, 1, 0};//Gb
        int width     = 1920;
        int height    = 1080;
        int dev_id    = 0;
        unsigned char* input = (unsigned char*)malloc(width * height);
        unsigned char* output = (unsigned char*)malloc(width * height * 3);
        bm_handle_t handle;
        bm_status_t dev_ret = bm_dev_request(&handle, dev_id);

        bm_image input_img;
        bm_image output_img;
        bm_image_create(handle, height, width, FORMAT_BAYER_RG8, DATA_TYPE_EXT_1N_BYTE, &input_img);
        //bm_image_create(handle, height, width, FORMAT_BAYER, DATA_TYPE_EXT_1N_BYTE, &input_img); //bayerBG8
        bm_image_create(handle, height, width, FORMAT_RGB_PLANAR, DATA_TYPE_EXT_1N_BYTE, &output_img);
        bm_image_alloc_dev_mem(input_img, BMCV_HEAP_ANY);
        bm_image_alloc_dev_mem(output_img, BMCV_HEAP_ANY);

        unsigned char kernel_data[KERNEL_SIZE];
        memset(kernel_data, 0, KERNEL_SIZE);
        // constructing convd_kernel_data
        for (int i = 0;i < 12;i++) {
            for (int j = 0;j < 9;j++) {
                kernel_data[i * 9 * 64 + 64 * j] = convd_kernel_rg8[i * 9 + j];
                //kernel_data[i * 9 * 64 + 64 * j] = convd_kernel_bg8[i * 9 + j];
            }
        }

        bm_image_copy_host_to_device(input_img, (void **)input);
        bmcv_image_bayer2rgb(handle, kernel_data, input_img, output_img);
        bm_image_copy_device_to_host(output_img, (void **)(&output));
        bm_image_destroy(input_img);
        bm_image_destroy(output_img);
        free(input);
        free(output);
        bm_dev_free(handle);

