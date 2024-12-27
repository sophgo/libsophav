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

      #include <stdint.h>
      #include <stdlib.h>
      #include <stdio.h>
      #include "bmcv_api_ext_c.h"


      #define MAX_SORT_NUM (500000)

      typedef float bm_sort_data_type_t;
      typedef enum { ASCEND_ORDER, DESCEND_ORDER } cdma_sort_order_e;

      typedef struct {
          int   index;
          float val;
      } __attribute__((packed)) sort_t;

      int32_t main() {
          int dev_id = 0;
          int data_num = 1 + rand() % 500000;
          int sort_num = 1 + rand() % data_num;
          int ret = 0;
          bm_handle_t handle;
          cdma_sort_order_e order1 = DESCEND_ORDER;
          // cdma_sort_order_e order2 = ASCEND_ORDER;
          ret = bm_dev_request(&handle, dev_id);

          bm_sort_data_type_t *src_data = (bm_sort_data_type_t*)malloc(data_num * sizeof(float));
          int *src_index_p = (int*)malloc(data_num * sizeof(int));
          sort_t *ref_res = (sort_t*)malloc(data_num * sizeof(sort_t));
          sort_t *cdma_res = (sort_t*)malloc(sort_num * sizeof(sort_t));
          bm_sort_data_type_t *dst_data = (bm_sort_data_type_t*)malloc(sort_num * sizeof(bm_sort_data_type_t));
          int *dst_data_index = (int*)malloc(sort_num * sizeof(int));
          bool index_enable = rand() % 2 ? true : false;
          bool auto_index = rand() % 2 ? true : false;
          printf("data num: %d, sort num: %d\n", data_num, sort_num);

          // produce src data and index
          for (int32_t i = 0; i < data_num; i++) {
              if(auto_index){
                src_index_p[i] = i;
              }else{
                src_index_p[i] = rand() % MAX_SORT_NUM;
              }
              ref_res[i].index = src_index_p[i];
              ref_res[i].val = ((float)(rand() % MAX_SORT_NUM)) / 100;
              src_data[i] = ref_res[i].val;
          }

          int                 *dst_index_p = NULL;
          bm_sort_data_type_t *dst_data_p  = NULL;
          dst_index_p = (int*)malloc(sort_num * sizeof(int));
          dst_data_p = (bm_sort_data_type_t*)malloc(sort_num * sizeof(int));

          bmcv_sort(handle, bm_mem_from_system(src_index_p), bm_mem_from_system(src_data), data_num,
                    bm_mem_from_system(dst_index_p), bm_mem_from_system(dst_data_p), sort_num, (int)order1,
                    index_enable, auto_index);

          for (int i = 0; i < sort_num; i++) {
              cdma_res[i].index = dst_index_p[i];
              cdma_res[i].val   = dst_data_p[i];
          }
          free(dst_index_p);
          free(dst_data_p);

          // release memory
          free(src_data);
          free(src_index_p);
          free(ref_res);
          free(cdma_res);
          free(dst_data);
          free(dst_data_index);

          bm_dev_free(handle);
          return ret;
      }