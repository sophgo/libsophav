
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "bmcv_internal.h"
#include "bmcv_a2_base64.h"

#ifdef __linux__
  #include <sys/ioctl.h>
#else
  #include <windows.h>
#endif

#define MAX_LEN 178956972  //128M/3*4
#define MAX_LOOP_LEN 3145728 //3M

static unsigned long int base64_compute_dstlen(unsigned long int len, bool enc) {
    unsigned long roundup;

    if (len)
    // encode
    if (enc)
        return (len + 2) /
                3 * 4;
    // decode
    if (len == 0)
        return 0;
    roundup = len / 4 * 3;
    return roundup;
}

bm_status_t base64_operation_ioctl(int fd, unsigned long long src_addr, unsigned long long dst_addr, unsigned long loop_len, bool direction)
{
    struct cvi_spacc_base64_inner base;
    base.src = src_addr;
    base.dst = dst_addr;
    base.len = loop_len;
    base.customer_code = 0;
    base.action = direction;
    int ret = ioctl(fd, IOCTL_SPACC_BASE64_INNER, &base);
    if (ret < 0) {
        bmlib_log("BASE64", BMLIB_LOG_ERROR, "ioctl failed!\n");
        return BM_ERR_DEVNOTREADY;
    }
    return BM_SUCCESS;
}

bm_status_t base64_sys_sys(bm_handle_t handle, int fd, bm_device_mem_t src_buf_device,
                            bm_device_mem_t dst_buf_device, unsigned long loop_len,
                            unsigned long long src_addr, unsigned long long dst_addr,
                            bool direction)
{
    bm_status_t ret = BM_SUCCESS;
    ret = bm_malloc_device_byte(handle, &src_buf_device, loop_len);
    if (ret != BM_SUCCESS){
        bmlib_log("BASE64", BMLIB_LOG_ERROR, "bm_malloc_device_byte error\r\n");
        goto cleanup;
    }
    ret = bm_memcpy_s2d(handle, src_buf_device, (void *)src_addr);
    if(ret != BM_SUCCESS){
        bmlib_log("BASE64", BMLIB_LOG_ERROR, "bm_memcpy_s2d error\r\n");
        goto cleanup;
    }
    ret = bm_malloc_device_byte(handle, &dst_buf_device, base64_compute_dstlen(loop_len, direction));
    if(ret != BM_SUCCESS){
        bmlib_log("BASE64", BMLIB_LOG_ERROR, "bm_malloc_device_byte error\r\n");
        goto cleanup;
    }
    // system addr
    ret = base64_operation_ioctl(fd, bm_mem_get_device_addr(src_buf_device),
                                    bm_mem_get_device_addr(dst_buf_device),
                                    loop_len, direction);
    if(ret != BM_SUCCESS) {
        goto cleanup;
    }
    ret = bm_memcpy_d2s(handle, (void *)dst_addr, dst_buf_device);
    if(ret != BM_SUCCESS) {
        bmlib_log("BASE64", BMLIB_LOG_ERROR, "bm_memcpy_d2s error\r\n");
        goto cleanup;
    }

cleanup:
    if(src_buf_device.size != 0) bm_free_device(handle, src_buf_device);
    if(dst_buf_device.size != 0) bm_free_device(handle, dst_buf_device);
    return ret;
}

bm_status_t base64_sys_dev(bm_handle_t handle, int fd, bm_device_mem_t src_buf_device,
                            unsigned long loop_len, unsigned long long src_addr,
                            unsigned long long dst_addr, bool direction)
{
    bm_status_t ret = BM_SUCCESS;
    ret = bm_malloc_device_byte(handle, &src_buf_device, loop_len);
    if(ret != BM_SUCCESS){
        bmlib_log("BASE64", BMLIB_LOG_ERROR, "bm_malloc_device_byte error\r\n");
        goto cleanup;
    }
    ret = bm_memcpy_s2d(handle, src_buf_device, (void *)src_addr);
    if(ret != BM_SUCCESS) {
        bmlib_log("BASE64", BMLIB_LOG_ERROR, "bm_memcpy_s2d error\r\n");
        goto cleanup;
    }
    ret = base64_operation_ioctl(fd, bm_mem_get_device_addr(src_buf_device),
                                    dst_addr,
                                    loop_len, direction);
    if(ret != BM_SUCCESS) {
        goto cleanup;
    }

cleanup:
    if(src_buf_device.size != 0) bm_free_device(handle, src_buf_device);
    return ret;
}

bm_status_t base64_dev_sys(bm_handle_t handle, int fd, bm_device_mem_t dst_buf_device,
                            unsigned long loop_len, unsigned long long src_addr,
                            unsigned long long dst_addr, bool direction)
{
    bm_status_t ret = BM_SUCCESS;
    ret = bm_malloc_device_byte(handle, &dst_buf_device, base64_compute_dstlen(loop_len, direction));
    if(ret != BM_SUCCESS) {
        bmlib_log("BASE64", BMLIB_LOG_ERROR, "bm_malloc_device_byte error\r\n");
        goto cleanup;
    }
    ret = base64_operation_ioctl(fd, src_addr,
                                    bm_mem_get_device_addr(dst_buf_device),
                                    loop_len, direction);
    if(ret != BM_SUCCESS) {
        goto cleanup;
    }
    ret = bm_memcpy_d2s(handle, (void *)dst_addr, dst_buf_device);
    if(ret != BM_SUCCESS) {
        bmlib_log("BASE64", BMLIB_LOG_ERROR, "bm_memcpy_d2s error\r\n");
        goto cleanup;
    }

cleanup:
    if(dst_buf_device.size != 0) bm_free_device(handle, dst_buf_device);
    return ret;
}

bm_status_t base64_dev_dev(bm_handle_t handle, int fd, unsigned long loop_len,
                            unsigned long long src_addr, unsigned long long dst_addr, bool direction)
{
    return base64_operation_ioctl(fd, src_addr, dst_addr, loop_len, direction);
}

int bm_get_base64_optation_status(bm_device_mem_t src, bm_device_mem_t dst, unsigned long long* src_addr, unsigned long long* dst_addr)
{
    int status = 0;
    if ((bm_mem_get_type(src) == BM_MEM_TYPE_SYSTEM) && (bm_mem_get_type(dst) == BM_MEM_TYPE_SYSTEM))
    {
        *src_addr = (unsigned long long)bm_mem_get_system_addr(src);
        *dst_addr = (unsigned long long)bm_mem_get_system_addr(dst);
        status = 1;
    } else if ((bm_mem_get_type(src) == BM_MEM_TYPE_SYSTEM) && (bm_mem_get_type(dst) == BM_MEM_TYPE_DEVICE))
    {
        *src_addr = (unsigned long long)bm_mem_get_system_addr(src);
        *dst_addr = bm_mem_get_device_addr(dst);
        status = 2;
    } else if ((bm_mem_get_type(src) == BM_MEM_TYPE_DEVICE) && (bm_mem_get_type(dst) == BM_MEM_TYPE_SYSTEM))
    {
        *src_addr = bm_mem_get_device_addr(src);
        *dst_addr = (unsigned long long)bm_mem_get_system_addr(dst);
        status = 3;
    } else if ((bm_mem_get_type(src) == BM_MEM_TYPE_DEVICE) && (bm_mem_get_type(dst) == BM_MEM_TYPE_DEVICE))
    {
        *src_addr = bm_mem_get_device_addr(src);
        *dst_addr = bm_mem_get_device_addr(dst);
        status = 0;
    }
    return status;
}

bm_status_t bmcv_base64_codec(bm_handle_t handle, bm_device_mem_t src,
    bm_device_mem_t dst, unsigned long len, bool direction) {

    bm_status_t ret = BM_SUCCESS;
    unsigned long loop_len;
    unsigned long long src_addr = 0; //naive addr of src as src is a struct
    unsigned long long dst_addr = 0;
    bm_device_mem_t src_buf_device;
    bm_device_mem_t dst_buf_device;
    memset(&src_buf_device, 0, sizeof(bm_device_mem_t));
    memset(&dst_buf_device, 0, sizeof(bm_device_mem_t));

    if (handle == NULL) {
        bmlib_log("BASE64", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_DEVNOTREADY;
    }
    if (len > MAX_LEN) {
        bmlib_log("BASE64", BMLIB_LOG_ERROR, "base64 lenth should be less than 128M!\n");
        return BM_ERR_PARAM;
    }
    int fd = open("/dev/spacc", O_RDWR);
    if (fd < 0) {
        bmlib_log("BASE64", BMLIB_LOG_ERROR, "open /dev/spacc failed\n");
        return BM_ERR_DEVNOTREADY;
    }

    int sys_addr = bm_get_base64_optation_status(src, dst, &src_addr, &dst_addr);

    // MAX_LOOP_LEN M data to enc or dec
    while (len > 0) {
        if (len > MAX_LOOP_LEN) {
            loop_len = MAX_LOOP_LEN;
            len = len - MAX_LOOP_LEN;
        } else {
            loop_len = len;
            len = 0;
        }
        switch(sys_addr) {
            case 1://all sys addr
                ret = base64_sys_sys(handle, fd, src_buf_device, dst_buf_device, loop_len, src_addr, dst_addr, direction);
                if(ret != BM_SUCCESS) goto cleanup;
                break;
            case 2://src sys addr,dst device addr
                ret = base64_sys_dev(handle, fd, src_buf_device, loop_len, src_addr, dst_addr, direction);
                if(ret != BM_SUCCESS) goto cleanup;
                break;
            case 3://dst sys addr,src device addr
                ret = base64_dev_sys(handle, fd, dst_buf_device, loop_len, src_addr, dst_addr, direction);
                if(ret != BM_SUCCESS) goto cleanup;
                break;
            case 0://all dst addr
                ret = base64_dev_dev(handle, fd, loop_len, src_addr, dst_addr, direction);
                if(ret != BM_SUCCESS) goto cleanup;
                break;
            default:
                bmlib_log("BASE64", BMLIB_LOG_ERROR, "bm_get_base64_optation_status return value is error\n");
                ret = BM_ERR_NOMEM; goto cleanup;
        }
        // addr offset
        src_addr = src_addr + (unsigned long long)loop_len;
        dst_addr = dst_addr + (unsigned long long)base64_compute_dstlen
                   ((unsigned long)loop_len, direction);
    }

cleanup:
    close(fd);
    return ret;
}

bm_status_t bmcv_base64_enc(bm_handle_t handle, bm_device_mem_t src,
    bm_device_mem_t dst, unsigned long len[2])
{
    // BASE64_ENC
    return bmcv_base64_codec(handle, src, dst, len[0], 1);
}
bm_status_t bmcv_base64_dec(bm_handle_t handle, bm_device_mem_t src,
    bm_device_mem_t dst, unsigned long len[2])
{
    // BASE64_DEC
    return bmcv_base64_codec(handle, src, dst, len[0], 0);
}
