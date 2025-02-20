#ifndef __JPEG_ENC_COMMON_H__
#define __JPEG_ENC_COMMON_H__

#include <stdio.h>
#include "bm_jpeg_interface.h"

#define ENC_ALIGN 16

#define UNUSED(x)   ((void)(x))

typedef struct {
    int width;  // actual width
    int height;  // actual height
    int y_stride;
    int c_stride;
    int aligned_height;
    int pix_fmt;
    int cbcr_interleave;
    int rotate;
    int external_memory;
    int bitstream_size;
    int bs_set_on_process;
    int timeout;
    int timeout_count;
    int bs_heap;
    int fb_heap;
    int quality_factor;
} EncParam;

typedef struct {
#ifdef BM_PCIE_MODE
    int device_index;
#endif
    char *input_filename;
    char *output_filename;
    char *refer_filename;
    int loop_num;
    int inst_idx;

    EncParam enc_params;
} EncInputParam;

void* acquire_output_buffer(void *context, size_t size, void **acquired_handle);
void finish_output_buffer(void *context, void *acquired_handle);
BmJpuEncReturnCodes start_encode(BmJpuJPEGEncoder *jpeg_encoder, EncParam *enc_params, FILE *fp_in, FILE *fp_out, int inst_idx, const uint8_t *ref_md5);

#endif /* __JPEG_ENC_COMMON_H__ */
