#ifndef BM_PCIE_MODE
#include <stdio.h>
#include <stdlib.h>
#include "bmcv_a2_ive_ext.h"
#include "bmcv_internal.h"

#define BITMAP_1BIT 1
#define BITMAP_8BIT 0

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))
#define IVE_STRIDE_ALIGN 16

enum DIRECTION {
    VERTICAL = 0,
    HORIZON = 1,
    SLASH = 2,
    BACK_SLASH = 3,
    ZERO = 4,
};

// read image file with original width and height
void bm_ive_read_bin(bm_image src, const char *input_name)
{
    bm_read_compact_bin(src, input_name);
    return;
}

// write image file with original width and height
void bm_ive_write_bin(bm_image dst, const char *output_name)
{
    bm_write_compact_bin(dst, output_name);
    return;
}

bm_status_t bm_ive_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
    bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride)
{
    bm_status_t ret = BM_SUCCESS;
    bm_image_private image_private;
    ret = fill_default_image_private(&image_private, img_h, img_w, image_format, data_type);
    for(int i = 0; i < image_private.plane_num; i++)
        stride[i] = ALIGN(image_private.memory_layout[i].pitch_stride, IVE_STRIDE_ALIGN);
    return ret;
}

bm_status_t bmcv_ive_and(
    bm_handle_t          handle,
    bm_image             input1,
    bm_image             input2,
    bm_image             output)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid)
    {
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_and(handle, input1, input2, output);
            break;

        default:
            ret = BM_NOT_SUPPORTED;
            break;
        }

    return ret;
}

bm_status_t bmcv_ive_add(
    bm_handle_t          handle,
    bm_image             input1,
    bm_image             input2,
    bm_image             output,
    bmcv_ive_add_attr    attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid)
    {
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_add(handle, input1, input2, output, attr);
            break;

        default:
            ret = BM_NOT_SUPPORTED;
            break;
        }

    return ret;
}

bm_status_t bmcv_ive_or(
    bm_handle_t          handle,
    bm_image             input1,
    bm_image             input2,
    bm_image             output)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid)
    {
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_or(handle, input1, input2, output);
            break;

        default:
            ret = BM_NOT_SUPPORTED;
            break;
        }

    return ret;
}

bm_status_t bmcv_ive_xor(
    bm_handle_t          handle,
    bm_image             input1,
    bm_image             input2,
    bm_image             output)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid)
    {
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_xor(handle, input1, input2, output);
            break;

        default:
            ret = BM_NOT_SUPPORTED;
            break;
        }

    return ret;
}

bm_status_t bmcv_ive_sub(
    bm_handle_t          handle,
    bm_image             input1,
    bm_image             input2,
    bm_image             output,
    bmcv_ive_sub_attr      attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid)
    {
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_sub(handle, input1, input2, output, attr);
            break;

        default:
            ret = BM_NOT_SUPPORTED;
            break;
        }

    return ret;
}

bm_status_t bmcv_ive_thresh(
    bm_handle_t               handle,
    bm_image                  input,
    bm_image                  output,
    bmcv_ive_thresh_mode      thresh_mode,
    bmcv_ive_thresh_attr      attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid)
    {
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_thresh(handle, input, output, thresh_mode, attr);
            break;

        default:
            ret = BM_NOT_SUPPORTED;
            break;
        }

    return ret;
}
bm_status_t bmcv_ive_dma_set(
    bm_handle_t                      handle,
    bm_image                         image,
    bmcv_ive_dma_set_mode            dma_set_mode,
    unsigned long long               val)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid)
    {
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_dma_set(handle, image, dma_set_mode, val);
            break;

        default:
            ret = BM_NOT_SUPPORTED;
            break;
        }

    return ret;
}

bm_status_t bmcv_ive_dma(
    bm_handle_t                      handle,
    bm_image                         input,
    bm_image                         output,
    bmcv_ive_dma_mode                dma_mode,
    bmcv_ive_interval_dma_attr *     attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid)
    {
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_dma(handle, input, output, dma_mode, attr);
            break;

        default:
            ret = BM_NOT_SUPPORTED;
            break;
        }

    return ret;
}

bm_status_t bmcv_ive_map(
    bm_handle_t             handle,
    bm_image                input,
    bm_image                output,
    bm_device_mem_t         map_table)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid)
    {
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_map(handle, input, output, map_table);
            break;

        default:
            ret = BM_NOT_SUPPORTED;
            break;
        }

    return ret;
}

bm_status_t bmcv_ive_hist(
        bm_handle_t          handle,
        bm_image             input,
        bm_device_mem_t      output)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
    if (output.size != 1024){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "output size alloc less 1024 %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
    }
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_hist(handle, input, output);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}

bm_status_t bmcv_ive_integ(
        bm_handle_t              handle,
        bm_image                 input,
        bm_device_mem_t          output,
        bmcv_ive_integ_ctrl_s    integ_attr){
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_integ(handle, input, output, integ_attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}

bm_status_t bmcv_ive_ncc(
        bm_handle_t          handle,
        bm_image             input1,
        bm_image             input2,
        bm_device_mem_t      output){
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_ncc(handle, input1, input2, output);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}

bm_status_t bmcv_ive_ord_stat_filter(
        bm_handle_t                   handle,
        bm_image                      input,
        bm_image                      output,
        bmcv_ive_ord_stat_filter_mode mode)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_ord_stat_filter(handle, &input, &output, mode);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}

bm_status_t bmcv_ive_lbp(
        bm_handle_t              handle,
        bm_image                 input,
        bm_image                 output,
        bmcv_ive_lbp_ctrl_attr   lbp_attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_lbp(handle, &input, &output, &lbp_attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}

bm_status_t bmcv_ive_dilate(
        bm_handle_t           handle,
        bm_image              input,
        bm_image              output,
        unsigned char         dilate_mask[25])
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_dilate(handle, &input, &output, dilate_mask);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}

bm_status_t bmcv_ive_erode(
        bm_handle_t           handle,
        bm_image              input,
        bm_image              output,
        unsigned char         erode_mask[25])
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_erode(handle, &input, &output, erode_mask);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}

bm_status_t bmcv_ive_mag_and_ang(
        bm_handle_t                   handle,
        bm_image  *                   input,
        bm_image  *                   mag_output,
        bm_image  *                   ang_output,
        bmcv_ive_mag_and_ang_ctrl     attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_mag_and_ang(handle, input, mag_output, ang_output, &attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}

bm_status_t bmcv_ive_sobel(
        bm_handle_t            handle,
        bm_image *             input,
        bm_image *             output_h,
        bm_image *             output_v,
        bmcv_ive_sobel_ctrl    sobel_attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_sobel(handle, input, output_h, output_v, &sobel_attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}

bm_status_t bmcv_ive_norm_grad(
        bm_handle_t              handle,
        bm_image *               input,
        bm_image *               output_h,
        bm_image *               output_v,
        bm_image *               output_hv,
        bmcv_ive_normgrad_ctrl   normgrad_attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_normgrad(handle, input, output_h, output_v, output_hv, &normgrad_attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}

bm_status_t bmcv_ive_gmm(
        bm_handle_t            handle,
        bm_image               input,
        bm_image               output_fg,
        bm_image               output_bg,
        bm_device_mem_t        output_model,
        bmcv_ive_gmm_ctrl      gmm_attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_gmm(handle, &input, &output_fg, &output_bg, &output_model, &gmm_attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}

bm_status_t bmcv_ive_gmm2(
        bm_handle_t            handle,
        bm_image *             input,
        bm_image *             input_factor,
        bm_image *             output_fg,
        bm_image *             output_bg,
        bm_image *             output_match_model_info,
        bm_device_mem_t        output_model,
        bmcv_ive_gmm2_ctrl     gmm2_attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_gmm2(handle, input, input_factor, output_fg,
                            output_bg, output_match_model_info, &output_model, &gmm2_attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}

void boundary_check(bm_ive_point_u16 *point, int width, int height)
{
    if ((short)point->u16_x < 0) {
        point->u16_x = 0;
    }
    if ((short)point->u16_y < 0) {
        point->u16_y = 0;
    }
    if (point->u16_x >= width) {
        point->u16_x = width - 1;
    }
    if (point->u16_y >= height) {
        point->u16_y = height - 1;
    }
}

bm_status_t bmcv_image_ive_canny_edge(bm_handle_t handle, bm_device_mem_t *hys_edge,
         unsigned char* dst_edge, int width, int height, int stride)
{
    bm_status_t ret = BM_SUCCESS;
    bm_ive_point_u16 p1, p2, p;

    unsigned char *visited = malloc(height * stride * sizeof(unsigned char));
    unsigned char *pdst = malloc(height * stride * sizeof(unsigned char));
    unsigned char* p_edge_map = malloc(stride * height * sizeof(unsigned char));
    memset(p_edge_map, 0, stride * height * sizeof(unsigned char));
    memset(visited, 0, stride * height * sizeof(unsigned char));
    memset(pdst, 0, stride * height * sizeof(unsigned char));

    ret = bm_memcpy_d2s(handle, p_edge_map, *hys_edge);
    if(ret != BM_SUCCESS){
        printf("dst_edge d2s error \n");
        free(visited);
        free(pdst);
        free(p_edge_map);
        return ret;
    }

    for(int i = 0; i < height; i++){
        for(int j = 0; j < stride; j++){
            int index = i * stride + j;
            if(p_edge_map[index] == 2)
                pdst[index] = 255;
            else
                pdst[index] = 0;
        }

    }

    for(int y = 1; y < height - 1; y++){
        for(int x = 1; x < width - 1; x++){
            // if pixel not in upper or had visited, skip
            if(p_edge_map[y * stride + x] != 2 ||
               visited[y * stride + x >0])
               continue;
            p.u16_x = x;
            p.u16_y = y;
            visited[p.u16_y * stride + p.u16_x] = 255;
            pdst[p.u16_y * stride + p.u16_x] = 255;

            for(unsigned char dir = 0; dir < 4; dir++){
                if(dir == VERTICAL){
                    p1.u16_x = p.u16_x + 1;
                    p1.u16_y = p.u16_y;
                    p2.u16_x = p.u16_x - 1;
                    p2.u16_y = p.u16_y;
                } else if(dir == HORIZON){
                    p1.u16_x = p.u16_x;
                    p1.u16_y = p.u16_y + 1;
                    p2.u16_x = p.u16_x;
                    p2.u16_y = p.u16_y - 1;
                } else if (dir == SLASH){
                    p1.u16_x = p.u16_x + 1;
                    p1.u16_y = p.u16_y - 1;
                    p2.u16_x = p.u16_x - 1;
                    p2.u16_y = p.u16_y + 1;
                } else if (dir == BACK_SLASH){
                    p1.u16_x = p.u16_x + 1;
                    p1.u16_y = p.u16_y + 1;
                    p2.u16_x = p.u16_x - 1;
                    p2.u16_y = p.u16_y - 1;
                } else {
                    p1.u16_x = p.u16_x;
                    p1.u16_y = p.u16_y;
                    p2.u16_x = p.u16_x;
                    p2.u16_y = p.u16_y;
                }
                boundary_check(&p1, width, height);
                boundary_check(&p2, width, height);

                if (p_edge_map[p1.u16_y * stride + p1.u16_x] == 0 &&
                    visited[p1.u16_y * stride + p1.u16_x] == 0) {
                    visited[p1.u16_y * stride + p1.u16_x] = 255;
                    pdst[p1.u16_y * stride + p1.u16_x] = 255;
                }
                if (p_edge_map[p2.u16_y * stride + p2.u16_x] == 0 &&
                    visited[p2.u16_y * stride + p2.u16_x] == 0) {
                    visited[p2.u16_y * stride + p2.u16_x] = 255;
                    pdst[p2.u16_y * stride + p2.u16_x] = 255;
                }
            }
        }
    }

    memcpy(dst_edge, pdst, stride * height * sizeof(unsigned char));

    free(visited);
    free(pdst);
    free(p_edge_map);
    return ret;
}

bm_status_t bmcv_ive_canny_internal(
    bm_handle_t                    handle,
    bm_image *                     input,
    bm_device_mem_t                output_edge,
    bmcv_ive_canny_hys_edge_ctrl * canny_hys_edge_attr)
{
    bm_image tmp_edge;
    bm_device_mem_t tmp_stack;
    int width = input->width;
    int height = input->height;
    int stack_len = width * height * sizeof(bm_ive_point_u16) + sizeof(bm_ive_canny_stack_size);
    int stride[4];
    bm_image_get_stride(*input, stride);

    unsigned char *stack_res = malloc(stack_len);
    memset(stack_res, 0, stack_len);

    bm_image_create(handle, height, width, input->image_format,
                                   DATA_TYPE_EXT_1N_BYTE, &tmp_edge, stride);

    bm_status_t ret = bm_image_alloc_dev_mem(tmp_edge, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("tmp_edge bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        return ret;
    }

    ret = bm_malloc_device_byte(handle, &tmp_stack, stack_len);
    if (ret != BM_SUCCESS) {
        printf("tmp_stack bm_malloc_device_byte failed. ret = %d\n", ret);
        return 0;
    }

    ret = bm_ive_canny_hsy_edge(handle, input, &tmp_edge, &tmp_stack, canny_hys_edge_attr);
    if(ret != BM_SUCCESS){
        printf("bm_ive_canny_hsy_edge failed , ret is %d \n", ret);
        return ret;
    }

    ret = bmcv_image_ive_canny_edge(handle, tmp_edge.image_private->data,
                                    (unsigned char*)output_edge.u.system.system_addr,
                                    tmp_edge.width, tmp_edge.height, stride[0]);
    return ret;
}

bm_status_t bmcv_ive_canny(
        bm_handle_t                    handle,
        bm_image                       input,
        bm_device_mem_t                output_edge,
        bmcv_ive_canny_hys_edge_ctrl   canny_hys_edge_attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bmcv_ive_canny_internal(handle, &input, output_edge, &canny_hys_edge_attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}


bm_status_t bmcv_ive_filter(
        bm_handle_t                  handle,
        bm_image                     input,
        bm_image                     output,
        bmcv_ive_filter_ctrl         filter_attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_filter(handle, input, output, filter_attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}

bm_status_t bmcv_ive_csc(
        bm_handle_t     handle,
        bm_image        input,
        bm_image        output,
        csc_type_t      csc_type)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_csc(handle, &input, &output, csc_type);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}

bm_status_t bmcv_ive_resize(
    bm_handle_t               handle,
    bm_image                  input,
    bm_image                  output,
    bmcv_resize_algorithm     resize_mode)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_resize(handle, &input, &output, resize_mode);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}

bm_status_t bmcv_ive_stcandicorner(
        bm_handle_t                   handle,
        bm_image                      input,
        bm_image                      output,
        bmcv_ive_stcandicorner_attr   stcandicorner_attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_stCandiCorner(handle, &input, &output, &stcandicorner_attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}

bm_status_t bmcv_ive_gradfg(
    bm_handle_t             handle,
    bm_image                input_bgdiff_fg,
    bm_image                input_fggrad,
    bm_image                input_bggrad,
    bm_image                output_gradfg,
    bmcv_ive_gradfg_attr    gradfg_attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_gradFg(handle, &input_bgdiff_fg,
                    &input_fggrad, &input_bggrad, &output_gradfg, &gradfg_attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}

bm_status_t bmcv_image_ive_sad_u8(bm_handle_t handle, bm_image dst_){
    bm_status_t ret;
    bm_image dst_sad_u8;
    int width = dst_.width;
    int height = dst_.height;
    bm_image_format_ext fmt = dst_.image_format;
    bm_image_data_format_ext dtype = dst_.data_type;
    int stride[4];
    bm_image_get_stride(dst_, stride);

    bm_image_create(handle, height, width, fmt, dtype, &dst_sad_u8, stride);

    ret = bm_image_alloc_dev_mem(dst_sad_u8, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("dst_sad_u8 bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        return ret;
    }

    bmcv_ive_interval_dma_attr dma_attr;
    memset(&dma_attr, 0, sizeof(bmcv_ive_interval_dma_attr));
    dma_attr.elem_size = 1;
    dma_attr.hor_seg_size = 2;
    dma_attr.ver_seg_rows = 1;

    dst_sad_u8.image_private->memory_layout->pitch_stride = stride[0] / 2 ;

    ret = bmcv_ive_dma(handle, dst_, dst_sad_u8, 1, &dma_attr);
    if(ret != BM_SUCCESS){
        printf("dst_sad_u8 bmcv_ive_dma failed. ret = %d\n", ret);
        bm_image_destroy(&dst_sad_u8);
        return ret;
    }

    dst_.image_private->data->u.device.device_addr = dst_sad_u8.image_private->data->u.device.device_addr;
    return ret;
}

bm_status_t bmcv_ive_sad(
        bm_handle_t                handle,
        bm_image *                 input,
        bm_image *                 output_sad,
        bm_image *                 output_thr,
        bmcv_ive_sad_attr *        sad_attr,
        bmcv_ive_sad_thresh_attr*  thresh_attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_sad(handle, input, output_sad, output_thr, sad_attr, thresh_attr);
            if(output_sad->data_type == DATA_TYPE_EXT_1N_BYTE)
                ret = bmcv_image_ive_sad_u8(handle, *output_sad);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}

bm_status_t bmcv_ive_match_bgmodel(
        bm_handle_t                   handle,
        bm_image                      cur_img,
        bm_image                      bgmodel_img,
        bm_image                      fgflag_img,
        bm_image                      diff_fg_img,
        bm_device_mem_t               stat_data_mem,
        bmcv_ive_match_bgmodel_attr   attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_match_bgmodel(handle, &cur_img, &bgmodel_img,
                    &fgflag_img, &diff_fg_img, &stat_data_mem, &attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}

bm_status_t bmcv_ive_update_bgmodel(
    bm_handle_t                    handle,
    bm_image  *                    cur_img,
    bm_image  *                    bgmodel_img,
    bm_image  *                    fgflag_img,
    bm_image  *                    bg_img,
    bm_image  *                    chgsta_img,
    bm_device_mem_t                stat_data_mem,
    bmcv_ive_update_bgmodel_attr   attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_update_bgmodel(handle, cur_img, bgmodel_img,
                           fgflag_img, bg_img, chgsta_img, &stat_data_mem, &attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}

bm_status_t bmcv_ive_ccl(
        bm_handle_t          handle,
        bm_image             src_dst_image,
        bm_device_mem_t      ccblob_output,
        bmcv_ive_ccl_attr    ccl_attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_ccl(handle, &src_dst_image, &ccblob_output, &ccl_attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}

bm_status_t bmcv_ive_bernsen(
    bm_handle_t           handle,
    bm_image              input,
    bm_image              output,
    bmcv_ive_bernsen_attr attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_bernsen(handle, input, output, attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}

bm_status_t bmcv_ive_filter_and_csc(
    bm_handle_t             handle,
    bm_image                input,
    bm_image                output,
    bmcv_ive_filter_ctrl    filter_attr,
    csc_type_t              csc_type)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_filterAndCsc(handle, input, output, filter_attr, csc_type);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}

bm_status_t bmcv_ive_16bit_to_8bit(
    bm_handle_t                 handle,
    bm_image                    input,
    bm_image                    output,
    bmcv_ive_16bit_to_8bit_attr attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_16bit_to_8bit(handle, input, output, attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}

bm_status_t bmcv_ive_frame_diff_motion(
    bm_handle_t                     handle,
    bm_image                        input1,
    bm_image                        input2,
    bm_image                        output,
    bmcv_ive_frame_diff_motion_attr attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned chipid = BM1688;
#ifndef _FPGA
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
        return ret;
#endif
    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_ive_frame_diff_motion(handle, input1, input2, output, attr);
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}
#endif