#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define TDE_ALIGN 32
#define PI 3.14159265358979323846

extern int md5_cmp(unsigned char* got, unsigned char* exp ,int size);

void calculateRegularPolygon(bmcv_point_t vertices[], int n, double centerX, double centerY, double radius) {
    for (int i = 0; i < n; i++) {
        double theta = 2 * PI * i / n;
        vertices[i].x = centerX + radius * cos(theta);
        vertices[i].y = centerY + radius * sin(theta);
    }
}

static void write_bin(void *va, char *name, int size) {
    FILE *fp_dst = fopen(name, "wb");
    fwrite(va, 1, size, fp_dst);
    fclose(fp_dst);
}

static int calculate_affine_from_3_points(
    bmcv_point_t src[3],
    bmcv_point_t dst[3],
    float* mat)
{
    float u1 = src[1].x - src[0].x;
    float v1 = src[1].y - src[0].y;
    float u2 = src[2].x - src[0].x;
    float v2 = src[2].y - src[0].y;

    float u1p = dst[1].x - dst[0].x;
    float v1p = dst[1].y - dst[0].y;
    float u2p = dst[2].x - dst[0].x;
    float v2p = dst[2].y - dst[0].y;

    float det = u1 * v2 - u2 * v1;
    if (fabs(det) < 1e-10f) {
        return -1;
    }

    float inv_det = 1.0f / det;

    // [a b] = [u1p u2p] * [v2 -v1] * inv_det
    // [c d]   [v1p v2p]   [-u2 u1]
    float a = (u1p * v2 - u2p * v1) * inv_det;
    float b = (u2p * u1 - u1p * u2) * inv_det;
    float c = (v1p * v2 - v2p * v1) * inv_det;
    float d = (v2p * u1 - v1p * u2) * inv_det;

    float tx = dst[0].x - (a * src[0].x + b * src[0].y);
    float ty = dst[0].y - (c * src[0].x + d * src[0].y);

    mat[0] = a;
    mat[1] = b;
    mat[2] = tx;
    mat[3] = c;
    mat[4] = d;
    mat[5] = ty;
    return 0;
}
typedef struct quick_ctx_ {
    int i;
} quick_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int src_h = 1080, src_w = 1920, dst_w = 1920, dst_h = 1080, dev_id = 0;
bm_image_format_ext src_fmt = FORMAT_RGB_PACKED, dst_fmt = FORMAT_ARGB_PACKED;
char *src_name = "/opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin";
char *bg_name = "/opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin";
char *dst_name = "dst.bin";
bmcv_rect_t rect = {.start_x = 0, .start_y = 0, .crop_w = 1920, .crop_h = 1080};
bmcv_rect_t src_rect = {.start_x = 0, .start_y = 0, .crop_w = 1920, .crop_h = 1080};
bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR;
bmcv_color_ext color = {.a = 255, .r = 255, .g = 0, .b = 0};
int thick = 2, rot_angle = 0, global_alpha = 0;
bm_handle_t handle = NULL;
int tde_align[3] = {TDE_ALIGN, TDE_ALIGN, TDE_ALIGN};
char *md5 = "6b799a014979f602ed9d75ec4c72c79f";

static void *fill(void* arg) {
    bm_status_t ret;
    bm_image image;
    unsigned int i = 0;
    unsigned long long time_single, time_total = 0, time_avg = 0, time_max = 0;
    quick_ctx ctx = *(quick_ctx *)arg;
    struct timeval tv[2];

    bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &image, NULL);

    ret = bm_image_alloc_dev_mem(image, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dst. ret = %d\n", ret);
        exit(-1);
    }
    for(i = 0;i < test_loop_times; i++){
        gettimeofday(tv, NULL);

        bmcv_tde_fill(handle, image, color, &rect);

        gettimeofday(tv + 1, NULL);
        time_single = (unsigned int)((tv[1].tv_sec - tv[0].tv_sec) *
            1000000 + tv[1].tv_usec - tv[0].tv_usec);
        if (time_single > time_max) time_max = time_single;
        time_total = time_total + time_single;
    }
    time_avg = time_total / test_loop_times;

    if(md5 == NULL)
        bm_write_bin(image, dst_name);
    else{
        int image_byte_size[4] = {0};
        bm_image_get_byte_size(image, image_byte_size);
        int byte_size = image_byte_size[0] + image_byte_size[1] +
            image_byte_size[2] + image_byte_size[3];
        unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
        void* out_ptr[4] = {(void*)output_ptr,
            (void*)((unsigned char*)output_ptr + image_byte_size[0]),
            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] +
            image_byte_size[2])};
        bm_image_copy_device_to_host(image, (void **)out_ptr);
        if(md5_cmp(output_ptr, (unsigned char*)md5, byte_size)!=0){
            write_bin(output_ptr, "error_cmp.bin", byte_size);
            bm_image_destroy(&image);
            exit(-1);
        }
        free(output_ptr);
    }
    bm_image_destroy(&image);

    char dst_fmt_str[100];
    format_to_str(image.image_format, dst_fmt_str);

    printf("idx:%d, %d*%d, %s\n",ctx.i,dst_w,dst_h,dst_fmt_str);
    printf("idx:%d, bmcv_tde_fill:loop %d cycles, time_max = %llu, time_avg = %llu\n",
        ctx.i, test_loop_times, time_max, time_avg);

    return 0;
}

static void *line(void* arg) {
    bm_status_t ret;
    bm_image image;
    unsigned int i = 0;
    unsigned long long time_single, time_total = 0, time_avg = 0, time_max = 0;
    quick_ctx ctx = *(quick_ctx *)arg;
    struct timeval tv[2];
    bmcv_point_t start = {.x = rect.start_x, .y = rect.start_y};
    bmcv_point_t end = {.x = rect.crop_w, .y = rect.crop_h};

    bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &image, NULL);

    ret = bm_image_alloc_dev_mem(image, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dst. ret = %d\n", ret);
        exit(-1);
    }
    bm_read_bin(image, src_name);

    for(i = 0;i < test_loop_times; i++){
        gettimeofday(tv, NULL);

        bmcv_tde_line(handle, image, start, end, color, thick);

        gettimeofday(tv + 1, NULL);

        time_single = (unsigned int)((tv[1].tv_sec - tv[0].tv_sec) *
            1000000 + tv[1].tv_usec - tv[0].tv_usec);
        if (time_single > time_max) time_max = time_single;
        time_total = time_total + time_single;

        if (i == 0) {
            if(md5 == NULL)
                bm_write_bin(image, dst_name);
            else{
                int image_byte_size[4] = {0};
                bm_image_get_byte_size(image, image_byte_size);
                int byte_size = image_byte_size[0] + image_byte_size[1] +
                    image_byte_size[2] + image_byte_size[3];
                unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
                void* out_ptr[4] = {(void*)output_ptr,
                    (void*)((unsigned char*)output_ptr + image_byte_size[0]),
                    (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                    (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] +
                    image_byte_size[2])};
                bm_image_copy_device_to_host(image, (void **)out_ptr);
                if(md5_cmp(output_ptr, (unsigned char*)md5, byte_size)!=0){
                    write_bin(output_ptr, "error_cmp.bin", byte_size);
                    bm_image_destroy(&image);
                    exit(-1);
                }
                free(output_ptr);
            }
        }
    }
    time_avg = time_total / test_loop_times;

    bm_image_destroy(&image);

    char dst_fmt_str[100];
    format_to_str(image.image_format, dst_fmt_str);

    printf("idx:%d, %d*%d, %s\n",ctx.i,dst_w,dst_h,dst_fmt_str);
    printf("idx:%d, bmcv_tde_line:loop %d cycles, time_max = %llu, time_avg = %llu\n",
        ctx.i, test_loop_times, time_max, time_avg);

    return 0;
}

static void *draw(void* arg) {
    bm_status_t ret;
    bm_image image;
    unsigned int i = 0;
    unsigned long long time_single, time_total = 0, time_avg = 0, time_max = 0;
    quick_ctx ctx = *(quick_ctx *)arg;
    struct timeval tv[2];
    bmcv_point_t vertices[rect.crop_w];
    calculateRegularPolygon(vertices, rect.crop_w, rect.start_x, rect.start_y, rect.crop_h);

    bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &image, NULL);

    ret = bm_image_alloc_dev_mem(image, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dst. ret = %d\n", ret);
        exit(-1);
    }
    bm_read_bin(image, src_name);

    for(i = 0;i < test_loop_times; i++){
        gettimeofday(tv, NULL);

        bmcv_tde_draw(handle, image, vertices, rect.crop_w, color);

        gettimeofday(tv + 1, NULL);

        time_single = (unsigned int)((tv[1].tv_sec - tv[0].tv_sec) *
            1000000 + tv[1].tv_usec - tv[0].tv_usec);
        if (time_single > time_max) time_max = time_single;
        time_total = time_total + time_single;

        if (i == 0) {
            if(md5 == NULL)
                bm_write_bin(image, dst_name);
            else{
                int image_byte_size[4] = {0};
                bm_image_get_byte_size(image, image_byte_size);
                int byte_size = image_byte_size[0] + image_byte_size[1] +
                    image_byte_size[2] + image_byte_size[3];
                unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
                void* out_ptr[4] = {(void*)output_ptr,
                    (void*)((unsigned char*)output_ptr + image_byte_size[0]),
                    (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                    (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] +
                    image_byte_size[2])};
                bm_image_copy_device_to_host(image, (void **)out_ptr);
                if(md5_cmp(output_ptr, (unsigned char*)md5, byte_size)!=0){
                    write_bin(output_ptr, "error_cmp.bin", byte_size);
                    bm_image_destroy(&image);
                    exit(-1);
                }
                free(output_ptr);
            }
        }
    }
    time_avg = time_total / test_loop_times;

    bm_image_destroy(&image);

    char dst_fmt_str[100];
    format_to_str(image.image_format, dst_fmt_str);

    printf("idx:%d, %d*%d, %s\n",ctx.i,dst_w,dst_h,dst_fmt_str);
    printf("idx:%d, bmcv_tde_draw:loop %d cycles, time_max = %llu, time_avg = %llu\n",
        ctx.i, test_loop_times, time_max, time_avg);

    return 0;
}

static void *convert(void* arg) {
    bm_status_t ret;
    bm_image input, image;
    unsigned int i = 0;
    unsigned long long time_single, time_total = 0, time_avg = 0, time_max = 0;
    quick_ctx ctx = *(quick_ctx *)arg;
    struct timeval tv[2];

    bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &input, tde_align);
    bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &image, NULL);

    ret = bm_image_alloc_dev_mem(input, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dst. ret = %d\n", ret);
        exit(-1);
    }
    bm_read_compact_bin(input, src_name);

    ret = bm_image_alloc_dev_mem(image, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dst. ret = %d\n", ret);
        exit(-1);
    }

    if (rot_angle == 90 || rot_angle == 270) {
        color.a = 0;
        color.r = 0;
        color.g = 0;
        color.b = 0;
        bmcv_tde_fill(handle, image, color, NULL);
    }

    for(i = 0;i < test_loop_times; i++){
        gettimeofday(tv, NULL);

        bmcv_tde_convert(handle, input, image, rot_angle, 0, &src_rect, &rect);

        gettimeofday(tv + 1, NULL);
        time_single = (unsigned int)((tv[1].tv_sec - tv[0].tv_sec) *
            1000000 + tv[1].tv_usec - tv[0].tv_usec);
        if (time_single > time_max) time_max = time_single;
        time_total = time_total + time_single;
    }
    time_avg = time_total / test_loop_times;

    if(md5 == NULL)
        bm_write_bin(image, dst_name);
    else{
        int image_byte_size[4] = {0};
        bm_image_get_byte_size(image, image_byte_size);
        int byte_size = image_byte_size[0] + image_byte_size[1] +
            image_byte_size[2] + image_byte_size[3];
        unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
        void* out_ptr[4] = {(void*)output_ptr,
            (void*)((unsigned char*)output_ptr + image_byte_size[0]),
            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] +
            image_byte_size[2])};
        bm_image_copy_device_to_host(image, (void **)out_ptr);
        if(md5_cmp(output_ptr, (unsigned char*)md5, byte_size)!=0){
            write_bin(output_ptr, "error_cmp.bin", byte_size);
            bm_image_destroy(&image);
            bm_image_destroy(&input);
            exit(-1);
        }
        free(output_ptr);
    }
    bm_image_destroy(&image);
    bm_image_destroy(&input);

    char dst_fmt_str[100];
    char src_fmt_str[100];
    format_to_str(image.image_format, dst_fmt_str);
    format_to_str(input.image_format, src_fmt_str);

    printf("idx:%d, %d*%d->%d*%d, %s->%s\n",ctx.i,src_w,src_h,dst_w,dst_h,src_fmt_str,dst_fmt_str);
    printf("idx:%d, bmcv_tde_convert:loop %d cycles, time_max = %llu, time_avg = %llu\n",
        ctx.i, test_loop_times, time_max, time_avg);

    return 0;
}

static void *overlay(void* arg) {
    bm_status_t ret;
    bm_image input, image;
    unsigned int i = 0;
    unsigned long long time_single, time_total = 0, time_avg = 0, time_max = 0;
    quick_ctx ctx = *(quick_ctx *)arg;
    struct timeval tv[2];

    bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &input, tde_align);
    bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &image, NULL);

    ret = bm_image_alloc_dev_mem(input, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dst. ret = %d\n", ret);
        exit(-1);
    }
    bm_read_compact_bin(input, src_name);

    ret = bm_image_alloc_dev_mem(image, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dst. ret = %d\n", ret);
        exit(-1);
    }
    bm_read_bin(image, bg_name);

    for(i = 0;i < test_loop_times; i++){
        gettimeofday(tv, NULL);

        bmcv_tde_convert(handle, input, image, 0, global_alpha, NULL, &rect);

        gettimeofday(tv + 1, NULL);
        time_single = (unsigned int)((tv[1].tv_sec - tv[0].tv_sec) *
            1000000 + tv[1].tv_usec - tv[0].tv_usec);
        if (time_single > time_max) time_max = time_single;
        time_total = time_total + time_single;
    }
    time_avg = time_total / test_loop_times;

    if(md5 == NULL)
        bm_write_bin(image, dst_name);
    else{
        int image_byte_size[4] = {0};
        bm_image_get_byte_size(image, image_byte_size);
        int byte_size = image_byte_size[0] + image_byte_size[1] +
            image_byte_size[2] + image_byte_size[3];
        unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
        void* out_ptr[4] = {(void*)output_ptr,
            (void*)((unsigned char*)output_ptr + image_byte_size[0]),
            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] +
            image_byte_size[2])};
        bm_image_copy_device_to_host(image, (void **)out_ptr);
        if(md5_cmp(output_ptr, (unsigned char*)md5, byte_size)!=0){
            write_bin(output_ptr, "error_cmp.bin", byte_size);
            bm_image_destroy(&image);
            bm_image_destroy(&input);
            exit(-1);
        }
        free(output_ptr);
    }
    bm_image_destroy(&image);
    bm_image_destroy(&input);

    char dst_fmt_str[100];
    char src_fmt_str[100];
    format_to_str(image.image_format, dst_fmt_str);
    format_to_str(input.image_format, src_fmt_str);

    printf("idx:%d, %d*%d->%d*%d, %s->%s\n",ctx.i,src_w,src_h,dst_w,dst_h,src_fmt_str,dst_fmt_str);
    printf("idx:%d, bmcv_tde_convert:loop %d cycles, time_max = %llu, time_avg = %llu\n",
        ctx.i, test_loop_times, time_max, time_avg);

    return 0;
}

static void *warp(void* arg) {
    bm_status_t ret;
    bm_image input, image;
    bmcv_affine_matrix matrix;
    bmcv_point_t src_p[3], dst_p[3];
    unsigned int i = 0;
    unsigned long long time_single, time_total = 0, time_avg = 0, time_max = 0;
    quick_ctx ctx = *(quick_ctx *)arg;
    struct timeval tv[2];

    bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &input, tde_align);
    bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &image, NULL);

    ret = bm_image_alloc_dev_mem(input, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dst. ret = %d\n", ret);
        exit(-1);
    }
    bm_read_compact_bin(input, src_name);

    ret = bm_image_alloc_dev_mem(image, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dst. ret = %d\n", ret);
        exit(-1);
    }

    src_p[0].x = src_rect.start_x; src_p[1].x = src_rect.crop_w; src_p[2].x = rect.start_x;
    src_p[0].y = src_rect.start_y; src_p[1].y = src_rect.crop_h; src_p[2].y = rect.start_y;

    dst_p[0].x = 0; dst_p[1].x = dst_w; dst_p[2].x = 0;
    dst_p[0].y = 0; dst_p[1].y = 0; dst_p[2].y = dst_h;

    calculate_affine_from_3_points(src_p, dst_p, matrix.m);

    for(i = 0;i < test_loop_times; i++){
        gettimeofday(tv, NULL);
        bmcv_tde_warp_affine(handle, matrix, input, image, 1);
        gettimeofday(tv + 1, NULL);
        time_single = (unsigned int)((tv[1].tv_sec - tv[0].tv_sec) *
            1000000 + tv[1].tv_usec - tv[0].tv_usec);
        if (time_single > time_max) time_max = time_single;
        time_total = time_total + time_single;
    }
    time_avg = time_total / test_loop_times;

    if(md5 == NULL)
        bm_write_bin(image, dst_name);
    else{
        int image_byte_size[4] = {0};
        bm_image_get_byte_size(image, image_byte_size);
        int byte_size = image_byte_size[0] + image_byte_size[1] +
            image_byte_size[2] + image_byte_size[3];
        unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
        void* out_ptr[4] = {(void*)output_ptr,
            (void*)((unsigned char*)output_ptr + image_byte_size[0]),
            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] +
            image_byte_size[2])};
        bm_image_copy_device_to_host(image, (void **)out_ptr);
        if(md5_cmp(output_ptr, (unsigned char*)md5, byte_size)!=0){
            write_bin(output_ptr, "error_cmp.bin", byte_size);
            bm_image_destroy(&image);
            bm_image_destroy(&input);
            exit(-1);
        }
        free(output_ptr);
    }
    bm_image_destroy(&image);
    bm_image_destroy(&input);

    char dst_fmt_str[100];
    char src_fmt_str[100];
    format_to_str(image.image_format, dst_fmt_str);
    format_to_str(input.image_format, src_fmt_str);

    printf("idx:%d, %d*%d->%d*%d, %s->%s\n",ctx.i,src_w,src_h,dst_w,dst_h,src_fmt_str,dst_fmt_str);
    printf("idx:%d, bmcv_tde_warp_affine:loop %d cycles, time_max = %llu, time_avg = %llu\n",
        ctx.i, test_loop_times, time_max, time_avg);

    return 0;
}

static void print_help(char **argv){
    printf("please follow this order:\n \
        %s fill w h fmt stx stx rectw recth dst_name data(Hexadecimal) thread_num loop_num md5\n \
        %s convert inw inh infmt inname stx sty rectw recth outw outh outfmt outname stx sty rectw recth rot_angle thread_num loop_num md5\n \
        %s overlay inw inh infmt inname outw outh outfmt bgname stx sty rectw recth outname global_alpha thread_num loop_num md5\n \
        %s line w h fmt inname stx stx endx endy dst_name data(Hexadecimal) thick thread_num loop_num md5\n \
        %s draw w h fmt inname stx stx point_num radius dst_name data(Hexadecimal) thread_num loop_num md5\n \
        %s warp w h fmt inname outw outh outfmt out_name inx0 iny0 inx1 iny1 inx2 iny2 thread_num loop_num md5\n \
        %s thread_num loop_num\n", argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0]);
};

int main(int argc, char **argv) {
    char *endptr;
    unsigned int data = 0xffff0000;// red
    char *cmd = "convert";
    unsigned short cmd_num = 255;

    if (argc >= 2) cmd = argv[1];
    if (strncmp(cmd, "fill", 4) == 0 && argc >= 11) {
        dst_w = atoi(argv[2]);
        dst_h = atoi(argv[3]);
        dst_fmt = (bm_image_format_ext)atoi(argv[4]);
        rect.start_x = atoi(argv[5]);
        rect.start_y = atoi(argv[6]);
        rect.crop_w = atoi(argv[7]);
        rect.crop_h = atoi(argv[8]);
        dst_name = argv[9];
        data = strtol(argv[10], &endptr, 16);
        cmd_num = 11;
    } else if (strncmp(cmd, "convert", 4) == 0 && argc >= 19) {
        src_w = atoi(argv[2]);
        src_h = atoi(argv[3]);
        src_fmt = (bm_image_format_ext)atoi(argv[4]);
        src_name = argv[5];
        src_rect.start_x = atoi(argv[6]);
        src_rect.start_y = atoi(argv[7]);
        src_rect.crop_w = atoi(argv[8]);
        src_rect.crop_h = atoi(argv[9]);
        dst_w = atoi(argv[10]);
        dst_h = atoi(argv[11]);
        dst_fmt = (bm_image_format_ext)atoi(argv[12]);
        dst_name = argv[13];
        rect.start_x = atoi(argv[14]);
        rect.start_y = atoi(argv[15]);
        rect.crop_w = atoi(argv[16]);
        rect.crop_h = atoi(argv[17]);
        rot_angle = atoi(argv[18]);
        cmd_num = 19;
    } else if (strncmp(cmd, "overlay", 4) == 0 && argc >= 16) {
        src_w = atoi(argv[2]);
        src_h = atoi(argv[3]);
        src_fmt = (bm_image_format_ext)atoi(argv[4]);
        src_name = argv[5];
        dst_w = atoi(argv[6]);
        dst_h = atoi(argv[7]);
        dst_fmt = (bm_image_format_ext)atoi(argv[8]);
        bg_name = argv[9];
        rect.start_x = atoi(argv[10]);
        rect.start_y = atoi(argv[11]);
        rect.crop_w = atoi(argv[12]);
        rect.crop_h = atoi(argv[13]);
        dst_name = argv[14];
        global_alpha = atoi(argv[15]);
        cmd_num = 16;
    } else if (strncmp(cmd, "line", 4) == 0 && argc >= 13) {
        dst_w = atoi(argv[2]);
        dst_h = atoi(argv[3]);
        dst_fmt = (bm_image_format_ext)atoi(argv[4]);
        src_name = argv[5];
        rect.start_x = atoi(argv[6]);
        rect.start_y = atoi(argv[7]);
        rect.crop_w = atoi(argv[8]);// endx
        rect.crop_h = atoi(argv[9]);// endy
        dst_name = argv[10];
        data = strtol(argv[11], &endptr, 16);
        thick = atoi(argv[12]);
        cmd_num = 13;
    } else if (strncmp(cmd, "warp", 4) == 0 && argc >= 16) {
        src_w = atoi(argv[2]);
        src_h = atoi(argv[3]);
        src_fmt = (bm_image_format_ext)atoi(argv[4]);
        src_name = argv[5];
        dst_w = atoi(argv[6]);
        dst_h = atoi(argv[7]);
        dst_fmt = (bm_image_format_ext)atoi(argv[8]);
        dst_name = argv[9];
        src_rect.start_x = atoi(argv[10]);// x0
        src_rect.start_y = atoi(argv[11]);// y0
        src_rect.crop_w = atoi(argv[12]);// x1
        src_rect.crop_h = atoi(argv[13]);// y1
        rect.start_x = atoi(argv[14]);// x2
        rect.start_y = atoi(argv[15]);// y2
        cmd_num = 16;
    } else if (strncmp(cmd, "draw", 4) == 0 && argc >= 12) {
        dst_w = atoi(argv[2]);
        dst_h = atoi(argv[3]);
        dst_fmt = (bm_image_format_ext)atoi(argv[4]);
        src_name = argv[5];
        rect.start_x = atoi(argv[6]);
        rect.start_y = atoi(argv[7]);
        rect.crop_w = atoi(argv[8]);// point_num
        rect.crop_h = atoi(argv[9]);// radius
        dst_name = argv[10];
        data = strtol(argv[11], &endptr, 16);
        cmd_num = 12;
    } else if (argc == 2) {
        cmd = "convert";
        if (atoi(argv[1]) < 0) {
            print_help(argv);
            exit(-1);
        } else {
            test_threads_num  = atoi(argv[1]);
        }
    } else if (argc == 3){
        cmd = "convert";
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if (argc > 3) {
        printf("command input error\n");
        print_help(argv);
        exit(-1);
    }

    if (argc >= cmd_num+3) {
        md5 = argv[cmd_num+2];
    } else if (argc > 4){
        md5 = NULL;
    }
    if (argc >= cmd_num+2) {
        test_threads_num = atoi(argv[cmd_num]);
        test_loop_times  = atoi(argv[cmd_num+1]);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    pthread_t pid[test_threads_num];
    quick_ctx ctx[test_threads_num];
    if (strncmp(cmd, "fill", 4) == 0) {
        color.a = (data >> 24) & 0xff;
        color.r = (data >> 16) & 0xff;
        color.g = (data >> 8) & 0xff;
        color.b = data & 0xff;
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            if (pthread_create(
                    &pid[i], NULL, fill, (void *)(ctx + i))) {
                perror("create thread failed\n");
                exit(-1);
            }
        }
    }
    if (strncmp(cmd, "convert", 4) == 0) {
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            if (pthread_create(
                    &pid[i], NULL, convert, (void *)(ctx + i))) {
                perror("create thread failed\n");
                exit(-1);
            }
        }
    }
    if (strncmp(cmd, "overlay", 4) == 0) {
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            if (pthread_create(
                    &pid[i], NULL, overlay, (void *)(ctx + i))) {
                perror("create thread failed\n");
                exit(-1);
            }
        }
    }
    if (strncmp(cmd, "line", 4) == 0) {
        color.a = (data >> 24) & 0xff;
        color.r = (data >> 16) & 0xff;
        color.g = (data >> 8) & 0xff;
        color.b = data & 0xff;
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            if (pthread_create(
                    &pid[i], NULL, line, (void *)(ctx + i))) {
                perror("create thread failed\n");
                exit(-1);
            }
        }
    }
    if (strncmp(cmd, "draw", 4) == 0) {
        color.a = (data >> 24) & 0xff;
        color.r = (data >> 16) & 0xff;
        color.g = (data >> 8) & 0xff;
        color.b = data & 0xff;
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            if (pthread_create(
                    &pid[i], NULL, draw, (void *)(ctx + i))) {
                perror("create thread failed\n");
                exit(-1);
            }
        }
    }
    if (strncmp(cmd, "warp", 4) == 0) {
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            if (pthread_create(
                    &pid[i], NULL, warp, (void *)(ctx + i))) {
                perror("create thread failed\n");
                exit(-1);
            }
        }
    }
    for (int i = 0; i < test_threads_num; i++) {
        ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            perror("Thread join failed");
            exit(-1);
        }
    }
    bm_dev_free(handle);
    printf("--------ALL THREADS TEST OVER---------\n");

    return 0;
}

