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
} DecInputParam;

int save_yuv(FILE *fp, BmJpuJPEGDecInfo *dec_info, uint8_t *virt_addr);
BmJpuDecReturnCodes start_decode(BmJpuJPEGDecoder *jpeg_decoder, uint8_t *bs_buffer, size_t bs_size, FILE *fp_out, int inst_idx, const uint8_t *ref_md5, int dump_crop);

#endif /* __JPEG_DEC_COMMON_H__ */