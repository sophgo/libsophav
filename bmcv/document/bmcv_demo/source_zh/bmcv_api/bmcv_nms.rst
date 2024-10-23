bmcv_nms
=========

该接口用于消除网络计算得到过多的物体框，并找到最佳物体框。


**接口形式:**

    .. code-block:: c

        bm_status_t bmcv_nms(bm_handle_t handle,
                    bm_device_mem_t input_proposal_addr,
                    int proposal_size,
                    float nms_threshold,
                    bm_device_mem_t output_proposal_addr);


**参数说明:**

* bm_handle_t handle

  输入参数。bm_handle 句柄。

* bm_device_mem_t input_proposal_addr

  输入参数。输入物体框数据所在地址，输入物体框数据结构为 face_rect_t，详见下面数据结构说明。需要调用 bm_mem_from_system()将数据地址转化成转化为 bm_device_mem_t 所对应的结构。

* int proposal_size

  输入参数。物体框个数。

* float nms_threshold

  输入参数。过滤物体框的阈值，分数小于该阈值的物体框将会被过滤掉。

* bm_device_mem_t output_proposal_addr

  输出参数。输出物体框数据所在地址，输出物体框数据结构为 nms_proposal_t，详见下面数据结构说明。需要调用 bm_mem_from_system() 将数据地址转化成转化为 bm_device_mem_t 所对应的结构。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他: 失败


**数据类型说明:**

face_rect_t 描述了一个物体框坐标位置以及对应的分数。

    .. code-block:: c

        struct face_rect_t {
            float x1;
            float y1;
            float x2;
            float y2;
            float score;
        };

    * x1 描述了物体框左边缘的横坐标

    * y1 描述了物体框上边缘的纵坐标

    * x2 描述了物体框右边缘的横坐标

    * y2 描述了物体框下边缘的纵坐标

    * score 描述了物体框对应的分数


nms_proposal_t 描述了输出物体框的信息。

    .. code-block:: c

        struct nms_proposal_t {
            struct face_rect_t face_rect[MAX_PROPOSAL_NUM];
            int size;
            int capacity;
            struct face_rect_t* begin;
            struct face_rect_t* end;
        };

    * face_rect 描述了经过过滤后的物体框信息

    * size 描述了过滤后得到的物体框个数

    * capacity 描述了过滤后物体框最大个数

    * begin 暂不使用

    * end 暂不使用


**示例代码:**

    .. code-block:: c

        struct face_rect_t* proposal_rand = (struct face_rect_t*)malloc(sizeof(struct face_rect_t) * MAX_PROPOSAL_NUM);
        struct nms_proposal_t* output_proposal = (struct nms_proposal_t*)malloc(struct nms_proposal_t);
        int proposal_size = 32;
        float nms_threshold = 0.2;
        int i;
        bm_status_t ret = BM_SUCCESS;
        bm_handle_t handle = NULL;

        for (i = 0; i < proposal_size; i++) {
            proposal_rand[i].x1 = 200;
            proposal_rand[i].x2 = 210;
            proposal_rand[i].y1 = 200;
            proposal_rand[i].y2 = 210;
            proposal_rand[i].score = 0.23;
        }

        ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
            printf("Create bm handle failed.\n");
            goto exit0;
        }

        ret = bmcv_nms(handle, bm_mem_from_system(proposal_rand), proposal_size, nms_threshold, \
                    bm_mem_from_system(output_proposal));
        if (ret != BM_SUCCESS) {
            printf("the bmcv_nms failed!, the ret = %d\n", ret);
            goto exit1;
        }

        exit1:
        bm_dev_free(handle);
        exit0:
        free(proposal_rand);
        free(output_proposal);
        return ret;

**注意事项:**

1. 该 api 可输入的最大 proposal 数为 50000。