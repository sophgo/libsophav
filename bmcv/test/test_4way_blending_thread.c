#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include "string.h"
#include "getopt.h"
#include <sys/time.h>
#include <pthread.h>
#include <stdatomic.h>
#include "signal.h"
#include "bmcv_api_ext_c.h"

#ifdef __linux__
#include <sys/time.h>
#else
#include <windows.h>
#include "time.h"
#endif

#define ALIGN(x, a)      (((x) + ((a)-1)) & ~((a)-1))
extern void bm_read_bin(bm_image src, const char *input_name);
extern void bm_write_bin(bm_image dst, const char *output_name);

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
_Atomic int threads_running = 1;
_Atomic int num_threads = 0;
int process_num = 0;

typedef struct {
    int     input_num;
    char* src_name[4];
    int src_w;
    int src_h;
    bm_image_format_ext src_fmt;
    char *dst_name;
    int dst_w;
    int dst_h;
    bm_image_format_ext dst_fmt;
    char *wgt_name[6];
    int loop_time;
    int dev_id;
    char *compare_name;
    struct stitch_param stitch_config;
    int wgt_len;
    bm_handle_t handle;
} cv_blending4_thread_arg_t;

static void user_usage() {
  printf(
    "-N : num,\n"
    "-a : src_name[0],\n"
    "-b : src_name[1],\n"
    "-c : src_name[2],\n"
    "-d : src_name[3],\n"
    "-e : src_w,\n"
    "-f : src_h,\n"
    "-g : src_fmt,     \n"
    "-h : dst_name,    \n"
    "-i : dst_w,  \n"
    "-j : dst_h,\n"
    "-k : dst_fmt,\n"
    "-l : ovlp_lx[0],\n"
    "-m : ovlp_rx[0],\n"
    "-n : ovlp_lx[1],\n"
    "-o : ovlp_rx[1],\n"
    "-p : ovlp_lx[2],\n"
    "-q : ovlp_rx[2],\n"
    "-r : wgt_name[0],     \n"
    "-s : wgt_name[1],    \n"
    "-t : wgt_name[2],     \n"
    "-u : wgt_name[3],    \n"
    "-v : wgt_name[4],     \n"
    "-w : wgt_name[5],    \n"
    "-x : loop_time,  \n"
    "-y : dev_id[0],\n"
    "-z : compare_name,\n"
    "-W : wgt_mode,\n"
    "-P : process_num,\n"
    "-H : user_usage \n"
  );
}
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

static int absdiff(unsigned char* input1, unsigned char* input2, int img_size)
{
  unsigned char output;
  int count = 0;

  for(int i = 0; i < img_size; i++)
  {
    output = abs(input1[i] - input2[i]);
    if(output > 1)
    {
      if(i+1 < img_size)
      {
        if(abs(input1[i+1] - input2[i+1]))
        {
          count++;
        }
      }
    }
  }

  return count;
}

int compare_file(bm_image dst, char * compare_name)
{
  int image_byte_size[4] = {0}, ret1 = 0, count = 0;
  bm_image_get_byte_size(dst, image_byte_size);
  int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
  unsigned char* input_ptr = NULL;
  unsigned char* bmcv_output_ptr = NULL;

  FILE *fp = fopen(compare_name, "r");
  if (fp == NULL) {
    printf("open data file, %s, error\n", compare_name);
    return -1;
  }

  input_ptr = (unsigned char *)malloc(byte_size);
  bmcv_output_ptr = (unsigned char *)malloc(byte_size);

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

  count = absdiff(input_ptr, bmcv_output_ptr, byte_size);

  if((0 <= count) && (1 >= count))
  {
    printf("stitch comparison success,count %d.\n",count);
  }
  else
  {
    printf("stitch comparison failed,count %d\n",count);
  }

exit:

  free(input_ptr);
  free(bmcv_output_ptr);
  fclose(fp);
  return ret1;
}

void blend_HandleSig(int signum)
{
  int count=100;

  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  printf("signal happen  %d \n",signum);


  atomic_store(&threads_running, 0);
  pthread_cond_broadcast(&cond);


  // Wait for all threads to stop
  while ((atomic_load(&num_threads) > 0) && (count > 0)) {
    usleep(10000); // Sleep for 10 ms
    printf("wait thread stop, remaning num_threads %d\n",atomic_load(&num_threads));
    count--;
  }

  exit(-1);
}

static int test_4way_blending(int input_num, char** src_name, int src_w, int src_h, bm_image_format_ext src_fmt,
                              char* dst_name, int dst_w, int dst_h, bm_image_format_ext dst_fmt,
                              char** wgt_name, int loop_time, int dev_id, int wgt_len, char* compare_name,
                              struct stitch_param stitch_config, bm_handle_t handle){
  int j = 0;
  int wgtWidth, wgtHeight;
  bm_image src[4];
  bm_image dst;
  int ret = 0;
  #ifdef __linux__
    struct timeval tv_start;
    struct timeval tv_end;
    struct timeval timediff;
  #endif
  unsigned long long time_single, time_total = 0, time_avg = 0;
  unsigned long long time_max = 0, time_min = 10000, fps = 0, pixel_per_sec = 0;
  atomic_fetch_add(&num_threads, 1);

  for(int i = 0;i < input_num; i++)
  {
    bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src[i],NULL);
    bm_image_alloc_dev_mem(src[i],1);
    bm_read_bin(src[i],src_name[i]);
  }

  for(int i = 0;i < (input_num - 1)*2; i++)
  {
    j = i/2;
    wgtWidth = ALIGN(stitch_config.ovlap_attr.ovlp_rx[j] - stitch_config.ovlap_attr.ovlp_lx[j] + 1, 16);
    wgtHeight = src_h;
    wgt_len = wgtWidth * wgtHeight;
    if (stitch_config.wgt_mode == BM_STITCH_WGT_UV_SHARE)
      wgt_len = wgt_len << 1;
    bm_dem_read_bin(handle, &stitch_config.wgt_phy_mem[j][i%2], wgt_name[i],  wgt_len);
  }

  bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst,NULL);
  bm_image_alloc_dev_mem(dst, 1);


  for(int i = 0;i < loop_time; i++){

    if (!atomic_load(&threads_running)) {
      break;
    }
#ifdef __linux__
    gettimeofday(&tv_start, NULL);
#endif

    ret = bmcv_blending(handle, input_num, src, dst, stitch_config);
    usleep(process_num * 1000 * 1000);
    // printf("sleep time =  %d us, %d ms\n", process_num * 1000 * 1000, process_num * 1000);
    if(0 != ret)
    {
      printf("bmcv_blending failed,ret = %d\n", ret);
      break;
    }

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
  fps = 1000000 / time_avg;
  pixel_per_sec = src_w * src_h * fps/1024/1024;

  if(NULL != dst_name)
  {
    bm_write_bin(dst, dst_name);
  }

  char src_fmt_str[100],dst_fmt_str[100];
  format_to_str(src[0].image_format, src_fmt_str);
  format_to_str(dst.image_format, dst_fmt_str);

  printf("%d*%d->%d*%d, %s->%s\n",src_w,src_h,dst_w,dst_h,src_fmt_str,dst_fmt_str);
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
  atomic_fetch_sub(&num_threads, 1);

  return ret;

};

void* test_blending(void* args){
  cv_blending4_thread_arg_t* cv_blending4_thread_arg = (cv_blending4_thread_arg_t*)args;
  char* src_name[4] = {NULL};
  char* wgt_name[6] = {NULL};
  struct stitch_param stitch_config;
  memset(&stitch_config, 0, sizeof(stitch_config));
  int input_num = cv_blending4_thread_arg->input_num;
  src_name[0] = cv_blending4_thread_arg->src_name[0];
  src_name[1] = cv_blending4_thread_arg->src_name[1];
  src_name[2] = cv_blending4_thread_arg->src_name[2];
  src_name[3] = cv_blending4_thread_arg->src_name[3];
  int src_w = cv_blending4_thread_arg->src_w;
  int src_h = cv_blending4_thread_arg->src_h;
  bm_image_format_ext src_fmt = cv_blending4_thread_arg->src_fmt;
  char * dst_name = cv_blending4_thread_arg->dst_name;
  int dst_w = cv_blending4_thread_arg->dst_w;
  int dst_h = cv_blending4_thread_arg->dst_h;
  bm_image_format_ext dst_fmt = cv_blending4_thread_arg->dst_fmt;
  wgt_name[0] = cv_blending4_thread_arg->wgt_name[0];
  wgt_name[1] = cv_blending4_thread_arg->wgt_name[1];
  wgt_name[2] = cv_blending4_thread_arg->wgt_name[2];
  wgt_name[3] = cv_blending4_thread_arg->wgt_name[3];
  wgt_name[4] = cv_blending4_thread_arg->wgt_name[4];
  wgt_name[5] = cv_blending4_thread_arg->wgt_name[5];
  int loop_time = cv_blending4_thread_arg->loop_time;
  int dev_id = cv_blending4_thread_arg->dev_id;
  int wgt_len = cv_blending4_thread_arg->wgt_len;
  char* compare_name = cv_blending4_thread_arg->compare_name;
  stitch_config.wgt_mode = cv_blending4_thread_arg->stitch_config.wgt_mode;
  stitch_config.ovlap_attr.ovlp_lx[2] = cv_blending4_thread_arg->stitch_config.ovlap_attr.ovlp_lx[2];
  stitch_config.ovlap_attr.ovlp_lx[1] = cv_blending4_thread_arg->stitch_config.ovlap_attr.ovlp_lx[1];
  stitch_config.ovlap_attr.ovlp_lx[0] = cv_blending4_thread_arg->stitch_config.ovlap_attr.ovlp_lx[0];
  stitch_config.ovlap_attr.ovlp_rx[2] = cv_blending4_thread_arg->stitch_config.ovlap_attr.ovlp_rx[2];
  stitch_config.ovlap_attr.ovlp_rx[1] = cv_blending4_thread_arg->stitch_config.ovlap_attr.ovlp_rx[1];
  stitch_config.ovlap_attr.ovlp_rx[0] = cv_blending4_thread_arg->stitch_config.ovlap_attr.ovlp_rx[0];
  bm_handle_t handle = cv_blending4_thread_arg->handle;
  if(0 != test_4way_blending(input_num, src_name, src_w, src_h, src_fmt, dst_name,
                            dst_w, dst_h, dst_fmt, wgt_name, loop_time, dev_id, wgt_len,
                            compare_name, stitch_config, handle)) {
    printf("---------Test 4way blending Failed!--------\n");
  } else {
    printf("----------Test 4way blending success!----------\n");
  }
  return (void*)0;
}

int main(int argc, char *argv[])
{
  struct option long_options[] = {
    {"num",         required_argument,   NULL,  'N'},
    {"src_name[0]", required_argument,   NULL,  'a'},
    {"src_name[1]", required_argument,   NULL,  'b'},
    {"src_name[2]", required_argument,   NULL,  'c'},
    {"src_name[3]", required_argument,   NULL,  'd'},
    {"src_w",       required_argument,   NULL,  'e'},
    {"src_h",       required_argument,   NULL,  'f'},
    {"src_fmt",     required_argument,   NULL,  'g'},
    {"dst_name",    required_argument,   NULL,  'h'},
    {"dst_w",       required_argument,   NULL,  'i'},
    {"dst_h",       required_argument,   NULL,  'j'},
    {"dst_fmt",     required_argument,   NULL,  'k'},
    {"ovlp_lx[0]",  required_argument,   NULL,  'l'},
    {"ovlp_rx[0]",  required_argument,   NULL,  'm'},
    {"ovlp_lx[1]",  required_argument,   NULL,  'n'},
    {"ovlp_rx[1]",  required_argument,   NULL,  'o'},
    {"ovlp_lx[2]",  required_argument,   NULL,  'p'},
    {"ovlp_rx[2]",  required_argument,   NULL,  'q'},
    {"wgt_name[0]", required_argument,   NULL,  'r'},
    {"wgt_name[1]", required_argument,   NULL,  's'},
    {"wgt_name[2]", required_argument,   NULL,  't'},
    {"wgt_name[3]", required_argument,   NULL,  'u'},
    {"wgt_name[4]", required_argument,   NULL,  'v'},
    {"wgt_name[5]", required_argument,   NULL,  'w'},
    {"loop_time",   required_argument,   NULL,  'x'},
    {"dev_id",      required_argument,   NULL,  'y'},
    {"compare_name",required_argument,   NULL,  'z'},
    {"wgt_mode",    required_argument,   NULL,  'W'},
    {"process_num", required_argument,   NULL,  'P'},
    {"user_usage",  no_argument,         NULL,  'H'},
  };

  if (argc == 2 && atoi(argv[1]) == -1) {
    example_test_cmd();
    return 0;
  }

  atomic_init(&num_threads, 0);
  atomic_init(&threads_running, 1);

  bm_handle_t handle = NULL;
  int src_h = 288, src_w = 4608, dst_w = 11520, dst_h = 288;
  bm_image_format_ext src_fmt = FORMAT_YUV420P, dst_fmt = FORMAT_YUV420P;
  char *src_name[4] = {NULL}, *dst_name = NULL, *wgt_name[6] = {NULL},*compare_name=NULL;
  int loop_time = 1;
  int  dev_id = 0, thread_num = 1, wgt_len = 0, input_num = 4;

  struct stitch_param stitch_config;
  memset(&stitch_config, 0, sizeof(stitch_config));
  stitch_config.wgt_mode = BM_STITCH_WGT_YUV_SHARE;

  signal(SIGINT, blend_HandleSig);
  signal(SIGTERM, blend_HandleSig);

  int ch = 0, opt_idx = 0;
  while ((ch = getopt_long(argc, argv, "T:N:a:b:c:d:e:f:g:h:i:j:k:l:m:n:o:p:q:r:s:t:u:v:w:x:y:z:W:P:H", long_options, &opt_idx)) != -1) {
    switch (ch) {
      case 'T':
        thread_num = atoi(optarg);
        break;
      case 'N':
        input_num = atoi(optarg);
        break;
      case 'a':
        src_name[0] = optarg;
        break;
      case 'b':
        src_name[1] = optarg;
        break;
      case 'c':
        src_name[2] = optarg;
        break;
      case 'd':
        src_name[3] = optarg;
        break;
      case 'e':
        src_w = atoi(optarg);
        break;
      case 'f':
        src_h = atoi(optarg);
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
      case 'n':
        stitch_config.ovlap_attr.ovlp_lx[1] = atoi(optarg);
        break;
      case 'o':
        stitch_config.ovlap_attr.ovlp_rx[1] = atoi(optarg);
        break;
      case 'p':
        stitch_config.ovlap_attr.ovlp_lx[2] = atoi(optarg);
        break;
      case 'q':
        stitch_config.ovlap_attr.ovlp_rx[2] = atoi(optarg);
        break;
      case 'r':
        wgt_name[0] = optarg;
        break;
      case 's':
        wgt_name[1] = optarg;
        break;
      case 't':
        wgt_name[2] = optarg;
        break;
      case 'u':
        wgt_name[3] = optarg;
        break;
      case 'v':
        wgt_name[4] = optarg;
        break;
      case 'w':
        wgt_name[5] = optarg;
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
      case 'P':
        process_num = atoi(optarg);
        break;
      case 'H':
        user_usage();
        return 0;
    }
  }

  bm_status_t ret    = bm_dev_request(&handle, dev_id);
  if (ret != BM_SUCCESS) {
      printf("Create bm handle failed. ret = %d\n", ret);
      exit(-1);
  }
  // test for multi-thread
  pthread_t pid[thread_num];

  cv_blending4_thread_arg_t cv_blending4_thread_arg[thread_num];
  for (int i = 0; i < thread_num; i++) {
        cv_blending4_thread_arg[i].input_num = input_num;
        cv_blending4_thread_arg[i].src_name[0] = src_name[0];
        cv_blending4_thread_arg[i].src_name[1] = src_name[1];
        cv_blending4_thread_arg[i].src_name[2] = src_name[2];
        cv_blending4_thread_arg[i].src_name[3] = src_name[3];
        cv_blending4_thread_arg[i].src_w = src_w;
        cv_blending4_thread_arg[i].src_h = src_h;
        cv_blending4_thread_arg[i].src_fmt = src_fmt;
        cv_blending4_thread_arg[i].dst_name = dst_name;
        cv_blending4_thread_arg[i].dst_w = dst_w;
        cv_blending4_thread_arg[i].dst_h = dst_h;
        cv_blending4_thread_arg[i].dst_fmt = dst_fmt;
        cv_blending4_thread_arg[i].wgt_name[0] = wgt_name[0];
        cv_blending4_thread_arg[i].wgt_name[1] = wgt_name[1];
        cv_blending4_thread_arg[i].wgt_name[2] = wgt_name[2];
        cv_blending4_thread_arg[i].wgt_name[3] = wgt_name[3];
        cv_blending4_thread_arg[i].wgt_name[4] = wgt_name[4];
        cv_blending4_thread_arg[i].wgt_name[5] = wgt_name[5];
        cv_blending4_thread_arg[i].loop_time = loop_time;
        cv_blending4_thread_arg[i].dev_id = dev_id;
        cv_blending4_thread_arg[i].wgt_len = wgt_len;
        cv_blending4_thread_arg[i].compare_name = compare_name;
        cv_blending4_thread_arg[i].stitch_config.wgt_mode = stitch_config.wgt_mode;
        cv_blending4_thread_arg[i].stitch_config.ovlap_attr.ovlp_rx[2] = stitch_config.ovlap_attr.ovlp_rx[2];
        cv_blending4_thread_arg[i].stitch_config.ovlap_attr.ovlp_lx[2] = stitch_config.ovlap_attr.ovlp_lx[2];
        cv_blending4_thread_arg[i].stitch_config.ovlap_attr.ovlp_rx[1] = stitch_config.ovlap_attr.ovlp_rx[1];
        cv_blending4_thread_arg[i].stitch_config.ovlap_attr.ovlp_lx[1] = stitch_config.ovlap_attr.ovlp_lx[1];
        cv_blending4_thread_arg[i].stitch_config.ovlap_attr.ovlp_lx[0] = stitch_config.ovlap_attr.ovlp_lx[0];
        cv_blending4_thread_arg[i].stitch_config.ovlap_attr.ovlp_rx[0] = stitch_config.ovlap_attr.ovlp_rx[0];
        cv_blending4_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_blending, &cv_blending4_thread_arg[i]) != 0) {
            printf("create thread failed\n");
            return -1;
        }
  }
  for (int i = 0; i < thread_num; i++) {
      ret = pthread_join(pid[i], NULL);
      if (ret != 0) {
          printf("Thread join failed\n");
          exit(-1);
      }
  }

  bm_dev_free(handle);

  return ret;
}