/*
 Resolution: 320 x 320
 Format: VG_LITE_RGBA8888
 Transformation: Translate/Scale
 Alpha Blending: None
 Related APIs: vg_lite_clear/vg_lite_translate/vg_lite_scale/vg_lite_draw
 Description: Draw a glyphs vector graphic .
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vg_lite.h"
#include "vg_lite_util.h"
#include "vladimir_font.h"

#define DEFAULT_SIZE   160;
static int   fb_width = 320, fb_height = 320;
static float fb_scale = 1.0f;

static vg_lite_buffer_t buffer;     /*offscreen framebuffer object for rendering.*/
static vg_lite_buffer_t * fb;

/*Glyph size is specified in pixels per em (ppem)
  The font data(vladimir_font) generated with 24 glyph size*/
#define PPEM_SIZE 24

static vg_lite_path_t path;


void build_path(float* dst, float *src, size_t size)
{
    float *dst_path = dst;
    float *src_path = src;
    uint32_t count = size/4;
    uint32_t iter = 0;
    uint8_t *operation;
    uint8_t op;
    for(iter; iter < count;)
    {
        op = (uint8_t) *src_path;
        *dst_path = *src_path++;
        operation = (uint8_t *)dst_path++;
        *operation = op;
        iter ++;
        switch(op)
        {
        case VLC_OP_END:
            break;
        case VLC_OP_QUAD:
            *dst_path ++ = *src_path++;
            *dst_path ++ = *src_path++;
            *dst_path ++ = *src_path++;
            *dst_path ++ = *src_path++;
            iter += 4;
            break;
        case VLC_OP_MOVE:
        case VLC_OP_LINE:
            *dst_path ++ = *src_path++;
            *dst_path ++ = *src_path++;
            iter += 2;
            break;
        default:
            break;
        }
    }
    return;
}

void cleanup(void)
{
    if (buffer.handle != NULL) {
        /* Free the offscreen framebuffer memory.*/
        vg_lite_free(&buffer);
    }

    vg_lite_close();
}

int main(int argc, const char * argv[])
{
    vg_lite_error_t error = VG_LITE_SUCCESS;
    vg_lite_matrix_t matrix;
    float *path_data;
    float offsetX = 0;
    float startX = 20.0f;
    float startY = 0;

    char string[] = "Hello,\nVerisilicon!";
    char character = 'H';
    int iter = 0;
    float lineHeight = 1.0f * PPEM_SIZE;
    font_path_header fontData;

    /* Initialize the draw.*/
    error = vg_lite_init(fb_width, fb_height);
    if (error) {
        printf("vg_lite engine init failed: vg_lite_init() returned error %d\n", error);
        cleanup();
        return -1;
    }

    fb_scale = (float)fb_width / DEFAULT_SIZE;
    printf("Framebuffer size: %d x %d\n", fb_width, fb_height);

    /*Allocate the off-screen buffer.*/
    buffer.width  = fb_width;
    buffer.height = fb_height;
    buffer.format = VG_LITE_RGBA8888;

    error = vg_lite_allocate(&buffer);
    if (error) {
        printf("vg_lite_allocate() returned error %d\n", error);
        cleanup();
        return -1;
    }
    fb = &buffer;

    /*Draw frame 1*/
    /* Clear the buffer with white.*/
    vg_lite_clear(fb, NULL, 0xFFFFFFFF);

    /* *** DRAW ****/
    vg_lite_identity(&matrix);
    /* Translate the matrix to the center of the buffer.*/
    vg_lite_translate( startX, startY, &matrix);
    /* Scale up matrix with fb_scale times.*/
    vg_lite_scale(fb_scale, fb_scale, &matrix);

    for(iter; string[iter] != '\0'; iter ++)
    {
        fontData = fontHeader[string[iter]];
        if(string[iter] == '\n')
        {
            vg_lite_translate(-offsetX, lineHeight, &matrix);
            offsetX = 0;
            continue;
        }

        memset(&path,0,sizeof(path));
        path.format = (vg_lite_format_t)fontData.format;
        path_data = malloc(fontData.pathSize);

        memcpy(path.bounding_box,fontData.boundingBox, 16);
        build_path(path_data,fontData.path_data,fontData.pathSize);

        path.path = path_data;
        path.path_changed = 1;
        path.path_length = fontData.pathSize;
        path.quality = VG_LITE_HIGH;

        /* Draw the path using the matrix.*/
        error = vg_lite_draw(fb, &path, VG_LITE_FILL_EVEN_ODD, &matrix, VG_LITE_BLEND_NONE, 0xFF0000FF);
        if (error) {
            printf("vg_lite_draw() returned error %d\n", error);
            cleanup();
            free(path_data);
            return -1;
        }

        free(path_data);
        vg_lite_finish();
        vg_lite_clear_path(&path);

        vg_lite_translate(fontData.dx, fontData.dy, &matrix);
        offsetX += fontData.dx;
    }

    /* Save PNG file.*/
    vg_lite_save_png("plyhs2_1.png", fb);

    /*Draw frame 2, scale a character*/
    vg_lite_identity(&matrix);
    vg_lite_translate( startX, startY, &matrix);
    vg_lite_scale(8.0, 8.0, &matrix);
    fontData = fontHeader[character];
    memset(&path,0,sizeof(path));
    path.format = (vg_lite_format_t)fontData.format;
    path_data = malloc(fontData.pathSize);

    memcpy(path.bounding_box,fontData.boundingBox, 16);
    build_path(path_data,fontData.path_data,fontData.pathSize);

    path.path = path_data;
    path.path_changed = 1;
    path.path_length = fontData.pathSize;
    path.quality = VG_LITE_HIGH;
    vg_lite_clear(fb, NULL, 0xFFFFFFFF);
    error = vg_lite_draw(fb, &path, VG_LITE_FILL_EVEN_ODD, &matrix, VG_LITE_BLEND_NONE, 0xFF0000FF);
    if (error) {
        printf("vg_lite_draw() returned error %d\n", error);
        cleanup();
        free(path_data);
        return -1;
    }

    free(path_data);
    vg_lite_finish();
    vg_lite_clear_path(&path);
    vg_lite_save_png("plyhs2_2.png", fb);

    /*Cleanup.*/
    cleanup();
    return 0;
}
