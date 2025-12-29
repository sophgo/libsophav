

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <math.h>
#ifdef __linux__
  #include <pthread.h>
  #include <sys/time.h>
#else
  #include <windows.h>
#endif
#include "bmcv_api_ext_c.h"

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

extern int base64_encode(void *dst, void *src, int size, char *box);
extern int base64_decode(void *dst, void *src, unsigned long size, char *box);

typedef struct{
    int loop_times;
    int real_data;
    char* input;
    int loopout_times;
} base64_thread_arg_t;

char const *base64std =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "+/=";

static int test_check_result_base(unsigned char *spacc, unsigned char *software, int len) {
// char may lead to problems here, have a look later
    int res = 0;
        int i;
    for (i = 0; i < len; ++i) {
        if (spacc[i] != software[i]) {
            printf(
        "%dth byte not match, golden is [%02x] but test is [%02x]\n"
                   , i, software[i], spacc[i]);
            res |= 1;
            break;
        }
    }
    printf("the test %s\n", res?"FAIL":"PASS");
    return res;
}


/********************************************************/
/*len here is the number of unsigned long                                   */
/*b is the pointer of the result of the encoding, put it external just*/
/*for easier being made use of the decoder test                          */
/********************************************************/
int test_base64_enc_sys(int len, int dst_len, char *src, char *dst, int loop) {
    bm_status_t ret;
    unsigned char* dst_soft;
    bm_handle_t handle;
    struct timeval t1, t2;
    unsigned long lenth[2];
    printf("---test--base64--enc--sys---\n");
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    lenth[0] = (unsigned long)len;
    gettimeofday(&t1, NULL);
    for(int i = 0; i < loop; i++)
        bmcv_base64_enc(handle, bm_mem_from_system(src), bm_mem_from_system(dst), lenth);
    gettimeofday(&t2, NULL);
    printf("bmcv excution inloop %d, avgtime = %ld(us)\n", loop, TIME_COST_US(t1, t2)/loop);

    dst_soft = (unsigned char*)malloc(dst_len + 3);
    gettimeofday(&t1, NULL);
    base64_encode(dst_soft, src, len, (char*)base64std);
    gettimeofday(&t2, NULL);
    printf("soft excution time = %ld(us)\n", TIME_COST_US(t1, t2));

    test_check_result_base((unsigned char *)dst,
        (unsigned char *)dst_soft, dst_len);
    free(dst_soft);
    bm_dev_free(handle);

    return 0;
}

int test_base64_dec_sys(int len, int dst_len, char *src, char *dst, int loop) {
    bm_status_t ret;
    unsigned char* dst_soft;
    bm_handle_t handle;
    struct timeval t1, t2;
    unsigned long lenth[2];
    printf("---test--base64--dec--sys---\n");

    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    lenth[0] = (unsigned long)len;
    gettimeofday(&t1, NULL);
    for(int i = 0; i < loop; i++)
        bmcv_base64_dec(handle, bm_mem_from_system(src), bm_mem_from_system(dst), lenth);
    gettimeofday(&t2, NULL);
    printf("bmcv excution inloop %d, avgtime = %ld(us)\n", loop, TIME_COST_US(t1, t2)/loop);

    dst_soft = (unsigned char*)malloc(dst_len + 3);
    gettimeofday(&t1, NULL);
    base64_decode(dst_soft, src, len, (char*)base64std);
    gettimeofday(&t2, NULL);
    printf("soft excution time = %ld(us)\n", TIME_COST_US(t1, t2));

    test_check_result_base((unsigned char *)dst,
        (unsigned char *)dst_soft, dst_len);
    free(dst_soft);
    bm_dev_free(handle);

    return 0;
}

int test_base64_enc_sys_dev(int len, int dst_len, char *src, char *dst, int loop) {
    bm_status_t ret;
    unsigned char* dst_soft;
    bm_handle_t handle;
    bm_device_mem_t dst_buf_device;
    struct timeval t1, t2;
    unsigned long lenth[2];
    printf("---test--base64--enc--sys--dev---\n");

    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    if(BM_SUCCESS != bm_malloc_device_byte(handle, &dst_buf_device, dst_len)) {
        printf("bm_malloc_device_byte error\r\n");
        return BM_ERR_NOMEM;
    }

    lenth[0] = (unsigned long)len;
    gettimeofday(&t1, NULL);
    for(int i = 0; i < loop; i++)
        bmcv_base64_enc(handle, bm_mem_from_system(src), dst_buf_device, lenth);
    gettimeofday(&t2, NULL);
    printf("bmcv excution inloop %d, avgtime = %ld(us)\n", loop, TIME_COST_US(t1, t2)/loop);
    if (BM_SUCCESS != bm_memcpy_d2s(handle, (void *)dst, dst_buf_device)) {
        printf("bm_memcpy_d2s error\r\n");
        return BM_ERR_NOMEM;
    }

    dst_soft = (unsigned char*)malloc(dst_len + 3);
    gettimeofday(&t1, NULL);
    base64_encode(dst_soft, src, len, (char*)base64std);
    gettimeofday(&t2, NULL);
    printf("soft excution time = %ld(us)\n", TIME_COST_US(t1, t2));

    test_check_result_base((unsigned char *)dst,
        (unsigned char *)dst_soft, dst_len);
    free(dst_soft);
    bm_free_device(handle, dst_buf_device);
    bm_dev_free(handle);

    return 0;
}

int test_base64_dec_sys_dev(int len, int dst_len, char *src, char *dst, int loop) {
    bm_status_t ret;
    unsigned char* dst_soft;
    bm_handle_t handle;
    bm_device_mem_t dst_buf_device;
    struct timeval t1, t2;
    unsigned long lenth[2];
    printf("---test--base64--dec--sys--dev---\n");

    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    if(BM_SUCCESS != bm_malloc_device_byte(handle, &dst_buf_device, dst_len)) {
        printf("bm_malloc_device_byte error\r\n");
        return BM_ERR_NOMEM;
    }

    lenth[0] = (unsigned long)len;
    gettimeofday(&t1, NULL);
    for(int i = 0; i < loop; i++)
        bmcv_base64_dec(handle, bm_mem_from_system(src), dst_buf_device, lenth);
    gettimeofday(&t2, NULL);
    printf("bmcv excution inloop %d, avgtime = %ld(us)\n", loop, TIME_COST_US(t1, t2)/loop);
    if (BM_SUCCESS != bm_memcpy_d2s(handle, (void *)dst, dst_buf_device)) {
        printf("bm_memcpy_d2s error\r\n");
        return BM_ERR_NOMEM;
    }

    dst_soft = (unsigned char*)malloc(dst_len + 3);
    gettimeofday(&t1, NULL);
    base64_decode(dst_soft, src, len, (char*)base64std);
    gettimeofday(&t2, NULL);
    printf("soft excution time = %ld(us)\n", TIME_COST_US(t1, t2));

    test_check_result_base((unsigned char *)dst,
        (unsigned char *)dst_soft, dst_len);
    free(dst_soft);
    bm_free_device(handle, dst_buf_device);
    bm_dev_free(handle);

    return 0;
}

int test_base64_enc_dev_sys(int len, int dst_len, char *src, char *dst, int loop) {
    bm_status_t ret;
    unsigned char* dst_soft;
    bm_handle_t handle;
    bm_device_mem_t src_buf_device;
    struct timeval t1, t2;
    unsigned long lenth[2];
    printf("---test--base64--enc--dev--sys---\n");
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    if(BM_SUCCESS != bm_malloc_device_byte(handle, &src_buf_device, len)) {
        printf("bm_malloc_device_byte error\r\n");
        return BM_ERR_NOMEM;
    }
    if(BM_SUCCESS != bm_memcpy_s2d(handle, src_buf_device, (void *)src)) {
        printf("bm_memcpy_s2d error\r\n");
        bm_free_device(handle, src_buf_device);
        return BM_ERR_NOMEM;
    }

    lenth[0] = (unsigned long)len;
    gettimeofday(&t1, NULL);
    for(int i = 0; i < loop; i++)
        bmcv_base64_enc(handle, src_buf_device, bm_mem_from_system(dst), lenth);
    gettimeofday(&t2, NULL);
    printf("bmcv excution inloop %d, avgtime = %ld(us)\n", loop, TIME_COST_US(t1, t2)/loop);

    dst_soft = (unsigned char*)malloc(dst_len + 3);
    gettimeofday(&t1, NULL);
    base64_encode(dst_soft, src, len, (char*)base64std);
    gettimeofday(&t2, NULL);
    printf("soft excution time = %ld(us)\n", TIME_COST_US(t1, t2));

    test_check_result_base((unsigned char *)dst,
        (unsigned char *)dst_soft, dst_len);
    free(dst_soft);
    bm_free_device(handle, src_buf_device);
    bm_dev_free(handle);

    return 0;
}

int test_base64_dec_dev_sys(int len, int dst_len, char *src, char *dst, int loop) {
    bm_status_t ret;
    unsigned char* dst_soft;
    bm_handle_t handle;
    bm_device_mem_t src_buf_device;
    struct timeval t1, t2;
    unsigned long lenth[2];
    printf("---test--base64--dec--dev--sys---\n");

    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    if(BM_SUCCESS != bm_malloc_device_byte(handle, &src_buf_device, len)) {
        printf("bm_malloc_device_byte error\r\n");
        return BM_ERR_NOMEM;
    }
    if(BM_SUCCESS != bm_memcpy_s2d(handle, src_buf_device, (void *)src)) {
        printf("bm_memcpy_s2d error\r\n");
        bm_free_device(handle, src_buf_device);
        return BM_ERR_NOMEM;
    }

    lenth[0] = (unsigned long)len;
    gettimeofday(&t1, NULL);
    for(int i = 0; i < loop; i++)
        bmcv_base64_dec(handle, src_buf_device, bm_mem_from_system(dst), lenth);
    gettimeofday(&t2, NULL);
    printf("bmcv excution inloop %d, avgtime = %ld(us)\n", loop, TIME_COST_US(t1, t2)/loop);

    dst_soft = (unsigned char*)malloc(dst_len + 3);
    gettimeofday(&t1, NULL);
    base64_decode(dst_soft, src, len, (char*)base64std);
    gettimeofday(&t2, NULL);
    printf("soft excution time = %ld(us)\n", TIME_COST_US(t1, t2));

    test_check_result_base((unsigned char *)dst,
        (unsigned char *)dst_soft, dst_len);
    free(dst_soft);
    bm_free_device(handle, src_buf_device);
    bm_dev_free(handle);

    return 0;
}

int test_base64_enc_dev(int len, int dst_len, char *src, char *dst, int loop) {
    bm_status_t ret;
    unsigned char* dst_soft;
    bm_device_mem_t src_buf_device;
    bm_device_mem_t dst_buf_device;
    bm_handle_t handle;
    struct timeval t1, t2;
    unsigned long lenth[2];
    printf("---test--base64--enc--dev---\n");

    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    if(BM_SUCCESS != bm_malloc_device_byte(handle, &src_buf_device, len)) {
        printf("bm_malloc_device_byte error\r\n");
        return BM_ERR_NOMEM;
    }
    if(BM_SUCCESS != bm_memcpy_s2d(handle, src_buf_device, (void *)src)) {
        printf("bm_memcpy_s2d error\r\n");
        bm_free_device(handle, src_buf_device);
        return BM_ERR_NOMEM;
    }
    if(BM_SUCCESS != bm_malloc_device_byte(handle, &dst_buf_device, dst_len)) {
        printf("bm_malloc_device_byte error\r\n");
        bm_free_device(handle, src_buf_device);
        return BM_ERR_NOMEM;
    }

    lenth[0] = (unsigned long)len;

    gettimeofday(&t1, NULL);
    for(int i = 0; i < loop; i++)
        bmcv_base64_enc(handle, src_buf_device, dst_buf_device, lenth);
    gettimeofday(&t2, NULL);
    printf("bmcv excution inloop %d, avgtime = %ld(us)\n", loop, TIME_COST_US(t1, t2)/loop);
    if (BM_SUCCESS != bm_memcpy_d2s(handle, (void *)dst, dst_buf_device)) {
        printf("bm_memcpy_d2s error\r\n");
        return BM_ERR_NOMEM;
    }

    dst_soft = (unsigned char*)malloc(dst_len + 3);
    gettimeofday(&t1, NULL);
    base64_encode(dst_soft, src, len, (char*)base64std);
    gettimeofday(&t2, NULL);
    printf("soft excution time = %ld(us)\n", TIME_COST_US(t1, t2));

    test_check_result_base((unsigned char *)dst,
        (unsigned char *)dst_soft, dst_len);
    free(dst_soft);
    bm_free_device(handle, src_buf_device);
    bm_free_device(handle, dst_buf_device);
    bm_dev_free(handle);

    return 0;
}

int test_base64_dec_dev(int len, int dst_len, char *src, char *dst, int loop) {
    bm_status_t ret;
    unsigned char* dst_soft;
    bm_handle_t handle;
    bm_device_mem_t src_buf_device;
    bm_device_mem_t dst_buf_device;
    unsigned long lenth[2];
    struct timeval t1, t2;
    printf("---test--base64--dec--dev---\n");

    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    if(BM_SUCCESS != bm_malloc_device_byte(handle, &src_buf_device, len)) {
        printf("bm_malloc_device_byte error\r\n");
        return BM_ERR_NOMEM;
    }
    if(BM_SUCCESS != bm_memcpy_s2d(handle, src_buf_device, (void *)src)) {
        printf("bm_memcpy_s2d error\r\n");
        bm_free_device(handle, src_buf_device);
        return BM_ERR_NOMEM;
    }
    if(BM_SUCCESS != bm_malloc_device_byte(handle, &dst_buf_device, dst_len)) {
        printf("bm_malloc_device_byte error\r\n");
        bm_free_device(handle, src_buf_device);
        return BM_ERR_NOMEM;
    }

    lenth[0] = (unsigned long)len;
    gettimeofday(&t1, NULL);
    for(int i = 0; i < loop; i++)
        bmcv_base64_dec(handle, src_buf_device, dst_buf_device, lenth);
    gettimeofday(&t2, NULL);
    printf("bmcv excution inloop %d, avgtime = %ld(us)\n", loop, TIME_COST_US(t1, t2)/loop);
    if (BM_SUCCESS != bm_memcpy_d2s(handle, (void *)dst, dst_buf_device)) {
        printf("bm_memcpy_d2s error\r\n");
        return BM_ERR_NOMEM;
    }

    dst_soft = (unsigned char*)malloc(dst_len + 3);
    gettimeofday(&t1, NULL);
    base64_decode(dst_soft, src, len, (char*)base64std);
    gettimeofday(&t2, NULL);
    printf("soft excution time = %ld(us)\n", TIME_COST_US(t1, t2));

    test_check_result_base((unsigned char *)dst,
        (unsigned char *)dst_soft, dst_len);
    free(dst_soft);
    bm_free_device(handle, src_buf_device);
    bm_free_device(handle, dst_buf_device);
    bm_dev_free(handle);

    return 0;
}

static void *test_base64_thread(void *args) {
    int ret = 0;
    base64_thread_arg_t* base64_thread = (base64_thread_arg_t*)args;
    int loop_times = base64_thread->loopout_times;
    int real_data = base64_thread->real_data;
    char* input = base64_thread->input;
    for (int i = 0; i < loop_times; i++) {
        int original_len, encoded_len;
        char *a, *b;
        printf("test_loopout_times = %d\n\n", i);
        if (1 == real_data){
            original_len = strlen(input);
            encoded_len = (original_len + 2) / 3 * 4;
            a = (char *)malloc((original_len + 3) * sizeof(char));
            b = (char *)malloc((encoded_len + 3) * sizeof(char));
            for (int j = 0; j < original_len; j++){
                a[j] = input[j];
            }
        } else {
            original_len = (rand() % 134217728) + 1; //128M
            encoded_len = (original_len + 2) / 3 * 4;
            a = (char *)malloc((original_len + 3) * sizeof(char));
            b = (char *)malloc((encoded_len + 3) * sizeof(char));
            for (int j = 0; j < original_len; j++){
                a[j] = (char)((rand() % 256) + 1);
            }
        }
        // input addr both are device
        ret |= test_base64_enc_dev(original_len, encoded_len,
            a, b, base64_thread->loop_times);
        ret |= test_base64_dec_dev(encoded_len, original_len,
            b, a, base64_thread->loop_times);
        // input addr is system; output is device
        ret |= test_base64_enc_sys_dev(original_len, encoded_len,
            a, b, base64_thread->loop_times);
        ret |= test_base64_dec_sys_dev(encoded_len, original_len,
            b, a, base64_thread->loop_times);
        // input is device; output is system
        ret |= test_base64_enc_dev_sys(original_len, encoded_len,
            a, b, base64_thread->loop_times);
        ret |= test_base64_dec_dev_sys(encoded_len, original_len,
            b, a, base64_thread->loop_times);
        // input addr both are system
        ret |= test_base64_enc_sys(original_len, encoded_len,
            a, b, base64_thread->loop_times);
        ret |= test_base64_dec_sys(encoded_len, original_len,
            b, a, base64_thread->loop_times);
        free(a);
        free(b);
    }
    if (ret == 0) {
        printf("all test pass of one thread\n");
    } else {
        printf("something not correct!\n");
    }

    return 0;
}

int main(int argc, char **argv) {
    int test_loop_times  = 1;
    int test_out_loop_times  = 1;
    int test_threads_num = 1;
    int real_data = 0;
    char * input = NULL;

    if (argc == 2 && atoi(argv[1]) == -1) {
        printf("usage: %d\n", argc);
        printf("%s test_loopin_times test_threads_num use_real_data(when use_real_data = 0, input = NULL) input(when use_real_data = 1,need to set input) loopout_times\n", argv[0]);
        printf("example:\n");
        printf("%s \n", argv[0]);
        printf("%s 1 1\n", argv[0]);
        printf("%s 1 1 1 FHJNUHI 1\n", argv[0]);
        return 0;
    }
    if (argc > 1) test_loop_times = atoi(argv[1]);
    if (argc > 2) test_threads_num =  atoi(argv[2]);
    if (argc > 3) {real_data = atoi(argv[3]);
    if (real_data < 0 || real_data > 1) printf("real_data can  only be 0 or 1\n");};
    if (argc > 4) input = argv[4];
    if (argc > 5) test_out_loop_times = atoi(argv[5]);

    if (test_loop_times > 1000000 || test_loop_times < 1) {
        printf("[TEST NMS] loop times should be 1~1000000");
        return -1;
    }
    if (test_threads_num > 4 || test_threads_num < 1) {
        printf("[TEST NMS] thread nums should be 1~4 ");
        return -1;
    }
    srand(time(NULL));
    // test for multi-thread
    pthread_t pid[test_threads_num];
    base64_thread_arg_t base64_thread[test_threads_num];
    for (int i = 0; i < test_threads_num; i++) {
        base64_thread[i].loop_times = test_loop_times;
        base64_thread[i].real_data = real_data;
        base64_thread[i].input = input;
        base64_thread[i].loopout_times = test_out_loop_times;
        if (pthread_create(
            &pid[i], NULL, test_base64_thread, &base64_thread[i])) {
            printf("create thread failed\n");
            return -1;
        }
    }
    int ret = 0;
    for (int i = 0; i < test_threads_num; i++) {
        ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            printf("Thread join failed");
            return -1;
        }
    }
    printf("--------ALL THREADS TEST OVER---------\n");
    return 0;
}

