
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __linux__
#include <sys/time.h>
#else
#include <windows.h>
#include "time.h"
#endif
#include <stdint.h>
#include "bmcv_api_ext_c.h"
#include "test_misc.h"
#include <assert.h>
#include <pthread.h>
typedef struct {
    int loop;
    int database_vecs_num;
    int query_vecs_num;
    int sort_cnt;
    int vec_dims;
    int slice_num;
    int is_sdc;
    int IP_metric;
    int input_dtype;
    int output_dtype;
    int show_result;
    bm_handle_t handle;
} faiss_indexPQ_thread_arg_t;

#define TEST_WITH_FAISS 0 //need g++ compiler

#if TEST_WITH_FAISS
#include <faiss/IndexPQ.h>
#include <faiss/IndexFlat.h>
#include <faiss/utils/distances.h>
using idx_t = faiss::Index::idx_t;
using MetricType = faiss::MetricType;

bm_status_t compare_result_with_faiss_ADC(
    int ny,
    int nx,
    int d,
    int m,
    int ksub,
    int dsub,
    int k,
    int IP_metric,
    bm_handle_t handle) {
    printf("database_vecs_num: %d\n", ny);
    printf("query_num:         %d\n", nx);
    printf("sort_count:        %d\n", k);
    printf("data_dims:         %d\n", d);
    printf("slice_num:         %d\n", m);
    printf("ksub :             %d\n", ksub);
    printf("dsub :             %d\n", dsub);
    printf("IP_metric:         %d\n", IP_metric);
    // input
    float *nxquery_input_sys = (float *)malloc(nx * d * sizeof(float));
    float *database_input_sys = (float *)malloc(ny * d * sizeof(float));
    float *centroids_input_sys = (float *)malloc(m * ksub * dsub * sizeof(float));
    unsigned char *nycodes_input_sys = (unsigned char *)malloc(ny * m * sizeof(unsigned char));

    // output
    float *fs_distance_output = (float *)malloc(nx * k * sizeof(float));
    idx_t *fs_index_output = (idx_t *)malloc(nx * k * sizeof(idx_t));
    float *distance_output_sys = (float *)malloc(nx * ny * sizeof(float));
    int *index_output_sys = (int *)malloc(nx * ny * sizeof(int));

    // fs index
    idx_t ntotal = ny;
    size_t nbits = 8;
    size_t M = m;
    size_t ks = ksub;
    size_t ds = dsub;

    //train faiss
    faiss::MetricType metric = IP_metric ? faiss::MetricType::METRIC_INNER_PRODUCT : faiss::MetricType::METRIC_L2;
    faiss::IndexPQ index(m * dsub, M, nbits, metric);
    index.pq.verbose = 1; // training log

    //set input_database
    for (int i = 0; i < ny; i++)
    {
        for (int j = 0; j < d; j++)
        {
            database_input_sys[i * d + j] = ((float)rand() / RAND_MAX) * 20.0f - 10.0f;
        }
    }

    index.train(ntotal, database_input_sys);
    index.reset();
    index.add(ntotal, database_input_sys);
    //set query
    for (int i = 0; i < nx; i++)
    {
        for (int j = 0; j < d; j++)
        {
            nxquery_input_sys[i * d + j] = ((float)rand() / RAND_MAX) * 20.0f - 10.0f;
        }
    }
    //set centroids
    for (size_t i = 0; i < M; i++)
    {
        for (size_t j = 0; j < ks; j++)
        {
            for (size_t k = 0; k < ds; k++)
            {
                centroids_input_sys[(i * ksub + j) * dsub + k] = index.pq.get_centroids(i, j)[k];
            }
        }
    }
    //set datacodes
    for (int i = 0; i < ny; i++)
    {
        for (int j = 0; j < m; j++)
        {
            nycodes_input_sys[i * m + j] = index.codes[i * m + j];
        }
    }
    //faiss search
    index.search(nx, nxquery_input_sys, k, fs_distance_output, fs_index_output);

    bm_device_mem_t centroids_input_dev;
    bm_device_mem_t nxquery_input_dev;
    bm_device_mem_t nycodes_input_dev;
    bm_device_mem_t distance_output_dev;
    bm_device_mem_t index_output_dev;

    int centroids_size = m * ksub * dsub * sizeof(float);
    int nxquery_size = nx * d * sizeof(float);
    int nycodes_size = ny * m * sizeof(char);
    int distance_size = nx * ny * sizeof(float);
    int index_size = nx * ny * sizeof(int);
    bm_malloc_device_byte(handle, &centroids_input_dev, centroids_size);
    bm_malloc_device_byte(handle, &nxquery_input_dev, nxquery_size);
    bm_malloc_device_byte(handle, &nycodes_input_dev, nycodes_size);
    bm_malloc_device_byte(handle, &distance_output_dev, distance_size);
    bm_malloc_device_byte(handle, &index_output_dev, index_size);

    bm_memcpy_s2d(handle, centroids_input_dev, centroids_input_sys);
    bm_memcpy_s2d(handle, nxquery_input_dev, nxquery_input_sys);
    bm_memcpy_s2d(handle, nycodes_input_dev, nycodes_input_sys);
    //tpu search
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
    bmcv_faiss_indexPQ_ADC(handle,
                           centroids_input_dev,
                           nxquery_input_dev,
                           nycodes_input_dev,
                           distance_output_dev,
                           index_output_dev,
                           d, m, ksub, ny, nx, k, IP_metric);
    gettimeofday(&t2, NULL);
    printf("TPU using time(us): %ld(us)\n", ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec));
    printf("TPU using time(ms): %ld(ms)\n", ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec) / 1000);

    bm_memcpy_d2s(handle, distance_output_sys, distance_output_dev);
    bm_memcpy_d2s(handle, index_output_sys, index_output_dev);

    printf("ADCsearch result:\n");
    for (int i = 0; i < k; i++)
    {
        printf("top:%d\n", i + 1);
        printf("tpu:   index:%d\t", index_output_sys[i]);
        printf("distance:%f\n", distance_output_sys[i]);
        printf("faiss: index:%d\t", fs_index_output[i]);
        printf("distance:%f\n", fs_distance_output[i]);
    }

    bm_free_device(handle, centroids_input_dev);
    bm_free_device(handle, nxquery_input_dev);
    bm_free_device(handle, nycodes_input_dev);
    bm_free_device(handle, distance_output_dev);
    bm_free_device(handle, index_output_dev);

    free(database_input_sys);
    free(centroids_input_sys);
    free(nxquery_input_sys);
    free(nycodes_input_sys);
    free(fs_distance_output);
    free(fs_index_output);
    free(distance_output_sys);
    free(index_output_sys);

    bm_dev_free(handle);
    return BM_SUCCESS;
}

bm_status_t compare_result_with_faiss_SDC(
    int ny,
    int nx,
    int d,
    int m,
    int ksub,
    int k,
    int IP_metric,
    bm_handle_t handle) {
    bm_status_t ret = BM_SUCCESS;
    // input
    float *query_input_sys = (float *)malloc(nx * d * sizeof(float));
    float *database_input_sys = (float *)malloc(ny * d * sizeof(float));
    float *sdc_table_input_sys = (float *)malloc(m * ksub * ksub * sizeof(float));
    unsigned char *nxcodes_input_sys = (unsigned char *)malloc(nx * m * sizeof(unsigned char));
    unsigned char *nycodes_input_sys = (unsigned char *)malloc(ny * m * sizeof(unsigned char));

    // output
    uint8_t *fs_query_codes = (uint8_t *)malloc(nx * m * sizeof(uint8_t));
    float *fs_distance_output = (float *)malloc(nx * k * sizeof(float));
    idx_t *fs_index_output = (idx_t *)malloc(nx * k * sizeof(idx_t));
    float *distance_output_sys = (float *)malloc(nx * ny * sizeof(float));
    int *index_output_sys = (int *)malloc(nx * ny * sizeof(int));

    // fs index
    idx_t ntotal = ny;
    size_t nbits = 8;
    size_t M = m;
    size_t ks = ksub;
    //train faiss
    faiss::MetricType metric = IP_metric ? faiss::MetricType::METRIC_INNER_PRODUCT : faiss::MetricType::METRIC_L2;
    faiss::IndexPQ index(d, M, nbits, metric);
    index.pq.verbose = 1; // training log

    //set input
    for (int i = 0; i < ny; i++)
    {
        for (int j = 0; j < d; j++)
        {
            database_input_sys[i * d + j] = ((float)rand() / RAND_MAX) * 20.0f - 10.0f;
        }
    }
    //set database
    index.train(ntotal, database_input_sys);
    index.reset();
    index.add(ntotal, database_input_sys);

    index.pq.compute_codes(query_input_sys, fs_query_codes, nx);
    index.pq.compute_sdc_table();

    for (int i = 0; i < nx; i++)
    {
        for (int j = 0; j < d; j++)
        {
            query_input_sys[i * d + j] = ((float)rand() / RAND_MAX) * 20.0f - 10.0f;
        }
    }
    //set query
    for (size_t i = 0; i < M; i++)
    {
        for (size_t j = 0; j < ks; j++)
        {
            for (size_t k = 0; k < ks; k++)
            {
                sdc_table_input_sys[(i * ksub + j) * ksub + k] = index.pq.sdc_table[(i * ksub + j) * ksub + k];
            }
        }
    }
    //set sdc_table
    for (int i = 0; i < ny; i++)
    {
        for (int j = 0; j < m; j++)
        {
            nycodes_input_sys[i * m + j] = index.codes[i * m + j];
        }
    }
    //set datacodes
    //faiss search
    index.search(nx, query_input_sys, k, fs_distance_output, fs_index_output);

    bm_device_mem_t sdc_table_input_dev;
    bm_device_mem_t nxcodes_input_dev;
    bm_device_mem_t nycodes_input_dev;
    bm_device_mem_t distance_output_dev;
    bm_device_mem_t index_output_dev;

    int sdc_table_size = m * ksub * ksub * sizeof(float);
    int nxcodes_size = nx * m * sizeof(char);
    int nycodes_size = ny * m * sizeof(char);
    int output_distance_size = nx * ny * sizeof(float);
    int output_index_size = nx * ny * sizeof(int);
    bm_malloc_device_byte(handle, &sdc_table_input_dev, sdc_table_size);
    bm_malloc_device_byte(handle, &nxcodes_input_dev, nxcodes_size);
    bm_malloc_device_byte(handle, &nycodes_input_dev, nycodes_size);
    bm_malloc_device_byte(handle, &distance_output_dev, output_distance_size);
    bm_malloc_device_byte(handle, &index_output_dev, output_index_size);

    bm_memcpy_s2d(handle, sdc_table_input_dev, sdc_table_input_sys);
    bm_memcpy_s2d(handle, nxcodes_input_dev, nxcodes_input_sys);
    bm_memcpy_s2d(handle, nycodes_input_dev, nycodes_input_sys);

    //tpu search
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
    bmcv_faiss_indexPQ_SDC(handle,
                           sdc_table_input_dev,
                           nxcodes_input_dev,
                           nycodes_input_dev,
                           distance_output_dev,
                           index_output_dev,
                           m, ksub, ny, nx, k, IP_metric);
    gettimeofday(&t2, NULL);
    printf("TPU using time(us): %ld(us)\n", ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec));
    printf("TPU using time(ms): %ld(ms)\n", ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec) / 1000);

    bm_memcpy_d2s(handle, distance_output_sys, distance_output_dev);
    bm_memcpy_d2s(handle, index_output_sys, index_output_dev);

    printf("SDCsearch result:\n");
    for (int i = 0; i < k; i++)
    {
        printf("top:%d\n", i + 1);
        printf("tpu:   index:%d\t", index_output_sys[i]);
        printf("distance:%f\n", distance_output_sys[i]);
        printf("faiss: index:%d\t", fs_index_output[i]);
        printf("distance:%f\n", fs_distance_output[i]);
    }

    bm_free_device(handle, sdc_table_input_dev);
    bm_free_device(handle, nxcodes_input_dev);
    bm_free_device(handle, nycodes_input_dev);
    bm_free_device(handle, distance_output_dev);
    bm_free_device(handle, index_output_dev);

    free(query_input_sys);
    free(database_input_sys);
    free(sdc_table_input_sys);
    free(nxcodes_input_sys);
    free(nycodes_input_sys);

    free(fs_query_codes);
    free(fs_distance_output);
    free(fs_index_output);
    free(distance_output_sys);
    free(index_output_sys);

    bm_dev_free(handle);
    return BM_SUCCESS;
}
#endif

bm_status_t faiss_indexPQ_ADC_test(
    int ny,
    int nx,
    int d,
    int m,
    int ks,
    int ds,
    int k,
    int IP_metric,
    int show_result,
    int input_dtype,
    int output_dtype,
    bm_handle_t handle) {
    printf("database_vecs_num: %d\n", ny);
    printf("query_num:         %d\n", nx);
    printf("sort_count:        %d\n", k);
    printf("data_dims:         %d\n", d);
    printf("slice_num:         %d\n", m);
    printf("dsub =             %d\n", ds);
    printf("ksub =             %d\n", ks);
    printf("IP_metric:         %d\n", IP_metric);
    printf("input_dtype:       %d\n", input_dtype);
    printf("output_dtype:      %d\n", output_dtype);
    printf("show_result:       %d\n", show_result);
    bm_status_t ret;
    int round  = 1;
    fp16 *centroids_input_sys_fp16 = (fp16 *)malloc(m * ks * ds * sizeof(fp16));
    fp16 *nxquery_input_sys_fp16 = (fp16 *)malloc(nx * d * sizeof(fp16));
    float *centroids_input_sys_fp32 = (float *)malloc(m * ks * ds * sizeof(float));
    float *nxquery_input_sys_fp32 = (float *)malloc(nx * d * sizeof(float));

    unsigned char *nycodes_input_sys = (unsigned char *)malloc(ny * m * sizeof(unsigned char));
    unsigned char *distance_output_sys = (unsigned char *)malloc(nx * ny * dtype_size((enum bm_data_type_t)output_dtype));
    int *index_output_sys = (int *)malloc(nx * ny * sizeof(int));

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < ks; j++) {
            for (int k = 0; k < ds; k++) {
                centroids_input_sys_fp32[i * ds * ks + j * ds + k] = ((float)rand() / RAND_MAX) * 20.0f;
                centroids_input_sys_fp16[i * ds * ks + j * ds + k] = fp32tofp16(centroids_input_sys_fp32[i * ds * ks + j * ds + k], round);
            }
        }
    }
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < d; j++) {
            nxquery_input_sys_fp32[i * d + j] = ((float)rand() / RAND_MAX) * 20.0f - 10.0f;
            nxquery_input_sys_fp16[i * d + j] = fp32tofp16(nxquery_input_sys_fp32[i * d + j], round);
        }
    }
    for (int i = 0; i < ny; i++) {
        for (int j = 0; j < m; j++) {
            nycodes_input_sys[i * m + j] = ((float)rand() / RAND_MAX) * 20.0f - 10.0f;
        }
    }

    bm_device_mem_t centroids_input_dev;
    bm_device_mem_t nxquery_input_dev;
    bm_device_mem_t nycodes_input_dev;
    bm_device_mem_t distance_output_dev;
    bm_device_mem_t index_output_dev;
    int centroids_size = m * ks * ds * dtype_size((enum bm_data_type_t)input_dtype);
    int nxquery_size = nx * d * dtype_size((enum bm_data_type_t)input_dtype);

    int nycodes_size = ny * m * sizeof(char);
    int distance_size = nx * ny * dtype_size((enum bm_data_type_t)output_dtype);
    int index_size = nx * ny * sizeof(int);
    bm_malloc_device_byte(handle, &centroids_input_dev, centroids_size);
    bm_malloc_device_byte(handle, &nxquery_input_dev, nxquery_size);
    bm_malloc_device_byte(handle, &nycodes_input_dev, nycodes_size);
    bm_malloc_device_byte(handle, &distance_output_dev, distance_size);
    bm_malloc_device_byte(handle, &index_output_dev, index_size);

    if (input_dtype == DT_FP16) {
        bm_memcpy_s2d(handle, centroids_input_dev, bm_mem_get_system_addr(bm_mem_from_system(centroids_input_sys_fp16)));
        bm_memcpy_s2d(handle, nxquery_input_dev, bm_mem_get_system_addr(bm_mem_from_system(nxquery_input_sys_fp16)));
    } else {
        bm_memcpy_s2d(handle, centroids_input_dev, bm_mem_get_system_addr(bm_mem_from_system(centroids_input_sys_fp32)));
        bm_memcpy_s2d(handle, nxquery_input_dev, bm_mem_get_system_addr(bm_mem_from_system(nxquery_input_sys_fp32)));
    }
    bm_memcpy_s2d(handle, nycodes_input_dev, bm_mem_get_system_addr(bm_mem_from_system(nycodes_input_sys)));

    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
    ret = bmcv_faiss_indexPQ_ADC_ext(handle,
                           centroids_input_dev,
                           nxquery_input_dev,
                           nycodes_input_dev,
                           distance_output_dev,
                           index_output_dev,
                           d, m, ks, ny, nx, k, IP_metric, input_dtype, output_dtype);
    gettimeofday(&t2, NULL);
    printf("TPU using time:%ldus\n", ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec));
    printf("TPU using time:%ldms\n", ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec) / 1000);

    if(ret != BM_SUCCESS) {
        bm_free_device(handle, centroids_input_dev);
        bm_free_device(handle, nxquery_input_dev);
        bm_free_device(handle, nycodes_input_dev);
        bm_free_device(handle, distance_output_dev);
        bm_free_device(handle, index_output_dev);
        free(centroids_input_sys_fp32);
        free(centroids_input_sys_fp16);
        free(nxquery_input_sys_fp32);
        free(nxquery_input_sys_fp16);
        free(nycodes_input_sys);
        free(distance_output_sys);
        free(index_output_sys);
        return ret;
    }
    bm_memcpy_d2s(handle, distance_output_sys, distance_output_dev);
    bm_memcpy_d2s(handle, index_output_sys, index_output_dev);

    if (show_result) {
        printf("ADCsearch result:\n");
        for (int i = 0; i < k; i++) {
            printf("top: %d\n", i + 1);
            printf("index: %d\t", index_output_sys[i]);
            if (output_dtype == DT_FP16) {
                printf("distance: %f\n", fp16tofp32(((fp16*)distance_output_sys)[i]));
            } else {
                printf("distance: %f\n", ((float*)distance_output_sys)[i]);
            }
        }
    }

    bm_free_device(handle, centroids_input_dev);
    bm_free_device(handle, nxquery_input_dev);
    bm_free_device(handle, nycodes_input_dev);
    bm_free_device(handle, distance_output_dev);
    bm_free_device(handle, index_output_dev);
    free(centroids_input_sys_fp32);
    free(centroids_input_sys_fp16);
    free(nxquery_input_sys_fp32);
    free(nxquery_input_sys_fp16);
    free(nycodes_input_sys);
    free(distance_output_sys);
    free(index_output_sys);
    return ret;
}

bm_status_t faiss_indexPQ_SDC_test(
    int ny,
    int nx,
    int m,
    int ks,
    int k,
    int IP_metric,
    int show_result,
    int input_dtype,
    int output_dtype,
    bm_handle_t handle) {
    printf("database_vecs_num: %d\n", ny);
    printf("query_num:         %d\n", nx);
    printf("sort_count:        %d\n", k);
    printf("slice_num:         %d\n", m);
    printf("IP_metric:         %d\n", IP_metric);
    printf("input_dtype:       %d\n", input_dtype);
    printf("output_dtype:      %d\n", output_dtype);
    printf("show_result:       %d\n", show_result);
    bm_status_t ret;
    fp16 *sdc_table_input_sys_fp16 = (fp16 *)malloc(m * ks * ks * sizeof(fp16));
    float *sdc_table_input_sys_fp32 = (float *)malloc(m * ks * ks * sizeof(float));
    unsigned char *nxcodes_input_sys = (unsigned char *)malloc(nx * m * sizeof(unsigned char));
    unsigned char *nycodes_input_sys = (unsigned char *)malloc(ny * m * sizeof(unsigned char));
    unsigned char *distance_output_sys = (unsigned char *)malloc(nx * ny * dtype_size((enum bm_data_type_t)output_dtype));
    int *index_output_sys = (int *)malloc(nx * ny * sizeof(int));

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < ks; j++) {
            for (int k = 0; k < ks; k++) {
                if (k > j) {
                    sdc_table_input_sys_fp32[i * ks * ks + j * ks + k] = (float)rand() / RAND_MAX * 20;
                    sdc_table_input_sys_fp16[i * ks * ks + j * ks + k] = fp32tofp16(sdc_table_input_sys_fp32[i * ks * ks + j * ks + k], 1);
                } else if (k < j) {
                    sdc_table_input_sys_fp32[i * ks * ks + j * ks + k] = sdc_table_input_sys_fp32[i * ks * ks + k * ks + j];
                    sdc_table_input_sys_fp16[i * ks * ks + j * ks + k] = sdc_table_input_sys_fp16[i * ks * ks + k * ks + j];
                } else {
                    sdc_table_input_sys_fp32[i * ks * ks + j * ks + k] = 0;
                    sdc_table_input_sys_fp16[i * ks * ks + j * ks + k] = fp32tofp16(0, 1);
                }
            }
        }
    }

    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < m; j++) {
            nxcodes_input_sys[i * m + j] = rand() % 256;
        }
    }

    for (int i = 0; i < ny; i++) {
        for (int j = 0; j < m; j++) {
            nycodes_input_sys[i * m + j] = rand() % 256;
        }
    }

    int sdc_table_size = m * ks * ks * dtype_size((enum bm_data_type_t)input_dtype);

    int nxcodes_size = nx * m * sizeof(char);
    int nycodes_size = ny * m * sizeof(char);
    int output_distance_size = nx * ny * dtype_size((enum bm_data_type_t)output_dtype);
    int output_index_size = nx * ny * dtype_size((enum bm_data_type_t)output_dtype);

    bm_device_mem_t sdc_table_input_dev;
    bm_device_mem_t nxcodes_input_dev;
    bm_device_mem_t nycodes_input_dev;
    bm_device_mem_t distance_output_dev;
    bm_device_mem_t index_output_dev;
    bm_malloc_device_byte(handle, &sdc_table_input_dev, sdc_table_size);
    bm_malloc_device_byte(handle, &nxcodes_input_dev, nxcodes_size);
    bm_malloc_device_byte(handle, &nycodes_input_dev, nycodes_size);
    bm_malloc_device_byte(handle, &distance_output_dev, output_distance_size);
    bm_malloc_device_byte(handle, &index_output_dev, output_index_size);
    if (input_dtype == DT_FP16) {
        bm_memcpy_s2d(handle, sdc_table_input_dev, sdc_table_input_sys_fp16);
    } else {
        bm_memcpy_s2d(handle, sdc_table_input_dev, sdc_table_input_sys_fp32);
    }
    bm_memcpy_s2d(handle, nxcodes_input_dev, nxcodes_input_sys);
    bm_memcpy_s2d(handle, nycodes_input_dev, nycodes_input_sys);

    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
    ret = bmcv_faiss_indexPQ_SDC_ext(handle,
                           sdc_table_input_dev,
                           nxcodes_input_dev,
                           nycodes_input_dev,
                           distance_output_dev,
                           index_output_dev,
                           m, ks, ny, nx, k, IP_metric, input_dtype, output_dtype);
    gettimeofday(&t2, NULL);
    printf("TPU using time:%ldus\n", ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec));
    printf("TPU using time:%ldms\n", ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec) / 1000);

    if(ret != BM_SUCCESS){
        bm_free_device(handle, sdc_table_input_dev);
        bm_free_device(handle, nxcodes_input_dev);
        bm_free_device(handle, nycodes_input_dev);
        bm_free_device(handle, distance_output_dev);
        bm_free_device(handle, index_output_dev);
        free(sdc_table_input_sys_fp16);
        free(sdc_table_input_sys_fp32);
        free(nxcodes_input_sys);
        free(nycodes_input_sys);
        free(distance_output_sys);
        free(index_output_sys);
        return ret;
    }

    bm_memcpy_d2s(handle, distance_output_sys, distance_output_dev);
    bm_memcpy_d2s(handle, index_output_sys, index_output_dev);

    if (show_result) {
        printf("SDCsearch result:\n");
        for (int i = 0; i < k; i++) {
            printf("top: %d\n", i + 1);
            printf("index: %d\t", ((int*)index_output_sys)[i]);
            if (output_dtype == DT_FP16) {
                printf("distance: %f", fp16tofp32(((fp16*)distance_output_sys)[i]));
            } else {
                printf("distance: %f", ((float*)distance_output_sys)[i]);
            }
            printf("\n");
        }
    }

    bm_free_device(handle, sdc_table_input_dev);
    bm_free_device(handle, nxcodes_input_dev);
    bm_free_device(handle, nycodes_input_dev);
    bm_free_device(handle, distance_output_dev);
    bm_free_device(handle, index_output_dev);
    free(sdc_table_input_sys_fp16);
    free(sdc_table_input_sys_fp32);
    free(nxcodes_input_sys);
    free(nycodes_input_sys);
    free(distance_output_sys);
    free(index_output_sys);
    return ret;
}

int param_check(int database_vecs_num, int query_vecs_num, int sort_cnt, int vec_dims){
    if(sort_cnt > database_vecs_num) {
        printf("sort_cnt cannot be greater than b!\n");
        return -1;
    }
    if(query_vecs_num > database_vecs_num) {
        printf("query_vecs_num cannot be greater than b!\n");
        return -1;
    }
    if(vec_dims > 512) {
        printf("vec_dims cannot be greater than 512!\n");
        return -1;
    }
    return 0;
}

void* test_faiss_indexPQ(void* args) {
    faiss_indexPQ_thread_arg_t* faiss_indexPQ_thread_arg = (faiss_indexPQ_thread_arg_t*)args;
    int loop = faiss_indexPQ_thread_arg->loop;
    int database_vecs_num = faiss_indexPQ_thread_arg->database_vecs_num;
    int query_vecs_num = faiss_indexPQ_thread_arg->query_vecs_num;
    int sort_cnt = faiss_indexPQ_thread_arg->sort_cnt;
    int vec_dims = faiss_indexPQ_thread_arg->vec_dims;
    int slice_num = faiss_indexPQ_thread_arg->slice_num;
    int is_sdc = faiss_indexPQ_thread_arg->is_sdc;
    int IP_metric = faiss_indexPQ_thread_arg->IP_metric;
    int input_dtype = faiss_indexPQ_thread_arg->input_dtype;
    int output_dtype = faiss_indexPQ_thread_arg->output_dtype;
    int show_result = faiss_indexPQ_thread_arg->show_result;
    bm_handle_t handle = faiss_indexPQ_thread_arg->handle;
    int nbits = 8;
    int ksub = 1 << nbits;
    for (int i = 0; i < loop; i++) {
        if(loop > 1) {
            sort_cnt = rand() % 50 + 1;
            database_vecs_num = rand() % 10000 + 1000 + sort_cnt;
            int input_dtype_num[] = {3, 5};  //3-fp16 5-fp32
            int output_dtype_num[] = {3, 5};  //3-fp16 5-fp32
            int dtype_size = sizeof(input_dtype_num) / sizeof(input_dtype_num[0]);
            int rand_num = rand() % dtype_size;
            input_dtype = input_dtype_num[rand_num];
            output_dtype = output_dtype_num[rand_num];
        }
        #if TEST_WITH_FAISS
        if(is_sdc) {
            if (BM_SUCCESS != compare_result_with_faiss_SDC(database_vecs_num, query_vecs_num, vec_dims, slice_num,
                              ksub, sort_cnt, IP_metric, handle)){
                printf("compare_result_with_faiss_SDC failed! \n");
                exit(-1);
            } else {
                printf("compare_result_with_faiss_SDC successful! \n");
            }
        } else {
            if (BM_SUCCESS != compare_result_with_faiss_ADC(database_vecs_num, query_vecs_num, vec_dims, slice_num,
                              ksub, vec_dims / slice_num, sort_cnt, IP_metric, handle)) {
                printf("compare_result_with_faiss_ADC failed! \n");
                exit(-1);
            } else {
                printf("compare_result_with_faiss_ADC successful! \n");
            }
        }
        #endif
        if (is_sdc) {
            if (BM_SUCCESS != faiss_indexPQ_SDC_test(database_vecs_num, query_vecs_num, slice_num, ksub, sort_cnt, IP_metric, show_result, input_dtype, output_dtype, handle)) {
                printf("test faiss_indexPQ_SDC failed! \n");
                exit(-1);
            } else {
                printf("test faiss_indexPQ_SDC successful! \n");
            }
        } else {
            if (BM_SUCCESS != faiss_indexPQ_ADC_test(database_vecs_num, query_vecs_num, vec_dims, slice_num, ksub, vec_dims / slice_num, sort_cnt, IP_metric, show_result, input_dtype, output_dtype, handle)) {
                printf("test faiss_indexPQ_ADC failed!\n");
                exit(-1);
            } else {
                printf("test faiss_indexPQ_ADC successful! \n");
            }
        }
    }
    return NULL;
}

int main(int argc, char *args[]) {
    struct timespec tp;
    clock_gettime(0, &tp);
    unsigned int seed = tp.tv_nsec;
    srand(seed);
    printf("random seed = %u\n", seed);
    int thread_num = 1;
    int loop = 1;
    int sort_cnt = 10;
    int database_vecs_num = rand() % 10000 + 1000 + sort_cnt;
    int query_vecs_num = 1;
    int vec_dims = 256;
    int slice_num = 32;
    int is_sdc = 0;
    int IP_metric = 1;
    int show_result = 0;
    int input_dtype = 5;
    int output_dtype = 5;

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: %d\n", argc);
        printf("%s thread_num loop_num database_num query_num sort_count data_dims slice_num is_sdc IP_metric input_dtype output_dtype show_result\n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) database_vecs_num = atoi(args[3]);
    if (argc > 4) query_vecs_num = atoi(args[4]);
    if (argc > 5) sort_cnt = atoi(args[5]);
    if (argc > 6) vec_dims = atoi(args[6]);
    if (argc > 7) slice_num = atoi(args[7]);
    if (argc > 8) is_sdc = atoi(args[8]);
    if (argc > 9) IP_metric = atoi(args[9]);
    if (argc > 10) input_dtype = atoi(args[10]);
    if (argc > 11) output_dtype = atoi(args[11]);
    if (argc > 12) show_result = atoi(args[12]);

    printf("thread_num:        %d\n", thread_num);
    printf("loop:              %d\n", loop);
    int ret = 0;
    ret = param_check(database_vecs_num, query_vecs_num, sort_cnt, vec_dims);
    if(ret != 0) {
        return -1;
    }
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (BM_SUCCESS != ret) {
        printf("request dev failed\n");
        return BM_ERR_FAILURE;
    }
    // test for multi-thread
    pthread_t pid[thread_num];
    faiss_indexPQ_thread_arg_t faiss_indexPQ_thread_arg[thread_num];
    for (int i = 0; i < thread_num; i++) {
        faiss_indexPQ_thread_arg[i].loop = loop;
        faiss_indexPQ_thread_arg[i].database_vecs_num = database_vecs_num;
        faiss_indexPQ_thread_arg[i].query_vecs_num = query_vecs_num;
        faiss_indexPQ_thread_arg[i].sort_cnt = sort_cnt;
        faiss_indexPQ_thread_arg[i].vec_dims = vec_dims;
        faiss_indexPQ_thread_arg[i].slice_num = slice_num;
        faiss_indexPQ_thread_arg[i].is_sdc = is_sdc;
        faiss_indexPQ_thread_arg[i].IP_metric = IP_metric;
        faiss_indexPQ_thread_arg[i].input_dtype = input_dtype;
        faiss_indexPQ_thread_arg[i].output_dtype = output_dtype;
        faiss_indexPQ_thread_arg[i].show_result = show_result;
        faiss_indexPQ_thread_arg[i].handle = handle;
        if (pthread_create(pid + i, NULL, test_faiss_indexPQ, faiss_indexPQ_thread_arg + i) != 0) {
            printf("create thread failed\n");
            return -1;
        }
    }
    for (int i = 0; i < thread_num; i++) {
        int ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            printf("Thread join failed\n");
            exit(-1);
        }
    }
    bm_dev_free(handle);
    return 0;
}
