#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "jpeg_dec_common.h"
#include "bm_jpeg_logging.h"


static void usage(char *program)
{
    static char options[] =
        "\t-i input_file\n"
        "\t-o output_file\n"
        "\t-n loop_num\n"
    #ifdef BM_PCIE_MODE
        "\t-d device index\n"
    #endif
        "\t-c crop function(0:disable  1:enable crop)\n"
        "\t-g rotate (default 0) [rotate mode[1:0]  0:No rotate  1:90  2:180  3:270] [rotator mode[2]:vertical flip] [rotator mode[3]:horizontal flip]\n"
        "\t-s scale (default 0) -> 0 to 3\n"
        "\t-r roi_x,roi_y,roi_w,roi_h\n"
        ;
    fprintf(stderr, "usage:\t%s [option]\n\noption:\n%s\n", program, options);
}

extern char *optarg;
static int parse_args(int argc, char *argv[], DecInputParam *input_params)
{
    int opt;

    memset(input_params, 0, sizeof(DecInputParam));

    while ((opt = getopt(argc, argv, "i:o:n:d:c:g:s:r:")) != -1) {
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
            case 'c':
                input_params->dump_crop = atoi(optarg);
                break;
            case 'g':
                input_params->rotate = atoi(optarg);
                break;
            case 's':
                input_params->scale = atoi(optarg);
                break;
            case 'r':
                sscanf(optarg, "%d,%d,%d,%d", &input_params->roi.x, &input_params->roi.y, &input_params->roi.w, &input_params->roi.h);
                break;
            default:
                usage(argv[0]);
                return -1;
        }
    }

    if (input_params->input_filename == NULL) {
        fprintf(stderr, "Missing input filename\n");
        usage(argv[0]);
        return -1;
    }

    if (input_params->output_filename == NULL) {
        fprintf(stderr, "Missing output filename\n");
        usage(argv[0]);
        return -1;
    }

    if (input_params->loop_num <= 0) {
        input_params->loop_num = 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    BmJpuDecReturnCodes ret = BM_JPU_DEC_RETURN_CODE_OK;
    DecInputParam input_params;
    BmJpuDecOpenParams dec_open_params;
    BmJpuJPEGDecoder *jpeg_decoder = NULL;
    FILE *fp_input = NULL;
    FILE *fp_output = NULL;
    size_t bs_size = 0;
    uint8_t *bs_buffer = NULL;
    int rotation_info = 0;
    int mirror_info = 0;
    int i;

    ret = parse_args(argc, argv, &input_params);
    if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
        return BM_JPU_DEC_RETURN_CODE_ERROR;
    }

    // bm_jpu_set_logging_threshold(BM_JPU_LOG_LEVEL_INFO);

    bm_jpu_dec_load(0);

    fp_input = fopen(input_params.input_filename, "rb");
    if (fp_input == NULL) {
        fprintf(stderr, "Opening %s for reading failed: %s\n", input_params.input_filename, strerror(errno));
        ret = BM_JPU_DEC_RETURN_CODE_ERROR;
        goto cleanup;
    }

    fp_output = fopen(input_params.output_filename, "wb");
    if (fp_output == NULL) {
        fprintf(stderr, "Opening %s for writing failed: %s\n", input_params.output_filename, strerror(errno));
        ret = BM_JPU_DEC_RETURN_CODE_ERROR;
        goto cleanup;
    }

    fseek(fp_input, 0, SEEK_END);
    bs_size = ftell(fp_input);
    fseek(fp_input, 0, SEEK_SET);

    printf("encoded input frame size: %lu bytes\n", bs_size);

    bs_buffer = (uint8_t *)malloc(bs_size);
    if (bs_buffer == NULL) {
        fprintf(stderr, "malloc bitstream buffer failed!\n");
        ret = BM_JPU_DEC_RETURN_CODE_ERROR;
        goto cleanup;
    }

    fread(bs_buffer, sizeof(uint8_t), bs_size, fp_input);

    for (i = 0; i < input_params.loop_num; i++) {
        printf("loop %d begin\n", i);
        /* open the jpeg decoder */
        memset(&dec_open_params, 0, sizeof(BmJpuDecOpenParams));

        dec_open_params.frame_width = 0;
        dec_open_params.frame_height = 0;
        dec_open_params.bs_buffer_size = bs_size;
        dec_open_params.buffer = bs_buffer;
        dec_open_params.color_format = BM_JPU_COLOR_FORMAT_BUTT;
        dec_open_params.scale_ratio = input_params.scale;

        rotation_info = input_params.rotate & 0x3;
        if (rotation_info != 0) {
            dec_open_params.rotationEnable = 1;
            if (rotation_info == 1) {
                dec_open_params.rotationAngle = 90;
            } else if (rotation_info == 2) {
                dec_open_params.rotationAngle = 180;
            } else if (rotation_info == 3) {
                dec_open_params.rotationAngle = 270;
            }
        }

        mirror_info = input_params.rotate & 0xc;
        if (mirror_info != 0) {
            dec_open_params.mirrorEnable = 1;
            if (mirror_info >> 2 == 1) {
                dec_open_params.mirrorDirection = 1;
            } else if (mirror_info >> 2 == 2) {
                dec_open_params.mirrorDirection = 2;
            } else {
                dec_open_params.mirrorDirection = 3;
            }
        }

        if (input_params.roi.x || input_params.roi.y || input_params.roi.w || input_params.roi.h) {
            dec_open_params.roiEnable = 1;
            dec_open_params.roiWidth = input_params.roi.w;
            dec_open_params.roiHeight = input_params.roi.h;
            dec_open_params.roiOffsetX = input_params.roi.x;
            dec_open_params.roiOffsetY = input_params.roi.y;
        }

        ret = bm_jpu_jpeg_dec_open(&jpeg_decoder, &dec_open_params, 0);
        if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
            fprintf(stderr, "open jpeg decoder failed!\n");
            goto cleanup;
        }

        fseek(fp_output, 0, SEEK_SET);
        ret = start_decode(jpeg_decoder, bs_buffer, bs_size, fp_output, 0, NULL, input_params.dump_crop);
        if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
            goto cleanup;
        }

        /* close the jpeg decoder */
        bm_jpu_jpeg_dec_close(jpeg_decoder);
        jpeg_decoder = NULL;
        printf("loop %d finished\n", i);
    }

cleanup:
    if (bs_buffer != NULL) {
        free(bs_buffer);
        bs_buffer = NULL;
    }

    if (jpeg_decoder != NULL) {
        bm_jpu_jpeg_dec_close(jpeg_decoder);
        jpeg_decoder = NULL;
    }

    if (fp_input != NULL) {
        fclose(fp_input);
    }

    if (fp_output != NULL) {
        fclose(fp_output);
    }

    bm_jpu_dec_unload(0);

    return ret;
}