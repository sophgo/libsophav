bmcv_batch_topk
------------------------------

**描述：**

计算每个 batch 中最大或最小的k个数，并返回index。

**语法：**

.. code-block:: c
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_batch_topk(
        bm_handle_t handle,
        bm_device_mem_t src_data_addr,
        bm_device_mem_t src_index_addr,
        bm_device_mem_t dst_data_addr,
        bm_device_mem_t dst_index_addr,
        bm_device_mem_t buffer_addr,
        int k,
        int batch,
        int *per_batch_cnt,
        int src_batch_stride,
        bool src_index_valid,
        bool same_batch_cnt,
        bool descending);

**参数：**

.. list-table:: bmcv_batch_topk 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用bm_dev_request获取。
    * - src_data_addr
      - 输入
      - 输入数据的地址信息。
    * - src_index_addr
      - 输入
      - 输入数据索引的地址信息，当src_index_valid为true时，设置该参数。
    * - dst_data_addr
      - 输出
      - 输出数据的地址信息。
    * - dst_index_addr
      - 输出
      - 输出数据索引的地址信息。
    * - buffer_addr
      - 输入
      - 缓冲区地址信息。
    * - k
      - 输入
      - 取每个batch中最大或最小数的数量，取值范围为1-100。
    * - batch
      - 输入
      - batch的数量，取值范围为1-20。
    * - \*per_batch_cnt
      - 输入
      - 每个batch的数据数量，取值范围为1-100000。
    * - src_batch_stride
      - 输入
      - 两个batch之间的间隔数。
    * - src_index_valid
      - 输入
      - 如果为true， 则使用src_index，否则使用自动生成的index。
    * - same_batch_cnt
      - 输入
      - 每个batch数据是否相同。
    * - descending
      - 输入
      - 对数据进行升序或者降序。

**返回值：**

该函数成功调用时, 返回 BM_SUCCESS。

**格式支持：**

该接口目前仅支持float32类型数据。

**代码示例：**

.. code-block:: c
    :linenos:
    :lineno-start: 1
    :force:

    int batch_num = 100000;
    int k = 100;
    int descending = rand() % 2;
    int batch = rand() % 20 + 1;
    int batch_stride = batch_num;
    bool bottom_index_valid = true;

    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    float* bottom_data = (float*)malloc(batch * batch_stride * sizeof(float));
    int* bottom_index = (int*)malloc(batch * batch_stride * sizeof(int));
    float* top_data = (float*)malloc(batch * batch_stride * sizeof(float));
    int* top_index = (int*)malloc(batch * batch_stride * sizeof(int));
    float* top_data_ref = (float*)malloc(batch * k * sizeof(float));
    int* top_index_ref = (int*)malloc(batch * k * sizeof(int));
    float* buffer = (float*)malloc(3 * batch_stride * sizeof(float));

    for(int i = 0; i < batch; i++){
        for(int j = 0; j < batch_num; j++){
            bottom_data[i * batch_stride + j] = rand() % 10000 * 1.0f;
            bottom_index[i * batch_stride + j] = i * batch_stride + j;
        }
    }

    bm_status_t ret = bmcv_batch_topk(
        handle,
        bm_mem_from_system((void*)bottom_data),
        bm_mem_from_system((void*)bottom_index),
        bm_mem_from_system((void*)top_data),
        bm_mem_from_system((void*)top_index),
        bm_mem_from_system((void*)buffer),
        bottom_index_valid,
        k,
        batch,
        &batch_num,
        true,
        batch_stride,
        descending);

    if(ret == BM_SUCCESS){
        int data_cmp = -1;
        int index_cmp = -1;
        data_cmp = array_cmp( (float*)top_data_ref,
                              (float*)top_data,
                              batch * k,
                              "topk data",
                              0);
        index_cmp = array_cmp( (float*)top_index_ref,
                              (float*)top_index,
                              batch * k,
                              "topk index",
                              0);
        if (data_cmp == 0 && index_cmp == 0) {
            printf("Compare success for topk data and index!\n");
        } else {
            printf("Compare failed for topk data and index!\n");
            exit(-1);
        }
    } else {
        printf("Compare failed for topk data and index!\n");
        exit(-1);
    }

    free(bottom_data);
    free(bottom_index);
    free(top_data);
    free(top_data_ref);
    free(top_index);
    free(top_index_ref);
    free(buffer);
    bm_dev_free(handle);
