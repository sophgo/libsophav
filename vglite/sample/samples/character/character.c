/*
 Description: Read raw files and use upload to
 generate gradient character images.It can count
 the running time if each frame on the linux platform.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vg_lite.h"
#include "vg_lite_util.h"


#define DEFAULT_SIZE   20;
static int fb_width = 600, fb_height = 32;
static float fb_scale = 1.0f;

/*offscreen framebuffer object for rendering.*/
static vg_lite_buffer_t buffer;
static vg_lite_buffer_t* fb;

void cleanup(void)
{
    if (buffer.handle != NULL) {
        /* Free the offscreen framebuffer memory.*/
        vg_lite_free(&buffer);
    }
    vg_lite_close();
}

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        printf("character  *.raw *.raw ....\n");
        printf("params: max 32 raw.\n");
        return 0;
    }
    vg_lite_linear_gradient_t grad;
    uint32_t ramps[] = { 0xffffffff, 0xff000000 };
    uint32_t stops[] = { 0, 36 };
    vg_lite_matrix_t* matGrad;
    vg_lite_error_t error = VG_LITE_SUCCESS;
    vg_lite_matrix_t matrix[24];
    float offsetX = 0;
    float startX = 0.0f;
    float startY = -5;
    FILE* fd[32];
    float* path_data[32];
    static vg_lite_path_t path[32];
    int32_t length;
    int row = 0;
    int column = 0;
    int frame = 0;
    int character = 0;
    int fileid = 0;
    int index = 0;
    int path_head_size = 0;
    /* Initialize the draw.*/
    error = vg_lite_init(640, 320);
    if (error) {
        printf("vg_lite engine init failed: vg_lite_init() returned error %d\n", error);
        cleanup();
        return -1;
    }

    fb_scale = 1.3;
    printf("Framebuffer size: %d x %d\n", fb_width, fb_height);

    /*Allocate the off-screen buffer.*/
    buffer.width = fb_width;
    buffer.height = fb_height;
    buffer.format = VG_LITE_BGRA8888;

    error = vg_lite_allocate(&buffer);
    if (error) {
        printf("vg_lite_allocate() returned error %d\n", error);
        cleanup();
        return -1;
    }
    fb = &buffer;
    /* Clear the buffer with white. */
    vg_lite_clear(fb, NULL, 0xFFFFFFFF);

    for (row = 0; row < 1; row++) {
        for (column = 0; column < 24; column++) {
            vg_lite_identity(&matrix[row * 24 + column]);
            /* Translate the matrix to the center of the buffer.*/
            vg_lite_scale(fb_scale, fb_scale, &matrix[row * 24 + column]);
            vg_lite_translate(18 * column, 0, &matrix[row * 24 + column]);
            vg_lite_translate(startX, -7, &matrix[row * 24 + column]);
        }
    }

    for (int i = 0; i< argc; i++) {
        fd[i] = fopen(argv[i+1], "rb");
    }

    for (fileid = 0; fileid < argc; fileid++) {
        memset(&path[fileid], 0, sizeof(path[fileid]));
        /* Distinguish between 64-bit and 32-bit platforms. */
        int length = sizeof(void*);
        if (length == 4) {
            fread(&path[fileid], sizeof(path[fileid]), 1, fd[fileid]);
        }
        else if (length == 8) {
            /* Read the data before path_length */
            int size1 = sizeof(vg_lite_float_t)*4 + sizeof(vg_lite_quality_t) + sizeof(vg_lite_format_t);
            fread(&path[fileid], size1 + 20, 1, fd[fileid]);
            uint32_t data = 0;
            /* Skip the effect of different upload sizes. */
            data = (((uint32_t)&(path[fileid].bounding_box)) +size1 + 32);
            /* 64bit platform path struct size is 112 bytes while 32bit platform is 80,so 32bytes less. */
            fread(data, sizeof(path[fileid]) - size1 - 20  - 32,1, fd[fileid]);
        }
        length = path[fileid].path_length;
        path_data[fileid] = malloc(path[fileid].path_length);
        fread(path_data[fileid], 1, path[fileid].path_length, fd[fileid]);
        fclose(fd[fileid]);
        path[fileid].path = path_data[fileid];
    }

    for (index = 0; index < argc; index++) {
        error = vg_lite_upload_path(&path[index]);
    }
    index = 0;


    for (frame = 0; frame < 1; frame++) {
        for (character = 0; character < argc; character++) {
            // index = character % 11;
            error = vg_lite_draw(fb, &path[character], VG_LITE_FILL_EVEN_ODD, &matrix[character], VG_LITE_BLEND_NONE, 0xFF0000FF);
            if (error) {
                printf("vg_lite_draw() returned error %d\n", error);
                cleanup();
                free(path_data);
                return -1;
            }

        }
        printf("frame%d\n", frame);
        error = vg_lite_finish();
    }
    vg_lite_save_png("plyhs2_1.png", fb);
    cleanup();
}

