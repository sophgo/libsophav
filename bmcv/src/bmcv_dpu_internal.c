#ifndef BM_PCIE_MODE
#include <stdio.h>
#include "bmcv_internal.h"
#include "bmcv_a2_dpu_ext.h"

#define DPU_STRIDE_ALIGN 16
#define FGS_STRIDE_ALIGN 32
#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

void dpu_read_bin(const char *path, unsigned char *data, int size)
{
    FILE *fp = fopen(path, "rb");
    if (fread((void *)data, 1, size, fp) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp);
}

bm_status_t bm_dpu_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
    bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride, bool bFgsRes)
{
    bm_status_t ret = BM_SUCCESS;
    int stride_align = (bFgsRes) ? FGS_STRIDE_ALIGN : DPU_STRIDE_ALIGN;
    bm_image_private image_private;
    ret = fill_default_image_private(&image_private, img_h, img_w, image_format, data_type);
    for(int i = 0; i < image_private.plane_num; i++)
        stride[i] = ALIGN(image_private.memory_layout[i].pitch_stride, stride_align);
    return ret;
}

bm_status_t bmcv_dpu_sgbm_disp( bm_handle_t          handle,
                                bm_image             *left_image,
                                bm_image             *right_image,
                                bm_image             *disp_image,
                                bmcv_dpu_sgbm_attrs  *dpu_attr,
                                bmcv_dpu_sgbm_mode   sgbm_mode){
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);

    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dpu get chipid error! \n");
        return ret;
    }

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_dpu_sgbm_disp_internal(handle, left_image, right_image, disp_image, dpu_attr, sgbm_mode);
            if(ret != BM_SUCCESS){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Bmcv dpu execution failed! \n");
                return ret;
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "not support! \n");
            break;
    }
    return ret;
}

bm_status_t bmcv_dpu_fgs_disp( bm_handle_t           handle,
                               bm_image             *guide_image,
                               bm_image             *smooth_image,
                               bm_image             *disp_image,
                               bmcv_dpu_fgs_attrs   *fgs_attr,
                               bmcv_dpu_fgs_mode    fgs_mode){
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);

    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dpu get chipid error! \n");
        return ret;
    }

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_dpu_fgs_disp_internal(handle, guide_image, smooth_image, disp_image, fgs_attr, fgs_mode);
            if(ret != BM_SUCCESS){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Bmcv dpu execution failed! \n");
                return ret;
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "not support! \n");
            break;
    }
    return ret;
}

bm_status_t bmcv_dpu_online_disp( bm_handle_t         handle,
                                  bm_image            *left_image,
                                  bm_image            *right_image,
                                  bm_image            *disp_image,
                                  bmcv_dpu_sgbm_attrs *dpu_attr,
                                  bmcv_dpu_fgs_attrs  *fgs_attr,
                                  bmcv_dpu_online_mode online_mode){
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);

    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dpu get chipid error! \n");
        return ret;
    }

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_dpu_online_disp_internal(handle, left_image, right_image, disp_image, dpu_attr, fgs_attr, online_mode);
            if(ret != BM_SUCCESS){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Bmcv dpu execution failed! \n");
                return ret;
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "not support! \n");
            break;
    }

    return ret;
}
#endif
