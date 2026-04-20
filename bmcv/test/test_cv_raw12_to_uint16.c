#include <stdio.h>
#include "stdlib.h"
#include <sys/time.h>
#include <pthread.h>
#include <stdint.h>
#include "bmcv_api_ext_c.h"

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

static void fill_raw12(unsigned char* input, int len) {
    for(int i = 0; i < len; i++) {
        input[i] = rand() % 256;
    }
}

void raw12_to_uint16_cpu(unsigned char *input_data, uint16_t *output_cpu, int w, int h) {
    const size_t pixel_cnt = (size_t)w * (size_t)h;
    const size_t group_cnt = pixel_cnt >> 1;

    size_t s_idx = 0;
    size_t d_idx = 0;

    for (size_t g = 0; g < group_cnt; ++g) {
        uint8_t b0 = input_data[s_idx++];
        uint8_t b1 = input_data[s_idx++];
        uint8_t b2 = input_data[s_idx++];

        uint16_t p0 = ((uint16_t)b0 << 4) | ( b2 & 0x0F );
        uint16_t p1 = ((uint16_t)b1 << 4) | ( b2 >> 4 );

        output_cpu[d_idx++] = p0;
        output_cpu[d_idx++] = p1;
    }
}

typedef struct {
    int loop_num;
    int height;
    int width;
    int use_real_img;
    const char* input_path;
    const char* output_path;
    bm_handle_t handle;
} raw12_to_uint16_thread_arg_t;

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

static int raw12_to_uint16_tpu(unsigned char* input, uint16_t* output, int height, int width, bm_handle_t handle) {
    bm_device_mem_t input_dev_mem, output_dev_mem;
    struct timeval t1, t2;
    bm_malloc_device_byte(handle, &input_dev_mem, width * height / 2 * 3 * sizeof(unsigned char));
    bm_malloc_device_byte(handle, &output_dev_mem, width * height * sizeof(uint16_t));
    bm_memcpy_s2d(handle, input_dev_mem, input);
    gettimeofday(&t1, NULL);
    bm_status_t ret = bmcv_raw12_to_uint16(handle, input_dev_mem, output_dev_mem, width, height);
    if (ret != BM_SUCCESS) {
        printf("bmcv_raw12_to_uint16 API process failed\n");
        bm_free_device(handle, input_dev_mem);
        bm_free_device(handle, output_dev_mem);
        return -1;
    }
    gettimeofday(&t2, NULL);
    printf("First Raw12_to_uint16 TPU using time = %ld(us)\n",  (long)TIME_COST_US(t1, t2));
    gettimeofday(&t1, NULL);
    ret = bmcv_raw12_to_uint16(handle, input_dev_mem, output_dev_mem, width, height);
    if (ret != BM_SUCCESS) {
        printf("bmcv_raw12_to_uint16 API process failed\n");
        bm_free_device(handle, input_dev_mem);
        bm_free_device(handle, output_dev_mem);
        return -1;
    }
    gettimeofday(&t2, NULL);
    printf("Second Raw12_to_uint16 TPU using time = %ld(us)\n",  (long)TIME_COST_US(t1, t2));
    bm_memcpy_d2s(handle, output, output_dev_mem);
    bm_free_device(handle, input_dev_mem);
    bm_free_device(handle, output_dev_mem);
    return 0;
}

static int cmp(uint16_t *tpu_output, uint16_t *cpu_output, int len) {
    for (int i = 0; i < len; i++) {
        if (tpu_output[i] != cpu_output[i]) {
            printf("cmp error: idx=%d  cpu_output=%d  tpu_output=%d\n", i, cpu_output[i], tpu_output[i]);
            return -1;
        }
    }
    return 0;
}

static int test_raw12_to_uint16_random(int use_real_img, int height, int width, const char* input_path,
                                       const char* output_path, bm_handle_t handle) {
    printf("width: %d , height: %d\n", width, height);
    int ret;
    struct timeval t1, t2;

    unsigned char* input_data = (unsigned char*)malloc(width * height / 2 * 3 * sizeof(unsigned char));
    uint16_t* output_cpu = (uint16_t*)malloc(width * height * sizeof(uint16_t));
    uint16_t* output_tpu = (uint16_t*)malloc(width * height * sizeof(uint16_t));
    if (use_real_img) {
        ret = read_bin(input_path, input_data, width * height / 2 * 3);
        if(ret != 0) {
            free(input_data);
            free(output_cpu);
            free(output_tpu);
            return ret;
        }
    } else {
        fill_raw12(input_data, width * height / 2 * 3);
    }
    gettimeofday(&t1, NULL);
    raw12_to_uint16_cpu(input_data, output_cpu, height, width);
    gettimeofday(&t2, NULL);
    printf("Raw12_to_uint16 CPU using time = %ld(us)\n", (long)TIME_COST_US(t1, t2));
    ret = raw12_to_uint16_tpu(input_data, output_tpu, height, width, handle);
    if(ret != 0){
        free(input_data);
        free(output_cpu);
        free(output_tpu);
        return ret;
    }
    ret = cmp(output_tpu, output_cpu, width * height);
    if (ret == 0) {
        printf("TPU and CPU results comparison successful!\n");
        if (use_real_img) {
            ret = write_uint16_t_bin(output_path, output_tpu, width * height);
        }
    } else {
        printf("TPU and CPU results comparison failed!\n");
    }
    free(input_data);
    free(output_cpu);
    free(output_tpu);
    return ret;
}

void* test_raw12_to_uint16(void* args) {
    raw12_to_uint16_thread_arg_t* raw12_to_uint16_thread_arg = (raw12_to_uint16_thread_arg_t*)args;
    int loop_num = raw12_to_uint16_thread_arg->loop_num;
    int use_real_img = raw12_to_uint16_thread_arg->use_real_img;
    int height = raw12_to_uint16_thread_arg->height;
    int width = raw12_to_uint16_thread_arg->width;
    const char* input_path = raw12_to_uint16_thread_arg->input_path;
    const char* output_path = raw12_to_uint16_thread_arg->output_path;
    bm_handle_t handle = raw12_to_uint16_thread_arg->handle;
    for (int i = 0; i < loop_num; i++) {
        if (0 != test_raw12_to_uint16_random(use_real_img, height, width, input_path, output_path, handle)){
            printf("------TEST RAW12_TO_UINT16 FAILED------\n");
            bm_dev_free(handle);
            exit(-1);
        }
        printf("------TEST RAW12_TO_UINT16 PASSED!------\n");
    }
    return NULL;
}

int main(int argc, char* args[]) {
    struct timespec tp;
    clock_gettime(0, &tp);
    unsigned int seed = tp.tv_nsec;
    srand(seed);
    int use_real_img = 0;
    int loop = 1;
    int width = 640;
    int height = 480;
    int thread_num = 1;
    const char* input_path = NULL;
    const char* output_path = NULL;

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage:\n");
        printf("%s thread_num loop use_real_img width height input_path output_path)\n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 2\n", args[0]);
        printf("%s 2 1 0 640 480 \n", args[0]);
        printf("%s 1 1 0 640 480\n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) use_real_img = atoi(args[3]);
    if (argc > 4) width = atoi(args[4]);
    if (argc > 5) height = atoi(args[5]);
    if (argc > 6) input_path = args[6];
    if (argc > 7) output_path = args[7];

    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }
    // test for multi-thread
    pthread_t pid[thread_num];
    raw12_to_uint16_thread_arg_t raw12_to_uint16_thread_arg[thread_num];
    for (int i = 0; i < thread_num; i++) {
        raw12_to_uint16_thread_arg[i].loop_num = loop;
        raw12_to_uint16_thread_arg[i].use_real_img = use_real_img;
        raw12_to_uint16_thread_arg[i].height = height;
        raw12_to_uint16_thread_arg[i].width = width;
        raw12_to_uint16_thread_arg[i].handle = handle;
        raw12_to_uint16_thread_arg[i].input_path = input_path;
        raw12_to_uint16_thread_arg[i].output_path = output_path;
        if (pthread_create(pid + i, NULL, test_raw12_to_uint16, raw12_to_uint16_thread_arg + i) != 0) {
            printf("create thread failed\n");
            return -1;
        }
    }
    for (int i = 0; i < thread_num; i++) {
        int ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            printf("Thread join failed\n");
            exit(-1);
        }
    }
    bm_dev_free(handle);
    return ret;
}
