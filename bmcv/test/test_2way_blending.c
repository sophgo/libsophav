#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include "string.h"
#include "getopt.h"
#include <sys/time.h>
#include "bmcv_api_ext_c.h"


#define ALIGN(x, a)      (((x) + ((a)-1)) & ~((a)-1))
extern void bm_read_bin(bm_image src, const char *input_name);
extern void bm_write_bin(bm_image dst, const char *output_name);

static void example_test_cmd() {
  printf(
    "1. test_4way_blending  -N 2 -a c01_img1__4608x288.yuv -b c01_img2__4608x288.yuv -e 4608 -f 288 -g 0 -h 2way-6912x288.yuv420p -i 6912 -j 288 -k 0 -l 2304 -m 4607 -r c01_alpha12_444p_m2__0_288x2304.bin -s c01_beta12_444p_m2__0_288x2304.bin  -z c01_img12__6912x288.yuv\n"
    "2. test_4way_blending -N 2 -a c01_lft__4608x288_full_ovlp.yuv -b c01_rht__4608x288_full_ovlp.yuv -e 4608 -f 288 -g 0 -h 2way-4608x288.yuv420p  -i 4608 -j 288 -k 0 -l 0 -m 4607 -r c01_alpha_444p_m2__0_288x4608_full_ovlp.bin -s c01_beta_444p_m2__0_288x4608_full_ovlp.bin -z c01_result_420p_c2_4608x288_full_ovlp.yuv\n"
    "3. test_4way_blending -N 2 -a 1920x1080.yuv -b 1920x1080.yuv -e 1920 -f 1080 -g 0 -h 2way-3840x1080.yuv420p  -i 3840 -j 1080 -k 0 -l 1920 -m 1919 -z c01_result_420p_c2_3840x1080_none_ovlp.yuv \n"
    "4. test_4way_blending -N 2 -a c01_img1__1536x288.yuv  -b c01_img2__1536x288.yuv  -e 1536 -f 288 -g 0 -h 2way-3840x1080.yuv420p  -i 3072 -j 288 -k 0 -l 1536 -m 1535 -z  c01_result_420p_c2_3072x288_none_ovlp.yuv\n"
    "5. test_2way_blending  -a c01_img1__2304x288-lft.yuv -b c01_img2__4608x288-lft.yuv -c 2304 -d 288 -e 4608 -f 288 -g 0 -h 2way-4608x288.yuv420p -i 4608 -j 288  -k 0 -l 0 -m 2303  -r c01_alpha12_444p_m2__0_288x2304.bin -s c01_beta12_444p_m2__0_288x2304.bin -z c01_result_420p_c2_4608x288_lft_ovlp.yuv\n"
    "6. test_2way_blending  -a c01_img1__4608x288.yuv -b c01_img2__2304x288.yuv -c 4608 -d 288 -e 2304 -f 288 -g 0 -h 2way-4608x288.yuv420p -i 4608 -j 288  -k 0 -l 2304 -m 4607  -r c01_alpha12_444p_m2__0_288x2304.bin -s c01_beta12_444p_m2__0_288x2304.bin  -z c01_result_420p_c2_4608x288_rht_ovlp.yuv\n"
    "7. test_2way_blending  -a c01_lft__1536x384_pure_color.yuv  -b c01_rht__1088x384_pure_color.yuv -c 1536 -d 384 -e 1088 -f 384 -g 0 -h 2way-2400x384.yuv420p -i 2400 -j 384  -k 0 -l 1312 -m 1535  -r c01_alpha_444p_m2__0_384x224.bin  -s c01_beta_444p_m2__0_384x224.bin  -z c01_result_c2_2400x384_pure_color.yuv\n"
    "8. test_2way_blending  -a c01_lft__1536x384_pure_color.yuv  -b c01_rht__1088x384_pure_color.yuv -c 1536 -d 384 -e 1088 -f 384 -g 0 -h 2way-2400x384.yuv420p -i 2400 -j 384  -k 0 -l 1312 -m 1535  -r c01_alpha_m2__384x224_short.bin  -s c01_beta_m2__384x224_short.bin -z c01_result_c2_2400x384_pure_color1.yuv\n"
    "9. test_4way_blending  -N 2 -a c01_img1__4608x288.yuv -b c01_img2__4608x288.yuv -e 4608 -f 288 -g 14 -h 2way-6912x288.yuv400p -i 6912 -j 288 -k 14 -l 2304 -m 4607 -r c01_alpha12_444p_m2__0_288x2304.bin -s c01_beta12_444p_m2__0_288x2304.bin  -z c01_img12__6912x288_fmt400.yuv\n"
    "10.test_4way_blending  -N 2 -a c01_img1_422p__4608x288.yuv -b c01_img2_422p__4608x288.yuv -e 4608 -f 288 -g 1 -h 2way-6912x288.yuv422p -i 6912 -j 288 -k 1 -l 2304 -m 4607 -r c01_alpha12_444p_m2__0_288x2304.bin -s c01_beta12_444p_m2__0_288x2304.bin  -z c01_img12__6912x288_fmt422p.yuv\n"
    "11.test_4way_blending  -N 2 -a c01_img1_444p__4608x288.yuv -b c01_img2_444p__4608x288.yuv -e 4608 -f 288 -g 2 -h 2way-6912x288.yuv444p -i 6912 -j 288 -k 2 -l 2304 -m 4607 -r c01_alpha12_444p_m2__0_288x2304.bin -s c01_beta12_444p_m2__0_288x2304.bin  -z c01_img12__6912x288_fmt444p.yuv\n"
    "12.test_4way_blending  -N 2 -a c01_img1__4608x288.yuv -b c01_img2__4608x288.yuv -e 4608 -f 288 -g 0 -h 2way-6912x288.yuv420p -i 6912 -j 288 -k 0 -l 2304 -m 4607 -r c01_alpha12_m2__288x2304_short.bin -s c01_beta12_m2__288x2304_short.bin  -z c01_img12__6912x288_1.yuv -W 1\n"
    "13.test_4way_blending -N 4 -a c01_img1__4608x288.yuv -b c01_img2__4608x288.yuv  -c c01_img3__4608x288.yuv  -d c01_img4__4608x288.yuv -e 4608 -f 288 -g 0 -h 4way-11520x288.yuv420p -i 11520 -j 288 -k 0  -l 2304 -m 4607 -n 4608 -o 6911 -p 6912 -q 9215 -r c01_alpha12_444p_m2__0_288x2304.bin  -s c01_beta12_444p_m2__0_288x2304.bin -t c01_alpha23_444p_m2__0_288x2304.bin -u c01_beta23_444p_m2__0_288x2304.bin -v c01_alpha34_444p_m2__0_288x2304.bin  -w c01_beta34_444p_m2__0_288x2304.bin  -x 1 -y 0 -z c01_img1234__11520x288_real.yuv\n"
    "14.test_4way_blending -N 4 -a c01_lft__1024x1024.yuv -b c01_rht__1024x1024.yuv  -c c01_lft__1024x1024.yuv  -d c01_rht__1024x1024.yuv -e 1024 -f 1024 -g 0 -h 4way-3072x1024.yuv420p -i 3072 -j 1024 -k 0  -l 768 -m 1023 -n 1280 -o 1791 -p 2048 -q 2303 -r c01_alpha_444p_m2__0_256x1024.bin  -s c01_beta_444p_m2__0_256x1024.bin -t c01_alpha_444p_m2__0_1024x512.bin -u c01_beta_444p_m2__0_1024x512.bin -v c01_alpha_444p_m2__0_256x1024.bin  -w c01_beta_444p_m2__0_256x1024.bin -x 1 -y 0 -z c01_img1234__3072x1024_real.yuv\n"
    "15.test_4way_blending  -N 2 -a c01_lft__64x64.yuv -b c01_rht__64x64.yuv -e 64 -f 64 -g 0 -h 2way-96x64.yuv420p -i 96 -j 64 -k 0 -l 32 -m 63 -r c01_alpha_444p_m2__0_64x32.bin -s c01_beta_444p_m2__0_64x32.bin  -z c01_result_c2_96x64.yuv\n"
    "16.test_4way_blending  -N 2 -a c01_lft__128x128.yuv -b c01_rht__128x128.yuv -e 128 -f 128 -g 0 -h 2way-192x128.yuv420p -i 192 -j 128 -k 0 -l 64 -m 127 -r c01_alpha_444p_m2__0_128x64.bin -s c01_beta_444p_m2__0_128x64.bin  -z c01_result_c2_192x128.yuv\n"
    "17.test_4way_blending  -N 2 -a c01_lft__1024x1024.yuv -b c01_rht__1024x1024.yuv -e 1024 -f 1024 -g 0 -h 2way-1536x1024.yuv420p -i 1536 -j 1024 -k 0 -l 512 -m 1023 -r c01_alpha_444p_m2__0_1024x512.bin -s c01_beta_444p_m2__0_1024x512.bin  -z c01_result_c2_1536x1024.yuv\n"
  );
}

static void user_usage() {
  printf(
    "-a : src_name[0],\n"
    "-b : src_name[1],\n"
    "-c : src_w[0],\n"
    "-d : src_h[0],\n"
    "-e : src_w[1],\n"
    "-f : src_h[1],\n"
    "-g : src_fmt,     \n"
    "-h : dst_name,    \n"
    "-i : dst_w,  \n"
    "-j : dst_h,\n"
    "-k : dst_fmt,\n"
    "-l : ovlp_lx[0],\n"
    "-m : ovlp_rx[0],\n"
    "-r : wgt_name[0],     \n"
    "-s : wgt_name[1],    \n"
    "-x : loop_time,  \n"
    "-y : dev_id,\n"
    "-z : compare_name,\n"
    "-W : wgt_mode,\n"
    "-H : user_usage \n"
  );
}

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

int compare_file(bm_image dst, char * compare_name)
{
  int image_byte_size[4] = {0}, ret1 = 0;
  bm_image_get_byte_size(dst, image_byte_size);
  int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
  char* input_ptr = NULL;
  char* bmcv_output_ptr = NULL;

  FILE *fp = fopen(compare_name, "r");
  if (fp == NULL) {
    printf("open data file, %s, error\n", compare_name);
    return -1;
  }

  input_ptr = (char *)malloc(byte_size);
  bmcv_output_ptr = (char *)malloc(byte_size);

  void* out_ptr[4] = {(void*)bmcv_output_ptr,
                     (void*)((char*)bmcv_output_ptr + image_byte_size[0]),
                     (void*)((char*)bmcv_output_ptr + image_byte_size[0] + image_byte_size[1]),
                     (void*)((char*)bmcv_output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};

  bm_image_copy_device_to_host(dst, (void **)out_ptr);

  if (fread((void *)input_ptr, 1, byte_size, fp) < (unsigned int)byte_size){
      printf("file size is less than %d required bytes\n", byte_size);
      ret1 = -1;
      goto exit;
  };

  ret1 = memcmp(input_ptr, bmcv_output_ptr, byte_size);

  if(ret1 == 0 )
  {
    printf("stitch comparison success.\n");
  }
  else
  {
    printf("stitch comparison failed.\n");
  }

exit:

  free(input_ptr);
  free(bmcv_output_ptr);
  fclose(fp);
  return ret1;
}

int main(int argc, char *argv[]) {

  struct option long_options[] = {
    {"src_name[0]", required_argument,   NULL,  'a'},
    {"src_name[1]", required_argument,   NULL,  'b'},
    {"src_w[0]",    required_argument,   NULL,  'c'},
    {"src_h[0]",    required_argument,   NULL,  'd'},
    {"src_w[1]",    required_argument,   NULL,  'e'},
    {"src_h[1]",    required_argument,   NULL,  'f'},
    {"src_fmt",     required_argument,   NULL,  'g'},
    {"dst_name",    required_argument,   NULL,  'h'},
    {"dst_w",       required_argument,   NULL,  'i'},
    {"dst_h",       required_argument,   NULL,  'j'},
    {"dst_fmt",     required_argument,   NULL,  'k'},
    {"ovlp_lx",     required_argument,   NULL,  'l'},
    {"ovlp_rx",     required_argument,   NULL,  'm'},
    {"wgt_name[0]", required_argument,   NULL,  'r'},
    {"wgt_name[1]", required_argument,   NULL,  's'},
    {"loop_time",   required_argument,   NULL,  'x'},
    {"dev_id",      required_argument,   NULL,  'y'},
    {"compare_name",required_argument,   NULL,  'z'},
    {"wgt_mode",    required_argument,   NULL,  'W'},
    {"user_usage",  no_argument,         NULL,  'H'},
  };

#ifdef __linux__
    struct timeval tv_start;
    struct timeval tv_end;
    struct timeval timediff;
#endif

  if (argc == 2 && atoi(argv[1]) == -1) {
    example_test_cmd();
    return 0;
  }

  bm_handle_t handle = NULL;
  bm_image    src[2], dst;
  int src_h[2] = {0}, src_w[2] = {0}, dst_w = 6912, dst_h = 288, wgtWidth, wgtHeight;
  bm_image_format_ext src_fmt = FORMAT_YUV420P, dst_fmt = FORMAT_YUV420P;
  char *src_name[2] = {NULL}, *dst_name = NULL, *wgt_name[2] = {NULL},*compare_name=NULL;
  unsigned int i = 0, loop_time = 1;
  unsigned long long time_single, time_total = 0, time_avg = 0;
  unsigned long long time_max = 0, time_min = 10000, fps = 0, pixel_per_sec = 0;
  int  dev_id = 0, ret = 0,wgt_len =0, input_num =2;

  struct stitch_param stitch_config;
  memset(&stitch_config, 0, sizeof(stitch_config));
  stitch_config.wgt_mode = BM_STITCH_WGT_YUV_SHARE;

  int ch = 0, opt_idx = 0;
  while ((ch = getopt_long(argc, argv, "a:b:c:d:e:f:g:h:i:j:k:l:m:r:s:x:y:z:W:H", long_options, &opt_idx)) != -1) {
    switch (ch) {
      case 'a':
        src_name[0] = optarg;
        break;
      case 'b':
        src_name[1] = optarg;
        break;
      case 'c':
        src_w[0] = atoi(optarg);
        break;
      case 'd':
        src_h[0] = atoi(optarg);
        break;
      case 'e':
        src_w[1] = atoi(optarg);
          break;
      case 'f':
        src_h[1] = atoi(optarg);
        break;
      case 'g':
        src_fmt = (bm_image_format_ext)atoi(optarg);
        break;
      case 'h':
        dst_name = optarg;
        break;
      case 'i':
        dst_w = atoi(optarg);
        break;
      case 'j':
        dst_h = atoi(optarg);
        break;
      case 'k':
        dst_fmt = (bm_image_format_ext)atoi(optarg);
        break;
      case 'l':
        stitch_config.ovlap_attr.ovlp_lx[0] = atoi(optarg);
        break;
      case 'm':
        stitch_config.ovlap_attr.ovlp_rx[0] = atoi(optarg);
        break;
      case 'r':
        wgt_name[0] = optarg;
        break;
      case 's':
        wgt_name[1] = optarg;
        break;
      case 'x':
        loop_time = atoi(optarg);
        break;
      case 'y':
        dev_id = atoi(optarg);
        break;
      case 'z':
        compare_name = optarg;
        break;
      case 'W':
        stitch_config.wgt_mode = atoi(optarg);
        break;
      case 'H':
        user_usage();
        return 0;
    }
  }

  bm_status_t ret1    = bm_dev_request(&handle, dev_id);
  if (ret1 != BM_SUCCESS) {
      printf("Create bm handle failed. ret = %d\n", ret);
      exit(-1);
  }

  wgtWidth = ALIGN(stitch_config.ovlap_attr.ovlp_rx[0] - stitch_config.ovlap_attr.ovlp_lx[0] + 1, 16);
  wgtHeight = src_h[0];

  for(i = 0;i < 2; i++)
  {
    bm_image_create(handle, src_h[i], src_w[i], src_fmt, DATA_TYPE_EXT_1N_BYTE, &src[i],NULL);
    bm_image_alloc_dev_mem(src[i],1);
    bm_read_bin(src[i],src_name[i]);
    wgt_len = wgtWidth * wgtHeight;
    if (stitch_config.wgt_mode == BM_STITCH_WGT_UV_SHARE)
      wgt_len = wgt_len << 1;

    bm_dem_read_bin(handle, &stitch_config.wgt_phy_mem[0][i], wgt_name[i],  wgt_len);
  }
  bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst,NULL);
  bm_image_alloc_dev_mem(dst, 1);


  for(i = 0;i < loop_time; i++){
#ifdef __linux__
    gettimeofday(&tv_start, NULL);
#endif

    ret = bmcv_blending(handle, input_num, src, dst, stitch_config);
#ifdef __linux__
    gettimeofday(&tv_end, NULL);
    timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
    timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
    time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);
#endif

    if(time_single>time_max){time_max = time_single;}
    if(time_single<time_min){time_min = time_single;}
    time_total = time_total + time_single;
  }
  time_avg = time_total / loop_time;
  fps = 1000000 *2 / time_avg;
  pixel_per_sec = src_w[0] * src_h[0] * fps/1024/1024;

  if(NULL != dst_name)
  {
    bm_write_bin(dst, dst_name);
  }

  char src_fmt_str[100],dst_fmt_str[100];
  format_to_str(src[0].image_format, src_fmt_str);
  format_to_str(dst.image_format, dst_fmt_str);

  printf("%d*%d->%d*%d, %s->%s\n",src_w[0],src_h[0],dst_w,dst_h,src_fmt_str,dst_fmt_str);
  printf("bm_stitching:loop %d cycles, time_avg = %llu, fps %llu, %lluM pps\n\n",loop_time, time_avg, fps, pixel_per_sec);

  bmlib_log("BMCV",BMLIB_LOG_TRACE, "loop %d cycles, time_max = %llu, time_min = %llu, time_avg = %llu\n",
    loop_time, time_max, time_min, time_avg);

  if((NULL != compare_name) && ret==0)
  {
    ret = compare_file(dst, compare_name);

  }

  bm_image_destroy(&src[0]);
  bm_image_destroy(&src[1]);
  bm_image_destroy(&dst);
  bm_dev_free(handle);

  return ret;

}

