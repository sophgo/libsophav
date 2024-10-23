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

bm_status_t bm_image_to_frame(bm_image *image, pixel_format_e enPixelFormat, video_frame_info_s *stVideoFrame){
    stVideoFrame->video_frame.compress_mode = COMPRESS_MODE_NONE;
    stVideoFrame->video_frame.pixel_format = enPixelFormat;
    stVideoFrame->video_frame.video_format = VIDEO_FORMAT_LINEAR;
    stVideoFrame->video_frame.color_gamut = COLOR_GAMUT_BT709;
    stVideoFrame->video_frame.width = image->width;
    stVideoFrame->video_frame.height = image->height;
    stVideoFrame->video_frame.stride[0] =  image->image_private->memory_layout[0].pitch_stride;
    stVideoFrame->video_frame.time_ref = 0;
    stVideoFrame->video_frame.pts = 0;
    stVideoFrame->video_frame.align = 16;
    stVideoFrame->video_frame.dynamic_range = DYNAMIC_RANGE_SDR8;

    stVideoFrame->pool_id = 0;
    stVideoFrame->video_frame.length[0] = image->image_private->memory_layout[0].size;
    stVideoFrame->video_frame.length[1] = image->image_private->memory_layout[1].size;
    stVideoFrame->video_frame.phyaddr[0] = image->image_private->data[0].u.device.device_addr;
    stVideoFrame->video_frame.phyaddr[1] = image->image_private->data[1].u.device.device_addr;
    if (image->image_private->plane_num >= 3) {
        stVideoFrame->video_frame.length[2] = image->image_private->memory_layout[2].size;
        stVideoFrame->video_frame.phyaddr[2] = image->image_private->data[2].u.device.device_addr;
    }
    if (image->image_format == FORMAT_COMPRESSED){
        stVideoFrame->video_frame.ext_phy_addr = image->image_private->data[3].u.device.device_addr;
        stVideoFrame->video_frame.compress_mode = COMPRESS_MODE_FRAME;
    }

    for (int i = 0; i < image->image_private->plane_num; ++i) {
        if (stVideoFrame->video_frame.length[i] == 0)
            continue;
        stVideoFrame->video_frame.viraddr[i] = (unsigned char*)image->image_private->data[i].u.system.system_addr;
    }

    return BM_SUCCESS;
}

bm_status_t bm_image_format_to_cvi_(bm_image_format_ext fmt, bm_image_data_format_ext datatype, pixel_format_e * cvi_fmt){
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
    ret = (bm_status_t)ioctl(fd, DPU_GET_AVAIL_GROUP, grp_id);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                  "ret(%lx) bm_dpu_get_grp_id, %s: %s: %d\n",
                  (unsigned long)ret, filename(__FILE__), __func__, __LINE__);
        return ret;
    }
    return ret;
}

bm_status_t bm_dpu_create_grp(int fd, int DpuGrp, const dpu_grp_attr_s *pstGrpAttr){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_grp_attr grp_attr;

    if(pstGrpAttr == NULL){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "pstGrpAttr == NULL, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    grp_attr.dpu_grp_id = DpuGrp;
    memcpy(&grp_attr.grp_attr, pstGrpAttr, sizeof(dpu_grp_attr_s));

    ret = (bm_status_t)ioctl(fd, DPU_CREATE_GROUP, &grp_attr);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) create group fail, %s: %s: %d\n", DpuGrp, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_set_grp_attr(int fd, int DpuGrp, dpu_grp_attr_s *pstGrpAttr){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_grp_attr grp_att = {.dpu_grp_id = DpuGrp};
    if(pstGrpAttr == NULL){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "pstGrpAttr == NULL, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    memcpy(&grp_att.grp_attr, pstGrpAttr, sizeof(dpu_grp_attr_s));

    ret = (bm_status_t)ioctl(fd, DPU_SET_GRP_ATTR, &grp_att);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) set attr fail, %s: %s: %d\n", DpuGrp, filename(__FILE__), __func__, __LINE__);
        return ret;
    }
    return ret;
}

bm_status_t bm_dpu_destory_grp(int fd, int DpuGrp){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_grp_cfg grp_cfg = {.dpu_grp_id = DpuGrp};

    ret = (bm_status_t)ioctl(fd, DPU_DESTROY_GROUP, &grp_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) destory group fail, %s: %s: %d\n", DpuGrp, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_set_chn(int fd, int DpuGrp, int DpuChn, const dpu_chn_attr_s *pstChnAttr){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_chn_attr chn_atr;

    if(pstChnAttr == NULL){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "pstChnAttr == NULL, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    chn_atr.dpu_grp_id = DpuGrp;
    chn_atr.dpu_chn_id = DpuChn;

    memcpy(&chn_atr.chn_attr, pstChnAttr, sizeof(chn_atr.chn_attr));

    ret = (bm_status_t)ioctl(fd, DPU_SET_CHN_ATTR, &chn_atr);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuChn(%d) set group fail, %s: %s: %d\n", DpuChn, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_enable_chn(int fd, int DpuGrp, int DpuChn){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_chn_cfg chn_cfg = {.dpu_grp_id = DpuGrp, .dpu_chn_id = DpuChn};
    ret = (bm_status_t)ioctl(fd, DPU_ENABLE_CHN, &chn_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) DpuChn(%d) enable fail, %s: %s: %d\n", DpuGrp, DpuChn, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_disable_chn(int fd, int DpuGrp, int DpuChn){
    bm_status_t ret;
    struct dpu_chn_cfg en_chn_cfg = {.dpu_grp_id = DpuGrp, .dpu_chn_id = DpuChn};

    ret = (bm_status_t)ioctl(fd, DPU_DISABLE_CHN, &en_chn_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) DpuChn(%d) disable fail, %s: %s: %d\n", DpuGrp, DpuChn, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_start_grp(int fd, int DpuGrp){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_grp_cfg grp_cfg = {.dpu_grp_id = DpuGrp};

    ret = (bm_status_t)ioctl(fd, DPU_START_GROUP, &grp_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) start group fail, %s: %s: %d\n", DpuGrp, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_stop_grp(int fd, int DpuGrp){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_grp_cfg grp_cfg = {.dpu_grp_id = DpuGrp};

    ret = (bm_status_t)ioctl(fd, DPU_STOP_GROUP, &grp_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) stop group fail, %s: %s: %d\n", DpuGrp, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_send_frame(int fd, int DpuGrp, bm_image *left_input, bm_image *right_input, int s32Millisec){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_set_frame_cfg frame_cfg = {.dpu_grp_id = DpuGrp, .millisec = s32Millisec};

    video_frame_info_s pst_left_frame;
	video_frame_info_s pst_right_frame;
    pixel_format_e pixel_fomat = 0;

    memset(&pst_left_frame, 0, sizeof(pst_left_frame));
    memset(&pst_right_frame, 0, sizeof(pst_right_frame));
    bm_image_format_to_cvi_(left_input->image_format, left_input->data_type, &pixel_fomat);

    bm_image_to_frame(left_input, pixel_fomat, &pst_left_frame);
    bm_image_to_frame(right_input, pixel_fomat, &pst_right_frame);

    memcpy(&frame_cfg.src_left_frame, &pst_left_frame, sizeof(frame_cfg.src_left_frame));
    memcpy(&frame_cfg.src_right_frame, &pst_right_frame, sizeof(frame_cfg.src_right_frame));

    ret = (bm_status_t)ioctl(fd, DPU_SEND_FRAME, &frame_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) send frame fail, %s: %s: %d\n", DpuGrp, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_send_Chnframe(int fd, int DpuGrp, int DpuChn, bm_image *output_image, int s32Millisec){
    bm_status_t ret = BM_ERR_FAILURE;
    struct dpu_get_frame_cfg frame_cfg = {.dpu_grp_id = DpuGrp, .dpu_chn_id = DpuChn, .millisec = s32Millisec};

    video_frame_info_s pst_output_frame;
    pixel_format_e pixel_fomat = 0;

    memset(&pst_output_frame, 0, sizeof(pst_output_frame));
    bm_image_format_to_cvi_(output_image->image_format, output_image->data_type, &pixel_fomat);

    bm_image_to_frame(output_image, pixel_fomat, &pst_output_frame);

    memcpy(&frame_cfg.video_frame, &pst_output_frame, sizeof(frame_cfg.video_frame));

    ret = (bm_status_t)ioctl(fd, DPU_SEND_CHN_FRAME, &frame_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) DpuChn(%d) send frame fail, %s: %s: %d\n", DpuGrp, DpuChn, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    return ret;
}

bm_status_t bm_dpu_get_frame(int fd, int DpuGrp, int DpuChn, bm_image *output, int s32Millisec){
    bm_status_t ret = BM_SUCCESS;
    struct dpu_get_frame_cfg frame_cfg = {.dpu_grp_id = DpuGrp, .dpu_chn_id = DpuChn, .millisec = s32Millisec};

    video_frame_info_s dst_output_frame;
    pixel_format_e format = 0;

    memset(&dst_output_frame, 0, sizeof(dst_output_frame));
    bm_image_format_to_cvi_(output->image_format, output->data_type, &format);
    bm_image_to_frame(output, format, &dst_output_frame);

    memcpy(&frame_cfg.video_frame, &dst_output_frame, sizeof(frame_cfg.video_frame));

    ret = (bm_status_t)ioctl(fd, DPU_GET_FRAME, &frame_cfg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DpuGrp(%d) get frame fail, %s: %s: %d\n", DpuGrp, filename(__FILE__), __func__, __LINE__);
        return ret;
    }

    for(int i = 0; i < output->image_private->plane_num; i++){
        output->image_private->data[i].u.device.device_addr = dst_output_frame.video_frame.phyaddr[i];
    }

    return ret;
}

bm_status_t bm_dpu_release_frame(int fd, int DpuGrp, int DpuChn, bm_image *output) {
    bm_status_t ret = BM_SUCCESS;
    struct dpu_release_frame_cfg frame_cfg = {.dpu_chn_id = DpuChn, .dpu_grp_id = DpuGrp};

    video_frame_info_s dst_output_frame;
    pixel_format_e format = 0;

    memset(&dst_output_frame, 0, sizeof(dst_output_frame));
    bm_image_format_to_cvi_(output->image_format, output->data_type, &format);
    bm_image_to_frame(output, format, &dst_output_frame);

    memcpy(&frame_cfg.video_frame, &dst_output_frame, sizeof(frame_cfg.video_frame));

    ret = (bm_status_t)ioctl(fd, DPU_RELEASE_FRAME, &frame_cfg);
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
                          dpu_grp_attr_s dpuGrpAttr){
    bm_status_t ret = BM_SUCCESS;
    int fd = 0;
    unsigned int DpuGrp = 0;
    int DpuChn = 0;
    dpu_chn_attr_s chn;
    chn.img_size.width = output->width;
    chn.img_size.height = output->height;

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

bm_status_t set_default_dpu_param(dpu_grp_attr_s *grp, bm_image *left_input, bm_image *right_input){
    grp->left_image_size.height = left_input->height;
    grp->left_image_size.width = left_input->width;
    grp->right_image_size.width = right_input->width;
    grp->right_image_size.height = right_input->height;
    grp->disp_range = 1;
    grp->mask_mode = 4;
    grp->dispstartpos = 0;
    grp->rshift1 = 3;
    grp->rshift2 = 2;
    grp->cap1 = 1800;
    grp->cap2 = 14400;
    grp->uniqratio = 25;
    grp->dispshift = 4;
    grp->censusshift = 1;
    grp->dcc_dir = 1;
    grp->fxbaseline = 864000;
    grp->fgsmaxcount = 19;
    grp->fgsmaxt = 3;
    grp->dpu_depth_unit = 1;
    grp->isbtcostout = 0;
    grp->need_src_frame = 0;
    return 0;
}

bm_status_t bm_dpu_sgbm_disp_internal( bm_handle_t          handle,
                                       bm_image             *left_image,
                                       bm_image             *right_image,
                                       bm_image             *disp_image,
                                       bmcv_dpu_sgbm_attrs  *dpu_attr,
                                       bmcv_dpu_sgbm_mode   sgbm_mode){
    bm_status_t ret = BM_SUCCESS;
    dpu_grp_attr_s dpu_grp;
    memset(&dpu_grp, 0, sizeof(dpu_grp_attr_s));
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

    dpu_grp.dpu_mode = sgbm_mode;
    dpu_grp.disp_range = dpu_attr->disp_range_en;
    dpu_grp.mask_mode = dpu_attr->bfw_mode_en;
    dpu_grp.dispstartpos = dpu_attr->disp_start_pos;
    dpu_grp.rshift1 = dpu_attr->dpu_rshift1;
    dpu_grp.rshift2 = dpu_attr->dpu_rshift2;
    dpu_grp.cap1 = dpu_attr->dpu_ca_p1;
    dpu_grp.cap2 = dpu_attr->dpu_ca_p2;
    dpu_grp.uniqratio = dpu_attr->dpu_uniq_ratio;
    dpu_grp.dispshift = dpu_attr->dpu_disp_shift;
    dpu_grp.censusshift = dpu_attr->dpu_census_shift;
    dpu_grp.dcc_dir = dpu_attr->dcc_dir_en;

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
    dpu_grp_attr_s dpu_grp;
    memset(&dpu_grp, 0, sizeof(dpu_grp_attr_s));
    set_default_dpu_param(&dpu_grp, guide_image, smooth_image);

    if((fgs_mode != DPU_FGS_MUX0) && (fgs_mode != DPU_FGS_MUX1)){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "FGS output mode not supported \n");
        return BM_ERR_FAILURE;
    }
    dpu_grp.dpu_mode = fgs_mode;

    ret = check_bm_dpu_image_param(guide_image, smooth_image, disp_image);
    if(ret != BM_SUCCESS){
        return ret;
    }

    dpu_grp.dpu_mode = fgs_mode;
    dpu_grp.fxbaseline = fgs_attr->fxbase_line;
    dpu_grp.fgsmaxcount = fgs_attr->fgs_max_count;
    dpu_grp.fgsmaxt = fgs_attr->fgs_max_t;
    dpu_grp.dpu_depth_unit = fgs_attr->depth_unit_en;

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
    dpu_grp_attr_s dpu_grp;
     memset(&dpu_grp, 0, sizeof(dpu_grp_attr_s));
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

    dpu_grp.dpu_mode = online_mode;
    dpu_grp.disp_range = dpu_attr->disp_range_en;
    dpu_grp.mask_mode = dpu_attr->bfw_mode_en;
    dpu_grp.dispstartpos = dpu_attr->disp_start_pos;
    dpu_grp.rshift1 = dpu_attr->dpu_rshift1;
    dpu_grp.rshift2 = dpu_attr->dpu_rshift2;
    dpu_grp.cap1 = dpu_attr->dpu_ca_p1;
    dpu_grp.cap2 = dpu_attr->dpu_ca_p2;
    dpu_grp.uniqratio = dpu_attr->dpu_uniq_ratio;
    dpu_grp.dispshift = dpu_attr->dpu_disp_shift;
    dpu_grp.censusshift = dpu_attr->dpu_census_shift;
    dpu_grp.dcc_dir = dpu_attr->dcc_dir_en;
    dpu_grp.fxbaseline = fgs_attr->fxbase_line;
    dpu_grp.fgsmaxcount = fgs_attr->fgs_max_count;
    dpu_grp.fgsmaxt = fgs_attr->fgs_max_t;
    dpu_grp.dpu_depth_unit = fgs_attr->depth_unit_en;

    ret = bm_dpu_basic(handle, left_image, right_image, disp_image, dpu_grp);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dpu basic failed \n");
        return ret;
    }
    return ret;
}