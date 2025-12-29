#ifndef __JPEG_DEC_COMMON_H__
#define __JPEG_DEC_COMMON_H__

#include <stdio.h>
#include "bm_jpeg_interface.h"

typedef struct {
    int x;
    int y;
    int w;
    int h;
} jpu_rect;

typedef struct {
    char *input_filename;
    char *output_filename;
    char *dataset_path;
    int loop_num;
#ifdef BM_PCIE_MODE
    int device_index;
#endif
    int dump_crop;
    int rotate;
    int scale;
    jpu_rect roi;

    char *refer_filename;
    int inst_idx;
    int cbcr_interleave;
    int external_mem;
    int bs_size;
    int fb_num;
    int fb_size;
    int bs_heap;
    int fb_heap;
    int timeout_on_open;
    int timeout_count_on_open;
    int timeout_on_process;
    int timeout_count_on_process;
    int min_width;
    int min_height;
    int max_width;
    int max_height;
} DecInputParam;

int save_yuv(FILE *fp, BmJpuJPEGDecInfo *dec_info, uint8_t *virt_addr);
BmJpuDecReturnCodes start_decode(BmJpuJPEGDecoder *jpeg_decoder, uint8_t *bs_buffer, size_t bs_size, FILE *fp_out, const uint8_t *ref_md5, int dump_crop, int timeout, int timeout_count);

#endif /* __JPEG_DEC_COMMON_H__ */
