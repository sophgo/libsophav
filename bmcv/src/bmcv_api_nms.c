#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "bmcv_common.h"
#include "bmcv_internal.h"
#include <stdlib.h>


#define SG_API_ID_NMS 49
#define MAX_PROPOSAL_NUM_TPU 20000

static int compareBBox(const void* a, const void* b)
{
    const face_rect_t* rectA = (const face_rect_t*)a;
    const face_rect_t* rectB = (const face_rect_t*)b;

    return (rectB->score > rectA->score) - (rectB->score < rectA->score);
}

static float iou(face_rect_t a, face_rect_t b)
{
    float x1 = bm_max(a.x1, b.x1);
    float y1 = bm_max(a.y1, b.y1);
    float x2 = bm_min(a.x2, b.x2);
    float y2 = bm_min(a.y2, b.y2);

    if (x2 < x1 || y2 < y1) return 0;

    float a_width = a.x2 - a.x1 + 1;
    float a_height = a.y2 - a.y1 + 1;
    float b_width = b.x2 - b.x1 + 1;
    float b_height = b.y2 - b.y1 + 1;

    float inter_area = (x2 - x1 + 1) * (y2 - y1 + 1);
    float iou = inter_area / ((a_width * a_height) + (b_width * b_height) - inter_area);

    return iou;
}

static int cpu_nms(face_rect_t* proposals, const float nms_threshold, face_rect_t* nmsProposals,
                    int size, int* result_size)
{
    bool* keep = (bool*)malloc(size * sizeof(bool));
    face_rect_t* bboxes = (face_rect_t*)malloc(size * sizeof(face_rect_t));
    *result_size = 0;
    int i, j;

    memset(keep, true, size * sizeof(bool));
    memcpy(bboxes, proposals, size * sizeof(face_rect_t));
    qsort(bboxes, size, sizeof(bboxes[0]), compareBBox);

    for (i = 0; i < size; ++i) {
        if (keep[i]) {
            for(j = i + 1; j < size; j++) {
                if (keep[j] && (iou(bboxes[i], bboxes[j]) > nms_threshold)) {
                    keep[j] = false;
                }
            }
        }
    }

    j = 0;
    for (i = 0; i < size; ++i) {
        if (keep[i]) {
            (*result_size)++;
            nmsProposals[j++] = bboxes[i];
        }
    }

    free(keep);
    free(bboxes);
    return 0;
}

bm_status_t bmcv_nms(bm_handle_t handle, bm_device_mem_t input_proposal_addr, int proposal_size,
                    float nms_threshold, bm_device_mem_t output_proposal_addr)
{
    bm_api_cv_nms_t api;
    bm_device_mem_t input_proposal_buf_device;
    bm_device_mem_t output_buf_device;
    bm_device_mem_t iou_buf_device;
    face_rect_t* input_proposals = NULL;
    nms_proposal_t* output_proposals = NULL;
    face_rect_t* nms_proposal;
    int nms_type = HARD_NMS;
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid;
    int result_size = 0;

    if (handle == NULL) {
        BMCV_ERR_LOG("Can not get handle!\r\n");
        return BM_ERR_FAILURE;
    }

    if (proposal_size < MIN_PROPOSAL_NUM || proposal_size > MAX_PROPOSAL_NUM) {
        BMCV_ERR_LOG("proposal_size not support:%d\r\n", proposal_size);
        return BM_NOT_SUPPORTED;
    }

    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("get chipid is error !\n");
        return ret;
    }

    if (proposal_size > MAX_PROPOSAL_NUM_TPU) {
        if (bm_mem_get_type(input_proposal_addr) == BM_MEM_TYPE_DEVICE) {
            input_proposals = (face_rect_t*)malloc(proposal_size * sizeof(face_rect_t));
            ret = bm_memcpy_d2s(handle, input_proposals, input_proposal_addr);
            if (ret != BM_SUCCESS) {
                BMCV_ERR_LOG("bm_memcpy_d2s error\r\n");
                free(input_proposals);
                return BM_ERR_FAILURE;
            }
        } else {
            input_proposals = (face_rect_t *)bm_mem_get_system_addr(input_proposal_addr);
        }

        if (bm_mem_get_type(output_proposal_addr) == BM_MEM_TYPE_DEVICE) {
            output_proposals = (nms_proposal_t*)malloc(sizeof(nms_proposal_t));
        } else {
            output_proposals = (nms_proposal_t*)bm_mem_get_system_addr(output_proposal_addr);
        }

        nms_proposal = (face_rect_t*)malloc(proposal_size * sizeof(face_rect_t));
        cpu_nms(input_proposals, nms_threshold, nms_proposal, proposal_size, &result_size);

        output_proposals->size = result_size;
        memcpy(output_proposals->face_rect, nms_proposal, output_proposals->size * sizeof(face_rect_t));

        if (bm_mem_get_type(output_proposal_addr) == BM_MEM_TYPE_DEVICE) {
            ret = bm_memcpy_s2d(handle, output_proposal_addr, output_proposals);
            if (ret != BM_SUCCESS) {
                BMCV_ERR_LOG("bm_memcpy_s2d error\r\n");
                return BM_ERR_FAILURE;
            }
            free(output_proposals);
        }

        if (bm_mem_get_type(input_proposal_addr) == BM_MEM_TYPE_DEVICE) {
            free(input_proposals);
        }

        return BM_SUCCESS;
    }

    if (proposal_size <= MAX_PROPOSAL_NUM_TPU) {
        if (bm_mem_get_type(input_proposal_addr) == BM_MEM_TYPE_SYSTEM) {
            ret = bm_malloc_device_byte(handle, &input_proposal_buf_device, sizeof(face_rect_t) * proposal_size);
            if (ret != BM_SUCCESS) {
                BMCV_ERR_LOG("bm_malloc_device_byte error\r\n");
                goto err0;
            }
            ret = bm_memcpy_s2d(handle, input_proposal_buf_device, bm_mem_get_system_addr(input_proposal_addr));
            if (ret != BM_SUCCESS) {
                BMCV_ERR_LOG("bm_memcpy_s2d error\r\n");
                goto err1;
            }
        } else {
            input_proposal_buf_device = input_proposal_addr;
        }

        if (bm_mem_get_type(output_proposal_addr) == BM_MEM_TYPE_SYSTEM) {
            ret = bm_malloc_device_byte(handle, &output_buf_device, sizeof(nms_proposal_t));
            if (ret != BM_SUCCESS) {
                BMCV_ERR_LOG("bm_malloc_device_byte error\r\n");
                goto err1;
            }
        } else {
            output_buf_device = output_proposal_addr;
        }

        api.input_proposal_addr = bm_mem_get_device_addr(input_proposal_buf_device);
        api.output_proposal_addr = bm_mem_get_device_addr(output_buf_device);
        api.proposal_size = proposal_size;
        api.nms_threshold = nms_threshold;
        api.nms_type = nms_type;
        api.score_threshold = 0;
        api.iou_addr = 0;
        api.sigma = 0;
        api.weighting_method = 0;
        api.eta = 0;
        api.hard_nms_version = 1;
        api.keep_top_k = proposal_size;

        ret = bm_malloc_device_byte(handle, &iou_buf_device, proposal_size * sizeof(float));
        if (ret != BM_SUCCESS) {
            BMCV_ERR_LOG("bm_malloc_device_byte error\r\n");
            goto err2;
        }
        api.iou_addr = bm_mem_get_device_addr(iou_buf_device);
        api.all_mask_addr = 0;

        switch(chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_kernel_main_launch(handle, SG_API_ID_NMS, (void *)&api, sizeof(api));
                if (ret != BM_SUCCESS) {
                    printf("bm_tpu_kernel_launch failed!\n");
                    goto err3;
                }
                break;
            default:
                printf("BM_NOT_SUPPORTED!\n");
                ret = BM_NOT_SUPPORTED;
                break;
        }
        if (bm_mem_get_type(output_proposal_addr) == BM_MEM_TYPE_SYSTEM) {
            ret = bm_memcpy_d2s(handle, bm_mem_get_system_addr(output_proposal_addr), output_buf_device);
            if (ret != BM_SUCCESS) {
                BMCV_ERR_LOG("bm_memcpy_d2s error\r\n");
                goto err3;
            }
        }

        err3:
            bm_free_device(handle, iou_buf_device);
        err2:
            if (bm_mem_get_type(output_proposal_addr) == BM_MEM_TYPE_SYSTEM) {
                bm_free_device(handle, output_buf_device);
            }

        err1:
            if (bm_mem_get_type(input_proposal_addr) == BM_MEM_TYPE_SYSTEM) {
                bm_free_device(handle, input_proposal_buf_device);
            }
        err0:
            return ret;
    }

    return BM_SUCCESS;
}