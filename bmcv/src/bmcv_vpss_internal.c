#include <stdio.h>
#include <stdlib.h>
#include "bmcv_a2_vpss_ext.h"
#include "bmcv_internal.h"

#define BITMAP_1BIT 1
#define BITMAP_8BIT 0

bm_status_t bmcv_image_vpp_basic(
	bm_handle_t           handle,
	int                   in_img_num,
	bm_image*             input,
	bm_image*             output,
	int*                  crop_num_vec,
	bmcv_rect_t*          crop_rect,
	bmcv_padding_attr_t*  padding_attr,
	bmcv_resize_algorithm algorithm,
	csc_type_t            csc_type,
	csc_matrix_t*         matrix)
{
	unsigned int chipid = BM1688;
	bm_status_t ret = BM_SUCCESS;

#ifndef _FPGA
	ret = bm_get_chipid(handle, &chipid);
	if (BM_SUCCESS != ret)
		return ret;
#endif

	switch(chipid)
	{
#ifndef USING_CMODEL
		case BM1688_PREV:
		case BM1688:
			ret = bm_vpss_basic(handle, in_img_num, input,
				output, crop_num_vec, crop_rect, padding_attr, algorithm, csc_type, matrix);
			break;
#endif

		default:
			ret = BM_ERR_NOFEATURE;
			break;
	}

	return ret;
}

bm_status_t bmcv_image_vpp_convert(
	bm_handle_t             handle,
	int                     output_num,
	bm_image                input,
	bm_image*               output,
	bmcv_rect_t*            crop_rect,
	bmcv_resize_algorithm   algorithm)
{
	unsigned int chipid = BM1688;
	bm_status_t ret = BM_SUCCESS;

#ifndef _FPGA
	ret = bm_get_chipid(handle, &chipid);
	if (BM_SUCCESS != ret)
		return ret;
#endif

	switch(chipid)
	{
#ifndef USING_CMODEL
		case BM1688_PREV:
		case BM1688:
			ret = bm_vpss_convert_internal(
				handle, output_num, input, output, crop_rect, algorithm, NULL);

			break;
#endif
		default:
			ret = BM_ERR_NOFEATURE;
			break;
	}

	return ret;
}

bm_status_t bmcv_image_vpp_csc_matrix_convert(
	bm_handle_t           handle,
	int                   output_num,
	bm_image              input,
	bm_image*             output,
	csc_type_t            csc,
	csc_matrix_t*         matrix,
	bmcv_resize_algorithm algorithm,
	bmcv_rect_t*          crop_rect)
{
	unsigned int chipid = BM1688;
	bm_status_t ret = BM_SUCCESS;

#ifndef _FPGA
	ret = bm_get_chipid(handle, &chipid);
	if (BM_SUCCESS != ret)
		return ret;
#endif

	switch(chipid)
	{
#ifndef USING_CMODEL
		case BM1688_PREV:
		case BM1688:
			ret = bm_vpss_csc_matrix_convert(
				handle, output_num, input, output, csc, matrix, algorithm, crop_rect);
			break;
#endif
		default:
			ret = BM_ERR_NOFEATURE;
			break;
	}

	return ret;
}

bm_status_t bmcv_image_storage_convert_with_csctype(
	bm_handle_t      handle,
	int              image_num,
	bm_image*        input_,
	bm_image*        output_,
	csc_type_t       csc_type)
{

	bm_status_t ret = BM_SUCCESS;
	unsigned int chipid = BM1688;

#ifndef _FPGA
	ret = bm_get_chipid(handle, &chipid);
	if (BM_SUCCESS != ret)
		return ret;
#endif

	switch(chipid)
	{
		case BM1688_PREV:
		case BM1688:
			ret = bm_vpss_storage_convert(handle, image_num, input_, output_, csc_type);
			break;

		default:
			ret = BM_ERR_NOFEATURE;
			break;
	}

	return ret;

}

bm_status_t bmcv_image_storage_convert(
	bm_handle_t      handle,
	int              image_num,
	bm_image*        input_,
	bm_image*        output_)
{

	bm_status_t ret = BM_SUCCESS;
	csc_type_t csc_type = CSC_MAX_ENUM;

	ret = bmcv_image_storage_convert_with_csctype(handle, image_num, input_, output_, csc_type);
	return ret;
}

bm_status_t bmcv_image_vpp_convert_padding(
	bm_handle_t             handle,
	int                     output_num,
	bm_image                input,
	bm_image*               output,
	bmcv_padding_attr_t*    padding_attr,
	bmcv_rect_t*            crop_rect,
	bmcv_resize_algorithm   algorithm)
{
	unsigned int chipid = BM1688;
	bm_status_t ret = BM_SUCCESS;

#ifndef _FPGA
	ret = bm_get_chipid(handle, &chipid);
	if (BM_SUCCESS != ret)
		return ret;
#endif

	switch(chipid)
	{
		case BM1688_PREV:
		case BM1688:
			ret = bm_vpss_cvt_padding(handle, output_num, input, output,\
				padding_attr, crop_rect, algorithm, NULL);
			break;

		default:
			ret = BM_ERR_NOFEATURE;
			break;
	}

	return ret;
}

bm_status_t bmcv_image_resize(
        bm_handle_t          handle,
        int                  input_num,
        bmcv_resize_image    resize_attr[],
        bm_image *           input,
        bm_image *           output) {
	bm_status_t ret = BM_SUCCESS;
	bmcv_padding_attr_t padding_attr;
	bmcv_resize_algorithm algorithm;
	bmcv_rect_t crop_rect;
	padding_attr.dst_crop_stx = 0;
	padding_attr.dst_crop_sty = 0;

	for (int i = 0; i < input_num; i++) {
		padding_attr.dst_crop_w = resize_attr[i].resize_img_attr->out_width;
		padding_attr.dst_crop_h = resize_attr[i].resize_img_attr->out_height;
		padding_attr.if_memset = resize_attr[i].stretch_fit;
		padding_attr.padding_r = resize_attr[i].padding_r;
		padding_attr.padding_g = resize_attr[i].padding_g;
		padding_attr.padding_b = resize_attr[i].padding_b;
		algorithm = resize_attr[i].interpolation;
		crop_rect.start_x = resize_attr[i].resize_img_attr->start_x;
		crop_rect.start_y = resize_attr[i].resize_img_attr->start_y;
		crop_rect.crop_w = resize_attr[i].resize_img_attr->in_width;
		crop_rect.crop_h = resize_attr[i].resize_img_attr->in_height;
		ret = bmcv_image_vpp_convert_padding(handle, 1, input[i], output+i, &padding_attr, &crop_rect, algorithm);
	}

	return ret;
};

bm_status_t bmcv_image_draw_rectangle(
	bm_handle_t   handle,
	bm_image      image,
	int           rect_num,
	bmcv_rect_t * rects,
	int           line_width,
	unsigned char r,
	unsigned char g,
	unsigned char b)
{
	unsigned int chipid = BM1688;
	bm_status_t ret = BM_SUCCESS;

#ifndef _FPGA
	ret = bm_get_chipid(handle, &chipid);
	if (BM_SUCCESS != ret)
		return ret;
#endif

	u8 draw_val[3] = {r, g, b};
	switch(chipid)
	{
		case BM1688_PREV:
		case BM1688:
			ret = bm_vpss_draw_rectangle(handle, image, rect_num, rects, line_width, draw_val[0], draw_val[1], draw_val[2]);
			break;

		default:
			ret = BM_ERR_NOFEATURE;
			break;
	}

	return ret;
}

bm_status_t bmcv_image_csc_convert_to(
	bm_handle_t             handle,
	int                     img_num,
	bm_image*               input,
	bm_image*               output,
	int*                    crop_num_vec,
	bmcv_rect_t*            crop_rect,
	bmcv_padding_attr_t*    padding_attr,
	bmcv_resize_algorithm   algorithm,
	csc_type_t              csc_type,
	csc_matrix_t*           matrix,
	bmcv_convert_to_attr*   convert_to_attr)
{
	unsigned int chipid = BM1688;
	bm_status_t ret = BM_SUCCESS;

#ifndef _FPGA
	ret = bm_get_chipid(handle, &chipid);
	if (BM_SUCCESS != ret)
		return ret;
#endif

	switch(chipid)
	{
		case BM1688_PREV:
		case BM1688:
			ret = bm_vpss_csc_convert_to(handle, img_num, input,
					output, crop_num_vec, crop_rect, padding_attr, algorithm, csc_type, matrix, convert_to_attr);
			break;

		default:
			ret = BM_ERR_NOFEATURE;
			break;
	}

	return ret;
}

bm_status_t bmcv_image_copy_to_vpss(
	bm_handle_t         handle,
	bmcv_copy_to_atrr_t copy_to_attr,
	bm_image            input,
	bm_image            output)
{
	unsigned int chipid = BM1688;
	bm_status_t ret = BM_SUCCESS;

#ifndef _FPGA
	ret = bm_get_chipid(handle, &chipid);
	if (BM_SUCCESS != ret)
		return ret;
#endif

	unsigned int data_type = input.data_type;

	switch(chipid)
	{
		case BM1688_PREV:
		case BM1688:
			if ((data_type == DATA_TYPE_EXT_1N_BYTE)) {
				ret = bm_vpss_copy_to(handle, copy_to_attr, input, output);
			} else if ((data_type == DATA_TYPE_EXT_FLOAT32) || (data_type == DATA_TYPE_EXT_1N_BYTE_SIGNED)) {
				// ret = bmcv_image_copy_to_(handle, copy_to_attr, input, output);
				bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
				"not support, %s: %s: %d\n",
				filename(__FILE__), __func__, __LINE__);
				ret = BM_ERR_DATA;
			}
			break;

		default:
			ret = BM_ERR_NOFEATURE;
			break;
	}
	return ret;
}

bm_status_t bmcv_image_vpp_stitch(
	bm_handle_t           handle,
	int                   input_num,
	bm_image*             input,
	bm_image              output,
	bmcv_rect_t*          dst_crop_rect,
	bmcv_rect_t*          src_crop_rect,
	bmcv_resize_algorithm algorithm)
{
	unsigned int chipid = BM1688;
	bm_status_t ret = BM_SUCCESS;

#ifndef _FPGA
	ret = bm_get_chipid(handle, &chipid);
	if (BM_SUCCESS != ret)
		return ret;
#endif

	switch(chipid)
	{
		case BM1688_PREV:
		case BM1688:
			ret = bm_vpss_stitch(handle,
				input_num, input, output, dst_crop_rect, src_crop_rect, algorithm);
			break;

		default:
			ret = BM_ERR_NOFEATURE;
			break;
	}

	return ret;
}

static bm_status_t bmcv_image_mosaic_check(
	bm_handle_t           handle,
	int                   mosaic_num,
	bm_image              input,
	bmcv_rect_t *         crop_rect,
	int                   is_expand)
{
	unsigned char align_fac = (is_full_image(input.image_format)) ? MOSAIC_SIZE : (MOSAIC_SIZE*2);
	if (is_expand != 0 && is_expand != 1) {
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "is_expand out of range, is_expand=%d, %s: %s: %d\n",
						is_expand, filename(__FILE__), __func__, __LINE__);
		return BM_ERR_FAILURE;
	}
	if (mosaic_num > 512 || mosaic_num < 1) {
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "mosaic_num out of range, mosaic_num=%d, %s: %s: %d\n",
						mosaic_num, filename(__FILE__), __func__, __LINE__);
		return BM_ERR_FAILURE;
	}
	if (handle == NULL) {
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "handle is nullptr");
		return BM_ERR_FAILURE;
	}
	if (input.image_private == NULL) {
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "input is nullptr");
		return BM_ERR_FAILURE;
	}
	for (int i=0; i<mosaic_num; i++) {
		crop_rect[i].crop_w = crop_rect[i].crop_w + (is_expand << 4);
		crop_rect[i].crop_h = crop_rect[i].crop_h + (is_expand << 4);
		crop_rect[i].start_x = crop_rect[i].start_x - (is_expand << 3);
		crop_rect[i].start_y = crop_rect[i].start_y - (is_expand << 3);
		if (crop_rect[i].start_x < 0) {
			crop_rect[i].crop_w = crop_rect[i].crop_w + crop_rect[i].start_x;
			crop_rect[i].start_x = 0;
		}
		if (crop_rect[i].start_y < 0) {
			crop_rect[i].crop_h = crop_rect[i].crop_h + crop_rect[i].start_y;
			crop_rect[i].start_y = 0;
		}
		crop_rect[i].crop_w = VPPALIGN(crop_rect[i].crop_w, align_fac);
		crop_rect[i].crop_h = VPPALIGN(crop_rect[i].crop_h, align_fac);
		if (crop_rect[i].crop_w + crop_rect[i].start_x > input.width) {
			crop_rect[i].start_x -= ((crop_rect[i].crop_w + crop_rect[i].start_x - input.width) % align_fac);
			crop_rect[i].crop_w -= ((crop_rect[i].crop_w + crop_rect[i].start_x - input.width) >> 3) << 3;
		}
		if (crop_rect[i].crop_h + crop_rect[i].start_y > input.height) {
			crop_rect[i].start_y -= ((crop_rect[i].crop_h + crop_rect[i].start_y - input.height) % align_fac);
			crop_rect[i].crop_h -= ((crop_rect[i].crop_h + crop_rect[i].start_y - input.height) >> 3) << 3;
		}
		if (crop_rect[i].start_x < 0 || crop_rect[i].start_y < 0 || \
				crop_rect[i].crop_w < 8 || crop_rect[i].crop_h < 8 || \
				crop_rect[i].crop_w + crop_rect[i].start_x > input.width || crop_rect[i].crop_h + crop_rect[i].start_y > input.height) {
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "mosaic_rect out of range, i=%d, stx=%d, sty=%d, crop_w=%d, crop_h=%d, image_w=%d, image_h=%d, %s: %s: %d\n",
						i, crop_rect[i].start_x, crop_rect[i].start_y, crop_rect[i].crop_w, crop_rect[i].crop_h, input.width, input.height, filename(__FILE__), __func__, __LINE__);
			return BM_ERR_FAILURE;
		}
	}
	return BM_SUCCESS;
}

bm_status_t bmcv_image_mosaic(
	bm_handle_t           handle,
	int                   mosaic_num,
	bm_image              input,
	bmcv_rect_t *         mosaic_rect,
	int                   is_expand)
{
	unsigned int chipid = BM1688;
	bm_status_t ret = BM_SUCCESS;
#ifndef _FPGA
	ret = bm_get_chipid(handle, &chipid);
	if (BM_SUCCESS != ret)
		return ret;
#endif
	bmcv_rect_t * crop_rect = (bmcv_rect_t *)malloc(sizeof(bmcv_rect_t) * mosaic_num);
	memcpy(crop_rect, mosaic_rect, sizeof(bmcv_rect_t) * mosaic_num);
	ret = bmcv_image_mosaic_check(handle, mosaic_num, input, crop_rect, is_expand);
	if (BM_SUCCESS != ret)
		goto mosaic_fail;
	switch(chipid)
	{
		case BM1688_PREV:
		case BM1688:
			ret = bm_vpss_mosaic(handle, mosaic_num, input, crop_rect);
			break;

		default:
			ret = BM_ERR_NOFEATURE;
			break;
	}
mosaic_fail:
	free(crop_rect);
	return ret;
}

bm_status_t bmcv_image_fill_rectangle(
	bm_handle_t   handle,
	bm_image      image,
	int           rect_num,
	bmcv_rect_t * rects,
	unsigned char r,
	unsigned char g,
	unsigned char b)
{
	if (rect_num == 0)
			return BM_SUCCESS;
	if (rect_num < 0) {
			BMCV_ERR_LOG("rect num less than 0\n");
			return BM_ERR_PARAM;
	}
	if (!image.image_private) {
			BMCV_ERR_LOG("invalidate image, not created\n");
			return BM_ERR_PARAM;
	}
	if (image.data_type != DATA_TYPE_EXT_1N_BYTE) {
			BMCV_ERR_LOG("invalidate image, data type should be DATA_TYPE_EXT_1N_BYTE\n");
			return BM_ERR_PARAM;
	}

	bm_status_t ret = BM_SUCCESS;
	unsigned int chipid = BM1688;

#ifndef _FPGA
	ret = bm_get_chipid(handle, &chipid);
	if (BM_SUCCESS != ret)
		return ret;
#endif

	switch(chipid)
	{
		case BM1688_PREV:
		case BM1688:
			ret = bm_vpss_fill_rectangle(handle, &image, rect_num, rects, r, g, b);
			if (ret != BM_SUCCESS) {
				BMCV_ERR_LOG("error bm1688 fill rectangle\n");
			}
			break;
		default:
			return BM_ERR_NOFEATURE;
	}
	return ret;
}

static bm_status_t bmcv_vpss_bitmap_mem_to_argb8888(
	bm_handle_t           handle,
	bm_device_mem_t *     bitmap_mem,
	int                   bitmap_type,
	int                   pitch,
	bmcv_color_t          color,
	bm_image *			  overlay_image)
{
	bm_status_t ret = BM_SUCCESS;
	int overlay_mem_pitch = (bitmap_type == BITMAP_1BIT) ? (pitch << 3) : pitch;
	int overlay_height = bitmap_mem->size / pitch;
	bm_image_create(handle, overlay_height, overlay_mem_pitch, FORMAT_ARGB_PACKED, DATA_TYPE_EXT_1N_BYTE, overlay_image, NULL);
	ret = bm_image_alloc_dev_mem(overlay_image[0], BMCV_HEAP1_ID);
	if (ret != BM_SUCCESS) {
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_image_alloc_dev_mem fail, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
		return ret;
	}
	int overlay_size = overlay_height * overlay_mem_pitch * 4;
	int bitmap_size = bitmap_mem->size;
	unsigned char* overlay_mem = (unsigned char *)malloc(overlay_size);
	unsigned char* bitmap_buffer = (unsigned char *)malloc(bitmap_size);
#ifdef _FPGA
	ret = bm_memcpy_d2s_fpga(handle, (void*)bitmap_buffer, bitmap_mem[0]);
#else
	ret = bm_memcpy_d2s(handle, (void*)bitmap_buffer, bitmap_mem[0]);
#endif
	if (ret != BM_SUCCESS) {
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_memcpy_d2s fail, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
		goto fail;
	}
	if (bitmap_type == BITMAP_1BIT) {
		for (int j = 0; j < overlay_height; j++) {
			for (int k = 0; k < pitch; k++) {
				int idx = j*pitch+k;
				int watermark_idx = idx / 8;
				int binary_idx = (idx % 8);
				overlay_mem[((j*pitch*4)+k*4)] = color.b; //FORMAT_ARGB_PACKED排列方式为BGRABGRA
				overlay_mem[((j*pitch*4)+k*4+1)] = color.g;
				overlay_mem[((j*pitch*4)+k*4+2)] = color.r;
				overlay_mem[((j*pitch*4)+k*4+3)] = ((bitmap_buffer[watermark_idx] >> binary_idx) & 1) * 255;
			}
		}
	} else {
		for (int j = 0; j < overlay_height; j++) {
			for (int k = 0; k < pitch; k++) {
				overlay_mem[((j*pitch*4)+k*4)] = color.b;
				overlay_mem[((j*pitch*4)+k*4+1)] = color.g;
				overlay_mem[((j*pitch*4)+k*4+2)] = color.r;
				overlay_mem[((j*pitch*4)+k*4+3)] = (bitmap_buffer[j*pitch+k] + 1) / 2;
			}
		}
	}
	void* in_ptr[4] = {(void*)overlay_mem, NULL, NULL, NULL};
	ret = bm_image_copy_host_to_device(overlay_image[0], (void **)in_ptr);
	if (ret != BM_SUCCESS)
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_image_copy_host_to_device fail, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
fail:
	free(overlay_mem);
	free(bitmap_buffer);
	return ret;
}

static bm_status_t bmcv_vpss_watermark_superpose(
	bm_handle_t           handle,
	bm_image *            image,
	bm_device_mem_t *     bitmap_mem,
	int                   bitmap_num,
	int                   bitmap_type,
	int                   pitch,
	bmcv_rect_t *         rects,
	bmcv_color_t          color)
{
	bm_status_t ret = BM_SUCCESS;
	bm_image * overlay_image = (bm_image *)malloc(sizeof(bm_image) * bitmap_num);
	for (int i = 0; i < bitmap_num; i++) {
		ret = bmcv_vpss_bitmap_mem_to_argb8888(handle, bitmap_mem + i, bitmap_type, pitch, color, overlay_image + i);
		if (ret != BM_SUCCESS) {
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bitmap_mem(%d)_to_argb8888 fail, %s: %s: %d\n", i, filename(__FILE__), __func__, __LINE__);
			goto fail;
		}
	}
	for (int i = 0; i < bitmap_num; i++) {
		ret = bm_vpss_overlay(handle, image[i], 1, rects + i, overlay_image + i);
	}
fail:
	for (int i = 0; i < bitmap_num; i++)
		bm_image_destroy(overlay_image + i);
	free(overlay_image);
	return ret;
}

static bm_status_t bmcv_vpss_watermark_repeat_superpose(
	bm_handle_t           handle,
	bm_image              image,
	bm_device_mem_t       bitmap_mem,
	int                   bitmap_num,
	int                   bitmap_type,
	int                   pitch,
	bmcv_rect_t *         rects,
	bmcv_color_t          color)
{
	bm_status_t ret = BM_SUCCESS;
	bm_image overlay_image[bitmap_num];
	ret = bmcv_vpss_bitmap_mem_to_argb8888(handle, &bitmap_mem, bitmap_type, pitch, color, overlay_image);
	if (ret != BM_SUCCESS) {
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bitmap_mem_to_argb8888 fail, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
		goto fail;
	}
	for (int i = 1; i < bitmap_num; i++)
		overlay_image[i] = overlay_image[0];
	ret = bm_vpss_overlay(handle, image, bitmap_num, rects, overlay_image);
fail:
	bm_image_destroy(overlay_image);
	return ret;
}

bm_status_t bmcv_image_watermark_superpose(
	bm_handle_t           handle,
	bm_image *            image,
	bm_device_mem_t *     bitmap_mem,
	int                   bitmap_num,
	int                   bitmap_type,
	int                   pitch,
	bmcv_rect_t *         rects,
	bmcv_color_t          color)
{
	unsigned int chipid = BM1688;
	bm_status_t ret = BM_SUCCESS;
#ifndef _FPGA
	ret = bm_get_chipid(handle, &chipid);
	if (BM_SUCCESS != ret)
		return ret;
#endif

	switch(chipid)
	{
		case BM1688_PREV:
		case BM1688:
			ret = bmcv_vpss_watermark_superpose(handle, image, bitmap_mem, bitmap_num, bitmap_type, pitch, rects, color);
			break;

		default:
			ret = BM_ERR_NOFEATURE;
			break;
	}
	return ret;
};

bm_status_t bmcv_image_watermark_repeat_superpose(
	bm_handle_t handle,
	bm_image image,
	bm_device_mem_t bitmap_mem,
	int bitmap_num,
	int bitmap_type,
	int pitch,
	bmcv_rect_t * rects,
	bmcv_color_t color)
{
	unsigned int chipid = BM1688;
	bm_status_t ret = BM_SUCCESS;
#ifndef _FPGA
	ret = bm_get_chipid(handle, &chipid);
	if (BM_SUCCESS != ret)
		return ret;
#endif

	switch(chipid)
	{
		case BM1688_PREV:
		case BM1688:
			ret = bmcv_vpss_watermark_repeat_superpose(handle, image, bitmap_mem, bitmap_num, bitmap_type, pitch, rects, color);
			break;

		default:
			ret = BM_ERR_NOFEATURE;
			break;
	}
	return ret;

}

bm_status_t bmcv_image_overlay(
	bm_handle_t         handle,
	bm_image            image,
	int                 overlay_num,
	bmcv_rect_t*        overlay_info,
	bm_image*           overlay_image)
{
	unsigned int chipid = BM1688;
	bm_status_t ret = BM_SUCCESS;
#ifndef _FPGA
	ret = bm_get_chipid(handle, &chipid);
	if (BM_SUCCESS != ret)
		return ret;
#endif

	switch(chipid)
	{
		case BM1688_PREV:
		case BM1688:
			ret = bm_vpss_overlay(handle, image, overlay_num, overlay_info, overlay_image);
			break;

		default:
			ret = BM_ERR_NOFEATURE;
			break;
	}
	return ret;
}

bm_status_t bmcv_image_flip(
  bm_handle_t          handle,
  bm_image             input,
  bm_image             output,
  bmcv_flip_mode       flip_mode)
{
	unsigned int chipid = BM1688;
	bm_status_t ret = BM_SUCCESS;
#ifndef _FPGA
	ret = bm_get_chipid(handle, &chipid);
	if (BM_SUCCESS != ret)
		return ret;
#endif

	switch(chipid)
	{
		case BM1688_PREV:
		case BM1688:
			ret = bm_vpss_flip(handle, input, output, flip_mode);
			break;

		default:
			ret = BM_ERR_NOFEATURE;
			break;
	}
	return ret;
}