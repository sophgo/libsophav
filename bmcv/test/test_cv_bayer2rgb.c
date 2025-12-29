#include "bmcv_api_ext_c.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t lock;
#define NPU_NUM 32
#define KERNEL_SIZE 3 * 3 * 3 * 4 * 32
#define CONVD_MATRIX 12 * 9
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef struct {
    int loop_num;
    int height;
    int width;
    int src_type;
    bool use_real_img;
    char* input_path;
    char* output_path;
    bm_handle_t handle;
} bayer2rgb_thread_arg_t;

extern void bayer2rgb_cpu(unsigned char* dstImgCpu_r, unsigned char* dstImgCpu_g, unsigned char* dstImgCpu_b,
                   unsigned char* srcImg, int height, int width, int src_type);

void sleep_(unsigned long _t){
    #ifdef __linux__
      usleep(_t);
    #else
      Sleep(_t);
    #endif
}

static int parameters_check(int width, int height, int src_type)
{
    if (width % 2 != 0 || height % 2 != 0){
        printf("Unsupported value : Odd widths and heights are not supported \n");
        return -1;
    }
    if (src_type != 0 && src_type != 1) {
        printf("src_type only support 0(bg8) or 1(rg8) now! \n");
        return -1;
    }
    return 0;
}

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
void readBin(const char *path, unsigned char* input_data, int size) {
    FILE *fp_src = fopen(path, "rb");
    if (fp_src == NULL) {
        printf("无法打开输出文件 %s\n", path);
        return;
    }
    if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size) {
        printf("file size is less than %d required bytes\n", size);
    };
    fclose(fp_src);
}

static int bayer2rgb_tpu(bm_handle_t handle, unsigned char* input, unsigned char* output,
                         unsigned char* convd_kernel, int height, int width, int src_type) {
    struct timeval t1, t2;
    bm_image input_img;
    bm_image output_img;
    bm_status_t ret;
    pthread_mutex_lock(&lock);
    if (src_type == 0) {
        ret = bm_image_create(handle, height, width, FORMAT_BAYER, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
    } else {
        ret = bm_image_create(handle, height, width, FORMAT_BAYER_RG8, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
    }
    if(ret != BM_SUCCESS){
        return -1;
    }
    bm_image_create(handle, height, width, FORMAT_RGB_PLANAR, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);
    bm_image_alloc_dev_mem(input_img, BMCV_HEAP1_ID);
    bm_image_alloc_dev_mem(output_img, BMCV_HEAP1_ID);
    bm_image_copy_host_to_device(input_img, (void **)(&input));
    gettimeofday(&t1,NULL);
    bmcv_image_bayer2rgb(handle, convd_kernel, input_img, output_img);
    gettimeofday(&t2,NULL);
    printf("Bayer2rgb TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    bm_image_copy_device_to_host(output_img, (void **)(&output));
    bm_image_destroy(&input_img);
    bm_image_destroy(&output_img);
    pthread_mutex_unlock(&lock);
    return 0;
}

int resultCompare2(unsigned char* output_data, unsigned char* dstImgCpu_r, unsigned char* dstImgCpu_g, unsigned char* dstImgCpu_b, int width, int height) {

    for (int i = 0;i < height;i++)
        for (int j = 0;j < width;j++) {
            if (abs(dstImgCpu_r[i * width + j] - output_data[i * width + j]) > 1) {
                printf(" dstImgCpu_r , i = %d, j = %d\n", i, j);
                printf(" dstImgCpu_r = %u, output_tpu = %u\n", dstImgCpu_r[i * width + j], output_data[i * width + j]);
                return -1;
            }

            if (abs(dstImgCpu_g[i * width + j] - output_data[height * width + i * width + j]) > 1) {
                printf(" dstImgCpu_g , i = %d, j = %d\n", i, j);
                printf(" dstImgCpu_g = %u, output_tpu = %u\n", dstImgCpu_g[i * width + j], output_data[height * width + i * width + j]);
             return -1;
            }

            if (abs(dstImgCpu_b[i * width + j] - output_data[height * width * 2 + i * width + j]) > 1) {
                printf(" dstImgCpu_b , i = %d, j = %d\n", i, j);
                printf(" dstImgCpu_b = %u, output_tpu = %u\n", dstImgCpu_b[i * width + j], output_data[height * width * 2 + i * width + j]);
                return -1;
            }
        }

    return 0;
}

int test_bayer2rgb_random(bm_handle_t handle, int height, int width, bool use_real_img,
                          const char *src_name, const char *output_path, int src_type) {
    int ret = 0;
    struct timeval t1, t2;
    printf("height = %d, width = %d, src_type = %d\n", height, width, src_type);
    unsigned char* output_tpu = (unsigned char*)malloc(width * height * 3);
    unsigned char* input_data = (unsigned char*)malloc(width * height);
    unsigned char* dstImgCpu_r = (unsigned char*)malloc(width * height);
    unsigned char* dstImgCpu_g = (unsigned char*)malloc(width * height);
    unsigned char* dstImgCpu_b = (unsigned char*)malloc(width * height);
    unsigned char kernel_data[KERNEL_SIZE] = {0};

	// reconstructing kernel data
	for (int i = 0;i < 12;i++) {
        for (int j = 0;j < 9;j++) {
            if (src_type == 0) {
                kernel_data[i * 9 * NPU_NUM + NPU_NUM * j] = convd_kernel_bg8[i * 9 + j];
            } else {
                kernel_data[i * 9 * NPU_NUM + NPU_NUM * j] = convd_kernel_rg8[i * 9 + j];
            }
        }
    }
    // constructing input data
    if (use_real_img == true) {
        printf("test real image !\n");
        readBin(src_name, input_data, height * width);
    } else {
        for (int i = 0;i < height;i++) {
            for (int j = 0;j < width;j++) {
                input_data[i * width + j] = rand() % 255;
            }
        }
    }
    gettimeofday(&t1,NULL);
    bayer2rgb_cpu(dstImgCpu_r, dstImgCpu_g, dstImgCpu_b, input_data, height, width, src_type);
    gettimeofday(&t2,NULL);

    printf("Bayer2rgb CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    ret = bayer2rgb_tpu(handle, input_data, output_tpu, kernel_data, height, width, src_type);

    if (ret != 0) {
        free(output_tpu);
        free(input_data);
        free(dstImgCpu_r);
        free(dstImgCpu_g);
        free(dstImgCpu_b);
        return ret;
    }
    ret = resultCompare2(output_tpu, dstImgCpu_r, dstImgCpu_g, dstImgCpu_b, width, height);
    ret == 0 ? printf(" compare successful! \n") : printf(" compare failed! \n");
    if (use_real_img == true) {
        FILE *fp_dst = fopen(output_path, "wb");
        if (fp_dst == NULL) {
            printf("无法打开输出文件 %s\n", output_path);
            ret = -1;
        } else {
            fwrite((void *)output_tpu, 1, height * width * 3, fp_dst);
        }
        fclose(fp_dst);
    }
    free(output_tpu);
    free(input_data);
    free(dstImgCpu_r);
    free(dstImgCpu_g);
    free(dstImgCpu_b);
    return ret;
}

void* test_bayer2rgb(void* args) {
    bayer2rgb_thread_arg_t* bayer2rgb_thread_arg = (bayer2rgb_thread_arg_t*)args;
    bm_handle_t handle = bayer2rgb_thread_arg->handle;
    int loop_num = bayer2rgb_thread_arg->loop_num;
    int height = bayer2rgb_thread_arg->height;
    int width = bayer2rgb_thread_arg->width;
    int src_type = bayer2rgb_thread_arg->src_type;
    bool use_real_img = bayer2rgb_thread_arg->use_real_img;
    const char* input_path = bayer2rgb_thread_arg->input_path;
    const char* output_path = bayer2rgb_thread_arg->output_path;
    for (int i = 0; i < loop_num; i++) {
        if(loop_num > 1){
            height = 2 + rand() % 2047 * 2;
            width = 2 + rand() % 2047 * 2;
        }
        if (0 != test_bayer2rgb_random(handle, height, width, use_real_img, input_path, output_path, src_type)) {
            printf("------TEST BAYER2RGB FAILED------\n");
            exit(-1);
        }
        printf("------TEST BAYER2RGB PASSED!------\n");
    }
    return NULL;
}

int main(int argc, char* args[]) {
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    int loop = 1;
    int use_real_img = 0;
    int width = 2 + rand() % 2047 * 2;
    int height = 2 + rand() % 2047 * 2;
    int src_type = rand() % 2;
    int thread_num = 1;
    int check = 0;
    char *input_path = NULL;
    char *output_path = NULL;

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage:\n");
        printf("%s thread_num loop use_real_img width height src_type(0-bg8 1-rg8) input_path output_path\n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 2\n", args[0]);
        printf("%s 2 1\n", args[0]);
        printf("%s 1 1 0 1920 1080 0\n", args[0]);
        printf("%s 1 1 1 1024 1024 0 bayer.bin out/out_bayer2rgb.bin\n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) use_real_img = atoi(args[3]);
    if (argc > 4) width = atoi(args[4]);
    if (argc > 5) height = atoi(args[5]);
    if (argc > 6) src_type = atoi(args[6]);
    if (argc > 7) input_path = args[7];
    if (argc > 8) output_path = args[8];
    check = parameters_check(width, height, src_type);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }

    printf("seed = %d\n", seed);
    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }

    // test for multi-thread
    pthread_t pid[thread_num];
    bayer2rgb_thread_arg_t bayer2rgb_thread_arg[thread_num];
    for (int i = 0; i < thread_num; i++) {
        bayer2rgb_thread_arg[i].loop_num = loop;
        bayer2rgb_thread_arg[i].handle = handle;
        bayer2rgb_thread_arg[i].height = height;
        bayer2rgb_thread_arg[i].width = width;
        bayer2rgb_thread_arg[i].use_real_img = use_real_img;
        bayer2rgb_thread_arg[i].src_type = src_type;
        bayer2rgb_thread_arg[i].input_path = input_path;
        bayer2rgb_thread_arg[i].output_path = output_path;
        if (pthread_create(pid + i, NULL, test_bayer2rgb, bayer2rgb_thread_arg + i) != 0) {
            printf("create thread failed\n");
            bm_dev_free(handle);
            return -1;
        }
    }
    for (int i = 0; i < thread_num; i++) {
        void* ret_val;
        int ret = pthread_join(pid[i], &ret_val);
        if (ret != 0) {
            printf("Thread join failed\n");
            exit(-1);
        }
    }
    bm_dev_free(handle);
    return ret;
}
