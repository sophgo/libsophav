#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "jpeg_enc_common.h"
#include "bm_jpeg_logging.h"


#define MIN_MJPG_PIC_WIDTH  16
#define MIN_MJPG_PIC_HEIGHT 16
#define MAX_MJPG_PIC_WIDTH  32768 /* please refer to jpuconfig.h */
#define MAX_MJPG_PIC_HEIGHT 32768 /* please refer to jpuconfig.h */

static void usage(char *program)
{
    static char options[] =
#ifdef BM_PCIE_MODE
        "\t-d device index\n"
#endif
        "\t-f pixel format: 0.YUV420(default); 1.YUV422; 2.YUV444; 3.YUV400. (optional)\n"
        "\t-w actual width\n"
        "\t-h actual height\n"
        "\t-y luma stride (optional)\n"
        "\t-c chroma stride (optional)\n"
        "\t-v aligned height (optional)\n"
        "\t-i input file\n"
        "\t-o output file\n"
        "\t-n loop num\n"
        "\t-g rotate (default 0) [rotate mode[1:0]  0:No rotate  1:90  2:180  3:270] [rotator mode[2]:vertical flip] [rotator mode[3]:horizontal flip]\n"
        "For example,\n"
        "\tbmjpegenc -f 0 -w 100 -h 100 -y 128 -v 112 -i 100x100_yuv420.yuv -o 100x100_yuv420.jpg\n"
        "\tbmjpegenc -f 1 -w 100 -h 100 -y 128 -v 112 -i 100x100_yuv422.yuv -o 100x100_yuv422.jpg\n"
        "\tbmjpegenc -f 1 -w 100 -h 100 -y 128 -c 64 -v 112 -i 100x100_yuv422.yuv -o 100x100.jpg\n"
        ;

    fprintf(stderr, "usage:\t%s [option]\n\noption:\n%s\n", program, options);
}

extern char *optarg;
static int parse_args(int argc, char *argv[], EncInputParam *input_params)
{
    int opt;

    memset(input_params, 0, sizeof(EncInputParam));

    while ((opt = getopt(argc, argv, "i:o:n:d:w:h:v:f:y:c:g:")) != -1)
    {
        switch (opt) {
            case 'i':
                input_params->input_filename = optarg;
                break;
            case 'o':
                input_params->output_filename = optarg;
                break;
            case 'n':
                input_params->loop_num = atoi(optarg);
                break;
            case 'd':
            #ifdef BM_PCIE_MODE
                input_params->device_index = atoi(optarg);
            #endif
                break;
            case 'w':
                input_params->enc_params.width = atoi(optarg);
                break;
            case 'h':
                input_params->enc_params.height = atoi(optarg);
                break;
            case 'v':
                input_params->enc_params.aligned_height = atoi(optarg);
                break;
            case 'f':
                input_params->enc_params.pix_fmt = atoi(optarg);
                break;
            case 'y':
                input_params->enc_params.y_stride = atoi(optarg);
                break;
            case 'c':
                input_params->enc_params.c_stride = atoi(optarg);
                break;
            case 'g':
                input_params->enc_params.rotate = atoi(optarg);
                break;
            default:
                usage(argv[0]);
                return -1;
        }
    }

    if (input_params->input_filename == NULL) {
        fprintf(stderr, "Missing input filename\n\n");
        usage(argv[0]);
        return -1;
    }

    if (input_params->output_filename == NULL) {
        fprintf(stderr, "Missing output filename\n\n");
        usage(argv[0]);
        return -1;
    }

    if (input_params->loop_num <= 0) {
        input_params->loop_num = 1;
    }

    if (input_params->enc_params.width < MIN_MJPG_PIC_WIDTH || input_params->enc_params.width > MAX_MJPG_PIC_WIDTH) {
        fprintf(stderr, "width must be %d ~ %d\n\n", MIN_MJPG_PIC_WIDTH, MAX_MJPG_PIC_WIDTH);
        usage(argv[0]);
        return -1;
    }

    if (input_params->enc_params.height < MIN_MJPG_PIC_HEIGHT || input_params->enc_params.height > MAX_MJPG_PIC_HEIGHT) {
        fprintf(stderr, "height must be %d ~ %d\n\n", MIN_MJPG_PIC_HEIGHT, MAX_MJPG_PIC_HEIGHT);
        usage(argv[0]);
        return -1;
    }

    if (input_params->enc_params.y_stride != 0 && input_params->enc_params.y_stride < input_params->enc_params.width) {
        fprintf(stderr, "luma stride is less than width\n\n");
        usage(argv[0]);
        return -1;
    }

    if (input_params->enc_params.aligned_height != 0 && input_params->enc_params.aligned_height < input_params->enc_params.height) {
        fprintf(stderr, "aligned height is less than actual height\n\n");
        usage(argv[0]);
        return -1;
    }

    if (input_params->enc_params.pix_fmt < 0 || input_params->enc_params.pix_fmt > 3) {
        fprintf(stderr, "invalid pixel format: %d\n\n", input_params->enc_params.pix_fmt);
        usage(argv[0]);
        return -1;
    }


    if (input_params->enc_params.y_stride == 0) {
        input_params->enc_params.y_stride = input_params->enc_params.width;
    }

    if (input_params->enc_params.c_stride == 0) {
        if (input_params->enc_params.pix_fmt == 0 || input_params->enc_params.pix_fmt == 1) {
            input_params->enc_params.c_stride = input_params->enc_params.y_stride / 2;
        } else {
            input_params->enc_params.c_stride = input_params->enc_params.y_stride;
        }
    }

    if (input_params->enc_params.aligned_height == 0) {
        input_params->enc_params.aligned_height = input_params->enc_params.height;
    }

    if (input_params->enc_params.pix_fmt == 0) {
        input_params->enc_params.pix_fmt = BM_JPU_COLOR_FORMAT_YUV420;
        input_params->enc_params.cbcr_interleave = 0;
    } else if (input_params->enc_params.pix_fmt == 1) {
        input_params->enc_params.pix_fmt = BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL;
        input_params->enc_params.cbcr_interleave = 0;
    } else if (input_params->enc_params.pix_fmt == 2) {
        input_params->enc_params.pix_fmt = BM_JPU_COLOR_FORMAT_YUV444;
        input_params->enc_params.cbcr_interleave = 0;
    } else {
        input_params->enc_params.pix_fmt = BM_JPU_COLOR_FORMAT_YUV400;
        input_params->enc_params.cbcr_interleave = 0;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    BmJpuEncReturnCodes ret = BM_JPU_ENC_RETURN_CODE_OK;
    EncInputParam input_params;
    BmJpuJPEGEncoder *jpeg_encoder = NULL;
    FILE *fp_in = NULL;
    FILE *fp_out = NULL;
    int i;

    ret = parse_args(argc, argv, &input_params);
    if (ret != BM_JPU_ENC_RETURN_CODE_OK) {
        return ret;
    }

    // bm_jpu_set_logging_threshold(BM_JPU_LOG_LEVEL_INFO);

    bm_jpu_enc_load(0);

    fp_in = fopen(input_params.input_filename, "rb");
    if (fp_in == NULL) {
        fprintf(stderr, "Opening %s for reading failed: %s\n", input_params.input_filename, strerror(errno));
        ret = BM_JPU_ENC_RETURN_CODE_ERROR;
        goto cleanup;
    }

    fp_out = fopen(input_params.output_filename, "wb");
    if (fp_out == NULL) {
        fprintf(stderr, "Opening %s for writing failed: %s\n", input_params.output_filename, strerror(errno));
        ret = BM_JPU_ENC_RETURN_CODE_ERROR;
        goto cleanup;
    }

    for (i = 0; i < input_params.loop_num; i++) {
        printf("loop %d begin\n", i);
        /* open jpeg encoder */
        ret = bm_jpu_jpeg_enc_open(&jpeg_encoder, 0, 0);
        if (ret != BM_JPU_ENC_RETURN_CODE_OK) {
            fprintf(stderr, "open jpeg encoder failed!\n");
            goto cleanup;
        }

        fseek(fp_in, 0, SEEK_SET);
        fseek(fp_out, 0, SEEK_SET);
        ret = start_encode(jpeg_encoder, &input_params.enc_params, fp_in, fp_out, 0, NULL);
        if (ret != BM_JPU_ENC_RETURN_CODE_OK) {
            goto cleanup;
        }

        /* close jpeg encoder */
        ret = bm_jpu_jpeg_enc_close(jpeg_encoder);
        if (ret != BM_JPU_ENC_RETURN_CODE_OK) {
            fprintf(stderr, "close jpeg encoder failed!\n");
            goto cleanup;
        }
        jpeg_encoder = NULL;
        printf("loop %d finished\n", i);
    }

cleanup:
    if (jpeg_encoder != NULL) {
        bm_jpu_jpeg_enc_close(jpeg_encoder);
        jpeg_encoder = NULL;
    }

    if (fp_out != NULL) {
        fclose(fp_out);
    }

    if (fp_in != NULL) {
        fclose(fp_in);
    }

    bm_jpu_enc_unload(0);

    return ret;
}