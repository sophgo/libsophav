bmcv_matmul
============

该接口可以实现 8-bit 数据类型矩阵的乘法计算，如下公式：

  .. math:: C = (A\times B) >> rshift\_bit
    :label: eq:pfx

或者

  .. math:: C = alpha \times (A\times B) + beta
    :label: eq:pfy


其中，

* A 是输入的左矩阵，其数据类型可以是 unsigned char 或者 signed char 类型的 8-bit 数据，大小为（M，K）;

* B 是输入的右矩阵，其数据类型可以是 unsigned char 或者 signed char 类型的 8-bit 数据，大小为（K，N）;

* C 是输出的结果矩阵， 其数据类型长度可以是 int8、int16 或者 float32，用户配置决定。

   当 C 是 int8 或者 int16 时，执行上述公式 :eq:`eq:pfx` 的功能， 而其符号取决于A和B，当A和B均为无符号时C才为无符号数，否则为有符号;

   当 C 是 float32 时，执行上述公式 :eq:`eq:pfy` 的功能。

* rshift_bit 是矩阵乘积的右移数，当 C 是 int8 或者 int16 时才有效，由于矩阵的乘积有可能会超出 8-bit 或者 16-bit 的范围，所以用户可以配置一定的右移数，通过舍弃部分精度来防止溢出。

* alpha 和 beta 是 float32 的常系数，当 C 是 float32 时才有效。


**接口形式:**

    .. code-block:: c

        bm_status_t bmcv_matmul(
                    bm_handle_t handle,
                    int M,
                    int N,
                    int K,
                    bm_device_mem_t A,
                    bm_device_mem_t B,
                    bm_device_mem_t C,
                    int A_sign,
                    int B_sign,
                    int rshift_bit,
                    int result_type,
                    bool is_B_trans,
                    float alpha = 1,
                    float beta = 0);


**参数说明：**

* bm_handle_t handle

  输入参数。bm_handle 句柄

* int M

  输入参数。矩阵 A 和矩阵 C 的行数

* int N

  输入参数。矩阵 B 和矩阵 C 的列数

* int K

  输入参数。矩阵 A 的列数和矩阵 B 的行数

* bm_device_mem_t A

  输入参数。根据左矩阵 A 数据存放位置保存其 device 地址或者 host 地址。如果数据存放于 host 空间则内部会自动完成 s2d 的搬运

* bm_device_mem_t B

  输入参数。根据右矩阵 B 数据存放位置保存其 device 地址或者 host 地址。如果数据存放于 host 空间则内部会自动完成 s2d 的搬运。

* bm_device_mem_t C

  输出参数。根据矩阵 C 数据存放位置保存其 device 地址或者 host 地址。如果是 host 地址，则当beta不为0时，计算前内部会自动完成 s2d 的搬运，计算后再自动完成 d2s 的搬运。

* int A_sign

  输入参数。左矩阵A的符号，1 表示有符号，0 表示无符号。

* int B_sign

  输入参数。右矩阵B的符号，1 表示有符号，0 表示无符号。

* int rshift_bit

  输入参数。矩阵乘积的右移数，为非负数。只有当 result_type 等于 0 或者 1 时才有效。

* int result_type

  输入参数。输出的结果矩阵数据类型，0 表示是 int8，1 表示int16, 2 表示 float32。

* bool is_B_trans

  输入参数。输入右矩阵B是否需要计算前做转置。

* float alpha

  常系数，输入矩阵 A 和 B 相乘之后再乘上该系数，只有当 result_type 等于 2 时才有效，默认值为 1。

* float beta

  常系数，在输出结果矩阵 C 之前，加上该偏移量，只有当 result_type 等于 2 时才有效，默认值为 0。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他: 失败


**示例代码**

    .. code-block:: c

      #include "bmcv_api_ext_c.h"
      #include <math.h>
      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>
      #include "test_misc.h"

      static void assign_fix8b_matrix(void* mat, int size, int is_16bit)
      {
          int i;

          for (i = 0; i < size; i++) {
              if (is_16bit) {
                  *((signed short*)mat + i) = rand() % 65536 - 32768;
              } else {
                  *((signed char*)mat + i) = rand() % 256 - 128;
              }
          }
      }

      int main()
      {
          int M = 1 + rand() % 6;
          int K = 1 + rand() % 512;
          int N = 1 + rand() % 9216;
          // int trans_A = 0;  //whether to transpose
          int trans_B = 0;
          int A_sign = 0;  //unsigned or singned
          int B_sign = 0;
          int result_type = 0;  //0-int8 1-int16 2-flaot
          int right_shift_bit = 1;
          float alpha = 1;
          float beta = 0;
          int ret = 0;
          bm_handle_t handle;
          ret = bm_dev_request(&handle, 0);

          signed char* input_A;
          signed char* input_B;
          void* tpu_out;
          input_A = (signed char*)malloc(M * K * sizeof(signed char));
          input_B = (signed char*)malloc(K * N * sizeof(signed char));

          if (result_type == 0) {
              tpu_out = (signed char*)malloc(M * N * sizeof(signed char));
              memset(tpu_out, 0, M * N * sizeof(signed char));
          }

          assign_fix8b_matrix((void*)input_A, M * K, 0);
          assign_fix8b_matrix((void*)input_B, K * N, 0);

          ret = bmcv_matmul(handle, M, N, K, bm_mem_from_system((void*)input_A),
                          bm_mem_from_system((void*)input_B), bm_mem_from_system(tpu_out), A_sign,
                          B_sign, right_shift_bit, result_type, trans_B, alpha, beta);

          if (ret != BM_SUCCESS) {
              printf("Create bm handle failed. ret = %d\n", ret);
              bm_dev_free(handle);
              return -1;
          }

          free(input_A);
          free(input_B);
          free(tpu_out);

          bm_dev_free(handle);
          return ret;
      }
