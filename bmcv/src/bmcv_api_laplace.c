#include "bmcv_common.h"
#include "bmcv_internal.h"

#define KERNEL_LENGTH 9

static bm_status_t bmcv_check(bm_handle_t handle, bm_image input, bm_image output)
{
    if (handle == NULL) {
        bmlib_log("LAPLACIAN", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_FAILURE;
    }

    if (input.image_format != FORMAT_GRAY ||
        output.image_format != FORMAT_GRAY) {
        bmlib_log("LAPLACIAN", BMLIB_LOG_ERROR, "image format NOT supported!\r\n");
        return BM_NOT_SUPPORTED;
    }

    if (input.data_type != DATA_TYPE_EXT_1N_BYTE &&
        input.data_type != DATA_TYPE_EXT_1N_BYTE_SIGNED) {
        bmlib_log("LAPLACIAN", BMLIB_LOG_ERROR, "data type NOT supported!\r\n");
        return BM_NOT_SUPPORTED;
    }

    if (!bm_image_is_attached(input)) {
        bmlib_log("LAPLACIAN", BMLIB_LOG_ERROR, "input not malloc device memory!\r\n");
        return BM_ERR_PARAM;
    }

    if (!bm_image_is_attached(output)) {
        bmlib_log("LAPLACIAN", BMLIB_LOG_ERROR, "output not malloc device memory!\r\n");
        return BM_ERR_PARAM;
    }

    if (output.image_format != input.image_format ||
        output.data_type != input.data_type) {
        bmlib_log("LAPLACIAN", BMLIB_LOG_ERROR, "output and input NOT consistant!\r\n");
        return BM_ERR_PARAM;
    }

    return BM_SUCCESS;
}

bm_status_t bmcv_laplacian_check(int ksize, bm_image input, bm_image output)
{
    if (ksize !=1 && ksize != 3) {
        bmlib_log("LAPLACIAN", BMLIB_LOG_ERROR, "kernel ksize format NOT supported!\r\n");
        return BM_ERR_PARAM;
    }
    if (input.height > 4096 || input.width > 4096) {
        bmlib_log("LAPLACIAN", BMLIB_LOG_ERROR, " Unsupported size : size_max = 4096 x 4096\r\n");
        return BM_ERR_PARAM;
    }
    if (input.height < 2 || input.width < 2) {
        bmlib_log("LAPLACIAN", BMLIB_LOG_ERROR, " Unsupported size : size_min = 2 x 2\r\n");
        return BM_ERR_PARAM;
    }

    return BM_SUCCESS;

}

bm_status_t bmcv_image_laplacian(bm_handle_t handle, bm_image input, bm_image output, unsigned int ksize)
{
    bm_status_t ret = BM_SUCCESS;
    int8_t kernel[KERNEL_LENGTH] = { 2, 0, 2, 0, -8, 0, 2, 0, 2};
    bm_device_mem_t kernel_mem;
    bm_device_mem_t in_dev_mem[3], out_dev_mem[3];
    int channel;
    int i;
    int stride_i, stride_o;
    unsigned int chipid;
    int core_id = 0;
    bm_api_laplacian_t api;

    ret = bmcv_check(handle, input, output);
    if (ret != BM_SUCCESS) {
        printf("bm para check failed!\n");
        return ret;
    }

    ret = bmcv_laplacian_check(ksize, input, output);
    if (ret != BM_SUCCESS) {
        printf("bm laplacian para check failed!\n");
        return ret;
    }

    if (ksize == 1) {
        int8_t kernel_tmp[KERNEL_LENGTH] = {0, 1, 0, 1, -4, 1, 0, 1, 0};
        memcpy(kernel, kernel_tmp, sizeof(int8_t) * KERNEL_LENGTH);
    }

    ret = bm_malloc_device_byte(handle, &kernel_mem, KERNEL_LENGTH * sizeof(int));
    if (ret != BM_SUCCESS) {
        printf("bm malloc device mem failed!\n");
        return ret;
    }

    ret = bm_memcpy_s2d(handle, kernel_mem, kernel);
    if (ret != BM_SUCCESS){
        bm_free_device(handle, kernel_mem);
        return ret;
    }

    memset(&api, 0, sizeof(bm_api_laplacian_t));

    channel = bm_image_get_plane_num(input);

    ret = bm_image_get_stride(input, &stride_i);
    if (ret != BM_SUCCESS){
        printf("bm image get stride failed\n");
        bm_free_device(handle, kernel_mem);
        return ret;
    }
    ret = bm_image_get_stride(output, &stride_o);
    if (ret != BM_SUCCESS){
        printf("bm image get stride failed\n");
        bm_free_device(handle, kernel_mem);
        return ret;
    }
    ret = bm_image_get_device_mem(input, in_dev_mem);
    if (ret != BM_SUCCESS){
        printf("bm image get device mem failed\n");
        bm_free_device(handle, kernel_mem);
        return ret;
    }
    ret = bm_image_get_device_mem(output, out_dev_mem);
    if (ret != BM_SUCCESS){
        printf("bm image get device mem failed\n");
        bm_free_device(handle, kernel_mem);
        return ret;
    }
    for (i = 0; i < channel; ++i) {
        api.input_addr[i] = bm_mem_get_device_addr(in_dev_mem[i]);
        api.output_addr[i] = bm_mem_get_device_addr(out_dev_mem[i]);
    }

    api.channel = channel;
    api.is_packed = 0;
    api.delta = 0;
    api.stride_i[0] = stride_i;
    api.stride_o[0] = stride_o;
    api.kernel_addr = bm_mem_get_device_addr(kernel_mem);
    api.width[0] = input.width;
    api.height[0] = input.height;
    api.kw = ksize;
    api.kh = ksize;

    if (bm_get_chipid(handle, &chipid) != BM_SUCCESS) {
        printf("get chipid is error !\n");
        bm_free_device(handle, kernel_mem);
        return BM_ERR_FAILURE;
    }

    switch(chipid) {
        case BM1686:
            ret = bm_tpu_kernel_launch(handle, "cv_laplacian", (u8*)&api,
                                                sizeof(api), core_id);
            if (ret != BM_SUCCESS) {
                bmlib_log("LAPLACE", BMLIB_LOG_ERROR, "laplacian sync api error\n");
                bm_free_device(handle, kernel_mem);
                return ret;
            }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }

    bm_free_device(handle, kernel_mem);

    return ret;
}
