#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <sys/time.h>
#include <bmcv_api_ext_c.h>

extern int md5_cmp(unsigned char* got, unsigned char* exp ,int size);

int main(int argc, char* args[]){

    setlocale(LC_ALL, "");
    bm_status_t ret = BM_SUCCESS;
    wchar_t hexcode[256];
    int r = 255, g = 255, b = 0;
    int width = 1920, height = 1080, orgx = 0, orgy = 500;
    bm_image_format_ext fmt = FORMAT_RGB_PACKED;
    float fontScale = 2;
    char* output_path = "out.bmp";
    char* input_path = "/opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin";
    char *md5 = "f13fde34341865e451be167c2b2bf08f";
    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: %d\n", argc);
        printf("%s text_string r g b fontscale out_name md5 in_name w h fmt x y\n", args[0]);
        printf("example:\n");
        printf("%s bitmain.go\n", args[0]);
        printf("%s bitmain.go 255 255 255 2 out.bmp\n", args[0]);
        printf("%s bitmain.go 255 255 255 2 out.bmp f13fde34 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin 1920 1080 10 0 500\n", args[0]);
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
    if (argc > 5) fontScale = atof(args[5]);
    if (argc > 6) output_path = args[6];
    if (argc > 7) md5 = args[7];
    if (argc > 8) input_path = args[8];
    if (argc > 9) width = atoi(args[9]);
    if (argc > 10) height = atoi(args[10]);
    if (argc > 11) fmt = atoi(args[11]);
    if (argc > 12) orgx = atoi(args[12]);
    if (argc > 13) orgy = atoi(args[13]);

    bm_image image;
    bm_handle_t handle = NULL;
    bm_dev_request(&handle, 0);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &image, NULL);
    bm_image_alloc_dev_mem(image, BMCV_HEAP1_ID);
    bm_read_bin(image, input_path);
    bmcv_point_t org = {.x = orgx, .y = orgy};
    bmcv_color_t color = {.r = r, .g = g, .b = b};
    struct timeval tv[2];
    bm_image watermark;
    int time_single = 0;
    gettimeofday(tv, NULL);
    ret = bmcv_gen_text_watermark(handle, hexcode, color, fontScale, FORMAT_ARGB_PACKED, &watermark);
    if (ret != BM_SUCCESS) {
        printf("bmcv_gen_text_watermark fail\n");
        goto fail1;
    }

    gettimeofday(tv + 1, NULL);
    time_single = (unsigned int)((tv[1].tv_sec - tv[0].tv_sec) * 1000000 + tv[1].tv_usec - tv[0].tv_usec);
    printf("bmcv_gen_text_watermark time %d\n", time_single);

    bmcv_rect_t rect = {.start_x = org.x, .start_y = org.y, .crop_w = watermark.width, .crop_h = watermark.height};
    ret = bmcv_image_overlay(handle, image, 1, &rect, &watermark);
    if (ret != BM_SUCCESS) {
        printf("bmcv_image_overlay fail\n");
        goto fail2;
    }

    gettimeofday(tv + 1, NULL);
    time_single = (unsigned int)((tv[1].tv_sec - tv[0].tv_sec) * 1000000 + tv[1].tv_usec - tv[0].tv_usec);
    printf("all time %d\n", time_single);
    if (argc > 1) {
        bm_image_write_to_bmp(image, output_path);
        printf("bm_image_write_to_bmp path: %s\n", output_path);
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