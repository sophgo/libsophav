#include <memory.h>
#include "bmcv_internal.h"
#include "bmcv_common.h"
#include <math.h>
#include "bmcv_api_ext_c.h"
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <fontdata.h>
#include <langinfo.h>

#define ALIGN_TO(size, align) (((size) + (align) - 1) & ~((align) - 1))
#define SATURATE(a, s, e) ((a) > (e) ? (e) : ((a) < (s) ? (s) : (a)))
#define IS_CS_YUV(a) (a == FORMAT_NV12 || a == FORMAT_NV21 || a == FORMAT_NV16 \
                    || a == FORMAT_NV61 || a == FORMAT_NV24 || a == FORMAT_YUV420P \
                    || a == FORMAT_YUV422P || a == FORMAT_YUV444P)

typedef long long int64;
typedef unsigned char uchar;

typedef struct {
    uint8_t b; // Blue
    uint8_t g; // Green
    uint8_t r; // Red
    uint8_t a; // Alpha
} ARGB8888;

/*
 * Segmented layout of the 24x24 bitmap font bmcv_test_res_2424_unicode_1.
 * Each glyph is 24 rows x 3 bytes = 72 bytes, MSB-first per row,
 * same convention as the ASCII font 1624_ez.
 *
 *   [0x000000] U+3400-U+9FA5  x27558  1984176 B  Han ideographs (legacy data, bytes unchanged)
 *   [0x1E46B0] U+3000-U+303F  x   64    4608 B  CJK punctuation
 *   [0x1E58B0] U+3040-U+30FF  x  192   13824 B  Hiragana / Katakana
 *   [0x1E8EB0] U+AC00-U+D7A3  x11172  804384 B  Hangul syllables
 *   [0x2AD4D0] U+FF00-U+FFEF  x  240   17280 B  Fullwidth forms
 *
 * The table is ordered by ascending offset, matching the physical font order;
 * a linear scan is fast enough (at most 4 comparisons).
 */
#define CJK_GLYPH_BYTES 72
#define HAN_START       0x3400
#define HAN_END         0x9FA5

typedef struct {
    unsigned int start;
    unsigned int count;
    unsigned int offset;
} cjk_range_t;

static const cjk_range_t g_cjk_ranges[] = {
    { 0x3000,   64, 1984176 },   /* CJK punctuation */
    { 0x3040,  192, 1988784 },   /* Hiragana / Katakana */
    { 0xAC00, 11172, 2002608 },  /* Hangul syllables */
    { 0xFF00,  240, 2806992 },   /* Fullwidth forms */
};
#define CJK_RANGE_NUM (sizeof(g_cjk_ranges) / sizeof(g_cjk_ranges[0]))
#define CJK_INVALID_OFFSET ((unsigned int)-1)

/*
 * Return the byte offset of the 24x24 glyph for a codepoint.
 * CJK_INVALID_OFFSET means the font has no glyph for it.
 */
static inline unsigned int cjk_glyph_offset(unsigned int cp)
{
    unsigned int k;

    if (cp >= HAN_START && cp <= HAN_END) {
        return (cp - HAN_START) * CJK_GLYPH_BYTES;
    }

    for (k = 0; k < CJK_RANGE_NUM; ++k) {
        const cjk_range_t* r = &g_cjk_ranges[k];
        if (cp >= r->start && cp < r->start + r->count) {
            return r->offset + (cp - r->start) * CJK_GLYPH_BYTES;
        }
    }

    return CJK_INVALID_OFFSET;
}

static void paint_mat(unsigned char* font_buff, unsigned long offset, int datasize, int stride,
    unsigned char* vir_addr, bmcv_color_t color, unsigned char fontScale, unsigned char is_ascii){

    unsigned char *buff = font_buff + offset;
    unsigned char *mat = vir_addr;
    unsigned char bit = 3;
    int i, j, k, sw, sh;
    if(is_ascii)
        bit = 2; //16bit for ascii
    for (i = 0; i < 24; i++)
        for (j = 0; j < bit; j++)
            for (k = 0; k < 8; k++)
                for(sw = 0; sw < fontScale; sw++)
                    for(sh = 0; sh < fontScale; sh++)
                        if (buff[i * bit + j] & (0x80 >> k)) {
                            if (datasize == 4) {
                                mat[(i * fontScale + sh) * stride + ((j * 8 + k) * fontScale + sw) * 4] = color.b;
                                mat[(i * fontScale + sh) * stride + ((j * 8 + k) * fontScale + sw) * 4 + 1] = color.g;
                                mat[(i * fontScale + sh) * stride + ((j * 8 + k) * fontScale + sw) * 4 + 2] = color.r;
                                mat[(i * fontScale + sh) * stride + ((j * 8 + k) * fontScale + sw) * 4 + 3] = 255;
                            } else
                                mat[(i * fontScale + sh) * stride + (j * 8 + k) * fontScale + sw] = 255;
                        }
    return;
}

static void bilinear_argb8888(
    const ARGB8888* src, int src_w, int src_h,
    ARGB8888* dst, int dst_w, int dst_h, bmcv_color_t color)
{
    float x_step = (float)src_w / dst_w;
    float y_step = (float)src_h / dst_h;

    float inv_x_step = 1.0f / x_step;
    float inv_y_step = 1.0f / y_step;

    for (int y = 0; y < dst_h; ++y) {
        float src_y = y * y_step;
        int y1 = (int)src_y;
        int y2 = (y1 < src_h - 1) ? y1 + 1 : y1;
        float dy = src_y - y1;

        for (int x = 0; x < dst_w; ++x) {
            float src_x = x * x_step;
            int x1 = (int)src_x;
            int x2 = (x1 < src_w - 1) ? x1 + 1 : x1;
            float dx = src_x - x1;

            ARGB8888 p11 = src[y1 * src_w + x1];
            ARGB8888 p21 = src[y1 * src_w + x2];
            ARGB8888 p12 = src[y2 * src_w + x1];
            ARGB8888 p22 = src[y2 * src_w + x2];

            float w11 = (1 - dx) * (1 - dy) * inv_x_step * inv_y_step;
            float w21 = dx * (1 - dy) * inv_x_step * inv_y_step;
            float w12 = (1 - dx) * dy * inv_x_step * inv_y_step;
            float w22 = dx * dy * inv_x_step * inv_y_step;

            float a = 0;
            a += p11.a * w11 + p21.a * w21 + p12.a * w12 + p22.a * w22;

            dst[y * dst_w + x] = (ARGB8888){
                .a = (uint8_t)fminf(fmaxf(a, 0), 255),
                .r = color.r,
                .g = color.g,
                .b = color.b
            };
        }
    }
}

static bm_status_t resize_watermark(bm_handle_t handle, bm_image *out, bm_image *in,
    float fontscale, unsigned long long in_vaddr, bmcv_color_t color) {

    short resize_w, resize_h;
    bm_device_mem_t pmem;
    unsigned long long virt_addr = 0;
    bm_status_t ret = BM_SUCCESS;

    resize_w = ALIGN_TO((short)(in->width * fontscale), 4);
    resize_h = in->height * fontscale;

    ret = bm_image_create(handle, resize_h, resize_w, FORMAT_ARGB_PACKED, DATA_TYPE_EXT_1N_BYTE, out, NULL);
    if (ret != BM_SUCCESS)
        return ret;

    ret = bm_image_dev_mem_alloc(out[0], BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS)
        return ret;

    ret = bm_image_get_device_mem(out[0], &pmem);
    if (ret != BM_SUCCESS)
        goto fail;

#ifndef BM_PCIE_MODE
    ret = bm_mem_mmap_device_mem(handle, &pmem, &virt_addr);
    if (ret != BM_SUCCESS) {
        printf("resize_watermark mmap fail, paddr(0x%lx)\n", pmem.u.device.device_addr);
        goto fail;
    }
#else
    virt_addr = (unsigned long long)malloc(pmem.size);
#endif

    bilinear_argb8888((const ARGB8888*)in_vaddr, in->image_private->memory_layout->pitch_stride/4,
        in->height, (ARGB8888*)virt_addr, resize_w, resize_h, color);

#ifndef BM_PCIE_MODE
    ret = bm_mem_flush_device_mem(handle, &pmem);
    if (ret != BM_SUCCESS)
        goto fail;

    ret = bm_mem_unmap_device_mem(handle, (void *)virt_addr, resize_w * resize_h * 4);
    if (ret != BM_SUCCESS)
        goto fail;
#else
    ret = bm_memcpy_s2d(handle, pmem, (void *)virt_addr);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d fail, vaddr(0x%llx)\n", virt_addr);
        goto fail;
    }
#endif
    return ret;

fail:
    bm_image_destroy(out);
#ifdef BM_PCIE_MODE
    if (virt_addr)
        free((void *)virt_addr);
#endif
    return ret;
}

static bool is_integer_fp32(float x) {
    return fabsf(x - roundf(x)) < 1e-6f;
}

static bool determine_environment(void) {
    static bool is_utf8 = 0;
    char* codeset;
    if (!is_utf8) {
        setlocale(LC_CTYPE, "");
        codeset = nl_langinfo(CODESET);
        if (!codeset) {
            printf("nl_langinfo(CODESET) returned null\n");
            return 1;
        }

        is_utf8 = strcasecmp(codeset, "UTF-8") == 0 ||
            strcasecmp(codeset, "UTF8") == 0;
    }
    if (!is_utf8)
        printf("Current codeset: %s, please set it to UTF-8\n", codeset);
    return is_utf8;
}

bm_status_t bmcv_gen_text_watermark(
    bm_handle_t handle,
    const wchar_t* hexcode,
    bmcv_color_t color,
    float fontscale,
    bm_image_format_ext format,
    bm_image *output){

    bm_status_t ret = BM_SUCCESS;
    int hz_num = 0, en_num = 0, stride_w, output_w, datasize = 1;
    unsigned long offset; int idx = 0;
    unsigned long long virt_addr = 0;
    unsigned char fontscale_u8 = (unsigned char)fontscale;
    unsigned char *ASCII = bmcv_test_res_1624_ez;
    unsigned char *HZK = bmcv_test_res_2424_unicode_1;
    unsigned char need_resize =
        ((!is_integer_fp32(fontscale)) && fontscale > 0 && fontscale < 10 && format == FORMAT_ARGB_PACKED);
    bm_device_mem_t pmem;
    bm_image resize_bmimg;

    if (!determine_environment())
        return BM_ERR_PARAM;

    if (format != FORMAT_GRAY && format != FORMAT_ARGB_PACKED) {
        printf("format(%d) is not supported\n", format);
        return BM_ERR_PARAM;
    } else if (format == FORMAT_ARGB_PACKED)
        datasize = 4;

    fontscale_u8 = fontscale_u8 > 10 ? 10 : fontscale_u8;
    fontscale_u8 = fontscale_u8 < 1 ? 1 : fontscale_u8;
    for (int m = 0; hexcode[m] != 0; m++){
        if (cjk_glyph_offset((unsigned int)hexcode[m]) != CJK_INVALID_OFFSET)
            hz_num++;
        else
            en_num++;   /* all non-CJK chars are laid out with the ASCII width of 11 */
    }

    output_w = ALIGN_TO((hz_num * 24 + en_num * 11) * fontscale_u8, 4);

    if (output_w <= 0) {
        printf("hexcode(%ls) is null, output_w(%d)\n", hexcode, output_w);
        return BM_ERR_PARAM;
    }

    ret = bm_image_create(handle, 24 * fontscale_u8, output_w, format, DATA_TYPE_EXT_1N_BYTE, output, NULL);
    if (ret != BM_SUCCESS)
        return ret;

    stride_w = output->image_private->memory_layout[0].pitch_stride;
    ret = bm_image_dev_mem_alloc(output[0], BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS)
        goto fail;
    ret = bm_image_get_device_mem(output[0], &pmem);
    if (ret != BM_SUCCESS)
        goto fail;
#ifndef BM_PCIE_MODE
    ret = bm_mem_mmap_device_mem(handle, &pmem, &virt_addr);
    if (ret != BM_SUCCESS) {
        printf("bm_mem_mmap_device_mem fail, paddr(0x%lx)\n", pmem.u.device.device_addr);
        goto fail;
    }
#else
    virt_addr = (unsigned long long)malloc(pmem.size);
#endif
    memset((void*)virt_addr, 0, 24 * fontscale_u8 * stride_w);

    for (int m = 0; m < hz_num + en_num; m++){
        unsigned int cp = (unsigned int)hexcode[m];
        offset = cjk_glyph_offset(cp);
        if (offset != CJK_INVALID_OFFSET){
            /* 24x24 font (72 B per glyph): Han / kana / Hangul / fullwidth */
            if (idx + 24 * fontscale_u8 > output_w)
                break;  /* defensive truncation to avoid writing out of bounds */
            paint_mat(HZK, offset, datasize, stride_w, (unsigned char*)virt_addr + idx * datasize,
                color, fontscale_u8, false);
            idx += 24 * fontscale_u8;
        } else {
            /* 16x24 font (48 B per glyph), ASCII starts at 32; fall back to '?' when absent */
            if (cp < 32 || cp > 127)
                cp = '?';
            if (idx + 11 * fontscale_u8 > output_w)
                break;
            offset = (cp - 32) * 48;
            paint_mat(ASCII, offset, datasize, stride_w, (unsigned char*)virt_addr + idx * datasize,
                color, fontscale_u8, true);
            idx += 11 * fontscale_u8;
        }
    }
    output->width = idx;

    if (need_resize) {
        ret = resize_watermark(handle, &resize_bmimg, output, fontscale / (float)fontscale_u8, virt_addr, color);
        if (ret != BM_SUCCESS)
            goto fail;
    }

#ifndef BM_PCIE_MODE
    if (!need_resize) {
        ret = bm_mem_flush_device_mem(handle, &pmem);
        if (ret != BM_SUCCESS)
            goto fail;
    }
    ret = bm_mem_unmap_device_mem(handle, (void *)virt_addr, stride_w * 24 * fontscale_u8);
    if (ret != BM_SUCCESS) {
        printf("bm_mem_unmap_device_mem fail, vaddr(0x%llx)\n", virt_addr);
        goto fail;
    }
#else
    ret = bm_memcpy_s2d(handle, pmem, (void *)virt_addr);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d fail, vaddr(0x%llx)\n", virt_addr);
        goto fail;
    }
#endif

    if (need_resize) {
        bm_image_destroy(output);
        output[0] = resize_bmimg;
    }
fail:
    if (ret != BM_SUCCESS)
        bm_image_destroy(output);
#ifdef BM_PCIE_MODE
    if (virt_addr)
        free((void *)virt_addr);
#endif
    return ret;
}

static bm_status_t bmcv_overlay_put_text(bm_handle_t handle, bm_image image, const wchar_t* hexcode, bmcv_point_t org,
                                bmcv_color_t color, float fontScale){
    bm_status_t ret = BM_SUCCESS;
    bm_image watermark;
    ret = bmcv_gen_text_watermark(handle, hexcode, color, fontScale, FORMAT_ARGB_PACKED, &watermark);
    if (ret != BM_SUCCESS)
        return ret;

    bmcv_rect_t rect = {.start_x = org.x, .start_y = org.y, .crop_w = watermark.width, .crop_h = watermark.height};
    ret = bmcv_image_overlay(handle, image, 1, &rect, &watermark);

    bm_image_destroy(&watermark);
    return ret;
}

bm_status_t bmcv_image_put_text(bm_handle_t handle, bm_image image, const char* text, bmcv_point_t org,
                                bmcv_color_t color, float fontScale, int thickness)
{
    bm_status_t ret = BM_SUCCESS;
    size_t len = 0;
    wchar_t* wideStr = NULL;

    if (text == NULL) {
        bmlib_log("PUT_TEXT", BMLIB_LOG_ERROR, "text is NULL!\r\n");
        return BM_ERR_PARAM;
    }

    setlocale(LC_ALL, "");
    len = mbstowcs(NULL, text, 0);
    if (len == (size_t)-1) {
        bmlib_log("PUT_TEXT", BMLIB_LOG_ERROR, "text is not a valid multibyte string\r\n");
        return BM_ERR_PARAM;
    }
    wideStr = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
    if (wideStr == NULL) {
        bmlib_log("PUT_TEXT", BMLIB_LOG_ERROR, "malloc failed\r\n");
        return BM_ERR_NOMEM;
    }
    mbstowcs(wideStr, text, len + 1);
    ret = bmcv_overlay_put_text(handle, image, wideStr, org, color, fontScale);
    free(wideStr);
    return ret;
}