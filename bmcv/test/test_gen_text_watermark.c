#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <bmcv_api_ext_c.h>

extern int md5_cmp(unsigned char* got, unsigned char* exp ,int size);

int main(int argc, char* args[]){

    setlocale(LC_ALL, "");
    bm_status_t ret = BM_SUCCESS;
    wchar_t hexcode[256];
    int r = 255, g = 255, b = 0;
    unsigned char fontScale = 2;
    char* output_path = "out.bmp";
    char *md5 = "eb2edce6ba908bc2ce57c99652e2c7cc";
    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: %d\n", argc);
        printf("%s text_string r g b fontscale out_name\n", args[0]);
        printf("example:\n");
        printf("%s bitmain.go\n", args[0]);
        printf("%s bitmain.go 255 255 255 2 out.bmp\n", args[0]);
        return 0;
    }
    if (argc > 1)
        mbstowcs(hexcode, args[1], sizeof(hexcode) / sizeof(wchar_t)); //usigned
    else
        mbstowcs(hexcode, "算能sophgo", sizeof(hexcode) / sizeof(wchar_t)); //usigned
    printf("Received wide character string: %ls\n", hexcode);
    if (argc > 2) r = atoi(args[2]);
    if (argc > 3) g = atoi(args[3]);
    if (argc > 4) b = atoi(args[4]);
    if (argc > 5) fontScale = atoi(args[5]);
    if (argc > 6) output_path = args[6];
    if (argc > 7) md5 = args[7];

    bm_image image;
    bm_handle_t handle = NULL;
    bm_dev_request(&handle, 0);
    bm_image_create(handle, 1080, 1920, FORMAT_YUV420P, DATA_TYPE_EXT_1N_BYTE, &image, NULL);
    bm_image_alloc_dev_mem(image, BMCV_HEAP1_ID);
    bm_read_bin(image,"/opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin");
    bmcv_point_t org = {.x = 0, .y = 500};
    bmcv_color_t color = {.r = r, .g = g, .b = b};

    bm_image watermark;
    ret = bmcv_gen_text_watermark(handle, hexcode, color, fontScale, FORMAT_ARGB_PACKED, &watermark);
    if (ret != BM_SUCCESS) {
        printf("bmcv_gen_text_watermark fail\n");
        goto fail1;
    }

    bmcv_rect_t rect = {.start_x = org.x, .start_y = org.y, .crop_w = watermark.width, .crop_h = watermark.height};
    ret = bmcv_image_overlay(handle, image, 1, &rect, &watermark);
    if (ret != BM_SUCCESS) {
        printf("bmcv_image_overlay fail\n");
        goto fail2;
    }
    if (argc > 1) {
        bm_image_write_to_bmp(image, output_path);
        printf("output path: %s\n", output_path);
    }
    if (argc == 1 || argc > 7) {
        int image_byte_size[4] = {0};
        bm_image_get_byte_size(image, image_byte_size);
        int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
        unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
        void* out_ptr[4] = {(void*)output_ptr,
                            (void*)((unsigned char*)output_ptr + image_byte_size[0]),
                            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
        bm_image_copy_device_to_host(image, (void **)out_ptr);
        if(md5_cmp(output_ptr, (unsigned char*)md5, byte_size)!=0){
            bm_write_bin(image, "error_cmp.bin");
            ret = -1;
        }
        free(output_ptr);
    }

fail2:
    bm_image_destroy(&watermark);
fail1:
    bm_image_destroy(&image);
    bm_dev_free(handle);
    return ret;
}