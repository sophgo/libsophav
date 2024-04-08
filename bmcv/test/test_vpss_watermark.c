#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bmcv_api_ext_c.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned char __u8;
typedef unsigned int __u32;
typedef unsigned long __u64;
typedef __signed__ int __s32;

struct cvi_rect {
    __s32 left;
    __s32 top;
    __u32 width;
    __u32 height;
};
struct cvi_rgn_coverex_param {
    struct cvi_rect rect;
    __u32 color;
    __u8 enable;
};
enum cvi_rgn_format {
    CVI_RGN_FMT_ARGB8888,
    CVI_RGN_FMT_ARGB4444,
    CVI_RGN_FMT_ARGB1555,
    CVI_RGN_FMT_256LUT,
    CVI_RGN_FMT_16LUT,
    CVI_RGN_FMT_FONT,
    CVI_RGN_FMT_MAX
};

struct cvi_rgn_param {
    enum cvi_rgn_format fmt;
    struct cvi_rect rect;
    __u32 stride;
    __u64 phy_addr;
};

typedef struct border {
    bool rect_border_enable;
    u16 st_x;
    u16 st_y;
    u16 width;
    u16 height;
    u8 value_r;
    u8 value_g;
    u8 value_b;
    u8 thickness;
} border_t;

typedef struct coverex_cfg {
    struct cvi_rgn_coverex_param coverex_param[4];
    u8 cover_num;
} coverex_cfg;

typedef struct bm_rgn_cfg {
    struct cvi_rgn_param *param;
    u8 rgn_num;
} bmcv_rgn_cfg;

typedef struct bmcv_border {
    u8 border_num;
    border_t border_cfg[4];
} bmcv_border;

extern bm_status_t open_water(bm_handle_t handle, char *src_name, int src_size, bm_device_mem_t *dst);
extern bm_status_t bm_vpss_cmodel(bm_image *input,
                                  bm_image *output,
                                  bmcv_rect_t *input_rects,
                                  bmcv_padding_attr_t *padding_attr,
                                  bmcv_resize_algorithm algorithm,
                                  csc_type_t csc_type,
                                  csc_matrix_t *matrix,
                                  bmcv_convert_to_attr *convert_to_attr,
                                  bmcv_border *border_param,
                                  coverex_cfg *coverex_param,
                                  bmcv_rgn_cfg *gop_attr);

static void fill(unsigned char *input, int img_size) {
    for (int i = 0; i < img_size; i++) {
        input[i] = rand() % 256;
    }
}

int get_image_data_size(int width, int height, bm_image_format_ext format) {
    int size;
    switch (format) {
        case FORMAT_YUV420P:
        case FORMAT_NV12:
        case FORMAT_NV21:
            size = width * height * 3 / 2;
            break;
        case FORMAT_YUV422P:
        case FORMAT_YUV422_YUYV:
        case FORMAT_YUV422_YVYU:
        case FORMAT_YUV422_UYVY:
        case FORMAT_YUV422_VYUY:
        case FORMAT_NV16:
        case FORMAT_NV61:
            size = width * height * 2;
            break;
        case FORMAT_YUV444P:
        case FORMAT_RGB_PLANAR:
        case FORMAT_BGR_PLANAR:
        case FORMAT_RGB_PACKED:
        case FORMAT_BGR_PACKED:
            size = width * height * 3;
            break;
        case FORMAT_GRAY:
            size = width * height;
            break;
        default:
            printf("Unknown format! \n");
            size = width * height * 3;  // Default case, adjust as needed
            break;
    }
    return size;
}

bm_status_t bitmap_to_argb8888(
    unsigned char *bitmap,
    unsigned char *overlay_image,
    int water_width,
    int water_height,
    bmcv_color_t color) {
    // Loop through each pixel in the watermarked image
    for (int y = 0; y < water_height; ++y) {
        for (int x = 0; x < water_width; ++x) {
            int byteIndex = y * water_width + x;

            // alpha:0~255 --> 0~128
            unsigned char alpha = (bitmap[byteIndex] + 1) / 2;

            // Generate the ARGB8888 pixel
            unsigned char red = color.r;
            unsigned char green = color.g;
            unsigned char blue = color.b;

            // Calculate the index of the current pixel in the ARGB8888 array
            int pixelIndex = (y * water_width + x) * 4;  // Each pixel is 4 bytes

            // Store the ARGB values in the overlay_image array
            overlay_image[pixelIndex] = red;
            overlay_image[pixelIndex + 1] = green;
            overlay_image[pixelIndex + 2] = blue;
            overlay_image[pixelIndex + 3] = alpha;
        }
    }
    return BM_SUCCESS;
}

bm_status_t vpss_watermark_superpose(unsigned char *src_data,
                                     char *filename_water,
                                     int water_size,
                                     int in_width,
                                     int in_height,
                                     int water_width,
                                     bm_image_format_ext src_format,
                                     bmcv_rect_t *rects,
                                     bmcv_color_t color) {
    bm_handle_t handle = NULL;
    int dev_id = 0;
    bm_status_t ret = BM_SUCCESS;
    bm_image src;
    bm_device_mem_t water;

    ret = bm_dev_request(&handle, dev_id);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return ret;
    }

    open_water(handle, filename_water, water_size, &water);

    bm_image_create(handle, in_height, in_width, src_format, DATA_TYPE_EXT_1N_BYTE, &src, NULL);

    bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);

    int src_image_byte_size[4] = {0};
    bm_image_get_byte_size(src, src_image_byte_size);

    void *src_in_ptr[4] = {(void *)src_data,
                           (void *)((char *)src_data + src_image_byte_size[0]),
                           (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1]),
                           (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1] + src_image_byte_size[2])};
    bm_image_copy_host_to_device(src, (void **)src_in_ptr);

    ret = bmcv_image_watermark_superpose(handle, &src, &water, 1, 0, water_width, rects, color);
    if (ret != BM_SUCCESS) {
        printf("bm_vpss_watermark_superpose failed! \n");
    }

    bm_image_copy_device_to_host(src, (void **)src_in_ptr);

    bm_image_destroy(&src);
    bm_dev_free(handle);
    return ret;
}

bm_status_t cmodel_watermark_superpose(unsigned char *src_data,
                                       unsigned char *water_data,
                                       unsigned char *dst_data,
                                       int in_width,
                                       int in_height,
                                       int water_width,
                                       int water_height,
                                       bm_image_format_ext src_format,
                                       int rect_num,
                                       bmcv_rect_t *rects,
                                       bmcv_color_t color) {
    bm_handle_t handle = NULL;
    int dev_id = 0;
    bm_status_t ret = BM_SUCCESS;
    bm_image src, dst;
    bm_device_mem_t src_cmodel_mem[4];
    bm_device_mem_t dst_cmodel_mem[4];
    unsigned char *overlay_image = (unsigned char *)malloc(water_width * water_height * 4);

    ret = bm_dev_request(&handle, dev_id);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return ret;
    }

    bm_image_create(handle, in_height, in_width, src_format, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    bm_image_create(handle, in_height, in_width, src_format, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

    int src_image_byte_size[4] = {0};
    bm_image_get_byte_size(src, src_image_byte_size);

    void *src_in_ptr[4] = {(void *)src_data,
                           (void *)((char *)src_data + src_image_byte_size[0]),
                           (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1]),
                           (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1] + src_image_byte_size[2])};
    int src_plane_num = bm_image_get_plane_num(src);
    for (int i = 0; i < src_plane_num; i++) {
        src_cmodel_mem[i].u.device.device_addr = (unsigned long)src_in_ptr[i];
        src_cmodel_mem[i].size = src_image_byte_size[i];
        src_cmodel_mem[i].flags.u.mem_type = BM_MEM_TYPE_DEVICE;
    }

    bm_image_attach(src, src_cmodel_mem);

    int dst_image_byte_size[4] = {0};
    bm_image_get_byte_size(dst, dst_image_byte_size);

    void *dst_in_ptr[4] = {(void *)dst_data,
                           (void *)((char *)dst_data + dst_image_byte_size[0]),
                           (void *)((char *)dst_data + dst_image_byte_size[0] + dst_image_byte_size[1]),
                           (void *)((char *)dst_data + dst_image_byte_size[0] + dst_image_byte_size[1] + dst_image_byte_size[2])};
    int dst_plane_num = bm_image_get_plane_num(dst);
    for (int i = 0; i < dst_plane_num; i++) {
        dst_cmodel_mem[i].u.device.device_addr = (unsigned long)dst_in_ptr[i];
        dst_cmodel_mem[i].size = dst_image_byte_size[i];
        dst_cmodel_mem[i].flags.u.mem_type = BM_MEM_TYPE_DEVICE;
    }

    bm_image_attach(dst, dst_cmodel_mem);

    bitmap_to_argb8888(water_data, overlay_image, water_width, water_height, color);

    bmcv_rect_t crop_attr = {
        .start_x = 0,
        .start_y = 0,
        .crop_w = in_width,
        .crop_h = in_height};
    bmcv_convert_to_attr convert_to_attr = {
        .alpha_0 = 1,
        .beta_0 = 0,
        .alpha_1 = 1,
        .beta_1 = 0,
        .alpha_2 = 1,
        .beta_2 = 0,
    };
    bmcv_border my_border = {
        .border_num = 0,
        .border_cfg = {}};
    coverex_cfg my_coverex = {
        .coverex_param = {},
        .cover_num = 0};
    struct cvi_rgn_param my_rgn_param = {
        .fmt = CVI_RGN_FMT_ARGB8888,
        .rect = {
            .left = rects->start_x,
            .top = rects->start_y,
            .width = rects->crop_w,
            .height = rects->crop_h},
        .stride = water_width * 4,
        .phy_addr = (unsigned long)overlay_image};
    bmcv_rgn_cfg my_rgn_config = {
        .param = &my_rgn_param,
        .rgn_num = rect_num};

    ret = bm_vpss_cmodel(&src, &dst, &crop_attr, NULL, BMCV_INTER_LINEAR, CSC_YCbCr2RGB_BT601, NULL, &convert_to_attr, &my_border, &my_coverex, &my_rgn_config);

    bm_image_destroy(&src);
    bm_image_destroy(&dst);
    free(overlay_image);
    bm_dev_free(handle);
    return ret;
}

bm_status_t compare(unsigned char *vpss_output,
                    unsigned char *cmodel_output,
                    int u8size) {
    bm_status_t ret = BM_SUCCESS;
    int max_diff = INT_MIN;
    int min_diff = INT_MAX;
    int error_count = 0;
    int no_error_count = 0;
    int tolerable_error_count = 0;

    for (int i = 0; i < u8size; i++) {
        int diff = vpss_output[i] - cmodel_output[i];

        if (diff > max_diff) {
            max_diff = diff;
        }

        if (diff < min_diff) {
            min_diff = diff;
        }

        if (diff != 0) {
            if (abs(diff) <= 2) {
                tolerable_error_count++;
            } else {
                ret = BM_ERR_FAILURE;
                error_count++;
            }
        } else {
            no_error_count++;
        }
    }

    printf("Maximum pixel difference: %d\n", max_diff);
    printf("Minimum pixel difference: %d\n", min_diff);
    printf("Number of pixels with intolerable difference (>2): %d\n", error_count);
    printf("Number of pixels with tolerable difference (<= 2): %d\n", tolerable_error_count);
    printf("Number of pixels without difference: %d\n", no_error_count);

    return ret;
}

bm_status_t vpss_watermark_superpose_cmp(int use_real_img,
                                         char *filename_src,
                                         char *filename_water,
                                         char *filename_dst,
                                         int in_width,
                                         int in_height,
                                         int water_width,
                                         int water_height,
                                         bm_image_format_ext src_format,
                                         int rect_num,
                                         bmcv_rect_t *rects,
                                         bmcv_color_t color) {
    bm_status_t ret = BM_SUCCESS;

    int src_size = get_image_data_size(in_width, in_height, src_format);
    int water_size = water_width * water_height;
    unsigned char *input_data = (unsigned char *)malloc(src_size);
    unsigned char *water_data = (unsigned char *)malloc(water_size);
    unsigned char *vpss_output = (unsigned char *)malloc(src_size);
    unsigned char *cmodel_output = (unsigned char *)malloc(src_size);
    if (!input_data || !vpss_output || !cmodel_output || !water_data) {
        printf("Memory Allocation Failed \n");
        ret = BM_ERR_FAILURE;
        goto fail;
    }

    FILE *file;
    file = fopen(filename_water, "rb");
    fread(water_data, sizeof(unsigned char), water_size, file);
    fclose(file);

    if (use_real_img) {
        // read input.bin
        file = fopen(filename_src, "rb");
        fread(input_data, sizeof(unsigned char), src_size, file);
        fclose(file);
    } else {
        fill(input_data, src_size);
    }

    memcpy(vpss_output, input_data, src_size);

    vpss_watermark_superpose(vpss_output, filename_water, water_size, in_width, in_height, water_width, src_format, rects, color);

    cmodel_watermark_superpose(input_data, water_data, cmodel_output, in_width, in_height, water_width, water_height, src_format, rect_num, rects, color);

    /* // dump output
    char vpss_filename[128];
    char cmodel_filename[128];
    snprintf(vpss_filename, sizeof(vpss_filename), "vpss_%s", filename_dst);
    snprintf(cmodel_filename, sizeof(cmodel_filename), "cmodel_%s", filename_dst);

    file = fopen(vpss_filename, "wb");
    if (!file) {
        perror("Failed to open vpss destination file");
        ret = BM_ERR_FAILURE;
        goto fail;
    }
    fwrite(vpss_output, sizeof(unsigned char), src_size, file);
    fclose(file);

    file = fopen(cmodel_filename, "wb");
    if (!file) {
        perror("Failed to open cmodel destination file");
        ret = BM_ERR_FAILURE;
        goto fail;
    }
    fwrite(cmodel_output, sizeof(unsigned char), src_size, file);
    fclose(file); */

    ret = compare(vpss_output, cmodel_output, src_size);

fail:
    free(input_data);
    free(vpss_output);
    free(cmodel_output);
    return ret;
}

int main(int argc, char **argv) {
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    char filename_src[128];
    char filename_water[128];
    char filename_dst[128];
    int in_width = 0;
    int in_height = 0;
    int water_width = 0;
    int water_height = 0;
    int rect_num = 0;
    int use_real_img = 0;
    bm_image_format_ext src_format = 0;
    bmcv_rect_t rect_attr = {
        .start_x = 0,
        .start_y = 0,
        .crop_w = 0,
        .crop_h = 0};
    bmcv_color_t color = {
        .r = 0,
        .g = 0,
        .b = 0};

    printf("seed = %d\n", seed);
    if (argc != 16 && argc != 17) {
        printf("usage: %s in_w in_h water_w water_h src_format rect_num(1) start_x start_y crop_x crop_y  r_value g_value b_value use_real_img (src.bin water.bin)\n", argv[0]);
        printf("example:\n");
        printf("%s 1920 1080 128 128 8 1 900 480 128 128 151 255 152 1 res/128x128_sophgo.bin res/1920x1080_rgbp.bin\n", argv[0]);
        printf("%s 1920 1080 128 128 8 1 900 480 128 128 151 255 152 0 res/128x128_sophgo.bin\n", argv[0]);
        return -1;
    }

    in_width = atoi(argv[1]);
    in_height = atoi(argv[2]);
    water_width = atoi(argv[3]);
    water_height = atoi(argv[4]);
    src_format = atoi(argv[5]);
    rect_num = atoi(argv[6]);
    rect_attr.start_x = atoi(argv[7]);
    rect_attr.start_y = atoi(argv[8]);
    rect_attr.crop_w = atoi(argv[9]);
    rect_attr.crop_h = atoi(argv[10]);
    color.r = atoi(argv[11]);
    color.g = atoi(argv[12]);
    color.b = atoi(argv[13]);
    use_real_img = atoi(argv[14]);
    strncpy(filename_water, argv[15], sizeof(filename_water));
    if (argc == 17) {
        strncpy(filename_src, argv[16], sizeof(filename_src));
    }

    bm_status_t ret =
        vpss_watermark_superpose_cmp(use_real_img, filename_src, filename_water, filename_dst, in_width, in_height,
                                     water_width, water_height, src_format, rect_num, &rect_attr, color);

    if (ret != BM_SUCCESS) {
        printf("Compare vpss_watermark result with cmodel failed! \n");
    } else {
        printf("Compare vpss_watermark result with cmodel successfully ! \n");
    }
    return ret;
}