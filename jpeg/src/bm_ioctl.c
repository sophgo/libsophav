#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <linux/types.h>
#include "bm_ioctl.h"

#define PAGE_SIZE ((unsigned long)getpagesize())

static int g_jpu_mem_fd = -1;
static int g_jpu_mem_open_count = 0;

void bmjpeg_devm_open(void)
{
    int fd;

    if (g_jpu_mem_fd == -1) {
        fd = open("/dev/mem", O_RDWR | O_SYNC);
        if (fd == -1) {
            printf("cannot open '/dev/mem' \n");
            return;
        }
        g_jpu_mem_fd = fd;
        g_jpu_mem_open_count = 1;
    } else {
        g_jpu_mem_open_count++;
    }

    return;
}

void bmjpeg_devm_close(void)
{
    g_jpu_mem_open_count--;
    if (g_jpu_mem_open_count <= 0) {
        close(g_jpu_mem_fd);
        g_jpu_mem_fd = -1;
        g_jpu_mem_open_count = 0;
    }
}

void* bmjpeg_devm_map(uint64_t phys_addr, size_t len)
{
    off_t offset;
    void *map_base;
    uint32_t padding;

    if (g_jpu_mem_fd == -1) {
        return NULL;
    }

    offset = phys_addr & ~(sysconf(_SC_PAGE_SIZE) - 1);
    padding = phys_addr - offset;
    
    map_base = mmap(NULL, len + padding, PROT_READ | PROT_WRITE, MAP_SHARED, g_jpu_mem_fd, offset);
    if (map_base == MAP_FAILED) {
        perror("mmap failed\n");
        return NULL;
    }

    printf("[%s:%d] map_base = %#lx, padding = %d\n", __FILE__, __LINE__, (uint64_t)map_base, padding);

    return map_base + padding;
}

void bmjpeg_devm_unmap(void *virt_addr, size_t len)
{
    uint64_t addr;

    /* page align */
    addr = ((uint64_t)(uintptr_t)virt_addr) & ~(sysconf(_SC_PAGE_SIZE) - 1);
    munmap((void *)(uintptr_t)addr, len + (unsigned long)virt_addr - addr);

    return;
}

void* bmjpeg_ioctl_mmap(int chn_fd, unsigned long long phys_addr, size_t len)
{
    void *ret = 0;
    unsigned int aligned_size = (len + PAGE_SIZE - 1) & (~(PAGE_SIZE - 1));

    ret = mmap(NULL, aligned_size, PROT_READ | PROT_WRITE, MAP_SHARED, chn_fd, phys_addr | 0x1000000000);
    if (ret == MAP_FAILED) {
        perror("mmap failed\n");
        return NULL;
    }

    printf("[%s:%d] map_base = %#lx, size = %d\n", __FILE__, __LINE__, (uint64_t)ret, aligned_size);
    return ret;
}

void bmjpeg_ioctl_munmap(uint8_t *virt_addr, size_t len)
{
    unsigned int aligned_size = (len + PAGE_SIZE - 1) & (~(PAGE_SIZE - 1));

    munmap((void *)(uintptr_t)virt_addr, (unsigned long)aligned_size);

    return;
}

int bmjpeg_ioctl_get_chn(int chn_fd, int *chn_id)
{
    int ret = 0;
    int is_jpeg = 1;
    *chn_id = ioctl(chn_fd, DRV_VC_VCODEC_GET_CHN, &is_jpeg);
    return ret;
}

int bmjpeg_ioctl_set_chn(int chn_fd, int *chn_id)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VCODEC_SET_CHN, chn_id);
    return ret;
}

/* jpeg dec ioctl */
int bmjpeg_dec_ioctl_create_chn(int chn_fd, vdec_chn_attr_s *pstAttr)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_CREATE_CHN, pstAttr);
    return ret;
}

int bmjpeg_dec_ioctl_destroy_chn(int chn_fd)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_DESTROY_CHN, NULL);
    return ret;
}

int bmjpeg_dec_ioctl_get_chn_attr(int chn_fd, vdec_chn_attr_s *pstAttr)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_GET_CHN_ATTR, pstAttr);
    return ret;
}

int bmjpeg_dec_ioctl_set_chn_attr(int chn_fd, vdec_chn_attr_s *pstAttr)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_SET_CHN_ATTR, pstAttr);
    return ret;
}

int bmjpeg_dec_ioctl_get_chn_param(int chn_fd, vdec_chn_param_s *pstParam)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_GET_CHN_PARAM, pstParam);
    return ret;
}

int bmjpeg_dec_ioctl_set_chn_param(int chn_fd, vdec_chn_param_s *pstParam)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_SET_CHN_PARAM, pstParam);
    return ret;
}

int bmjpeg_dec_ioctl_start_recv_stream(int chn_fd)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_START_RECV_STREAM, NULL);
    return ret;
}

int bmjpeg_dec_ioctl_stop_recv_stream(int chn_fd)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_STOP_RECV_STREAM, NULL);
    return ret;
}

int bmjpeg_dec_ioctl_send_stream(int chn_fd, vdec_stream_ex_s *pstStreamEx)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_SEND_STREAM, pstStreamEx);
    return ret;
}

int bmjpeg_dec_ioctl_get_frame(int chn_fd, video_frame_info_ex_s *pstFrameInfoEx)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_GET_FRAME, pstFrameInfoEx);
    return ret;
}

int bmjpeg_dec_ioctl_release_frame(int chn_fd, const video_frame_info_s *pstFrameInfo)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_RELEASE_FRAME, pstFrameInfo);
    return ret;
}


/* jpeg enc ioctl */
int bmjpeg_enc_ioctl_create_chn(int chn_fd, venc_chn_attr_s *pstAttr)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VENC_CREATE_CHN, pstAttr);
    return ret;
}

int bmjpeg_enc_ioctl_destroy_chn(int chn_fd)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VENC_DESTROY_CHN, NULL);
    return ret;
}

int bmjpeg_enc_ioctl_get_chn_attr(int chn_fd, venc_chn_attr_s *pstAttr)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VENC_GET_CHN_ATTR, pstAttr);
    return ret;
}

int bmjpeg_enc_ioctl_start_recv_frame(int chn_fd, venc_recv_pic_param_s *pstRecvParam)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VENC_START_RECV_FRAME, pstRecvParam);
    return ret;
}

int bmjpeg_enc_ioctl_stop_recv_frame(int chn_fd)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VENC_STOP_RECV_FRAME, NULL);
    return ret;
}

int bmjpeg_enc_ioctl_send_frame(int chn_fd, video_frame_info_ex_s *pstFrameEx)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VENC_SEND_FRAME, pstFrameEx);
    return ret;
}

int bmjpeg_enc_ioctl_get_stream(int chn_fd, venc_stream_ex_s *pstStreamEx)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VENC_GET_STREAM, pstStreamEx);
    return ret;
}

int bmjpeg_enc_ioctl_query_status(int chn_fd, venc_chn_status_s *pstStatus)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VENC_QUERY_STATUS, pstStatus);
    return ret;
}

int bmjpeg_enc_ioctl_release_stream(int chn_fd, venc_stream_s *pstStream)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VENC_RELEASE_STREAM, pstStream);
    return ret;
}

int bmjpeg_enc_ioctl_reset_chn(int chn_fd)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VENC_RESET_CHN, NULL);
    return ret;
}