bmcv_sort
==========

该接口可以实现浮点数据的排序（升序/降序），并且支持排序后可以得到原数据所对应的索引值。


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_sort(
                    bm_handle_t handle,
                    bm_device_mem_t src_index_addr,
                    bm_device_mem_t src_data_addr,
                    bm_device_mem_t dst_index_addr,
                    bm_device_mem_t dst_data_addr,
                    int data_cnt,
                    int sort_cnt,
                    int order,
                    bool index_enable,
                    bool auto_index);


**参数说明：**

* bm_handle_t handle

  输入参数。bm_handle_t handle 设备环境句柄，通过调用bm_dev_request获取。

* bm_device_mem_t src_index_addr

  输入参数。每个输入数据所对应index的地址。如果使能index_enable并且不使用auto_index时，则该参数有效。bm_device_mem_t为内置表示地址的数据类型，可以使用函数bm_mem_from_system(addr)将用户使用的指针或地址转为该类型，用户可参考示例代码中的使用方式。

* bm_device_mem_t src_data_addr

  输入参数。待排序的输入数据所对应的地址。bm_device_mem_t为内置表示地址的数据类型，可以使用函数bm_mem_from_system(addr)将用户使用的指针或地址转为该类型，用户可参考示例代码中的使用方式。

* bm_device_mem_t dst_index_addr

  输出参数。排序后输出数据所对应index的地址，如果使能index_enable并且不使用auto_index 时，则该参数有效。bm_device_mem_t为内置表示地址的数据类型，可以使用函数bm_mem_from_system(addr)将用户使用的指针或地址转为该类型，用户可参考示例代码中的使用方式。

* bm_device_mem_t dst_data_addr

  输出参数。排序后的输出数据所对应的地址。bm_device_mem_t为内置表示地址的数据类型，可以使用函数bm_mem_from_system(addr)将用户使用的指针或地址转为该类型，用户可参考示例代码中的使用方式。

* int data_cnt

  输入参数。待排序的输入数据的数量。

* int sort_cnt

  输入参数。需要排序的数量，也就是输出结果的个数，包括排好序的数据和对应index。比如降序排列，如果只需要输出最大的前三个数据，该参数设置为3即可。

* int order

  输入参数。升序还是降序，0表示升序，1表示降序。

* bool index_enable

  输入参数。是否使能index。如果使能即可输出排序后数据所对应的index，否则src_index_addr和dst_index_addr这两个参数无效。

* bool auto_index

  输入参数。是否使能自动生成index功能。使用该功能的前提是index_enable参数为true，如果该参数也为true则表示按照输入数据的存储顺序从0开始计数作为index，参数src_index_addr便无效，输出结果中排好序数据所对应的index即存放于dst_index_addr地址中。


**返回值说明：**

* BM_SUCCESS: 成功

* 其他: 失败


**注意事项：**

1. 要求sort_cnt <= data_cnt，且 data_cnt 最大值为 100000。

2. 若需要使用auto index功能，前提是参数 index_enable 为 true。


**示例代码**

    .. code-block:: c

        int data_cnt = 100;
        int sort_cnt = 50;
        float src_data_p[100];
        int src_index_p[100];
        float dst_data_p[50];
        int dst_index_p[50];
        int order = 0;
        bm_handle_t handle;
        bm_status_t ret = BM_SUCCESS;

        struct timespec tp;
        clock_gettime(NULL, &tp);
        srand(tp.tv_nsec);

        for (int i = 0; i < 100; i++) {
            src_data_p[i] = rand() % 1000;
            src_index_p[i] = 100 - i;
        }

        ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
            printf("bm_dev_request failed. ret = %d\n", ret);
            exit(-1);
        }

        ret = bmcv_sort(handle, bm_mem_from_system(src_index_p), bm_mem_from_system(src_data_p), data_cnt,
                        bm_mem_from_system(dst_index_p), bm_mem_from_system(dst_data_p),
                        sort_cnt, order, true, false);
        if (ret != BM_SUCCESS) {
            printf("bmcv_sort failed. ret = %d\n", ret);
            exit(-1);
        }

        bm_dev_free(handle);