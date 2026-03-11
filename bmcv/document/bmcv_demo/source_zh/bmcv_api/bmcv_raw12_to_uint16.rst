bmcv_raw12_to_uint16
======================

将RAW-12bit数据转为16bit数据。


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_raw12_to_uint16(
                bm_handle_t handle,
                bm_device_mem_t input_dev_mem,
                bm_device_mem_t output_dev_mem,
                int width,
                int height);


**参数说明：**

* bm_handle_t handle

  输入参数。bm_handle 句柄。

* bm_device_mem_t input_dev_mem

  输入参数。输入数据的设备内存地址。

* bm_device_mem_t output_dev_mem

  输入参数。输出数据的设备内存地址。

* int width

  输入参数。输入图像的宽。

* int height

  输入参数。输入图像的高。


**返回值说明：**

* BM_SUCCESS: 成功

* 其他: 失败


**注意事项：**

1. 目前输入宽高仅支持640*480。


**示例代码**

    .. code-block:: c

        #include <stdio.h>
        #include "stdlib.h"
        #include <sys/time.h>
        #include <stdint.h>
        #include "bmcv_api_ext_c.h"

        #define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

        static int read_bin(const char *input_path, unsigned char *input_data, int len){
            FILE *fp = fopen(input_path, "rb");
            if (!fp) {
                fprintf(stderr, "Unable to open read file: %s\n", input_path);
                return -1;
            }
            int bytes_read = fread(input_data, sizeof(unsigned char), len, fp);
            if (bytes_read != len) {
                fprintf(stderr, "read error: Expected %d bytes, Actual %d bytes\n", len, bytes_read);
                return -1;
            }
            fclose(fp);
            return 0;
        }

        static int write_uint16_t_bin(const char *output_path, uint16_t *output_data, int len) {
            FILE *fp_dst = fopen(output_path, "wb");
            if (fp_dst == NULL) {
                printf("Unable to open write file %s\n", output_path);
                return -1;
            }
            int bytes_write = fwrite(output_data, sizeof(uint16_t), len, fp_dst);
            if (bytes_write != len) {
                fprintf(stderr, "write error: Expected %d bytes, Actual %d bytes\n", len, bytes_write);
                return -1;
            }
            fclose(fp_dst);
            return 0;
        }

        int main(int argc, char* args[]) {
            int width = 640;
            int height = 480;
            const char* input_path = "/path/to/input.bin";
            const char* output_path = "/path/to/output.bin";

            bm_handle_t handle;
            bm_status_t ret = bm_dev_request(&handle, 0);
            if (ret != BM_SUCCESS) {
                printf("Create bm handle failed. ret = %d\n", ret);
                return -1;
            }
            unsigned char* input_data = (unsigned char*)malloc(width * height / 2 * 3 * sizeof(unsigned char));
            uint16_t* output_tpu = (uint16_t*)malloc(width * height * sizeof(uint16_t));
            ret = read_bin(input_path, input_data, width * height / 2 * 3);
            bm_device_mem_t input_dev_mem, output_dev_mem;
            struct timeval t1, t2;
            bm_malloc_device_byte(handle, &input_dev_mem, width * height / 2 * 3 * sizeof(unsigned char));
            bm_malloc_device_byte(handle, &output_dev_mem, width * height * sizeof(uint16_t));
            bm_memcpy_s2d(handle, input_dev_mem, input_data);
            gettimeofday(&t1, NULL);
            ret = bmcv_raw12_to_uint16(handle, input_dev_mem, output_dev_mem, width, height);
            if (ret != BM_SUCCESS) {
                printf("bmcv_raw12_to_uint16 API process failed\n");
                bm_free_device(handle, input_dev_mem);
                bm_free_device(handle, output_dev_mem);
                return -1;
            }
            gettimeofday(&t2, NULL);
            printf("Raw12_to_uint16 TPU using time = %ld(us)\n",  (long)TIME_COST_US(t1, t2));
            bm_memcpy_d2s(handle, output_tpu, output_dev_mem);
            write_uint16_t_bin(output_path, output_tpu, width * height * sizeof(uint16_t));
            bm_free_device(handle, input_dev_mem);
            bm_free_device(handle, output_dev_mem);
            bm_dev_free(handle);
            return ret;
        }