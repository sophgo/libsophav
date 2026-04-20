#ifndef BM_PCIE_MODE
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include "vg_lite.h"
#include "vg_lite_util.h"
#include "bmcv_internal.h"

#define TDE_STRIDE_ALIGN 16U

static u8 device = 0;
pthread_mutex_t task_mutex0 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t task_mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t *task_mutex[2] = {&task_mutex0, &task_mutex1};
vg_lite_error_t bm_vg_lite_init(
	u8 dev_id,
	vg_lite_int32_t tess_width,
	vg_lite_int32_t tess_height);
vg_lite_error_t bm_vg_lite_close(u8 dev_id);
vg_lite_error_t bm_vg_lite_finish(u8 dev_id);
vg_lite_error_t bm_vg_lite_clear(
	u8 dev_id,
	vg_lite_buffer_t * target,
	vg_lite_rectangle_t * rect,
	vg_lite_color_t color);
vg_lite_error_t bm_vg_lite_source_global_alpha(
	uint8_t dev_id,
	vg_lite_global_alpha_t alpha_mode,
	vg_lite_uint8_t alpha_value);
vg_lite_error_t bm_vg_lite_blit_rect(
	uint8_t dev_id,
	vg_lite_buffer_t* target,
	vg_lite_buffer_t* source,
	vg_lite_rectangle_t* rect,
	vg_lite_matrix_t* matrix,
	vg_lite_blend_t blend,
	vg_lite_color_t color,
	vg_lite_filter_t filter);
vg_lite_error_t bm_vg_lite_init_path(
	uint8_t dev_id,
	vg_lite_path_t* path,
	vg_lite_format_t data_format,
	vg_lite_quality_t quality,
	vg_lite_uint32_t path_length,
	vg_lite_pointer path_data,
	vg_lite_float_t min_x, vg_lite_float_t min_y,
	vg_lite_float_t max_x, vg_lite_float_t max_y);
vg_lite_error_t bm_vg_lite_append_path(
	uint8_t dev_id,
	vg_lite_path_t *path,
	vg_lite_uint8_t *cmd,
	vg_lite_pointer data,
	vg_lite_uint32_t seg_count);
vg_lite_error_t bm_vg_lite_draw(
	uint8_t dev_id,
	vg_lite_buffer_t* target,
	vg_lite_path_t* path,
	vg_lite_fill_t fill_rule,
	vg_lite_matrix_t* matrix,
	vg_lite_blend_t blend,
	vg_lite_color_t color);
uint64_t bm_vg_lite_dmabuf_to_paddr(uint8_t dev_id, int dma_buf_fd);
void bm_vg_lite_set_high_addr(uint8_t dev_id, uint32_t read, uint32_t write);

static int bm_format_to_vglite_format(
	vg_lite_buffer_format_t *vg_fmt,
	bm_image_format_ext      format)
{
	switch (format) {
	case FORMAT_RGB_PACKED:
		*vg_fmt = VG_LITE_RGB888;
		break;
	case FORMAT_BGR_PACKED:
		*vg_fmt = VG_LITE_BGR888;
		break;
	case FORMAT_YUV422_YUYV:
		*vg_fmt = VG_LITE_YUY2;
		break;
	case FORMAT_NV12:
		*vg_fmt = VG_LITE_NV12;
		break;
	case FORMAT_NV16:
		*vg_fmt = VG_LITE_NV16;
		break;
	case FORMAT_YUV420P:
		*vg_fmt = VG_LITE_YV12;
		break;
	case FORMAT_YUV422P:
		*vg_fmt = VG_LITE_YV16;
		break;
	case FORMAT_YUV444P:
		*vg_fmt = VG_LITE_YV24;
		break;
	case FORMAT_GRAY:
		*vg_fmt = VG_LITE_A8;
		break;
	case FORMAT_ARGB1555_PACKED:
		*vg_fmt = VG_LITE_BGRA5551;
		break;
	case FORMAT_ABGR1555_PACKED:
		*vg_fmt = VG_LITE_RGBA5551;
		break;
	case FORMAT_ARGB4444_PACKED:
		*vg_fmt = VG_LITE_BGRA4444;
		break;
	case FORMAT_ABGR4444_PACKED:
		*vg_fmt = VG_LITE_RGBA4444;
		break;
	case FORMAT_ABGR_PACKED:
		*vg_fmt = VG_LITE_RGBA8888;
		break;
	case FORMAT_ARGB_PACKED:
		*vg_fmt = VG_LITE_BGRA8888;
		break;
	default:
		printf("not support fmt(%d)\n", format);
		return -1;
	}
	return 0;
}

static int tde_vglite_init_buf(vg_lite_buffer_t *vgbuf, bm_image image, bool source)
{
	if (bm_format_to_vglite_format(&vgbuf->format, image.image_format) != 0) return BM_ERR_DATA;
	vgbuf->tiled = VG_LITE_LINEAR;
	vgbuf->image_mode = VG_LITE_NORMAL_IMAGE_MODE;
	vgbuf->transparency_mode = VG_LITE_IMAGE_OPAQUE;

	vgbuf->width = (int32_t)image.width;
	vgbuf->height = (int32_t)image.height;
	vgbuf->stride = (int32_t)image.image_private->memory_layout[0].pitch_stride;
	vgbuf->address = image.image_private->data[0].u.device.device_addr;
	vgbuf->memory = (uint8_t *)image.image_private->data[0].u.device.device_addr;
	vgbuf->handle = NULL;

	memset(&vgbuf->yuv, 0, sizeof(vgbuf->yuv));

	if (image.image_private->plane_num >= 2) {
		vgbuf->yuv.uv_planar = image.image_private->data[1].u.device.device_addr;
		vgbuf->yuv.uv_memory = (uint8_t *)image.image_private->data[1].u.device.device_addr;
		vgbuf->yuv.uv_stride = image.image_private->memory_layout[1].pitch_stride;
		vgbuf->yuv.uv_height = image.image_private->memory_layout[1].H;
		vgbuf->yuv.uv_bytes = image.image_private->data[1].size;
	}

	if (image.image_private->plane_num >= 3) {
		vgbuf->yuv.v_planar = image.image_private->data[2].u.device.device_addr;
		vgbuf->yuv.v_memory = (uint8_t *)image.image_private->data[2].u.device.device_addr;
		vgbuf->yuv.v_stride = image.image_private->memory_layout[2].pitch_stride;
		vgbuf->yuv.v_height = image.image_private->memory_layout[2].H;
		vgbuf->yuv.v_bytes = image.image_private->data[2].size;
	}

	/*Test for stride alignment*/
	if (source) {
		short stride_align = TDE_STRIDE_ALIGN;
		switch (vgbuf->format) {
		case VG_LITE_ARGB8888:
		case VG_LITE_ABGR8888:
		case VG_LITE_RGBA8888:
		case VG_LITE_BGRA8888:
		case VG_LITE_YUY2:
		case VG_LITE_NV12:
		case VG_LITE_NV16:
		case VG_LITE_YV12:
		case VG_LITE_YV24:
			stride_align = 32;
			break;
		default:
			break;
		}
		if(vgbuf->stride % (stride_align * sizeof(uint8_t)) != 0x0U) {
			printf("Src buffer stride (%u bytes) not aligned to %lu bytes.\n",
				vgbuf->stride, stride_align * sizeof(uint8_t));
			return BM_ERR_DATA;
		}
	}

	return BM_SUCCESS;
}

bm_status_t bmcv_tde_fill(
	bm_handle_t    handle,
	bm_image       image,
	bmcv_color_ext color,
	bmcv_rect_t*   rect)
{
	bm_status_t ret = BM_SUCCESS;
	vg_lite_buffer_t vgbuf = {0};
	vg_lite_rectangle_t vg_rect = {.x = 0, .y = 0, .width = image.width, .height = image.height};
	u32 addr_h = 0;
	u8 dev_id = device;
	device = (device + 1) % 2;

	ret = tde_vglite_init_buf(&vgbuf, image, false);
	if (ret != BM_SUCCESS){
		printf("tde_vglite_init_buf failed.\n");
		return ret;
	}

	if (rect) {
		vg_rect.x = rect->start_x;
		vg_rect.y = rect->start_y;
		vg_rect.width = rect->crop_w;
		vg_rect.height = rect->crop_h;
	}
	ret = bm_vg_lite_init(dev_id, 1920, 1080);
	if (ret != BM_SUCCESS){
		printf("bm_vg_lite_init failed.\n");
		return ret;
	}

	addr_h = (image.image_private->data[0].u.device.device_addr >> 32);
	if (vgbuf.address != 0 && vgbuf.address < 0x100) {
		u64 paddr = bm_vg_lite_dmabuf_to_paddr(dev_id, vgbuf.address);
		vgbuf.address = paddr;
		vgbuf.memory = (void*)paddr;
		addr_h = paddr >> 32;
	}
	bm_vg_lite_set_high_addr(dev_id, 0, addr_h);

	pthread_mutex_lock(task_mutex[dev_id]);
	ret = bm_vg_lite_clear(dev_id, &vgbuf, &vg_rect, *(unsigned int*)&color);
	if (ret != BM_SUCCESS)
		printf("vg_lite_clear failed.\n");
	bm_vg_lite_finish(dev_id);
	pthread_mutex_unlock(task_mutex[dev_id]);
	bm_vg_lite_close(dev_id);
	return ret;
}

bm_status_t bmcv_tde_convert(
	bm_handle_t    handle,
	bm_image       input,
	bm_image       output,
	int            rot_angle,
	int            global_alpha,
	bmcv_rect_t*   src_rect,
	bmcv_rect_t*   dst_rect)
{
	bm_status_t ret = BM_SUCCESS;
	vg_lite_matrix_t matrix;
	vg_lite_float_t delta_x = 0, delta_y = 0;
	vg_lite_float_t offset_x, offset_y;
	vg_lite_float_t center_x, center_y;
	vg_lite_float_t scale_x, scale_y;
	int rot_w, rot_h, mov_w = 0, mov_h = 0;
	vg_lite_rectangle_t rect = { 0 };
	vg_lite_buffer_t src_buf = { 0 };
	vg_lite_buffer_t dst_buf = { 0 };
	bmcv_rect_t _src_rect, _dst_rect;
	vg_lite_float_t deg = 0.0f;
	vg_lite_blend_t vg_blend = VG_LITE_BLEND_NORMAL_LVGL;
	u32 raddr_h = 0, waddr_h = 0;
	u8 dev_id = device;
	device = (device + 1) % 2;

	ret = tde_vglite_init_buf(&src_buf, input, true);
	if (ret != BM_SUCCESS){
		printf("tde_vglite_init_buf input failed.\n");
		return ret;
	}

	ret = tde_vglite_init_buf(&dst_buf, output, false);
	if (ret != BM_SUCCESS){
		printf("tde_vglite_init_buf output failed.\n");
		return ret;
	}

	if (src_rect == NULL) {
		_src_rect.start_x = 0;
		_src_rect.start_y = 0;
		_src_rect.crop_w = input.width;
		_src_rect.crop_h = input.height;
	} else
		_src_rect = src_rect[0];

	if (dst_rect == NULL) {
		_dst_rect.start_x = 0;
		_dst_rect.start_y = 0;
		_dst_rect.crop_w = output.width;
		_dst_rect.crop_h = output.height;
	} else
		_dst_rect = dst_rect[0];

	rot_w = _src_rect.crop_w;
	rot_h = _src_rect.crop_h;
	scale_x = _dst_rect.crop_w * 1.0f / rot_w;
	scale_y = _dst_rect.crop_h * 1.0f / rot_h;
	deg = (vg_lite_float_t )rot_angle;
	switch (rot_angle) {
		case 0:
			break;
		case 90:
			rot_w = _src_rect.crop_h;
			rot_h = _src_rect.crop_w;
			delta_x = ((vg_lite_float_t)_src_rect.crop_h - _src_rect.crop_w) / 2;
			delta_y = ((vg_lite_float_t)_src_rect.crop_w - _src_rect.crop_h) / 2;
			scale_x = _dst_rect.crop_h * 1.0f / rot_h;
			scale_y = _dst_rect.crop_w * 1.0f / rot_w;
			mov_h = rot_w - _dst_rect.crop_w;
			break;
		case 180:
			mov_w = _dst_rect.crop_w;
			mov_h = _dst_rect.crop_h;
			break;
		case 270:
			rot_w = _src_rect.crop_h;
			rot_h = _src_rect.crop_w;
			delta_x = ((vg_lite_float_t)_src_rect.crop_h - _src_rect.crop_w) / 2;
			delta_y = ((vg_lite_float_t)_src_rect.crop_w - _src_rect.crop_h) / 2;
			scale_x = _dst_rect.crop_h * 1.0f / rot_h;
			scale_y = _dst_rect.crop_w * 1.0f / rot_w;
			mov_w = rot_h - _dst_rect.crop_h;
			break;
		default:
			printf("not supported rot angle(%d)\n", rot_angle);
			return BM_ERR_PARAM;
	}
	// move to center of the surface dst_rect
	offset_x = rot_w / 2.0f;
	offset_y = rot_h / 2.0f;
	center_x = offset_x + _dst_rect.start_x;
	center_y = offset_y + _dst_rect.start_y;

	ret = bm_vg_lite_init(dev_id, 1920, 1080);
	if (ret != BM_SUCCESS){
		printf("bm_vg_lite_init failed.\n");
		return ret;
	}

	raddr_h = (input.image_private->data[0].u.device.device_addr >> 32);
	waddr_h = (output.image_private->data[0].u.device.device_addr >> 32);

	if (src_buf.address != 0 && src_buf.address < 0x100) {
		u64 paddr = bm_vg_lite_dmabuf_to_paddr(dev_id, src_buf.address);
		src_buf.address = paddr;
		src_buf.memory = (void*)paddr;
		raddr_h = paddr >> 32;
	}
	if (dst_buf.address != 0 && dst_buf.address < 0x100) {
		u64 paddr = bm_vg_lite_dmabuf_to_paddr(dev_id, dst_buf.address);
		dst_buf.address = paddr;
		dst_buf.memory = (void*)paddr;
		waddr_h = paddr >> 32;
	}

	bm_vg_lite_set_high_addr(dev_id, raddr_h, waddr_h);
	vg_lite_identity(&matrix);
	vg_lite_translate(center_x, center_y, &matrix);
	vg_lite_rotate(deg, &matrix);
	vg_lite_translate(-offset_x, -offset_y, &matrix);
	vg_lite_translate(delta_x, delta_y, &matrix);
	vg_lite_translate(mov_w, mov_h, &matrix);
	vg_lite_scale(scale_x, scale_y, &matrix);

	rect.x = _src_rect.start_x;
	rect.y = _src_rect.start_y;
	rect.width = _src_rect.crop_w;
	rect.height = _src_rect.crop_h;

	pthread_mutex_lock(task_mutex[dev_id]);
	if (global_alpha <= 0 && global_alpha >= -15) {
		vg_blend = (vg_lite_blend_t)(-global_alpha);
	} else if (global_alpha > 0 && global_alpha < 256) {
		bm_vg_lite_source_global_alpha(dev_id, VG_LITE_SCALED, global_alpha);
	} else {
		printf("not supported overlay mode(%d)\n", global_alpha);
		ret = BM_ERR_PARAM;
		goto fail;
	}
	ret = bm_vg_lite_blit_rect(dev_id, &dst_buf, &src_buf,
		&rect, &matrix, vg_blend, 0, VG_LITE_FILTER_POINT);
	if (ret != BM_SUCCESS)
		printf("vg_lite_blit_rect fail\n");
	bm_vg_lite_finish(dev_id);
	bm_vg_lite_source_global_alpha(dev_id, VG_LITE_SCALED, 255);
fail:
	pthread_mutex_unlock(task_mutex[dev_id]);
	bm_vg_lite_close(dev_id);
	return ret;
}

bm_status_t bmcv_tde_warp_affine(
    bm_handle_t handle,
    bmcv_affine_matrix matrix,
    bm_image input,
    bm_image output,
    int use_bilinear)
{
	bm_status_t ret = BM_SUCCESS;
	vg_lite_matrix_t vg_matrix;
	vg_lite_rectangle_t rect = { 0 };
	vg_lite_buffer_t src_buf = { 0 };
	vg_lite_buffer_t dst_buf = { 0 };
	vg_lite_blend_t vg_blend = VG_LITE_BLEND_NORMAL_LVGL;
	vg_lite_filter_t filter = VG_LITE_FILTER_POINT;
	u32 raddr_h = 0, waddr_h = 0;
	u8 dev_id = device;
	device = (device + 1) % 2;

	ret = tde_vglite_init_buf(&src_buf, input, true);
	if (ret != BM_SUCCESS){
		printf("tde_vglite_init_buf input failed.\n");
		return ret;
	}

	ret = tde_vglite_init_buf(&dst_buf, output, false);
	if (ret != BM_SUCCESS){
		printf("tde_vglite_init_buf output failed.\n");
		return ret;
	}

	ret = bm_vg_lite_init(dev_id, 1920, 1080);
	if (ret != BM_SUCCESS){
		printf("bm_vg_lite_init failed.\n");
		return ret;
	}

	raddr_h = (input.image_private->data[0].u.device.device_addr >> 32);
	waddr_h = (output.image_private->data[0].u.device.device_addr >> 32);

	if (src_buf.address != 0 && src_buf.address < 0x100) {
		u64 paddr = bm_vg_lite_dmabuf_to_paddr(dev_id, src_buf.address);
		src_buf.address = paddr;
		src_buf.memory = (void*)paddr;
		raddr_h = paddr >> 32;
	}
	if (dst_buf.address != 0 && dst_buf.address < 0x100) {
		u64 paddr = bm_vg_lite_dmabuf_to_paddr(dev_id, dst_buf.address);
		dst_buf.address = paddr;
		dst_buf.memory = (void*)paddr;
		waddr_h = paddr >> 32;
	}

	bm_vg_lite_set_high_addr(dev_id, raddr_h, waddr_h);
	vg_lite_identity(&vg_matrix);

	rect.x = 0;
	rect.y = 0;
	rect.width = input.width;
	rect.height = input.height;

	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 3; j++)
			vg_matrix.m[i][j] = matrix.m[i*3+j];
	vg_matrix.m[2][2] = 1;

	if (use_bilinear)
		filter = VG_LITE_FILTER_BI_LINEAR;

	pthread_mutex_lock(task_mutex[dev_id]);
	ret = bm_vg_lite_blit_rect(dev_id, &dst_buf, &src_buf,
		&rect, &vg_matrix, vg_blend, 0, filter);
	if (ret != BM_SUCCESS)
		printf("vg_lite_blit_rect fail\n");
	bm_vg_lite_finish(dev_id);
	pthread_mutex_unlock(task_mutex[dev_id]);

	bm_vg_lite_close(dev_id);
	return ret;
}

bm_status_t bmcv_tde_line(
	bm_handle_t      handle,
	bm_image         image,
	bmcv_point_t     start,
	bmcv_point_t     end,
	bmcv_color_ext   color,
	int              thick)
{
	bm_status_t ret = BM_SUCCESS;
	vg_lite_matrix_t matrix;
	uint32_t data_size;
	uint32_t addr_h = 0;
	vg_lite_path_t path = {0};
	vg_lite_buffer_t fb = {0};
	static uint8_t sides_cmd[] = {
		VLC_OP_MOVE,
		VLC_OP_LINE,
		VLC_OP_LINE,
		VLC_OP_LINE,
		VLC_OP_END
	};
	static float sides_data_left[] = {
		0, 0,
		50, 0,
		50, 50,
		0, 50,
	};
	vg_lite_blend_t vg_blend = VG_LITE_BLEND_NORMAL_LVGL;
	u8 dev_id = device;
	device = (device + 1) % 2;

	ret = tde_vglite_init_buf(&fb, image, false);
	if (ret != BM_SUCCESS){
		printf("tde_vglite_init_buf failed.\n");
		return ret;
	}

	if (color.a == 0) color.a = 255;

	ret = bm_vg_lite_init(dev_id, 1920, 1080);
	if (ret != BM_SUCCESS){
		printf("bm_vg_lite_init failed.\n");
		return ret;
	}

	addr_h = (image.image_private->data[0].u.device.device_addr >> 32);
	if (fb.address != 0 && fb.address < 0x100) {
		u64 paddr = bm_vg_lite_dmabuf_to_paddr(dev_id, fb.address);
		fb.address = paddr;
		fb.memory = (void*)paddr;
		addr_h = paddr >> 32;
	}
	bm_vg_lite_set_high_addr(dev_id, 0, addr_h);

	vg_lite_identity(&matrix);

	if (start.y == end.y) {
		sides_data_left[0] = BM_MIN(start.x, end.x);
		sides_data_left[1] = start.y - thick / 2.0;
		sides_data_left[2] = BM_MAX(end.x, start.x);
		sides_data_left[3] = end.y - thick / 2.0;
		sides_data_left[4] = BM_MAX(end.x, start.x);
		sides_data_left[5] = end.y + thick / 2.0;
		sides_data_left[6] = BM_MIN(start.x, end.x);
		sides_data_left[7] = start.y + thick / 2.0;
	} else if(start.x == end.x){
		sides_data_left[0] = start.x + thick / 2.0;
		sides_data_left[1] = BM_MIN(start.y, end.y);
		sides_data_left[2] = end.x + thick / 2.0;
		sides_data_left[3] = BM_MAX(end.y, start.y);
		sides_data_left[4] = end.x - thick / 2.0;
		sides_data_left[5] = BM_MAX(end.y, start.y);
		sides_data_left[6] = start.x - thick / 2.0;
		sides_data_left[7] = BM_MIN(start.y, end.y);
	} else if(((start.x < end.x) && (start.y < end.y)) ||
		((start.x > end.x) && (start.y > end.y))) {
		int w = thick / 2;
		sides_data_left[0] = BM_MIN(start.x, end.x) + w;
		sides_data_left[1] = BM_MIN(start.y, end.y) - w;
		sides_data_left[2] = BM_MAX(end.x, start.x) + w;
		sides_data_left[3] = BM_MAX(end.y, start.y) - w;
		sides_data_left[4] = BM_MAX(end.x, start.x) - w;
		sides_data_left[5] = BM_MAX(end.y, start.y) + w;
		sides_data_left[6] = BM_MIN(start.x, end.x) - w;
		sides_data_left[7] = BM_MIN(start.y, end.y) + w;
	} else if(((start.x < end.x) && (start.y > end.y)) ||
		((start.x > end.x) && (start.y < end.y))) {
		int w = thick / 2;
		sides_data_left[0] = BM_MAX(end.x, start.x) + w;
		sides_data_left[1] = BM_MIN(end.y, start.y) + w;
		sides_data_left[2] = BM_MIN(start.x, end.x) + w;
		sides_data_left[3] = BM_MAX(start.y, end.y) + w;
		sides_data_left[4] = BM_MIN(start.x, end.x) - w;
		sides_data_left[5] = BM_MAX(start.y, end.y) - w;
		sides_data_left[6] = BM_MAX(end.x, start.x) - w;
		sides_data_left[7] = BM_MIN(end.y, start.y) - w;
	}

	data_size = vg_lite_get_path_length(sides_cmd, sizeof(sides_cmd), VG_LITE_FP32);

	pthread_mutex_lock(task_mutex[dev_id]);
	ret = bm_vg_lite_init_path(dev_id, &path, VG_LITE_FP32,
		VG_LITE_HIGH, data_size, NULL, 0, 0, 0, 0);
	if (ret != BM_SUCCESS) {
		printf("vg_lite_init_path fail\n");
		goto fail;
	}
	ret = bm_vg_lite_append_path(dev_id, &path, sides_cmd, sides_data_left, sizeof(sides_cmd));
	if (ret != BM_SUCCESS) {
		printf("vg_lite_append_path fail\n");
		goto fail;
	}

	ret = bm_vg_lite_draw(dev_id, &fb, &path, VG_LITE_FILL_NON_ZERO,
		&matrix, vg_blend, *(unsigned int*)&color);
	if (ret != BM_SUCCESS) {
		printf("vg_lite_draw fail\n");
		goto fail;
	}
	bm_vg_lite_finish(dev_id);
fail:
	pthread_mutex_unlock(task_mutex[dev_id]);
	ret = vg_lite_clear_path(&path);
	if (ret != BM_SUCCESS)
		printf("vg_lite_clear_path fail\n");
	bm_vg_lite_close(dev_id);
	return ret;
}

bm_status_t bmcv_tde_draw(
	bm_handle_t      handle,
	bm_image         image,
	bmcv_point_t    *point,
	int              path_num,
	bmcv_color_ext   color)
{
	bm_status_t ret = BM_SUCCESS;
	vg_lite_matrix_t matrix;
	uint32_t data_size;
	vg_lite_path_t path = {0};
	vg_lite_buffer_t fb = {0};
	uint8_t sides_cmd[path_num + 1];
	float sides_data_left[path_num * 2];
	vg_lite_blend_t vg_blend = VG_LITE_BLEND_NORMAL_LVGL;
	uint32_t addr_h = 0;
	u8 dev_id = device;
	device = (device + 1) % 2;

	ret = tde_vglite_init_buf(&fb, image, false);
	if (ret != BM_SUCCESS){
		printf("tde_vglite_init_buf failed.\n");
		return ret;
	}

	if (color.a == 0) color.a = 255;

	ret = bm_vg_lite_init(dev_id, 1920, 1080);
	if (ret != BM_SUCCESS){
		printf("bm_vg_lite_init failed.\n");
		return ret;
	}

	addr_h = (image.image_private->data[0].u.device.device_addr >> 32);
	if (fb.address != 0 && fb.address < 0x100) {
		u64 paddr = bm_vg_lite_dmabuf_to_paddr(dev_id, fb.address);
		fb.address = paddr;
		fb.memory = (void*)paddr;
		addr_h = paddr >> 32;
	}
	bm_vg_lite_set_high_addr(dev_id, 0, addr_h);

	vg_lite_identity(&matrix);

	for (int i = 0; i < path_num; i++) {
		sides_data_left[2 * i] = point[i].x;
		sides_data_left[2 * i + 1] = point[i].y;
		sides_cmd[i + 1] = VLC_OP_LINE;
	}
	sides_cmd[0] = VLC_OP_MOVE;
	sides_cmd[path_num] = VLC_OP_END;

	data_size = vg_lite_get_path_length(sides_cmd, sizeof(sides_cmd), VG_LITE_FP32);

	pthread_mutex_lock(task_mutex[dev_id]);
	ret = bm_vg_lite_init_path(dev_id, &path, VG_LITE_FP32,
		VG_LITE_HIGH, data_size, NULL, 0, 0, 0, 0);
	if (ret != BM_SUCCESS) {
		printf("vg_lite_init_path fail\n");
		goto fail;
	}
	ret = bm_vg_lite_append_path(dev_id, &path, sides_cmd, sides_data_left, sizeof(sides_cmd));
	if (ret != BM_SUCCESS) {
		printf("vg_lite_append_path fail\n");
		goto fail;
	}

	ret = bm_vg_lite_draw(dev_id, &fb, &path, VG_LITE_FILL_NON_ZERO,
		&matrix, vg_blend, *(unsigned int*)&color);
	if (ret != BM_SUCCESS) {
		printf("vg_lite_draw fail\n");
		goto fail;
	}
	bm_vg_lite_finish(dev_id);
fail:
	pthread_mutex_unlock(task_mutex[dev_id]);
	ret = vg_lite_clear_path(&path);
	if (ret != BM_SUCCESS)
		printf("vg_lite_clear_path fail\n");
	bm_vg_lite_close(dev_id);
	return ret;
}
#endif