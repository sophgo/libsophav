#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include "string.h"
#include "getopt.h"
#include "signal.h"
#include "bmcv_api_ext_c.h"
#include <stdatomic.h>

#define ALIGN(x, a)      (((x) + ((a)-1)) & ~((a)-1))
void bm_dem_read_bin(bm_handle_t handle, bm_device_mem_t* dmem, const char *input_name, unsigned int size)
{
  if (access(input_name, F_OK) != 0 || strlen(input_name) == 0 || 0 >= size)
  {
    return;
  }

  char* input_ptr = (char *)malloc(size);
  FILE *fp_src = fopen(input_name, "rb+");

  if (fread((void *)input_ptr, 1, size, fp_src) < (unsigned int)size){
      printf("file size is less than %d required bytes\n", size);
  };
  fclose(fp_src);

  if (BM_SUCCESS != bm_malloc_device_byte(handle, dmem, size)){
    printf("bm_malloc_device_byte failed\n");
  }


  if (BM_SUCCESS != bm_memcpy_s2d(handle, *dmem, input_ptr)){
    printf("bm_memcpy_s2d failed\n");
  }

  free(input_ptr);
  return;
}

void blend_HandleSig(int signum)
{

  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  printf("signal happen  %d \n",signum);

  exit(-1);
}

int main() {
  bm_handle_t handle = NULL;
  bm_image    src[2], dst;
  int src_h[2] = {288, 288}, src_w[2] = {2304, 4608}, dst_w = 4608, dst_h = 288, wgtWidth, wgtHeight;
  bm_image_format_ext src_fmt = FORMAT_YUV420P, dst_fmt = FORMAT_YUV420P;
  char *src_name[2] = {"/home/linaro/A2test/bmcv/test/res/c01_img1__2304x288-lft.yuv", "/home/linaro/A2test/bmcv/test/res/c01_img2__4608x288-lft.yuv"}, *dst_name = "/home/linaro/A2test/bmcv/test/res/out/2way_blending.bin", *wgt_name[2] = {"/home/linaro/A2test/bmcv/test/res/c01_alpha12_444p_m2__0_288x2304.bin", "/home/linaro/A2test/bmcv/test/res/c01_beta12_444p_m2__0_288x2304.bin"}; /*,*compare_name=NULL;*/

  int  dev_id = 0, ret = 0,wgt_len =0, input_num =2;

  struct stitch_param stitch_config;
  memset(&stitch_config, 0, sizeof(stitch_config));
  stitch_config.wgt_mode = BM_STITCH_WGT_YUV_SHARE;

  signal(SIGINT, blend_HandleSig);
  signal(SIGTERM, blend_HandleSig);

  bm_status_t ret1    = bm_dev_request(&handle, dev_id);
  if (ret1 != BM_SUCCESS) {
      printf("Create bm handle failed. ret = %d\n", ret);
      exit(-1);
  }
  stitch_config.ovlap_attr.ovlp_rx[0] = 2303;
  stitch_config.ovlap_attr.ovlp_lx[0] = 0;
  wgtWidth = ALIGN(stitch_config.ovlap_attr.ovlp_rx[0] - stitch_config.ovlap_attr.ovlp_lx[0] + 1, 16);
  wgtHeight = src_h[0];

  int byte_size = 0;
  for(int i = 0;i < 2; i++)
  {
    bm_image_create(handle, src_h[i], src_w[i], src_fmt, DATA_TYPE_EXT_1N_BYTE, &src[i],NULL);
    bm_image_alloc_dev_mem(src[i], 1);
    byte_size  = src_w[i] * src_h[i] * 3 /2;
    unsigned char *input_data = (unsigned char *)malloc(byte_size);
    FILE *fp_src = fopen(src_name[i], "rb");
    if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
      printf("file size is less than required bytes%d\n", byte_size);
    };
    fclose(fp_src);
    void* in_ptr[4] = {(void *)input_data, (void *)((unsigned char*)input_data + src_w[i] * src_h[i]), (void *)((unsigned char*)input_data + src_w[i] * src_h[i] * 5 / 4)};
    bm_image_copy_host_to_device(src[i], in_ptr);
    wgt_len = wgtWidth * wgtHeight;
    if (stitch_config.wgt_mode == BM_STITCH_WGT_UV_SHARE)
      wgt_len = wgt_len << 1;

    bm_dem_read_bin(handle, &stitch_config.wgt_phy_mem[0][i], wgt_name[i],  wgt_len);
  }
  bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst,NULL);
  bm_image_alloc_dev_mem(dst, 1);

  ret = bmcv_blending(handle, input_num, src, dst, stitch_config);

  unsigned char* output_ptr = (unsigned char*)malloc(byte_size);
  void* out_ptr[4] = {(void*)output_ptr, (void*)((unsigned char*)output_ptr + dst_w * dst_h), (void*)((unsigned char*)output_ptr + dst_w * dst_h * 5 / 4)};
  bm_image_copy_device_to_host(dst, (void **)out_ptr);

  FILE *fp_dst = fopen(dst_name, "wb");
  if (fwrite((void *)output_ptr, 1, byte_size, fp_dst) < (unsigned int)byte_size){
      printf("file size is less than %d required bytes\n", byte_size);
  };
  fclose(fp_dst);

  bm_image_destroy(&src[0]);
  bm_image_destroy(&src[1]);
  bm_image_destroy(&dst);
  bm_dev_free(handle);

  return ret;

}

