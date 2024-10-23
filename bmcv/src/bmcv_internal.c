#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>
#include "bmcv_internal.h"
#include "test_misc.h"

#define bm_min(x, y) (((x)) < ((y)) ? (x) : (y))
#define bm_max(x, y) (((x)) > ((y)) ? (x) : (y))

#ifdef __linux__
#define __USE_GNU
#include <dlfcn.h>
#endif
#ifdef _WIN32
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#endif
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __attribute__((visibility("default")))
#endif
#define FIRMWARE_NAME "libbm1688_kernel_module.so"
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

DLLEXPORT const char *libbmcv_version_string =
    "libbmcv_version: " LIBSOPHAV_VERSION
    ", branch: " BRANCH
    ", minor version: " COMMIT_COUNT
    ", commit hash: " COMMIT_HASH
    ", commit date: " COMMIT_DATE;

int array_cmp_float(float *p_exp, float *p_got, int len, float delta)
{
    int idx = 0;

    for (idx = 0; idx < len; idx++) {
        if (bm_max(fabs(p_exp[idx]), fabs(p_got[idx])) > 1.0) {
            if (fabs(p_exp[idx] - p_got[idx]) / bm_max(fabs(p_exp[idx]), fabs(p_got[idx])) > delta) {
                printf("err at index %d exp %.20f got %.20f, diff=%.20f\n",
                     idx, p_exp[idx], p_got[idx], p_exp[idx] - p_got[idx]);
                return -1;
            }
        } else {
            if (fabs(p_exp[idx] - p_got[idx]) > delta) {
                printf("err at index %d exp %.20f got %.20f, diff=%.20f\n",
                 idx, p_exp[idx], p_got[idx], p_exp[idx] - p_got[idx]);
                return -1;
            }
        }
    }
    return 0;
}

int find_tpufirmaware_path(char fw_path[512], const char* path){
    char* ptr;
    int dirname_len;
    int ret = 0;
#ifdef __linux__
    Dl_info dl_info;
    const char* path1 = "/opt/sophon/libsophon-current/lib/tpu_module/";

    /* 1.test ./libbm1688_kernel_module.so */
    ret = access(path, F_OK);
    if (ret == 0){
        memset(fw_path, 0, 512);
        strcpy(fw_path, path);
        return ret;
    }

    /* 2. test /system/data/lib/vpu_firmware/chagall.bin */
    memset(fw_path, 0, 512);
    strcpy(fw_path, path1);
    strcat(fw_path, path);
    ret = access(fw_path, F_OK);
    if (ret == 0)
    {
        return ret;
    }

    /* 3.test libbmcv_so_path/libbm1688_kernel_module.so */
    ret = dladdr((void*)find_tpufirmaware_path, &dl_info);
    if (ret == 0){
        printf("dladdr() failed: %s\n", dlerror());
        return -1;
    }
    if (dl_info.dli_fname == NULL){
        printf("%s is NOT a symbol\n", __FUNCTION__);
        return -1;
    }

    ptr = (char*)strrchr(dl_info.dli_fname, '/');
    if (!ptr){
        printf("Invalid absolute path name of libbmvpu.so\n");
        return -1;
    }

    dirname_len = ptr - dl_info.dli_fname + 1;
    if (dirname_len <= 0){
        printf("Invalid length of folder name\n");
        return -1;
    }

    memset(fw_path, 0, 512);
    strncpy(fw_path, dl_info.dli_fname, dirname_len);
    strcat(fw_path, "tpu_module/");
    strcat(fw_path, path);

    ret = access(fw_path, F_OK);
    return ret;
#endif

#ifdef _WIN32
    strcpy(fw_path, ".\\libbm1688_kernel_module.so");

    /* 1. test ./libbm1688_kernel_module.so */
    ret = _access(fw_path, 0);
    if (ret == 0){
        return ret;
    }

    /* 2. test libbmcv_so_path/libbm1688_kernel_module.so */
    LPTSTR  strDLLPath1 = (char*)malloc(512);
    GetModuleFileName((HINSTANCE)&__ImageBase, strDLLPath1, _MAX_PATH);

    ptr = strrchr(strDLLPath1, '\\');

    if (!ptr){
        printf("Invalid absolute path name of libbmvpu.so\n");
        return -1;
    }

    dirname_len = strlen(strDLLPath1) - strlen(ptr) + 1;
    if (dirname_len <= 0)
    {
        printf("Invalid length of folder name\n");
        return -1;
    }
    memset(fw_path, 0, 512);
    strncpy(fw_path, strDLLPath1, dirname_len);
    strcat(fw_path, "tpu_module\\");
    strcat(fw_path, path);

    free(strDLLPath1);
    ret = _access(fw_path, 0);
    if (ret == 0){
        return ret;
    }
    return -1;
#endif
}

bm_status_t bm_load_tpu_module(bm_handle_t handle, tpu_kernel_module_t *tpu_module, int core_id){
    const  char* fw_fname = FIRMWARE_NAME;
    static char fw_path[512] = {0};
    static bool first = true;
    if(first){
        if(0 != find_tpufirmaware_path(fw_path, fw_fname)){
            printf("libbm1688_kernel_module.so does not exist\n");
            return BM_ERR_FAILURE;
        }
        first = false;
    }
    int key_size = strlen(FIRMWARE_NAME);
    *tpu_module = tpu_kernel_load_module_file_key_to_core(handle,
                                                 fw_path,
                                                 FIRMWARE_NAME,
                                                 key_size,
                                                 core_id);
    if(*tpu_module == NULL){
        printf("tpu_module is null\n");
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t bm_tpu_kernel_launch(bm_handle_t handle,
                         const char *func_name,
                         void       *param,
                         size_t      size,
                         int         core_id){
    tpu_kernel_module_t tpu_module = NULL;
    tpu_kernel_function_t func_id = 0;
    bm_status_t ret = BM_SUCCESS;
    ret = bm_load_tpu_module(handle, &tpu_module, core_id);
    if(ret != BM_SUCCESS){
        printf("module load error! \n");
        return BM_ERR_FAILURE;
    }
    func_id = tpu_kernel_get_function_from_core(handle, tpu_module, (char*)func_name, core_id);
    ret = tpu_kernel_launch_from_core(handle, func_id, param, size, core_id);
    if(BM_SUCCESS != tpu_kernel_free_module(handle, tpu_module)){
        printf("tpu module unload failed \n");
        return BM_ERR_FAILURE;
    }
    return ret;
}

bm_status_t bm_kernel_main_launch(bm_handle_t handle, int api_id, void *param, size_t size)
{
    bm_status_t ret = BM_SUCCESS;
    size_t full_size = sizeof(struct dynamic_load_param) + size;
    struct dynamic_load_param* full_param = (struct dynamic_load_param*)malloc(full_size * sizeof(unsigned char));
    full_param->api_id = api_id;
    full_param->size = size;
    memcpy(full_param->param, param, size);

    tpu_kernel_module_t tpu_module = NULL;
    tpu_kernel_function_t func_id = 0;
    ret = bm_load_tpu_module(handle, &tpu_module, 0);
    if(ret != BM_SUCCESS){
        printf("module load error! \n");
        free(full_param);
        return BM_ERR_FAILURE;
    }
    func_id = tpu_kernel_get_function_from_core(handle, tpu_module, "tpu_kernel_main", 0);
    ret = tpu_kernel_launch_from_core(handle, func_id, full_param, full_size, 0);
    if(ret != BM_SUCCESS){
        printf("tpu_kernel_launch_from_core error! \n");
        free(full_param);
        return BM_ERR_FAILURE;
    }
    ret = tpu_kernel_unload_module_from_core(handle, tpu_module, 0);
    if(ret != BM_SUCCESS){
        printf("tpu module unload failed \n");
        free(full_param);
        return BM_ERR_FAILURE;
    }
    free(full_param);
    return ret;
}

int array_cmp_fix16b(void *p_exp, void *p_got, int sign, int len,  int delta)
{
    int idx = 0;
    int exp_int = 0;
    int got_int = 0;
    int error = 0;

    for (idx = 0; idx < len; idx++) {
        if (sign) {
            exp_int = (int)(*((short *)p_exp + idx));
            got_int = (int)(*((short *)p_got + idx));
        } else {
            exp_int = (int)(*((unsigned short *)p_exp + idx));
            got_int = (int)(*((unsigned short *)p_got + idx));
        }
        error = abs(exp_int - got_int);

        if (error > delta) {
            printf("error at index %d exp %d got %d\n", idx, exp_int, got_int);
            return -1;
        }
    }

    return 0;
}

int array_cmp_fix8b(void *p_exp, void *p_got, int sign, int len, int delta)
{
    int idx = 0;
    int error = 0;
    int exp_int = 0;
    int got_int = 0;

    for (idx = 0; idx < len; idx++) {

        if (sign) {
            exp_int = (int)(*((char *)p_exp + idx));
            got_int = (int)(*((char *)p_got + idx));
        } else {
            exp_int = (int)(*((unsigned char *)p_exp + idx));
            got_int = (int)(*((unsigned char *)p_got + idx));
        }
        error = abs(exp_int - got_int);

        if (error > delta) {
            printf("error at index %d exp %d got %d\n",idx, exp_int, got_int);
            return -1;
        }
    }

  return 0;
}

bm_status_t sg_malloc_device_mem(bm_handle_t handle, sg_device_mem_st *pmem, unsigned int size) {
    if (BM_SUCCESS != bm_malloc_device_byte(handle, &(pmem->bm_device_mem), size)) {
        pmem->flag = 0;
        return BM_ERR_NOMEM;
    }
    pmem->flag = 1;
    return BM_SUCCESS;
}

bm_status_t sg_image_alloc_dev_mem(bm_image image, int heap_id) {
    if (BM_SUCCESS != bm_image_alloc_dev_mem(image, heap_id)) {
        return BM_ERR_NOMEM;
    }
    return BM_SUCCESS;
}

void sg_free_device_mem(bm_handle_t handle, sg_device_mem_st mem) {
    if (mem.flag == 1)
        bm_free_device(handle, mem.bm_device_mem);
}

static uint16_t fp32idata_to_fp16idata(uint32_t f,int round_method)
{
    uint32_t f_exp;
    uint32_t f_sig;
    uint32_t h_sig = 0x0u;
    uint16_t h_sgn, h_exp;

    h_sgn = (uint16_t)((f & 0x80000000u) >> 16);
    f_exp = (f & 0x7f800000u);

    /* Exponent overflow/NaN converts to signed inf/NaN */
    if (f_exp >= 0x47800000u) {
        if (f_exp == 0x7f800000u) { /* Inf or NaN */
            f_sig = (f & 0x007fffffu);
            if (f_sig != 0) {
                return 0x7fffu; /* NaN */
            } else {
                return (uint16_t)(h_sgn + 0x7c00u); /* signed inf */
            }
        } else {
            return (uint16_t)(h_sgn + 0x7c00u); /* overflow to signed inf */
      }
    }

/* Exponent underflow converts to a subnormal half or signed zero */
#ifdef CUDA_T
    if (f_exp <= 0x38000000u)////exp= -15, fp16is denormal ,the smallest value of fp16 normal = 1x2**(-14)
#else
    if (f_exp < 0x38000000u)
#endif
    {
    /*
     * Signed zeros, subnormal floats, and floats with small
     * exponents all convert to signed zero half-floats.
     */
        if (f_exp < 0x33000000u) {
            return h_sgn;
        }
        /* Make the subnormal significand */
        f_exp >>= 23;
        f_sig = (0x00800000u + (f & 0x007fffffu));

/* Handle rounding by adding 1 to the bit beyond half precision */
// ROUND_TO_NEAREST_EVEN
    /*
     * If the last bit in the half significand is 0 (already even), and
     * the remaining bit pattern is 1000...0, then we do not add one
     * to the bit after the half significand.  In all other cases, we do.
     */
        switch (round_method) {
            case 0:
                if ((f_sig & ((0x01u << (127 - f_exp)) - 1)) != (0x01u << (125 - f_exp))) {
                    f_sig += 1 << (125 - f_exp);
                }
                f_sig >>= (126 - f_exp);
                h_sig = (uint16_t)(f_sig);
                break;
            case 1:
                f_sig += 1 << (125 - f_exp); /*JUST ROUND */
                f_sig >>= (126 - f_exp);
                h_sig = (uint16_t)(f_sig);
                break;
            case 2:
                f_sig >>= (126 - f_exp); /* ROUND_TO_ZERO */
                h_sig = (uint16_t)(f_sig);
                break;
            case 3:
                if ((f_sig & ((0x01u << (126 - f_exp)) - 1)) != 0x0u ) { /*JUST_FLOOR*/
                    if(h_sgn)
                    f_sig  += 1 << (126 - f_exp);
                }
                f_sig >>= (126 - f_exp);
                h_sig = (uint16_t)(f_sig);
                break;
            case 4:
                if ((f_sig & ((0x01u << (126 - f_exp)) - 1)) != 0x0u ) { /*just_ceil */
                    if( !h_sgn)
                    f_sig  += 1 << (126 - f_exp);
                }
                f_sig >>= (126 - f_exp);
                h_sig = (uint16_t)(f_sig);
                break;
            default:
                f_sig >>= (126 - f_exp); /* ROUND_TO_ZERO */
                h_sig = (uint16_t)(f_sig);
                break;
        }
    /*
     * If the rounding causes a bit to spill into h_exp, it will
     * increment h_exp from zero to one and h_sig will be zero.
     * This is the correct result.
     */
        return (uint16_t)(h_sgn + h_sig);
    }

    /* Regular case with no overflow or underflow */
    h_exp = (uint16_t)((f_exp - 0x38000000u) >> 13);
    /* Handle rounding by adding 1 to the bit beyond half precision */
    f_sig = (f & 0x007fffffu);

// ROUND_TO_NEAREST_EVEN
/*
    * If the last bit in the half significand is 0 (already even), and
    * the remaining bit pattern is 1000...0, then we do not add one
    * to the bit after the half significand.  In all other cases, we do.
    */
    switch (round_method) {
        case 0:
            if ((f_sig & 0x00003fffu) != 0x00001000u) {
              f_sig = f_sig + 0x00001000u;
            }
              h_sig = (uint16_t)(f_sig >> 13);
              break;
        case 1:
              f_sig = f_sig + 0x00001000u; // JUST_ROUND
              h_sig = (uint16_t)(f_sig >> 13);
              break;
        case 2:
            h_sig = (uint16_t)(f_sig >> 13); // ROUND_TO_ZERO
            break;
        case 3:
            if ((f_sig & 0x00001fffu) != 0x00000000u) { // JUST_FLOOR
              if ( h_sgn ) {
                f_sig = f_sig + 0x00002000u;
              }
            }
            h_sig = (uint16_t)(f_sig >> 13);
            break;
        case 4:
            if ((f_sig & 0x00001fffu) != 0x00000000u) { // JUST_CEIL
              if ( !h_sgn )
                f_sig = f_sig + 0x00002000u;
            }
            h_sig = (uint16_t)(f_sig >> 13);
            break;
        default:
            h_sig = (uint16_t)(f_sig >> 13); // ROUND_TO_ZERO
            break;
    }
/*
* If the rounding causes a bit to spill into h_exp, it will
* increment h_exp by one and h_sig will be zero.  This is the
* correct result.  h_exp may increment to 15, at greatest, in
* which case the result overflows to a signed inf.
*/

  return h_sgn + h_exp + h_sig;
}

static void set_denorm(void)
{
#ifdef __x86_64__
    const int MXCSR_DAZ = (1<<6);
    const int MXCSR_FTZ = (1<<15);
    unsigned int mxcsr = __builtin_ia32_stmxcsr ();
    mxcsr |= MXCSR_DAZ | MXCSR_FTZ;
    __builtin_ia32_ldmxcsr (mxcsr);
#endif
}

int dtype_size(enum bm_data_type_t data_type)
{
    int size = 0;
    switch (data_type) {
        case DT_FP32:   size = 4; break;
        case DT_UINT32: size = 4; break;
        case DT_INT32:  size = 4; break;
        case DT_FP16:   size = 2; break;
        case DT_BFP16:  size = 2; break;
        case DT_INT16:  size = 2; break;
        case DT_UINT16: size = 2; break;
        case DT_INT8:   size = 1; break;
        case DT_UINT8:  size = 1; break;
        default: break;
    }
    return size;
}

fp16 fp32tofp16 (float A,int round_method)
{
    union fp16_data res;
    union fp32_data ina;

    ina.fdata = A;
    //// VppInfo("round_method =%d  \n",round_method);
    set_denorm();
    res.idata = fp32idata_to_fp16idata(ina.idata, round_method);
    return res.ndata;
}

float fp16tofp32(fp16 h)
{
    union fp16_data dfp16;
    dfp16.ndata = h;
    union fp32_data ret32;
    uint32_t sign = (dfp16.idata & 0x8000) << 16;
    uint32_t mantissa = (dfp16.idata & 0x03FF);
    uint32_t exponent = (dfp16.idata & 0x7C00) >> 10;
    float result;

    if (exponent == 0) {
        if (mantissa == 0) {
            //return *((float*)(&sign));
            memcpy(&result, &sign, sizeof(float));
            return result;
        } else {
            exponent = 1;
            while ((mantissa & 0x0400) == 0) {
                mantissa <<= 1;
                exponent++;
            }
            mantissa &= 0x03FF;
            exponent = (127 - 15 - exponent) << 23;
            mantissa <<= 13;
            ret32.idata = *((uint32_t*)(&sign)) | (*((uint32_t*)(&mantissa))) | \
                        (*((uint32_t*)(&exponent)));
            //return ret32.fdata;
            memcpy(&result, &(ret32.fdata), sizeof(float));
            return result;
        }
    } else if (exponent == 0x1F) {
        if (mantissa == 0) {
            //return *((float*)(&sign)) / 0.0f;
            memcpy(&result, &sign, sizeof(float));
            return result / 0.0f;
        } else {
            //return *((float*)(&sign)) / 0.0f;
            memcpy(&result, &sign, sizeof(float));
            return result / 0.0f;
        }
    } else {
        exponent = (exponent + (127 - 15)) << 23;
        mantissa <<= 13;
        ret32.idata = *((uint32_t*)(&sign)) | (*((uint32_t*)(&mantissa))) | \
                    (*((uint32_t*)(&exponent)));
        //return ret32.fdata;
        memcpy(&result, &(ret32.fdata), sizeof(float));
        return result;
    }
}

// extern bm_status_t bm_get_chipid(bm_handle_t handle, u32 *chipid);
// extern s32 CVI_SYS_Munmap(void *pVirAddr, u32 u32Size);
pthread_mutex_t fd_mutex = PTHREAD_MUTEX_INITIALIZER;

void bmcv_err_log_internal(char *      log_buffer,
                           size_t      string_sz,
                           const char *frmt,
                           ...) {
    va_list args;
    va_start(args, frmt);
    vsnprintf(log_buffer + string_sz, MAX_BMCV_LOG_BUFFER_SIZE, frmt, args);
    va_end(args);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "%s", log_buffer);
}

plane_layout set_plane_layout(int n, int c, int h, int w, int Dsize){

    int pitch_stride = Dsize * w;
    int channel_stride = pitch_stride * h;
    int batch_stride = channel_stride * c;
    int size = batch_stride * n;
    plane_layout _plane_layout = {
        n,
        c,
        h,
        w,
        Dsize,
        pitch_stride,
        channel_stride,
        batch_stride,
        size
    };
    return _plane_layout;
}

plane_layout align_width(plane_layout src, int align){
    ASSERT(align > 0 && ((align & (align - 1)) == 0));
    plane_layout res = src;
    res.pitch_stride     = ALIGN(res.W * res.data_size, align);
    res.channel_stride  = res.pitch_stride * res.H;
    res.batch_stride     = res.channel_stride * res.C;
    res.size             = res.batch_stride * res.N;
    return res;
}

plane_layout align_height(plane_layout src, int align){
    ASSERT(align > 0 && ((align & (align - 1)) == 0));
    plane_layout res = src;
    res.channel_stride  = res.pitch_stride * ALIGN(res.H, align);
    res.batch_stride     = res.channel_stride * res.C;
    res.size             = res.batch_stride * res.N;
    return res;
}

plane_layout align_channel(plane_layout  src,
                           int           pitch_align,
                           int           height_align){
    ASSERT(pitch_align > 0 && ((pitch_align & (pitch_align - 1)) == 0));
    ASSERT(height_align > 0 && ((height_align & (height_align - 1)) == 0));
    plane_layout res = src;

    res.pitch_stride = ALIGN(res.W * res.data_size, pitch_align);
    res.channel_stride  = res.pitch_stride * ALIGN(res.H, height_align);
    res.batch_stride     = res.channel_stride * res.C;
    res.size             = res.batch_stride * res.N;
    return res;
}

plane_layout stride_width(plane_layout src, int stride){
    plane_layout res = src;
    ASSERT_INFO(stride >= res.data_size * res.W && stride % res.data_size == 0,
        "invalidate stride: W_stride=%d, W=%d, data_size=%d\n", stride, res.W, res.data_size);
    res.pitch_stride    = stride;
    res.channel_stride  = res.pitch_stride * res.H;
    res.batch_stride     = res.channel_stride * res.C;
    res.size             = res.batch_stride * res.N;
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE,"stride_width stride=%d \n",stride);
    return res;
}

bm_status_t update_memory_layout(bm_handle_t     handle,
                                  bm_device_mem_t src,
                                  plane_layout    src_layout,
                                  bm_device_mem_t dst,
                                  plane_layout    dst_layout){
    ASSERT(src_layout.data_size == dst_layout.data_size &&
            src_layout.C == dst_layout.C && src_layout.N == dst_layout.N);

  int N = bm_min(src_layout.N, dst_layout.N);
  int C = bm_min(src_layout.C, dst_layout.C);
  int H = bm_min(src_layout.H, dst_layout.H);
  int W = bm_min(src_layout.W, dst_layout.W);

  bm_api_cv_width_align_t api = {bm_mem_get_device_addr(src),
                                 bm_mem_get_device_addr(dst),
                                 N,
                                 C,
                                 H,
                                 W,
                                 src_layout.batch_stride / src_layout.data_size,
                                 src_layout.channel_stride / src_layout.data_size,
                                 src_layout.pitch_stride / src_layout.data_size,
                                 dst_layout.batch_stride / dst_layout.data_size,
                                 dst_layout.channel_stride / dst_layout.data_size,
                                 dst_layout.pitch_stride / dst_layout.data_size,
                                 src_layout.data_size
                                 };
    int core_id = 0;
    unsigned int chipid;
    bm_status_t ret = BM_SUCCESS;

    ret = bm_get_chipid(handle, &chipid);
    if (ret != BM_SUCCESS) {
        printf("get chipid is error !\n");
        return BM_ERR_DEVNOTREADY;
    }

    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_width_align", (u8 *)&api, \
                                                sizeof(api), core_id);
            if(BM_SUCCESS != ret){
                bmlib_log("WIDTH_ALIGN", BMLIB_LOG_ERROR, "width_align sync api error\n");
                return ret;
            }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_ERR_NOFEATURE;
            break;
    }

    return ret;
}

void algorithm_to_str(bmcv_resize_algorithm algorithm, char* res)
{
  switch(algorithm)
  {
    case BMCV_INTER_NEAREST:
      strcpy(res, "BMCV_INTER_NEAREST");
      break;
    case BMCV_INTER_LINEAR:
      strcpy(res, "BMCV_INTER_LINEAR");
      break;
    case BMCV_INTER_BICUBIC:
      strcpy(res, "BMCV_INTER_BICUBIC");
      break;
    default:
      printf("%s:%d[%s] Not found such algorithm.\n",__FILE__, __LINE__, __FUNCTION__);
      break;
  }
}

void format_to_str(bm_image_format_ext format, char* res)
{
    switch(format)
    {
        case FORMAT_YUV420P:
            strcpy(res, "FORMAT_YUV420P");
            break;
        case FORMAT_YUV422P:
            strcpy(res, "FORMAT_YUV422P");
            break;
        case FORMAT_YUV444P:
            strcpy(res, "FORMAT_YUV444P");
            break;
        case FORMAT_NV12:
            strcpy(res, "FORMAT_NV12");
            break;
        case FORMAT_NV21:
            strcpy(res, "FORMAT_NV21");
            break;
        case FORMAT_NV16:
            strcpy(res, "FORMAT_NV16");
            break;
        case FORMAT_NV61:
            strcpy(res, "FORMAT_NV61");
            break;
        case FORMAT_NV24:
            strcpy(res, "FORMAT_NV24");
            break;
        case FORMAT_RGB_PLANAR:
            strcpy(res, "FORMAT_RGB_PLANAR");
            break;
        case FORMAT_BGR_PLANAR:
            strcpy(res, "FORMAT_BGR_PLANAR");
            break;
        case FORMAT_RGB_PACKED:
            strcpy(res, "FORMAT_RGB_PACKED");
            break;
        case FORMAT_BGR_PACKED:
            strcpy(res, "FORMAT_BGR_PACKED");
            break;
        case FORMAT_ARGB_PACKED:
            strcpy(res, "FORMAT_ARGB_PACKED");
            break;
        case FORMAT_ABGR_PACKED:
            strcpy(res, "FORMAT_ABGR_PACKED");
            break;
        case FORMAT_RGBP_SEPARATE:
            strcpy(res, "FORMAT_RGBP_SEPARATE");
            break;
        case FORMAT_BGRP_SEPARATE:
            strcpy(res, "FORMAT_BGRP_SEPARATE");
            break;
        case FORMAT_GRAY:
            strcpy(res, "FORMAT_GRAY");
            break;
        case FORMAT_COMPRESSED:
            strcpy(res, "FORMAT_COMPRESSED");
            break;
        case FORMAT_HSV_PLANAR:
            strcpy(res, "FORMAT_HSV_PLANAR");
            break;
        case FORMAT_YUV444_PACKED:
            strcpy(res, "FORMAT_YUV444_PACKED");
            break;
        case FORMAT_YVU444_PACKED:
            strcpy(res, "FORMAT_YVU444_PACKED");
            break;
        case FORMAT_YUV422_YUYV:
            strcpy(res, "FORMAT_YUV422_YUYV");
            break;
        case FORMAT_YUV422_YVYU:
            strcpy(res, "FORMAT_YUV422_YVYU");
            break;
        case FORMAT_YUV422_UYVY:
            strcpy(res, "FORMAT_YUV422_UYVY");
            break;
       case FORMAT_YUV422_VYUY:
            strcpy(res, "FORMAT_YUV422_VYUY");
            break;
       case FORMAT_RGBYP_PLANAR:
            strcpy(res, "FORMAT_RGBYP_PLANAR");
            break;
       case FORMAT_HSV180_PACKED:
            strcpy(res, "FORMAT_HSV180_PACKED");
            break;
       case FORMAT_HSV256_PACKED:
            strcpy(res, "FORMAT_HSV256_PACKED");
            break;
        case FORMAT_ARGB4444_PACKED:
            strcpy(res, "FORMAT_ARGB4444_PACKED");
            break;
        case FORMAT_ABGR4444_PACKED:
            strcpy(res, "FORMAT_ABGR4444_PACKED");
            break;
        case FORMAT_ARGB1555_PACKED:
            strcpy(res, "FORMAT_ARGB1555_PACKED");
            break;
        case FORMAT_ABGR1555_PACKED:
            strcpy(res, "FORMAT_ABGR1555_PACKED");
            break;

        default:
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_WARNING, "Not found such format%s: %s: %d\n",
                  filename(__FILE__), __func__, __LINE__);
            break;
    }
}

int is_csc_yuv_or_rgb(bm_image_format_ext format)
{
    int ret = COLOR_SPACE_YUV;

    switch(format)
    {
        case FORMAT_YUV420P:
        case FORMAT_YUV422P:
        case FORMAT_YUV444P:
        case FORMAT_NV12:
        case FORMAT_NV21:
        case FORMAT_NV16:
        case FORMAT_NV61:
        case FORMAT_NV24:
        case FORMAT_GRAY:
        case FORMAT_COMPRESSED:
        case FORMAT_YUV444_PACKED:
        case FORMAT_YVU444_PACKED:
        case FORMAT_YUV422_YUYV:
        case FORMAT_YUV422_YVYU:
        case FORMAT_YUV422_UYVY:
        case FORMAT_YUV422_VYUY:
            ret = COLOR_SPACE_YUV;
            break;
        case FORMAT_RGB_PLANAR:
        case FORMAT_BGR_PLANAR:
        case FORMAT_RGB_PACKED:
        case FORMAT_BGR_PACKED:
        case FORMAT_RGBP_SEPARATE:
        case FORMAT_BGRP_SEPARATE:
        case FORMAT_ARGB_PACKED:
        case FORMAT_ABGR_PACKED:
            ret = COLOR_SPACE_RGB;
            break;
        case FORMAT_HSV_PLANAR:
        case FORMAT_HSV180_PACKED:
        case FORMAT_HSV256_PACKED:
            ret = COLOR_SPACE_HSV;
            break;
        case FORMAT_RGBYP_PLANAR:
            ret = COLOR_SPACE_RGBY;
            break;
        default:
            ret = COLOR_NOT_DEF;
            break;
    }
    return ret;
}

void bm_read_bin(bm_image src, const char *input_name)
{
    int image_byte_size[4] = {0};
    for (int i = 0; i < src.image_private->plane_num; i++) {
        image_byte_size[i] = src.image_private->memory_layout[i].size;
    }
    int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
    char* input_ptr = (char *)malloc(byte_size);

    void* in_ptr[4] = {(void*)input_ptr,
                     (void*)((char*)input_ptr + image_byte_size[0]),
                     (void*)((char*)input_ptr + image_byte_size[0] + image_byte_size[1]),
                     (void*)((char*)input_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
    // bm_image_copy_device_to_host(src, in_ptr);
    FILE *fp_src = fopen(input_name, "rb");
    // for (int i = 0; i < src.image_private->plane_num; i++) {
    //     if (fread((void *)in_ptr[i], 1, image_byte_size[i], fp_src) < (unsigned int)image_byte_size[i]){
    //         printf("file size is less than %d required bytes\n", image_byte_size[i]);
    //         printf("0x%lx, 0x%llx\n", src.image_private->data[i].u.device.device_addr, (unsigned long long)in_ptr[i]);
    //     }
    //     CVI_SYS_Munmap(in_ptr[i], image_byte_size[i]);
    // }
    if (fread((void *)input_ptr, 1, byte_size, fp_src) < (unsigned int)byte_size){
        printf("file size is less than %d required bytes\n", byte_size);
    };
    fclose(fp_src);
    bm_image_copy_host_to_device(src, (void **)in_ptr);
    free(input_ptr);
    return;
}

void bm_write_bin(bm_image dst, const char *output_name)
{
    int image_byte_size[4] = {0};
    char md5_str[MD5_STRING_LENGTH + 1];
    for (int i = 0; i < dst.image_private->plane_num; i++) {
        image_byte_size[i] = dst.image_private->memory_layout[i].size;
    }

    int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
    unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
    void* out_ptr[4] = {(void*)output_ptr,
                      (void*)((unsigned char*)output_ptr + image_byte_size[0]),
                      (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                      (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};

    bm_image_copy_device_to_host(dst, (void **)out_ptr);
    md5_get(output_ptr, byte_size, md5_str, 1);
    FILE *fp_dst = fopen(output_name, "wb");
    fwrite((void *)output_ptr, 1, byte_size, fp_dst);
    fclose(fp_dst);
    free(output_ptr);
    return;
}

bm_status_t open_binary_to_alpha(
    bm_handle_t           handle,
    char *                src_name,
    int                   src_size,
    int                   transparency,
    bm_device_mem_t *     dst)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned char * src = (unsigned char *)malloc(sizeof(unsigned char) * src_size);
    unsigned char * dst_addr = (unsigned char *)malloc(sizeof(unsigned char) * src_size * 8);

    ret = bm_malloc_device_byte(handle, dst, src_size * 8);
    if(ret != BM_SUCCESS){
        printf("bm_malloc_device_byte fail %s: %s: %d\n", __FILE__, __func__, __LINE__);
        goto fail;
    }

    FILE * fp_src = fopen(src_name, "rb");
    size_t read_size = fread((void *)src, src_size, 1, fp_src);
    printf("fread %ld byte\n", read_size);
    fclose(fp_src);

    for(int i=0; i<(src_size * 8); i++){
        int watermask_idx = i / 8;
        int binary_idx = (i % 8);
        dst_addr[i] = ((src[watermask_idx] >> binary_idx) & 1) * transparency;
    }
#ifdef _FPGA
    ret = bm_memcpy_s2d_fpga(handle, dst[0], (void*)dst_addr);
#else
    ret = bm_memcpy_s2d(handle, dst[0], (void*)dst_addr);
#endif
    if(ret != BM_SUCCESS){
        printf("bm_memcpy_d2s fail %s: %s: %d\n", __FILE__, __func__, __LINE__);
    }

fail:
    free(src);
    free(dst_addr);
    return ret;
}

bm_status_t open_water(
    bm_handle_t           handle,
    char *                src_name,
    int                   src_size,
    bm_device_mem_t *     dst)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned char * src = (unsigned char *)malloc(sizeof(unsigned char) * src_size);
    ret = bm_malloc_device_byte(handle, dst, src_size);
    if(ret != BM_SUCCESS){
        printf("bm_malloc_device_byte fail %s: %s: %d\n", __FILE__, __func__, __LINE__);
        goto fail;
    }

    FILE * fp_src = fopen(src_name, "rb");
    size_t read_size = fread((void *)src, src_size, 1, fp_src);
    printf("fread %ld byte\n", read_size);
    fclose(fp_src);
#ifdef _FPGA
    ret = bm_memcpy_s2d_fpga(handle, dst[0], (void*)src);
#else
    ret = bm_memcpy_s2d(handle, dst[0], (void*)src);
#endif
    if(ret != BM_SUCCESS){
        printf("bm_memcpy_s2d fail %s: %s: %d\n", __FILE__, __func__, __LINE__);
    }
fail:
    free(src);
    return ret;
}

u8 is_full_image(bm_image_format_ext image_format){
	if(image_format == FORMAT_YUV420P || image_format == FORMAT_YUV422P ||
		image_format == FORMAT_NV12 || image_format == FORMAT_NV21 ||
		image_format == FORMAT_NV16 || image_format == FORMAT_NV61 ||
		image_format == FORMAT_YUV422_UYVY || image_format == FORMAT_YUV422_VYUY ||
		image_format == FORMAT_YUV422_YUYV || image_format == FORMAT_YUV422_YVYU)
		return 0;
	else
		return 1;
}

u8 is_yuv420_image(bm_image_format_ext image_format){
	if(image_format == FORMAT_YUV420P ||
		image_format == FORMAT_NV12 || image_format == FORMAT_NV21)
		return 1;
	else
		return 0;
}

static int mem_fd = -1, vpss_fd = -1, dpu_fd = -1, dwa_fd = -1, ldc_fd = -1;
static int mem_use_num = 0;
bm_status_t bm_get_mem_fd(int* fd){
    bm_status_t ret = BM_SUCCESS;
    pthread_mutex_lock(&fd_mutex);
    if(mem_fd < 0 || mem_use_num == 0)
        mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if(mem_fd < 0){
        BMCV_ERR_LOG("open mem fail\n");
        ret = BM_ERR_DEVNOTREADY;
    } else {
        *fd = mem_fd;
        mem_use_num++;
        printf("open mem fd(%d)\n", mem_fd);
    }
    pthread_mutex_unlock(&fd_mutex);
    return ret;
};

bm_status_t bm_get_vpss_fd(int* fd){
    bm_status_t ret = BM_SUCCESS;
    pthread_mutex_lock(&fd_mutex);
    if(vpss_fd < 0)
        vpss_fd = open("/dev/soph-vpss", O_RDWR /* required */  | O_NONBLOCK | O_CLOEXEC, 0);
    pthread_mutex_unlock(&fd_mutex);
    if(vpss_fd < 0){
        BMCV_ERR_LOG("open vpss fail\n");
        ret = BM_ERR_DEVNOTREADY;
    } else {
        *fd = vpss_fd;
    }
    return ret;
};

bm_status_t bm_get_dpu_fd(int* fd){
    bm_status_t ret = BM_SUCCESS;
    pthread_mutex_lock(&fd_mutex);
    if(dpu_fd < 0)
        dpu_fd = open("/dev/soph-dpu", O_RDWR /* required */  | O_NONBLOCK | O_CLOEXEC, 0);
    pthread_mutex_unlock(&fd_mutex);
    if(dpu_fd < 0){
        BMCV_ERR_LOG("open dpu fail\n");
        ret = BM_ERR_DEVNOTREADY;
    } else {
        *fd = dpu_fd;
    }

    return ret;
};

bm_status_t bm_get_dwa_fd(int* fd){
    bm_status_t ret = BM_SUCCESS;
    pthread_mutex_lock(&fd_mutex);
    if(dwa_fd < 0)
        dwa_fd = open("/dev/soph-ldc", O_RDWR /* required */  | O_NONBLOCK | O_CLOEXEC, 0);
    pthread_mutex_unlock(&fd_mutex);
    if(dwa_fd < 0){
        BMCV_ERR_LOG("open dwa fail\n");
        ret = BM_ERR_DEVNOTREADY;
    } else {
        *fd = dwa_fd;
    }
    return ret;
};

bm_status_t bm_get_ldc_fd(int* fd){
    bm_status_t ret = BM_SUCCESS;
    pthread_mutex_lock(&fd_mutex);
    if(ldc_fd < 0)
        ldc_fd = open("/dev/soph-ldc", O_RDWR /* required */  | O_NONBLOCK | O_CLOEXEC, 0);
    pthread_mutex_unlock(&fd_mutex);
    if(ldc_fd < 0){
        BMCV_ERR_LOG("open ldc fail\n");
        ret = BM_ERR_DEVNOTREADY;
    } else {
        *fd = ldc_fd;
    }
    return ret;
};

bm_status_t bm_destroy_mem_fd(void){
    pthread_mutex_lock(&fd_mutex);
    if(mem_fd > 0 && mem_use_num == 1){
        close(mem_fd);
        // printf("close mem fd(%d)\n", mem_fd);
        mem_fd = -1;
    }
    mem_use_num--;
    pthread_mutex_unlock(&fd_mutex);
    return BM_SUCCESS;
};

bm_status_t bm_destroy_vpss_fd(void){
    pthread_mutex_lock(&fd_mutex);
    if(vpss_fd > 0){
        close(vpss_fd);
        vpss_fd = -1;
    }
    pthread_mutex_unlock(&fd_mutex);
    return BM_SUCCESS;
};

bm_status_t bm_destroy_dpu_fd(void){
    pthread_mutex_lock(&fd_mutex);
    if(dpu_fd > 0){
        close(dpu_fd);
        dpu_fd = -1;
    }

    pthread_mutex_unlock(&fd_mutex);
    return BM_SUCCESS;
};

bm_status_t bm_destroy_ldc_fd(void){
    pthread_mutex_lock(&fd_mutex);
    if(ldc_fd > 0){
        close(ldc_fd);
        ldc_fd = -1;
    }
    pthread_mutex_unlock(&fd_mutex);
    return BM_SUCCESS;
};

bm_status_t bm_destroy_dwa_fd(void){
    pthread_mutex_lock(&fd_mutex);
    if(dwa_fd > 0){
        close(dwa_fd);
        dwa_fd = -1;
    }
    pthread_mutex_unlock(&fd_mutex);
    return BM_SUCCESS;
}

int open_device(const char *dev_name, int *fd)
{
  struct stat st;

  *fd = open(dev_name, O_RDWR /* required */  | O_NONBLOCK | O_CLOEXEC, 0);
  if (-1 == *fd) {
    fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno,
      strerror(errno));
    return -1;
  }

  if (-1 == fstat(*fd, &st)) {
    close(*fd);
    fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name,
      errno, strerror(errno));
    return -1;
  }

  if (!S_ISCHR(st.st_mode)) {
    close(*fd);
    fprintf(stderr, "%s is no device\n", dev_name);
    return -ENODEV;
  }
  return 0;
}

int close_device(int *fd)
{
  if (*fd == -1)
    return -1;

  if (-1 == close(*fd)) {
    fprintf(stderr, "%s: fd(%d) failure\n", __func__, *fd);
    return -1;
  }

  *fd = -1;

  return 0;
}

unsigned long long bmcv_calc_cbcr_addr(unsigned long long y_addr, unsigned int y_stride, unsigned int frame_height)
{
  u64 c_addr = 0, y_len=0;

  y_len = y_stride * ALIGN(frame_height,32);
  c_addr = y_addr + y_len;
  return c_addr;
}

bm_status_t bm_image_check(bm_image image)
{
    if (image.image_private == NULL) {
        return BM_ERR_FAILURE;
    }
    if (image.image_private->handle == NULL) {
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t bm_image_zeros(bm_image image)
{
    //tpu memset
    unsigned long long device_addr = 0;
    if(bm_image_check(image) != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
          "please check image.image_private or image.image_private->handle %s: %s: %d\n",
          filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    for (int i = 0; i < image.image_private->plane_num; i++) {
      device_addr = bm_mem_get_device_addr(image.image_private->data[i]);
      if((device_addr > 0x4ffffffff) || (device_addr < 0x100000000))
        {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "device memory should between 0x100000000 and 0x4ffffffff  %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
        }
      bm_memset_device(image.image_private->handle, 0, image.image_private->data[i]);
    }
    return BM_SUCCESS;
}
