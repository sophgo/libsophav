#include <stdint.h>
#include <stdio.h>
#include "bmcv_common.h"
#include "bmcv_internal.h"
#define NO_USE 0

// Expanded by first row|A|
float getA(float arcs[3][3],int n)
{
    if(n == 1)
        return arcs[0][0];
    float ans = 0;
    float temp[3][3];
    int i, j, k;
    for(i = 0; i < n; i++){
        for(j = 0; j < n-1; j++)
            for(k = 0; k < n-1; k++)
                temp[j][k] = arcs[j+1][(k>=i)?k+1:k];
        float t = getA(temp,n-1);
        if(i % 2 == 0)
            ans += arcs[0][i]*t;
        else
            ans -= arcs[0][i]*t;
    }
    return ans;
}

//Calculate the remainder equation corresponding to each element in each column of each row to form A*
void getAStart(float arcs[3][3],int n,float ans[3][3])
{
    if(n == 1){
        ans[0][0] = 1;
        return;
    }
    int i, j, k, t;
    float temp[3][3];
    for(i = 0; i < n; i++)
        for(j = 0; j < n; j++){
            for(k = 0; k < n-1; k++)
                for(t = 0; t< n-1; t++)
                    temp[k][t] = arcs[k>=i?k+1:k][t>=j?t+1:t];
            ans[j][i] = getA(temp,n-1);
            if((i + j) % 2 == 1){
                ans[j][i] =- ans[j][i];
            }
        }
}

void inverse_matrix(int n, float arcs[3][3], float astar[3][3])
{
    int i, j;
    float a = getA(arcs, n);
    if(a == 0)
        printf("can not transform!\n");
    else{
        getAStart(arcs, n, astar);
        for(i = 0; i < n; i++)
            for(j = 0; j < n; j++)
                astar[i][j] = astar[i][j]/a;
    }
}

static bm_status_t bmcv_perspective_check(
    bm_handle_t                    handle,
    int                            image_num,
    bmcv_perspective_image_matrix  matrix[4],
    bm_image*                      input,
    bm_image*                      output)
{
    if (handle == NULL) {
        //bmlib_log("PERSPECTIVE", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
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
    float m[9];
    bm_image_get_stride(output[0], &dw_stride);

    if ((input[0].data_type == DATA_TYPE_EXT_FP16) ||
        (output[0].data_type == DATA_TYPE_EXT_FP16)||
        (input[0].data_type == DATA_TYPE_EXT_BF16) ||
        (output[0].data_type == DATA_TYPE_EXT_BF16)){
        // printf("data type not support\n");

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
         printf("expect  the same input / output image format\n");
        return BM_NOT_SUPPORTED;
    }
    if (src_format != FORMAT_RGB_PLANAR && src_format != FORMAT_BGR_PLANAR) {
        printf("Not supported input image format\n");
        return BM_NOT_SUPPORTED;
    }

    bm_image_is_attached(input[0]);
    if (src_type == DATA_TYPE_EXT_1N_BYTE) {
        for (int i = 1; i < image_num; i++) {
            if (src_format != input[i].image_format || src_type != input[i].data_type) {
                printf("Expected consistant input image format\n");
                return BM_NOT_SUPPORTED;
            }
            if (image_sh != input[i].height || image_sw != input[i].width) {
                printf("Expected consistant input image size\n");
                return BM_NOT_SUPPORTED;
            }
            bm_image_is_attached(input[i]);
        }
    }
    int out_num = 0;
    for (int i = 0; i < image_num; i++) {
        out_num += matrix[i].matrix_num;
    }
    if (out_num <= 0) {
        printf("Illegal out_num\n");
        return BM_NOT_SUPPORTED;
    }
    if (dst_type == DATA_TYPE_EXT_1N_BYTE) {
        for (int i = 1; i < out_num; i++) {
            if (src_format != output[i].image_format || src_type != output[i].data_type) {
                printf(" Expect consistant output image format\n");
                return BM_NOT_SUPPORTED;
            }
            if (image_dh != output[i].height || image_dw != output[i].width) {
                printf(" Expect  consistant output image size\n");
                return BM_NOT_SUPPORTED;
            }
            int stride = 0;
            bm_image_get_stride(output[i], &stride);
            if (dw_stride != stride) {
                printf(" Expect  consistant output image stride\n");
                return BM_NOT_SUPPORTED;
            }
        }
    }
    for (int i = 0; i < image_num; i++) {
        for (int j = 0; j < matrix[i].matrix_num; j++) {
            for(int a=0;a<9;a++)
                m[a]=matrix[i].matrix[j].m[a];
            int left_top_x = (int)((m[0] * 0 + m[1] * 0 + m[2]) / (m[6] * 0 + m[7] * 0 + m[8]));
            int left_top_y = (int)((m[3] * 0 + m[4] * 0 + m[5]) / (m[6] * 0 + m[7] * 0 + m[8]));
            int left_btm_x = (int)((m[0] * 0 + m[1] * (image_dh - 1) + m[2]) / (m[6] * 0 + m[7] * (image_dh - 1) + m[8]));
            int left_btm_y = (int)((m[3] * 0 + m[4] * (image_dh - 1) + m[5]) / (m[6] * 0 + m[7] * (image_dh - 1) + m[8]));
            int right_top_x = (int)((m[0] * (image_dw - 1) + m[1] * 0 + m[2]) / (m[6] * (image_dw - 1) + m[7] * 0 + m[8]));
            int right_top_y = (int)((m[3] * (image_dw - 1) + m[4] * 0 + m[5]) / (m[6] * (image_dw - 1) + m[7] * 0 + m[8]));
            int right_btm_x = (int)((m[0] * (image_dw - 1) + m[1] * (image_dh - 1) + m[2]) / (m[6] * (image_dw - 1) + m[7] * (image_dh - 1) + m[8]));
            int right_btm_y = (int)((m[3] * (image_dw - 1) + m[4] * (image_dh - 1) + m[5]) / (m[6] * (image_dw - 1) + m[7] * (image_dh - 1) + m[8]));
            if (left_top_x < 0 || left_top_x > image_sw ||
                left_top_y < 0 || left_top_y > image_sh ||
                left_btm_x < 0 || left_btm_x > image_sw ||
                left_btm_y < 0 || left_btm_y > image_sh ||
                right_top_x < 0 || right_top_x > image_sw ||
                right_top_y < 0 || right_top_y > image_sh ||
                right_btm_x < 0 || right_btm_x > image_sw ||
                right_btm_y < 0 || right_btm_y > image_sh) {

                printf("Output image is out of input image range\n");
                return BM_NOT_SUPPORTED;
            }
        }
    }

    return BM_SUCCESS;
}

static void get_perspective_transform(int* sx, int* sy, int dw, int dh, float* matrix) {
    int A = sx[3] + sx[0] - sx[1] - sx[2];
    int B = sy[3] + sy[0] - sy[1] - sy[2];
    int C = sx[2] - sx[3];
    int D = sy[2] - sy[3];
    int E = sx[1] - sx[3];
    int F = sy[1] - sy[3];
    matrix[8] = 1;
    matrix[7] = ((float)(A * F - B * E) / (float)dh) / (float)(C * F - D * E);
    matrix[6] = ((float)(A * D - B * C) / (float)dw) / (float)(D * E - C * F);
    matrix[0] = (matrix[6] * dw * sx[1] + sx[1] - sx[0]) / dw;
    matrix[1] = (matrix[7] * dh * sx[2] + sx[2] - sx[0]) / dh;
    matrix[2] = sx[0];
    matrix[3] = (matrix[6] * dw * sy[1] + sy[1] - sy[0]) / dw;
    matrix[4] = (matrix[7] * dh * sy[2] + sy[2] - sy[0]) / dh;
    matrix[5] = sy[0];
}

static bm_status_t per_image_deal_nearest(bm_handle_t handle,
                        int image_dh,
                        int image_dw,
                        bm_image input,
                        bm_image output,
                        bm_device_mem_t tensor_output,
                        bmcv_perspective_image_matrix matrix){
    int image_c = 3;
    bm_status_t ret = BM_SUCCESS;
    bm_device_mem_t tensor_S;
    bm_device_mem_t tensor_temp;
    bm_device_mem_t tensor_out_align;
    bm_device_mem_t tensor_input;
    bm_image_get_device_mem(input, &tensor_input);
    bm_image_get_device_mem(output, &tensor_output);
    ret = bm_malloc_device_byte(handle, &tensor_S, image_dh * image_dw * image_c * 4);
    if(BM_SUCCESS != ret) {
        printf("Bm_malloc error\n");
        return ret;
    }

    sg_api_cv_warp_perspective_1684x_t param;
    param.image_n  = NO_USE;
    param.image_sh = input.height;
    param.image_sw = input.width;
    param.image_dh = image_dh;
    param.image_dw = image_dw;
    param.type     = 1;   // 1: perspective_nearest

    param.input_image_addr = bm_mem_get_device_addr(tensor_input);
    ret = bm_malloc_device_byte(handle, &tensor_temp, input.height * input.width * 2);
    if (BM_SUCCESS != ret) {
        printf("Bm_malloc error\n");
        bm_free_device(handle, tensor_S);
        return ret;
    }
    ret = bm_malloc_device_byte(handle, &tensor_out_align, output.height * output.width * 2);
    if (BM_SUCCESS != ret) {
        printf("Bm_malloc error\n");
        bm_free_device(handle, tensor_S);
        bm_free_device(handle, tensor_temp);
        return ret;
    }
    param.output_image_addr = bm_mem_get_device_addr(tensor_output);
    param.index_image_addr = bm_mem_get_device_addr(tensor_S);
    param.input_image_addr_align = bm_mem_get_device_addr(tensor_temp);
    param.out_image_addr_align = bm_mem_get_device_addr(tensor_out_align);
    bm_image_get_stride(input, &(param.src_w_stride));
    bm_image_get_stride(output, &(param.dst_w_stride));
    for (int i = 0;i < 9;i++){
        param.m.m[i] = matrix.matrix->m[i];
    }
    int core_id = 0;
    ret = bm_tpu_kernel_launch(handle, "sg_cv_warp_perspective_1684x", (u8 *)&param, sizeof(param), core_id);
    if (BM_SUCCESS != ret) {
        printf("Sg_cv_warp_perspective_1688 error\n");
        bm_free_device(handle, tensor_S);
        bm_free_device(handle, tensor_temp);
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
                        bmcv_perspective_image_matrix matrix){
    int image_c = 3;
    bm_status_t ret = BM_SUCCESS;
    bm_device_mem_t tensor_S;
    bm_device_mem_t tensor_temp;
    bm_device_mem_t tensor_out_align;
    bm_device_mem_t tensor_input;
    bm_image_get_device_mem(input, &tensor_input);
    bm_image_get_device_mem(output, &tensor_output);
    ret = bm_malloc_device_byte(handle, &tensor_S, image_dh * image_dw * image_c * 4);
    if (BM_SUCCESS != ret) return ret;

    sg_api_cv_warp_perspective_1684x_t param;
    param.image_n  = image_c;
    param.image_sh = input.height;
    param.image_sw = input.width;
    param.image_dh = image_dh;
    param.image_dw = image_dw;
    param.type     = 3;   // 0: affine_nearest  1: perspective_nearest
                          // 2: affine_bilinear 3: perspective_bilinear
    param.input_image_addr = bm_mem_get_device_addr(tensor_input);
    ret = bm_malloc_device_byte(handle, &tensor_temp, input.height * input.width * 2);
    if (BM_SUCCESS != ret) {
        bm_free_device(handle, tensor_S);
        return ret;
    }
    ret = bm_malloc_device_byte(handle, &tensor_out_align, output.height * output.width * 2);
    if (BM_SUCCESS != ret) {
        bm_free_device(handle, tensor_S);
        bm_free_device(handle, tensor_temp);
        return ret;
    }
    param.output_image_addr = bm_mem_get_device_addr(tensor_output);
    param.index_image_addr = bm_mem_get_device_addr(tensor_S);
    param.input_image_addr_align = bm_mem_get_device_addr(tensor_temp);
    param.out_image_addr_align = bm_mem_get_device_addr(tensor_out_align);
    bm_image_get_stride(input, &(param.src_w_stride));
    bm_image_get_stride(output, &(param.dst_w_stride));

    for (int i = 0;i < 9;i++){
        param.m.m[i] = matrix.matrix->m[i];
    }
    int core_id = 0;
    ret = bm_tpu_kernel_launch(handle, "sg_cv_warp_perspective_1684x", (u8 *)&param, sizeof(param), core_id);
    if (BM_SUCCESS != ret) {
        bm_free_device(handle, tensor_S);
        bm_free_device(handle, tensor_temp);
	bm_free_device(handle, tensor_out_align);
        return ret;
    }
    bm_free_device(handle, tensor_S);
    bm_free_device(handle, tensor_temp);
    bm_free_device(handle, tensor_out_align);
    return BM_SUCCESS;
}
bm_status_t bmcv_image_warp_perspective_1684X(
    bm_handle_t handle,
    int image_num,
    bmcv_perspective_image_matrix matrix[4],
    int image_dh,
    int image_dw,
    bm_image* input,
    bm_image* output,
    int use_bilinear)
{
    bm_status_t ret = BM_SUCCESS;
    bm_device_mem_t tensor_output[4];
    #ifdef __linux__
        bool output_alloc_flag[image_num];
    #else
        std::shared_ptr<bool> output_alloc_flag_(new bool[image_num], std::default_delete<bool[]>());
        bool*                 output_alloc_flag = output_alloc_flag_.get();
    #endif
    if(BM_SUCCESS != bmcv_perspective_check(handle, image_num, matrix, input, output)) {
        return BM_ERR_FAILURE;
    }
    for (int num = 0;num < image_num;num++) {
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

    for (int num = 0;num < image_num;num++) {
        if (!use_bilinear) {
        ret = per_image_deal_nearest(handle, image_dh, image_dw, input[num], output[num],
                    tensor_output[num], matrix[num]);
        } else {
        ret = per_image_deal_bilinear(handle, image_dh, image_dw, input[num], output[num],
                    tensor_output[num], matrix[num]);
        }
        if (BM_SUCCESS != ret) {
            for (int free_idx = 0; free_idx < num; free_idx ++) {
                if (output_alloc_flag[free_idx])
                    bm_image_detach(output[free_idx]);
            }
            return ret;
        }
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_image_warp_perspective(
    bm_handle_t                   handle,
    int                           image_num,
    bmcv_perspective_image_matrix matrix[4],
    bm_image *                    input,
    bm_image *                    output,
    int                           use_bilinear)
{
    unsigned int chipid;
    bm_get_chipid(handle, &chipid);
    if (chipid == BM1688){
        return bmcv_image_warp_perspective_1684X(
                handle,
                image_num,
                matrix,
                output->height,
                output->width,
                input,
                output,
                use_bilinear);
    }
    else {
        printf(" NOT SUPPORT! \n");
        return BM_ERR_FAILURE;
    }
}

bm_status_t bmcv_image_warp_perspective_with_coordinate(
    bm_handle_t                       handle,
    int                               image_num,
    bmcv_perspective_image_coordinate coord[4],
    bm_image *                        input,
    bm_image *                        output,
    int                               use_bilinear)
{
    bmcv_perspective_image_matrix matrix[4];
    int dh = output[0].height;
    int dw = output[0].width;
    int coord_sum = 0;
    for (int i = 0; i < image_num; i++) {
        coord_sum += coord[i].coordinate_num;
    }
    #ifdef __linux__
    bmcv_perspective_matrix mat[coord_sum];
    #else
    std::shared_ptr<bmcv_perspective_matrix> mat_(new bmcv_perspective_matrix[coord_sum], std::default_delete<bmcv_perspective_matrix[]>());
    bmcv_perspective_matrix* mat = mat_.get();
    #endif
    int idx = 0;
    for (int i = 0; i < image_num; i++) {
        int index = idx;
        matrix[i].matrix_num = coord[i].coordinate_num;
        for (int j = 0; j < matrix[i].matrix_num; j++) {
            get_perspective_transform(coord[i].coordinate[j].x,
                                      coord[i].coordinate[j].y,
                                      dw - 1,
                                      dh - 1,
                                      mat[idx].m);
            idx++;
        }
        matrix[i].matrix = mat + index;
    }

    return bmcv_image_warp_perspective(handle,
                                       image_num,
                                       matrix,
                                       input,
                                       output,
                                       use_bilinear);
}

bm_status_t bmcv_image_warp_perspective_similar_to_opencv(
    bm_handle_t                       handle,
    int                               image_num,
    bmcv_perspective_image_matrix     matrix[4],
    bm_image *                        input,
    bm_image *                        output,
    int                               use_bilinear)
{
    float matrix_tem[3][3];
    float matrix_tem_inv[3][3];
    for (int i = 0; i < image_num; i++) {
        for(int matrix_no = 0; matrix_no < matrix[i].matrix_num; matrix_no++){
            memset(matrix_tem, 0, sizeof(matrix_tem));
            memset(matrix_tem_inv, 0, sizeof(matrix_tem_inv));
            for(int a=0;a<9;a++){
                    matrix_tem[(a/3)][(a%3)]=matrix[i].matrix->m[a];
            }
            inverse_matrix(3, matrix_tem, matrix_tem_inv);
            for(int a=0;a<9;a++){
                    matrix[i].matrix->m[a]=matrix_tem_inv[(a/3)][(a%3)];
            }
        }
    }

    return bmcv_image_warp_perspective(handle,
                                       image_num,
                                       matrix,
                                       input,
                                       output,
                                       use_bilinear);
}