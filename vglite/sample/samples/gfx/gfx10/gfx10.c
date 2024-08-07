
/*  gfx10.c */
/*  gfx10 */
/*  Created by ke.liao on 9/6/15. */
/*  Copyright (c) 2015 Vivante Corporation. All rights reserved. */

#include <stdio.h>
#include <stdlib.h>
#include "vg_lite.h"
#include "vg_lite_util.h"

static vg_lite_buffer_t buffer;
static vg_lite_buffer_t * fb;

signed char path_t1[] = {
    2,  -100,  -100,
    4,  100,  -100,
    4,  100,  -90,
    4,  5,  -90,
    4,  5,  100,
    4,  -5,    100,
    4,  -5,    -90,
    4,  -100,   -90,
    0
};

signed char path_t2[] = {
    2,  -100,  -100,
    4,  100,  -100,
    4,  100,  -60,
    4,  90,   -80,
    4,  10,  -80,
    4,  10,  90,
    4,  30,  100,
    4,  -30,    100,
    4,  -10,  90,
    4,  -10,    -80,
    4,  -90,   -80,
    4,  -100,  -60,
    0
};

signed char path_e1[] = {
    2,  -60,  -100,
    4,  60,  -100,
    4,  60,  -90,
    4,  -50,  -90,
    4,  -50,  -5,
    4,  40,    -5,
    4,  40,    5,
    4,  -50,  5,
    4,  -50,    90,
    4,  60,    90,
    4,  60,    100,
    4,  -60,   100,
    0
};

signed char path_e2[] = {
    2,  -80,  -100,
    4,  60,  -100,
    4,  60,  -60,
    4,  50,  -80,
    4,  -40,  -80,
    4,  -40,  -10,
    4,  30,   -10,
    4,  40,   -30,
    4,  40,    30,
    4,  30,    10,
    4,  -40,   10,
    4,  -40,   80,
    4,  50,    80,
    4,  60,    60,
    4,  60,    100,
    4,  -80,   100,
    4,  -60,   80,
    4,  -60,  -80,
    0
};

signed char path_x1[] = {
    2,  -100,  -100,
    4,  -90,  -100,
    4,  100,   100,
    4,  90,  100,
    0
};

signed char path_x2[] = {
    2,  90,  -100,
    4,  100,  -100,
    4,  -90,  100,
    4,  -100,    100,
    0
};

signed char path_x3[] = {
    2,  -120,  -100,
    4,  -60,  -100,
    4,  -70,  -90,
    4,  90,   90,
    4,  120,   100,
    4,  60,    100,
    4,  70,   90,
    4,  -90,    -90,
    0
};

signed char path_x4[] = {
    2,  60,  -100,
    4,  120,  -100,
    4,  90,  -90,
    4,  -70,  90,
    4,  -60,    100,
    4,  -120,   100,
    4,  -90,    90,
    4,  70,    -90,
    0
};

static vg_lite_path_t path[][8] = {
    { {{-160, -120, 160, 120}, VG_LITE_HIGH, VG_LITE_S8, {0}, sizeof(path_t1), path_t1,1}, },
    { {{-160, -120, 160, 120}, VG_LITE_HIGH, VG_LITE_S8, {0}, sizeof(path_e1), path_e1,1}, },
    {
        {{-160, -120, 160, 120}, VG_LITE_HIGH, VG_LITE_S8, {0}, sizeof(path_x1), path_x1,1},
        {{-160, -120, 160, 120}, VG_LITE_HIGH, VG_LITE_S8, {0}, sizeof(path_x2), path_x2,1},
    },
    { {{-160, -120, 160, 120}, VG_LITE_HIGH, VG_LITE_S8, {0}, sizeof(path_t1), path_t1,1}, },
    { {{-160, -120, 160, 120}, VG_LITE_HIGH, VG_LITE_S8, {0}, sizeof(path_t2), path_t2,1}, },
    { {{-160, -120, 160, 120}, VG_LITE_HIGH, VG_LITE_S8, {0}, sizeof(path_e2), path_e2,1}, },
    {
        {{-160, -120, 160, 120}, VG_LITE_HIGH, VG_LITE_S8, {0}, sizeof(path_x3), path_x3,1},
        {{-160, -120, 160, 120}, VG_LITE_HIGH, VG_LITE_S8, {0}, sizeof(path_x4), path_x4,1},
    },
    { {{-160, -120, 160, 120}, VG_LITE_HIGH, VG_LITE_S8, {0}, sizeof(path_t2), path_t2,1}, },
};

uint32_t color_data[] = {
    0xFF0000FF, 0x0000FF00
};

int pathCount[] = {
    1, 1, 2, 1, 1, 1, 2, 1, 
};

int pathNum = 4;

void cleanup(void)
{
    if (buffer.handle != NULL) {
        /* Free the buffer memory. */
        vg_lite_free(&buffer);
    }
}

int main(int argc, const char * argv[])
{
    int i, j, k;
    vg_lite_error_t error = VG_LITE_SUCCESS;

    /* Allocate the buffer. */
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
    for (k = 0; k < 2; k++) {
        for (i = 0; i < pathNum; i++) {
            for (j = 0; j < pathCount[i]; j++)
            {
                vg_lite_matrix_t matrix;
                vg_lite_identity(&matrix);
                vg_lite_translate(fb->width / pathNum * (i + 0.5f), fb->height / 2 * (k + 0.5f), &matrix);
                vg_lite_scale(fb->width / 320.0f / pathNum, fb->width / 320.0f / pathNum, &matrix);
                
                /* Draw the path using the matrix. */
                error = vg_lite_draw(fb, &path[i + k * 4][j], VG_LITE_FILL_EVEN_ODD, &matrix, VG_LITE_BLEND_NONE, 0xFF0000FF);
                if (error) {
                    printf("vg_lite_draw() returned error %d\n", error);
                    cleanup();
                    return -1;
                }
            }
        }
    }
    vg_lite_finish();
    vg_lite_save_png("gfx10.png", fb);

    /* Cleanup. */
    cleanup();

    return 0;
}
