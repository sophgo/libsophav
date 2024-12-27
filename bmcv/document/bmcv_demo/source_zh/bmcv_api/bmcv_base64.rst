
bmcv_base64_enc(dec)
------------------------------

| 【描述】

| base64 网络传输中常用的编码方式，利用64个常用字符来对6位二进制数编码。


| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_base64_enc(bm_handle_t handle,
            bm_device_mem_t src,
            bm_device_mem_t dst,
            unsigned long len[2])

    bm_status_t bmcv_base64_dec(bm_handle_t handle,
            bm_device_mem_t src,
            bm_device_mem_t dst,
            unsigned long len[2])


| 【参数】

.. list-table:: bmcv_ive_add 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \src
      - 输入
      - 输入字符串所在地址，类型为bm_device_mem_t。需要调用 bm_mem_from_system()将数据地址转化成转化为 bm_device_mem_t 所对应的结构。
    * - \dst
      - 输出
      - 输出字符串所在地址，类型为bm_device_mem_t。需要调用 bm_mem_from_system()将数据地址转化成转化为 bm_device_mem_t 所对应的结构。
    * - \len[2]
      - 输出
      - 进行base64编码或解码的长度，单位是字节。其中len[0]代表输入长度，需要调用者给出。而len[1]为输出长度，由api计算后给出。


| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

**代码示例：**

    .. code-block:: c

      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>
      #include <assert.h>
      #include <math.h>
      #include "bmcv_api_ext_c.h"



      int main() {
        int original_len = (rand() % 134217728) + 1; //128M
        int encoded_len = (original_len + 2) / 3 * 4;
        char* src = (char *)malloc((original_len + 3) * sizeof(char));
        char* dst = (char *)malloc((encoded_len + 3) * sizeof(char));
        for (int j = 0; j < original_len; j++){
          src[j] = (char)((rand() % 256) + 1);
        }
        bm_handle_t handle;
        int ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
          printf("Create bm handle failed. ret = %d\n", ret);
          exit(-1);
        }
        unsigned long lenth[2];
        lenth[0] = (unsigned long)original_len;

        bmcv_base64_enc(handle, bm_mem_from_system(src), bm_mem_from_system(dst), lenth);
        bmcv_base64_dec(handle, bm_mem_from_system(dst), bm_mem_from_system(src), lenth);

        bm_dev_free(handle);
        free(src);
        free(dst);
        return 0;
      }

