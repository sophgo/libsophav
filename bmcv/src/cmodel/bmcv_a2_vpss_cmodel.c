#include "bmcv_api_ext_c.h"
#include "bmcv_cmodel.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

bm_status_t vpss_csctype_to_csc_cfg(
    bm_image               *input,
    bm_image               *output,
    csc_type_t*            csc_type,
    bmcv_csc_cfg*          csc_cfg)
{
    bm_status_t ret = BM_SUCCESS;

    csc_cfg->is_fancy = false;

    if(*csc_type <= CSC_RGB2YPbPr_BT709)
        csc_cfg->csc_type = *csc_type;
    else {
        int input_color_space = COLOR_SPACE_YUV, output_color_space = COLOR_SPACE_YUV;

        input_color_space = is_csc_yuv_or_rgb(input[0].image_format);
        output_color_space = is_csc_yuv_or_rgb(output[0].image_format);

        switch(*csc_type)
        {
            case CSC_USER_DEFINED_MATRIX:
            case CSC_MAX_ENUM:
                if((input_color_space == COLOR_SPACE_YUV) && (output_color_space == COLOR_SPACE_RGB))
                    csc_cfg->csc_type = VPSS_CSC_YCbCr2RGB_BT601;
                if((input_color_space == COLOR_SPACE_RGB) && (output_color_space == COLOR_SPACE_YUV))
                    csc_cfg->csc_type = VPSS_CSC_RGB2YCbCr_BT601;
                if((input_color_space == COLOR_SPACE_YUV) && (output_color_space == COLOR_SPACE_YUV))
                    csc_cfg->csc_type = VPSS_CSC_YCbCr2YCbCr_BT601;
                if((input_color_space == COLOR_SPACE_RGB) && (output_color_space == COLOR_SPACE_RGB))
                    csc_cfg->csc_type = VPSS_CSC_RGB2RGB;
                break;
            case CSC_FANCY_PbPr_BT601:
                if((input_color_space == COLOR_SPACE_YUV) && (output_color_space == COLOR_SPACE_RGB))
                    csc_cfg->csc_type = VPSS_CSC_YPbPr2RGB_BT601;
                if((input_color_space == COLOR_SPACE_RGB) && (output_color_space == COLOR_SPACE_YUV))
                    csc_cfg->csc_type = VPSS_CSC_RGB2YPbPr_BT601;
                if((input_color_space == COLOR_SPACE_YUV) && (output_color_space == COLOR_SPACE_YUV))
                    csc_cfg->csc_type = VPSS_CSC_YPbPr2YPbPr_BT601;
                if((input_color_space == COLOR_SPACE_RGB) && (output_color_space == COLOR_SPACE_RGB))
                    csc_cfg->csc_type = VPSS_CSC_RGB2RGB;
                csc_cfg->is_fancy = true;
                break;
            case CSC_FANCY_PbPr_BT709:
                if((input_color_space == COLOR_SPACE_YUV) && (output_color_space == COLOR_SPACE_RGB))
                    csc_cfg->csc_type = VPSS_CSC_YPbPr2RGB_BT709;
                if((input_color_space == COLOR_SPACE_RGB) && (output_color_space == COLOR_SPACE_YUV))
                    csc_cfg->csc_type = VPSS_CSC_RGB2YPbPr_BT709;
                if((input_color_space == COLOR_SPACE_YUV) && (output_color_space == COLOR_SPACE_YUV))
                    csc_cfg->csc_type = VPSS_CSC_YPbPr2YPbPr_BT709;
                if((input_color_space == COLOR_SPACE_RGB) && (output_color_space == COLOR_SPACE_RGB))
                    csc_cfg->csc_type = VPSS_CSC_RGB2RGB;
                csc_cfg->is_fancy = true;
                break;
            default:
                ret = BM_NOT_SUPPORTED;
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                    "bm_vpss csc_type param %d not support,%s: %s: %d\n",
                    *csc_type, filename(__FILE__), __func__, __LINE__);
                return ret;
        }
    }
	return ret;
}

unsigned short u8_to_fp16(unsigned char val) {
    // Special case for 0
    if (val == 0) {
        return 0;
    }

    // Convert the u8 value into a float
    float f_val = (float)val;

    // Convert float to its integer representation without violating aliasing rules
    unsigned int f_as_int;
    memcpy(&f_as_int, &f_val, sizeof(f_val));

    // Convert float to FP16 (simplified version)
    // Note: This won't handle infinities, NaN, or denormals
    unsigned short sign = (f_as_int >> 31) & 0x1;
    unsigned short exp = ((f_as_int & 0x7F800000) >> 23) - 127 + 15;
    unsigned short rounding_bit = (f_as_int & (1 << 12)) >> 12;
    unsigned short frac = (f_as_int & 0x007FE000) >> 13;  // Only keeping upper 10 bits

    // Rounding
    frac += rounding_bit;

    unsigned short fp16_val = (sign << 15) | (exp << 10) | frac;
    return fp16_val;
}

unsigned short u8_to_bf16(unsigned char val) {
    // Convert the u8 value into a float
    float f_val = (float)val;

    // Convert float to its integer representation without violating aliasing rules
    unsigned int f_as_int;
    memcpy(&f_as_int, &f_val, sizeof(f_val));

    // Convert float to bf16 (simplified version)
    // Note: This won't handle infinities, NaN, or denormals
    unsigned short sign = (f_as_int >> 31) & 0x1;
    unsigned short exp = (f_as_int & 0x7F800000) >> 23;  // Keep the same exponent as fp32
    unsigned short rounding_bit = (f_as_int & (1 << 15)) >> 15;
    unsigned short frac = (f_as_int & 0x007FFF80) >> 16;  // Only keeping upper 7 bits

    // Rounding
    frac += rounding_bit;

    unsigned short bf16_val = (sign << 15) | (exp << 7) | frac;
    return bf16_val;
}

void convert_u8_to_float32(unsigned char *u8_data, float *float_data, int size) {
    for (int i = 0; i < size; i++) {
        float_data[i] = (float)u8_data[i];
    }
}

void convert_u8_to_fp16(unsigned char *input, unsigned short *output, int size) {
    for (int i = 0; i < size; i++) {
        output[i] = u8_to_fp16(input[i]);
    }
}

void convert_u8_to_bf16(unsigned char *input, unsigned short *output, int size) {
    for (int i = 0; i < size; i++) {
        output[i] = u8_to_bf16(input[i]);
    }
}

void _h2v2_upsample(int width, int height, u8* input_data, u8* output_data, u8 is_fancy){
    if(is_fancy)
        h2v2_fancy_upsample(width, height, input_data, output_data);
    else
        h2v2_upsample(width, height, input_data, output_data);
};
void _h2v1_upsample(int width, int height, u8* input_data, u8* output_data, u8 is_fancy){
    if(is_fancy)
        h2v1_fancy_upsample(width, height, input_data, output_data);
    else
        h2v1_upsample(width, height, input_data, output_data);
};

bm_status_t vpss_upsample(void **src, unsigned char *dst, bm_image_format_ext format, int width, int height, u8 is_fancy) {
    unsigned char *u_data = NULL;
    unsigned char *v_data = NULL;
    unsigned char *y_data = NULL;
    unsigned char *y_temp, *u_temp, *v_temp, *uv_data, *yuv_data;
    switch (format) {
        case FORMAT_YUV420P:
            memcpy(dst, src[0], width * height);
            u_data = (unsigned char *)malloc(width / 2 * (height / 2 + 2));
            v_data = (unsigned char *)malloc(width / 2 * (height / 2 + 2));
            // U component(repeat the first and last row)
            memcpy((void *)(u_data), src[1], width / 2);
            memcpy((void *)(u_data + width / 2), src[1], width * height / 4);
            memcpy((void *)(u_data + width / 2 * (height / 2 + 1)),
                   src[1] + width / 2 * (height / 2 - 1), width / 2);
            // V component(repeat the first and last row)
            memcpy((void *)(v_data), src[2], width / 2);
            memcpy((void *)(v_data + width / 2), src[2], width * height / 4);
            memcpy((void *)(v_data + width / 2 * (height / 2 + 1)),
                   src[2] + width / 2 * (height / 2 - 1), width / 2);

            _h2v2_upsample(width, height, u_data + width / 2, dst + width * height, is_fancy);
            _h2v2_upsample(width, height, v_data + width / 2, dst + 2 * width * height, is_fancy);
            break;

        case FORMAT_YUV422P:
            memcpy(dst, src[0], width * height);
            _h2v1_upsample(width, height, src[1], dst + width * height, is_fancy);
            _h2v1_upsample(width, height, src[2], dst + 2 * width * height, is_fancy);
            break;

        case FORMAT_NV12:
            memcpy(dst, src[0], width * height);
            u_data = (unsigned char *)malloc(width / 2 * (height / 2 + 2));
            v_data = (unsigned char *)malloc(width / 2 * (height / 2 + 2));
            uv_data = (unsigned char *)src[1];
            u_temp = u_data, v_temp = v_data;
            // copy the first row
            for (int i = 0; i < width; i += 2) {
                *(u_temp++) = uv_data[i];
                *(v_temp++) = uv_data[i + 1];
            }
            for (int i = 0; i < width * height / 2; i += 2) {
                *(u_temp++) = uv_data[i];
                *(v_temp++) = uv_data[i + 1];
            }
            // copy the last row
            for (int i = width * (height / 2 - 1); i < width * height / 2; i += 2) {
                *(u_temp++) = uv_data[i];
                *(v_temp++) = uv_data[i + 1];
            }
            _h2v2_upsample(width, height, u_data + width / 2, dst + width * height, is_fancy);
            _h2v2_upsample(width, height, v_data + width / 2, dst + 2 * width * height, is_fancy);
            break;

        case FORMAT_NV21:
            memcpy(dst, src[0], width * height);
            u_data = (unsigned char *)malloc(width / 2 * (height / 2 + 2));
            v_data = (unsigned char *)malloc(width / 2 * (height / 2 + 2));
            uv_data = (unsigned char *)src[1];
            u_temp = u_data, v_temp = v_data;
            // copy the first row
            for (int i = 0; i < width; i += 2) {
                *(v_temp++) = uv_data[i];
                *(u_temp++) = uv_data[i + 1];
            }
            for (int i = 0; i < width * height / 2; i += 2) {
                *(v_temp++) = uv_data[i];
                *(u_temp++) = uv_data[i + 1];
            }
            // copy the last row
            for (int i = width * (height / 2 - 1); i < width * height / 2; i += 2) {
                *(v_temp++) = uv_data[i];
                *(u_temp++) = uv_data[i + 1];
            }
            _h2v2_upsample(width, height, u_data + width / 2, dst + width * height, is_fancy);
            _h2v2_upsample(width, height, v_data + width / 2, dst + 2 * width * height, is_fancy);
            break;

        case FORMAT_NV16:
            memcpy(dst, src[0], width * height);
            u_data = (unsigned char *)malloc(width * height / 2);
            v_data = (unsigned char *)malloc(width * height / 2);
            uv_data = (unsigned char *)src[1];
            u_temp = u_data, v_temp = v_data;
            for (int i = 0; i < width * height; i += 2) {
                *(u_temp++) = uv_data[i];
                *(v_temp++) = uv_data[i + 1];
            }
            _h2v1_upsample(width, height, u_data, dst + width * height, is_fancy);
            _h2v1_upsample(width, height, v_data, dst + 2 * width * height, is_fancy);
            break;

        case FORMAT_NV61:
            memcpy(dst, src[0], width * height);
            u_data = (unsigned char *)malloc(width * height / 2);
            v_data = (unsigned char *)malloc(width * height / 2);
            uv_data = (unsigned char *)src[1];
            u_temp = u_data, v_temp = v_data;
            for (int i = 0; i < width * height; i += 2) {
                *(v_temp++) = uv_data[i];
                *(u_temp++) = uv_data[i + 1];
            }
            _h2v1_upsample(width, height, u_data, dst + width * height, is_fancy);
            _h2v1_upsample(width, height, v_data, dst + 2 * width * height, is_fancy);
            break;

        case FORMAT_YUV422_YUYV:
            yuv_data = (unsigned char *)src[0];
            y_data = (unsigned char *)malloc(width * height);
            u_data = (unsigned char *)malloc(width * height / 2);
            v_data = (unsigned char *)malloc(width * height / 2);
            y_temp = y_data, u_temp = u_data, v_temp = v_data;
            for (int i = 0; i < width * height; i += 2) {
                *(y_temp++) = *(yuv_data++);
                *(u_temp++) = *(yuv_data++);
                *(y_temp++) = *(yuv_data++);
                *(v_temp++) = *(yuv_data++);
            }
            memcpy(dst, y_data, width * height);
            _h2v1_upsample(width, height, u_data, dst + width * height, is_fancy);
            _h2v1_upsample(width, height, v_data, dst + 2 * width * height, is_fancy);
            break;

        case FORMAT_YUV422_YVYU:
            yuv_data = (unsigned char *)src[0];
            y_data = (unsigned char *)malloc(width * height);
            u_data = (unsigned char *)malloc(width * height / 2);
            v_data = (unsigned char *)malloc(width * height / 2);
            y_temp = y_data, u_temp = u_data, v_temp = v_data;
            for (int i = 0; i < width * height; i += 2) {
                *(y_temp++) = *(yuv_data++);
                *(v_temp++) = *(yuv_data++);
                *(y_temp++) = *(yuv_data++);
                *(u_temp++) = *(yuv_data++);
            }
            memcpy(dst, y_data, width * height);
            _h2v1_upsample(width, height, u_data, dst + width * height, is_fancy);
            _h2v1_upsample(width, height, v_data, dst + 2 * width * height, is_fancy);
            break;

        case FORMAT_YUV422_UYVY:
            yuv_data = (unsigned char *)src[0];
            y_data = (unsigned char *)malloc(width * height);
            u_data = (unsigned char *)malloc(width * height / 2);
            v_data = (unsigned char *)malloc(width * height / 2);
            y_temp = y_data, u_temp = u_data, v_temp = v_data;
            for (int i = 0; i < width * height; i += 2) {
                *(u_temp++) = *(yuv_data++);
                *(y_temp++) = *(yuv_data++);
                *(v_temp++) = *(yuv_data++);
                *(y_temp++) = *(yuv_data++);
            }
            memcpy(dst, y_data, width * height);
            _h2v1_upsample(width, height, u_data, dst + width * height, is_fancy);
            _h2v1_upsample(width, height, v_data, dst + 2 * width * height, is_fancy);
            break;

        case FORMAT_YUV422_VYUY:
            yuv_data = (unsigned char *)src[0];
            y_data = (unsigned char *)malloc(width * height);
            u_data = (unsigned char *)malloc(width * height / 2);
            v_data = (unsigned char *)malloc(width * height / 2);
            y_temp = y_data, u_temp = u_data, v_temp = v_data;
            for (int i = 0; i < width * height; i += 2) {
                *(v_temp++) = *(yuv_data++);
                *(y_temp++) = *(yuv_data++);
                *(u_temp++) = *(yuv_data++);
                *(y_temp++) = *(yuv_data++);
            }
            memcpy(dst, y_data, width * height);
            _h2v1_upsample(width, height, u_data, dst + width * height, is_fancy);
            _h2v1_upsample(width, height, v_data, dst + 2 * width * height, is_fancy);
            break;

        default:
            printf("The input format is not supported!");
            break;
    }
    if (y_data) free(y_data);
    if (u_data) free(u_data);
    if (v_data) free(v_data);
    return BM_SUCCESS;
}

bm_status_t vpss_yuv2rgbp(unsigned char *src, int width, int height, bmcv_csc_cfg csc_cfg) {
    unsigned char *R = src;
    unsigned char *G = src + width * height;
    unsigned char *B = src + 2 * width * height;

    unsigned char *Y = src;
    unsigned char *U = src + width * height;
    unsigned char *V = src + 2 * width * height;

    double coefYR, coefYG, coefYB, coefUR, coefUG, coefUB, coefVR, coefVG, coefVB;
    double offsetY = 0, offsetU = 0, offsetV = 0;

    switch (csc_cfg.csc_type) {
        case VPSS_CSC_YCbCr2RGB_BT601:
        case VPSS_CSC_YCbCr2YCbCr_BT601:
            coefYR = 1.1643835616438400, coefYG = 0.0000000000000000, coefYB = 1.5958974359999800, offsetY = 16;
            coefUR = 1.1643835616438400, coefUG = -0.3936638456015240, coefUB = -0.8138402992560010, offsetU = 128;
            coefVR = 1.1643835616438400, coefVG = 2.0160738896544600, coefVB = 0.0000000000000000, offsetV = 128;
            break;

        case VPSS_CSC_YCbCr2RGB_BT709:
        case VPSS_CSC_YCbCr2YCbCr_BT709:
            coefYR = 1.1643835616438400, coefYG = 0.0000000000000000, coefYB = 1.7926522634175300, offsetY = 16;
            coefUR = 1.1643835616438400, coefUG = -0.2132370215686210, coefUB = -0.5330040401422640, offsetU = 128;
            coefVR = 1.1643835616438400, coefVG = 2.1124192819911800, coefVB = 0.0000000000000000, offsetV = 128;
            break;

        case VPSS_CSC_YPbPr2RGB_BT601:
        case VPSS_CSC_YPbPr2YPbPr_BT601:
            coefYR = 1.0000000000000000, coefYG = 0.0000000000000000, coefYB = 1.4018863751529200;
            coefUR = 1.0000000000000000, coefUG = -0.3458066722146720, coefUB = -0.7149028511111540, offsetU = 128;
            coefVR = 1.0000000000000000, coefVG = 1.7709825540494100, coefVB = 0.0000000000000000, offsetV = 128;
            break;

        case VPSS_CSC_YPbPr2RGB_BT709:
        case VPSS_CSC_YPbPr2YPbPr_BT709:
            coefYR = 1.0000000000000000, coefYG = 0.0000000000000000, coefYB = 1.5747219882569700;
            coefUR = 1.0000000000000000, coefUG = -0.1873140895347890, coefUB = -0.4682074705563420, offsetU = 128;
            coefVR = 1.0000000000000000, coefVG = 1.8556153692785300, coefVB = 0.0000000000000000, offsetV = 128;
            break;

        default:
            return BM_NOT_SUPPORTED;  // Invalid csc_type
    }

    for (int i = 0; i < width * height; i++) {
        double r = R[i] - offsetY;
        double g = G[i] - offsetU;
        double b = B[i] - offsetV;

        double y = coefYR * r + coefYG * g + coefYB * b;
        double u = coefUR * r + coefUG * g + coefUB * b;
        double v = coefVR * r + coefVG * g + coefVB * b;

        // Clipping values to the range 0-255
        Y[i] = y < 0 ? 0 : y > 254.5 ? 255 : (unsigned char)(y + 0.5);
        U[i] = u < 0 ? 0 : u > 254.5 ? 255 : (unsigned char)(u + 0.5);
        V[i] = v < 0 ? 0 : v > 254.5 ? 255 : (unsigned char)(v + 0.5);
    }
    return BM_SUCCESS;
}

bm_status_t vpss_rgbp2rgb(unsigned char *src, int width, int height) {
    unsigned char *rgbPixel = (unsigned char *)malloc(3 * width * height);
    unsigned char *R_plane = src;
    unsigned char *G_plane = src + width * height;
    unsigned char *B_plane = src + 2 * width * height;
    for (int i = 0; i < width * height; i++) {
        rgbPixel[3 * i] = R_plane[i];
        rgbPixel[3 * i + 1] = G_plane[i];
        rgbPixel[3 * i + 2] = B_plane[i];
    }
    memcpy(src, rgbPixel, 3 * width * height);
    free(rgbPixel);
    return BM_SUCCESS;
}

bm_status_t vpss_bgr2rgb(void **src, unsigned char *dst, int width, int height) {
    unsigned char *bgrData = (unsigned char *)src[0];
    unsigned char r, g, b;
    for (int i = 0; i < width * height; i++) {
        b = *bgrData++;
        g = *bgrData++;
        r = *bgrData++;

        *dst++ = r;
        *dst++ = g;
        *dst++ = b;
    }
    return BM_SUCCESS;
}

bm_status_t vpss_crop(unsigned char *src,
                      unsigned char *dst,
                      int srcWidth,
                      int srcHeight,
                      bmcv_rect_t *input_crop_rect) {
    // Boundary checks
    if (input_crop_rect->start_x + input_crop_rect->crop_w > srcWidth ||
        input_crop_rect->start_y + input_crop_rect->crop_h > srcHeight) {
        printf("Error: Crop region exceeds the boundaries of src\n");
        return BM_ERR_PARAM;
    }

    // Pointers for navigation through src and dst
    unsigned char *srcPtr =
        src + (input_crop_rect->start_y * srcWidth + input_crop_rect->start_x) * 3;
    unsigned char *dstPtr = dst;

    for (int j = 0; j < input_crop_rect->crop_h; j++) {
        memcpy(dstPtr, srcPtr, input_crop_rect->crop_w * 3);
        dstPtr += input_crop_rect->crop_w * 3;
        srcPtr += srcWidth * 3;
    }
    return BM_SUCCESS;
}

bm_status_t vpss_resize(unsigned char *src,
                        unsigned char *dst,
                        int in_w,
                        int in_h,
                        int out_w,
                        int out_h,
                        bmcv_resize_algorithm algorithm) {
    resize_image src_resize = {.data = src,
                               .height = in_h,
                               .width = in_w,
                               .step = 3 * in_w * sizeof(unsigned char),
                               .elemSize = 3 * sizeof(unsigned char)};
    resize_image dst_resize = {.data = dst,
                               .height = out_h,
                               .width = out_w,
                               .step = 3 * out_w * sizeof(unsigned char),
                               .elemSize = 3 * sizeof(unsigned char)};
    switch (algorithm) {
        case BMCV_INTER_NEAREST:
            Resize_NN(src_resize, dst_resize);
            break;
        case BMCV_INTER_LINEAR:
            Resize_linear(src_resize, dst_resize);
            break;

        case BMCV_INTER_BICUBIC:
            resize_cubic(src_resize, dst_resize);
            break;
        default:
            return BM_NOT_SUPPORTED;
    }
    return BM_SUCCESS;
}

bm_status_t vpss_border(unsigned char *src, int in_w, int in_h, bmcv_border *border_param) {
    for (int n = 0; n < border_param->border_num; n++) {
        border_t current_border = border_param->border_cfg[n];
        if (!current_border.rect_border_enable) continue;

        // Boundary processing
        if (current_border.st_x > in_w || current_border.st_y > in_h) {
            printf(
                "Error: Rectangle start point is outside the image! (%d, %d) in an image of size "
                "(%d, %d)\n",
                current_border.st_x, current_border.st_y, in_w, in_h);
            return BM_ERR_PARAM;
        }
        if (current_border.thickness > MIN(current_border.width, current_border.height) / 2)
            continue;
        current_border.height = MIN(current_border.height, in_h - current_border.st_y);
        current_border.width = MIN(current_border.width, in_w - current_border.st_x);

        for (int j = current_border.st_y; j <= current_border.st_y + current_border.height; j++) {
            for (int i = current_border.st_x; i <= current_border.st_x + current_border.width;
                 i++) {
                if (i >= in_w || j >= in_h) continue;

                int isBorder =
                    (i < current_border.st_x + current_border.thickness ||
                     i > current_border.st_x + current_border.width - current_border.thickness ||
                     j < current_border.st_y + current_border.thickness ||
                     j > current_border.st_y + current_border.height - current_border.thickness);

                if (isBorder) {
                    int index = (j * in_w + i) * 3;
                    src[index] = current_border.value_r;
                    src[index + 1] = current_border.value_g;
                    src[index + 2] = current_border.value_b;
                }
            }
        }
    }
    return BM_SUCCESS;
}

bm_status_t vpss_coverx(unsigned char *src, int in_w, int in_h, coverex_cfg *convert_to_attr) {
    for (int n = 0; n < convert_to_attr->cover_num; n++) {
        struct rgn_coverex_param current_cover = convert_to_attr->coverex_param[n];

        // Boundary processing
        if (!current_cover.enable) continue;
        if (current_cover.rect.left > in_w || current_cover.rect.top > in_h) {
            printf(
                "Error: Rectangle start point is outside the image! (%d, %d) in an image of size "
                "(%d, %d)\n",
                current_cover.rect.left, current_cover.rect.top, in_w, in_h);
            return BM_ERR_PARAM;
        }

        unsigned char r = (current_cover.color >> 16) & 0xFF;
        unsigned char g = (current_cover.color >> 8) & 0xFF;
        unsigned char b = current_cover.color & 0xFF;

        for (int j = current_cover.rect.top;
             j <= current_cover.rect.top + current_cover.rect.height; j++) {
            for (int i = current_cover.rect.left;
                 i <= current_cover.rect.left + current_cover.rect.width; i++) {
                if (i >= in_w || j >= in_h) continue;

                int index = (j * in_w + i) * 3;
                src[index] = r;
                src[index + 1] = g;
                src[index + 2] = b;
            }
        }
    }
    return BM_SUCCESS;
}

bm_status_t vpss_gop(unsigned char *src, int in_w, int in_h, bmcv_rgn_cfg *gop_attr) {
    for (int n = 0; n < gop_attr->rgn_num; n++) {
        struct rgn_param current_rgn = gop_attr->param[n];

        if (current_rgn.fmt != RGN_FMT_ARGB8888) continue;  // only for RGBA8888

        unsigned char *overlay = (unsigned char *)(current_rgn.phy_addr);
        for (int y = 0; y < current_rgn.rect.height; y++) {
            for (int x = 0; x < current_rgn.rect.width; x++) {
                int dst_x = current_rgn.rect.left + x;
                int dst_y = current_rgn.rect.top + y;

                if (dst_x < 0 || dst_x >= in_w || dst_y < 0 || dst_y >= in_h) continue;

                unsigned char *overlay_pixel = overlay + y * current_rgn.stride + x * 4;
                unsigned char *src_pixel = src + (dst_y * in_w + dst_x) * 3;

                int alpha = overlay_pixel[3];
                alpha = 2 * alpha - 1;
                alpha = alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha);
                int inv_alpha = 255 - alpha;

                src_pixel[0] = (src_pixel[0] * inv_alpha + overlay_pixel[0] * alpha) / 255;
                src_pixel[1] = (src_pixel[1] * inv_alpha + overlay_pixel[1] * alpha) / 255;
                src_pixel[2] = (src_pixel[2] * inv_alpha + overlay_pixel[2] * alpha) / 255;
            }
        }
    }
    return BM_SUCCESS;
}

bm_status_t vpss_convertto(unsigned char *src,
                           int width,
                           int height,
                           bmcv_convert_to_attr *convert_to_attr) {
    if (!src || !convert_to_attr) {
        return BM_ERR_DATA;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char *pixel = src + (y * width + x) * 3;

            float newVal;
            newVal = (int)round(convert_to_attr->alpha_0 * pixel[0] + convert_to_attr->beta_0);
            pixel[0] = (unsigned char)MIN(MAX(newVal, 0), 255);

            newVal = (int)round(convert_to_attr->alpha_1 * pixel[1] + convert_to_attr->beta_1);
            pixel[1] = (unsigned char)MIN(MAX(newVal, 0), 255);

            newVal = (int)round(convert_to_attr->alpha_2 * pixel[2] + convert_to_attr->beta_2);
            pixel[2] = (unsigned char)MIN(MAX(newVal, 0), 255);
        }
    }

    return BM_SUCCESS;
}

bm_status_t vpss_padding(unsigned char *src,
                         unsigned char *dst,
                         int out_w,
                         int out_h,
                         bmcv_padding_attr_t *padding_attr) {
    if (padding_attr == NULL) {
        memcpy(dst, src, out_w * out_h * 3);
        return BM_SUCCESS;
    }

    // Boundary checks
    if (padding_attr->dst_crop_stx + padding_attr->dst_crop_w > out_w ||
        padding_attr->dst_crop_sty + padding_attr->dst_crop_h > out_h) {
        printf("Invalid padding parameter!\n");
        return BM_ERR_PARAM;
    }

    // filling dst with padding_r, g, b
    for (int i = 0; i < out_w * out_h; i++) {
        dst[3 * i] = padding_attr->padding_r;
        dst[3 * i + 1] = padding_attr->padding_g;
        dst[3 * i + 2] = padding_attr->padding_b;
    }

    for (int j = 0; j < padding_attr->dst_crop_h; j++) {
        for (int i = 0; i < padding_attr->dst_crop_w; i++) {
            int y = padding_attr->dst_crop_sty + j;
            int x = padding_attr->dst_crop_stx + i;
            int index_dst = (y * out_w + x) * 3;
            int index_src = (j * padding_attr->dst_crop_w + i) * 3;
            dst[index_dst] = src[index_src];
            dst[index_dst + 1] = src[index_src + 1];
            dst[index_dst + 2] = src[index_src + 2];
        }
    }

    return BM_SUCCESS;
}

bm_status_t vpss_rgb2bgr(void **dst, unsigned char *src, int width, int height) {
    unsigned char *bgrData = (unsigned char *)dst[0];
    unsigned char r, b, g;
    for (int i = 0; i < width * height; i++) {
        r = *src++;
        g = *src++;
        b = *src++;

        *bgrData++ = b;
        *bgrData++ = g;
        *bgrData++ = r;
    }
    return BM_SUCCESS;
}

bm_status_t vpss_rgb2rgbp(unsigned char *src, int width, int height) {
    int num_pixels = width * height;
    unsigned char *tempR = (unsigned char *)malloc(num_pixels);
    unsigned char *tempG = (unsigned char *)malloc(num_pixels);
    unsigned char *tempB = (unsigned char *)malloc(num_pixels);

    // Extract RGB values to the temporary buffers
    for (int i = 0; i < num_pixels; i++) {
        tempR[i] = src[3 * i];
        tempG[i] = src[3 * i + 1];
        tempB[i] = src[3 * i + 2];
    }

    // Copy back to the source in planar format
    memcpy(src, tempR, num_pixels);
    memcpy(src + num_pixels, tempG, num_pixels);
    memcpy(src + 2 * num_pixels, tempB, num_pixels);

    // Clean up the temporary buffers
    free(tempR);
    free(tempG);
    free(tempB);
    return BM_SUCCESS;
}

bm_status_t vpss_rgbp2yuv(unsigned char *src, int width, int height, bmcv_csc_cfg csc_cfg) {
    unsigned char *R = src;
    unsigned char *G = src + width * height;
    unsigned char *B = src + 2 * width * height;

    unsigned char *Y = src;
    unsigned char *U = src + width * height;
    unsigned char *V = src + 2 * width * height;

    double coefYR, coefYG, coefYB, coefUR, coefUG, coefUB, coefVR, coefVG, coefVB;
    double offsetY = 0, offsetU = 0, offsetV = 0;

    switch (csc_cfg.csc_type) {
        case VPSS_CSC_RGB2YCbCr_BT601:
        case VPSS_CSC_YCbCr2YCbCr_BT601:
            coefYR = 0.2568370271402130, coefYG = 0.5036437166574750, coefYB = 0.0983427856140760, offsetY = 16;
            coefUR = -0.1483362360666210, coefUG = -0.2908794502078890, coefUB = 0.4392156862745100, offsetU = 128;
            coefVR = 0.4392156862745100, coefVG = -0.3674637551088690, coefVB = -0.0717519311656413, offsetV = 128;
            break;

        case VPSS_CSC_RGB2YCbCr_BT709:
        case VPSS_CSC_YCbCr2YCbCr_BT709:
            coefYR = 0.1826193815131790, coefYG = 0.6142036888240730, coefYB = 0.0620004590745127, offsetY = 16;
            coefUR = -0.1006613638136630, coefUG = -0.3385543224608470, coefUB = 0.4392156862745100, offsetU = 128;
            coefVR = 0.4392156862745100, coefVG = -0.3989444541822880, coefVB = -0.0402712320922214, offsetV = 128;
            break;

        case VPSS_CSC_RGB2YPbPr_BT601:
        case VPSS_CSC_YPbPr2YPbPr_BT601:
            coefYR = 0.2990568124235360, coefYG = 0.5864344646011700, coefYB = 0.1145087229752940;
            coefUR = -0.1688649115936980, coefUG = -0.3311350884063020, coefUB = 0.500, offsetU = 128;
            coefVR = 0.500, coefVG = -0.4183181140748280, coefVB = -0.0816818859251720, offsetV = 128;
            break;

        case VPSS_CSC_RGB2YPbPr_BT709:
        case VPSS_CSC_YPbPr2YPbPr_BT709:
            coefYR = 0.2126390058715100, coefYG = 0.7151686787677560, coefYB = 0.0721923153607340;
            coefUR = -0.1145921775557320, coefUG = -0.3854078224442680, coefUB = 0.500, offsetU = 128;
            coefVR = 0.500, coefVG = -0.4541555170378730, coefVB = -0.0458444829621270, offsetV = 128;
            break;

        default:
            return BM_NOT_SUPPORTED;  // Invalid csc_type
    }

    for (int i = 0; i < width * height; i++) {
        double r = R[i];
        double g = G[i];
        double b = B[i];

        double y = coefYR * r + coefYG * g + coefYB * b + offsetY;
        double u = coefUR * r + coefUG * g + coefUB * b + offsetU;
        double v = coefVR * r + coefVG * g + coefVB * b + offsetV;

        // Clipping values to the range 0-255
        Y[i] = y < 0 ? 0 : y > 254.5 ? 255 : (unsigned char)(y + 0.5);
        U[i] = u < 0 ? 0 : u > 254.5 ? 255 : (unsigned char)(u + 0.5);
        V[i] = v < 0 ? 0 : v > 254.5 ? 255 : (unsigned char)(v + 0.5);
    }
    return BM_SUCCESS;
}

bm_status_t vpss_rgb2hsvp(void **dst, unsigned char *src, int width, int height) {
    if (!src || !dst || width <= 0 || height <= 0) {
        return BM_ERR_FAILURE;
    }

    // Assuming dst[0], dst[1], dst[2] are already allocated.
    unsigned char *h_plane = dst[0];
    unsigned char *s_plane = dst[1];
    unsigned char *v_plane = dst[2];

    for (int i = 0; i < width * height; ++i) {
        float r = src[3 * i] / 255.0f;
        float g = src[3 * i + 1] / 255.0f;
        float b = src[3 * i + 2] / 255.0f;

        float max = fmaxf(fmaxf(r, g), b);
        float min = fminf(fminf(r, g), b);
        float delta = max - min;

        float h = 0.0f;
        float s = (max == 0) ? 0.0f : (delta / max);
        float v = max;

        if (delta != 0.0f) {
            if (r == max) {
                h = 30.0f * (g - b) / delta + (g < b ? 180.0f : 0.0f);
            } else if (g == max) {
                h = 30.0f * (b - r) / delta + 60.0f;
            } else if (b == max) {
                h = 30.0f * (r - g) / delta + 120.0f;
            }
        }

        // Store in planes directly
        h_plane[i] = (unsigned char)(h + 0.5f);           // rounding
        s_plane[i] = (unsigned char)(s * 255.0f + 0.5f);  // rounding
        v_plane[i] = (unsigned char)(v * 255.0f + 0.5f);  // rounding
    }

    return BM_SUCCESS;
}

bm_status_t vpss_rgb2hsv180(void **dst, unsigned char *src, int width, int height) {
    if (!src || !dst || width <= 0 || height <= 0) {
        return BM_ERR_FAILURE;
    }

    // Assuming dst[0], dst[1], dst[2] are already allocated.
    unsigned char *hsv_data = dst[0];

    for (int i = 0; i < width * height; ++i) {
        float r = src[3 * i] / 255.0f;
        float g = src[3 * i + 1] / 255.0f;
        float b = src[3 * i + 2] / 255.0f;

        float max = fmaxf(fmaxf(r, g), b);
        float min = fminf(fminf(r, g), b);
        float delta = max - min;

        float h = 0.0f;
        float s = (max == 0) ? 0.0f : (delta / max);
        float v = max;

        if (delta != 0.0f) {
            if (r == max) {
                h = 30.0f * (g - b) / delta + (g < b ? 180.0f : 0.0f);
            } else if (g == max) {
                h = 30.0f * (b - r) / delta + 60.0f;
            } else if (b == max) {
                h = 30.0f * (r - g) / delta + 120.0f;
            }
        }

        // Store in planes directly
        hsv_data[3 * i] = (unsigned char)(h + 0.5f);               // rounding
        hsv_data[3 * i + 1] = (unsigned char)(s * 255.0f + 0.5f);  // rounding
        hsv_data[3 * i + 2] = (unsigned char)(v * 255.0f + 0.5f);  // rounding
    }

    return BM_SUCCESS;
}

bm_status_t vpss_rgb2hsv256(void **dst, unsigned char *src, int width, int height) {
    if (!src || !dst || width <= 0 || height <= 0) {
        return BM_ERR_FAILURE;
    }

    // Assuming dst[0], dst[1], dst[2] are already allocated.
    unsigned char *hsv_data = dst[0];

    for (int i = 0; i < width * height; ++i) {
        float r = src[3 * i] / 255.0f;
        float g = src[3 * i + 1] / 255.0f;
        float b = src[3 * i + 2] / 255.0f;

        float max = fmaxf(fmaxf(r, g), b);
        float min = fminf(fminf(r, g), b);
        float delta = max - min;

        float h = 0.0f;
        float s = (max == 0) ? 0.0f : (delta / max);
        float v = max;

        if (delta != 0.0f) {
            if (r == max) {
                h = 60.0f * (g - b) / delta + (g < b ? 360.0f : 0.0f);
            } else if (g == max) {
                h = 60.0f * (b - r) / delta + 120.0f;
            } else if (b == max) {
                h = 60.0f * (r - g) / delta + 240.0f;
            }
        }

        // Normalize H to the range [0, 255]
        h = (h * 255.0f / 360.0f);

        // Store in planes directly
        hsv_data[3 * i] = (unsigned char)(h + 0.5f);               // rounding
        hsv_data[3 * i + 1] = (unsigned char)(s * 255.0f + 0.5f);  // rounding
        hsv_data[3 * i + 2] = (unsigned char)(v * 255.0f + 0.5f);  // rounding
    }

    return BM_SUCCESS;
}

bm_status_t vpss_downsample(
    void **dst, unsigned char *src, bm_image_format_ext format, int width, int height) {
    unsigned char *u_data = NULL;
    unsigned char *v_data = NULL;
    unsigned char *y_data = NULL;
    unsigned char *y_temp, *u_temp, *v_temp, *uv_data, *yuv_data;

    switch (format) {
        case FORMAT_YUV420P:
            memcpy(dst[0], src, width * height);
            h2v2_downsample(width, height, src + width * height, dst[1]);
            h2v2_downsample(width, height, src + width * height * 2, dst[2]);
            break;

        case FORMAT_YUV422P:
            memcpy(dst[0], src, width * height);
            h2v1_downsample(width, height, src + width * height, dst[1]);
            h2v1_downsample(width, height, src + width * height * 2, dst[2]);
            break;

        case FORMAT_NV12:
            memcpy(dst[0], src, width * height);
            u_data = (unsigned char *)malloc(width * height / 4);
            v_data = (unsigned char *)malloc(width * height / 4);
            h2v2_downsample(width, height, src + width * height, u_data);
            h2v2_downsample(width, height, src + width * height * 2, v_data);

            uv_data = (unsigned char *)dst[1];
            u_temp = u_data, v_temp = v_data;
            for (int i = 0; i < width * height / 4; i++) {
                *(uv_data++) = *(u_temp++);
                *(uv_data++) = *(v_temp++);
            }
            break;

        case FORMAT_NV21:
            memcpy(dst[0], src, width * height);
            u_data = (unsigned char *)malloc(width * height / 4);
            v_data = (unsigned char *)malloc(width * height / 4);
            h2v2_downsample(width, height, src + width * height, u_data);
            h2v2_downsample(width, height, src + width * height * 2, v_data);

            uv_data = (unsigned char *)dst[1];
            u_temp = u_data, v_temp = v_data;
            for (int i = 0; i < width * height / 4; i++) {
                *(uv_data++) = *(v_temp++);
                *(uv_data++) = *(u_temp++);
            }
            break;

        case FORMAT_NV16:
            memcpy(dst[0], src, width * height);
            u_data = (unsigned char *)malloc(width * height / 2);
            v_data = (unsigned char *)malloc(width * height / 2);
            h2v1_downsample(width, height, src + width * height, u_data);
            h2v1_downsample(width, height, src + width * height * 2, v_data);

            uv_data = (unsigned char *)dst[1];
            u_temp = u_data, v_temp = v_data;
            for (int i = 0; i < width * height / 2; i++) {
                *(uv_data++) = *(u_temp++);
                *(uv_data++) = *(v_temp++);
            }
            break;

        case FORMAT_NV61:
            memcpy(dst[0], src, width * height);
            u_data = (unsigned char *)malloc(width * height / 2);
            v_data = (unsigned char *)malloc(width * height / 2);
            h2v1_downsample(width, height, src + width * height, u_data);
            h2v1_downsample(width, height, src + width * height * 2, v_data);

            uv_data = (unsigned char *)dst[1];
            u_temp = u_data, v_temp = v_data;
            for (int i = 0; i < width * height / 2; i++) {
                *(uv_data++) = *(v_temp++);
                *(uv_data++) = *(u_temp++);
            }
            break;

        case FORMAT_YUV422_YUYV:
            y_data = (unsigned char *)malloc(width * height);
            u_data = (unsigned char *)malloc(width * height / 2);
            v_data = (unsigned char *)malloc(width * height / 2);
            memcpy(y_data, src, width * height);
            h2v1_downsample(width, height, src + width * height, u_data);
            h2v1_downsample(width, height, src + width * height * 2, v_data);

            yuv_data = (unsigned char *)dst[0];
            y_temp = y_data, u_temp = u_data, v_temp = v_data;
            for (int i = 0; i < width * height / 2; i++) {
                *(yuv_data++) = *(y_temp++);
                *(yuv_data++) = *(u_temp++);
                *(yuv_data++) = *(y_temp++);
                *(yuv_data++) = *(v_temp++);
            }
            break;

        case FORMAT_YUV422_YVYU:
            y_data = (unsigned char *)malloc(width * height);
            u_data = (unsigned char *)malloc(width * height / 2);
            v_data = (unsigned char *)malloc(width * height / 2);
            memcpy(y_data, src, width * height);
            h2v1_downsample(width, height, src + width * height, u_data);
            h2v1_downsample(width, height, src + width * height * 2, v_data);

            yuv_data = (unsigned char *)dst[0];
            y_temp = y_data, u_temp = u_data, v_temp = v_data;
            for (int i = 0; i < width * height / 2; i++) {
                *(yuv_data++) = *(y_temp++);
                *(yuv_data++) = *(v_temp++);
                *(yuv_data++) = *(y_temp++);
                *(yuv_data++) = *(u_temp++);
            }
            break;

        case FORMAT_YUV422_UYVY:
            y_data = (unsigned char *)malloc(width * height);
            u_data = (unsigned char *)malloc(width * height / 2);
            v_data = (unsigned char *)malloc(width * height / 2);
            memcpy(y_data, src, width * height);
            h2v1_downsample(width, height, src + width * height, u_data);
            h2v1_downsample(width, height, src + width * height * 2, v_data);

            yuv_data = (unsigned char *)dst[0];
            y_temp = y_data, u_temp = u_data, v_temp = v_data;
            for (int i = 0; i < width * height / 2; i++) {
                *(yuv_data++) = *(u_temp++);
                *(yuv_data++) = *(y_temp++);
                *(yuv_data++) = *(v_temp++);
                *(yuv_data++) = *(y_temp++);
            }
            break;

        case FORMAT_YUV422_VYUY:
            y_data = (unsigned char *)malloc(width * height);
            u_data = (unsigned char *)malloc(width * height / 2);
            v_data = (unsigned char *)malloc(width * height / 2);
            memcpy(y_data, src, width * height);
            h2v1_downsample(width, height, src + width * height, u_data);
            h2v1_downsample(width, height, src + width * height * 2, v_data);

            yuv_data = (unsigned char *)dst[0];
            y_temp = y_data, u_temp = u_data, v_temp = v_data;
            for (int i = 0; i < width * height / 2; i++) {
                *(yuv_data++) = *(v_temp++);
                *(yuv_data++) = *(y_temp++);
                *(yuv_data++) = *(u_temp++);
                *(yuv_data++) = *(y_temp++);
            }
            break;

        default:
            break;
    }
    if (y_data) free(y_data);
    if (u_data) free(u_data);
    if (v_data) free(v_data);
    return BM_SUCCESS;
}

bm_status_t vpss_u8_to_datatype(bm_image *output) {
    int datatype_byte_size = 1;
    switch (output->image_format) {
        case DATA_TYPE_EXT_FLOAT32:
            datatype_byte_size = 4;
            break;
        case DATA_TYPE_EXT_FP16:
        case DATA_TYPE_EXT_BF16:
            datatype_byte_size = 2;
            break;
        case DATA_TYPE_EXT_1N_BYTE:
        case DATA_TYPE_EXT_1N_BYTE_SIGNED:
            datatype_byte_size = 1;
            break;
        default:
            datatype_byte_size = 1;
    }

    int image_byte_size[4] = {0};
    int u8_image_byte_size[4] = {0};
    int plane_num = output->image_private->plane_num;
    int total_u8_byte_size = 0;

    bm_image_get_byte_size(*output, image_byte_size);
    for (int i = 0; i < plane_num; i++) {
        u8_image_byte_size[i] = image_byte_size[i] / datatype_byte_size;
        total_u8_byte_size += u8_image_byte_size[i];
    }

    unsigned char *output_data = (unsigned char *)malloc(total_u8_byte_size);
    void *output_addr[3] = {output->image_private->data[0].u.system.system_addr,
                            output->image_private->data[1].u.system.system_addr,
                            output->image_private->data[2].u.system.system_addr};

    int offset = 0;
    for (int i = 0; i < plane_num; i++) {
        memcpy(output_data + offset, output_addr[i], u8_image_byte_size[i]);
        offset += u8_image_byte_size[i];
    }

    switch (output->data_type) {
        case DATA_TYPE_EXT_FLOAT32: {
            float *float_data = (float *)malloc(total_u8_byte_size * sizeof(float));
            convert_u8_to_float32(output_data, float_data, total_u8_byte_size);
            offset = 0;
            for (int i = 0; i < output->image_private->plane_num; i++) {
                memcpy(output_addr[i], float_data + offset, image_byte_size[i]);
                offset += image_byte_size[i];
            }
            free(float_data);
            break;
        }
        case DATA_TYPE_EXT_1N_BYTE: {
            // No conversion needed since it's already in u8 format
            break;
        }
        case DATA_TYPE_EXT_1N_BYTE_SIGNED: {
            for (int i = 0; i < total_u8_byte_size; i++) {
                if (output_data[i] > 127) {
                    output_data[i] = 127;
                }
            }
            offset = 0;
            for (int i = 0; i < output->image_private->plane_num; i++) {
                memcpy(output_addr[i], output_data + offset, image_byte_size[i]);
                offset += image_byte_size[i];
            }
            break;
        }
        case DATA_TYPE_EXT_FP16: {
            unsigned short *fp16_data =
                (unsigned short *)malloc(total_u8_byte_size * sizeof(unsigned short));
            convert_u8_to_fp16(output_data, fp16_data, total_u8_byte_size);
            offset = 0;
            for (int i = 0; i < plane_num; i++) {
                memcpy(output_addr[i], fp16_data + offset, image_byte_size[i]);
                offset += image_byte_size[i];
            }
            free(fp16_data);
            break;
        }
        case DATA_TYPE_EXT_BF16: {
            unsigned short *bf16_data =
                (unsigned short *)malloc(total_u8_byte_size * sizeof(unsigned short));
            convert_u8_to_bf16(output_data, bf16_data, total_u8_byte_size);
            offset = 0;
            for (int i = 0; i < plane_num; i++) {
                memcpy(output_addr[i], bf16_data + offset, image_byte_size[i]);
                offset += image_byte_size[i];
            }
            free(bf16_data);
            break;
        }
        default:
            printf("This data type isn't supported!\n");
            break;
    }
    free(output_data);
    return BM_SUCCESS;
}

bm_status_t bm_vpss_cmodel(bm_image *input,
                           bm_image *output,
                           bmcv_rect_t *input_crop_rect,
                           bmcv_padding_attr_t *padding_attr,
                           bmcv_resize_algorithm algorithm,
                           csc_type_t csc_type,
                           csc_matrix_t *matrix,
                           bmcv_convert_to_attr *convert_to_attr,
                           bmcv_border *border_param,
                           coverex_cfg *coverex_param,
                           bmcv_rgn_cfg *gop_attr) {
    bm_status_t ret = BM_SUCCESS;
    bmcv_csc_cfg csc_cfg;
    vpss_csctype_to_csc_cfg(input, output, &csc_type, &csc_cfg);
    int in_w = input->width, in_h = input->height;
    int crop_w = input_crop_rect->crop_w, crop_h = input_crop_rect->crop_h;
    int out_w = output->width, out_h = output->height;

    // if padding, dst_w < out_w, dst_h < out_h; if not, dst_w = out_w, dst_h = out_h
    int dst_w = output->width, dst_h = output->height;
    if (padding_attr != NULL && padding_attr->if_memset != 0) {
        dst_w = padding_attr->dst_crop_w;
        dst_h = padding_attr->dst_crop_h;
    }

    unsigned char *input_ = (unsigned char *)malloc(in_w * in_h * 3);
    unsigned char *crop_ = (unsigned char *)malloc(crop_w * crop_h * 3);
    unsigned char *resize_ = (unsigned char *)malloc(dst_w * dst_h * 3);
    unsigned char *output_ = (unsigned char *)malloc(out_w * out_h * 3);
    // upsample(to yuv444)
    void *input_addr[3] = {input->image_private->data[0].u.system.system_addr,
                           input->image_private->data[1].u.system.system_addr,
                           input->image_private->data[2].u.system.system_addr};
    void *output_addr[3] = {output->image_private->data[0].u.system.system_addr,
                            output->image_private->data[1].u.system.system_addr,
                            output->image_private->data[2].u.system.system_addr};
    if (input->image_format == FORMAT_YUV420P || input->image_format == FORMAT_YUV422P ||
        input->image_format == FORMAT_NV12 || input->image_format == FORMAT_NV21 ||
        input->image_format == FORMAT_NV16 || input->image_format == FORMAT_NV61 ||
        input->image_format == FORMAT_YUV422_YUYV || input->image_format == FORMAT_YUV422_YVYU ||
        input->image_format == FORMAT_YUV422_UYVY || input->image_format == FORMAT_YUV422_VYUY) {
        vpss_upsample(input_addr, input_, input->image_format, in_w, in_h, csc_cfg.is_fancy);
    } else if (input->image_format == FORMAT_YUV444P) {
        memcpy(input_, input_addr[0], in_w * in_h);
        memcpy(input_ + in_w * in_h, input_addr[1], in_w * in_h);
        memcpy(input_ + in_w * in_h * 2, input_addr[2], in_w * in_h);
    }

    // yuv/bgrp2rgbp
    if (input->image_format == FORMAT_YUV420P || input->image_format == FORMAT_YUV422P ||
        input->image_format == FORMAT_NV12 || input->image_format == FORMAT_NV21 ||
        input->image_format == FORMAT_NV16 || input->image_format == FORMAT_NV61 ||
        input->image_format == FORMAT_YUV422_YUYV || input->image_format == FORMAT_YUV422_YVYU ||
        input->image_format == FORMAT_YUV422_UYVY || input->image_format == FORMAT_YUV422_VYUY ||
        input->image_format == FORMAT_YUV444P) {
        vpss_yuv2rgbp(input_, in_w, in_h, csc_cfg);
    } else if (input->image_format == FORMAT_RGB_PLANAR) {
        memcpy(input_, input_addr[0], in_w * in_h * 3);
    } else if (input->image_format == FORMAT_RGBP_SEPARATE) {
        memcpy(input_, input_addr[0], in_w * in_h);
        memcpy(input_ + in_w * in_h, input_addr[1], in_w * in_h);
        memcpy(input_ + in_w * in_h * 2, input_addr[2], in_w * in_h);
    } else if (input->image_format == FORMAT_BGR_PLANAR) {
        memcpy(input_ + in_w * in_h * 2, input_addr[0], in_w * in_h);
        memcpy(input_ + in_w * in_h, input_addr[0] + in_w * in_h, in_w * in_h);
        memcpy(input_, input_addr[0] + in_w * in_h * 2, in_w * in_h);
    } else if (input->image_format == FORMAT_BGRP_SEPARATE) {
        memcpy(input_ + in_w * in_h * 2, input_addr[0], in_w * in_h);
        memcpy(input_ + in_w * in_h, input_addr[1], in_w * in_h);
        memcpy(input_, input_addr[2], in_w * in_h);
    }

    // rgbp2rgb
    if (input->image_format == FORMAT_RGB_PLANAR || input->image_format == FORMAT_RGBP_SEPARATE ||
        input->image_format == FORMAT_BGR_PLANAR || input->image_format == FORMAT_BGRP_SEPARATE ||
        input->image_format == FORMAT_YUV420P || input->image_format == FORMAT_YUV422P ||
        input->image_format == FORMAT_NV12 || input->image_format == FORMAT_NV21 ||
        input->image_format == FORMAT_NV16 || input->image_format == FORMAT_NV61 ||
        input->image_format == FORMAT_YUV422_YUYV || input->image_format == FORMAT_YUV422_YVYU ||
        input->image_format == FORMAT_YUV422_UYVY || input->image_format == FORMAT_YUV422_VYUY ||
        input->image_format == FORMAT_YUV444P) {
        vpss_rgbp2rgb(input_, in_w, in_h);
    }

    if (input->image_format == FORMAT_BGR_PACKED) {
        vpss_bgr2rgb(input_addr, input_, in_w, in_h);
    }

    if (input->image_format == FORMAT_RGB_PACKED) {
        memcpy(input_, input_addr[0], in_w * in_h * 3);
    }

    // crop
    vpss_crop(input_, crop_, in_w, in_h, input_crop_rect);

    // resize
    vpss_resize(crop_, resize_, crop_w, crop_h, dst_w, dst_h, algorithm);

    // border
    vpss_border(resize_, dst_w, dst_h, border_param);

    // cover
    vpss_coverx(resize_, dst_w, dst_h, coverex_param);

    // gop
    vpss_gop(resize_, dst_w, dst_h, gop_attr);

    // convertto
    vpss_convertto(resize_, dst_w, dst_h, convert_to_attr);

    // padding
    vpss_padding(resize_, output_, out_w, out_h, padding_attr);

    if (output->image_format == FORMAT_RGB_PACKED) {
        memcpy(output_addr[0], output_, out_w * out_h * 3);
    }

    if (output->image_format == FORMAT_BGR_PACKED) {
        vpss_rgb2bgr(output_addr, output_, out_w, out_h);
    }

    // rgb2rgbp
    if (output->image_format == FORMAT_RGB_PLANAR || output->image_format == FORMAT_RGBP_SEPARATE ||
        output->image_format == FORMAT_BGR_PLANAR || output->image_format == FORMAT_BGRP_SEPARATE ||
        output->image_format == FORMAT_YUV420P || output->image_format == FORMAT_YUV422P ||
        output->image_format == FORMAT_NV12 || output->image_format == FORMAT_NV21 ||
        output->image_format == FORMAT_NV16 || output->image_format == FORMAT_NV61 ||
        output->image_format == FORMAT_YUV422_YUYV || output->image_format == FORMAT_YUV422_YVYU ||
        output->image_format == FORMAT_YUV422_UYVY || output->image_format == FORMAT_YUV422_VYUY ||
        output->image_format == FORMAT_YUV444P) {
        vpss_rgb2rgbp(output_, out_w, out_h);
    }

    // yuv/bgrp2rgbp
    if (output->image_format == FORMAT_YUV420P || output->image_format == FORMAT_YUV422P ||
        output->image_format == FORMAT_NV12 || output->image_format == FORMAT_NV21 ||
        output->image_format == FORMAT_NV16 || output->image_format == FORMAT_NV61 ||
        output->image_format == FORMAT_YUV422_YUYV || output->image_format == FORMAT_YUV422_YVYU ||
        output->image_format == FORMAT_YUV422_UYVY || output->image_format == FORMAT_YUV422_VYUY ||
        output->image_format == FORMAT_YUV444P) {
        vpss_rgbp2yuv(output_, out_w, out_h, csc_cfg);
    } else if (output->image_format == FORMAT_RGB_PLANAR) {
        memcpy(output_addr[0], output_, out_w * out_h * 3);
    } else if (output->image_format == FORMAT_RGBP_SEPARATE) {
        memcpy(output_addr[0], output_, out_w * out_h);
        memcpy(output_addr[1], output_ + out_w * out_h, out_w * out_h);
        memcpy(output_addr[2], output_ + out_w * out_h * 2, out_w * out_h);
    } else if (output->image_format == FORMAT_BGR_PLANAR) {
        memcpy(output_addr[0], output_ + out_w * out_h * 2, out_w * out_h);
        memcpy(output_addr[0] + out_w * out_h, output_ + out_w * out_h, out_w * out_h);
        memcpy(output_addr[0] + out_w * out_h * 2, output_, out_w * out_h);
    } else if (output->image_format == FORMAT_BGRP_SEPARATE) {
        memcpy(output_addr[0], output_, out_w * out_h);
        memcpy(output_addr[1], output_ + out_w * out_h, out_w * out_h);
        memcpy(output_addr[2], output_ + out_w * out_h * 2, out_w * out_h);
    }

    if (output->image_format == FORMAT_YUV420P || output->image_format == FORMAT_YUV422P ||
        output->image_format == FORMAT_NV12 || output->image_format == FORMAT_NV21 ||
        output->image_format == FORMAT_NV16 || output->image_format == FORMAT_NV61 ||
        output->image_format == FORMAT_YUV422_YUYV || output->image_format == FORMAT_YUV422_YVYU ||
        output->image_format == FORMAT_YUV422_UYVY || output->image_format == FORMAT_YUV422_VYUY) {
        vpss_downsample(output_addr, output_, output->image_format, out_w, out_h);
    } else if (output->image_format == FORMAT_YUV444P) {
        memcpy(output_addr[0], output_, out_w * out_h);
        memcpy(output_addr[1], output_ + out_w * out_h, out_w * out_h);
        memcpy(output_addr[2], output_ + out_w * out_h * 2, out_w * out_h);
    }

    // rgb2hsvp
    if (output->image_format == FORMAT_HSV_PLANAR) {
        vpss_rgb2hsvp(output_addr, output_, out_w, out_h);
    }
    if (output->image_format == FORMAT_HSV180_PACKED) {
        vpss_rgb2hsv180(output_addr, output_, out_w, out_h);
    }
    if (output->image_format == FORMAT_HSV256_PACKED) {
        vpss_rgb2hsv256(output_addr, output_, out_w, out_h);
    }

    // datatype
    if (output->data_type != DATA_TYPE_EXT_1N_BYTE) vpss_u8_to_datatype(output);

    free(input_);
    free(crop_);
    free(resize_);
    free(output_);
    return ret;
}
