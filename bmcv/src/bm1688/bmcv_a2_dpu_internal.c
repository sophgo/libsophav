#include <memory.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bmcv_internal.h"
#include "bmcv_a2_dpu_internal.h"
#ifdef __linux__
#include <sys/ioctl.h>
#endif

#define S32MILLISEC 4000
// DPU_GRP_ATTR_S dpu_grp;

bm_status_t bm_image_to_frame(bm_image *image, PIXEL_FORMAT_E enPixelFormat, VIDEO_FRAME_INFO_S *stVideoFrame){
    stVideoFrame->stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    stVideoFrame->stVFrame.enPixelFormat = enPixelFormat;
    stVideoFrame->stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    stVideoFrame->stVFrame.enColorGamut = COLOR_GAMUT_BT709;
    stVideoFrame->stVFrame.u32Width = image->width;
    stVideoFrame->stVFrame.u32Height = image->height;
    stVideoFrame->stVFrame.u32Stride[0] =  image->image_private->memory_layout[0].pitch_stride;
    stVideoFrame->stVFrame.u32TimeRef = 0;
    stVideoFrame->stVFrame.u64PTS = 0;
    stVideoFrame->stVFrame.u32Align = 16;
    stVideoFrame->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;

    stVideoFrame->u32PoolId = 0;
    stVideoFrame->stVFrame.u32Length[0] = image->image_private->memory_layout[0].size;
    stVideoFrame->stVFrame.u32Length[1] = image->image_private->memory_layout[1].size;
    stVideoFrame->stVFrame.u64PhyAddr[0] = image->image_private->data[0].u.device.device_addr;
    stVideoFrame->stVFrame.u64PhyAddr[1] = image->image_private->data[1].u.device.device_addr;
    if (image->image_private->plane_num >= 3) {
        stVideoFrame->stVFrame.u32Length[2] = image->image_private->memory_layout[2].size;
        stVideoFrame->stVFrame.u64PhyAddr[2] = image->image_private->data[2].u.device.device_addr;
    }
    if (image->image_format == FORMAT_COMPRESSED){
        stVideoFrame->stVFrame.u64ExtPhyAddr = image->image_private->data[3].u.device.device_addr;
        stVideoFrame->stVFrame.enCompressMode = COMPRESS_MODE_FRAME;
    }

    for (int i = 0; i < image->image_private->plane_num; ++i) {
        if (stVideoFrame->stVFrame.u32Length[i] == 0)
            continue;
        stVideoFrame->stVFrame.pu8VirAddr[i] = (unsigned char*)image->image_private->data[i].u.system.system_addr;
    }

    return BM_SUCCESS;
}

bm_status_t bm_image_format_to_cvi_(bm_image_format_ext fmt, bm_image_data_format_ext datatype, PIXEL_FORMAT_E * cvi_fmt){
  if((datatype != DATA_TYPE_EXT_1N_BYTE) && (datatype != DATA_TYPE_EXT_U16)){
    switch(datatype){
      case DATA_TYPE_EXT_1N_BYTE_SIGNED:
        *cvi_fmt = PIXEL_FORMAT_INT8_C3_PLANAR;
        break;
      case DATA_TYPE_EXT_FLOAT32:
        *cvi_fmt = PIXEL_FORMAT_FP32_C3_PLANAR;
        break;
      case DATA_TYPE_EXT_FP16:
        *cvi_fmt = PIXEL_FORMAT_FP16_C3_PLANAR;
        break;
      case DATA_TYPE_EXT_BF16:
        *cvi_fmt = PIXEL_FORMAT_BF16_C3_PLANAR;
        break;
      default:
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "datatype(%d) not supported, %s: %s: %d\n", datatype, filename(__FILE__), __func__, __LINE__);
        return BM_NOT_SUPPORTED;
    }
    return BM_SUCCESS;
  }
  switch(fmt){
    case FORMAT_YUV420P:
    case FORMAT_COMPRESSED:
      *cvi_fmt = PIXEL_FORMAT_YUV_PLANAR_420;
      break;
    case FORMAT_YUV422P:
      *cvi_fmt = PIXEL_FORMAT_YUV_PLANAR_422;
      break;
    case FORMAT_YUV444P:
      *cvi_fmt = PIXEL_FORMAT_YUV_PLANAR_444;
      break;
    case FORMAT_NV12:
      *cvi_fmt = PIXEL_FORMAT_NV12;
      break;
    case FORMAT_NV21:
      *cvi_fmt = PIXEL_FORMAT_NV21;
      break;
    case FORMAT_NV16:
      *cvi_fmt = PIXEL_FORMAT_NV16;
      break;
    case FORMAT_NV61:
      *cvi_fmt = PIXEL_FORMAT_NV61;
      break;
    case FORMAT_RGBP_SEPARATE:
      *cvi_fmt = PIXEL_FORMAT_RGB_888_PLANAR;
      break;
    case FORMAT_BGRP_SEPARATE:
      *cvi_fmt = PIXEL_FORMAT_BGR_888_PLANAR;
      break;
    case FORMAT_RGB_PACKED:
      *cvi_fmt = PIXEL_FORMAT_RGB_888;
      break;
    case FORMAT_BGR_PACKED:
      *cvi_fmt = PIXEL_FORMAT_BGR_888;
      break;
    case FORMAT_HSV_PLANAR:
      *cvi_fmt = PIXEL_FORMAT_HSV_888_PLANAR;
      break;
    case FORMAT_ARGB_PACKED:
      *cvi_fmt = PIXEL_FORMAT_ARGB_8888;
      break;
    case FORMAT_YUV444_PACKED:
      *cvi_fmt = PIXEL_FORMAT_RGB_888;
      break;
    case FORMAT_GRAY:
      *cvi_fmt = PIXEL_FORMAT_YUV_400;
      break;
    case FORMAT_YUV422_YUYV:
      *cvi_fmt = PIXEL_FORMAT_YUYV;
      break;
    case FORMAT_YUV422_YVYU:
      *cvi_fmt = PIXEL_FORMAT_YVYU;
      break;
    case FORMAT_YUV422_UYVY:
      *cvi_fmt = PIXEL_FORMAT_UYVY;
      break;
    case FORMAT_YUV422_VYUY:
      *cvi_fmt = PIXEL_FORMAT_VYUY;
      break;
    default:
      bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "fmt(%d) not supported, %s: %s: %d\n", fmt, filename(__FILE__), __func__, __LINE__);
      return BM_NOT_SUPPORTED;
  }
  return BM_SUCCESS;
}

bm_status_t check_dpu_grp_valid(int grp){
    if ((grp >= DPU_MAX_GRP_NUM) || (grp < 0)) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DPUGrp(%d) exceeds Max(%d), %s: %s: %d\n", grp, DPU_MAX_GRP_NUM, filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t check_dpu_chn_valid(int chn){
    if(chn >= DPU_MAX_CHN_NUM || chn < 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DPUChn(%d) exceeds Max(%d), %s: %s: %d\n", chn, DPU_MAX_GRP_NUM, filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t bm_dpu_get_grp_id(int fd, unsigned int *grp_id){
    bm_status_t ret = BM_SUCCESS;
    ret = (bm_status_t)ioctl(fd, CVI_DPU_GET_AVAIL_GROUP, grp_id);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                  "ret(%lx) bm_dpu_get_grp_id, %s: %s: %d\n",
                  (unsigned long)ret, filename(__FILE__), __func__, __LINE__);
        return ret;
    }
    return ret;
}

bm_status_t bm_dpu_create_grp(int fd, int DpuGrp, const DPU_GRP_ATTR_S *pstGrpAttr){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_grp_attr grp_attr;

    if(pstGrpAttr == NULL){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "pstGrpAttr == NULL, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    grp_attr.DpuGrp = DpuGrp;
    memcpy(&grp_attr.stGrpAttr, pstGrpAttr, sizeof(DPU_GRP_ATTR_S));

    ret = (bm_status_t)ioctl(fd, CVI_DPU_CREATE_GROUP, &grp_attr);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) create group fail, %s: %s: %d\n", DpuGrp, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_set_grp_attr(int fd, int DpuGrp, DPU_GRP_ATTR_S *pstGrpAttr){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_grp_attr grp_att = {.DpuGrp = DpuGrp};
    if(pstGrpAttr == NULL){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "pstGrpAttr == NULL, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    memcpy(&grp_att.stGrpAttr, pstGrpAttr, sizeof(DPU_GRP_ATTR_S));

    ret = (bm_status_t)ioctl(fd, CVI_DPU_SET_GRP_ATTR, &grp_att);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) set attr fail, %s: %s: %d\n", DpuGrp, filename(__FILE__), __func__, __LINE__);
        return ret;
    }
    return ret;
}

bm_status_t bm_dpu_destory_grp(int fd, int DpuGrp){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_grp_cfg grp_cfg = {.DpuGrp = DpuGrp};

    ret = (bm_status_t)ioctl(fd, CVI_DPU_DESTROY_GROUP, &grp_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) destory group fail, %s: %s: %d\n", DpuGrp, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_set_chn(int fd, int DpuGrp, int DpuChn, const DPU_CHN_ATTR_S *pstChnAttr){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_chn_attr chn_atr;

    if(pstChnAttr == NULL){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "pstChnAttr == NULL, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    chn_atr.DpuGrp = DpuGrp;
    chn_atr.DpuChn = DpuChn;

    memcpy(&chn_atr.stChnAttr, pstChnAttr, sizeof(chn_atr.stChnAttr));

    ret = (bm_status_t)ioctl(fd, CVI_DPU_SET_CHN_ATTR, &chn_atr);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuChn(%d) set group fail, %s: %s: %d\n", DpuChn, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_enable_chn(int fd, int DpuGrp, int DpuChn){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_chn_cfg chn_cfg = {.DpuGrp = DpuGrp, .DpuChn = DpuChn};
    ret = (bm_status_t)ioctl(fd, CVI_DPU_ENABLE_CHN, &chn_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) DpuChn(%d) enable fail, %s: %s: %d\n", DpuGrp, DpuChn, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_disable_chn(int fd, int DpuGrp, int DpuChn){
    bm_status_t ret;
    struct dpu_chn_cfg en_chn_cfg = {.DpuGrp = DpuGrp, .DpuChn = DpuChn};

    ret = (bm_status_t)ioctl(fd, CVI_DPU_DISABLE_CHN, &en_chn_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) DpuChn(%d) disable fail, %s: %s: %d\n", DpuGrp, DpuChn, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_start_grp(int fd, int DpuGrp){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_grp_cfg grp_cfg = {.DpuGrp = DpuGrp};

    ret = (bm_status_t)ioctl(fd, CVI_DPU_START_GROUP, &grp_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) start group fail, %s: %s: %d\n", DpuGrp, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_stop_grp(int fd, int DpuGrp){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_grp_cfg grp_cfg = {.DpuGrp = DpuGrp};

    ret = (bm_status_t)ioctl(fd, CVI_DPU_STOP_GROUP, &grp_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) stop group fail, %s: %s: %d\n", DpuGrp, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_send_frame(int fd, int DpuGrp, bm_image *left_input, bm_image *right_input, int s32Millisec){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_set_frame_cfg frame_cfg = {.DpuGrp = DpuGrp, .s32MilliSec = s32Millisec};

    VIDEO_FRAME_INFO_S pst_left_frame;
	VIDEO_FRAME_INFO_S pst_right_frame;
    PIXEL_FORMAT_E pixel_fomat = 0;

    memset(&pst_left_frame, 0, sizeof(pst_left_frame));
    memset(&pst_right_frame, 0, sizeof(pst_right_frame));
    bm_image_format_to_cvi_(left_input->image_format, left_input->data_type, &pixel_fomat);

    bm_image_to_frame(left_input, pixel_fomat, &pst_left_frame);
    bm_image_to_frame(right_input, pixel_fomat, &pst_right_frame);

    memcpy(&frame_cfg.stSrcLeftFrame, &pst_left_frame, sizeof(frame_cfg.stSrcLeftFrame));
    memcpy(&frame_cfg.stSrcRightFrame, &pst_right_frame, sizeof(frame_cfg.stSrcRightFrame));

    ret = (bm_status_t)ioctl(fd, CVI_DPU_SEND_FRAME, &frame_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) send frame fail, %s: %s: %d\n", DpuGrp, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_send_Chnframe(int fd, int DpuGrp, int DpuChn, bm_image *output_image, int s32Millisec){
    bm_status_t ret = BM_ERR_FAILURE;
    struct dpu_get_frame_cfg frame_cfg = {.DpuGrp = DpuGrp, .DpuChn = DpuChn, .s32MilliSec = s32Millisec};

    VIDEO_FRAME_INFO_S pst_output_frame;
    PIXEL_FORMAT_E pixel_fomat = 0;

    memset(&pst_output_frame, 0, sizeof(pst_output_frame));
    bm_image_format_to_cvi_(output_image->image_format, output_image->data_type, &pixel_fomat);

    bm_image_to_frame(output_image, pixel_fomat, &pst_output_frame);

    memcpy(&frame_cfg.stFrameInfo, &pst_output_frame, sizeof(frame_cfg.stFrameInfo));

    ret = (bm_status_t)ioctl(fd, CVI_DPU_SEND_CHN_FRAME, &frame_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) DpuChn(%d) send frame fail, %s: %s: %d\n", DpuGrp, DpuChn, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_get_frame(int fd, int DpuGrp, int DpuChn, bm_image *output, int s32Millisec){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_get_frame_cfg frame_cfg = {.DpuGrp = DpuGrp, .DpuChn = DpuChn, .s32MilliSec = s32Millisec};

    VIDEO_FRAME_INFO_S dst_output_frame;
    PIXEL_FORMAT_E format = 0;

    memset(&dst_output_frame, 0, sizeof(dst_output_frame));
    bm_image_format_to_cvi_(output->image_format, output->data_type, &format);
    bm_image_to_frame(output, format, &dst_output_frame);

    memcpy(&frame_cfg.stFrameInfo, &dst_output_frame, sizeof(frame_cfg.stFrameInfo));

    ret = (bm_status_t)ioctl(fd, CVI_DPU_GET_FRAME, &frame_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) get frame fail, %s: %s: %d\n", DpuGrp, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    for(int i = 0; i < output->image_private->plane_num; i++){
        output->image_private->data[i].u.device.device_addr = dst_output_frame.stVFrame.u64PhyAddr[i];
    }

    return ret;
}

bm_status_t bm_dpu_release_frame(int fd, int DpuGrp, int DpuChn, bm_image *output) {
    bm_status_t ret = BM_SUCCESS;
    struct dpu_release_frame_cfg frame_cfg = {.DpuChn = DpuChn, .DpuGrp = DpuGrp};

    VIDEO_FRAME_INFO_S dst_output_frame;
    PIXEL_FORMAT_E format = 0;

    memset(&dst_output_frame, 0, sizeof(dst_output_frame));
    bm_image_format_to_cvi_(output->image_format, output->data_type, &format);
    bm_image_to_frame(output, format, &dst_output_frame);

    memcpy(&frame_cfg.stFrameInfo, &dst_output_frame, sizeof(frame_cfg.stFrameInfo));

    ret = (bm_status_t)ioctl(fd, CVI_DPU_RELEASE_FRAME, &frame_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) DpuChn(%d) release frame fail, %s: %s: %d\n", DpuGrp, DpuChn, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_basic( bm_handle_t handle,
                          bm_image *left_input,
                          bm_image *right_input,
                          bm_image *output,
                          DPU_GRP_ATTR_S dpuGrpAttr){
    bm_status_t ret = BM_SUCCESS;
    int fd = 0;
    unsigned int DpuGrp = 0;
    int DpuChn = 0;
    DPU_CHN_ATTR_S chn;
    chn.stImgSize.u32Width = output->width;
    chn.stImgSize.u32Height = output->height;

    ret = bm_get_dpu_fd(&fd);

    ret = bm_dpu_get_grp_id(fd, &DpuGrp);
    if(ret != BM_SUCCESS){
        goto fail;
    }

    ret = check_dpu_grp_valid(DpuGrp);
    if(ret != BM_SUCCESS)
        goto fail;

    ret = check_dpu_chn_valid(DpuChn);
    if(ret != BM_SUCCESS)
        goto fail;

    // init dpu
    ret = bm_dpu_create_grp(fd, DpuGrp, &dpuGrpAttr);
    if(ret != BM_SUCCESS)
        goto fail;

    ret = bm_dpu_set_grp_attr(fd, DpuGrp, &dpuGrpAttr);
    if(ret != BM_SUCCESS)
        goto fail;

    ret = bm_dpu_set_chn(fd, DpuGrp, DpuChn, &chn);
    if(ret != BM_SUCCESS)
        goto fail;

    ret = bm_dpu_enable_chn(fd, DpuGrp, DpuChn);
    if(ret != BM_SUCCESS)
        goto fail;

    // start dpu
    ret = bm_dpu_start_grp(fd, DpuGrp);
    if(ret != BM_SUCCESS)
        goto fail;

    // send frame
    ret = bm_dpu_send_Chnframe(fd, DpuGrp, DpuChn, output, 4000);
    if(ret != BM_SUCCESS)
        goto fail;

    ret = bm_dpu_send_frame(fd, DpuGrp, left_input, right_input, 4000);
    if(ret != BM_SUCCESS)
        goto fail;

    // get frame
    ret = bm_dpu_get_frame(fd, DpuGrp, DpuChn, output, 4000);
    if(ret != BM_SUCCESS)
        goto fail;

    // release frame
    ret = bm_dpu_release_frame(fd, DpuGrp, DpuChn, output);

fail:
    if(BM_SUCCESS != bm_dpu_disable_chn(fd, DpuGrp, DpuChn)){
        return BM_ERR_FAILURE;
    }

    if(BM_SUCCESS != bm_dpu_stop_grp(fd, DpuGrp)){
        return BM_ERR_FAILURE;
    }

    if(BM_SUCCESS != bm_dpu_destory_grp(fd, DpuGrp)){
        return BM_ERR_FAILURE;
    }

    // ret = bm_destroy_dpu_fd();

    return ret;
}

bm_status_t check_bm_dpu_image_param(bm_image *left_input, bm_image *right_input, bm_image *output){
    bm_status_t ret = BM_SUCCESS;
    int left_width = left_input->width;
    int left_height = left_input->height;

    if((left_input->data_type != DATA_TYPE_EXT_1N_BYTE) ||
       (right_input->data_type != DATA_TYPE_EXT_1N_BYTE)){

        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "Data type only support DATA_TYPE_EXT_1N_BYTE! \n");
        return BM_ERR_FAILURE;
    }

    if((output->data_type != DATA_TYPE_EXT_1N_BYTE ) &&
       (output->data_type != DATA_TYPE_EXT_U16)){

        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "Data type not support! \n");
        return BM_ERR_FAILURE;
    }

    if((left_input->image_format != FORMAT_GRAY) ||
       (right_input->image_format != FORMAT_GRAY) ||
       (output->image_format != FORMAT_GRAY)){

        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "Img format only supported FORMAT_GRAY! \n");
        return BM_ERR_FAILURE;
    }

    if((left_width != right_input->width) ||
       (left_height != right_input->height)){

        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "The width and height of the left and right images need to be the same.! \n");
        return BM_ERR_FAILURE;
    }

    if((left_width != output->width) ||
       (left_height != output->height)){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "The width and height of img error.! left_height is %d , left_width is %d , output_width is %d , output_height is %d \n", left_height, left_width, output->width, output->height);
        return BM_ERR_FAILURE;
    }

    if((left_width < 64) || (left_width > 1920) || (left_height < 64) || (left_height > 1080)){

        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "The width and height of img error.! left_height is %d , left_width is %d \n", left_height, left_width);
        return BM_ERR_FAILURE;
    }

    if(left_width % 4 != 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "The width should 4 align \n");
        return BM_ERR_FAILURE;
    }

    return ret;
}

bm_status_t set_default_dpu_param(DPU_GRP_ATTR_S *grp, bm_image *left_input, bm_image *right_input){
    grp->stLeftImageSize.u32Height = left_input->height;
    grp->stLeftImageSize.u32Width = left_input->width;
    grp->stRightImageSize.u32Width = right_input->width;
    grp->stRightImageSize.u32Height = right_input->height;
    grp->enDispRange = 1;
    grp->enMaskMode = 4;
    grp->u16DispStartPos = 0;
    grp->u32Rshift1 = 3;
    grp->u32Rshift2 = 2;
    grp->u32CaP1 = 1800;
    grp->u32CaP2 = 14400;
    grp->u32UniqRatio = 25;
    grp->u32DispShift = 4;
    grp->u32CensusShift = 1;
    grp->enDccDir = 1;
    grp->u32FxBaseline = 864000;
    grp->u32FgsMaxCount = 19;
    grp->u32FgsMaxT = 3;
    grp->enDpuDepthUnit = 1;
    grp->bIsBtcostOut = 0;
    grp->bNeedSrcFrame = 0;
    return 0;
}

bm_status_t bm_dpu_sgbm_disp_internal( bm_handle_t          handle,
                                       bm_image             *left_image,
                                       bm_image             *right_image,
                                       bm_image             *disp_image,
                                       bmcv_dpu_sgbm_attrs  *dpu_attr,
                                       bmcv_dpu_sgbm_mode   sgbm_mode){
    bm_status_t ret = BM_SUCCESS;
    DPU_GRP_ATTR_S dpu_grp;
    memset(&dpu_grp, 0, sizeof(DPU_GRP_ATTR_S));
    set_default_dpu_param(&dpu_grp, left_image, right_image);

    if((sgbm_mode != DPU_SGBM_MUX0) && (sgbm_mode != DPU_SGBM_MUX1) && (sgbm_mode != DPU_SGBM_MUX2)){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "SGBM output mode not supported \n");
        return BM_ERR_FAILURE;
    }

    if(dpu_attr->disp_start_pos < 0 || dpu_attr->disp_start_pos > 239 || dpu_attr->disp_start_pos > (left_image->width - 16)){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "The disp_start_pos value error.! \n");
        return BM_ERR_FAILURE;
    }
    ret = check_bm_dpu_image_param(left_image, right_image, disp_image);
    if(ret != BM_SUCCESS){
        return ret;
    }

    dpu_grp.enDpuMode = sgbm_mode;
    dpu_grp.enDispRange = dpu_attr->disp_range_en;
    dpu_grp.enMaskMode = dpu_attr->bfw_mode_en;
    dpu_grp.u16DispStartPos = dpu_attr->disp_start_pos;
    dpu_grp.u32Rshift1 = dpu_attr->dpu_rshift1;
    dpu_grp.u32Rshift2 = dpu_attr->dpu_rshift2;
    dpu_grp.u32CaP1 = dpu_attr->dpu_ca_p1;
    dpu_grp.u32CaP2 = dpu_attr->dpu_ca_p2;
    dpu_grp.u32UniqRatio = dpu_attr->dpu_uniq_ratio;
    dpu_grp.u32DispShift = dpu_attr->dpu_disp_shift;
    dpu_grp.u32CensusShift = dpu_attr->dpu_census_shift;
    dpu_grp.enDccDir = dpu_attr->dcc_dir_en;

    ret = bm_dpu_basic(handle, left_image, right_image, disp_image, dpu_grp);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dpu basic failed \n");
        return ret;
    }
    return ret;
}

bm_status_t bm_dpu_fgs_disp_internal(  bm_handle_t          handle,
                                       bm_image             *guide_image,
                                       bm_image             *smooth_image,
                                       bm_image             *disp_image,
                                       bmcv_dpu_fgs_attrs   *fgs_attr,
                                       bmcv_dpu_fgs_mode    fgs_mode){
    bm_status_t ret = BM_SUCCESS;
    DPU_GRP_ATTR_S dpu_grp;
    memset(&dpu_grp, 0, sizeof(DPU_GRP_ATTR_S));
    set_default_dpu_param(&dpu_grp, guide_image, smooth_image);

    if((fgs_mode != DPU_FGS_MUX0) && (fgs_mode != DPU_FGS_MUX1)){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "FGS output mode not supported \n");
        return BM_ERR_FAILURE;
    }
    dpu_grp.enDpuMode = fgs_mode;

    ret = check_bm_dpu_image_param(guide_image, smooth_image, disp_image);
    if(ret != BM_SUCCESS){
        return ret;
    }

    dpu_grp.enDpuMode = fgs_mode;
    dpu_grp.u32FxBaseline = fgs_attr->fxbase_line;
    dpu_grp.u32FgsMaxCount = fgs_attr->fgs_max_count;
    dpu_grp.u32FgsMaxT = fgs_attr->fgs_max_t;
    dpu_grp.enDpuDepthUnit = fgs_attr->depth_unit_en;

    ret = bm_dpu_basic(handle, guide_image, smooth_image, disp_image, dpu_grp);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dpu basic failed \n");
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_online_disp_internal( bm_handle_t             handle,
                                         bm_image                *left_image,
                                         bm_image                *right_image,
                                         bm_image                *disp_image,
                                         bmcv_dpu_sgbm_attrs     *dpu_attr,
                                         bmcv_dpu_fgs_attrs      *fgs_attr,
                                         bmcv_dpu_online_mode    online_mode){
    bm_status_t ret = BM_SUCCESS;
    DPU_GRP_ATTR_S dpu_grp;
     memset(&dpu_grp, 0, sizeof(DPU_GRP_ATTR_S));
    set_default_dpu_param(&dpu_grp, left_image, right_image);

    if((online_mode != DPU_ONLINE_MUX0) && (online_mode != DPU_ONLINE_MUX1) &&
       (online_mode != DPU_ONLINE_MUX2)){

        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "FGS output mode not supported \n");
        return BM_ERR_FAILURE;
    }

    if(dpu_attr->disp_start_pos < 0 || dpu_attr->disp_start_pos > 239 ||
       dpu_attr->disp_start_pos > (left_image->width - 16)){

        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "The disp_start_pos value error.! \n");
        return BM_ERR_FAILURE;
    }
    ret = check_bm_dpu_image_param(left_image, right_image, disp_image);
    if(ret != BM_SUCCESS){
        return ret;
    }

    dpu_grp.enDpuMode = online_mode;
    dpu_grp.enDispRange = dpu_attr->disp_range_en;
    dpu_grp.enMaskMode = dpu_attr->bfw_mode_en;
    dpu_grp.u16DispStartPos = dpu_attr->disp_start_pos;
    dpu_grp.u32Rshift1 = dpu_attr->dpu_rshift1;
    dpu_grp.u32Rshift2 = dpu_attr->dpu_rshift2;
    dpu_grp.u32CaP1 = dpu_attr->dpu_ca_p1;
    dpu_grp.u32CaP2 = dpu_attr->dpu_ca_p2;
    dpu_grp.u32UniqRatio = dpu_attr->dpu_uniq_ratio;
    dpu_grp.u32DispShift = dpu_attr->dpu_disp_shift;
    dpu_grp.u32CensusShift = dpu_attr->dpu_census_shift;
    dpu_grp.enDccDir = dpu_attr->dcc_dir_en;
    dpu_grp.u32FxBaseline = fgs_attr->fxbase_line;
    dpu_grp.u32FgsMaxCount = fgs_attr->fgs_max_count;
    dpu_grp.u32FgsMaxT = fgs_attr->fgs_max_t;
    dpu_grp.enDpuDepthUnit = fgs_attr->depth_unit_en;

    ret = bm_dpu_basic(handle, left_image, right_image, disp_image, dpu_grp);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dpu basic failed \n");
        return ret;
    }
    return ret;
}