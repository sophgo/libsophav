#include "bmcv_api_ext_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include "test_misc.h"

#ifdef __linux__
#include <sys/time.h>
#include <time.h>
#else
#include <windows.h>
#include <time.h>
#endif

#define SCORE_RAND_LEN_MAX 50000
#define ERR_MAX 1e-6
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef struct {
    int loop;
    int num;
    float nms_threshold;
    bm_handle_t handle;
} cv_nms_thread_arg_t;

extern int cpu_nms(face_rect_t* proposals, const float nms_threshold, face_rect_t* nmsProposals,
                    int size, int* result_size);

static int result_compare(face_rect_t* cpu_ref, nms_proposal_t* tpu_res, int result_size)
{
    int i;

    /*there is no compare the result size*/
    for (i = 0; i < tpu_res->size; i++) {
        if (cpu_ref[i].x1 != tpu_res->face_rect[i].x1 || cpu_ref[i].x2 != tpu_res->face_rect[i].x2 ||
            cpu_ref[i].y1 != tpu_res->face_rect[i].y1 || cpu_ref[i].y2 != tpu_res->face_rect[i].y2 ||
            fabs(cpu_ref[i].score - tpu_res->face_rect[i].score) > ERR_MAX) {
                printf("error[%d], cpu:x1=%f, x2=%f, y1=%f, y2=%f, score=%f, \
                        tpu:x1=%f, x2=%f, y1=%f, y2=%f, score=%f\n",
                        i, cpu_ref[i].x1, cpu_ref[i].x2, cpu_ref[i].y1, cpu_ref[i].y2, cpu_ref[i].score,
                        tpu_res->face_rect[i].x1, tpu_res->face_rect[i].x2, tpu_res->face_rect[i].y1,
                        tpu_res->face_rect[i].y2, tpu_res->face_rect[i].score);
                return -1;
            }
    }

    return 0;
}

static int tpu_nms(face_rect_t* proposals, const float nms_threshold, nms_proposal_t* nmsProposals,
                    int num, bm_handle_t handle)
{
    bm_status_t ret = BM_SUCCESS;
    struct timeval t1, t2;

    gettimeofday(&t1, NULL);
    ret = bmcv_nms(handle, bm_mem_from_system(proposals), num, nms_threshold, bm_mem_from_system(nmsProposals));
    if (ret != BM_SUCCESS) {
        printf("Calculate bm_nms failed.\n");
        bm_dev_free(handle);
        return -1;
    }
    gettimeofday(&t2, NULL);
    printf("BMCV_NMS TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    return 0;
}

static int test_nms(int num, float nms_threshold, bm_handle_t handle)
{
    int ret = 0;
    int result_size = 0;
    int i;
    face_rect_t* input = (face_rect_t*)malloc(num * sizeof(face_rect_t));
    face_rect_t* cpu_out = (face_rect_t*)malloc(num * sizeof(face_rect_t));
    nms_proposal_t tpu_out[1];
    struct timeval t1, t2;

    for (i = 0; i < num; ++i) {
        input[i].x1 = ((float)(rand() % 100)) / 10;
        input[i].x2 = input[i].x1 + ((float)(rand() % 100)) / 10;
        input[i].y1 = ((float)(rand() % 100)) / 10;
        input[i].y2 = input[i].y1 + ((float)(rand() % 100)) / 10;
        input[i].score = (float)rand() / (float)RAND_MAX;
    }

    gettimeofday(&t1, NULL);
    ret = cpu_nms(input, nms_threshold, cpu_out, num, &result_size);
    if (ret != 0) {
        printf(" test_nms cpu_nms failed!\n");
        goto exit;
    }
    gettimeofday(&t2, NULL);
    printf("BMCV_NMS CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    ret = tpu_nms(input, nms_threshold, tpu_out, num, handle);
    if (ret != 0) {
        printf(" test_nms tpu_nms failed!\n");
        goto exit;
    }

    ret = result_compare(cpu_out, tpu_out, result_size);
    if (ret != 0) {
        printf(" test_nms result_compare failed!\n");
        goto exit;
    }

exit:
    free(cpu_out);
    free(input);
    return ret;
}

void* test_thread_nms(void* args) {
    cv_nms_thread_arg_t* cv_nms_thread_arg = (cv_nms_thread_arg_t*)args;
    int loop = cv_nms_thread_arg->loop;
    int num = cv_nms_thread_arg->num;
    float nms_threshold = cv_nms_thread_arg->nms_threshold;
    bm_handle_t handle = cv_nms_thread_arg->handle;

    for (int i = 0; i < loop; ++i) {
        int ret = test_nms(num, nms_threshold, handle);
        if (ret) {
            printf("------Test NMS Failed!------\n");
            exit(-1);
        }
        printf("------Test NMS Success!------\n");
    }
    return (void*)0;
}

int main(int argc, char* args[])
{
    struct timespec tp;
    clock_gettime(0, &tp);
    srand(tp.tv_nsec);

    int loop = 1;
    int thread_num = 1;
    int num = rand() % SCORE_RAND_LEN_MAX + 1;
    int i;
    int ret = 0;
    float nms_threshold = 0.7;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);

    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("uasge: %d\n", argc);
        printf("%s thread_num loop num nms_threshold\n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 2 1 10 0.7 \n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) num = atoi(args[3]);
    if (argc > 4) nms_threshold = (float)atof(args[4]);

    pthread_t pid[thread_num];
    cv_nms_thread_arg_t cv_nms_thread_arg[thread_num];
    for (int i = 0; i < thread_num; i++) {
        cv_nms_thread_arg[i].loop = loop;
        cv_nms_thread_arg[i].num = num;
        cv_nms_thread_arg[i].nms_threshold = nms_threshold;
        cv_nms_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_thread_nms, &cv_nms_thread_arg[i]) != 0) {
            printf("create thread failed\n");
            return -1;
        }
    }
    for (i = 0; i < thread_num; i++) {
        ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            printf("Thread join failed\n");
            exit(-1);
        }
    }
    bm_dev_free(handle);
    return ret;
}