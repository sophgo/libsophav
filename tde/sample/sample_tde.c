#include <stdio.h>
#include "bmlib_runtime.h"
#include "tde.h"
#include "vg_lite.h"


static void tde_create_surface(cvi_tde_surface *surface, unsigned int colorfmt, unsigned int w, unsigned int h, unsigned int stride);

// test cases
static int sample_fill_surface(cvi_tde_surface *surface,
                        cvi_tde_rect *rect, unsigned int fill_data);
static int sample_quick_copy(cvi_tde_surface *src_surface,
                        cvi_tde_surface *dst_surface,
                        cvi_tde_rect *src_rect,
                        cvi_tde_rect *dst_rect);
static int sample_quick_resize(cvi_tde_surface *src_surface,
                        cvi_tde_surface *dst_surface,
                        cvi_tde_rect *src_rect,
                        cvi_tde_rect *dst_rect);
static int sampel_draw_line(cvi_tde_surface *dst_surface,
                        const cvi_tde_line *line,
                        unsigned int num);

int main(void)
{
    int rc = 0;
    const unsigned int surf_width = 256;
    const unsigned int surf_height = 256;
    cvi_tde_rect src_rect;
    cvi_tde_rect dst_rect;
    unsigned int fill_data = 0xFFFF00;   // yellow
    cvi_tde_surface src_surface = {0};
    cvi_tde_surface dst_surface = {0};
    unsigned char *back_ground_vir = NULL;
    unsigned char num = 0;
    cvi_tde_line lines[] = {
        {0, 64, 256, 64, 3, 0xffff},
        {0, 128, 256, 128, 3, 0xffff},
        {0, 192, 256, 192, 3, 0xff00},
        {64, 0, 64, 256, 3, 0xffff},
        {128, 0, 128, 256, 3, 0xffff},
        {192, 0, 192, 256, 3, 0xff00},
        {128, 0, 256, 128, 3, 0xff0000},
        {0, 128, 128, 256, 3, 0xff0000},
        {128, 0, 0, 128, 3, 0xff0000},
        {256, 128, 128, 256, 3, 0xff0000},
        {0, 0, 256, 256, 3, 0xff0000},
        {256, 0, 0, 256, 3, 0xff0000},
        {128, 0, 256, 192, 3, 0xff00},
        {128, 0, 256, 256, 3, 0xff00},
        {128, 0, 192, 128, 3, 0xff00},
        {128, 0, 192, 192, 3, 0xff00},
        {128, 0, 192,256, 3, 0xff00},
        {128, 0, 0, 192, 3, 0xff00},
        {128, 0, 0, 256, 3, 0xff00},
        {128, 0, 64, 128, 3, 0xff00},
        {128, 0, 64, 192, 3, 0xff00},
        {128, 0, 64, 256, 3, 0xff00},
        {0, 0, 256, 64, 3, 0xff0000},
        {0, 0, 256, 128, 3, 0xff0000},
        {256, 0, 0, 64, 3, 0xff0000},
        {256, 0, 0, 128, 3, 0xff0000},
        {192, 0, 256, 64, 3, 0xff00},
        {192, 0, 256, 128, 3, 0xff00},
        {64, 0, 0, 64, 3, 0xff00},
        {64, 0, 0, 128, 3, 0xff00}
    };

    bm_handle_t handle;
    int soc_idx = 0;
    bm_status_t ret;
    ret = bm_dev_request(&handle, soc_idx);
    if (ret != BM_SUCCESS) {
      return TDE_FAILURE;
    }

    rc = tde_open();
    if (rc != TDE_SUCCESS) {
        return TDE_FAILURE;
    }

    bm_device_mem_t* pmem = (bm_device_mem_t*)malloc(sizeof(bm_device_mem_t));
    ret = bm_malloc_device_byte_heap( handle, pmem, 0x01, surf_width * surf_height * 4 *2);
    if (ret != 0)
    {
        return TDE_FAILURE;
    }

    unsigned long long virt_addr = 0;
    ret = bm_mem_mmap_device_mem(handle, pmem, &virt_addr);
    if(ret != BM_SUCCESS){
        return BM_ERR_FAILURE;
    }

    src_surface.phys_addr = pmem->u.device.device_addr;
    back_ground_vir = (unsigned char *)virt_addr;

    if (back_ground_vir == NULL || src_surface.phys_addr == 0) {
        return TDE_FAILURE;
    }


    tde_create_surface(&src_surface, CVI_TDE_COLOR_FORMAT_ARGB8888, surf_width, surf_height, surf_width * 4);
    tde_create_surface(&dst_surface, CVI_TDE_COLOR_FORMAT_ARGB8888, surf_width, surf_height, surf_width * 4);
    dst_surface.phys_addr = src_surface.phys_addr + src_surface.stride * src_surface.height;

    src_rect.pos_x = 0;
    src_rect.pos_y = 0;
    src_rect.width = src_surface.width;
    src_rect.height = src_surface.height;

    // fill with yellow color
    if (sample_fill_surface(&src_surface, &src_rect, fill_data) == TDE_SUCCESS) {
        save_bmp("sample_tde_back_ground.bmp", back_ground_vir, src_surface.width, src_surface.height,
            src_surface.stride, src_surface.color_format);
        save_raw("sample_tde_back_ground.raw", back_ground_vir, src_surface.width, src_surface.height,
            src_surface.stride, src_surface.color_format);
    }

    // fill with blue color
    if (sample_fill_surface(&dst_surface, &src_rect, 0xFF) == TDE_SUCCESS) {
        save_bmp("sample_tde_dst_surface.bmp", back_ground_vir + src_surface.stride * src_surface.height, src_surface.width,
            src_surface.height, src_surface.stride, src_surface.color_format);
        save_raw("sample_tde_dst_surface.raw", back_ground_vir + src_surface.stride * src_surface.height, src_surface.width,
            src_surface.height, src_surface.stride, src_surface.color_format);
    }


    src_rect.pos_x = 0;
    src_rect.pos_y = 0;
    src_rect.width = src_surface.width / 2;
    src_rect.height = src_surface.height / 2;

    dst_rect.pos_x = 0;
    dst_rect.pos_y = dst_surface.height / 2;
    dst_rect.width = dst_surface.width / 2;
    dst_rect.height = dst_surface.height / 2;

    if (sample_quick_copy(&src_surface, &dst_surface, &src_rect, &dst_rect) == TDE_SUCCESS) {
        printf("quick copy success!\n");
        bm_mem_flush_device_mem(handle, pmem);
        save_bmp("sample_tde_quick_copy.bmp", back_ground_vir + src_surface.stride * src_surface.height, src_surface.width,
            src_surface.height, src_surface.stride, src_surface.color_format);
        save_raw("sample_tde_quick_copy.raw", back_ground_vir + src_surface.stride * src_surface.height, src_surface.width,
            src_surface.height, src_surface.stride, src_surface.color_format);
    }


    src_rect.pos_x = 0;
    src_rect.pos_y = 0;
    src_rect.width = 50;
    src_rect.height = 50;

    dst_rect.pos_x = 0;
    dst_rect.pos_y = 0;
    dst_rect.width = dst_surface.width - 100;
    dst_rect.height = dst_surface.height / 2;

    if (sample_quick_resize(&src_surface, &dst_surface, &src_rect, &dst_rect) == TDE_SUCCESS) {
        printf("quick resize success!\n");
        bm_mem_flush_device_mem(handle, pmem);
        save_bmp("sample_tde_quick_resize.bmp", back_ground_vir + src_surface.stride * src_surface.height, src_surface.width,
            src_surface.height, src_surface.stride, src_surface.color_format);
        save_raw("sample_tde_quick_resize.raw", back_ground_vir + src_surface.stride * src_surface.height, src_surface.width,
            src_surface.height, src_surface.stride, src_surface.color_format);
    }

    num = sizeof(lines) / sizeof(lines[0]);
    if (sampel_draw_line(&dst_surface, lines, num) == TDE_SUCCESS) {
        printf("draw line success!\n");
        bm_mem_flush_device_mem(handle, pmem);
        save_bmp("sample_draw_line.bmp", back_ground_vir + src_surface.stride * src_surface.height, src_surface.width,
            src_surface.height, src_surface.stride, src_surface.color_format);
        save_raw("sample_draw_line.raw", back_ground_vir + src_surface.stride * src_surface.height, src_surface.width,
            src_surface.height, src_surface.stride, src_surface.color_format);
    }

    ret = bm_mem_unmap_device_mem(handle,(void *)virt_addr,surf_width * surf_height * 4 *2);

    tde_close();

    bm_free_device(handle,*(pmem));
    return TDE_SUCCESS;
}

void tde_create_surface(cvi_tde_surface *surface, unsigned int colorfmt, unsigned int w, unsigned int h, unsigned int stride)
{
    surface->color_format = colorfmt;
    surface->width = w;
    surface->height = h;
    surface->stride = stride;
    surface->alpha0 = 0xff;
    surface->alpha1 = 0xff;
    surface->alpha_max_is_255 = CVI_TRUE;
    surface->support_alpha_ex_1555 = CVI_TRUE;
}

int sample_fill_surface(cvi_tde_surface *surface, cvi_tde_rect *rect, unsigned int fill_data)
{
    int ret;
    int handle = 0;
    cvi_tde_none_src none_src = {0};

    /* 1. start job */
    handle = tde_begin_job();
    if (handle == CVI_ERR_TDE_INVALID_HANDLE) {
        return TDE_FAILURE;
    }

    /* 2. do some operations to surface  */
    none_src.dst_surface = surface;
    none_src.dst_rect = rect;
    ret = tde_quick_fill(handle, &none_src, fill_data);
    if (ret < 0) {
        tde_cancel_job(handle);
        return TDE_FAILURE;
    }

    /* 3. submit job */
    ret = tde_end_job(handle, CVI_FALSE, CVI_TRUE, 1000); /* 1000 time out */
    if (ret < 0) {
        tde_cancel_job(handle);
        return TDE_FAILURE;
    }

    return TDE_SUCCESS;
}


int sample_quick_copy(cvi_tde_surface *src_surface,
                        cvi_tde_surface *dst_surface,
                        cvi_tde_rect *src_rect,
                        cvi_tde_rect *dst_rect)
{
    int ret;
    int handle = 0;
    cvi_tde_single_src single_src = {0};

    /* 1. start job */
    handle = tde_begin_job();
    if (handle == CVI_ERR_TDE_INVALID_HANDLE) {
        return TDE_FAILURE;
    }

    /* 2. do some operations to surface */
    single_src.src_surface = src_surface;
    single_src.dst_surface = dst_surface;
    single_src.src_rect = src_rect;
    single_src.dst_rect = dst_rect;
    ret = tde_quick_copy(handle, &single_src);
    if (ret < 0) {
        tde_cancel_job(handle);
        return TDE_FAILURE;
    }

    /* 5. submit job */
    ret = tde_end_job(handle, CVI_FALSE, CVI_TRUE, 1000); /* 1000 time out */
    if (ret < 0) {
        tde_cancel_job(handle);
        return TDE_FAILURE;
    }

    return TDE_SUCCESS;
}

static int sample_quick_resize(cvi_tde_surface *src_surface,
                        cvi_tde_surface *dst_surface,
                        cvi_tde_rect *src_rect,
                        cvi_tde_rect *dst_rect)
{
    int ret;
    int handle = 0;
    cvi_tde_single_src single_src = {0};

    /* 1. start job */
    handle = tde_begin_job();
    if (handle == CVI_ERR_TDE_INVALID_HANDLE) {
        return TDE_FAILURE;
    }

    /* 2. do some operations to surface */
    single_src.src_surface = src_surface;
    single_src.dst_surface = dst_surface;
    single_src.src_rect = src_rect;
    single_src.dst_rect = dst_rect;
    ret = tde_quick_resize(handle, &single_src);
    if (ret < 0) {
        tde_cancel_job(handle);
        return TDE_FAILURE;
    }

    /* 5. submit job */
    ret = tde_end_job(handle, CVI_FALSE, CVI_TRUE, 1000); /* 1000 time out */
    if (ret < 0) {
        tde_cancel_job(handle);
        return TDE_FAILURE;
    }

    return TDE_SUCCESS;
}

static int sampel_draw_line(cvi_tde_surface *dst_surface,
                        const cvi_tde_line *line,
                        unsigned int num)
{
    int ret;
    int handle = 0;

    /* 1. start job */
    handle = tde_begin_job();
    if (handle == CVI_ERR_TDE_INVALID_HANDLE) {
        return TDE_FAILURE;
    }

    /* 2. do some operations to surface */
    ret = tde_draw_line(handle, dst_surface, line, num);
    if (ret < 0) {
        tde_cancel_job(handle);
        return TDE_FAILURE;
    }

    /* 5. submit job */
    ret = tde_end_job(handle, CVI_FALSE, CVI_TRUE, 1000); /* 1000 time out */
    if (ret < 0) {
        tde_cancel_job(handle);
        return TDE_FAILURE;
    }

    return TDE_SUCCESS;}