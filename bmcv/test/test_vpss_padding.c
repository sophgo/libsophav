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

typedef struct border {
    bool rect_border_enable;
    u16 st_x;
    u16 st_y;
    u16 width;
    u16 height;
    u8 value_r;
    u8 value_g;
    u8 value_b;
    u16 thickness;
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

extern bm_status_t bm_vpss_cmodel(bm_image *input,
                                  bm_image *output,
                                  bmcv_rect_t *input_crop_rect,
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

bm_status_t vpss_convert_padding(unsigned char *src_data,
                                 unsigned char *dst_data,
                                 int in_width,
                                 int in_height,
                                 int out_width,
                                 int out_height,
                                 bm_image_format_ext src_format,
                                 bm_image_format_ext dst_format,
                                 bmcv_padding_attr_t *padding_rect,
                                 bmcv_rect_t *crop_rect,
                                 bmcv_resize_algorithm algorithm) {
    bm_handle_t handle = NULL;
    int dev_id = 0;
    bm_status_t ret = BM_SUCCESS;
    bm_image src, dst;

    ret = bm_dev_request(&handle, dev_id);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return ret;
    }

    bm_image_create(handle, in_height, in_width, src_format, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    bm_image_create(handle, out_height, out_width, dst_format, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);
    bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

    int src_image_byte_size[4] = {0};
    bm_image_get_byte_size(src, src_image_byte_size);

    void *src_in_ptr[4] = {(void *)src_data,
                           (void *)((char *)src_data + src_image_byte_size[0]),
                           (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1]),
                           (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1] + src_image_byte_size[2])};

    bm_image_copy_host_to_device(src, (void **)src_in_ptr);

    ret = bmcv_image_vpp_convert_padding(handle, 1, src, &dst, padding_rect, crop_rect, algorithm);
    if (ret != BM_SUCCESS) {
        printf("bmcv_image_convert_padding failed! \n");
    }

    int dst_image_byte_size[4] = {0};
    bm_image_get_byte_size(dst, dst_image_byte_size);

    void *dst_in_ptr[4] = {(void *)dst_data,
                           (void *)((char *)dst_data + dst_image_byte_size[0]),
                           (void *)((char *)dst_data + dst_image_byte_size[0] + dst_image_byte_size[1]),
                           (void *)((char *)dst_data + dst_image_byte_size[0] + dst_image_byte_size[1] + dst_image_byte_size[2])};

    bm_image_copy_device_to_host(dst, (void **)dst_in_ptr);

    bm_image_destroy(&src);
    bm_image_destroy(&dst);
    bm_dev_free(handle);
    return ret;
}

bm_status_t cmodel_convert_padding(unsigned char *src_data,
                                   unsigned char *dst_data,
                                   int in_width,
                                   int in_height,
                                   int out_width,
                                   int out_height,
                                   bm_image_format_ext src_format,
                                   bm_image_format_ext dst_format,
                                   bmcv_padding_attr_t *padding_rect,
                                   bmcv_rect_t *crop_rect,
                                   bmcv_resize_algorithm algorithm) {
    bm_handle_t handle = NULL;
    int dev_id = 0;
    bm_status_t ret = BM_SUCCESS;
    bm_image src, dst;
    bm_device_mem_t src_cmodel_mem[4];
    bm_device_mem_t dst_cmodel_mem[4];

    ret = bm_dev_request(&handle, dev_id);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return ret;
    }

    bm_image_create(handle, in_height, in_width, src_format, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    bm_image_create(handle, out_height, out_width, dst_format, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

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
    bmcv_rgn_cfg my_rgn_config = {
        .param = NULL,
        .rgn_num = 0};

    ret = bm_vpss_cmodel(&src, &dst, crop_rect, padding_rect, algorithm, CSC_YCbCr2RGB_BT601, NULL, &convert_to_attr, &my_border, &my_coverex, &my_rgn_config);
    if (ret != BM_SUCCESS) {
        printf("bm_vpss_cmodel failed! \n");
    }

    bm_image_destroy(&src);
    bm_image_destroy(&dst);
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

bm_status_t vpss_convert_padding_cmp(int use_real_img,
                                     char *filename_src,
                                     char *filename_dst,
                                     int in_width,
                                     int in_height,
                                     int out_width,
                                     int out_height,
                                     bm_image_format_ext src_format,
                                     bm_image_format_ext dst_format,
                                     bmcv_padding_attr_t *padding_rect,
                                     bmcv_rect_t *crop_rect,
                                     bmcv_resize_algorithm algorithm) {
    bm_status_t ret = BM_SUCCESS;

    int src_size = get_image_data_size(in_width, in_height, src_format);
    int dst_size = get_image_data_size(out_width, out_height, dst_format);
    unsigned char *input_data = (unsigned char *)malloc(src_size);
    unsigned char *vpss_output = (unsigned char *)malloc(dst_size);
    unsigned char *cmodel_output = (unsigned char *)malloc(dst_size);
    if (!input_data || !vpss_output || !cmodel_output) {
        printf("Memory Allocation Failed \n");
        ret = BM_ERR_FAILURE;
        goto fail;
    }

    FILE *file;
    if (use_real_img) {
        // read input.bin
        file = fopen(filename_src, "rb");
        if (!file) {
            perror("Failed to open source file");
            ret = BM_ERR_FAILURE;
            goto fail;
        }
        fread(input_data, sizeof(unsigned char), src_size, file);
        fclose(file);
    } else {
        fill(input_data, src_size);
    }

    vpss_convert_padding(input_data, vpss_output, in_width, in_height, out_width, out_height, src_format, dst_format, padding_rect, crop_rect, algorithm);

    cmodel_convert_padding(input_data, cmodel_output, in_width, in_height, out_width, out_height, src_format, dst_format, padding_rect, crop_rect, algorithm);

    // //dump output
    // char vpss_filename[128];
    // char cmodel_filename[128];
    // snprintf(vpss_filename, sizeof(vpss_filename), "vpss_%s", filename_dst);
    // snprintf(cmodel_filename, sizeof(cmodel_filename), "cmodel_%s", filename_dst);
    // file = fopen(vpss_filename, "wb");
    // if (!file) {
    //     perror("Failed to open vpss destination file");
    //     ret = BM_ERR_FAILURE;
    //     goto fail;
    // }
    // fwrite(vpss_output, sizeof(unsigned char), dst_size, file);
    // fclose(file);
    // file = fopen(cmodel_filename, "wb");
    // if (!file) {
    //     perror("Failed to open cmodel destination file");
    //     ret = BM_ERR_FAILURE;
    //     goto fail;
    // }
    // fwrite(cmodel_output, sizeof(unsigned char), dst_size, file);
    // fclose(file);

    ret = compare(vpss_output, cmodel_output, dst_size);

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
    char filename_dst[128];
    int in_width = 0;
    int in_height = 0;
    int out_width = 0;
    int out_height = 0;
    bm_image_format_ext src_format = 0;
    bm_image_format_ext dst_format = 0;
    bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR;
    int use_real_img = 0;

    bmcv_rect_t crop_attr = {
        .start_x = 0,
        .start_y = 0,
        .crop_w = 0,
        .crop_h = 0
        };

    bmcv_padding_attr_t padding_attr = {
        .dst_crop_stx = 0,
        .dst_crop_sty = 0,
        .dst_crop_w = 0,
        .dst_crop_h = 0,
        .padding_r = 0,
        .padding_g = 0,
        .padding_b = 0,
        .if_memset = 1
        };

    printf("seed = %d\n", seed);
    if (argc != 19 && argc != 20) {
        printf("usage: %s in_w in_h crop_stx crop_sty crop_w crop_h dst_crop_stx dst_crop_sty dst_crop_w dst_crop_h padding_r padding_g padding_b out_w out_h src_format algorithm use_real_img (src.bin)\n", argv[0]);
        printf("example:\n");
        printf("use real image:\n");
        printf("%s 1920 1080 480 270 960 540 20 20 960 960 151 255 152 1000 1000 8 1 1 res/1920x1080_rgbp.bin\n", argv[0]);
        printf("use random numbers:\n");
        printf("%s 1920 1080 480 270 960 540 20 20 960 960 151 255 152 1000 1000 8 1 0\n", argv[0]);
        return -1;
    }

    in_width = atoi(argv[1]);
    in_height = atoi(argv[2]);
    crop_attr.start_x = atoi(argv[3]);
    crop_attr.start_y = atoi(argv[4]);
    crop_attr.crop_w = atoi(argv[5]);
    crop_attr.crop_h = atoi(argv[6]);
    padding_attr.dst_crop_stx = atoi(argv[7]);
    padding_attr.dst_crop_sty = atoi(argv[8]);
    padding_attr.dst_crop_w = atoi(argv[9]);
    padding_attr.dst_crop_h = atoi(argv[10]);
    padding_attr.padding_r = atoi(argv[11]);
    padding_attr.padding_g = atoi(argv[12]);
    padding_attr.padding_b = atoi(argv[13]);
    out_width = atoi(argv[14]);
    out_height = atoi(argv[15]);
    src_format = atoi(argv[16]);
    algorithm = atoi(argv[17]);
    use_real_img = atoi(argv[18]);
    if (argc == 20) {
        memcpy(filename_src, argv[19], sizeof(filename_src));
    }

    dst_format = src_format;
    bm_status_t ret = vpss_convert_padding_cmp(use_real_img, filename_src, filename_dst, in_width, in_height, out_width, out_height, src_format, dst_format, &padding_attr, &crop_attr, algorithm);

    if (ret != BM_SUCCESS) {
        printf("Compare vpss_convert_padding result with cmodel failed! \n");
    } else {
        printf("Compare vpss_convert_padding result with cmodel successfully! !\n");
    }
    return ret;
}
