#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vg_lite.h"
#include "vg_lite_util.h"

#define DEFAULT_SIZE   256.0f;
#define __func__ __FUNCTION__
char *error_type[] = 
{
    "VG_LITE_SUCCESS",
    "VG_LITE_INVALID_ARGUMENT",
    "VG_LITE_OUT_OF_MEMORY",
    "VG_LITE_NO_CONTEXT",
    "VG_LITE_TIMEOUT",
    "VG_LITE_OUT_OF_RESOURCES",
    "VG_LITE_GENERIC_IO",
    "VG_LITE_NOT_SUPPORT",
};
#define IS_ERROR(status)         (status > 0)
#define CHECK_ERROR(Function) \
    error = Function; \
    if (IS_ERROR(error)) \
    { \
        printf("[%s: %d] failed.error type is %s\n", __func__, __LINE__,error_type[error]);\
        goto ErrorHandler; \
    }

static int   fb_width = 256, fb_height = 256;
static float fb_scale = 1.0f;

static vg_lite_buffer_t buffer,buffer1;
static vg_lite_buffer_t * fb;

vg_lite_compress_mode_t compress_mode[] = {
    VG_LITE_DEC_NON_SAMPLE,
    VG_LITE_DEC_HSAMPLE
};

void cleanup(void)
{
    if (buffer.handle != NULL) {
        vg_lite_free(&buffer);
    }

    if (buffer1.handle != NULL) {
        vg_lite_free(&buffer1);
    }
    vg_lite_close();
}

int main(int argc, const char * argv[])
{
    vg_lite_rectangle_t rect = { 64, 64, 64, 64 };
    vg_lite_error_t error = VG_LITE_SUCCESS;
    vg_lite_matrix_t matrix;
    vg_lite_blend_t blend = VG_LITE_BLEND_SRC_OVER;
    char filename[30];
    char filename1[30];
    int i;

    CHECK_ERROR(vg_lite_init(0, 0));

    fb_scale = (float)fb_width / DEFAULT_SIZE;
    printf("Framebuffer size: %d x %d\n", fb_width, fb_height);
    if(!vg_lite_query_feature(gcFEATURE_BIT_VG_DEC_COMPRESS)) {
        printf(" dec compress is not support\n");
    }
    for(i = 0 ; i < sizeof(compress_mode)/sizeof(compress_mode[0]); i++)
    {
        sprintf(filename,"clear_compressed%d.png",i + 1);
        sprintf(filename1,"clear_%d.dec",i + 1);
        buffer.width  = fb_width;
        buffer.height = fb_height;
        buffer.format = VG_LITE_RGBA8888;
        buffer.compress_mode = compress_mode[i];

        CHECK_ERROR(vg_lite_allocate(&buffer));
        fb = &buffer;
    
        CHECK_ERROR(vg_lite_clear(fb, NULL, 0xFFFF0000));
        CHECK_ERROR(vg_lite_clear(fb, &rect, 0xFF0000FF));
        CHECK_ERROR(vg_lite_finish());
        if (vg_lite_save_decnano_compressd_data(filename1,fb)) {
            printf("save dec file error\n");
            goto ErrorHandler;
        }
        CHECK_ERROR(vg_lite_free(fb));

        if (vg_lite_load_dev_info_to_buffer(fb, filename1)) {
            printf("load dec file error\n");
            goto ErrorHandler;
        }
        CHECK_ERROR(vg_lite_allocate(fb));
        if(vg_lite_load_decnano_compressd_data(fb, filename1)) {
            printf("load dec file error\n");
            goto ErrorHandler;
        }
        buffer1.width  = fb_width;
        buffer1.height = fb_height;
        buffer1.format = VG_LITE_RGBA8888;
        buffer1.compress_mode = VG_LITE_DEC_DISABLE;
        if(vg_lite_query_feature(gcFEATURE_BIT_VG_IM_DEC_INPUT)) {
            CHECK_ERROR(vg_lite_allocate(&buffer1));
            vg_lite_clear(&buffer1,NULL,0xFF0000ff);
            vg_lite_identity(&matrix);
            CHECK_ERROR(vg_lite_blit( &buffer1, fb, &matrix, blend, 0xFF00FF00, VG_LITE_FILTER_POINT));
            CHECK_ERROR(vg_lite_finish());
            vg_lite_save_png(filename, &buffer1);
            vg_lite_free(&buffer1);
            vg_lite_free(fb);
        }
        else {
            printf(" dec file input is not support\n");
        }
    }

ErrorHandler:
    cleanup();
    return 0;
}
