

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

typedef struct{
    int loop_times;
    int real_data;
    char* input;
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

static int base64_enc_core(void *dst, void *src, char *box) {
    // convert 3x8bits to 4x6bits
    unsigned char *d, *s;
        d = (unsigned char*)dst;
        s = (unsigned char*)src;
    d[0] = box[(s[0] >> 2) & 0x3f];
    d[1] = box[((s[0] & 0x3) << 4) | (s[1] >> 4)];
    d[2] = box[((s[1] & 0xf) << 2) | (s[2] >> 6)];
    d[3] = box[(s[2] & 0x3f)];
    return 0;
}

static int base64_enc_left(void *dst, void *src, int size, char *box) {
    // convert 3x8bits to 4x6bits
    unsigned char *d, s[3];
    memset(s, 0x00, sizeof(s));
    memcpy(s, src, size);
        d = (unsigned char *)dst;
    d[0] = box[(s[0] >> 2) & 0x3f];
    d[1] = box[((s[0] & 0x3) << 4) | (s[1] >> 4)];
    d[2] = box[((s[1] & 0xf) << 2) | (s[2] >> 6)];
    d[3] = box[64];
    if ( size == 1 )
        d[2] = d[3];
    return 0;
}

static int base64_encode(void *dst, void *src, int size, char *box) {
    unsigned long block = size / 3;
    unsigned long left = size % 3;
        unsigned char *d = (unsigned char *)dst;
        unsigned char *s = (unsigned char *)src;
        unsigned i;
    for (i = 0; i < block; ++i) {
        base64_enc_core(d, s, box);
        d += 4;
        s += 3;
    }
    if (left)
        base64_enc_left(d, s, left, box);
    return 0;
}

static int base64_dec_core(void *dst, void *src, unsigned char *box) {
    unsigned char *d, *s;
    unsigned char t[4];
        d = (unsigned char *)dst;
        s = (unsigned char *)src;
    t[0] = box[s[0]];
    t[1] = box[s[1]];
    t[2] = box[s[2]];
    t[3] = box[s[3]];

    d[0] = (t[0] << 2) | (t[1] >> 4);
    d[1] = (t[1] << 4) | (t[2] >> 2);
    d[2] = (t[2] << 6) | (t[3]);

    return 0;
}

static int base64_box_t(unsigned char box_t[256], char *box) {
        int i;

        memset(box_t, 0x00, 256);
        for (i = 0; i < 64; ++i) {
            box_t[(unsigned int)box[i]] = i;
        }
        box_t[64] = 0;
        return 0;
}

int base64_decode(void *dst, void *src, unsigned long size, char *box) {
        unsigned char *d = (unsigned char *)dst;
        char *s = (char *)src;
    assert(size % 4 == 0);
    unsigned char box_t[256];
    base64_box_t(box_t, box);
    unsigned long block = size / 4;
    unsigned long i;
    for (i = 0; i < block; ++i) {
        base64_dec_core(d, s, box_t);
        d += 3;
        s += 4;
    }
    return 0;
}

/********************************************************/
/*len here is the number of unsigned long                                   */
/*b is the pointer of the result of the encoding, put it external just*/
/*for easier being made use of the decoder test                          */
/********************************************************/
int test_base64_enc_sys(int len, int dst_len, char *src, char *dst) {
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
    bmcv_base64_enc(handle, bm_mem_from_system(src), bm_mem_from_system(dst), lenth);
    gettimeofday(&t2, NULL);
    printf("bmcv excution time = %ld(us)\n", TIME_COST_US(t1, t2));

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

int test_base64_dec_sys(int len, int dst_len, char *src, char *dst) {
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
    bmcv_base64_dec(handle, bm_mem_from_system(src), bm_mem_from_system(dst), lenth);
    gettimeofday(&t2, NULL);
    printf("bmcv excution time = %ld(us)\n", TIME_COST_US(t1, t2));

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

int test_base64_enc_sys_dev(int len, int dst_len, char *src, char *dst) {
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
    bmcv_base64_enc(handle, bm_mem_from_system(src), dst_buf_device, lenth);
    gettimeofday(&t2, NULL);
    printf("bmcv excution time = %ld(us)\n", TIME_COST_US(t1, t2));
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

int test_base64_dec_sys_dev(int len, int dst_len, char *src, char *dst) {
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
    bmcv_base64_dec(handle, bm_mem_from_system(src), dst_buf_device, lenth);
    gettimeofday(&t2, NULL);
    printf("bmcv excution time = %ld(us)\n", TIME_COST_US(t1, t2));
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

int test_base64_enc_dev_sys(int len, int dst_len, char *src, char *dst) {
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
    bmcv_base64_enc(handle, src_buf_device, bm_mem_from_system(dst), lenth);
    gettimeofday(&t2, NULL);
    printf("bmcv excution time = %ld(us)\n", TIME_COST_US(t1, t2));

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

int test_base64_dec_dev_sys(int len, int dst_len, char *src, char *dst) {
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
    bmcv_base64_dec(handle, src_buf_device, bm_mem_from_system(dst), lenth);
    gettimeofday(&t2, NULL);
    printf("bmcv excution time = %ld(us)\n", TIME_COST_US(t1, t2));

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

int test_base64_enc_dev(int len, int dst_len, char *src, char *dst) {
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
    bmcv_base64_enc(handle, src_buf_device, dst_buf_device, lenth);
    gettimeofday(&t2, NULL);
    printf("bmcv excution time = %ld(us)\n", TIME_COST_US(t1, t2));
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

int test_base64_dec_dev(int len, int dst_len, char *src, char *dst) {
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
    bmcv_base64_dec(handle, src_buf_device, dst_buf_device, lenth);
    gettimeofday(&t2, NULL);
    printf("bmcv excution time = %ld(us)\n", TIME_COST_US(t1, t2));
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
    int loop_times = base64_thread->loop_times;
    int real_data = base64_thread->real_data;
    char* input = base64_thread->input;
    if (1 == real_data){
        for (int i = 0; i < loop_times; i++) {
            printf("test_loop_times = %d\n\n", i);
            int original_len = strlen(input);
            int encoded_len = (original_len + 2) / 3 * 4;
            char* a = (char *)malloc((original_len + 3) * sizeof(char));
            char* b = (char *)malloc((encoded_len + 3) * sizeof(char));
            for (int j = 0; j < original_len; j++){
                a[j] = input[j];
            }
            // input addr both are system
            ret |= test_base64_enc_sys(original_len, encoded_len,
                a, b);
            ret |= test_base64_dec_sys(encoded_len, original_len,
                b, a);
            // input addr is system; output is device
            ret |= test_base64_enc_sys_dev(original_len, encoded_len,
                a, b);
            ret |= test_base64_dec_sys_dev(encoded_len, original_len,
                b, a);
            // input is device; output is system
            ret |= test_base64_enc_dev_sys(original_len, encoded_len,
                a, b);
            ret |= test_base64_dec_dev_sys(encoded_len, original_len,
                b, a);
            // input addr both are device
            ret |= test_base64_enc_dev(original_len, encoded_len,
                a, b);
            ret |= test_base64_dec_dev(encoded_len, original_len,
                b, a);
            free(a);
            free(b);
        }
    } else{
        for (int i = 0; i < loop_times; i++) {
            printf("test_loop_times = %d\n\n", i);
            int original_len = (rand() % 134217728) + 1; //128M
            int encoded_len = (original_len + 2) / 3 * 4;
            char* a = (char *)malloc((original_len + 3) * sizeof(char));
            char* b = (char *)malloc((encoded_len + 3) * sizeof(char));
            for (int j = 0; j < original_len; j++){
                a[j] = (char)((rand() % 256) + 1);
            }

            // input addr both are system
            ret |= test_base64_enc_sys(original_len, encoded_len,
                a, b);
            ret |= test_base64_dec_sys(encoded_len, original_len,
                b, a);
            // input addr is system; output is device
            ret |= test_base64_enc_sys_dev(original_len, encoded_len,
                a, b);
            ret |= test_base64_dec_sys_dev(encoded_len, original_len,
                b, a);
            // input is device; output is system
            ret |= test_base64_enc_dev_sys(original_len, encoded_len,
                a, b);
            ret |= test_base64_dec_dev_sys(encoded_len, original_len,
                b, a);
            // input addr both are device
            ret |= test_base64_enc_dev(original_len, encoded_len,
                a, b);
            ret |= test_base64_dec_dev(encoded_len, original_len,
                b, a);
            free(a);
            free(b);
        }
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
    int test_threads_num = 1;
    int real_data = 0;
    char * input = NULL;

    if (argc == 2 && atoi(argv[1]) == -1) {
        printf("usage: %d\n", argc);
        printf("%s test_loop_times test_threads_num use_real_data(when use_real_data = 0, input = NULL) input(when use_real_data = 1,need to set input) \n", argv[0]);
        printf("example:\n");
        printf("%s \n", argv[0]);
        printf("%s 1 1\n", argv[0]);
        printf("%s 1 1 1 FHJNUHI \n", argv[0]);
        return 0;
    }
    if (argc > 1) test_loop_times = atoi(argv[1]);
    if (argc > 2) test_threads_num =  atoi(argv[2]);
    if (argc > 3) {real_data = atoi(argv[3]);
    if (real_data < 0 || real_data > 1) printf("real_data can  only be 0 or 1\n");};
    if (argc > 4) input = argv[4];

    if (test_loop_times > 1000000 || test_loop_times < 1) {
        printf("[TEST NMS] loop times should be 1~1000000");
        return -1;
    }
    if (test_threads_num > 4 || test_threads_num < 1) {
        printf("[TEST NMS] thread nums should be 1~4 ");
        return -1;
    }
    // test for multi-thread
    pthread_t pid[test_threads_num];
    base64_thread_arg_t base64_thread[test_threads_num];
    for (int i = 0; i < test_threads_num; i++) {
        base64_thread[i].loop_times = test_loop_times;
        base64_thread[i].real_data = real_data;
        base64_thread[i].input = input;
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
    printf("--------ALL THREADS TEST OVER---------");
    return 0;
}

