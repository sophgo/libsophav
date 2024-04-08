#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include "bmcv_a2_common_internal.h"
#include "bmcv_blend_ioctl.h"
#include "bmcv_api_ext_c.h"

struct vdev {
  int fd;
  pthread_mutex_t mutex;
};

#define DEV_STITCH_NAME "/dev/soph-stitch"
#define TWO_WAY_BLENDING   2
#define THREE_WAY_BLENDING 3
#define FOUR_WAY_BLENDING  4
#define BLEND_ALIGN       16

#define MIN_WIDTH       64
#define MAX_WIDTH       4608
#define MAX_HEIGHT      8192


#define BLEND_SUCCESS                    (0)
#define BLEND_ERR_FAILURE               (-1)
#define BLEND_INCORRECT_INPUT_NUM       (-2)
#define BLEND_INCORRECT_WIDTH_OR_HEIGHT (-3)
#define BLEND_INCORRECT_FMT             (-4)
#define BLEND_INCORRECT_WGT_MODE        (-5)
#define BLEND_INCORRECT_LX_RX           (-6)
#define BLEND_INCORRECT_BD_LX_RX        (-7)
#define BLEND_INCORRECT_WGT_SIZE        (-8)
#define BLEND_INCORRECT_WIDTH_MATCH     (-9)


/* For STITCH*/
#define STITCH_TIMEOUT 60000

int open_device(const char *dev_name, int *fd);
int close_device(int *fd);

static atomic_bool stitch_init_once = ATOMIC_VAR_INIT(false);

static struct vdev dev_stitch;;

static inline int check_stitch_src_id_valid(int idx)
{
  if ((idx >= STITCH_MAX_SRC_NUM) || (idx < 0))
  {
    BMCV_ERR_LOG("bmcv_blending, src_id(%d) exceeds Max(%d)\n", idx, STITCH_MAX_SRC_NUM - 1);
    return BLEND_INCORRECT_INPUT_NUM;
  }
  return BM_SUCCESS;
}

static int bm_stitch_dev_open(void)
{
  int s32Ret = BM_SUCCESS;

  dev_stitch.fd = -1;

  s32Ret=open_device(DEV_STITCH_NAME, &dev_stitch.fd);

  if (-1 == s32Ret)
  {
    perror("open soph-stitch fail\n");
    s32Ret = BM_ERR_FAILURE;
    return s32Ret;
  }

  return s32Ret;
}

static inline int bm_stitch_dev_close(void)
{
  int s32Ret = BM_SUCCESS;

  s32Ret = close_device(&dev_stitch.fd);
  if (s32Ret != BM_SUCCESS) {
    perror("stitch close failed\n");
    s32Ret = BM_ERR_FAILURE;
    return s32Ret;
  }

  dev_stitch.fd = -1;
  return s32Ret;
}

static inline int get_stitch_fd(int *fd)
{
  if (NULL == fd) {
    BMCV_ERR_LOG("stitch, null ptr fd\n");
    return BM_ERR_FAILURE;
  }

  if (dev_stitch.fd < 0) {
    BMCV_ERR_LOG("stitch, get stitch fd fail\n");
    return BM_ERR_FAILURE;
  }

  *fd = dev_stitch.fd;
  return BM_SUCCESS;
}

__attribute__((unused)) static int bm_stitch_Reset(void)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  s32Ret = cvi_stitch_reset(fd);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, stitch_reset fail\n");
    return s32Ret;
  }

  return BM_SUCCESS;
}

static int bm_stitch_int(void)
{
  int s32Ret = BM_SUCCESS;
  int fd = -1;

  bool expect = false;

  // Only init once until exit.
  if (!atomic_compare_exchange_strong(&stitch_init_once, &expect, true))
    return BM_SUCCESS;

  if (bm_stitch_dev_open() != BM_SUCCESS)
    return BM_ERR_FAILURE;

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  s32Ret = cvi_stitch_init(fd);
  if (s32Ret != BM_SUCCESS) {
     BMCV_ERR_LOG("stitch, init fail\n");
    return s32Ret;
  }

  s32Ret = cvi_stitch_reset(fd);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, stitch_reset fail\n");
    return s32Ret;
  }

  return s32Ret;
}

int bm_stitch_deinit(void)
{
  int s32Ret = BM_SUCCESS;
  int fd = -1;

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  s32Ret = cvi_stitch_deinit(fd);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, deinit fail\n");
    return s32Ret;
  }

#if 0
  s32Ret = bm_stitch_dev_close();
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("bm_stitch_dev_close fail\n");
    return s32Ret;
  }
#endif

  return s32Ret;
}

static int bm_stitch_SetSrcAttr(STITCH_SRC_ATTR_S *srcAttr)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;

  if(NULL == srcAttr)
  {
    BMCV_ERR_LOG("bm_stitch_SetSrcAttr, srcAttr NULL\n");
    return BM_ERR_FAILURE;
  }

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  s32Ret = cvi_stitch_set_src_attr(fd, srcAttr);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, stitch_set_src_attr fail\n");
    return s32Ret;
  }

  return BM_SUCCESS;
}

static __attribute__((unused)) int bm_stitch_GetSrcAttr(STITCH_SRC_ATTR_S *srcAttr)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;

  if(NULL == srcAttr)
  {
    BMCV_ERR_LOG("bm_stitch_GetSrcAttr, srcAttr NULL\n");
    return BM_ERR_FAILURE;
  }

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  s32Ret = cvi_stitch_get_src_attr(fd, srcAttr);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, stitch_set_src_attr fail\n");
    return s32Ret;
  }

  return BM_SUCCESS;
}

static int bm_stitch_SetChnAttr(STITCH_CHN_ATTR_S *chnAttr)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;

  if(NULL == chnAttr)
  {
    BMCV_ERR_LOG("bm_stitch_SetChnAttr, chnAttr NULL\n");
    return BM_ERR_FAILURE;
  }

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  s32Ret = cvi_stitch_set_chn_attr(fd, chnAttr);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, stitch_set_chn_attr fail\n");
    return s32Ret;
  }

  return BM_SUCCESS;
}

static __attribute__((unused)) int bm_stitch_GetChnAttr(STITCH_CHN_ATTR_S *chnAttr)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;

  if(NULL == chnAttr)
  {
    BMCV_ERR_LOG("bm_stitch_GetChnAttr, chnAttr NULL\n");
    return BM_ERR_FAILURE;
  }

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  s32Ret = cvi_stitch_get_chn_attr(fd, chnAttr);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, stitch_get_chn_attr fail\n");
    return s32Ret;
  }

  return BM_SUCCESS;
}

static int bm_stitch_SetOpAttr(STITCH_OP_ATTR_S *opAttr)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;

  if(NULL == opAttr)
  {
    BMCV_ERR_LOG("bm_stitch_SetOpAttr, opAttr NULL\n");
    return BM_ERR_FAILURE;
  }

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  s32Ret = cvi_stitch_set_op_attr(fd, opAttr);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, stitch_set_op_attr fail\n");
    return s32Ret;
  }

  return BM_SUCCESS;
}

static __attribute__((unused)) int bm_stitch_GetOpAttr(STITCH_OP_ATTR_S *opAttr)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;

  if(NULL == opAttr)
  {
    BMCV_ERR_LOG("bm_stitch_GetOpAttr, opAttr NULL\n");
    return BM_ERR_FAILURE;
  }

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  s32Ret = cvi_stitch_get_op_attr(fd, opAttr);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, stitch_get_op_attr fail\n");
    return s32Ret;
  }

  return BM_SUCCESS;
}

static int bm_stitch_SetWgtAttr(STITCH_WGT_ATTR_S *wgtAttr)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;

  if(NULL == wgtAttr)
  {
    BMCV_ERR_LOG("bm_stitch_SetWgtAttr, wgtAttr NULL\n");
    return BM_ERR_FAILURE;
  }

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  s32Ret = cvi_stitch_set_wgt_attr(fd, wgtAttr);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, cvi_stitch_set_wgt_attr fail\n");
    return s32Ret;
  }

  return BM_SUCCESS;
}
__attribute__((unused)) static int bm_stitch_GetWgtAttr(STITCH_WGT_ATTR_S *wgtAttr)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;

  if (NULL == wgtAttr)
  {
    BMCV_ERR_LOG("bm_stitch_GetWgtAttr, wgtAttr NULL\n");
    return BM_ERR_FAILURE;
  }

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  s32Ret = cvi_stitch_get_wgt_attr(fd, wgtAttr);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, cvi_stitch_get_wgt_attr fail\n");
    return s32Ret;
  }

  return BM_SUCCESS;
}

__attribute__((unused)) static int bm_stitch_SetRegX(CVI_U8 regX)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  s32Ret = cvi_stitch_set_regx(fd, regX);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, stitch_set_regx fail\n");
    return s32Ret;
  }

  return BM_SUCCESS;
}

static int bm_stitch_EnableDev(void)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  s32Ret = cvi_stitch_dev_enable(fd);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, stitch_EnableDev fail\n");
    return s32Ret;
  }

  return BM_SUCCESS;
}

static int bm_stitch_DisableDev(void)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  s32Ret = cvi_stitch_dev_disable(fd);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, stitch_dev_disable fail\n");
    return s32Ret;
  }

  return BM_SUCCESS;
}

static int bm_stitch_SendFrame(int srcIdx, const VIDEO_FRAME_INFO_S *VideoFrame, int MilliSec)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;
  struct stitch_src_frm_cfg cfg;

  if(NULL == VideoFrame)
  {
    BMCV_ERR_LOG("bm_stitch_SendFrame, VideoFrame NULL\n");
    return BM_ERR_FAILURE;
  }

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  s32Ret = check_stitch_src_id_valid(srcIdx);
  if (s32Ret != BM_SUCCESS)
    return s32Ret;

  cfg.src_id = srcIdx;
  cfg.MilliSec = MilliSec;
  memcpy(&cfg.VideoFrame, VideoFrame, sizeof(cfg.VideoFrame));

  s32Ret = cvi_stitch_send_src_frame(fd, &cfg);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, src_id(%d) send frame fail\n", srcIdx);
    return s32Ret;
  }

  return BM_SUCCESS;
}

static int bm_stitch_SendChnFrame(const VIDEO_FRAME_INFO_S *VideoFrame, int MilliSec)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;
  struct stitch_chn_frm_cfg cfg;

  if(NULL == VideoFrame)
  {
    BMCV_ERR_LOG("bm_stitch_SendChnFrame, VideoFrame NULL\n");
    return BM_ERR_FAILURE;
  }

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  cfg.MilliSec = MilliSec;
  memcpy(&cfg.VideoFrame, VideoFrame, sizeof(cfg.VideoFrame));

  s32Ret = cvi_stitch_send_chn_frame(fd, &cfg);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, chn send frame fail\n");
    return s32Ret;
  }

  return BM_SUCCESS;
}


static int bm_stitch_GetChnFrame(VIDEO_FRAME_INFO_S *VideoFrame, int MilliSec)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;
  struct stitch_chn_frm_cfg cfg;

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  if(NULL == VideoFrame)
  {
    BMCV_ERR_LOG("bm_stitch_GetChnFrame, bm_image output NULL\n");
    return BM_ERR_FAILURE;
  }

  memset(&cfg, 0, sizeof(cfg));
  cfg.MilliSec = MilliSec;

  s32Ret = cvi_stitch_get_chn_frame(fd, &cfg);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, get chn frame fail\n");
    return s32Ret;
  }
  memcpy(VideoFrame, &cfg.VideoFrame, sizeof(*VideoFrame));

  return BM_SUCCESS;
}

__attribute__((unused)) static int bm_stitch_ReleaseChnFrame(VIDEO_FRAME_INFO_S *VideoFrame)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;
  struct stitch_chn_frm_cfg cfg;

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  if(NULL == VideoFrame)
  {
    BMCV_ERR_LOG("bm_stitch_ReleaseChnFrame, VideoFrame NULL\n");
    return BM_ERR_FAILURE;
  }

  memset(&cfg, 0, sizeof(cfg));
  memcpy(&cfg.VideoFrame, VideoFrame, sizeof(*VideoFrame));

  s32Ret = cvi_stitch_release_chn_frame(fd, &cfg);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, release chn frame fail\n");
    return s32Ret;
  }

  return BM_SUCCESS;
}

static int bm_stitch_DumpRegInfo(void)
{
  int fd = -1;
  int s32Ret = BM_SUCCESS;

  if (get_stitch_fd(&fd) != BM_SUCCESS)
    return BM_ERR_FAILURE;

  s32Ret = cvi_stitch_dump_reginfo(fd);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("stitch, dump reg info fail\n");
    return s32Ret;
  }

  return BM_SUCCESS;
}

extern bm_status_t bm_image_format_to_cvi(bm_image_format_ext fmt, bm_image_data_format_ext datatype, PIXEL_FORMAT_E * cvi_fmt);
static void Create_SrcAttr_wgtAttr(bm_image* input, int input_num, struct stitch_param stitch_config, STITCH_SRC_ATTR_S *srcAttr,STITCH_WGT_ATTR_S* wgtAttr)
{
  int i = 0;
  for(i = 0; i < input_num; i++)
  {
    srcAttr->size[i].u32Width  = input[i].width;
    srcAttr->size[i].u32Height = input[i].height;
    srcAttr->bd_attr.bd_lx[i] = stitch_config.bd_attr.bd_lx[i];//left img, bd_attr from algo
    srcAttr->bd_attr.bd_rx[i] = stitch_config.bd_attr.bd_rx[i];
  }

  bm_image_format_to_cvi(input[0].image_format,input[0].data_type,&srcAttr->fmt_in);
  switch (input_num)
  {
    case 2:
      srcAttr->way_num = STITCH_2_WAY;
      break;
    case 4:
      srcAttr->way_num = STITCH_4_WAY;
      for (int i = 1; i < 3; i++) {
        srcAttr->ovlap_attr.ovlp_lx[i] = stitch_config.ovlap_attr.ovlp_lx[i];
        srcAttr->ovlap_attr.ovlp_rx[i] = stitch_config.ovlap_attr.ovlp_rx[i];
        wgtAttr->size_wgt[i].u32Width = ALIGN(srcAttr->ovlap_attr.ovlp_rx[i] - srcAttr->ovlap_attr.ovlp_lx[i] + 1, STITCH_ALIGN);
        wgtAttr->size_wgt[i].u32Height = srcAttr->size[i].u32Height;
        wgtAttr->phy_addr_wgt[i][0] = bm_mem_get_device_addr(stitch_config.wgt_phy_mem[i][0]);
        wgtAttr->phy_addr_wgt[i][1] = bm_mem_get_device_addr(stitch_config.wgt_phy_mem[i][1]);
      }
      break;
    default:
      BMCV_ERR_LOG("stitch, create way_num error\n");
      break;
  }

  srcAttr->ovlap_attr.ovlp_lx[0] = stitch_config.ovlap_attr.ovlp_lx[0];
  srcAttr->ovlap_attr.ovlp_rx[0] = stitch_config.ovlap_attr.ovlp_rx[0];

  wgtAttr->size_wgt[0].u32Width = ALIGN(srcAttr->ovlap_attr.ovlp_rx[0] - srcAttr->ovlap_attr.ovlp_lx[0] + 1, STITCH_ALIGN);
  wgtAttr->size_wgt[0].u32Height = srcAttr->size[0].u32Height;
  wgtAttr->phy_addr_wgt[0][0] = bm_mem_get_device_addr(stitch_config.wgt_phy_mem[0][0]);
  wgtAttr->phy_addr_wgt[0][1] = bm_mem_get_device_addr(stitch_config.wgt_phy_mem[0][1]);

  return;
}

static void CreatechnAttr(bm_image output, STITCH_CHN_ATTR_S *chnAttr)
{
  bm_image_format_to_cvi(output.image_format,output.data_type, &chnAttr->fmt_out);
  chnAttr->size.u32Width  = output.width;
  chnAttr->size.u32Height = output.height;
  return;
}

static void cfg_Frame(bm_image stitch_image, SIZE_S stSize, PIXEL_FORMAT_E enPixelFormat,VIDEO_FRAME_INFO_S* stVideoFrame)
{
  int i = 0;

  memset(stVideoFrame, 0, sizeof(VIDEO_FRAME_INFO_S));

  stVideoFrame->u32PoolId = 0;
  stVideoFrame->stVFrame.enCompressMode = COMPRESS_MODE_NONE;
  stVideoFrame->stVFrame.enPixelFormat = enPixelFormat;
  stVideoFrame->stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
  stVideoFrame->stVFrame.enColorGamut = COLOR_GAMUT_BT709;
  stVideoFrame->stVFrame.u32Width = stSize.u32Width;
  stVideoFrame->stVFrame.u32Height = stSize.u32Height;
  stVideoFrame->stVFrame.u32TimeRef = 0;
  stVideoFrame->stVFrame.u64PTS = 0;
  stVideoFrame->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;
  stVideoFrame->stVFrame.u32Align = STITCH_ALIGN;
  for(i = 0; i < 3; i++)
  {
    stVideoFrame->stVFrame.u32Stride[i] = stitch_image.image_private->memory_layout[i].pitch_stride;
    stVideoFrame->stVFrame.u32Length[i] = stitch_image.image_private->memory_layout[i].size;
    stVideoFrame->stVFrame.u64PhyAddr[i] = stitch_image.image_private->data[i].u.device.device_addr;
    stVideoFrame->stVFrame.pu8VirAddr[i] = NULL; //(unsigned char*)input.image_private->data[i].u.system.system_addr
  }
  return;
}

static int send_input_Frame(bm_image input,int src_id, SIZE_S stSize, PIXEL_FORMAT_E enPixelFormat)
{
  int s32Ret = BM_SUCCESS;
  VIDEO_FRAME_INFO_S stVideoFrame;

  cfg_Frame(input,stSize,enPixelFormat,&stVideoFrame);

  s32Ret = bm_stitch_SendFrame(src_id, &stVideoFrame, 1000);
  if (s32Ret != BM_SUCCESS)
    BMCV_ERR_LOG("stitch_send_VideoFrame fail.\n");

  return s32Ret;
}

static int send_output_Frame(bm_image output, SIZE_S stSize, PIXEL_FORMAT_E enPixelFormat)
{
  int s32Ret = BM_SUCCESS;
  VIDEO_FRAME_INFO_S stVideoFrame;

  cfg_Frame(output,stSize,enPixelFormat,&stVideoFrame);
  s32Ret = bm_stitch_SendChnFrame(&stVideoFrame, 1000);
  if (s32Ret != BM_SUCCESS)
    BMCV_ERR_LOG("stitch_send_VideoFrame fail.\n");

  return s32Ret;
}

static inline int check_fmt_param(bm_image image, bm_image_format_ext fmt, int w, int h)
{
  int s32Ret = BLEND_SUCCESS;
  int stride[4] = {0};

  if (bm_image_get_stride(image, stride) != 0)
  {
    BMCV_ERR_LOG("bmcv_blending,stride should be 16 align, %s: %s: %d\n",
    filename(__FILE__), __func__, __LINE__);
    return BLEND_ERR_FAILURE;
  }

  if((fmt != FORMAT_RGBP_SEPARATE) && (fmt != FORMAT_BGRP_SEPARATE) && \
    (fmt != FORMAT_YUV420P) && (fmt != FORMAT_YUV422P) && \
    (fmt != FORMAT_YUV444P) && (fmt != FORMAT_GRAY)) {
    BMCV_ERR_LOG("bmcv blend not support fmt(%d)\n", fmt);
    return BLEND_INCORRECT_FMT;
  }

  if (fmt == FORMAT_YUV422P)
  {
    if (w % 0x1) {
      BMCV_ERR_LOG("bmcv_blending, YUV_422 width(%d) should be even\n", w);
      s32Ret= BLEND_INCORRECT_WIDTH_OR_HEIGHT;
      return s32Ret;
    }
    if (0 != (stride[0] % (BLEND_ALIGN*2)))
    {
      BMCV_ERR_LOG("bmcv_blending, fmt YUV_422 %d, stride %d should be 32 byte align\n", fmt,stride[0]);
      s32Ret= BLEND_INCORRECT_WIDTH_OR_HEIGHT;
      return s32Ret;
    }
  }
  else if ((fmt == FORMAT_YUV420P))
  {
    if (w & 0x1) {
      BMCV_ERR_LOG("bmcv_blending, YUV_420 width(%d) should be even.\n", w);
      s32Ret= BLEND_INCORRECT_WIDTH_OR_HEIGHT;
    }
    if (h & 0x1) {
      BMCV_ERR_LOG("bmcv_blending, YUV_420 height(%d) should be even.\n", h);
      s32Ret= BLEND_INCORRECT_WIDTH_OR_HEIGHT;
    }
    if (0 != (stride[0] % (BLEND_ALIGN*2)))
    {
      BMCV_ERR_LOG("bmcv_blending, fmt YUV_420 %d, stride %d should be 32 byte align\n", fmt,stride[0]);
      s32Ret= BLEND_INCORRECT_WIDTH_OR_HEIGHT;
      return s32Ret;
    }
  }
  else
  {
    if (0 != (stride[0] % BLEND_ALIGN))
    {
      BMCV_ERR_LOG("bmcv_blending, fmt %d, stride %d should be 16 byte align\n", fmt,stride[0]);
      s32Ret= BLEND_INCORRECT_WIDTH_OR_HEIGHT;
      return s32Ret;
    }
  }

  return s32Ret;
}
static inline int check_input_output_fmt(bm_image_format_ext input_fmt, bm_image_format_ext output_fmt)
{
  int s32Ret = BLEND_SUCCESS;
  switch (input_fmt){
    case FORMAT_RGBP_SEPARATE:
    case FORMAT_BGRP_SEPARATE:
      if(input_fmt != output_fmt)
        s32Ret = BLEND_INCORRECT_FMT;
      break;

    case FORMAT_GRAY:
    case FORMAT_YUV422P:
    case FORMAT_YUV444P:
    case FORMAT_YUV420P:
      if((FORMAT_GRAY != output_fmt) && (FORMAT_YUV422P != output_fmt) && (FORMAT_YUV444P != output_fmt) && (FORMAT_YUV420P != output_fmt))
        s32Ret = BLEND_INCORRECT_FMT;
      break;

    default:
      s32Ret = BLEND_INCORRECT_FMT;
  }
  if(BLEND_SUCCESS != s32Ret)
  {
    BMCV_ERR_LOG("bmcv blend not support input_fmt(%d), output_fmt(%d)\n", input_fmt,output_fmt);
  }
  return s32Ret;
}

static int inline check_blend_param(bm_handle_t handle, int input_num, bm_image* input,bm_image output,struct stitch_param stitch_config)
{
  int wgtWidth = 0, wgtHeight = 0, wgt_len = 0, frame_idx = 0;
  int s32Ret = BLEND_SUCCESS;

  if ((input_num != FOUR_WAY_BLENDING) && (input_num != TWO_WAY_BLENDING))
  {
    BMCV_ERR_LOG("bmcv_blending, input_num(%d) should be 2 or 4\n", input_num);
    return BLEND_INCORRECT_INPUT_NUM;
  }

  if (output.data_type != DATA_TYPE_EXT_1N_BYTE)
  {
    BMCV_ERR_LOG("bmcv_blending,only support DATA_TYPE_EXT_1N_BYTE, %s: %s: %d\n",
    filename(__FILE__), __func__, __LINE__);
    return BLEND_ERR_FAILURE;
  }

  s32Ret=check_fmt_param(output, output.image_format, output.width, output.height);
  if(BLEND_SUCCESS != s32Ret)
  {
    return s32Ret;
  }

  for (frame_idx = 0; frame_idx < input_num; frame_idx++) {
    if(!input[frame_idx].image_private->attached ||
       !output.image_private->attached)
    {
      BMCV_ERR_LOG("bmcv_blending, not correctly create bm_image ,frame [%d] input or output not attache mem %s: %s: %d\n",
      frame_idx,filename(__FILE__), __func__, __LINE__);
      return BLEND_ERR_FAILURE;
    }
    if(input[frame_idx].data_type != DATA_TYPE_EXT_1N_BYTE)
    {
      BMCV_ERR_LOG("bmcv_blending, only support DATA_TYPE_EXT_1N_BYTE,frame [%d], %s: %s: %d\n",
      frame_idx, filename(__FILE__), __func__, __LINE__);
      return BLEND_ERR_FAILURE;
    }

    if(input[0].image_format != input[frame_idx].image_format)
    {
      BMCV_ERR_LOG("bmcv_blending, color format should keep it the same,frame [%d], %s: %s: %d\n",
      frame_idx, filename(__FILE__), __func__, __LINE__);
      return BLEND_ERR_FAILURE;
    }
    if(output.height!= input[frame_idx].height)
    {
      BMCV_ERR_LOG("bmcv_blending, height should keep it the same,frame [%d], %s: %s: %d\n",
      frame_idx, filename(__FILE__), __func__, __LINE__);
      return BLEND_ERR_FAILURE;
    }

    s32Ret= check_fmt_param(input[frame_idx], input[frame_idx].image_format, input[frame_idx].width, input[frame_idx].height);
    if(BLEND_SUCCESS != s32Ret)
    {
      return s32Ret;
    }
    if((stitch_config.bd_attr.bd_lx[frame_idx] != 0) || (stitch_config.bd_attr.bd_rx[frame_idx] != 0)) {
      BMCV_ERR_LOG("bmcv blend,(%d) bd_attr,bd lx %d,bd rx % invalid\n",
      frame_idx,stitch_config.bd_attr.bd_lx[frame_idx], stitch_config.bd_attr.bd_rx[frame_idx]);
      return BLEND_INCORRECT_BD_LX_RX;
    }


   if ((input[frame_idx].width < MIN_WIDTH) || (input[frame_idx].width > MAX_WIDTH))
   {
     BMCV_ERR_LOG("bmcv_blending,id %d, width %d should be 64~4608\n", frame_idx,input[frame_idx].width);
     s32Ret= BLEND_INCORRECT_WIDTH_OR_HEIGHT;
     return s32Ret;
   }

   if ((input[frame_idx].height < MIN_WIDTH) || (input[frame_idx].height > MAX_HEIGHT))
   {
     BMCV_ERR_LOG("bmcv_blending,id %d, height %d  should be 64~8192\n", frame_idx,input[frame_idx].height);
     s32Ret= BLEND_INCORRECT_WIDTH_OR_HEIGHT;
     return s32Ret;
   }

  }
#if 0
      int plane_num = bm_image_get_plane_num(input[frame_idx]);
      if(plane_num == 0 || bm_image_get_device_mem(input[frame_idx], device_mem) != BM_SUCCESS)
      {
        BMCV_ERR_LOG("not correctly create input[%d] bm_image, get plane num or device mem err %s: %s: %d\n",
        frame_idx, filename(__FILE__), __func__, __LINE__);
        return BLEND_ERR_FAILURE;
      }

      u64 device_addr = 0;
      int i = 0;

      for (i = 0; i < plane_num; i++)
      {
        device_addr = device_mem[i].u.device.device_addr;
        if((device_addr > 0x4ffffffff) || (device_addr < 0x100000000))
        {
          BMCV_ERR_LOG("input[%d] device memory should between 0x100000000 and 0x4ffffffff %u, %s: %s: %d\n",
          frame_idx, device_addr, filename(__FILE__), __func__, __LINE__);
          return BLEND_ERR_FAILURE;
        }
      }

      plane_num = bm_image_get_plane_num(output);
      if(plane_num == 0 || bm_image_get_device_mem(output[frame_idx], device_mem) != BM_SUCCESS)
      {
        BMCV_ERR_LOG(
        "not correctly create output[%d] bm_image, get plane num or device mem err %s: %s: %d\n",
        frame_idx, filename(__FILE__), __func__, __LINE__);
        return BLEND_ERR_FAILURE;
      }

      for (i = 0; i < plane_num; i++)
      {
        device_addr = device_mem[i].u.device.device_addr;
        if((device_addr > 0x4ffffffff) || (device_addr < 0x100000000))
        {
          BMCV_ERR_LOG( "output[%d] device memory should between 0x100000000 and 0x4ffffffff %u, %s: %s: %d\n",
          frame_idx, device_addr, filename(__FILE__), __func__, __LINE__);
          return BLEND_ERR_FAILURE;
        }
      }
#endif


  if (stitch_config.wgt_mode < BM_STITCH_WGT_YUV_SHARE || stitch_config.wgt_mode >= BM_STITCH_WGT_SEP) {
    BMCV_ERR_LOG("bmcv blend wgt_mode(%d) invalid\n", stitch_config.wgt_mode);
    return BLEND_INCORRECT_WGT_MODE;
  }
  for (frame_idx = 0; frame_idx < input_num - 1; frame_idx++)
  {
    if((stitch_config.ovlap_attr.ovlp_lx[frame_idx] < 0) || (stitch_config.ovlap_attr.ovlp_rx[frame_idx]< 0)) {
      BMCV_ERR_LOG("bmcv blend,(%d) ovlp,lx %d,rx % invalid\n",
      frame_idx,stitch_config.ovlap_attr.ovlp_lx[frame_idx], stitch_config.ovlap_attr.ovlp_rx[frame_idx]);
      return BLEND_INCORRECT_LX_RX;
    }
    wgtWidth = ALIGN(stitch_config.ovlap_attr.ovlp_rx[frame_idx] - stitch_config.ovlap_attr.ovlp_lx[frame_idx] + 1, BLEND_ALIGN);
    wgtHeight = output.height;
    wgt_len = wgtWidth * wgtHeight;
    if (stitch_config.wgt_mode == BM_STITCH_WGT_UV_SHARE)
      wgt_len = wgt_len << 1;

    if(0 != wgt_len)
    {
      if((wgt_len != stitch_config.wgt_phy_mem[frame_idx][0].size) ||
        (wgt_len != stitch_config.wgt_phy_mem[frame_idx][1].size))
      {
        BMCV_ERR_LOG("bmcv blend,(%d) wgt_len 0x%x, wgt device memory 0:0x%x, wgt device memory 1:0x%x\n",
        frame_idx,wgt_len,stitch_config.wgt_phy_mem[frame_idx][0].size, stitch_config.wgt_phy_mem[frame_idx][1].size);
        return BLEND_INCORRECT_WGT_SIZE;
      }
      if (!bm_mem_get_device_addr(stitch_config.wgt_phy_mem[frame_idx][0])||!bm_mem_get_device_addr(stitch_config.wgt_phy_mem[frame_idx][1]))
      {
        BMCV_ERR_LOG("bmcv blend,(%d) wgt_len 0x%x, wgt device addr0:0x%x, wgt device addr1:0x%x\n",
        frame_idx,wgt_len,bm_mem_get_device_addr(stitch_config.wgt_phy_mem[frame_idx][0]), bm_mem_get_device_addr(stitch_config.wgt_phy_mem[frame_idx][1]));
        return BLEND_INCORRECT_WGT_SIZE;
      }
    }
  }

  if(input_num == TWO_WAY_BLENDING)
  {
    if (input[1].width + stitch_config.ovlap_attr.ovlp_lx[0] != output.width)
    {
      BMCV_ERR_LOG("bmcv blend, input[1].width 0x%x + ovlp_lx[0]0x%x != output.width 0x%x \n",input[1].width, stitch_config.ovlap_attr.ovlp_lx[0],output.width);
      return BLEND_INCORRECT_WIDTH_MATCH;
    }
    if (input[0].width != stitch_config.ovlap_attr.ovlp_rx[0] + 1)
    {
      BMCV_ERR_LOG("bmcv blend, input[0].width 0x%x != ovlp_rx[0] 0x%x +1 \n",input[0].width, stitch_config.ovlap_attr.ovlp_rx[0]);
      return BLEND_INCORRECT_WIDTH_MATCH;
    }
  }
  if(input_num == FOUR_WAY_BLENDING)
  {
    if (input[3].width + stitch_config.ovlap_attr.ovlp_lx[2] != output.width)
    {
      BMCV_ERR_LOG("bmcv blend, input[3].width 0x%x + ovlp_lx[2]0x%x != output.width 0x%x \n",
      input[3].width, stitch_config.ovlap_attr.ovlp_lx[2],output.width);
      return BLEND_INCORRECT_WIDTH_MATCH;
    }
    if (input[1].width + stitch_config.ovlap_attr.ovlp_lx[0] != 1 + stitch_config.ovlap_attr.ovlp_rx[1])
    {
      BMCV_ERR_LOG("bmcv blend, input[1].width 0x%x != 1 + ovlp_rx[1]0x%x - ovlp_lx[0]0x%x \n",
      input[1].width, stitch_config.ovlap_attr.ovlp_rx[1],stitch_config.ovlap_attr.ovlp_lx[0]);
      return BLEND_INCORRECT_WIDTH_MATCH;
    }

    if (input[2].width + stitch_config.ovlap_attr.ovlp_lx[1] != 1 + stitch_config.ovlap_attr.ovlp_rx[2])
    {
      BMCV_ERR_LOG("bmcv blend, input[2].width 0x%x != 1 + ovlp_rx[2]0x%x - ovlp_lx[1]0x%x \n",
      input[2].width, stitch_config.ovlap_attr.ovlp_rx[2],stitch_config.ovlap_attr.ovlp_lx[1]);
      return BLEND_INCORRECT_WIDTH_MATCH;
    }
  }

  s32Ret= check_input_output_fmt(input[0].image_format,output.image_format);
  if(BLEND_SUCCESS != s32Ret)
  {
    return s32Ret;
  }
  return s32Ret;
}

//output.width == input[0].width + input[1].width - (1 + stitch_config.ovlap_attr[0].ovlp_rx - stitch_config.ovlap_attr[0].ovlp_lx);
static int bmcv_blending_asic(bm_handle_t handle, int input_num, bm_image* input,bm_image output,struct stitch_param stitch_config)
{
  int s32Ret = 0;
  STITCH_SRC_ATTR_S srcAttr;
  STITCH_CHN_ATTR_S chnAttr;
  STITCH_OP_ATTR_S  opAttr;
  STITCH_WGT_ATTR_S wgtAttr;
  VIDEO_FRAME_INFO_S stVideoFrameOut;

  s32Ret = check_blend_param(handle, input_num, input, output, stitch_config);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("bmcv blend, check_blend_param failed!\n");
    return s32Ret;
  }

  s32Ret = bm_stitch_int();
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("bm_stitch_int failed!\n");
    goto exit1;
  }

  Create_SrcAttr_wgtAttr(input, input_num, stitch_config, &srcAttr,&wgtAttr);
  s32Ret = bm_stitch_SetSrcAttr(&srcAttr);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("bm_stitch_SetSrcAttr failed!\n");
    goto exit2;
  }

  CreatechnAttr(output, &chnAttr);
  s32Ret = bm_stitch_SetChnAttr(&chnAttr);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("bm_stitch_SetChnAttr failed!\n");
    goto exit2;
  }

  opAttr.data_src = STITCH_DATA_SRC_DDR;
  opAttr.wgt_mode = stitch_config.wgt_mode;

  s32Ret = bm_stitch_SetOpAttr(&opAttr);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("bm_stitch_SetOpAttr failed!\n");
    goto exit2;
  }

  s32Ret = bm_stitch_SetWgtAttr(&wgtAttr);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("bm_stitch_SetWgtAttr failed!\n");
    goto exit2;
  }

  /*start stitch*/
  s32Ret = bm_stitch_EnableDev();
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("bm_stitch_EnableDev failed!\n");
    goto exit3;
  }

  s32Ret = send_output_Frame(output, chnAttr.size,chnAttr.fmt_out);
  if (s32Ret != BM_SUCCESS)
  {
    BMCV_ERR_LOG("stitch_send_VideoFrame[%d] fail, s32Ret: 0x%x !\n", 0, s32Ret);
    goto exit3;
  }

  s32Ret = send_input_Frame(input[0],0, srcAttr.size[0], srcAttr.fmt_in);
  if (s32Ret != BM_SUCCESS)
  {
    BMCV_ERR_LOG("stitch_send_VideoFrame[%d] fail, s32Ret: 0x%x !\n", 0, s32Ret);
    goto exit3;
  }
  s32Ret = send_input_Frame(input[1],1, srcAttr.size[1], srcAttr.fmt_in);
  if (s32Ret != BM_SUCCESS)
  {
    BMCV_ERR_LOG("stitch_send_VideoFrame[%d] fail, s32Ret: 0x%x !\n", 1, s32Ret);
    goto exit3;
  }

  if (STITCH_4_WAY == srcAttr.way_num)
  {
    s32Ret = send_input_Frame(input[2],2, srcAttr.size[2], srcAttr.fmt_in);
    if (s32Ret != BM_SUCCESS)
    {
      BMCV_ERR_LOG("stitch_send_VideoFrame[%d] fail, s32Ret: 0x%x !\n", 2, s32Ret);
      goto exit3;
    }
    s32Ret = send_input_Frame(input[3],3, srcAttr.size[3], srcAttr.fmt_in);
    if (s32Ret != BM_SUCCESS)
    {
      BMCV_ERR_LOG("stitch_send_VideoFrame[%d] fail, s32Ret: 0x%x !\n", 3, s32Ret);
      goto exit3;
    }
  }

  s32Ret = bm_stitch_GetChnFrame(&stVideoFrameOut, STITCH_TIMEOUT);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("CVI_STITCH_GetChnFrame fail. s32Ret: 0x%x !\n", s32Ret);
    //bm_stitch_Reset();
    bm_stitch_DumpRegInfo();
    goto exit3;
  }

#if 0
  s32Ret = bm_stitch_ReleaseChnFrame(&stVideoFrameOut);
  if (s32Ret != BM_SUCCESS) {
    BMCV_ERR_LOG("bm_stitch_ReleaseChnFrame fail s32Ret: 0x%x !\n", s32Ret);
    goto exit3;
  }
#endif

exit3:
  bm_stitch_DisableDev();
exit2:
exit1:
  return s32Ret;
}

int bmcv_three_way_blending(bm_handle_t handle, int input_num, bm_image* input,bm_image output,struct stitch_param stitch_config)
{
  int s32Ret = 0;
  bm_image  out_temp, input_temp[2];
  struct stitch_param stitch_config_temp;
  int o_width;

  if(NULL == handle)
  {
    BMCV_ERR_LOG("bmcv three_way_blending, handle should not be NULL!\n");
    return BLEND_ERR_FAILURE;
  }

  o_width = input[0].width + input[1].width - 1 - stitch_config.ovlap_attr.ovlp_rx[0] + stitch_config.ovlap_attr.ovlp_lx[0];

  bm_image_create(handle, input[0].height, o_width, input[0].image_format, DATA_TYPE_EXT_1N_BYTE, &out_temp,NULL);
  bm_image_alloc_dev_mem(out_temp, BMCV_HEAP_ANY);
  s32Ret = bmcv_blending_asic(handle, TWO_WAY_BLENDING, input, out_temp, stitch_config);
  if(0 != s32Ret)
  {
    BMCV_ERR_LOG("bmcv three_way_blending fail. s32Ret: 0x%x !\n", s32Ret);
    goto exit0;
  }


  input_temp[0] = out_temp;
  input_temp[1] = input[2];
  stitch_config_temp.ovlap_attr.ovlp_lx[0] = stitch_config.ovlap_attr.ovlp_lx[1];
  stitch_config_temp.ovlap_attr.ovlp_rx[0] = stitch_config.ovlap_attr.ovlp_rx[1];
  stitch_config_temp.bd_attr.bd_lx[0] = stitch_config_temp.bd_attr.bd_rx[0] = 0;
  stitch_config_temp.bd_attr.bd_lx[1] = stitch_config_temp.bd_attr.bd_rx[1] = 0;
  stitch_config_temp.wgt_phy_mem[0][0] = stitch_config.wgt_phy_mem[1][0];
  stitch_config_temp.wgt_phy_mem[0][1] = stitch_config.wgt_phy_mem[1][1];
  stitch_config_temp.wgt_mode = stitch_config.wgt_mode;
  s32Ret = bmcv_blending_asic(handle, TWO_WAY_BLENDING, input_temp, output, stitch_config_temp);

exit0:
  bm_image_destroy(&out_temp);
  return s32Ret;
}

bm_status_t bmcv_blending(bm_handle_t handle, int input_num, bm_image* input,bm_image output,struct stitch_param stitch_config)
{
  bm_status_t s32Ret = 0;

  // checkout_param();
  if(THREE_WAY_BLENDING == input_num)
  {
    s32Ret = bmcv_three_way_blending(handle, input_num, input, output, stitch_config);
  }
  else
  {
    s32Ret = bmcv_blending_asic(handle, input_num, input, output, stitch_config);
  }

  if(0 != s32Ret)
  {
    BMCV_ERR_LOG("%d images bmcv_blending fail. s32Ret: 0x%x !\n", input_num, s32Ret);
  }

  return s32Ret;
}

