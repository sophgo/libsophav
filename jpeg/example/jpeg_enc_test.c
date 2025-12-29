#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include "jpeg_enc_common.h"
#include "bm_jpeg_internal.h"


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
        "\t--quality_factor, -q     encode quality factor (default 85), range is [1, 100]\n"
        "\t--cbcr_interleave        Cb/Cr component is interleaved or not (default 0) [0: planared, 1: Cb/Cr interleaved, 2: Cr/Cb interleaved]\n"
        "\t--external_memory        bitstream memory is allocated by user, set addr on enc_open\n"
        "\t--bitstream_size         bitstream buffer size set by user\n"
        "\t--bs_set_on_process      set bitstream buffer when invoking encode interface\n"
        "\t--timeout                timeout set by user\n"
        "\t--timeout_count          timeout count set by user\n"
        "\t--bs_heap                sync with BM1684(X), invalid on BM1688\n"
        "\t--fb_heap                sync with BM1684(X), invalid on BM1688\n"
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
    int option_index = 0;
    static struct option long_options[] = {
        {"help", no_argument, NULL, '0'},
        {"input", required_argument, NULL, 'i'},
        {"output", required_argument, NULL, 'o'},
        {"loop_number", required_argument, NULL, 'n'},
        {"format", required_argument, NULL, 'f'},
        {"width", required_argument, NULL, 'w'},
        {"height", required_argument, NULL, 'h'},
        {"y_stride", required_argument, NULL, 'y'},
        {"c_stride", required_argument, NULL, 'c'},
        {"v_stride", required_argument, NULL, 'v'},
        {"cbcr_interleave", required_argument, NULL, '0'},
        {"rotate", required_argument, NULL, 'g'},
        {"quality_factor", required_argument, NULL, 'q'},
        {"external_memory", no_argument, NULL, '0'},
        {"bitstream_size", required_argument, NULL, '0'},
        {"bs_set_on_process", no_argument, NULL, '0'},
        {"bs_heap", required_argument, NULL, '0'},
        {"fb_heap", required_argument, NULL, '0'},
        {"timeout", required_argument, NULL, '0'},
        {"timeout_count", required_argument, NULL, '0'},
        {NULL, 0, NULL, 0}
    };

    memset(input_params, 0, sizeof(EncInputParam));

    while ((opt = getopt_long(argc, argv, "i:o:n:d:w:h:v:f:y:c:g:q:",long_options, &option_index)) != -1)
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
            case 'q':
                input_params->enc_params.quality_factor = atoi(optarg);
                break;
            case '0':
                if (!strcmp(long_options[option_index].name, "cbcr_interleave")) {
                    input_params->enc_params.cbcr_interleave = atoi(optarg);
                } else if (!strcmp(long_options[option_index].name, "external_memory")) {
                    input_params->enc_params.external_memory = 1;
                } else if (!strcmp(long_options[option_index].name, "bitstream_size")) {
                    input_params->enc_params.bitstream_size = atoi(optarg);
                } else if (!strcmp(long_options[option_index].name, "bs_set_on_process")) {
                    input_params->enc_params.bs_set_on_process = 1;
                } else if (!strcmp(long_options[option_index].name, "quality_factor")) {
                    input_params->enc_params.quality_factor = atoi(optarg);
                } else if (!strcmp(long_options[option_index].name, "bs_heap")) {
                    input_params->enc_params.bs_heap = atoi(optarg);
                } else if (!strcmp(long_options[option_index].name, "fb_heap")) {
                    input_params->enc_params.fb_heap = atoi(optarg);
                } else if (!strcmp(long_options[option_index].name, "timeout")) {
                    input_params->enc_params.timeout = atoi(optarg);
                } else if (!strcmp(long_options[option_index].name, "timeout_count")) {
                    input_params->enc_params.timeout_count = atoi(optarg);
                } else {
                    usage(argv[0]);
                    return -1;
                }
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
        if ((input_params->enc_params.pix_fmt == 0 || input_params->enc_params.pix_fmt == 1) && (input_params->enc_params.cbcr_interleave == 0)) {
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
    } else if (input_params->enc_params.pix_fmt == 1) {
        input_params->enc_params.pix_fmt = BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL;
    } else if (input_params->enc_params.pix_fmt == 2) {
        input_params->enc_params.pix_fmt = BM_JPU_COLOR_FORMAT_YUV444;
    } else {
        input_params->enc_params.pix_fmt = BM_JPU_COLOR_FORMAT_YUV400;
    }

    if (input_params->enc_params.quality_factor == 0) {
        input_params->enc_params.quality_factor = 85;
    }

    if (input_params->enc_params.quality_factor < 1 || input_params->enc_params.quality_factor > 100) {
        fprintf(stderr, "Invalid params: quality should be in [1, 100]\n\n");
        usage(argv[0]);
        return -1;
    }

    if (input_params->enc_params.bitstream_size < 0) {
        input_params->enc_params.bitstream_size = 0;
    }

    if ((input_params->enc_params.external_memory || input_params->enc_params.bs_set_on_process) && input_params->enc_params.bitstream_size == 0) {
        fprintf(stderr, "Invalid params: bitstream buffer size should be larger than 0\n\n");
        usage(argv[0]);
        return -1;
    }
#define BS_SIZE_MASK (16 * 1024)
    input_params->enc_params.bitstream_size = (input_params->enc_params.bitstream_size + BS_SIZE_MASK - 1) & ~(BS_SIZE_MASK - 1);
    return 0;
}

int main(int argc, char *argv[])
{
    BmJpuEncReturnCodes ret = BM_JPU_ENC_RETURN_CODE_OK;
    EncInputParam input_params;
    BmJpuJPEGEncoder *jpeg_encoder = NULL;
    bm_handle_t bm_handle;
    bm_device_mem_t* bs_buffer = NULL;
    bm_jpu_phys_addr_t bs_addr = 0;
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

    if (input_params.enc_params.external_memory) {
        ret = bm_dev_request(&bm_handle, 0);
        if (ret != (BmJpuEncReturnCodes)BM_SUCCESS) {
            fprintf(stderr, "failed to request device 0\n");
            ret = -1;
            goto cleanup;
        }

        bs_buffer = (bm_device_mem_t*)malloc(input_params.enc_params.bitstream_size);
        ret = bm_malloc_device_byte_heap(bm_handle, bs_buffer, 1, input_params.enc_params.bitstream_size);
        if (ret != (BmJpuEncReturnCodes)BM_SUCCESS) {
            fprintf(stderr, "failed to malloc bitstream buffer, size=%d\n", input_params.enc_params.bitstream_size);
            ret = -1;
            goto cleanup;
        }
        bs_addr = bs_buffer->u.device.device_addr;
    }

    for (i = 0; i < input_params.loop_num; i++) {
        printf("loop %d begin\n", i);
        /* open jpeg encoder */
        ret = bm_jpu_jpeg_enc_open(&jpeg_encoder, bs_addr, input_params.enc_params.bitstream_size, 0);
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

    if(bs_buffer) {
        bm_free_device(bm_handle, *bs_buffer);
        bs_buffer = NULL;
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
