#ifndef BM_PCIE_MODE
#include <stdio.h>
#include "bmcv_internal.h"
#include "bmcv_a2_dwa_ext.h"

bm_status_t bmcv_dwa_rot(bm_handle_t          handle,
                         bm_image             input_image,
                         bm_image             output_image,
                         bmcv_rot_mode        rot_mode){
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);

    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dwa get chipid error! \n");
        return ret;
    }

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bmcv_dwa_rot_internel(handle, input_image, output_image, rot_mode);
            if(ret != BM_SUCCESS){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Bmcv dwa execution failed! \n");
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

bm_status_t bmcv_dwa_gdc(bm_handle_t          handle,
                         bm_image             input_image,
                         bm_image             output_image,
                         bmcv_gdc_attr        ldc_attr){

    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);

    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dwa get chipid error! \n");
        return ret;
    }

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bmcv_dwa_gdc_internel(handle, input_image, output_image, ldc_attr);
            if(ret != BM_SUCCESS){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Bmcv dwa execution failed! \n");
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

bm_status_t bmcv_dwa_fisheye(bm_handle_t          handle,
                             bm_image             input_image,
                             bm_image             output_image,
                             bmcv_fisheye_attr_s  fisheye_attr){
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);

    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dwa get chipid error! \n");
        return ret;
    }

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bmcv_dwa_fisheye_internel(handle, input_image, output_image, fisheye_attr);
            if(ret != BM_SUCCESS){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bmcv dwa execution failed! \n");
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

bm_status_t bmcv_dwa_affine(bm_handle_t          handle,
                            bm_image             input_image,
                            bm_image             output_image,
                            bmcv_affine_attr_s   affine_attr){
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);

    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dwa get chipid error! \n");
        return ret;
    }

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bmcv_dwa_affine_internel(handle, input_image, output_image, affine_attr);
            if(ret != BM_SUCCESS){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Bmcv dwa execution failed! \n");
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

bm_status_t bmcv_dwa_dewarp(bm_handle_t          handle,
                            bm_image             input_image,
                            bm_image             output_image,
                            bm_device_mem_t      grid_info){
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);

    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dwa get chipid error! \n");
        return ret;
    }

    switch(chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bmcv_dwa_dewarp_internel(handle, input_image, output_image, grid_info);
            if(ret != BM_SUCCESS){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Bmcv dwa execution failed! \n");
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