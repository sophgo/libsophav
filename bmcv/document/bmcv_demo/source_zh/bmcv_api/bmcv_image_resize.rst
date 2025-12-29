bmcv_image_resize
=================


该接口用于实现图像尺寸的变化,如放大、缩小、抠图等功能。


**接口形式:**

    .. code-block:: c

        bm_status_t bmcv_image_resize(
                bm_handle_t handle,
                int input_num,
                bmcv_resize_image resize_attr[4],
                bm_image* input,
                bm_image* output
        );


**参数说明:**

* bm_handle_t handle

输入参数。bm_handle 句柄。

* int input_num

输入参数。输入图片数，最多支持 4。

* bmcv_resize_image resize_attr [4]

输入参数。每张图片对应的 resize 参数,最多支持 4 张图片。

* bm_image\* input

输入参数。输入 bm_image。每个 bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以使用 bm_image_alloc_dev_mem 或者 bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。

* bm_image\* output

输出参数。输出 bm_image。每个 bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以通过 bm_image_alloc_dev_mem 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存，如果不主动分配将在 api 内部进行自行分配。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他:失败


**数据类型说明:**


    .. code-block:: c

        typedef struct bmcv_resize_s{
                int start_x;
                int start_y;
                int in_width;
                int in_height;
                int out_width;
                int out_height;
        }bmcv_resize_t;

        typedef struct bmcv_resize_image_s{
                bmcv_resize_t *resize_img_attr;
                int roi_num;
                unsigned char stretch_fit;
                unsigned char padding_b;
                unsigned char padding_g;
                unsigned char padding_r;
                unsigned int interpolation;
        }bmcv_resize_image;


* bmcv_resize_image 描述了一张图中 resize 配置信息。

* roi_num 描述了一副图中需要进行 resize 的子图总个数。

* stretch_fit 表示是否按照原图比例对图片进行缩放，1 表示无需按照原图比例进行缩放，0 表示按照原图比例进行缩放，当采用这种方式的时候，结果图片中为进行缩放的地方将会被填充成特定值。

* padding_b 表示当 stretch_fit 设成 0 的情况下，b 通道上被填充的值。

* padding_r 表示当 stretch_fit 设成 0 的情况下，r 通道上被填充的值。

* padding_g 表示当 stretch_fit 设成 0 的情况下，g 通道上被填充的值。

* interpolation 表示缩图所使用的算法。BMCV_INTER_NEAREST 表示最近邻算法，BMCV_INTER_LINEAR 表示线性插值算法。

* start_x 描述了 resize 起始横坐标(相对于原图)，常用于抠图功能。

* start_y 描述了 resize 起始纵坐标(相对于原图)，常用于抠图功能。

* in_width 描述了crop图像的宽。

* in_height 描述了crop图像的高。

* out_width 描述了输出图像的宽。

* out_height 描述了输出图像的高。


**注意事项:**

该接口的注意事项与 bmcv_image_vpp_basic 接口相同。
