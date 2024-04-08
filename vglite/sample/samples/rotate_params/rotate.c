/*
 Resolution: 320 x 480
 Format: VG_LITE_RGB565
 Transformation: Rotate/Scale/Translate
 Alpha Blending: None
 Related APIs: vg_lite_clear/vg_lite_translate/vg_lite_scale/vg_lite_rotate/vg_lite_blit
 Description: Load outside landscape image, then blit it to blue dest buffer with rotate/scale/translate.
 Image filter type is selected by hardware feature gcFEATURE_BIT_VG_IM_FILTER(ON: VG_LITE_FILTER_BI_LINEAR, OFF: VG_LITE_FILTER_POINT).
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "vg_lite.h"
#include "vg_lite_util.h"

#define DEFAULT_SIZE   320.0f;
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
static int   fb_width = 320, fb_height = 480;
static float fb_scale = 1.0f;

static vg_lite_buffer_t buffer;     //offscreen framebuffer object for rendering.
static vg_lite_buffer_t * fb;
static vg_lite_buffer_t buf;
static vg_lite_buffer_t *fb2;

static vg_lite_buffer_t image;

void cleanup(void)
{
    if (buffer.handle != NULL) {
        // Free the buffer memory.
        vg_lite_free(&buffer);
    }

    if (image.handle != NULL) {
        // Free the image memory.
        vg_lite_free(&image);
    }

    vg_lite_close();
}


void test_rotate(vg_lite_buffer_t *image, int format, int loops, int slep_us, int dump) {

    vg_lite_filter_t filter;
    vg_lite_matrix_t matrix;

    // Initialize vg_lite engine.
    vg_lite_error_t error = VG_LITE_SUCCESS;
    vg_lite_init(32, 32);

    int ret  = 0;

    // Set image filter type according to hardware feature.
    filter = VG_LITE_FILTER_POINT;

    printf("Framebuffer size: %d x %d\n", image->width, image->height);

    // Allocate the off-screen buffer.
    buf.width  = image->width;
    buf.height = image->height;
    buf.format = format;

    vg_lite_allocate(&buf);
    fb2 = &buf;

    // Clear the buf with blue.
    // ret = vg_lite_clear(fb2, NULL, 0xFFFF0000);
    // Build a 33 degree rotation matrix from the center of the buf.
    vg_lite_identity(&matrix);
    // Translate the matrix to the center of the buf.
    vg_lite_translate(image->width / 2.0f, image->height / 2.0f, &matrix);
    // Do a 33 degree rotation.
    vg_lite_rotate(33.0f, &matrix);
    // Translate the matrix again to adjust matrix location.
    vg_lite_translate(image->width / -2.0f, image->height / -2.0f, &matrix);
    // Scale matrix according to buf size.
    vg_lite_scale((vg_lite_float_t) image->width / (vg_lite_float_t) image->width,
                  (vg_lite_float_t) image->height / (vg_lite_float_t) image->height, &matrix);

    if (slep_us != 0) {
        usleep(slep_us);
    }
    // Blit the image using the matrix.
    vg_lite_blit(fb2, image, &matrix, VG_LITE_BLEND_NONE, 0, filter);
    vg_lite_finish();

    // Save PNG file.
    if (dump == 1) {
        static unsigned int num = 0;
        char filename[256] = {0};
        sprintf(filename, "rotate%d.png", num++);
        vg_lite_save_png(filename, fb2);
    }

    vg_lite_free(&buf);
    return;
}

int main(int argc, const char * argv[])
{

    printf("rotate_params main  2023-04-19 14:32\n");
    if (argc < 5) {
        printf("rotate_params width height format loops [dump png] [sleep(us)]\n");
        printf("\nformat:   0 VG_LITE_RGBA8888;\n");
        printf("            1 VG_LITE_RGB565;\n");
        printf("            2 VG_LITE_ARGB8888;\n");
        printf("            3 VG_LITE_RGB888;\n");
        printf("            4 VG_LITE_ABGR1555;\n");
        printf("sleep(us): for fps  control, defult not set. sleep(7000) core0 is 35fps.\n");
        printf("eg: \n");
        printf("    rotata_params 4096 2160 0 10 0 100\n");
        return 0;
    }


    fb_width  = atoi(argv[1]);
    fb_height = atoi(argv[2]);

    int format    = atoi(argv[3]);
    int loops     = atoi(argv[4]);

    int dump      = 0;
    if (argc == 6) {
        dump      = atoi(argv[5]);
    }

    int slep_us   = 0;
    if (argc == 7) {
        slep_us   = atoi(argv[6]);
    }


    vg_lite_filter_t filter;
    vg_lite_matrix_t matrix;

    // Initialize vg_lite engine.
    vg_lite_error_t error = VG_LITE_SUCCESS;
    CHECK_ERROR(vg_lite_init(32, 32));


    vg_lite_rectangle_t rect = { fb_width/4, fb_height/4, fb_width/2, fb_height/2 };

    // Set image filter type according to hardware feature.
    filter = VG_LITE_FILTER_POINT;

    // Load the image.
    if (vg_lite_load_raw(&image, "landscape.raw") != 0) {
        printf("load raw file error\n");
        cleanup();
        return -1;
    }

    printf("input raw size: %d x %d\n", image.width, image.height);
    printf("Framebuffer size: %d x %d\n", fb_width, fb_height);

    // Allocate the off-screen buffer.
    buffer.width  = fb_width;
    buffer.height = fb_height;
    buffer.format = format;
    CHECK_ERROR(vg_lite_allocate(&buffer));
    fb = &buffer;

    // Clear the buffer with blue.
    CHECK_ERROR(vg_lite_clear(fb, NULL, 0xFFFF0000));
    // Build a 33 degree rotation matrix from the center of the buffer.
    vg_lite_identity(&matrix);
    // Translate the matrix to the center of the buffer.
    vg_lite_translate(fb_width / 2.0f, fb_height / 2.0f, &matrix);

    // Do a 33 degree rotation.
    // vg_lite_rotate(33.0f, &matrix);

    // Translate the matrix again to adjust matrix location.
    vg_lite_translate(fb_width / -2.0f, fb_height / -2.0f, &matrix);

    // Scale matrix according to buffer size.
    vg_lite_scale((vg_lite_float_t) fb_width / (vg_lite_float_t) image.width,
                  (vg_lite_float_t) fb_height / (vg_lite_float_t) image.height, &matrix);

    // printf("main line=%d \n", __LINE__);
    //CHECK_ERROR(vg_lite_clear(fb, &rect, 0xFF0000FF));

    // Blit the image using the matrix.
    CHECK_ERROR(vg_lite_blit(fb, &image, &matrix, VG_LITE_BLEND_NONE, 0, filter));
    CHECK_ERROR(vg_lite_finish());
    // Save PNG file.
    if (dump == 1) {
        vg_lite_save_png("rotate.png", fb);
    }

    for (int i = 0; i< loops; i++) {
        test_rotate(fb, format, loops, slep_us, dump);
    }
ErrorHandler:
    // Cleanup.
    cleanup();
    return 0;
}
