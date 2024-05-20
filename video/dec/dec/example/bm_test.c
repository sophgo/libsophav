/*****************************************************************************
 *
 *    Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
 *
 *    bmvid is licensed under the 2-Clause BSD License except for the
 *    third-party components.
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#ifdef __linux__
#include <unistd.h>
#include <getopt.h>
#include <semaphore.h>
#include <sys/time.h>
#include <pthread.h>
#elif _WIN32
#include <windows.h>
#include <time.h>
#include "windows/libusb-1.0.18/examples/getopt/getopt.h"
#endif
#include <time.h>
#include <termios.h>

#include "osal.h"
#include "bm_vpudec_interface.h"

#define defaultReadBlockLen 0x80000
int readBlockLen = defaultReadBlockLen;
int injectionPercent = 0;
int injectLost = 1; // default as lost data, or scramble the data
int injectWholeBlock = 0;

typedef struct BMTestConfig_struct {
    int streamFormat;
    int compareType;
    int compareNum;
    int instanceNum;
    int bsMode;

	int wirteYuv;
    int outputFormat;

#ifdef	BM_PCIE_MODE
    int pcie_board_id;
#endif
    BOOL result;
    int log_level;
    int compare_skip_num;
    int first_comp_idx;

    int wtlFormat;
    BMVidCodHandle vidCodHandle;
    char outputPath[MAX_FILE_PATH];
    char inputPath[MAX_FILE_PATH];
    char refYuvPath[MAX_FILE_PATH];

    unsigned char bStop;    /* replace bmvpu_dec_get_status */
} BMTestConfig;

static int write_yuv(BMVidCodHandle vidCodHandle, BMVidFrame *stVFrame, int frame_idx)
{
	unsigned int i = 0;
    int core_idx, inst_idx;

    core_idx = bmvpu_dec_get_core_idx(vidCodHandle);
    inst_idx = bmvpu_dec_get_inst_idx(vidCodHandle);

    if(stVFrame->compressed_mode == 0) //yuv
    {
        FILE *yuv_fp = NULL;
        char yuv_filename[256];

        sprintf(yuv_filename, "core%d_inst%d_frame%d_%dx%d.yuv", core_idx, inst_idx, frame_idx, stVFrame->width, stVFrame->height);
        yuv_fp = fopen(yuv_filename, "wb+");
        if(yuv_fp == NULL){
            printf("open output file failed\n");
            return -1;
        }

        if(stVFrame->cbcrInterleave == 0){
            if (stVFrame->stride[0]) {
                for(i=0; i<stVFrame->height; i++)
                    fwrite(stVFrame->buf[0] + i*stVFrame->stride[0], 1, stVFrame->width, yuv_fp);
            }

            if (stVFrame->stride[1]) {
                for(i=0; i<stVFrame->height/2; i++)
                    fwrite(stVFrame->buf[1] + i*stVFrame->stride[1], 1, stVFrame->width/2, yuv_fp);
            }

            if (stVFrame->stride[2]) {
                for(i=0; i<stVFrame->height/2; i++)
                    fwrite(stVFrame->buf[2] + i*stVFrame->stride[2], 1, stVFrame->width/2, yuv_fp);
            }
        }
        else{
            if (stVFrame->stride[0]) {
                for(i=0; i<stVFrame->height; i++)
                    fwrite(stVFrame->buf[0] + i*stVFrame->stride[0], 1, stVFrame->width, yuv_fp);
            }

            if (stVFrame->stride[1]) {
                for(i=0; i<stVFrame->height/2; i++)
                    fwrite(stVFrame->buf[1] + i*stVFrame->stride[1], 1, stVFrame->width, yuv_fp);
            }
        }
        fclose(yuv_fp);
    }
    else{   //fbc
        char fbc_filename[4][256];
        FILE *fbc_fp[4];

        sprintf(fbc_filename[0], "frame%d_fbc_Y.dat", frame_idx);
        sprintf(fbc_filename[1], "frame%d_fbc_Cb.dat", frame_idx);
        sprintf(fbc_filename[2], "frame%d_fbc_Ytable.dat", frame_idx);
        sprintf(fbc_filename[3], "frame%d_fbc_Cbtable.dat", frame_idx);
        fbc_fp[0] = fopen(fbc_filename[0], "wb");
        fbc_fp[1] = fopen(fbc_filename[1], "wb");
        fbc_fp[2] = fopen(fbc_filename[2], "wb");
        fbc_fp[3] = fopen(fbc_filename[3], "wb");

        fwrite(stVFrame->buf[0], 1, stVFrame->stride[0]*stVFrame->height, fbc_fp[0]);
        fwrite(stVFrame->buf[1], 1, stVFrame->stride[1]*stVFrame->height/2, fbc_fp[1]);
        fwrite(stVFrame->buf[2], 1, stVFrame->stride[2], fbc_fp[2]);
        fwrite(stVFrame->buf[3], 1, stVFrame->stride[3], fbc_fp[3]);

        for(int i=0; i<4; i++)
            fclose(fbc_fp[i]);
    }

	return 0;
}

static int write_yuv2(FILE *out_f, BMVidFrame *stVFrame)
{
    unsigned int i = 0;

    if(stVFrame->cbcrInterleave == 0){
        if (stVFrame->stride[0]) {
            for(i=0; i<stVFrame->height; i++)
                fwrite(stVFrame->buf[0] + i*stVFrame->stride[0], 1, stVFrame->width, out_f);
        }

        if (stVFrame->stride[1]) {
            for(i=0; i<stVFrame->height/2; i++)
                fwrite(stVFrame->buf[1] + i*stVFrame->stride[1], 1, stVFrame->width/2, out_f);
        }

        if (stVFrame->stride[2]) {
            for(i=0; i<stVFrame->height/2; i++)
                fwrite(stVFrame->buf[2] + i*stVFrame->stride[2], 1, stVFrame->width/2, out_f);
        }
    }
    else{
        if (stVFrame->stride[0]) {
            for(i=0; i<stVFrame->height; i++)
                fwrite(stVFrame->buf[0] + i*stVFrame->stride[0], 1, stVFrame->width, out_f);
        }

        if (stVFrame->stride[1]) {
            for(i=0; i<stVFrame->height/2; i++)
                fwrite(stVFrame->buf[1] + i*stVFrame->stride[1], 1, stVFrame->width, out_f);
        }
    }
}

int ret = 0;

static int parse_args(int argc, char **argv, BMTestConfig* par);

static void process_output(void* arg)
{
    int Ret = 0;
    BMTestConfig *testConfigPara = (BMTestConfig *)arg;
    BMVidCodHandle vidCodHandle = (BMVidCodHandle)testConfigPara->vidCodHandle;
    // BMVidHandle vidHandle = (BMVidHandle)vidCodHandle;
    struct timeval TimeoutVal;
    fd_set read_fds;
    int fd;
#ifdef BM_PCIE_MODE
    DecHandle decHandle = vidHandle->codecInst;
    int coreIdx = decHandle->coreIdx;
#endif
    BMVidFrame *pFrame=NULL;
    BOOL match, alloced = FALSE, result = TRUE;
    FILE* fpRef;
    uint8_t* pRefMem = NULL;
    int32_t readLen = -1, compare = 0, compareNum = 1;
    uint32_t frameSize = 0, ySize = 0, allocedSize = 0;
    int total_frame = 0, cur_frame_idx = 0;
    uint64_t start_time, dec_time;
    VLOG(INFO, "Enter process_output!\n");

    FILE *fpOut = NULL;
    int frame_write_num = testConfigPara->wirteYuv;
    int writeYuv = 0;
    writeYuv = testConfigPara->wirteYuv;
    if (testConfigPara->compareType && testConfigPara->refYuvPath)
    {
        fpRef = fopen(testConfigPara->refYuvPath, "rb");
        if(fpRef == NULL) {
            VLOG(ERR, "Can't open reference yuv file: %s\n", testConfigPara->refYuvPath);
        }
        else {
            compare = testConfigPara->compareType;
        }
    }

    int core_idx = bmvpu_dec_get_core_idx(vidCodHandle);
    int inst_idx = bmvpu_dec_get_inst_idx(vidCodHandle);

    start_time = osal_gettime();
    pFrame = (BMVidFrame *)malloc(sizeof(BMVidFrame));
    fd = bmvpu_dec_get_device_fd(vidCodHandle);

    // while(bmvpu_dec_get_status(vidCodHandle)<=BMDEC_CLOSE) /* bm_vpu_dec_get_status未实现 */
    while(testConfigPara->bStop != 1)
    {
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        Ret = select(fd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if(Ret < 0){
            continue;
        }

        if((Ret = bmvpu_dec_get_output(vidCodHandle, pFrame)) == BM_SUCCESS)
        {
            //VLOG(INFO, "get a frame, display index: %d\n", pFrame->frameIdx);
            /* compare yuv if there is reference yum file */
            if(compare)
            {
                ySize = pFrame->stride[0]*pFrame->height;
                frameSize = ySize*3/2;

                if((alloced == TRUE) && (frameSize != allocedSize)) //if sequence change, free memory then malloc
                {
                    free(pRefMem);
                    alloced = FALSE;
                }

                if(alloced == FALSE)
                {
                    pRefMem = malloc(frameSize);
                    if(pRefMem == NULL)
                    {
                        VLOG(ERR, "Can't alloc reference yuv memory\n");
                        break;
                    }
                    alloced = TRUE;
                    allocedSize = frameSize;
                }

                if(total_frame == 0) {
                    fseek(fpRef, 0L, SEEK_END);
                    if(compare == 1)
                        total_frame = ftell(fpRef)/frameSize;
                    else
                        total_frame = ftell(fpRef)/99;
                    fseek(fpRef, 0L, SEEK_SET);
                }

                if(cur_frame_idx==total_frame && result == TRUE) {
                    //srand((unsigned)time(NULL));
                    if(testConfigPara->compare_skip_num > 0) {
                        testConfigPara->first_comp_idx = rand()%(testConfigPara->compare_skip_num + 1);
                        //printf("first compare frame idx : %d\n", testConfigPara->first_comp_idx);
                    }
                    dec_time = osal_gettime();
                    if(dec_time - start_time != 0)
                        printf("Inst %d: fps %.2f, passed!\n", testConfigPara->instanceNum, (float)total_frame*1000/(dec_time-start_time));
                    start_time = dec_time;
                    compareNum++;
                    cur_frame_idx = 0;
                }

                if((compare == 1) && (result == TRUE)) //yuv compare
                {
                    if(cur_frame_idx<testConfigPara->first_comp_idx || (cur_frame_idx-testConfigPara->first_comp_idx)%(testConfigPara->compare_skip_num+1) != 0)
                    {
                        //result = TRUE;
                        //testConfigPara->result = TRUE;
                        cur_frame_idx += 1;
                        bmvpu_dec_clear_output(vidCodHandle, pFrame);
                        continue;
                    }

                    fseek(fpRef, frameSize*cur_frame_idx, SEEK_SET);

                    readLen = fread(pRefMem, 1, frameSize, fpRef);

                    if(readLen  == frameSize)
                    {
                        uint8_t *buf0 = NULL;
                        uint8_t *buf1 = NULL;
                        uint8_t *buf2 = NULL;
#ifdef BM_PCIE_MODE
                        if(pFrame->size > 0 && pFrame->size <= 8192*4096*3) {
                            //optimize the framebuffer cdma copy.
                            buf0 = malloc(pFrame->size);
                            buf1 = buf0 + (unsigned int)(pFrame->buf[5] - pFrame->buf[4]);
                            buf2 = buf0 + (unsigned int)(pFrame->buf[6] - pFrame->buf[4]);
                        }
                        else {
                            buf0 = NULL;
                        }
                        if(buf0 == NULL) {
                            printf("the frame buffer size maybe error..\n");
                            result = FALSE;
                            break;
                        }
                        vdi_read_memory(coreIdx, (u64)(pFrame->buf[4]), buf0, pFrame->size,   VDI_LITTLE_ENDIAN);
#else
                        buf0 = pFrame->buf[0];
                        buf1 = pFrame->buf[1];
                        buf2 = pFrame->buf[2];
#endif
                        if(pFrame->cbcrInterleave == 0){
                            match = (memcmp(pRefMem, (void*)buf0, ySize) == 0 ? TRUE : FALSE);
                            if (match == FALSE)
                            {
                                result = FALSE;
                                VLOG(ERR, "MISMATCH WITH GOLDEN Y in frame %d\n", cur_frame_idx);
                            }

                            match = (memcmp(pRefMem+ySize, (void*)buf1, ySize/4) == 0 ? TRUE : FALSE);
                            if (match == FALSE)
                            {
                                result = FALSE;
                                VLOG(ERR, "MISMATCH WITH GOLDEN U in frame %d\n", cur_frame_idx);
                            }

                            match = (memcmp(pRefMem+ySize*5/4, (void*)buf2, ySize/4) == 0 ? TRUE : FALSE);
                            if (match == FALSE)
                            {
                                result = FALSE;
                                VLOG(ERR, "MISMATCH WITH GOLDEN V in frame %d\n", cur_frame_idx);
                            }
                        }
                        else{
                            match = (memcmp(pRefMem, (void*)buf0, ySize) == 0 ? TRUE : FALSE);
                            if(match == FALSE)
                            {
                                result = FALSE;
                                VLOG(ERR, "MISMATCH WITH GOLDEN Y in frame %d\n", cur_frame_idx);
                            }

                            match = (memcmp(pRefMem+ySize, (void*)buf1, ySize/2) == 0 ? TRUE : FALSE);
                            if(match == FALSE)
                            {
                                result = FALSE;
                                VLOG(ERR, "MISMATCH WITH GOLDEN Cb、Cr in frame %d\n", cur_frame_idx);
                            }
                        }
#ifdef BM_PCIE_MODE
                        free(buf0);
#endif
                    }
                    else
                    {
                        result = FALSE;
                        VLOG(ERR, "NOT ENOUGH DATA\n");
                    }
                }
                else if((compare == 2) && (result == TRUE)) //yuv md5 compare
                {
                    unsigned char yMd[16], uMd[16], vMd[16];
                    char yMd5Str[33], uMd5Str[33], vMd5Str[33];
                    char yRefMd5Str[33], uRefMd5Str[33], vRefMd5Str[33];
                    int i;
                    if(cur_frame_idx<testConfigPara->first_comp_idx || (cur_frame_idx-testConfigPara->first_comp_idx)%(testConfigPara->compare_skip_num+1) != 0)
                    {
                        //result = TRUE;
                        //testConfigPara->result = TRUE;
                        cur_frame_idx += 1;
                        bmvpu_dec_clear_output(vidCodHandle, pFrame);
                        continue;
                    }

                    fseek(fpRef, 99*cur_frame_idx, SEEK_SET);
#ifdef BM_PCIE_MODE
                    uint8_t *buf0 = NULL;
                    uint8_t *buf1;
                    uint8_t *buf2;
                    if(pFrame->size > 0 && pFrame->size <= 8192*4096*3) {
                        //optimize the framebuffer cdma copy.
                        buf0 = malloc(pFrame->size);
                        buf1 = buf0 + (unsigned int)(pFrame->buf[5] - pFrame->buf[4]);
                        buf2 = buf0 + (unsigned int)(pFrame->buf[6] - pFrame->buf[4]);
                    }
                    else {
                        buf0 = NULL;
                    }
                    if(buf0 == NULL) {
                        printf("the frame buffer size maybe error..\n");
                        result = FALSE;
                        break;
                    }
                    vdi_read_memory(coreIdx, (u64)(pFrame->buf[4]), buf0, pFrame->size,   VDI_LITTLE_ENDIAN);

                    MD5(buf0, ySize, yMd);
                    MD5(buf1, ySize/4, uMd);
                    MD5(buf2, ySize/4, vMd);

                    free(buf0);
#else
                    MD5((uint8_t*)pFrame->buf[0], ySize, yMd);
                    MD5((uint8_t*)pFrame->buf[1], ySize/4, uMd);
                    MD5((uint8_t*)pFrame->buf[2], ySize/4, vMd);
#endif

                    for(i=0; i<16; i++)
                    {
                        snprintf(yMd5Str + i*2, 2+1, "%02x", yMd[i]);
                        snprintf(uMd5Str + i*2, 2+1, "%02x", uMd[i]);
                        snprintf(vMd5Str + i*2, 2+1, "%02x", vMd[i]);
                    }

                    readLen = fread(pRefMem, 1, 99, fpRef);

                    if(readLen == 99)
                    {
                        match = (memcmp(pRefMem, yMd5Str, 32) == 0 ? TRUE : FALSE);
                        if (match == FALSE)
                        {
                            snprintf(yRefMd5Str, 33, "%s", (char *)pRefMem);
                            result = FALSE;
                            VLOG(ERR, "MISMATCH WITH GOLDEN Y in frame %d, %s, %s\n", cur_frame_idx, yMd5Str, yRefMd5Str);
                        }

                        match = (memcmp(pRefMem+33, uMd5Str, 32) == 0 ? TRUE : FALSE);
                        if (match == FALSE)
                        {
                            snprintf(uRefMd5Str, 33, "%s", (char *)(pRefMem+33));
                            result = FALSE;
                            VLOG(ERR, "MISMATCH WITH GOLDEN U in frame %d, %s, %s\n", cur_frame_idx, uMd5Str, uRefMd5Str);
                        }

                        match = (memcmp(pRefMem+66, vMd5Str, 32) == 0 ? TRUE : FALSE);
                        if (match == FALSE)
                        {
                            snprintf(vRefMd5Str, 33, "%s", (char *)(pRefMem+66));
                            result = FALSE;
                            VLOG(ERR, "MISMATCH WITH GOLDEN V in frame %d, %s, %s\n", cur_frame_idx, vMd5Str, vRefMd5Str);
                        }
                    }
                    else
                    {
                        result = FALSE;
                        VLOG(ERR, "NOT ENOUGH DATA\n");
                    }
                }
                if(result == FALSE && testConfigPara->result == TRUE) {
                    int stream_size = 0x700000;
                    int len = 0;

                    int core_idx = bmvpu_dec_get_core_idx(vidCodHandle);
                    int inst_idx = bmvpu_dec_get_inst_idx(vidCodHandle);

                    unsigned char *p_stream = malloc(stream_size);
                    if(ySize != 0) {
                        char yuv_filename[256]={0};
                        FILE *fp = NULL;

                        sprintf(yuv_filename, "core%d_inst%d_frame%d_dump_%dx%d.yuv", core_idx, inst_idx, cur_frame_idx, pFrame->width, pFrame->height);
                        VLOG(ERR, "get error and dump yuvfile: %s\n", yuv_filename);
                        fp = fopen(yuv_filename, "wb+");

                        if(fp != NULL) {
#ifndef BM_PCIE_MODE
                            fwrite(pFrame->buf[0], 1, ySize, fp);
                            if(pFrame->cbcrInterleave == 0){
                                fwrite(pFrame->buf[1], 1, ySize/4, fp);
                                fwrite(pFrame->buf[2], 1, ySize/4, fp);
                            }
                            else{
                                fwrite(pFrame->buf[1], 1, ySize/2, fp);
                            }

#else
                            uint8_t *buf0 = malloc(ySize);
                            uint8_t *buf1 = malloc(ySize/4);
                            uint8_t *buf2 = malloc(ySize/4);

                            vdi_read_memory(coreIdx, (u64)(pFrame->buf[4]), buf0, ySize,   VDI_LITTLE_ENDIAN);
                            vdi_read_memory(coreIdx, (u64)(pFrame->buf[5]), buf1, ySize/4, VDI_LITTLE_ENDIAN);
                            vdi_read_memory(coreIdx, (u64)(pFrame->buf[6]), buf2, ySize/4, VDI_LITTLE_ENDIAN);

                            fwrite(buf0, 1, ySize, fp);
                            fwrite(buf1, 1, ySize/4, fp);
                            fwrite(buf2, 1, ySize/4, fp);

                            free(buf0);
                            free(buf1);
                            free(buf2);
#endif
                            fclose(fp);
                        }

                        if(compare == 1 && pRefMem != NULL) {
                            memset(yuv_filename, 0, 256);
                            sprintf(yuv_filename, "core%d_inst%d_frame%d_gold_%dx%d.yuv", core_idx, inst_idx, cur_frame_idx, pFrame->width, pFrame->height);

                            fp = fopen(yuv_filename, "wb+");

                            if(fp != NULL) {
                                fwrite(pRefMem, 1, ySize*3/2, fp);
                                fclose(fp);
                            }
                        }
                    }
                    else {
                        VLOG(ERR, "get error and ySize: %d\n", ySize);
                    }

                    /* bmvpu_dec_dump_stream is not support now */
                    // if(p_stream != NULL) {
                    //     len = bmvpu_dec_dump_stream(vidCodHandle, p_stream, stream_size);

                    //     if(len > 0) {
                    //         char stream_filename[256]={0};
                    //         FILE *fp = NULL;

                    //         sprintf(stream_filename, "core%d_inst%d_len%d_stream_dump.bin", core_idx, inst_idx, len);
                    //         VLOG(ERR, "get error and dump streamfile: %s\n", stream_filename);
                    //         fp = fopen(stream_filename, "wb+");

                    //         if(fp != NULL) {
                    //             fwrite(p_stream, 1, len, fp);
                    //             fclose(fp);
                    //         }
                    //     }
                    //     free(p_stream);
                    // }
                }
                testConfigPara->result = result;
            }
            else {
                if(cur_frame_idx != 0 && (cur_frame_idx % 1000) == 0) {
                    dec_time = osal_gettime();
                    if(dec_time - start_time != 0)
                        printf("Inst %d: fps %.2f, passed!\n", testConfigPara->instanceNum, (float)(cur_frame_idx+1)*1000/(dec_time-start_time));
                }
            }

            //add write file
            if (writeYuv && frame_write_num != 0) {
#ifdef BM_PCIE_MODE
                char yuv_filename[256] = { 0 };
                memset(yuv_filename,0,256);

                ySize = pFrame->stride[0] * pFrame->height;
                frameSize = ySize * 3 / 2;
                if (cur_frame_idx < testConfigPara->first_comp_idx || (cur_frame_idx - testConfigPara->first_comp_idx) % (testConfigPara->compare_skip_num + 1) != 0)
                {
                    //result = TRUE;
                    //testConfigPara->result = TRUE;
                    cur_frame_idx += 1;
                    bmvpu_dec_clear_output(vidHandle, pFrame);
                    continue;
                }

                uint8_t* buf0 = NULL;
                uint8_t* buf1 = NULL;
                uint8_t* buf2 = NULL;

                if (pFrame->size > 0 && pFrame->size <= 8192 * 4096 * 3) {
                    //optimize the framebuffer cdma copy.
                    buf0 = malloc(pFrame->size);
                    buf1 = buf0 + (unsigned int)(pFrame->buf[5] - pFrame->buf[4]);
                    buf2 = buf0 + (unsigned int)(pFrame->buf[6] - pFrame->buf[4]);
                }
                else {
                    buf0 = NULL;
                }
                if (buf0 == NULL) {
                    printf("the frame buffer size maybe error..\n");
                    break;
                }
                vdi_read_memory(coreIdx, (u64)(pFrame->buf[4]), buf0, pFrame->size, VDI_LITTLE_ENDIAN);

                int core_idx = bmvpu_dec_get_core_idx(vidHandle);
                int inst_idx = bmvpu_dec_get_inst_idx(vidHandle);
                sprintf(yuv_filename, "core%d_inst%d_frame%d_dump_%dx%d.yuv", core_idx, inst_idx, cur_frame_idx, pFrame->width, pFrame->height);
                FILE* fpWriteyuv = NULL;
                fpWriteyuv = fopen(yuv_filename, "wb+");
                if (ySize != 0) {
                    if (fpWriteyuv != NULL) {
                        fwrite(buf0, 1, ySize, fpWriteyuv);
                        fwrite(buf1, 1, ySize / 4, fpWriteyuv);
                        fwrite(buf2, 1, ySize / 4, fpWriteyuv);
                    }
                }
                if (fpWriteyuv) {
                    fflush(fpWriteyuv);
                    fclose(fpWriteyuv);
                    fpWriteyuv = NULL;
                }
                if (buf0)
                    free(buf0);
#else
                write_yuv(vidCodHandle, pFrame, cur_frame_idx);
#endif
                if (frame_write_num > 0){
                    frame_write_num--;
                }
            }

            cur_frame_idx += 1;
            bmvpu_dec_clear_output(vidCodHandle, pFrame);
        }
        else
        {
#ifdef __linux__
            usleep(1000);
#elif _WIN32
            Sleep(100);
#endif
        }
        usleep(100);
    }

    VLOG(INFO, "Exit chn %d core %d process_output!\n", inst_idx, core_idx);
    if(compare)
    {
        fflush(fpRef);
        fclose(fpRef);
        VLOG(INFO, "Inst %d: verification %d %s!\n", testConfigPara->instanceNum, compareNum, result == TRUE ? "passed" :"failed");
        if(result == FALSE) ret = -1;
    }
    if(pRefMem)
        free(pRefMem);
    if(pFrame != NULL)
    {
        free(pFrame);
        pFrame == NULL;
    }
}

static void dec_test(void* arg)
{
    FILE* fpIn;
    uint8_t* pInMem;
    int32_t readLen = -1;
    BMVidStream vidStream;
    BMVidDecParam param = {0};
    BMTestConfig *testConfigPara = (BMTestConfig *)arg;
    BMTestConfig process_output_Para;
    BMVidCodHandle vidHandle;
    char *inputPath = (char *)testConfigPara->inputPath;
    pthread_t vpu_thread;
    int compareNum = 1, num = 0, j = 0;
    struct timeval tv;
    int wrptr = 0;
    uint8_t bEndOfStream, bFindStart, bFindEnd;
    int UsedBytes, FreeLen;

    memcpy(&process_output_Para, testConfigPara, sizeof(BMTestConfig));

    fpIn = fopen(inputPath, "rb");
    if(fpIn==NULL)
    {
        VLOG(ERR, "Can't open input file: %s\n", inputPath);
        ret = -1;
        return;
    }

    param.streamFormat = testConfigPara->streamFormat;
    param.wtlFormat = testConfigPara->wtlFormat;
    param.extraFrameBufferNum = 1;
    param.streamBufferSize = 0x500000;
    param.enable_cache = 1;
    param.bsMode = testConfigPara->bsMode;   /* VIDEO_MODE_STREAM */
    param.core_idx=-1;
    if(testConfigPara->outputFormat == 1)
    {
        param.cbcrInterleave = 1;
        param.nv21 = 0;
    }
    else if(testConfigPara->outputFormat == 2)
    {
        param.cbcrInterleave = 1;
        param.nv21 = 1;
    }
    else
    {
        param.cbcrInterleave = 0;
        param.nv21 = 0;
    }

//    param.wtlFormat = 101;
#ifdef	BM_PCIE_MODE
    param.pcie_board_id = testConfigPara->pcie_board_id;
#endif
    if (bmvpu_dec_create(&vidHandle, param)!=BM_SUCCESS)
    {
        VLOG(ERR, "Can't create decoder.\n");
        ret = -1;
        return;
    }

    VLOG(INFO, "Decoder Create success, chn %d, inputpath %s!\n", (int)*(int *)vidHandle, inputPath);

    pInMem = (unsigned char *)malloc(defaultReadBlockLen);
    if(pInMem == NULL)
    {
        VLOG(ERR, "Can't get input memory\n");
        ret = -1;
        return;
    }

    vidStream.buf = pInMem;
    vidStream.header_size = 0;
    vidStream.pts = 0;

    int core_idx = bmvpu_dec_get_core_idx(vidHandle);
    int inst_idx = bmvpu_dec_get_inst_idx(vidHandle);
    process_output_Para.vidCodHandle = vidHandle;
    process_output_Para.bStop = 0;

    pthread_create(&vpu_thread, NULL, process_output, (void*)&process_output_Para);

    if (testConfigPara->compareNum)
        compareNum = testConfigPara->compareNum;
    else
        compareNum = 1;
#ifdef __linux__
    gettimeofday(&tv, NULL);
    srand((unsigned int)tv.tv_usec);
#elif _WIN32
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    srand((unsigned int)wtm.wSecond);
#endif

    for (num = 0; num < compareNum; num++)
    {
        printf("compare loop = %d, current loop = %d\n", compareNum, num);

        UsedBytes = 0;
        while (1)
        {
            /* code */
            if(testConfigPara->bsMode == 0){ /* BS_MODE_INTERRUPT */
#ifdef BM_PCIE_MODE
                memset(pInMem,0,defaultReadBlockLen);
#endif
                wrptr = 0;
                do{
                    FreeLen = (defaultReadBlockLen-wrptr) >readBlockLen? readBlockLen:(defaultReadBlockLen-wrptr);
                    if( FreeLen == 0 )
                        break;
                    if((readLen = fread(pInMem+wrptr, 1, FreeLen, fpIn))>0)
                    {
                        int toLost = 0;
                        if(readLen != FreeLen)
                            vidStream.end_of_stream = 1;
                        else
                            vidStream.end_of_stream = 0;

                        if(injectionPercent == 0)
                        {
                            wrptr += readLen;
                            if(testConfigPara->result == FALSE)
                            {
                                goto OUT1;
                            }
                        }
                        else{
                            if((rand()%100) <= injectionPercent)
                            {
                                if(injectWholeBlock)
                                {
                                    toLost = readLen;
                                    if (injectLost)
                                        continue;
                                    else
                                    {
                                        for(j=0;j<toLost;j++)
                                            *((char*)(pInMem+wrptr+j)) = rand()%256;
                                        wrptr += toLost;
                                    }
                                }
                                else{
                                    toLost = rand()%readLen;
                                    while(toLost == readLen)
                                    {
                                        toLost = rand()%readLen;
                                    }
                                    if(injectLost)
                                    {
                                        readLen -= toLost;
                                        memset(pInMem+wrptr+readLen, 0, toLost);
                                    }
                                    else
                                    {
                                        for(j=0;j<toLost;j++)
                                            *((char *)(pInMem+wrptr+readLen-toLost+j)) = rand()%256;
                                    }
                                    wrptr += readLen;
                                }
                            }
                            else
                            {
                                wrptr += readLen;
                            }
                        }
                    }
                }while(readLen > 0);
//1682 pcie card cdma need 4B aligned
#ifdef BM_PCIE_MODE
                readLen = (wrptr + 3)/4*4;
                vidStream.length = readLen;
#else
                vidStream.length = wrptr;
#endif
            }
            else{
                bEndOfStream = 0;
                bFindStart = 0;
                bFindEnd = 0;
                int i;

                ret = fseek(fpIn, UsedBytes, SEEK_SET);
                readLen = fread(pInMem, 1, defaultReadBlockLen, fpIn);
                if(readLen == 0){
                    break;
                }

                if(testConfigPara->streamFormat == 0) { /* H264 */
                    for (i = 0; i < readLen - 8; i++) {
                        int tmp = pInMem[i + 3] & 0x1F;

                        if (pInMem[i] == 0 && pInMem[i + 1] == 0 && pInMem[i + 2] == 1 &&
                            (((tmp == 0x5 || tmp == 0x1) && ((pInMem[i + 4] & 0x80) == 0x80)) ||
                            (tmp == 20 && (pInMem[i + 7] & 0x80) == 0x80))) {
                            bFindStart = 1;
                            i += 8;
                            break;
                        }
                    }

                    for (; i < readLen - 8; i++) {
                        int tmp = pInMem[i + 3] & 0x1F;

                        if (pInMem[i] == 0 && pInMem[i + 1] == 0 && pInMem[i + 2] == 1 &&
                            (tmp == 15 || tmp == 7 || tmp == 8 || tmp == 6 ||
                                ((tmp == 5 || tmp == 1) && ((pInMem[i + 4] & 0x80) == 0x80)) ||
                                (tmp == 20 && (pInMem[i + 7] & 0x80) == 0x80))) {
                            bFindEnd = 1;
                            break;
                        }
                    }

                    if (i > 0)
                        readLen = i;
                    if (bFindStart == 0) {
                        printf("chn %d can not find H264 start code!readLen %d, s32UsedBytes %d.!\n",
                                (int)*((int *)vidHandle), readLen, UsedBytes);
                    }
                    if (bFindEnd == 0) {
                        readLen = i + 8;
                    }
                }
                else if(testConfigPara->streamFormat == 12) { /* H265 */
                    int bNewPic = 0;

                    for(i=0; i<readLen-6; i++){
                        int tmp = (pInMem[i + 3] & 0x7E) >> 1;

                        bNewPic = (pInMem[i + 0] == 0 && pInMem[i + 1] == 0 && pInMem[i + 2] == 1 &&
					   (tmp <= 21) && ((pInMem[i + 5] & 0x80) == 0x80));

                        if (bNewPic) {
                            bFindStart = 1;
                            i += 6;
                            break;
                        }
                    }

                    for (; i < readLen - 6; i++) {
                        int tmp = (pInMem[i + 3] & 0x7E) >> 1;

                        bNewPic = (pInMem[i + 0] == 0 && pInMem[i + 1] == 0 && pInMem[i + 2] == 1 &&
                            (tmp == 32 || tmp == 33 || tmp == 34 || tmp == 39 || tmp == 40 ||
                                ((tmp <= 21) && (pInMem[i + 5] & 0x80) == 0x80)));

                        if (bNewPic) {
                            bFindEnd = 1;
                            break;
                        }
                    }
                    if (i > 0)
                        readLen = i;

                    if (bFindEnd == 0) {
                        readLen = i + 6;
                    }
                }
                else{
                    VLOG(ERR, "Error: the stream type is invalid!\n");
                    return -1;
                }

                vidStream.length = readLen;
                vidStream.end_of_stream = 0;
            }

            int result = 0;
            while((result = bmvpu_dec_decode(vidHandle, vidStream))!=BM_SUCCESS){
#ifdef __linux__
                usleep(1000);
#elif _WIN32
                Sleep(1);
#endif
            }

            if(testConfigPara->bsMode == 0){
                if (wrptr < defaultReadBlockLen){
                    break;
                }
            }
            else{
                UsedBytes += readLen;
                vidStream.pts += 30;
            }
            usleep(1000);
        }
        fseek(fpIn, 0, SEEK_SET);
    }
OUT1:
    bmvpu_dec_flush(vidHandle);

    while (bmvpu_dec_get_status(vidHandle)!=BMDEC_STOP)
    {
#ifdef __linux__
        usleep(2);
#elif _WIN32
        Sleep(1);
#endif
    }

    process_output_Para.bStop = 1;

    pthread_join(vpu_thread, NULL);
    VLOG(INFO, "EXIT\n");
    bmvpu_dec_delete(vidHandle);
    free(pInMem);
    pInMem = NULL;
}

static void
Help(const char *programName)
{
    fprintf(stderr, "------------------------------------------------------------------------------\n");
    // fprintf(stderr, "%s(API v%d.%d.%d)\n", programName, API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);
    fprintf(stderr, "%s(API v%d.%d.%d)\n", programName);
    fprintf(stderr, "\tAll rights reserved by Bitmain\n");
    fprintf(stderr, "------------------------------------------------------------------------------\n");
    fprintf(stderr, "%s [option] --input bistream\n", programName);
    fprintf(stderr, "-h                 help\n");
    fprintf(stderr, "-v                 set max log level\n");
    fprintf(stderr, "                   0: none, 1: error(default), 2: warning, 3: info, 4: trace\n");
    fprintf(stderr, "-c                 compare with golden\n");
    fprintf(stderr, "                   0 : no comparison\n");
    fprintf(stderr, "                   1 : compare with golden yuv that specified --ref-yuv option\n");
    fprintf(stderr, "                   2 : compare with golden yuv md5 that specified --ref-yuv option\n");
    fprintf(stderr, "-n                 compare count\n");
    fprintf(stderr, "-m                 bitstream mode\n");
    fprintf(stderr, "                   0 : BS_MODE_INTERRUPT\n");
    fprintf(stderr, "                   2 : BS_MODE_PIC_END\n");
    fprintf(stderr, "--input            bitstream path\n");
    fprintf(stderr, "--output           YUV path\n");
    fprintf(stderr, "--output_format    0 : I420, 1 : NV12, 2 : NV21\n");
    fprintf(stderr, "--stream-type      0,12, default 0 (H.264:0, H.265:12)\n");
    fprintf(stderr, "--ref-yuv          golden yuv path\n");
    fprintf(stderr, "--instance         instance number\n");
	fprintf(stderr, "--write_yuv        0 no writing , num write frame numbers\n");
    fprintf(stderr, "--wtl-format       yuv format. default 0.\n");
    fprintf(stderr, "--read-block-len      block length of read from file, default is 0x80000\n");
    fprintf(stderr, "--inject-percent      percent of blocks to introduce lost/scramble data, will introduce random length of data at %% of blocks, or the whole block \n");
    fprintf(stderr, "--inject-lost         type of injection, default is 1 for data lost, set to 0 for scramble the data\n");
    fprintf(stderr, "--inject-whole-block  lost the whole block, default is lost part of the block\n");

#ifdef	BM_PCIE_MODE
    fprintf(stderr, "--pcie_board_id    select pcie card by pci_board_id\n");
#endif
}

int main(int argc, char **argv)
{
    int i = 0;
    pthread_t vpu_thread[MAX_NUM_INSTANCE * MAX_NUM_VPU_CORE];
    BMTestConfig *testConfigPara = malloc(MAX_NUM_INSTANCE * MAX_NUM_VPU_CORE * sizeof(BMTestConfig));//[MAX_NUM_INSTANCE * MAX_NUM_VPU_CORE];
    BMTestConfig testConfigOption;
    if(testConfigPara == NULL) {
        printf("can not allocate memory. please check it.\n");
        return -1;
    }
    parse_args(argc, argv, &testConfigOption);
    SetMaxLogLevel(testConfigOption.log_level);

    printf("compareNum: %d\n", testConfigOption.compareNum);
    printf("instanceNum: %d\n", testConfigOption.instanceNum);

    for(i = 0; i < testConfigOption.instanceNum; i++)
        memset(&testConfigPara[i], 0, sizeof(BMTestConfig));

    for(i = 0; i < testConfigOption.instanceNum; i++)
    {
        memcpy(&(testConfigPara[i]), &(testConfigOption), sizeof(BMTestConfig));
        testConfigPara[i].instanceNum = i;
        testConfigPara[i].result = TRUE;
        printf("inputpath: %s\n", testConfigPara[i].inputPath);
        printf("refYuvPath: %s\n", testConfigPara[i].refYuvPath);
    }

    for(i = 0; i < testConfigOption.instanceNum; i++)
    {
        pthread_create(&vpu_thread[i], NULL, dec_test, (void*)&testConfigPara[i]);
    }

    for(i = 0; i < testConfigOption.instanceNum; i++)
    {
        pthread_join(vpu_thread[i], NULL);
    }
    free(testConfigPara);
    return ret;
}

static struct option   options[] = {
    {"output",                1, NULL, 0},
    {"input",                 1, NULL, 0},
    {"codec",                 1, NULL, 0},
    {"stream-type",           1, NULL, 0},
    {"ref-yuv",               1, NULL, 0},
    {"instance",              1, NULL, 0},
	{"write_yuv",             1, NULL, 0},
    {"wtl-format",            1, NULL, 0},
    {"comp-skip",             1, NULL, 0},
    {"read-block-len",        1, NULL, 0},
    {"inject-percent",        1, NULL, 0},
    {"inject-lost",           1, NULL, 0},
    {"inject-whole-block",    1, NULL, 0},
    {"output_format",         1, NULL, 0},
#ifdef	BM_PCIE_MODE
    {"pcie_board_id",         1, NULL, 0},
#endif
    {NULL,                    0, NULL, 0},
};

static int parse_args(int argc, char **argv, BMTestConfig* par)
{
    int index;
    int32_t opt;

    /* default setting. */
    memset(par, 0, sizeof(BMTestConfig));

    par->instanceNum = 1;
    par->log_level = 4;
    par->streamFormat = 0; // H264   0 264  12 265
    par->bsMode = 0;

    while ((opt=getopt_long(argc, argv, "v:c:h:n:m:", options, &index)) != -1)
    {
        switch (opt)
        {
        case 'c':
            par->compareType = atoi(optarg);
            break;
        case 'n':
            par->compareNum = atoi(optarg);
            break;
        case 'm':
            par->bsMode = atoi(optarg);
            break;
        case 0:
            if (!strcmp(options[index].name, "output"))
            {
                memcpy(par->outputPath, optarg, strlen(optarg));
                // ChangePathStyle(par->outputPath);
            }
            else if (!strcmp(options[index].name, "input"))
            {
                memcpy(par->inputPath, optarg, strlen(optarg));
                // ChangePathStyle(par->inputPath);
            }
            else if (!strcmp(options[index].name, "ref-yuv"))
            {
                memcpy(par->refYuvPath, optarg, strlen(optarg));
                // ChangePathStyle(par->refYuvPath);
            }
            else if (!strcmp(options[index].name, "stream-type"))
            {
                par->streamFormat = (int)atoi(optarg);
            }
            else if (!strcmp(options[index].name, "instance"))
            {
                par->instanceNum = atoi(optarg);
            }
			else if (!strcmp(options[index].name, "write_yuv"))
            {
                par->wirteYuv = atoi(optarg);
            }
            else if (!strcmp(options[index].name, "wtl-format"))
            {
                par->wtlFormat = (int)atoi(optarg);
            }
            else if(!strcmp(options[index].name, "comp-skip"))
            {
                par->compare_skip_num = atoi(optarg);
            }
            else if(!strcmp(options[index].name, "read-block-len"))
            {
                readBlockLen = atoi(optarg);
            }
            else if(!strcmp(options[index].name, "inject-percent"))
            {
                injectionPercent = atoi(optarg);
            }
            else if(!strcmp(options[index].name, "inject-lost"))
            {
                injectLost = atoi(optarg);
            }
            else if(!strcmp(options[index].name, "inject-whole-block"))
            {
                injectWholeBlock = atoi(optarg);
            }
            else if(!strcmp(options[index].name, "output_format"))
            {
                par->outputFormat = atoi(optarg);
            }
#ifdef	BM_PCIE_MODE
            else if (!strcmp(options[index].name, "pcie_board_id")) {
                par->pcie_board_id = (int)atoi(optarg);
            }
#endif
            break;
        case 'v':
            par->log_level = atoi(optarg);
            break;
        case 'h':
        case '?':
        default:
            fprintf(stderr, "%s\n", optarg);
            Help(argv[0]);
            exit(1);
        }
    }

    if (par->instanceNum <= 0 || par->instanceNum > MAX_NUM_INSTANCE * MAX_NUM_VPU_CORE)
    {
        fprintf(stderr, "Invalid instanceNum(%d)\n", par->instanceNum);
        Help(argv[0]);
        exit(1);
    }

    if (par->log_level < NONE || par->log_level > TRACE)
    {
        fprintf(stderr, "Wrong log level: %d\n", par->log_level);
        Help(argv[0]);
        exit(1);
    }

    return 0;
}
