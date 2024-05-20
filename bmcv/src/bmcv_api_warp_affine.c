#include <stdint.h>
#include <stdio.h>
#include "bmcv_common.h"
#include "bmcv_internal.h"
#define NO_USE 0
static float det(float arcs[3][3],int n){
    if(n == 1){
        return arcs[0][0];
    }
    float ans = 0;
    float temp[3][3];
    int i,j,k;
    for(i = 0;i < n;i++){
        for(j = 0;j < n - 1;j++){
            for(k = 0;k < n - 1;k++){
                temp[j][k] = arcs[j+1][(k >= i) ? k+1 : k];
            }
        }
        float t = det(temp, n-1);
        if(i%2 == 0){
            ans += arcs[0][i] * t;
        }
        else{
            ans -=  arcs[0][i] * t;
        }
    }
    return ans;
}

static void inverse_matrix(float matrix[3][3], float matrix_inv[2][3]){
    float det_value = det(matrix, 3);

    matrix_inv[0][0] = matrix[1][1] / det_value;
    matrix_inv[0][1] = -matrix[0][1] / det_value;
    matrix_inv[0][2] = (matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1]) / det_value;
    matrix_inv[1][0] = - matrix[1][0] / det_value;
    matrix_inv[1][1] = matrix[0][0] / det_value;
    matrix_inv[1][2] = (matrix[0][2] * matrix[1][0] - matrix[0][0] * matrix[1][2]) / det_value;
}

static bm_status_t per_image_deal_nearest(bm_handle_t handle,
                        int image_dh,
                        int image_dw,
                        bm_image input,
                        bm_image output,
                        bm_device_mem_t tensor_output,
                        bmcv_affine_image_matrix matrix){
    int image_c = 3;
    bm_device_mem_t tensor_input;
    bm_device_mem_t tensor_temp;
    bm_device_mem_t tensor_S;
    bm_device_mem_t tensor_out_align;
    bm_status_t ret = BM_SUCCESS;
    bm_image_get_device_mem(input, &tensor_input);
    bm_image_get_device_mem(output, &tensor_output);
    sg_api_cv_warp_t param;
    int image_sh = input.height;
    int image_sw = input.width;
    param.output_image_addr = bm_mem_get_device_addr(tensor_output);
    param.input_image_addr = bm_mem_get_device_addr(tensor_input);
    ret = bm_malloc_device_byte(handle, &tensor_temp, input.height * input.width * 2);
    if (BM_SUCCESS != ret) {
        printf("bm_malloc error\n");
        return ret;
    }
    ret = bm_malloc_device_byte(handle, &tensor_out_align, output.height * output.width * 2);
    if (BM_SUCCESS != ret) {
        printf("bm_malloc error\n");
        bm_free_device(handle, tensor_temp);
        return ret;
    }
    param.input_image_addr_align = bm_mem_get_device_addr(tensor_temp);
    param.out_image_addr_align   = bm_mem_get_device_addr(tensor_out_align);
    param.image_n  = NO_USE;
    param.image_sh = image_sh;
    param.image_sw = image_sw;
    param.image_dh = image_dh;
    param.image_dw = image_dw;
    param.type = 0;   // 0: affine_nearest

    for (int i = 0;i < 6;i++){
        param.m.m[i] = matrix.matrix->m[i];
    }

    bm_image_get_stride(input, &(param.src_w_stride));
    ret = bm_malloc_device_byte(handle, &tensor_S, image_dh * image_dw * image_c * 4);
    if(BM_SUCCESS != ret) {
        printf("bm_malloc error\n");
        bm_free_device(handle, tensor_temp);
        bm_free_device(handle, tensor_out_align);
        return ret;
    }
    param.index_image_addr = bm_mem_get_device_addr(tensor_S);
    bm_image_get_stride(output, &(param.dst_w_stride));
    int core_id = 0;
    ret = bm_tpu_kernel_launch(handle, "sg_cv_warp_affine_1684x", (u8 *)&param, sizeof(param), core_id);
    if(BM_SUCCESS != ret){
        printf("sg_cv_warp_affine_a2 error\n");
        bm_free_device(handle, tensor_temp);
        bm_free_device(handle, tensor_out_align);
        return ret;
    }

    bm_free_device(handle, tensor_S);
    bm_free_device(handle, tensor_temp);
    bm_free_device(handle, tensor_out_align);
    return BM_SUCCESS;
}

static bm_status_t per_image_deal_bilinear(bm_handle_t handle,
                        int image_dh,
                        int image_dw,
                        bm_image input,
                        bm_image output,
                        bm_device_mem_t tensor_output,
                        bmcv_affine_image_matrix matrix,
                        bm_image_data_format_ext input_format,
                        bm_image_data_format_ext output_format){
    int image_c = 3;
    bm_device_mem_t tensor_input;
    bm_device_mem_t tensor_temp_r;
    bm_device_mem_t tensor_temp_g;
    bm_device_mem_t tensor_temp_b;
    bm_device_mem_t tensor_S;
    bm_device_mem_t tensor_sys_lu;
    bm_device_mem_t tensor_sys_ld;
    bm_device_mem_t tensor_sys_ru;
    bm_device_mem_t tensor_sys_rd;
    bm_device_mem_t tensor_out_align_a;
    bm_device_mem_t tensor_out_align_b;
    bm_status_t ret = BM_SUCCESS;
    bm_image_get_device_mem(input, &tensor_input);
    bm_image_get_device_mem(output, &tensor_output);
    sg_api_cv_warp_bilinear_t param;
    int image_sh = input.height;
    int image_sw = input.width;
    param.output_image_addr = bm_mem_get_device_addr(tensor_output);
    param.input_image_addr = bm_mem_get_device_addr(tensor_input);

    ret = bm_malloc_device_byte(handle, &tensor_temp_r, input.height * input.width * 4);
    if (BM_SUCCESS != ret) {
        printf("bm_malloc error\n");
        goto ERR_0;
    }

    ret = bm_malloc_device_byte(handle, &tensor_temp_g, input.height * input.width * 4);
    if (BM_SUCCESS != ret) {
        printf("bm_malloc error\n");
        goto ERR_1;
    }

    ret = bm_malloc_device_byte(handle, &tensor_temp_b, input.height * input.width * 4);
    if (BM_SUCCESS != ret) {
        printf("bm_malloc error\n");
        goto ERR_2;
    }

    ret = bm_malloc_device_byte(handle, &tensor_out_align_a, output.height * output.width * 4);
    if (BM_SUCCESS != ret) {
        printf("bm_malloc error\n");
        goto ERR_3;
    }

    ret = bm_malloc_device_byte(handle, &tensor_out_align_b, output.height * output.width * 4);
    if (BM_SUCCESS != ret) {
        printf("bm_malloc error\n");
        goto ERR_4;
    }

    param.input_image_addr_align_r = bm_mem_get_device_addr(tensor_temp_r);
    param.input_image_addr_align_g = bm_mem_get_device_addr(tensor_temp_g);
    param.input_image_addr_align_b = bm_mem_get_device_addr(tensor_temp_b);
    param.out_image_addr_align_a   = bm_mem_get_device_addr(tensor_out_align_a);
    param.out_image_addr_align_b   = bm_mem_get_device_addr(tensor_out_align_b);
    param.image_sh = image_sh;
    param.image_sw = image_sw;
    param.image_dh = image_dh;
    param.image_dw = image_dw;
    param.image_c = (input.image_format == FORMAT_BGR_PLANAR || input.image_format == FORMAT_RGB_PLANAR) ? 3 : 1;
    param.input_format  = (bm_image_data_format_ext)input_format;
    param.output_format = (bm_image_data_format_ext)output_format;
    param.type = 0;   // 0: affine_nearest

    for (int i = 0;i < 6;i++){
        param.m.m[i] = matrix.matrix->m[i];
    }

    ret = bm_malloc_device_byte(handle, &tensor_S, image_dh * image_dw * image_c * 4);
    if(BM_SUCCESS != ret) {
        printf("bm_malloc error\n");
        goto ERR_5;
    }

    ret = bm_malloc_device_byte(handle, &tensor_sys_lu, image_dh * image_dw * image_c * 4);
    if(BM_SUCCESS != ret) {
        printf("bm_malloc error\n");
        goto ERR_6;
    }

    ret = bm_malloc_device_byte(handle, &tensor_sys_ld, image_dh * image_dw * image_c * 4);
    if(BM_SUCCESS != ret) {
        printf("bm_malloc error\n");
        goto ERR_6;
    }

    ret = bm_malloc_device_byte(handle, &tensor_sys_ru, image_dh * image_dw * image_c * 4);
    if(BM_SUCCESS != ret) {
        printf("bm_malloc error\n");
        goto ERR_7;
    }

    ret = bm_malloc_device_byte(handle, &tensor_sys_rd, image_dh * image_dw * image_c * 4);
    if(BM_SUCCESS != ret) {
        printf("bm_malloc error\n");
        goto ERR_8;
    }

    param.index_image_addr = bm_mem_get_device_addr(tensor_S);
    param.system_image_addr_lu = bm_mem_get_device_addr(tensor_sys_lu);
    param.system_image_addr_ld = bm_mem_get_device_addr(tensor_sys_ld);
    param.system_image_addr_ru = bm_mem_get_device_addr(tensor_sys_ru);
    param.system_image_addr_rd = bm_mem_get_device_addr(tensor_sys_rd);
    int core_id = 0;
    ret = bm_tpu_kernel_launch(handle, "cv_api_warp_affine_bilinear_1684x", &param, sizeof(param), core_id);
    if(ret != BM_SUCCESS){
        printf("cv_api_warp_affine_bilinear_a2 error\n");
        goto ERR_9;
    }

ERR_9:
    bm_free_device(handle, tensor_sys_rd);
ERR_8:
    bm_free_device(handle, tensor_sys_ru);
ERR_7:
    bm_free_device(handle, tensor_sys_ld);
ERR_6:
    bm_free_device(handle, tensor_sys_lu);
ERR_5:
    bm_free_device(handle, tensor_S);
ERR_4:
    bm_free_device(handle, tensor_out_align_b);
ERR_3:
    bm_free_device(handle, tensor_out_align_a);
ERR_2:
    bm_free_device(handle, tensor_temp_b);
ERR_1:
    bm_free_device(handle, tensor_temp_g);
ERR_0:
    bm_free_device(handle, tensor_temp_r);

  return ret;
}

 bm_status_t bmcv_image_warp_affine_1684X(
    bm_handle_t handle,
    int image_num,
    bmcv_affine_image_matrix matrix[4],
    int image_dh,
    int image_dw,
    bm_image* input,
    bm_image* output,
    int use_bilinear){
    bm_status_t ret = BM_SUCCESS;
    bm_device_mem_t tensor_output[4];
    #ifdef __linux__
    bool output_alloc_flag[image_num];
    #else
    std::shared_ptr<bool> output_alloc_flag_(new bool[image_num], std::default_delete<bool[]>());
    bool*                 output_alloc_flag = output_alloc_flag_.get();
    #endif
    for (int num = 0;num < image_num;num++){
        output_alloc_flag[num] = false;
        if(!bm_image_is_attached(output[num])) {
            if(BM_SUCCESS !=bm_image_alloc_dev_mem(output[num], BMCV_HEAP_ANY)) {
                printf("bm_image_alloc_dev_mem error\r\n");
                for (int free_idx = 0; free_idx < num; free_idx ++) {
                    if (output_alloc_flag[free_idx]) {
                        bm_image_detach(output[num]);
                    }
                }
                return BM_ERR_FAILURE;
            }
            output_alloc_flag[num] = true;
        }
    }

    for (int num = 0;num < image_num;num++){
        if(use_bilinear){
            bm_image_data_format_ext input_format = input[0].data_type;
            bm_image_data_format_ext output_format = output[0].data_type;
            ret = per_image_deal_bilinear(handle, image_dh, image_dw, input[num], output[num],
                    tensor_output[num], matrix[num], input_format, output_format);
        }else{
            ret = per_image_deal_nearest(handle, image_dh, image_dw, input[num], output[num],
                    tensor_output[num], matrix[num]);
        }
        if (BM_SUCCESS != ret){
            for (int free_idx = 0; free_idx < num; free_idx ++) {
                if (output_alloc_flag[free_idx]) {
                    bm_image_detach(output[free_idx]);
                }
            }
            return ret;
        }
    }
    return BM_SUCCESS;
}

static bm_status_t bmcv_warp_check(
    bm_handle_t              handle,
    int                      image_num,
    bmcv_affine_image_matrix      matrix[4],
    bm_image*                input,
    bm_image*                output)
{
    if (handle == NULL) {
        printf("Can not get handle!\r\n");
        return BM_ERR_FAILURE;
    }
    bm_image_format_ext src_format = input[0].image_format;
    bm_image_data_format_ext src_type = input[0].data_type;
    bm_image_format_ext dst_format = output[0].image_format;
    bm_image_data_format_ext dst_type = output[0].data_type;
    int image_sh = input[0].height;
    int image_sw = input[0].width;
    int image_dh = output[0].height;
    int image_dw = output[0].width;
    int dw_stride = 0;
    bm_image_get_stride(output[0], &dw_stride);

    if ((input[0].data_type == DATA_TYPE_EXT_FP16) ||
        (output[0].data_type == DATA_TYPE_EXT_FP16)||
        (input[0].data_type == DATA_TYPE_EXT_BF16) ||
        (output[0].data_type == DATA_TYPE_EXT_BF16)){
        printf("data type not support\n");

        return BM_NOT_SUPPORTED;
    }

    if (!input || !output) {
        return BM_ERR_PARAM;
    }
    if (image_num < 0 || image_num > 4) {
        printf("expect 1 <= image_num <= 4 \n");
        return BM_NOT_SUPPORTED;
    }
    if (src_format != dst_format || src_type != dst_type) {
        printf("expect  the same input / output image format \n");
        return BM_NOT_SUPPORTED;
    }
    if (src_format != FORMAT_RGB_PLANAR && src_format != FORMAT_BGR_PLANAR) {
        printf("Not supported input image format \n");
        return BM_NOT_SUPPORTED;
    }
    ASSERT(bm_image_is_attached(input[0]));
    if (src_type == DATA_TYPE_EXT_1N_BYTE) {
        for (int i = 1; i < image_num; i++) {
            if (src_format != input[i].image_format || src_type != input[i].data_type) {
                printf("expected consistant input image format \n");
                return BM_NOT_SUPPORTED;
            }
            if (image_sh != input[i].height || image_sw != input[i].width) {
                printf("expected consistant input image size  \n");
                return BM_NOT_SUPPORTED;
            }
            ASSERT(bm_image_is_attached(input[i]));
        }
    }
    int out_num = 0;
    for (int i = 0; i < image_num; i++) {
        out_num += matrix[i].matrix_num;
    }
    if (out_num <= 0) {
        printf("illegal out_num\n");
        return BM_NOT_SUPPORTED;
    }
    if (dst_type == DATA_TYPE_EXT_1N_BYTE) {
        for (int i = 1; i < out_num; i++) {
            if (src_format != output[i].image_format || src_type != output[i].data_type) {
                printf("expect consistant output image format\n");
                return BM_NOT_SUPPORTED;
            }
            if (image_dh != output[i].height || image_dw != output[i].width) {
                printf("expect consistant output image size\n");
                return BM_NOT_SUPPORTED;
            }
            int stride = 0;
            bm_image_get_stride(output[i], &stride);
            if (dw_stride != stride) {
                printf("expect consistant output image stride \n");
                return BM_NOT_SUPPORTED;
            }
        }
    }

    return BM_SUCCESS;
}

inline int warp_get_idx(int input, int matrix_sigma[4], int image_num)
{
    for (int i = 0; i < image_num; i++) {
        if (input < matrix_sigma[i]) {
            return i;
        }
    }
    return 0;
}

bm_status_t bmcv_image_warp_affine(
    bm_handle_t handle,
    int image_num,
    bmcv_affine_image_matrix matrix[4],
    bm_image *input,
    bm_image *output,
    int use_bilinear)
{
    if(BM_SUCCESS !=bmcv_warp_check(handle, image_num, matrix, input, output)) {
        BMCV_ERR_LOG("bm_memcpy_s2d error\r\n");
        return BM_ERR_FAILURE;
    }

    unsigned int chipid;
    bm_get_chipid(handle, &chipid);
    if (chipid == BM1688){
        return bmcv_image_warp_affine_1684X(handle,
                image_num,
                &matrix[0],
                output->height,
                output->width,
                input,
                output,
                use_bilinear);
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_image_warp_affine_similar_to_opencv(
    bm_handle_t handle,
    int image_num,
    bmcv_affine_image_matrix matrix[4],
    bm_image *input,
    bm_image *output,
    int use_bilinear)
{
    UNUSED(use_bilinear);
    float matrix_tem[3][3];
    float matrix_tem_inv[2][3];
    for (int i = 0; i < image_num; i++) {
        for(int matrix_no = 0; matrix_no < matrix[i].matrix_num; matrix_no++){
            memset(matrix_tem, 0, sizeof(matrix_tem));
            memset(matrix_tem_inv, 0, sizeof(matrix_tem_inv));
            for(int a = 0;a < 6;a++){
                matrix_tem[(a/3)][(a%3)] = matrix[i].matrix->m[a];
            }
            matrix_tem[2][0] = 0;
            matrix_tem[2][1] = 0;
            matrix_tem[2][2] = 1;
            inverse_matrix(matrix_tem, matrix_tem_inv);
            for(int a = 0;a < 6;a++){
                float temp = matrix_tem_inv[(a/3)][(a%3)];
                matrix[i].matrix->m[a] = temp;
            }
        }
    }

    return bmcv_image_warp_affine(handle,
            image_num,
            &matrix[0],
            input, output,use_bilinear);
}

bm_status_t bmcv_image_warp(bm_handle_t            handle,
                            int                    image_num,
                            bmcv_affine_image_matrix matrix[4],
                            bm_image *             input,
                            bm_image *             output) {
    return bmcv_image_warp_affine(handle, image_num, matrix, input, output, 0);
}
