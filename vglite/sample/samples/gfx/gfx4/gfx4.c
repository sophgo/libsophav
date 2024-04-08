
/* rectangle.c */
/* rectangle */
/* Created by ke.liao on 9/6/15. */
/* Copyright (c) 2015 Vivante Corporation. All rights reserved. */


#include <stdio.h>
#include <stdlib.h>
#include "vg_lite.h"
#include "vg_lite_util.h"

static vg_lite_buffer_t buffer;
static vg_lite_buffer_t * fb;

signed char path_data1[] = {
    2,  0,  0,
    4,  1,  0,
    4,  1,  1,
    4,  0,  1,
    0
};

signed char path_data2[] = {
    2,  -100,  -60,
    4,   100,  -60,
    4,   100,   60,
    4,  -100,   60,
    0
};

static vg_lite_path_t path[] = {
    {{-320, -240, 320, 240}, VG_LITE_LOW, VG_LITE_S8, {0}, sizeof(path_data1), path_data1,1},
    {{-320, -240, 320, 240}, VG_LITE_LOW, VG_LITE_S8, {0}, sizeof(path_data2), path_data2,1}
};

uint32_t color_data[] = {
    0xFF0000FF, 0x0000FF00
};

void cleanup(void)
{
    if (buffer.handle != NULL) {
        /* Free the buffer memory. */
        vg_lite_free(&buffer);
    }
}

int main(int argc, const char * argv[])
{
    vg_lite_matrix_t matrix;
    vg_lite_error_t error = VG_LITE_SUCCESS;
    /* Try to open the framebuffer. */
    if (argc == 3) {
        buffer.width = atoi(argv[1]);
        buffer.height = atoi(argv[2]);
    } else {
        buffer.width  = 640;
        buffer.height = 480;
    }
    /* Initialize the draw. */
    error = vg_lite_init(buffer.width, buffer.height);
    if (error) {
        printf("vg_lite_draw_init() returned error %d\n", error);
        cleanup();
        return -1;
    }
    buffer.format = VG_LITE_BGRA8888;   
    error = vg_lite_allocate(&buffer);
    if (error) {
        printf("vg_lite_allocate() returned error %d\n", error);
        cleanup();
        return -1;
    }
    fb = &buffer;
    /* Clear the buffer */
    vg_lite_clear(fb, NULL, 0xFFFF0000);

    /* Setup a scale at center of buffer. */
    vg_lite_identity(&matrix);
    vg_lite_translate(fb->width / 2, fb->height / 2, &matrix);
    /* Draw the path using the matrix. */
    error = vg_lite_draw(fb, &path[0], VG_LITE_FILL_EVEN_ODD, &matrix, VG_LITE_BLEND_NONE, 0xFF0000FF);
    if (error) {
        printf("vg_lite_draw() returned error %d\n", error);
        cleanup();
        return -1;
    }
    vg_lite_finish();
    vg_lite_save_png("gfx4.png", fb);
    /* Cleanup. */
    cleanup();

    return 0;
}
