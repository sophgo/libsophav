#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include "jpeg_dec_common.h"
#include "bm_jpeg_internal.h"

#define MAX_DATASET_PICS_NUM 1024
#define MAX_PICS_PATH_LENGTH 1024

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
        "\t--cbcr_interleave            Cb/Cr component is interleaved or not (default 0) [0: planared, 1: Cb/Cr interleaved, 2: Cr/Cb interleaved]\n"
        "\t--external_memory            bitstream and framebuffer memory is allocated by user\n"
        "\t--bitstream_size             bitstream buffer size set by user\n"
        "\t--framebuffer_num            params sync with BM1684(X), invalid on BM1688\n"
        "\t--framebuffer_size           framebuffer size set by user (only valid if external_memory is enabled)\n"
        "\t--bs_heap                    params sync with BM1684(X), invalid on BM1688\n"
        "\t--fb_heap                    params sync with BM1684(X), invalid on BM1688\n"
        "\t--timeout_on_open            set timeout on decoder open\n"
        "\t--timeout_count_on_open      set timeout count on decoder open\n"
        "\t--timeout_on_process         set timeout on bm_jpu_jpeg_dec_decode\n"
        "\t--timeout_count_on_process   set timeout count on bm_jpu_jpeg_dec_decode\n"
        "\t--min_width                  set the min width that the decoder could resolve (default 0)\n"
        "\t--min_height                 set the min height that the decoder could resolve (default 0)\n"
        "\t--max_width, -w              set the max width that the decoder could resolve (default 0)\n"
        "\t--max_height, -h             set the max height that the decoder could resolve (default 0)\n"
        ;
    fprintf(stderr, "usage:\t%s [option]\n\noption:\n%s\n", program, options);
}

extern char *optarg;
static int parse_args(int argc, char *argv[], DecInputParam *params)
{
    int opt;
    int option_index = 0;
    static struct option long_options[] = {
        {"help", no_argument, NULL, '0'},
        {"input", required_argument, NULL, 'i'},
        {"output", required_argument, NULL, 'o'},
        {"dataset_path", required_argument, NULL, '0'},
        {"loop_number", required_argument, NULL, 'n'},
        {"dump_crop", required_argument, NULL, 'c'},
        {"rotate", required_argument, NULL, 'g'},
        {"scale", required_argument, NULL, 's'},
        {"roi_rect", required_argument, NULL, 'r'},
        {"cbcr_interleave", required_argument, NULL, '0'},
        {"external_memory", no_argument, NULL, '0'},
        {"bitstream_size", required_argument, NULL, '0'},
        {"framebuffer_num", required_argument, NULL, '0'},
        {"framebuffer_size", required_argument, NULL, '0'},
        {"bs_heap", required_argument, NULL, '0'},
        {"fb_heap", required_argument, NULL, '0'},
        {"timeout_on_open", required_argument, NULL, '0'},
        {"timeout_count_on_open", required_argument, NULL, '0'},
        {"timeout_on_process", required_argument, NULL, '0'},
        {"timeout_count_on_process", required_argument, NULL, '0'},
        {"min_width", required_argument, NULL, '0'},
        {"min_height", required_argument, NULL, '0'},
        {"max_width", required_argument, NULL, 'w'},
        {"max_height", required_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    memset(params, 0, sizeof(DecInputParam));

    while ((opt = getopt_long(argc, argv, "i:o:n:d:c:g:s:r:w:h:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'i':
                params->input_filename = optarg;
                break;
            case 'o':
                params->output_filename = optarg;
                break;
            case 'n':
                params->loop_num = atoi(optarg);
                break;
            case 'c':
                params->dump_crop = atoi(optarg);
                break;
            case 'g':
                params->rotate = atoi(optarg);
                break;
            case 's':
                params->scale = atoi(optarg);
                break;
            case 'r':
                sscanf(optarg, "%d,%d,%d,%d", &params->roi.x, &params->roi.y, &params->roi.w, &params->roi.h);
                break;
            case 'w':
                params->max_width = atoi(optarg);
                break;
            case 'h':
                params->max_height = atoi(optarg);
                break;
            case '0':
                if (!strcmp(long_options[option_index].name, "dataset_path")) {
                    params->dataset_path = optarg;
                } else if (!strcmp(long_options[option_index].name, "cbcr_interleave")) {
                    params->cbcr_interleave = atoi(optarg);
                } else if (!strcmp(long_options[option_index].name, "external_memory")) {
                    params->external_mem = 1;
                } else if (!strcmp(long_options[option_index].name, "bitstream_size")) {
                    params->bs_size = atoi(optarg);
                } else if (!strcmp(long_options[option_index].name, "framebuffer_size")) {
                    params->fb_size = atoi(optarg);
                } else if (!strcmp(long_options[option_index].name, "timeout_on_open")) {
                    params->timeout_on_open = atoi(optarg);
                } else if (!strcmp(long_options[option_index].name, "timeout_count_on_open")) {
                    params->timeout_count_on_open = atoi(optarg);
                } else if (!strcmp(long_options[option_index].name, "timeout_on_process")) {
                    params->timeout_on_process = atoi(optarg);
                } else if (!strcmp(long_options[option_index].name, "timeout_count_on_process")) {
                    params->timeout_count_on_process = atoi(optarg);
                } else if (!strcmp(long_options[option_index].name, "min_width")) {
                    params->min_width = atoi(optarg);
                } else if (!strcmp(long_options[option_index].name, "min_height")) {
                    params->min_height = atoi(optarg);
                } else if (!strcmp(long_options[option_index].name, "max_width")) {
                    params->max_width = atoi(optarg);
                } else if (!strcmp(long_options[option_index].name, "max_height")) {
                    params->max_height = atoi(optarg);
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

    if (params->input_filename == NULL) {
        fprintf(stderr, "Missing input filename\n");
        usage(argv[0]);
        return -1;
    }

    if (params->output_filename == NULL) {
        fprintf(stderr, "Missing output filename\n");
        usage(argv[0]);
        return -1;
    }

    if (params->loop_num <= 0) {
        params->loop_num = 1;
    }

    if (params->max_width < 0 || params->max_height < 0) {
        fprintf(stderr, "Invalid params: max_width and max_height could not be less than 0\n\n");
        usage(argv[0]);
        return -1;
    }

    if (params->bs_size < 0) {
        fprintf(stderr, "Invalid params: bitstream_size could not be less than 0\n\n");
        usage(argv[0]);
        return -1;
    }

#define BS_SIZE_MASK 0x4000
    params->bs_size = (params->bs_size + BS_SIZE_MASK - 1) & ~(BS_SIZE_MASK - 1);

    if (params->external_mem) {
        if (params->fb_size < 0) {
            fprintf(stderr, "Invalid params: framebuffer_size could not be less than 0\n\n");
            usage(argv[0]);
            return -1;
        }
    }
    return 0;
}

int load_pics(char* dataset_path, int* pic_nums, char** input_names, char** output_names)
{
    int ret = 0;
    int i = 0;
    DIR * dir = NULL;
    struct dirent * dirp;

    dir = opendir(dataset_path);
    if(dir == NULL)
    {
        printf("open %s fail\n", dataset_path);
        ret = -1;
    }

    while(1)
    {
        dirp = readdir(dir);
        if(dirp == NULL)
        {
            break;
        }

        if (0 == strcmp(dirp->d_name, ".") || 0 == strcmp(dirp->d_name, ".."))
            continue;

        input_names[i] = (char*)malloc(MAX_PICS_PATH_LENGTH);
        output_names[i] = (char*)malloc(MAX_PICS_PATH_LENGTH);
        sprintf(input_names[i], "%s/%s", dataset_path, dirp->d_name);
        sprintf(output_names[i], "%s%s%s", "out_", dirp->d_name, ".yuv");

        i++;
    }

    *pic_nums = i;
    return ret;
}

int main(int argc, char *argv[])
{
    BmJpuDecOpenParams open_params;
    BmJpuJPEGDecoder *jpeg_decoder = NULL;
    FILE **input_files = NULL;
    FILE **output_files = NULL;
    uint8_t **bs_buf = NULL;
    char **dataset_inputs = NULL;
    char **dataset_outputs = NULL;
    DecInputParam dec_params;
    int ret = 0;
    int i = 0, j = 0;
    int *input_size = 0;
    int pics_num = 1;
    bm_handle_t bm_handle = NULL;
    bm_device_mem_t bitstream_buffer = {0};
    bm_device_mem_t frame_buffer = {0};
    bm_jpu_phys_addr_t frame_buffer_phys_addr = 0;

    ret = parse_args(argc, argv, &dec_params);
    if (ret != 0) {
        return -1;
    }

    if(dec_params.dataset_path){
        dataset_inputs = malloc(MAX_DATASET_PICS_NUM*sizeof(char*));
        dataset_outputs = malloc(MAX_DATASET_PICS_NUM*sizeof(char*));
        ret = load_pics(dec_params.dataset_path, &pics_num, dataset_inputs, dataset_outputs);
    } else {
        dataset_inputs = malloc(sizeof(char*));
        dataset_outputs = malloc(sizeof(char*));
        dataset_inputs[0] = dec_params.input_filename;
        dataset_outputs[0] = dec_params.output_filename;
    }

    input_files = malloc(pics_num * sizeof(FILE*));
    output_files = malloc(pics_num * sizeof(FILE*));
    input_size = malloc(pics_num * sizeof(int));
    bs_buf = malloc(pics_num * sizeof(uint8_t*));

    for(i=0; i<pics_num; i++)
    {
        input_files[i] = fopen(dataset_inputs[i], "rb");
        if (input_files[i] == NULL) {
            fprintf(stderr, "Opening %s for reading failed: %s\n", dec_params.input_filename, strerror(errno));
            ret = -1;
            goto cleanup;
        }

        output_files[i] = fopen(dataset_outputs[i], "wb");
        if (output_files[i] == NULL) {
            fprintf(stderr, "Opening %s for writing failed: %s\n", dec_params.output_filename, strerror(errno));
            ret = -1;
            goto cleanup;
        }

        /* Determine size of the input file to be able to read all of its bytes in one go */
        fseek(input_files[i], 0, SEEK_END);
        input_size[i] = ftell(input_files[i]);
        fseek(input_files[i], 0, SEEK_SET);

        printf("encoded input frame size: %d bytes\n", input_size[i]);

        /* Allocate buffer for the input data, and read the data into it */
        bs_buf[i] = (uint8_t*)malloc(input_size[i]);
        if (bs_buf[i] == NULL) {
            fprintf(stderr, "malloc failed!\n");
            ret = -1;
            goto cleanup;
        }

        if (fread(bs_buf[i], 1, input_size[i], input_files[i]) < (size_t)input_size[i]) {
            printf("failed to read in whole file!\n");
            ret = -1;
            goto cleanup;
        }
    }

    /* User memory config */
    if (dec_params.external_mem) {
        ret = bm_dev_request(&bm_handle, 0);
        if (ret != BM_SUCCESS) {
            fprintf(stderr, "failed to request handle\n");
            ret = -1;
            goto cleanup;
        }

        if (dec_params.bs_size) {
            ret = bm_malloc_device_byte_heap(bm_handle, &bitstream_buffer, 1, dec_params.bs_size);
            if (ret != BM_SUCCESS) {
                fprintf(stderr, "failed to malloc bitstream buffer, size=%d\n", dec_params.bs_size);
                ret = -1;
                goto cleanup;
            }
            printf("external bs: %lx\n", bitstream_buffer.u.device.device_addr);
        }

        if (dec_params.fb_size) {
            ret = bm_malloc_device_byte_heap(bm_handle, &frame_buffer, 1, dec_params.fb_size);
            if (ret != BM_SUCCESS) {
                fprintf(stderr, "failed to malloc frame buffer, size=%d\n", dec_params.fb_size);
                ret = -1;
                goto cleanup;
            }
            frame_buffer_phys_addr = frame_buffer.u.device.device_addr;
            printf("external fb: %llx\n", frame_buffer_phys_addr);
        }
    }

    bm_jpu_set_logging_threshold(BM_JPU_LOG_LEVEL_ERROR);

    /* Load JPU */
    ret = bm_jpu_dec_load(0);
    if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
        ret = -1;
        goto cleanup;
    }

    memset(&open_params, 0, sizeof(BmJpuDecOpenParams));
    open_params.min_frame_width = dec_params.min_width;
    open_params.min_frame_height = dec_params.min_height;
    open_params.max_frame_width = dec_params.max_width;
    open_params.max_frame_height = dec_params.max_height;
    open_params.chroma_interleave = (BmJpuChromaFormat)dec_params.cbcr_interleave;
    open_params.bs_buffer_size = dec_params.bs_size;
    open_params.device_index = 0;
    open_params.timeout = dec_params.timeout_on_open;
    open_params.timeout_count = dec_params.timeout_count_on_open;
    if (dec_params.roi.w && dec_params.roi.h) {
        open_params.roiEnable = 1;
        open_params.roiWidth = dec_params.roi.w;
        open_params.roiHeight = dec_params.roi.h;
        open_params.roiOffsetX = dec_params.roi.x;
        open_params.roiOffsetY = dec_params.roi.y;
    }

    /* Get rotate angle */
    if ((dec_params.rotate & 0x3) != 0) {
        open_params.rotationEnable = 1;
        if ((dec_params.rotate & 0x3) == 1) {
            open_params.rotationAngle = BM_JPU_ROTATE_90;
        } else if ((dec_params.rotate & 0x3) == 2) {
            open_params.rotationAngle = BM_JPU_ROTATE_180;
        } else if ((dec_params.rotate & 0x3) == 3) {
            open_params.rotationAngle = BM_JPU_ROTATE_270;
        }
    }
    /* Get mirror direction */
    if ((dec_params.rotate & 0xc) != 0) {
        open_params.mirrorEnable = 1;
        if ((dec_params.rotate & 0xc) >> 2 == 1) {
            open_params.mirrorDirection = BM_JPU_MIRROR_VER;
        } else if ((dec_params.rotate & 0xc) >> 2 == 2) {
            open_params.mirrorDirection = BM_JPU_MIRROR_HOR;
        } else if ((dec_params.rotate & 0xc) >> 2 == 3) {
            open_params.mirrorDirection = BM_JPU_MIRROR_HOR_VER;
        }
    }

    /* Set scale */
    open_params.scale_ratio = dec_params.scale;

    /* Set external memory */
    if (dec_params.external_mem) {
        if (dec_params.bs_size) {
            open_params.bitstream_from_user = 1;
            open_params.bs_buffer_phys_addr = bitstream_buffer.u.device.device_addr;
        }

        if (dec_params.fb_size) {
            open_params.framebuffer_from_user = 1;
            open_params.framebuffer_num = 1;
            open_params.framebuffer_phys_addrs = &frame_buffer_phys_addr;
            open_params.framebuffer_size = dec_params.fb_size;
        }
    }

    ret = bm_jpu_jpeg_dec_open(&jpeg_decoder, &open_params, 0);
    if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
        ret = -1;
        goto cleanup;
    }

    for (i = 0; i < dec_params.loop_num; i++) {
        /* Open the JPEG decoder */
        for(j=0; j<pics_num; j++){
            fseek(output_files[j], 0, SEEK_SET);
            ret = start_decode(jpeg_decoder, bs_buf[j], input_size[j], output_files[j], NULL, dec_params.dump_crop, dec_params.timeout_on_process, dec_params.timeout_count_on_process);
            if (ret != 0)
                goto cleanup;
        }
    }
    /* Shut down the JPEG decoder */
    bm_jpu_jpeg_dec_close(jpeg_decoder);
    jpeg_decoder = NULL;

cleanup:
    if (jpeg_decoder != NULL) {
        /* Shut down the JPEG decoder */
        bm_jpu_jpeg_dec_close(jpeg_decoder);
        jpeg_decoder = NULL;
    }

    /* Unload JPU */
    bm_jpu_dec_unload(0);

    /* Free external memory */
    if (bm_handle != NULL) {
        if (frame_buffer.size != 0) {
            bm_free_device(bm_handle, frame_buffer);
        }

        if (bitstream_buffer.size != 0) {
            bm_free_device(bm_handle, bitstream_buffer);
        }

        bm_dev_free(bm_handle);
        bm_handle = NULL;
    }

    /* Input data is not needed anymore, so free the input buffer */
    for(i=0; i<pics_num; i++)
    {
        if (bs_buf[i] != NULL) {
            free(bs_buf[i]);
            bs_buf[i] = NULL;
        }

        if (input_files[i] != NULL) {
            fclose(input_files[i]);
            input_files[i] = NULL;
        }

        if (output_files[i] != NULL) {
            fclose(output_files[i]);
            output_files[i] = NULL;
        }
    }

    free(input_files);
    free(output_files);
    free(input_size);
    free(bs_buf);

    return ret;
}
