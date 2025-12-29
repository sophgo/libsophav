
#include "bmcv_api_ext_c.h"
#include "stdio.h"
#include "stdlib.h"
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include "bmcv_internal.h"
#include <math.h>
typedef struct {
    int loop;
    int if_use_img;
    int data_type;
    int format;
    int width;
    int height;
    int in_width_stride;
    int out_width_stride;
    char* src_name;
    char* dst_name;
    bm_handle_t handle;
} cv_width_align_thread_arg_t;

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

static int parameters_check(int height, int width, int format, int type)
{
    int error = 0;
    if (height > 4096 || width > 4096){
        printf("Unsupported size : size_max = 4096 x 4096 \n");
        error = -1;
    }
    if (format > 14) {
        printf("Unsupported image format!\n");
        error = -1;
    }
    if(type != 1 && type != 0){
        printf("parameters error : type = 1 / 0, 1 is uint8, 0 is float \n");
        error = -1;
    }
    return error;
}

#define BMCV_WIDTH_ALIGN_CMP(type, p_exp, p_got, count, cmp_expr, fmt) \
    do { \
        int ret = 0; \
        for (int j = 0; j < (count); j++) { \
            if (cmp_expr) { \
                printf("error: when idx=%d, exp=" fmt " but got=" fmt "\n", \
                       j, \
                       (type)(p_exp)[j], \
                       (type)(p_got)[j]); \
                return -1; \
            } \
        } \
        return ret; \
    } while (0)

static int bmcv_width_align_cmp_uchar(unsigned char *p_exp, unsigned char *p_got, int count) {
    BMCV_WIDTH_ALIGN_CMP(int, p_exp, p_got, count, (p_exp[j] != p_got[j]), "%d");
}

static int bmcv_width_align_cmp_float(float *p_exp, float *p_got, int count) {
    BMCV_WIDTH_ALIGN_CMP(float, p_exp, p_got, count, (fabs(p_exp[j] - p_got[j]) > 0.001f), "%f");
}


// static int bmcv_width_align_cmp(unsigned char *p_exp, unsigned char *p_got, int count) {
//     int ret = 0;
//     for (int j = 0; j < count; j++) {
//         if (p_exp[j] != p_got[j]) {
//             printf("error: when idx=%d,  exp=%d but got=%d\n",
//                    j,
//                    (int)p_exp[j],
//                    (int)p_got[j]);
//             return -1;
//         }
//     }

//     return ret;
// }
// static int bmcv_width_align_cmp(float *p_exp, float *p_got, int count) {
//     int ret = 0;
//     for (int j = 0; j < count; j++) {
//         if (p_exp[j] - p_got[j] > 0.001) {
//             printf("error: when idx=%d,  exp=%d but got=%d\n",
//                    j,
//                    (float)p_exp[j],
//                    (float)p_got[j]);
//             return -1;
//         }
//     }

//     return ret;
// }

static void memcpy_float(float *dst, float *src, size_t size) {
    memcpy(dst, src, size);
}

static void memcpy_uchar(unsigned char *dst, unsigned char *src, size_t size) {
    memcpy(dst, src, size);
}

static bm_status_t bmcv_width_align_ref(
    void *src_image,
    int data_type,
    bm_image_format_ext format,
    int image_h,
    int image_w,
    int *stride_src,
    int *stride_dst,
    void *dst_image) {

    // 定义处理函数类型
    typedef void (*CopyFunc)(void*, void*, size_t);
    CopyFunc copy_func = (data_type == 0) ?
        (CopyFunc)memcpy_float : (CopyFunc)memcpy_uchar;

    // 辅助宏定义
    #define COPY_PLANES(count, src_offsets, dst_offsets, widths) \
        for (int i = 0; i < (count); i++) { \
            void *src = (void*)((char*)src_image + (src_offsets)); \
            void *dst = (void*)((char*)dst_image + (dst_offsets)); \
            copy_func(dst, src, (widths) * data_size); \
        }

    int data_size = (data_type == 0) ? 4 : 1;

    switch (format) {
    case FORMAT_BGR_PLANAR:
    case FORMAT_RGB_PLANAR:
        COPY_PLANES(3 * image_h,
                   i * stride_src[0] * data_size,
                   i * stride_dst[0] * data_size,
                   image_w);
        break;

    case FORMAT_BGR_PACKED:
    case FORMAT_RGB_PACKED:
        COPY_PLANES(image_h,
                   i * stride_src[0] * data_size,
                   i * stride_dst[0] * data_size,
                   image_w * 3);
        break;

    case FORMAT_NV12:
        COPY_PLANES(image_h,
                   i * stride_src[0] * data_size,
                   i * stride_dst[0] * data_size,
                   image_w);
        COPY_PLANES(image_h / 2,
                   image_h * stride_src[0] + i * stride_src[1] * data_size,
                   image_h * stride_dst[0] + i * stride_dst[1] * data_size,
                   image_w);
        break;

    case FORMAT_NV16:
        COPY_PLANES(image_h,
                   i * stride_src[0] * data_size,
                   i * stride_dst[0] * data_size,
                   image_w);
        COPY_PLANES(image_h,
                   image_h * stride_src[0] + i * stride_src[1] * data_size,
                   image_h * stride_dst[0] + i * stride_dst[1] * data_size,
                   image_w);
        break;

    case FORMAT_YUV420P:
        COPY_PLANES(image_h,
                   i * stride_src[0] * data_size,
                   i * stride_dst[0] * data_size,
                   image_w);
        COPY_PLANES(image_h / 2,
                   image_h * stride_src[0] * data_size + i * stride_src[1] * data_size,
                   image_h * stride_dst[0] * data_size + i * stride_dst[1] * data_size,
                   image_w / 2);
        COPY_PLANES(image_h / 2,
                   image_h * stride_src[0] * data_size + (image_h / 2) * stride_src[1] * data_size + i * stride_src[2] * data_size,
                   image_h * stride_dst[0] * data_size + (image_h / 2) * stride_dst[1] * data_size + i * stride_dst[2] * data_size,
                   image_w / 2);
        break;

    case FORMAT_GRAY:
        COPY_PLANES(image_h,
                   i * stride_src[0] * data_size,
                   i * stride_dst[0] * data_size,
                   image_w);
        break;

    default:
        printf("Unsupported format\n");
        return BM_ERR_FAILURE;
    }

    return BM_SUCCESS;
}

static void fill_input_f32(float* input, int img_size)
{
    int i;

    for (i = 0; i < img_size; ++i) {
        input[i] = (float)rand() / (float)(RAND_MAX) * 255.0f;
    }
}

static void fill_input_u8(unsigned char* input, int img_size)
{
    int i;

    for (i = 0; i < img_size; ++i) {
        input[i] = rand() % 256;
    }
}

static void readBin(const char* path, unsigned char* input_data, int size)
{
    FILE *fp_src = fopen(path, "rb");
    if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size) {
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_src);
}

static void writeBin(const char * path, unsigned char* input_data, int size)
{
    FILE *fp_dst = fopen(path, "wb+");
    if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size) {
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_dst);
}

static int tpu_width_align(void* input, void* output, int format, int height,
                        int width, int *src_stride, int *dst_stride, int dtype, bm_handle_t handle)
{
    bm_status_t ret = BM_SUCCESS;
    int type = dtype;   // float: 0; uint: 1
    bm_image input_img, output_img;
    int src_stride_[3] = {0};
    int dst_stride_[3] = {0};
    switch (type) {
        case 0:
            for (int i = 0; i < 3; i++) {
                src_stride_[i] = src_stride[i] * 4;
                dst_stride_[i] = dst_stride[i] * 4;
            }
            break;
        default:
            for (int i = 0; i < 3; i++) {
                src_stride_[i] = src_stride[i];
                dst_stride_[i] = dst_stride[i];
            }
            break;
    }
    ret = bm_image_create(handle, height, width, (bm_image_format_ext)format, (bm_image_data_format_ext)type, &input_img, src_stride_);
    ret = bm_image_create(handle, height, width, (bm_image_format_ext)format, (bm_image_data_format_ext)type, &output_img, dst_stride_);
    ret = bm_image_alloc_dev_mem(input_img, 2);
    ret = bm_image_alloc_dev_mem(output_img, 2);

    int input_byte_size[4] = {0};
    int output_byte_size[4] = {0};
    bm_image_get_byte_size(input_img, input_byte_size);
    bm_image_get_byte_size(output_img, output_byte_size);

    void* input_addr[4] = {(void *)input,
                            (void *)((char*)input + input_byte_size[0]),
                            (void *)((char*)input + (input_byte_size[0] + input_byte_size[1])),
                            (void *)((char*)input + (input_byte_size[0] + input_byte_size[1] + input_byte_size[2]))};
    void* out_addr[4] = {(void *)output,
                            (void *)((char*)output + output_byte_size[0]),
                            (void *)((char*)output + (output_byte_size[0] + output_byte_size[1])),
                            (void *)((char*)output + (output_byte_size[0] + output_byte_size[1] + output_byte_size[2]))};

    ret = bm_image_copy_host_to_device(input_img, (void **)input_addr);
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
    ret = bmcv_width_align(handle, input_img, output_img);
    gettimeofday(&t2, NULL);
    printf("WIDTH_ALIGN TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    ret = bm_image_copy_device_to_host(output_img, (void**)out_addr);
    if (ret != BM_SUCCESS) {
        printf("ERROR: Cannot copy d2h\n");
        return -1;
    }

    bm_image_destroy(&input_img);
    bm_image_destroy(&output_img);
    return ret;
}

static int test_width_align_random(int if_use_img, int format, int data_type, int width, int height, int in_width_stride, int out_width_stride, char* src_name, char* dst_name, bm_handle_t handle) {
    int ret = 0;
    int image_w             = ALIGN(width, 2);
    int image_h             = ALIGN(height, 2);

    int raw_size            = 0;
    int src_size            = 0;
    int dst_size            = 0;

    int default_stride[3]   = {0};
    int src_stride[3]       = {0};
    int dst_stride[3]       = {0};
    switch (format) {
        case 8:
        case 9:
            // format      = FORMAT_BGR_PLANAR;
            default_stride[0] = image_w;
            src_stride[0]     = image_w + in_width_stride;
            dst_stride[0]     = image_w + out_width_stride;

            raw_size = image_h * image_w * 3;
            src_size = 3 * image_h * src_stride[0];
            dst_size = 3 * image_h * dst_stride[0];
            break;
        case 10:
        case 11:
            // image_format      = FORMAT_BGR_PACKED;
            default_stride[0] = 3 * image_w;
            src_stride[0]     = 3 * image_w + in_width_stride;
            dst_stride[0]     = 3 * image_w + out_width_stride;

            raw_size = image_h * image_w * 3;
            src_size = image_h * src_stride[0];
            dst_size = image_h * dst_stride[0];
            break;
        case 3:
            // image_format = FORMAT_NV12;
            image_w      = ALIGN(image_w, 2);
            image_h      = ALIGN(image_h, 2);

            default_stride[0] = image_w;
            src_stride[0]     = image_w + in_width_stride;
            dst_stride[0]     = image_w + out_width_stride;

            default_stride[1] = image_w;
            src_stride[1]     = image_w + in_width_stride;
            dst_stride[1]     = image_w + out_width_stride;

            raw_size = image_h * image_w * 3 / 2;
            src_size =
                image_h * src_stride[0] + image_h / 2 * src_stride[1];
            dst_size =
                image_h * dst_stride[0] + image_h / 2 * dst_stride[1];
            break;
        case 14:
            // image_format      = FORMAT_GRAY;
            default_stride[0] = image_w;
            src_stride[0]     = image_w + in_width_stride;
            dst_stride[0]     = image_w + out_width_stride;

            raw_size = image_h * image_w;
            src_size = image_h * src_stride[0];
            dst_size = image_h * dst_stride[0];
            break;
        case 5:
            // image_format = FORMAT_NV16;
            image_w      = ALIGN(image_w, 2);
            image_h      = ALIGN(image_h, 2);

            default_stride[0] = image_w;
            src_stride[0]     = image_w + in_width_stride;
            dst_stride[0]     = image_w + out_width_stride;

            default_stride[1] = image_w;
            src_stride[1]     = image_w + in_width_stride;
            dst_stride[1]     = image_w + out_width_stride;

            raw_size = image_h * image_w * 2;
            src_size = image_h * src_stride[0] + image_h * src_stride[1];
            dst_size = image_h * dst_stride[0] + image_h * dst_stride[1];
            break;
        case 0:
            // image_format = FORMAT_YUV420P;
            image_w      = ALIGN(image_w, 2);
            image_h      = ALIGN(image_h, 2);

            default_stride[0] = image_w;
            src_stride[0]     = image_w + in_width_stride;
            dst_stride[0]     = image_w + out_width_stride;

            default_stride[1] = image_w / 2;
            src_stride[1]     = image_w / 2 + in_width_stride;
            dst_stride[1]     = image_w / 2 + out_width_stride;

            default_stride[2] = image_w / 2;
            src_stride[2]     = image_w / 2 + in_width_stride;
            dst_stride[2]     = image_w / 2 + out_width_stride;

            raw_size = image_h * image_w * 3 / 2;
            src_size = image_h * src_stride[0] +
                        image_h / 2 * src_stride[1] +
                        image_h / 2 * src_stride[2];
            dst_size = image_h * dst_stride[0] +
                        image_h / 2 * dst_stride[1] +
                        image_h / 2 * dst_stride[2];
            break;
    }

    if (data_type == 0) {
        float* raw_img      = (float*)malloc(raw_size * sizeof(float));
        float* input        = (float*)malloc(src_size * sizeof(float));
        float* tpu_out      = (float*)malloc(dst_size * sizeof(float));

        if (if_use_img) {
            printf("Real img only support DTYPE_U8 \n");
            return -1;
        } else {
            printf("test random data ! \n");
            fill_input_f32(raw_img, raw_size);
        }

        bmcv_width_align_ref(raw_img, data_type, format, image_h, image_w, default_stride, src_stride, input);

        ret = tpu_width_align((void*)input, (void*)tpu_out, format, image_h, image_w, src_stride, dst_stride, data_type, handle);
        if (ret) {
            printf("the fp32 tpu_width_align is failed!\n");
            free(raw_img);
            free(input);
            free(tpu_out);
            return -1;
        }

        float* src_data = (float*)malloc(raw_size * sizeof(float));
        float* dst_data = (float*)malloc(raw_size * sizeof(float));
        bmcv_width_align_ref(input, data_type, format, image_h, image_w, src_stride, default_stride, src_data);
        bmcv_width_align_ref(tpu_out, data_type, format, image_h, image_w, dst_stride, default_stride, dst_data);
        ret = bmcv_width_align_cmp_float(src_data, dst_data, raw_size);
        if (ret != 0) {
            printf("TPU width_align failed!\n");
        }
        free(raw_img);
        free(input);
        free(tpu_out);
        free(src_data);
        free(dst_data);
    } else if (data_type == 1) {
        unsigned char* raw_img = (unsigned char*)malloc(raw_size * sizeof(unsigned char));
        unsigned char* input = (unsigned char*) malloc(src_size * sizeof(unsigned char));
        unsigned char* tpu_out = (unsigned char*) malloc(dst_size * sizeof(unsigned char));

        if (if_use_img) {
            printf("test real img \n");
            readBin(src_name, raw_img, raw_size);
        } else {
            printf("test random data ! \n");
            fill_input_u8(raw_img, raw_size);
        }

        bmcv_width_align_ref(raw_img, data_type, format, image_h, image_w, default_stride, src_stride, input);

        ret = tpu_width_align((void*)input, (void*)tpu_out, format, image_h, image_w, src_stride, dst_stride, data_type, handle);
        if (ret) {
            printf("the fp32 tpu_width_align is failed!\n");
            free(raw_img);
            free(input);
            free(tpu_out);
            return -1;
        }

        unsigned char* src_data = (unsigned char*)malloc(raw_size * sizeof(unsigned char));
        unsigned char* dst_data = (unsigned char*)malloc(raw_size * sizeof(unsigned char));
        bmcv_width_align_ref(input, data_type, format, image_h, image_w, src_stride, default_stride, src_data);

        bmcv_width_align_ref(tpu_out, data_type, format, image_h, image_w, dst_stride, default_stride, dst_data);

        ret = bmcv_width_align_cmp_uchar(src_data, dst_data, raw_size);
        if (ret != 0) {
            printf("TPU width_align failed!\n");
        }
        if(if_use_img){
            printf("output img by tpu\n");
            writeBin(dst_name, tpu_out, dst_size);
        }
        free(raw_img);
        free(input);
        free(tpu_out);
    }

    return ret;
}

void* test_width_align(void* args) {
    cv_width_align_thread_arg_t* cv_width_align_thread_arg = (cv_width_align_thread_arg_t*)args;
    int loop = cv_width_align_thread_arg->loop;
    int if_use_img = cv_width_align_thread_arg->if_use_img;
    int data_type = cv_width_align_thread_arg->data_type;
    int format = cv_width_align_thread_arg->format;
    int width = cv_width_align_thread_arg->width;
    int height = cv_width_align_thread_arg->height;
    int in_width_stride = cv_width_align_thread_arg->in_width_stride;
    int out_width_stride = cv_width_align_thread_arg->out_width_stride;
    char *src_name = cv_width_align_thread_arg->src_name;
    char *dst_name = cv_width_align_thread_arg->dst_name;
    bm_handle_t handle = cv_width_align_thread_arg->handle;
    for (int i = 0; i < loop; i++) {
        int ret = test_width_align_random(if_use_img, format, data_type, width, height, in_width_stride, out_width_stride, src_name, dst_name, handle);
        if (ret) {
            printf("------Test width_align Failed!------\n");
            exit(-1);
        }
        printf("------Test width_align Successed!------\n");
    }
    return (void*)0;
}

int main(int argc, char* args[])
{
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    int thread_num = 1;
    int loop = 1;
    int if_use_img = 0;
    int height = 1 + rand() % 4096;
    int width = 1 + rand() % 4096;
    int in_width_stride = width + rand() % 10;
    int out_width_stride = width + rand() % 10;
    int format_num[] = {0,1,2,8,9,10,11,12,13,14};
    int size = sizeof(format_num) / sizeof(format_num[0]);
    int rand_num = rand() % size;
    int format = format_num[rand_num];
    int data_type = 0;  //0: float; 1: uint;
    char* src_name = NULL;
    char* dst_name = NULL;
    int ret = 0;
    int i;
    int check = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: %d\n", argc);
        printf("%s thread_num loop if_use_img data_type format width height input_width_stride output_width_stride src_name dst_name(when use_real_img = 1,need to set src_name and dst_name) \n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 2\n", args[0]);
        printf("%s 2 1\n", args[0]);
        printf("%s 1 1 0 1 1 8 8 12 12 \n", args[0]);
        printf("%s 1 1 1 0 0 8 8 12 12 res/1920x1080_rgb out/out_add_weighted.bin \n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) if_use_img = atoi(args[3]);
    if (argc > 4) data_type = atoi(args[4]);
    if (argc > 5) format = atoi(args[5]);
    if (argc > 6) width = atoi(args[6]);
    if (argc > 7) height = atoi(args[7]);
    if (argc > 8) in_width_stride = atoi(args[8]);
    if (argc > 9) out_width_stride = atoi(args[9]);
    if (argc > 10) src_name = args[10];
    if (argc > 11) dst_name = args[11];

    check = parameters_check(height, width, format, data_type);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }

    printf("seed = %d\n", seed);

    pthread_t pid[thread_num];
    cv_width_align_thread_arg_t cv_width_align_thread_arg[thread_num];
    for (i = 0; i < thread_num; i++) {
        cv_width_align_thread_arg[i].loop = loop;
        cv_width_align_thread_arg[i].if_use_img = if_use_img;
        cv_width_align_thread_arg[i].height = height;
        cv_width_align_thread_arg[i].width = width;
        cv_width_align_thread_arg[i].in_width_stride = in_width_stride;
        cv_width_align_thread_arg[i].out_width_stride = out_width_stride;
        cv_width_align_thread_arg[i].format = format;
        cv_width_align_thread_arg[i].data_type = data_type;
        cv_width_align_thread_arg[i].src_name = src_name;
        cv_width_align_thread_arg[i].dst_name = dst_name;
        cv_width_align_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_width_align, &cv_width_align_thread_arg[i]) != 0) {
            printf("create thread failed\n");
            return -1;
        }
    }
    for (int i = 0; i < thread_num; i++) {
        ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            printf("Thread join failed\n");
            exit(-1);
        }
    }
    bm_dev_free(handle);
    return ret;
}
