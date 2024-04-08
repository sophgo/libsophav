#include <memory.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bmcv_a2_vpss_internal.h"
#ifdef __linux__
#include <sys/ioctl.h>
#endif

#define VPSS_TIMEOUT_MS 1000

#define DEFINE_CSC_COEF0(a, b, c) \
		.coef[0][0] = a, .coef[0][1] = b, .coef[0][2] = c,
#define DEFINE_CSC_COEF1(a, b, c) \
		.coef[1][0] = a, .coef[1][1] = b, .coef[1][2] = c,
#define DEFINE_CSC_COEF2(a, b, c) \
		.coef[2][0] = a, .coef[2][1] = b, .coef[2][2] = c,
#define BIT(nr) (1U << (nr))

static bmcv_vpss_csc_matrix csc_default_matrix[9] = {
	/*YUV2RGB 601 NOT FULL RANGE YCbCr2RGB_BT601*/
	// {1.1643835616438400, 0.0000000000000000, 1.5958974359999800, 0.0,
	//  1.1643835616438400,-0.3936638456015240,-0.8138402992560010, 0.0,
	//  1.1643835616438400, 2.0160738896544600, 0.0000000000000000, 0.0,
	//  16.0, 128.0, 128.0},
	{
		DEFINE_CSC_COEF0(1192,	0,		1634)
		DEFINE_CSC_COEF1(1192,	BIT(13) | 403,	BIT(13) | 833)
		DEFINE_CSC_COEF2(1192,	2064,		0)
		.sub[0] = 16,  .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	/*YUV2RGB 601 FULL RANGE YPbPr2RGB_BT601*/
	// {1.0000000000000000, 0.0000000000000000, 1.4018863751529200, 0.0,
	//  1.0000000000000000,-0.3458066722146720,-0.7149028511111540, 0.0,
	//  1.0000000000000000, 1.7709825540494100, 0.0000000000000000, 0.0,
	//  0.0, 128.0, 128.0},
	{
		DEFINE_CSC_COEF0(BIT(10),	0,		1436)
		DEFINE_CSC_COEF1(BIT(10),	BIT(13) | 354,	BIT(13) | 732)
		DEFINE_CSC_COEF2(BIT(10),	1813,		0)
		.sub[0] = 0,   .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	/*RGB2YUV 601 NOT FULL RANGE RGB2YCbCr_BT601*/
	// {0.2568370271402130, 0.5036437166574750, 0.0983427856140760, 16.0,
	// -0.1483362360666210,-0.2908794502078890, 0.4392156862745100, 128.0,
	//  0.4392156862745100,-0.3674637551088690,-0.0717519311656413, 128.0,
	//  0.0, 0.0, 0.0},
	{
		DEFINE_CSC_COEF0(263,		516,		101)
		DEFINE_CSC_COEF1(BIT(13)|152,	BIT(13)|298,	450)
		DEFINE_CSC_COEF2(450,		BIT(13)|376,	BIT(13)|73)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 16,  .add[1] = 128, .add[2] = 128
	},
	/*YUV2RGB 709 NOT FULL RANGE YCbCr2RGB_BT709*/
	// {1.1643835616438400, 0.0000000000000000, 1.7926522634175300, 0.0,
	//  1.1643835616438400,-0.2132370215686210,-0.5330040401422640, 0.0,
	//  1.1643835616438400, 2.1124192819911800, 0.0000000000000000, 0.0,
	//  16.0, 128.0, 128.0},
	{
		DEFINE_CSC_COEF0(1192,	0,		1836)
		DEFINE_CSC_COEF1(1192,	BIT(13) | 218,	BIT(13) | 546)
		DEFINE_CSC_COEF2(1192,	2163,		0)
		.sub[0] = 16,  .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	/*RGB2YUV 709 NOT FULL RANGE RGB2YCbCr_BT709*/
	// {0.1826193815131790, 0.6142036888240730, 0.0620004590745127, 16.0,
	// -0.1006613638136630,-0.3385543224608470, 0.4392156862745100, 128.0,
	//  0.4392156862745100,-0.3989444541822880,-0.0402712320922214, 128.0,
	//  0.0, 0.0, 0.0},
	{
		DEFINE_CSC_COEF0(187,		629,		63)
		DEFINE_CSC_COEF1(BIT(13)|103,	BIT(13)|347,	450)
		DEFINE_CSC_COEF2(450,		BIT(13)|409,	BIT(13)|41)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 16,  .add[1] = 128, .add[2] = 128
	},
	/*RGB2YUV 601 FULL RANGE RGB2YPbPr_BT601*/
	// {0.2990568124235360, 0.5864344646011700, 0.1145087229752940, 0.0,
	// -0.1688649115936980,-0.3311350884063020, 0.5000000000000000, 128.0,
	//  0.5000000000000000,-0.4183181140748280,-0.0816818859251720, 128.0,
	//  0.0, 0.0, 0.0},
	{
		DEFINE_CSC_COEF0(306,		601,		117)
		DEFINE_CSC_COEF1(BIT(13)|173,	BIT(13)|339,	512)
		DEFINE_CSC_COEF2(512,		BIT(13)|428,	BIT(13)|83)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 128, .add[2] = 128
	},
	/*YUV2RGB 709 FULL RANGE YPbPr2RGB_BT709*/
	// {1.0000000000000000, 0.0000000000000000, 1.5747219882569700, 0,
	//  1.0000000000000000,-0.1873140895347890,-0.4682074705563420, 0,
	//  1.0000000000000000, 1.8556153692785300, 0.0000000000000000, 0,
	//  0.0, 128.0, 128.0},
	{
		DEFINE_CSC_COEF0(BIT(10),	0,		1613)
		DEFINE_CSC_COEF1(BIT(10),	BIT(13) | 192,	BIT(13) | 479)
		DEFINE_CSC_COEF2(BIT(10),	1900,		0)
		.sub[0] = 0,   .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	/*RGB2YUV 709 FULL RANGE RGB2YPbPr_BT709*/
	// {0.2126390058715100, 0.7151686787677560, 0.0721923153607340, 0.0,
	// -0.1145921775557320,-0.3854078224442680, 0.5000000000000000, 128.0,
	//  0.5000000000000000,-0.4541555170378730,-0.0458444829621270, 128.0,
	//  0.0, 0.0, 0.0},
	{
		DEFINE_CSC_COEF0(218,		732,		74)
		DEFINE_CSC_COEF1(BIT(13)|117,	BIT(13)|395,	512)
		DEFINE_CSC_COEF2(512,		BIT(13)|465,	BIT(13)|47)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 128, .add[2] = 128
	},
	// none
	{
		DEFINE_CSC_COEF0(BIT(10),	0,		0)
		DEFINE_CSC_COEF1(0,		BIT(10),	0)
		DEFINE_CSC_COEF2(0,		0,		BIT(10))
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
#if 0
		1                                1.140014648       -145.921874944
		1            -0.395019531       -0.580993652       124.929687424
		1             2.031982422                          -260.093750016

	/*YUV2RGB 601 opencv: YUV2RGB  maosi simulation   8*/
	{0x3F800000, 0x00000000, 0x3F91EBFF, 0xC311EBFF,
	0x3F800000, 0xBECA3FFF, 0xBF14BBFF, 0x42F9DBFF,
	0x3F800000, 0x40020C00, 0x00000000, 0xC3820C00},

		// Coefficients for RGB to YUV420p conversion
		const int ITUR_BT_601_CRY =  269484;
		const int ITUR_BT_601_CGY =  528482;
		const int ITUR_BT_601_CBY =  102760;
		const int ITUR_BT_601_CRU = -155188;
		const int ITUR_BT_601_CGU = -305135;
		const int ITUR_BT_601_CBU =  460324;
		const int ITUR_BT_601_CGV = -385875;
		const int ITUR_BT_601_CBV = -74448;
		 269484    528482   102760    16.5
		-155188   -305135   460324    128.5
		 460324   -385875   -74448    128.5

		shift=1024*1024

	/*RGB2YUV420P, opencv:RGB2YUVi420 601 NOT FULL RANGE  9*/
	{0x3E83957F, 0x3F01061F, 0x3DC8B400, 0x41840000,
	0xBE178D00, 0xBE94FDE0, 0x3EE0C47F, 0x43008000,
	0x3EE0C47F, 0xBEBC6A60, 0xBD916800, 0x43008000},

	 8414, 16519, 3208,     16
	 -4865, -9528, 14392,   128
	 14392, -12061, -2332,  128

	 0.256774902     0.504119873       0.097900391          16
	 -0.148468018    -0.290771484       0.439208984          128
	 0.439208984    -0.36807251       -0.071166992          128

	/*RGB2YUV420P,  FFMPEG : BGR2i420  601 NOT FULL RANGE    10*/
	{0x3E8377FF, 0x3F010DFF, 0x3DC88000, 0x41800000,
	0xBE180800, 0xBE94DFFF, 0x3EE0DFFF, 0x43000000,
	0x3EE0DFFF, 0xBEBC7400, 0xBD91BFFF, 0x43000000},

	1.163999557               1.595999718   -204.787963904
	1.163999557 -0.390999794 -0.812999725    154.611938432
	1.163999557  2.017999649                -258.803955072

	/*YUV420P2bgr  OPENCV : i4202BGR  601   11*/
	{0x3F94FDEF, 0x0,        0x3FCC49B8, 0xC34CC9B8,
	0x3F94FDEF, 0xBEC8311F, 0xBF5020BF, 0x431A9CA7,
	0x3F94FDEF, 0x400126E7, 0x0,        0xC38166E7},
#endif
};

bm_status_t simple_check_bm_vpss_input_param(
	bm_handle_t             handle,
	bm_image*               input_or_output,
	int                     frame_number
)
{
	if (handle == NULL)
	{
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "handle is nullptr");
		return BM_ERR_FAILURE;
	}
	if (input_or_output == NULL)
	{
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "input or output is nullptr");
		return BM_ERR_FAILURE;
	}
	if((frame_number > VPSS_MAX_GRP_NUM) || (frame_number <= 0))
	{
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "input num should less than 512");
		return BM_NOT_SUPPORTED;
	}

	return BM_SUCCESS;
}

bm_status_t check_bm_vpss_bm_image_param(
	int                     frame_number,
	bm_image*               input,
	bm_image*               output)
{
	int stride[4];
	int frame_idx = 0;
	int plane_num = 0;
	bm_device_mem_t device_mem[4];

	for (frame_idx = 0; frame_idx < frame_number; frame_idx++) {

		if(!input[frame_idx].image_private->attached ||
			 !output[frame_idx].image_private->attached)
		{
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
				"not correctly create bm_image ,frame [%d] input or output not attache mem %s: %s: %d\n",
				frame_idx,filename(__FILE__), __func__, __LINE__);
			return BM_ERR_PARAM;
		}
		if(input[frame_idx].data_type != DATA_TYPE_EXT_1N_BYTE)
		{
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
				"input only support DATA_TYPE_EXT_1N_BYTE,frame [%d], %s: %s: %d\n",
				frame_idx, filename(__FILE__), __func__, __LINE__);
			return BM_NOT_SUPPORTED;
		}

		 if((output[frame_idx].data_type != DATA_TYPE_EXT_FLOAT32) &&
				(output[frame_idx].data_type != DATA_TYPE_EXT_1N_BYTE) &&
				(output[frame_idx].data_type != DATA_TYPE_EXT_1N_BYTE_SIGNED) &&
				(output[frame_idx].data_type != DATA_TYPE_EXT_FP16) &&
				(output[frame_idx].data_type != DATA_TYPE_EXT_BF16))
		 {
			 bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
				 "bm_vpss [%d] output data type %d not support %s: %s: %d\n",
				 frame_idx ,output[frame_idx].data_type, filename(__FILE__), __func__, __LINE__);
			 return BM_NOT_SUPPORTED;
		 }

		if((bm_image_get_stride(input[frame_idx], stride) != BM_SUCCESS) ||
			 (bm_image_get_stride(output[frame_idx], stride) != BM_SUCCESS))
		{
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
				"not correctly create input bm_image , frame [%d],input or output get stride err %s: %s: %d\n",
				frame_idx,filename(__FILE__), __func__, __LINE__);
			return BM_ERR_FAILURE;
		}

		plane_num = bm_image_get_plane_num(input[frame_idx]);
		if(plane_num == 0 || bm_image_get_device_mem(input[frame_idx], device_mem) != BM_SUCCESS)
		{
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
				"not correctly create input[%d] bm_image, get plane num or device mem err %s: %s: %d\n",
				frame_idx, filename(__FILE__), __func__, __LINE__);
			return BM_ERR_FAILURE;
		}
#ifndef USING_CMODEL
		u64 device_addr = 0;
		int i = 0;

		for (i = 0; i < plane_num; i++) {
			device_addr = device_mem[i].u.device.device_addr;
			if((device_addr > 0x4ffffffff) || (device_addr < 0x100000000))
			{
				bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
				  "input[%d] device memory should between 0x100000000 and 0x4ffffffff %u, %s: %s: %d\n",
				  frame_idx, device_addr, filename(__FILE__), __func__, __LINE__);
				return BM_ERR_FAILURE;
			}
		}
#endif
		plane_num = bm_image_get_plane_num(output[frame_idx]);
		if(plane_num == 0 || bm_image_get_device_mem(output[frame_idx], device_mem) != BM_SUCCESS)
		{
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
				"not correctly create output[%d] bm_image, get plane num or device mem err %s: %s: %d\n",
				frame_idx, filename(__FILE__), __func__, __LINE__);
			return BM_ERR_FAILURE;
		}
#ifndef USING_CMODEL
		for (i = 0; i < plane_num; i++) {
			device_addr = device_mem[i].u.device.device_addr;
			if((device_addr > 0x4ffffffff) || (device_addr < 0x100000000))
			{
				bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
				  "output[%d] device memory should between 0x100000000 and 0x4ffffffff %u, %s: %s: %d\n",
				  frame_idx, device_addr, filename(__FILE__), __func__, __LINE__);
				return BM_ERR_FAILURE;
			}
		}
#endif
	}

	return BM_SUCCESS;
}
bm_status_t check_bm_vpss_csctype(
	int                     frame_number,
	bm_image*               input,
	bm_image*               output,
	csc_type_t*             csc_type,
	csc_matrix_t*           matrix,
	bmcv_csc_cfg*           csc_cfg)
{
	bm_status_t ret = BM_SUCCESS;
	int input_color_space = COLOR_SPACE_YUV, output_color_space = COLOR_SPACE_YUV;
	int idx = 0;

	for (idx = 0; idx < frame_number; idx++) {
		input_color_space = is_csc_yuv_or_rgb(input[idx].image_format);
		output_color_space = is_csc_yuv_or_rgb(output[idx].image_format);

		if((is_csc_yuv_or_rgb(input[0].image_format) != is_csc_yuv_or_rgb(input[idx].image_format)) ||
			(is_csc_yuv_or_rgb(output[0].image_format) != is_csc_yuv_or_rgb(output[idx].image_format)))
		{
			ret = BM_ERR_PARAM;
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
				"bm_vpss Input and output color space changes will cause hardware hang,"
				" %s: %s: %d\n",
				filename(__FILE__), __func__, __LINE__);
			return ret;
		}
	}

	csc_cfg->is_fancy = false;
	csc_cfg->is_user_defined_matrix = false;

	if (*csc_type <= CSC_RGB2YPbPr_BT709)
		csc_cfg->csc_type = *csc_type;

	switch(*csc_type)
		{
			case CSC_YCbCr2RGB_BT601:
			case CSC_YPbPr2RGB_BT601:
			case CSC_YPbPr2RGB_BT709:
			case CSC_YCbCr2RGB_BT709:
				if(COLOR_SPACE_YUV != input_color_space)
				{
					ret = BM_NOT_SUPPORTED;
					break;
				}
				if((COLOR_SPACE_RGB != output_color_space) &&
					 (COLOR_SPACE_HSV != output_color_space) &&
					 (COLOR_SPACE_RGBY != output_color_space))
				{
					ret = BM_NOT_SUPPORTED;
					break;
				}
				break;
			case CSC_RGB2YCbCr_BT709:
			case CSC_RGB2YPbPr_BT601:
			case CSC_RGB2YCbCr_BT601:
			case CSC_RGB2YPbPr_BT709:
				if(COLOR_SPACE_RGB != input_color_space)
				{
					ret = BM_NOT_SUPPORTED;
					break;
				}
				if((COLOR_SPACE_YUV != output_color_space) &&
					 (COLOR_SPACE_RGBY != output_color_space))
				{
					ret = BM_NOT_SUPPORTED;
					break;
				}
				break;
			case CSC_USER_DEFINED_MATRIX:
				if(NULL == matrix)
				{
					ret = BM_NOT_SUPPORTED;
					bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
						"bm_vpss csc_type param %d is CSC_USER_DEFINED_MATRIX ,"
						"matrix can not be null, %s: %s: %d\n",
						*csc_type, filename(__FILE__), __func__, __LINE__);
					return ret;
				} else {
					csc_cfg->is_user_defined_matrix = true;
					csc_cfg->csc_matrix.coef[0][0] = matrix->csc_coe00 < 0 ? BIT(13)|(unsigned short)(-matrix->csc_coe00) : (unsigned short)matrix->csc_coe00;
					csc_cfg->csc_matrix.coef[0][1] = matrix->csc_coe01 < 0 ? BIT(13)|(unsigned short)(-matrix->csc_coe01) : (unsigned short)matrix->csc_coe01;
					csc_cfg->csc_matrix.coef[0][2] = matrix->csc_coe02 < 0 ? BIT(13)|(unsigned short)(-matrix->csc_coe02) : (unsigned short)matrix->csc_coe02;
					csc_cfg->csc_matrix.coef[1][0] = matrix->csc_coe10 < 0 ? BIT(13)|(unsigned short)(-matrix->csc_coe10) : (unsigned short)matrix->csc_coe10;
					csc_cfg->csc_matrix.coef[1][1] = matrix->csc_coe11 < 0 ? BIT(13)|(unsigned short)(-matrix->csc_coe11) : (unsigned short)matrix->csc_coe11;
					csc_cfg->csc_matrix.coef[1][2] = matrix->csc_coe12 < 0 ? BIT(13)|(unsigned short)(-matrix->csc_coe12) : (unsigned short)matrix->csc_coe12;
					csc_cfg->csc_matrix.coef[2][0] = matrix->csc_coe20 < 0 ? BIT(13)|(unsigned short)(-matrix->csc_coe20) : (unsigned short)matrix->csc_coe20;
					csc_cfg->csc_matrix.coef[2][1] = matrix->csc_coe21 < 0 ? BIT(13)|(unsigned short)(-matrix->csc_coe21) : (unsigned short)matrix->csc_coe21;
					csc_cfg->csc_matrix.coef[2][2] = matrix->csc_coe22 < 0 ? BIT(13)|(unsigned short)(-matrix->csc_coe22) : (unsigned short)matrix->csc_coe22;
					csc_cfg->csc_matrix.add[0] = matrix->csc_add0 < 0 ? 0 : (unsigned char)matrix->csc_add0;
					csc_cfg->csc_matrix.add[1] = matrix->csc_add1 < 0 ? 0 : (unsigned char)matrix->csc_add1;
					csc_cfg->csc_matrix.add[2] = matrix->csc_add2 < 0 ? 0 : (unsigned char)matrix->csc_add2;
					csc_cfg->csc_matrix.sub[0] = matrix->csc_sub0 < 0 ? 0 : (unsigned char)matrix->csc_sub0;
					csc_cfg->csc_matrix.sub[1] = matrix->csc_sub1 < 0 ? 0 : (unsigned char)matrix->csc_sub1;
					csc_cfg->csc_matrix.sub[2] = matrix->csc_sub2 < 0 ? 0 : (unsigned char)matrix->csc_sub2;
				}
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
	return ret;
}

bm_status_t check_bm_vpss_image_param(
	int                     frame_number,
	bm_image*               input,
	bm_image*               output,
	bmcv_rect_t*            input_crop_rect,
	bmcv_padding_attr_t*    padding_attr,
	bmcv_border*            border_param)
{
	bm_status_t ret = BM_SUCCESS;
	int frame_idx = 0;
	bmcv_rect_t  src_crop_rect, dst_crop_rect;


	if((frame_number > VPSS_MAX_GRP_NUM) || (frame_number <= 0))
	{
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
			" input num should less than %d  %s: %s: %d\n",
			VPSS_MAX_GRP_NUM, filename(__FILE__), __func__, __LINE__);
		return BM_NOT_SUPPORTED;
	}
	for (frame_idx = 0; frame_idx < frame_number; frame_idx++)
	{
		if(NULL == input_crop_rect)
		{
			src_crop_rect.start_x = 0;
			src_crop_rect.start_y = 0;
			src_crop_rect.crop_w  = input[frame_idx].width;
			src_crop_rect.crop_h  = input[frame_idx].height;
		}
		else{
			src_crop_rect = input_crop_rect[frame_idx];
		}

		if(NULL == padding_attr)
		{
			dst_crop_rect.start_x = 0;
			dst_crop_rect.start_y = 0;
			dst_crop_rect.crop_w  = output[frame_idx].width;
			dst_crop_rect.crop_h  = output[frame_idx].height;
		}
		else{
			if((padding_attr[frame_idx].if_memset != 0) && (padding_attr[frame_idx].if_memset != 1))
			{
				bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
					"frame [%d], padding_attr if_memset wrong  %s: %s: %d\n",
					frame_idx, filename(__FILE__), __func__, __LINE__);
				return BM_ERR_PARAM;
			}
			if(padding_attr[frame_idx].if_memset == 1) {
				dst_crop_rect.start_x = 0;
				dst_crop_rect.start_y = 0;
				dst_crop_rect.crop_w  = output[frame_idx].width;
				dst_crop_rect.crop_h  = output[frame_idx].height;
			} else if(padding_attr[frame_idx].if_memset == 0) {
				dst_crop_rect.start_x = padding_attr[frame_idx].dst_crop_stx;
				dst_crop_rect.start_y = padding_attr[frame_idx].dst_crop_sty;
				dst_crop_rect.crop_w  = padding_attr[frame_idx].dst_crop_w;
				dst_crop_rect.crop_h  = padding_attr[frame_idx].dst_crop_h;
			}
		}

		if((input[frame_idx].width  > VPSS_MAX_W) ||
			 (input[frame_idx].height > VPSS_MAX_H) ||
			 (input[frame_idx].height < VPSS_MIN_H) ||
			 (input[frame_idx].width  < VPSS_MIN_W) ||
			 (src_crop_rect.crop_w > VPSS_MAX_W) ||
			 (src_crop_rect.crop_h > VPSS_MAX_H) ||
			 (src_crop_rect.crop_w < VPSS_MIN_W) ||
			 (src_crop_rect.crop_h < VPSS_MIN_H) ||
			 (output[frame_idx].width  > VPSS_MAX_W) ||
			 (output[frame_idx].height > VPSS_MAX_H) ||
			 (output[frame_idx].width  < VPSS_MIN_W) ||
			 (output[frame_idx].height < VPSS_MIN_H) ||
			 (dst_crop_rect.crop_w > VPSS_MAX_W) ||
			 (dst_crop_rect.crop_h > VPSS_MAX_H) ||
			 (dst_crop_rect.crop_w < VPSS_MIN_W) ||
			 (dst_crop_rect.crop_h < VPSS_MIN_H) )
		{
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,\
				"bm_vpss frame_idx %d, width or height abnormal,"
				"input[frame_idx].width %d,input[frame_idx].height %d,"
				"src_crop_rect[frame_idx].crop_w %d,src_crop_rect[frame_idx].crop_h %d,"
				"output[frame_idx].width %d, output[frame_idx].height %d,"
				"dst_crop_rect[frame_idx].crop_w %d, dst_crop_rect[frame_idx].crop_h %d,"
				"%s: %s: %d\n",\
				frame_idx,input[frame_idx].width,input[frame_idx].height,src_crop_rect.crop_w,
				src_crop_rect.crop_h, output[frame_idx].width, output[frame_idx].height,
				dst_crop_rect.crop_w,dst_crop_rect.crop_h,
				filename(__FILE__), __func__, __LINE__);
			return BM_NOT_SUPPORTED;
		}
#if 0
		if(((!is_full_image(output[frame_idx].image_format)) && (output[frame_idx].width % 2 != 0)) ||
			(is_yuv420_image(output[frame_idx].image_format) && (output[frame_idx].height % 2 != 0))){
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,\
				"bm_vpss frame_idx(%d), width or height not be 2 align,"
				"output[frame_idx].width(%d),output[frame_idx].height(%d),"
				"output[frame_idx].image_format(%d),"
				"%s: %s: %d\n",\
				frame_idx,output[frame_idx].width,output[frame_idx].height,
				output[frame_idx].image_format,
				filename(__FILE__), __func__, __LINE__);
			return BM_NOT_SUPPORTED;
		}
#endif
		if((src_crop_rect.start_x < 0) ||
			 (src_crop_rect.start_y < 0) ||
			 (dst_crop_rect.start_x < 0) ||
			 (dst_crop_rect.start_y < 0) ||
			 (src_crop_rect.start_x + src_crop_rect.crop_w > input[frame_idx].width) ||
			 (src_crop_rect.start_y + src_crop_rect.crop_h > input[frame_idx].height) ||
			 (dst_crop_rect.start_x + dst_crop_rect.crop_w > output[frame_idx].width) ||
			 (dst_crop_rect.start_y + dst_crop_rect.crop_h > output[frame_idx].height)
		)
		{
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
				"frame [%d], input or output  crop is out of range  %s: %s: %d\n",
				frame_idx, filename(__FILE__), __func__, __LINE__);
			return BM_NOT_SUPPORTED;
		}

		if(NULL != border_param)
		{
			for(int i = 0; i < border_param[frame_idx].border_num; i++){
				if(border_param[frame_idx].border_cfg[i].rect_border_enable == 1)
				{
					if((border_param[frame_idx].border_cfg[i].st_x > input[frame_idx].width) ||
						(border_param[frame_idx].border_cfg[i].st_y > input[frame_idx].height) ||
						(output[frame_idx].data_type != DATA_TYPE_EXT_1N_BYTE))
					{
						bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
							"bm_vpss draw rectangle param wrong, frame(%d) idx(%d) maybe st_x(%d),st_y(%d) wrong or data_type(%d) not supported, %s: %s: %d\n",
							frame_idx, i, border_param[frame_idx].border_cfg[i].st_x, border_param[frame_idx].border_cfg[i].st_y, output[frame_idx].data_type, filename(__FILE__), __func__, __LINE__);
						return BM_ERR_PARAM;
					}
				}
			}
		}
	}

	return ret;
}

bm_status_t check_bm_vpss_param(
	int                     frame_number,
	bm_image*               input,
	bm_image*               output,
	bmcv_rect_t*            input_crop_rect,
	bmcv_padding_attr_t*    padding_attr,
	bmcv_resize_algorithm   algorithm,
	csc_type_t*             csc_type,
	csc_matrix_t*           matrix,
	bmcv_border*            border_param,
	bmcv_csc_cfg*           csc_cfg)
{

	bm_status_t ret = BM_SUCCESS;

	if((input == NULL) || (output == NULL))
	{
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
			"input or output is nullptr , %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
		return BM_ERR_PARAM;
	}

	ret = check_bm_vpss_bm_image_param(frame_number, input, output);
	if(ret != BM_SUCCESS)
	{
		return ret;
	}

	ret = check_bm_vpss_image_param(frame_number, input, output, input_crop_rect, padding_attr, border_param);
	if(ret != BM_SUCCESS)
	{
		return ret;
	}

	if((algorithm != BMCV_INTER_NEAREST) && (algorithm != BMCV_INTER_LINEAR) && (algorithm != BMCV_INTER_BICUBIC))
	{
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
			"bm_vpss not support algorithm %d,%s: %s: %d\n",
			algorithm, filename(__FILE__), __func__, __LINE__);
		return BM_NOT_SUPPORTED;
	}

	ret = check_bm_vpss_csctype(frame_number, input,output, csc_type, matrix, csc_cfg);
	if(ret != BM_SUCCESS)
	{
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
			"bm_vpss csctype %d, %s: %s: %d\n",
			*csc_type, filename(__FILE__), __func__, __LINE__);
		return ret;
	}

	return BM_SUCCESS;
}

bm_status_t check_bm_vpss_continuity(
	int                     frame_number,
	bm_image*               input_or_output)
{
	for (int i = 0; i < frame_number; i++)
	{
		if(input_or_output[i].image_private == NULL)
		{
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
				"bm_image image_private cannot be empty, %s: %s: %d\n",
				filename(__FILE__), __func__, __LINE__);
			return BM_NOT_SUPPORTED;
		}
	}
	return BM_SUCCESS;
}

bm_status_t bm_image_format_to_cvi(bm_image_format_ext fmt, bm_image_data_format_ext datatype, PIXEL_FORMAT_E * cvi_fmt){
	if(datatype != DATA_TYPE_EXT_1N_BYTE){
		switch(datatype){
			case DATA_TYPE_EXT_1N_BYTE_SIGNED:
				*cvi_fmt = PIXEL_FORMAT_INT8_C3_PLANAR;
				break;
			case DATA_TYPE_EXT_FLOAT32:
				*cvi_fmt = PIXEL_FORMAT_FP32_C3_PLANAR;
				break;
			case DATA_TYPE_EXT_FP16:
				*cvi_fmt = PIXEL_FORMAT_FP16_C3_PLANAR;
				break;
			case DATA_TYPE_EXT_BF16:
				*cvi_fmt = PIXEL_FORMAT_BF16_C3_PLANAR;
				break;
			default:
				bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "datatype(%d) not supported, %s: %s: %d\n", datatype, filename(__FILE__), __func__, __LINE__);
				return BM_NOT_SUPPORTED;
		}
		return BM_SUCCESS;
	}
	switch(fmt){
		case FORMAT_YUV420P:
		case FORMAT_COMPRESSED:
			*cvi_fmt = PIXEL_FORMAT_YUV_PLANAR_420;
			break;
		case FORMAT_YUV422P:
			*cvi_fmt = PIXEL_FORMAT_YUV_PLANAR_422;
			break;
		case FORMAT_YUV444P:
			*cvi_fmt = PIXEL_FORMAT_YUV_PLANAR_444;
			break;
		case FORMAT_NV12:
			*cvi_fmt = PIXEL_FORMAT_NV12;
			break;
		case FORMAT_NV21:
			*cvi_fmt = PIXEL_FORMAT_NV21;
			break;
		case FORMAT_NV16:
			*cvi_fmt = PIXEL_FORMAT_NV16;
			break;
		case FORMAT_NV61:
			*cvi_fmt = PIXEL_FORMAT_NV61;
			break;
		case FORMAT_RGBP_SEPARATE:
		case FORMAT_RGB_PLANAR:
			*cvi_fmt = PIXEL_FORMAT_RGB_888_PLANAR;
			break;
		case FORMAT_BGRP_SEPARATE:
		case FORMAT_BGR_PLANAR:
			*cvi_fmt = PIXEL_FORMAT_BGR_888_PLANAR;
			break;
		case FORMAT_RGB_PACKED:
			*cvi_fmt = PIXEL_FORMAT_RGB_888;
			break;
		case FORMAT_BGR_PACKED:
			*cvi_fmt = PIXEL_FORMAT_BGR_888;
			break;
		case FORMAT_HSV_PLANAR:
			*cvi_fmt = PIXEL_FORMAT_HSV_888_PLANAR;
			break;
		case FORMAT_ARGB_PACKED:
			*cvi_fmt = PIXEL_FORMAT_ARGB_8888;
			break;
		case FORMAT_YUV444_PACKED:
			*cvi_fmt = PIXEL_FORMAT_YUV_444;
			break;
		case FORMAT_GRAY:
			*cvi_fmt = PIXEL_FORMAT_YUV_400;
			break;
		case FORMAT_YUV422_YUYV:
			*cvi_fmt = PIXEL_FORMAT_YUYV;
			break;
		case FORMAT_YUV422_YVYU:
			*cvi_fmt = PIXEL_FORMAT_YVYU;
			break;
		case FORMAT_YUV422_UYVY:
			*cvi_fmt = PIXEL_FORMAT_UYVY;
			break;
		case FORMAT_YUV422_VYUY:
			*cvi_fmt = PIXEL_FORMAT_VYUY;
			break;
		case FORMAT_HSV180_PACKED:
			*cvi_fmt = PIXEL_FORMAT_HSV_888;
			break;
		default:
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "fmt(%d) not supported, %s: %s: %d\n", fmt, filename(__FILE__), __func__, __LINE__);
			return BM_NOT_SUPPORTED;
	}
	return BM_SUCCESS;
}

bm_status_t bm_csc_type_to_cvi(csc_type_t csc, COLOR_GAMUT_E * cvi_csc){
	switch (csc)
	{
	case CSC_YCbCr2RGB_BT601:
	case CSC_YPbPr2RGB_BT601:
	case CSC_RGB2YCbCr_BT601:
	case CSC_RGB2YPbPr_BT601:
	case CSC_MAX_ENUM:
		*cvi_csc = COLOR_GAMUT_BT601;
		break;
	case CSC_YCbCr2RGB_BT709:
	case CSC_YPbPr2RGB_BT709:
	case CSC_RGB2YCbCr_BT709:
	case CSC_RGB2YPbPr_BT709:
		*cvi_csc = COLOR_GAMUT_BT709;
		break;
	case CSC_USER_DEFINED_MATRIX:
		*cvi_csc = COLOR_GAMUT_USER;
	default:
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "csc_type(%d) not supported, %s: %s: %d\n", csc, filename(__FILE__), __func__, __LINE__);
		return BM_NOT_SUPPORTED;
	}
	return BM_SUCCESS;
}

bm_status_t bm_algorithm_to_cvi(bmcv_resize_algorithm algorithm, VPSS_SCALE_COEF_E * enCoef){
	switch (algorithm)
	{
	case BMCV_INTER_NEAREST:
		*enCoef = VPSS_SCALE_COEF_NEAREST;
		break;
	case BMCV_INTER_LINEAR:
		*enCoef = VPSS_SCALE_COEF_BILINEAR;
		break;
	case BMCV_INTER_BICUBIC:
		*enCoef = VPSS_SCALE_COEF_BICUBIC_OPENCV;
		break;
	default:
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "algorithm(%d) not supported, %s: %s: %d\n", algorithm, filename(__FILE__), __func__, __LINE__);
		return BM_NOT_SUPPORTED;
	}
	return BM_SUCCESS;
}

bm_status_t bm_send_image_frame(bm_image image, VIDEO_FRAME_INFO_S *stVideoFrame, PIXEL_FORMAT_E enPixelFormat)
{
	int planar_to_seperate = 0;
	if((image.image_format == FORMAT_RGB_PLANAR || image.image_format == FORMAT_BGR_PLANAR))
		planar_to_seperate = 1;

	stVideoFrame->stVFrame.enCompressMode = COMPRESS_MODE_NONE;
	stVideoFrame->stVFrame.enPixelFormat = enPixelFormat;
	stVideoFrame->stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
	stVideoFrame->stVFrame.u32Width = image.width;
	stVideoFrame->stVFrame.u32Height = image.height;
	if((!is_full_image(image.image_format)) || image.image_format == FORMAT_COMPRESSED)
		stVideoFrame->stVFrame.u32Width = stVideoFrame->stVFrame.u32Width & (~0x1);
	if(is_yuv420_image(image.image_format) || image.image_format == FORMAT_COMPRESSED)
		stVideoFrame->stVFrame.u32Height = stVideoFrame->stVFrame.u32Height & (~0x1);
	for (int i = 0; i < image.image_private->plane_num; ++i) {
		if((i == 3) && (image.image_format == FORMAT_COMPRESSED)){
			stVideoFrame->stVFrame.u64ExtPhyAddr = image.image_private->data[i].u.device.device_addr;
			stVideoFrame->stVFrame.u32ExtLength = image.image_private->memory_layout[i].size;
			stVideoFrame->stVFrame.pu8ExtVirtAddr = (unsigned char*)image.image_private->data[i].u.system.system_addr;
			stVideoFrame->stVFrame.enCompressMode = COMPRESS_MODE_FRAME;
		} else if((i > 0) && (image.image_format == FORMAT_COMPRESSED)){
			continue;
		} else {
			stVideoFrame->stVFrame.u32Stride[i] = image.image_private->memory_layout[i].pitch_stride;
			stVideoFrame->stVFrame.u32Length[i] = image.image_private->memory_layout[i].size;
			stVideoFrame->stVFrame.u64PhyAddr[i] = image.image_private->data[i].u.device.device_addr;
			stVideoFrame->stVFrame.pu8VirAddr[i] = (unsigned char*)image.image_private->data[i].u.system.system_addr;
		}
	}

	if(planar_to_seperate){
		for (int i = 1; i < 3; ++i) {
			stVideoFrame->stVFrame.u32Stride[i] = image.image_private->memory_layout[0].pitch_stride;
			stVideoFrame->stVFrame.u32Length[i] = image.image_private->memory_layout[0].pitch_stride * image.height;
			stVideoFrame->stVFrame.u64PhyAddr[i] = image.image_private->data[0].u.device.device_addr + image.image_private->memory_layout[0].pitch_stride * image.height * i;
		}
	}

	if(image.image_format == FORMAT_COMPRESSED){
		for (int i = 1, j = 2; i < 3; ++i, --j) {
			stVideoFrame->stVFrame.u32Stride[i] = image.image_private->memory_layout[j].pitch_stride;
			stVideoFrame->stVFrame.u32Length[i] = image.image_private->memory_layout[j].size;
			stVideoFrame->stVFrame.u64PhyAddr[i] = image.image_private->data[j].u.device.device_addr;
			stVideoFrame->stVFrame.pu8VirAddr[i] = (unsigned char*)image.image_private->data[j].u.system.system_addr;
		}
	}

	if((enPixelFormat == PIXEL_FORMAT_UINT8_C3_PLANAR || enPixelFormat == PIXEL_FORMAT_INT8_C3_PLANAR ||
		enPixelFormat == PIXEL_FORMAT_BF16_C3_PLANAR || enPixelFormat == PIXEL_FORMAT_FP16_C3_PLANAR || enPixelFormat == PIXEL_FORMAT_FP32_C3_PLANAR) &&
		(image.image_format == FORMAT_BGR_PLANAR || image.image_format == FORMAT_BGRP_SEPARATE)){
		stVideoFrame->stVFrame.u32Stride[0] = stVideoFrame->stVFrame.u32Stride[2];
		stVideoFrame->stVFrame.u32Length[0] = stVideoFrame->stVFrame.u32Length[2];
		stVideoFrame->stVFrame.u64PhyAddr[0] = stVideoFrame->stVFrame.u64PhyAddr[2];
		stVideoFrame->stVFrame.u32Stride[2] = image.image_private->memory_layout[0].pitch_stride;
		stVideoFrame->stVFrame.u32Length[2] = image.image_private->memory_layout[0].pitch_stride * image.height;
		stVideoFrame->stVFrame.u64PhyAddr[2] = image.image_private->data[0].u.device.device_addr;
	}

	stVideoFrame->stVFrame.u32Align = 1;
	stVideoFrame->stVFrame.u32TimeRef = 0;
	stVideoFrame->stVFrame.u64PTS = 0;
	stVideoFrame->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;
	stVideoFrame->u32PoolId = 0xffff;

	return BM_SUCCESS;
}

bm_status_t bm_vpss_set_grp_csc(u8 is_fancy, bmcv_vpss_csc_matrix *csc_cfg, struct vpss_grp_csc_cfg *cfg){
	for (u8 i = 0; i < 3; i++) {
		for (u8 j = 0; j < 3; j++)
			cfg->coef[i][j] = csc_cfg->coef[i][j];
		cfg->add[i] = csc_cfg->add[i];
		cfg->sub[i] = csc_cfg->sub[i];
	}

	if(!is_fancy)
		cfg->is_copy_upsample = true;

	cfg->enable = true;

	return BM_SUCCESS;
}

bm_status_t bm_vpss_set_chn_csc(bmcv_vpss_csc_matrix *csc_cfg, struct vpss_chn_csc_cfg *cfg){
	for (u8 i = 0; i < 3; i++) {
		for (u8 j = 0; j < 3; j++)
			cfg->coef[i][j] = csc_cfg->coef[i][j];
		cfg->add[i] = csc_cfg->add[i];
		cfg->sub[i] = csc_cfg->sub[i];
	}

	cfg->enable = true;

	return BM_SUCCESS;
}

bm_status_t bm_vpss_set_csc(bmcv_csc_cfg *csc_cfg, bm_vpss_cfg *vpss_cfg){
	bmcv_vpss_csc_matrix csc_matrix;
	if(csc_cfg->csc_type == VPSS_CSC_RGB2RGB)
		return BM_SUCCESS;
	if(csc_cfg->is_user_defined_matrix){
		csc_matrix = csc_cfg->csc_matrix;
		if(csc_cfg->csc_type == VPSS_CSC_RGB2YCbCr_BT601 || csc_cfg->csc_type == VPSS_CSC_RGB2YCbCr_BT709 || csc_cfg->csc_type == VPSS_CSC_RGB2YPbPr_BT601 || csc_cfg->csc_type == VPSS_CSC_RGB2YPbPr_BT709)
			bm_vpss_set_chn_csc(&csc_matrix, &vpss_cfg->chn_csc_cfg);
		else
			bm_vpss_set_grp_csc(csc_cfg->is_fancy, &csc_matrix, &vpss_cfg->grp_csc_cfg);
	} else {
		if(csc_cfg->csc_type <= VPSS_CSC_RGB2YPbPr_BT709){
			csc_matrix = csc_default_matrix[csc_cfg->csc_type];
			if(csc_cfg->csc_type == VPSS_CSC_YCbCr2RGB_BT601 || csc_cfg->csc_type == VPSS_CSC_YCbCr2RGB_BT709 || csc_cfg->csc_type == VPSS_CSC_YPbPr2RGB_BT601 || csc_cfg->csc_type == VPSS_CSC_YPbPr2RGB_BT709)
				bm_vpss_set_grp_csc(csc_cfg->is_fancy, &csc_matrix, &vpss_cfg->grp_csc_cfg);
			else
				bm_vpss_set_chn_csc(&csc_matrix, &vpss_cfg->chn_csc_cfg);
		} else if (csc_cfg->csc_type == VPSS_CSC_YCbCr2YCbCr_BT601){
				bm_vpss_set_grp_csc(csc_cfg->is_fancy, &csc_default_matrix[VPSS_CSC_YCbCr2RGB_BT601], &vpss_cfg->grp_csc_cfg);
				bm_vpss_set_chn_csc(&csc_default_matrix[VPSS_CSC_RGB2YCbCr_BT601], &vpss_cfg->chn_csc_cfg);
		} else if (csc_cfg->csc_type == VPSS_CSC_YCbCr2YCbCr_BT709){
				bm_vpss_set_grp_csc(csc_cfg->is_fancy, &csc_default_matrix[VPSS_CSC_YCbCr2RGB_BT601], &vpss_cfg->grp_csc_cfg);
				bm_vpss_set_chn_csc(&csc_default_matrix[VPSS_CSC_RGB2YCbCr_BT709], &vpss_cfg->chn_csc_cfg);
		} else if (csc_cfg->csc_type == VPSS_CSC_YPbPr2YPbPr_BT601){
				bm_vpss_set_grp_csc(csc_cfg->is_fancy, &csc_default_matrix[VPSS_CSC_YPbPr2RGB_BT601], &vpss_cfg->grp_csc_cfg);
				bm_vpss_set_chn_csc(&csc_default_matrix[VPSS_CSC_RGB2YPbPr_BT601], &vpss_cfg->chn_csc_cfg);
		} else if (csc_cfg->csc_type == VPSS_CSC_YPbPr2YPbPr_BT709){
				bm_vpss_set_grp_csc(csc_cfg->is_fancy, &csc_default_matrix[VPSS_CSC_YPbPr2RGB_BT709], &vpss_cfg->grp_csc_cfg);
				bm_vpss_set_chn_csc(&csc_default_matrix[VPSS_CSC_RGB2YPbPr_BT709], &vpss_cfg->chn_csc_cfg);
		}
	}
	return BM_SUCCESS;
}

bm_status_t bm_vpss_set_chn_draw_rect(bmcv_border* border_param, struct vpss_chn_draw_rect_cfg *draw_cfg){
	for(int i = 0; i < border_param->border_num; i++){
		draw_cfg->stDrawRect.astRect[i].bEnable = true;
		draw_cfg->stDrawRect.astRect[i].stRect.s32X = border_param->border_cfg[i].st_x;
		draw_cfg->stDrawRect.astRect[i].stRect.s32Y = border_param->border_cfg[i].st_y;
		draw_cfg->stDrawRect.astRect[i].stRect.u32Width = border_param->border_cfg[i].width;
		draw_cfg->stDrawRect.astRect[i].stRect.u32Height = border_param->border_cfg[i].height;
		draw_cfg->stDrawRect.astRect[i].u16Thick = border_param->border_cfg[i].thickness;
		draw_cfg->stDrawRect.astRect[i].u32BgColor = ((border_param->border_cfg[i].value_b) | (border_param->border_cfg[i].value_g << 8) | (border_param->border_cfg[i].value_r << 16));
	}
	return BM_SUCCESS;
}

bm_status_t bm_vpss_set_convertto(bmcv_convert_to_attr convertto_attr, struct vpss_chn_convert_cfg *cfg)
{
	cfg->stConvert.bEnable = true;
	cfg->stConvert.u32aFactor[0] = convertto_attr.alpha_0 < 0 ? ((u32)(-convertto_attr.alpha_0 * 8192) | (1 << 25)) : (u32)(convertto_attr.alpha_0 * 8192);
	cfg->stConvert.u32aFactor[1] = convertto_attr.alpha_1 < 0 ? ((u32)(-convertto_attr.alpha_1 * 8192) | (1 << 25)) : (u32)(convertto_attr.alpha_1 * 8192);
	cfg->stConvert.u32aFactor[2] = convertto_attr.alpha_2 < 0 ? ((u32)(-convertto_attr.alpha_2 * 8192) | (1 << 25)) : (u32)(convertto_attr.alpha_2 * 8192);
	cfg->stConvert.u32bFactor[0] = convertto_attr.beta_0 < 0 ? ((u32)(-convertto_attr.beta_0 * 8192) | (1 << 25)) : (u32)(convertto_attr.beta_0 * 8192);
	cfg->stConvert.u32bFactor[1] = convertto_attr.beta_1 < 0 ? ((u32)(-convertto_attr.beta_1 * 8192) | (1 << 25)) : (u32)(convertto_attr.beta_1 * 8192);
	cfg->stConvert.u32bFactor[2] = convertto_attr.beta_2 < 0 ? ((u32)(-convertto_attr.beta_2 * 8192) | (1 << 25)) : (u32)(convertto_attr.beta_2 * 8192);

	return BM_SUCCESS;
}

bm_status_t bm_vpss_chn_set_gop(bmcv_rgn_cfg* gop_attr, struct cvi_rgn_cfg *cfg){
	unsigned char layer_num = (gop_attr->rgn_num + 7) >> 3;
	unsigned char gop_num = 0;
	for(int i = 0; i < layer_num; i++){
		gop_num = (i == layer_num - 1) ? (gop_attr->rgn_num - (i << 3)) : 8;
		cfg[i].num_of_rgn = gop_num;
		for(int j = 0; j < gop_num; j++)
			cfg[i].param[j] = gop_attr->param[(i * 8 + j)];
	}
	return BM_SUCCESS;
}

// static void dump_vpss_param(
// 	int frame_idx,
// 	int grp_id,
// 	int chn,
// 	bm_image* input,
// 	bm_image* output,
// 	VPSS_GRP_ATTR_S stVpssGrpAttr,
// 	VPSS_CHN_ATTR_S astVpssChnAttr,
// 	VPSS_CROP_INFO_S pstCropInfo,
// 	bmcv_resize_algorithm algorithm,
// 	bmcv_csc_cfg* csc_cfg,
// 	bmcv_convert_to_attr* convert_to_attr,
// 	bmcv_border* border_param,
// 	coverex_cfg* coverex_param,
// 	bmcv_rgn_cfg* gop_attr){

// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " %s: %s: %d\n", __FILE__, __func__, __LINE__);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " frame_idx %d, grp_id %d, chn %d\n", frame_idx, grp_id, chn);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " stVpssGrpAttr.u32MaxW %d, stVpssGrpAttr.u32MaxH %d\n", stVpssGrpAttr.u32MaxW, stVpssGrpAttr.u32MaxH);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " stVpssGrpAttr.enPixelFormat %d\n", stVpssGrpAttr.enPixelFormat);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " input.data_type %d\n", input->data_type);
// 	for(int i = 0; i < input->image_private->plane_num; i++)
// 		bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " plane idx %d, input.device_addr %lx, input.stride %d\n", i, input->image_private->data[i].u.device.device_addr, input->image_private->memory_layout[i].pitch_stride);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " astVpssChnAttr.u32Width %d, astVpssChnAttr.u32Height %d\n", astVpssChnAttr.u32Width, astVpssChnAttr.u32Height);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " astVpssChnAttr.u32Depth %d, astVpssChnAttr.stNormalize.bEnable %d\n", astVpssChnAttr.u32Depth, astVpssChnAttr.stNormalize.bEnable);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " astVpssChnAttr.stNormalize.factor[0] %f, mean[0] %f\n", astVpssChnAttr.stNormalize.factor[0], astVpssChnAttr.stNormalize.mean[0]);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " astVpssChnAttr.stNormalize.factor[1] %f, mean[1] %f\n", astVpssChnAttr.stNormalize.factor[1], astVpssChnAttr.stNormalize.mean[1]);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " astVpssChnAttr.stNormalize.factor[2] %f, mean[2] %f\n", astVpssChnAttr.stNormalize.factor[2], astVpssChnAttr.stNormalize.mean[2]);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " astVpssChnAttr.stAspectRatio.enMode %d, bEnableBgColor %d\n", astVpssChnAttr.stAspectRatio.enMode, astVpssChnAttr.stAspectRatio.bEnableBgColor);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " astVpssChnAttr.stAspectRatio.stVideoRect.s32X %d, s32Y %d\n", astVpssChnAttr.stAspectRatio.stVideoRect.s32X, astVpssChnAttr.stAspectRatio.stVideoRect.s32Y);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " astVpssChnAttr.stAspectRatio.stVideoRect.u32Width %d, u32Height %d\n", astVpssChnAttr.stAspectRatio.stVideoRect.u32Width, astVpssChnAttr.stAspectRatio.stVideoRect.u32Height);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " astVpssChnAttr.stAspectRatio.u32BgColor %lx, astVpssChnAttr.enPixelFormat %d\n", astVpssChnAttr.stAspectRatio.u32BgColor, astVpssChnAttr.enPixelFormat);
// 	for(int i = 0; i < output->image_private->plane_num; i++)
// 		bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " plane idx %d, output.device_addr %lx, output.stride %d\n", i, output->image_private->data[i].u.device.device_addr, output->image_private->memory_layout[i].pitch_stride);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " astVpssChnAttr.bMirror %d, astVpssChnAttr.bFlip %d\n", astVpssChnAttr.bMirror, astVpssChnAttr.bFlip);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " pstCropInfo.bEnable %d, enCropCoordinate %d\n", pstCropInfo.bEnable, pstCropInfo.enCropCoordinate);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " pstCropInfo.stCropRect.s32X %d, s32Y %d\n", pstCropInfo.stCropRect.s32X, pstCropInfo.stCropRect.s32Y);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " pstCropInfo.stCropRect.u32Width %d, u32Height %d\n", pstCropInfo.stCropRect.u32Width, pstCropInfo.stCropRect.u32Height);
// 	bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " algorithm %d, csc_type %d is_fancy %d\n", algorithm, csc_cfg->csc_type, csc_cfg->is_fancy);
// 	if(csc_cfg->is_user_defined_matrix){
// 		bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " matrix.csc_coe00 %d, csc_coe01 %d\n", csc_cfg->csc_matrix.coef[0][0], csc_cfg->csc_matrix.coef[0][1]);
// 		bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " matrix.csc_coe02 %d, csc_coe10 %d\n", csc_cfg->csc_matrix.coef[0][2], csc_cfg->csc_matrix.coef[1][0]);
// 		bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " matrix.csc_coe11 %d, csc_coe12 %d\n", csc_cfg->csc_matrix.coef[1][1], csc_cfg->csc_matrix.coef[1][2]);
// 		bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " matrix.csc_coe20 %d, csc_coe21 %d\n", csc_cfg->csc_matrix.coef[2][0], csc_cfg->csc_matrix.coef[2][1]);
// 		bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " matrix.csc_coe22 %d, csc_add0 %d\n", csc_cfg->csc_matrix.coef[2][2], csc_cfg->csc_matrix.add[0]);
// 		bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " matrix.csc_add1 %d, csc_add2 %d\n", csc_cfg->csc_matrix.add[1], csc_cfg->csc_matrix.add[2]);
// 		bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " matrix.csc_sub0 %d, csc_sub1 %d\n", csc_cfg->csc_matrix.add[0], csc_cfg->csc_matrix.sub[1]);
// 		bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " matrix.csc_sub2 %d\n", csc_cfg->csc_matrix.sub[2]);
// 	}
// 	if(convert_to_attr){
// 		convert_to_attr += frame_idx;
// 		bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " convert_to_attr.alpha_0 %f, beta_0 %f\n", convert_to_attr->alpha_0, convert_to_attr->beta_0);
// 		bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " convert_to_attr.alpha_1 %f, beta_1 %f\n", convert_to_attr->alpha_1, convert_to_attr->beta_1);
// 		bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " convert_to_attr.alpha_2 %f, beta_2 %f\n", convert_to_attr->alpha_2, convert_to_attr->beta_2);
// 	}
// 	if(border_param){
// 		border_param += frame_idx;
// 		for(int i = 0; i < 4; i++){
// 			if(border_param->border_cfg[i].rect_border_enable){
// 				bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " border_idx %d border_param.rect_border_enable %d, st_x %d\n", i, border_param->border_cfg[i].rect_border_enable, border_param->border_cfg[i].st_x);
// 				bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " border_idx %d border_param.st_y %d, width %d\n", i, border_param->border_cfg[i].st_y, border_param->border_cfg[i].width);
// 				bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " border_idx %d border_param.height %d, value_r %d\n", i, border_param->border_cfg[i].height, border_param->border_cfg[i].value_r);
// 				bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " border_idx %d border_param.value_g %d, value_b %d\n", i, border_param->border_cfg[i].value_g, border_param->border_cfg[i].value_b);
// 				bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " border_idx %d border_param.thickness %d\n", i, border_param->border_cfg[i].thickness);
// 			}
// 		}
// 	}
// 	if(coverex_param){
// 		coverex_param += frame_idx;
// 		for(int i = 0; i < coverex_param->cover_num; i++){
// 			bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " cover_idx %d coverex_param.enable %d, color %lx\n", i, coverex_param->coverex_param[i].enable, coverex_param->coverex_param[i].color);
// 			bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " cover_idx %d coverex_param.left %d, width %d\n", i, coverex_param->coverex_param[i].rect.left, coverex_param->coverex_param[i].rect.width);
// 			bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " cover_idx %d coverex_param.top %d, height %d\n", i, coverex_param->coverex_param[i].rect.top, coverex_param->coverex_param[i].rect.height);
// 		}
// 	}
// 	if(gop_attr){
// 		gop_attr += frame_idx;
// 		for(int i = 0; i < gop_attr->rgn_num; i++){
// 			bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " gop_idx %d param.fmt %d, stride %d\n", i, gop_attr->param[i].fmt, gop_attr->param[i].stride);
// 			bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " gop_idx %d param.phy_addr %lx\n", i, gop_attr->param[i].phy_addr);
// 			bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " gop_idx %d param.left %d, top %d\n", i, gop_attr->param[i].rect.left, gop_attr->param[i].rect.top);
// 			bmlib_log("BMCV VPSS DUMP", BMLIB_LOG_ERROR, " gop_idx %d param.width %d, height %d\n", i, gop_attr->param[i].rect.width, gop_attr->param[i].rect.height);
// 		}
// 	}
// }

bm_status_t bm_vpss_asic(
	bm_handle_t             handle,
	int                     frame_number,
	bm_image*               input,
	bm_image*               output,
	bmcv_rect_t*            input_crop_rect,
	bmcv_padding_attr_t*    padding_attr,
	bmcv_resize_algorithm   algorithm,
	bmcv_csc_cfg*           csc_cfg,
	bmcv_convert_to_attr*   convert_to_attr,
	bmcv_border*            border_param,
	coverex_cfg*            coverex_param,
	bmcv_rgn_cfg*           gop_attr)
{
	bm_status_t ret = BM_SUCCESS;
	int fd = 0;
	bm_vpss_cfg vpss_cfg;
	VPSS_CROP_INFO_S pstCropInfo;
	ret = bm_get_vpss_fd(&fd);
	if(ret != BM_SUCCESS) return ret;

	for(int i = 0; i < frame_number; i++) {
		memset(&vpss_cfg, 0, sizeof(bm_vpss_cfg));

		vpss_cfg.grp_attr.stGrpAttr.stFrameRate.s32SrcFrameRate = 0x7fff;
		vpss_cfg.grp_attr.stGrpAttr.stFrameRate.s32DstFrameRate = 0x7fff;
		vpss_cfg.grp_attr.stGrpAttr.u32MaxW = input[i].width;
		vpss_cfg.grp_attr.stGrpAttr.u32MaxH = input[i].height;

		if((!is_full_image(input[i].image_format)) || input[i].image_format == FORMAT_COMPRESSED)
			vpss_cfg.grp_attr.stGrpAttr.u32MaxW = vpss_cfg.grp_attr.stGrpAttr.u32MaxW & (~0x1);
		if(is_yuv420_image(input[i].image_format) || input[i].image_format == FORMAT_COMPRESSED)
			vpss_cfg.grp_attr.stGrpAttr.u32MaxH = vpss_cfg.grp_attr.stGrpAttr.u32MaxH & (~0x1);
		ret = bm_image_format_to_cvi(input[i].image_format, input[i].data_type, &vpss_cfg.grp_attr.stGrpAttr.enPixelFormat);
		if(ret != BM_SUCCESS) return ret;

		vpss_cfg.chn_attr.stChnAttr.u32Width = output[i].width;
		vpss_cfg.chn_attr.stChnAttr.u32Height = output[i].height;
		if(!is_full_image(output[i].image_format))
			vpss_cfg.chn_attr.stChnAttr.u32Width = vpss_cfg.chn_attr.stChnAttr.u32Width & (~0x1);
		if(is_yuv420_image(output[i].image_format))
			vpss_cfg.chn_attr.stChnAttr.u32Height = vpss_cfg.chn_attr.stChnAttr.u32Height & (~0x1);
		vpss_cfg.chn_attr.stChnAttr.enVideoFormat = VIDEO_FORMAT_LINEAR;
		ret = bm_image_format_to_cvi(output[i].image_format, output[i].data_type, &vpss_cfg.chn_attr.stChnAttr.enPixelFormat);
		if(ret != BM_SUCCESS) return ret;

		if(convert_to_attr != NULL && output[i].data_type == DATA_TYPE_EXT_1N_BYTE)
			vpss_cfg.chn_attr.stChnAttr.enPixelFormat = PIXEL_FORMAT_UINT8_C3_PLANAR;

		vpss_cfg.chn_attr.stChnAttr.stFrameRate.s32SrcFrameRate = 30;
		vpss_cfg.chn_attr.stChnAttr.stFrameRate.s32DstFrameRate = 30;
		vpss_cfg.chn_attr.stChnAttr.u32Depth = 1;
		vpss_cfg.chn_attr.stChnAttr.bMirror = 0;
		vpss_cfg.chn_attr.stChnAttr.bFlip = 0;
		if((padding_attr != NULL) && (padding_attr[i].dst_crop_stx != 0 || padding_attr[i].dst_crop_sty != 0 ||
				padding_attr[i].dst_crop_w != output[i].width || padding_attr[i].dst_crop_h != output[i].width)){
			vpss_cfg.chn_attr.stChnAttr.stAspectRatio.enMode = ASPECT_RATIO_MANUAL;
			vpss_cfg.chn_attr.stChnAttr.stAspectRatio.stVideoRect.s32X = padding_attr[i].dst_crop_stx;
			vpss_cfg.chn_attr.stChnAttr.stAspectRatio.stVideoRect.s32Y = padding_attr[i].dst_crop_sty;
			vpss_cfg.chn_attr.stChnAttr.stAspectRatio.stVideoRect.u32Width = padding_attr[i].dst_crop_w;
			vpss_cfg.chn_attr.stChnAttr.stAspectRatio.stVideoRect.u32Height = padding_attr[i].dst_crop_h;
			if(!is_full_image(output[i].image_format))
				vpss_cfg.chn_attr.stChnAttr.stAspectRatio.stVideoRect.u32Width = vpss_cfg.chn_attr.stChnAttr.stAspectRatio.stVideoRect.u32Width & (~0x1);
			if(is_yuv420_image(output[i].image_format))
				vpss_cfg.chn_attr.stChnAttr.stAspectRatio.stVideoRect.u32Height = vpss_cfg.chn_attr.stChnAttr.stAspectRatio.stVideoRect.u32Height & (~0x1);
			if(padding_attr[i].if_memset == 1){
				vpss_cfg.chn_attr.stChnAttr.stAspectRatio.bEnableBgColor = 1;
				vpss_cfg.chn_attr.stChnAttr.stAspectRatio.u32BgColor = RGB_8BIT(padding_attr[i].padding_r, padding_attr[i].padding_g, padding_attr[i].padding_b);
			}
		}else{
			vpss_cfg.chn_attr.stChnAttr.stAspectRatio.enMode = ASPECT_RATIO_MANUAL;
			vpss_cfg.chn_attr.stChnAttr.stAspectRatio.stVideoRect.s32X = 0;
			vpss_cfg.chn_attr.stChnAttr.stAspectRatio.stVideoRect.s32Y = 0;
			vpss_cfg.chn_attr.stChnAttr.stAspectRatio.stVideoRect.u32Width = vpss_cfg.chn_attr.stChnAttr.u32Width;
			vpss_cfg.chn_attr.stChnAttr.stAspectRatio.stVideoRect.u32Height = vpss_cfg.chn_attr.stChnAttr.u32Height;
		}

		ret = bm_algorithm_to_cvi(algorithm, &vpss_cfg.chn_coef_level_cfg.enCoef);
		if (ret != BM_SUCCESS) return ret;

		bm_vpss_set_csc(csc_cfg, &vpss_cfg);

		if(input_crop_rect != NULL){
			if(input_crop_rect[i].start_x != 0 || input_crop_rect[i].start_y != 0 ||
					input_crop_rect[i].crop_w != input[i].width || input_crop_rect[i].crop_h != input[i].height){
				pstCropInfo.bEnable = true;
				pstCropInfo.enCropCoordinate = VPSS_CROP_ABS_COOR;
				pstCropInfo.stCropRect.s32X = input_crop_rect[i].start_x;
				pstCropInfo.stCropRect.s32Y = input_crop_rect[i].start_y;
				pstCropInfo.stCropRect.u32Width = input_crop_rect[i].crop_w;
				pstCropInfo.stCropRect.u32Height = input_crop_rect[i].crop_h;
				if(pstCropInfo.stCropRect.u32Width > 4608 || pstCropInfo.stCropRect.u32Height > 8189)
					vpss_cfg.chn_crop_cfg.stCropInfo = pstCropInfo;
				else {
					if((!is_full_image(input[i].image_format)) || input[i].image_format == FORMAT_COMPRESSED)
						pstCropInfo.stCropRect.u32Width = pstCropInfo.stCropRect.u32Width & (~0x1);
					if(is_yuv420_image(input[i].image_format) || input[i].image_format == FORMAT_COMPRESSED)
						pstCropInfo.stCropRect.u32Height = pstCropInfo.stCropRect.u32Height & (~0x1);
					vpss_cfg.grp_crop_cfg.stCropInfo = pstCropInfo;
				}
			}
		}

		if(convert_to_attr != NULL){
			bm_vpss_set_convertto(convert_to_attr[i], &vpss_cfg.chn_convert_cfg);
		}

		if(border_param != NULL && border_param[i].border_num > 0) {
			bm_vpss_set_chn_draw_rect(border_param + i, &vpss_cfg.chn_draw_rect_cfg);
		}

		if(coverex_param != NULL) {
			for(int i = 0; i < coverex_param->cover_num; i++)
				vpss_cfg.coverex_cfg.rgn_coverex_cfg.rgn_coverex_param[i] = coverex_param->coverex_param[i];
		}

		if(gop_attr != NULL && gop_attr->rgn_num > 0){
			bm_vpss_chn_set_gop(gop_attr, vpss_cfg.rgn_cfg);
		}

		bm_send_image_frame(input[i], &vpss_cfg.snd_frm_cfg.stVideoFrame, vpss_cfg.grp_attr.stGrpAttr.enPixelFormat);

		vpss_cfg.chn_frm_cfg.s32MilliSec = VPSS_TIMEOUT_MS;

		for(int k = 0; k < 8; k++) {
			bm_send_image_frame(output[i], &vpss_cfg.chn_frm_cfg.stVideoFrame, vpss_cfg.chn_attr.stChnAttr.enPixelFormat);
			ret = (bm_status_t)ioctl(fd, CVI_VPSS_BM_SEND_FRAME, &vpss_cfg);
			if (ret == BM_SUCCESS) break;
			// dump_vpss_param(i, grp_id, VpssChn, input + i, output + i, stVpssGrpAttr, astVpssChnAttr[VpssChn],
			// 	pstCropInfo, algorithm, csc_cfg, convert_to_attr, border_param, coverex_param, gop_attr);
		}
		if (ret != BM_SUCCESS){
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "ret(0x%lx), VPSS_BM_SEND_FRAME fail %s: %s: %d\n", (unsigned long)ret, __FILE__, __func__, __LINE__);
			break;
		}
	}
	return ret;
}

int is_need_width_align_input(bm_image input){
	if (((input.image_private->memory_layout[1].pitch_stride % 16) != 0) && is_yuv420_image(input.image_format))
		return 1;
	if ((input.image_private->data[0].u.device.device_addr % 2 != 0) &&
		(input.image_format == FORMAT_GRAY || is_yuv420_image(input.image_format) ||
		input.image_format == FORMAT_NV16 || input.image_format == FORMAT_NV61 ||
		input.image_format == FORMAT_YUV422_UYVY || input.image_format == FORMAT_YUV422_VYUY ||
		input.image_format == FORMAT_YUV422_YUYV || input.image_format == FORMAT_YUV422_YVYU))
		return 1;
	if ((input.image_private->data[1].u.device.device_addr % 2 != 0) &&
		(input.image_format == FORMAT_NV12 || input.image_format == FORMAT_NV21 ||
		input.image_format == FORMAT_NV16 || input.image_format == FORMAT_NV61))
		return 1;
	if ((input.image_private->memory_layout[0].pitch_stride % 4 != 0) &&
		(input.image_format == FORMAT_YUV422_UYVY || input.image_format == FORMAT_YUV422_VYUY ||
		input.image_format == FORMAT_YUV422_YUYV || input.image_format == FORMAT_YUV422_YVYU))
		return 1;
	return 0;
};

int is_need_width_align_output(bm_image output){
	if (output.image_format == FORMAT_NV12 || output.image_format == FORMAT_NV21 ||
		output.image_format == FORMAT_NV16 || output.image_format == FORMAT_NV61 ||
		output.image_format == FORMAT_YUV422_UYVY || output.image_format == FORMAT_YUV422_VYUY ||
		output.image_format == FORMAT_YUV422_YUYV || output.image_format == FORMAT_YUV422_YVYU)
		for(int i = 0; i < output.image_private->plane_num; i++)
			if(output.image_private->data[i].u.device.device_addr % 2 != 0)
				return 1;
	return 0;
};

bm_status_t fill_image_private(bm_image *res, int *stride);

bm_status_t bm_vpss_multi_parameter_processing(
	bm_handle_t             handle,
	int                     frame_number,
	bm_image*               input,
	bm_image*               output,
	bmcv_rect_t*            input_crop_rect,
	bmcv_padding_attr_t*    padding_attr,
	bmcv_resize_algorithm   algorithm,
	csc_type_t              csc_type,
	csc_matrix_t*           matrix,
	bmcv_convert_to_attr*   convert_to_attr,
	bmcv_border*            border_param,
	coverex_cfg*          	coverex_param,
	bmcv_rgn_cfg*           gop_attr)
{
	bm_status_t ret = BM_SUCCESS;
	bm_image input_align[frame_number];
	bmcv_csc_cfg csc_cfg;
	int output_is_alloc_mem[frame_number];
	int i;

	memset(&csc_cfg, 0, sizeof(bmcv_csc_cfg));

	for (i = 0; i < frame_number; i++) {
		int dst_stride[3];
		bm_image_get_stride(output[i], dst_stride);
		if (output[i].image_private->attached && is_need_width_align_output(output[i])){
			bm_image_detach(output[i]);
		}
		if (!output[i].image_private->attached) {
			int align_stride[3];
			for(int m = 0; m < output[i].image_private->plane_num; m++)
				align_stride[m] = ALIGN(dst_stride[m], 2);
			fill_image_private(output + i, align_stride);
			if (bm_image_alloc_dev_mem(output[i], BMCV_HEAP_ANY) != BM_SUCCESS) {
				bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "output dev alloc fail %s: %s: %d\n", __FILE__, __func__, __LINE__);
				for (int j = 0; j < frame_number; j++)
					bm_image_detach(output[j]);
				return BM_ERR_FAILURE;
			}
			output_is_alloc_mem[i] = 1;
		} else
			output_is_alloc_mem[i] = 0;
	}

	for (i = 0; i < frame_number; i++) {
		if (is_need_width_align_input(input[i])) {
			int src_stride[3];
			int align_stride[3];
			bm_image_get_stride(input[i], src_stride);
			for(int m = 0; m < input[i].image_private->plane_num; m++){
				if((m == 0) && (!(input[i].image_format == FORMAT_NV12 || input[i].image_format == FORMAT_NV21 ||
						input[i].image_format == FORMAT_NV16 || input[i].image_format == FORMAT_NV61 ||
						input[i].image_format == FORMAT_YUV422_UYVY || input[i].image_format == FORMAT_YUV422_VYUY ||
						input[i].image_format == FORMAT_YUV422_YUYV || input[i].image_format == FORMAT_YUV422_YVYU)))
					align_stride[m] = src_stride[m];
				else{
					align_stride[m] = ALIGN(src_stride[m], 16);
				}
			}
			bm_image_create(handle, input[i].height, input[i].width, input[i].image_format, input[i].data_type, input_align + i, align_stride);
			if(bm_image_alloc_dev_mem(input_align[i], BMCV_HEAP_ANY)!=BM_SUCCESS){
				bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "input align alloc fail %s: %s: %d\n", __FILE__, __func__, __LINE__);
				goto fail;
			}
			if(bmcv_width_align(handle, input[i], input_align[i])!=BM_SUCCESS){
				bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bmcv_width_align fail %s: %s: %d\n", __FILE__, __func__, __LINE__);
				goto fail;
			}
		} else
			input_align[i] = input[i];
	}

	ret = check_bm_vpss_param(
		frame_number, input_align, output, input_crop_rect, padding_attr, algorithm, &csc_type, matrix, border_param, &csc_cfg);
	if(ret != BM_SUCCESS) {
		bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_vpss error parameters found,%s: %s: %d\n",
			filename(__FILE__), __func__, __LINE__);
		goto fail;
	}

	ret = bm_vpss_asic(
		handle, frame_number, input_align, output, input_crop_rect, padding_attr, algorithm, &csc_cfg, convert_to_attr, border_param, coverex_param, gop_attr);
	if(ret != BM_SUCCESS)
		goto fail;

	for (i = 0; i < frame_number; i++)
		if (is_need_width_align_input(input[i]))
			bm_image_destroy(input_align + i);
	return ret;

fail:
	for (i = 0; i < frame_number; i++){
		if (is_need_width_align_input(input[i]))
			bm_image_destroy(input_align + i);
		if (output_is_alloc_mem[i])
			bm_image_detach(output[i]);
	}
	return ret;
}

bm_status_t check_bm_vpss_convert_to_param(bm_image* input, bm_image* output, int input_num){
	for(int i=0; i<input_num; i++){
		if((input[0].image_format != input[i].image_format) || (output[0].image_format != output[i].image_format)){
				bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
					"input and output different formats not supported, %s: %s: %d\n",
					filename(__FILE__), __func__, __LINE__);
				return BM_NOT_SUPPORTED;
		}
	}
	return BM_SUCCESS;
}

/*ax+b*/
bm_status_t bm_vpss_convert_to(
	bm_handle_t          handle,
	int                  input_num,
	bmcv_convert_to_attr convert_to_attr,
	bm_image *           input,
	bm_image *           output)
{
	bm_status_t ret = BM_SUCCESS;

	ret = simple_check_bm_vpss_input_param(handle, input, input_num);
	if(ret != BM_SUCCESS) {
		return ret;
	}

	ret = check_bm_vpss_continuity(input_num, input);
	if(ret != BM_SUCCESS) {
		return ret;
	}
	ret = check_bm_vpss_continuity(input_num, output);
	if(ret != BM_SUCCESS) {
		return ret;
	}

	ret = check_bm_vpss_convert_to_param(input, output, input_num);
	if(ret != BM_SUCCESS) {
		return ret;
	}

	bmcv_convert_to_attr *convert_to_attr_inner = (bmcv_convert_to_attr *)malloc(sizeof(bmcv_convert_to_attr) * input_num);

	if(input->image_format == FORMAT_BGR_PLANAR){
		float exc = convert_to_attr.alpha_0;
		convert_to_attr.alpha_0 = convert_to_attr.alpha_2;
		convert_to_attr.alpha_2 = exc;
		exc = convert_to_attr.beta_0;
		convert_to_attr.beta_0 = convert_to_attr.beta_2;
		convert_to_attr.beta_2 = exc;
	}

	for(int i=0; i<input_num; i++) {
		memcpy(&convert_to_attr_inner[i], &convert_to_attr, sizeof(bmcv_convert_to_attr));
	}

	ret = bm_vpss_multi_parameter_processing(
		handle, input_num, input, output, NULL, NULL, BMCV_INTER_LINEAR, CSC_MAX_ENUM, NULL, convert_to_attr_inner, NULL, NULL, NULL);

	free(convert_to_attr_inner);
	return ret;

}

bm_status_t bm_vpss_multi_input_multi_output(
	bm_handle_t             handle,
	int                     frame_number,
	bm_image*               input,
	bm_image*               output,
	bmcv_rect_t*            crop_rect,
	bmcv_padding_attr_t*    padding_attr,
	bmcv_resize_algorithm   algorithm,
	csc_type_t              csc_type,
	csc_matrix_t*           matrix)
{
	bm_status_t ret = BM_SUCCESS;

	ret = bm_vpss_multi_parameter_processing(
		handle, frame_number, input, output, crop_rect, padding_attr, algorithm, csc_type, matrix, NULL, NULL, NULL, NULL);

	return ret;

}

bm_status_t bm_vpss_single_input_multi_output(
	bm_handle_t             handle,
	int                     frame_number,
	bm_image                input,
	bm_image*               output,
	bmcv_rect_t*            crop_rect,
	bmcv_padding_attr_t*    padding_attr,
	bmcv_resize_algorithm   algorithm,
	csc_type_t              csc_type,
	csc_matrix_t*           matrix)
{
	int i;
	bm_status_t ret = BM_SUCCESS;

	ret = check_bm_vpss_continuity(frame_number, output);
	if(ret != BM_SUCCESS) {
		return ret;
	}


	bm_image *input_inner = (bm_image *)malloc(sizeof(bm_image) * frame_number);

	for(i = 0; i< frame_number; i++) {
		input_inner[i] = input;
	}

	ret = bm_vpss_multi_input_multi_output(
		handle, frame_number, input_inner, output, crop_rect, padding_attr, algorithm, csc_type, matrix);

	free(input_inner);
	return ret;
}

bm_status_t bm_vpss_multi_input_single_output(
	bm_handle_t             handle,
	int                     frame_number,
	bm_image*               input,
	bm_image                output,
	bmcv_rect_t*            crop_rect,
	bmcv_padding_attr_t*    padding_attr,
	bmcv_resize_algorithm   algorithm,
	csc_type_t              csc_type,
	csc_matrix_t*           matrix)
{
	int i;
	bm_status_t ret = BM_SUCCESS;
	ret = check_bm_vpss_continuity( frame_number, input);
	if(ret != BM_SUCCESS) {
		return ret;
	}

	bm_image *output_inner = (bm_image *)malloc(sizeof(bm_image) * frame_number);

	for(i = 0; i< frame_number; i++) {
		output_inner[i] = output;
	}

	ret = bm_vpss_multi_input_multi_output(
		handle, frame_number, input, output_inner, crop_rect, padding_attr, algorithm, csc_type, matrix);

	free(output_inner);

	return ret;
}

bm_status_t bm_vpss_csc_matrix_convert(
	bm_handle_t           handle,
	int                   output_num,
	bm_image              input,
	bm_image*             output,
	csc_type_t            csc,
	csc_matrix_t*         matrix,
	bmcv_resize_algorithm algorithm,
	bmcv_rect_t *         crop_rect)
{
	bm_status_t ret = BM_SUCCESS;

	ret = simple_check_bm_vpss_input_param(handle, output, output_num);
	if(ret != BM_SUCCESS)
		return ret;

	ret = bm_vpss_single_input_multi_output(
		handle, output_num, input, output, crop_rect, NULL, algorithm, csc, matrix);

	return ret;
}

bm_status_t bm_vpss_convert_internal(
	bm_handle_t             handle,
	int                     output_num,
	bm_image                input,
	bm_image*               output,
	bmcv_rect_t*            crop_rect_,
	bmcv_resize_algorithm   algorithm,
	csc_matrix_t *          matrix)
{
	bm_status_t ret = BM_SUCCESS;
	csc_type_t csc_type = CSC_MAX_ENUM;

	ret = bm_vpss_csc_matrix_convert(
		handle, output_num, input, output, csc_type, matrix, algorithm, crop_rect_);

	return ret;
}

bm_status_t bm_vpss_basic(
	bm_handle_t             handle,
	int                     in_img_num,
	bm_image*               input,
	bm_image*               output,
	int*                    crop_num_vec,
	bmcv_rect_t*            crop_rect,
	bmcv_padding_attr_t*    padding_attr,
	bmcv_resize_algorithm   algorithm,
	csc_type_t              csc_type,
	csc_matrix_t*           matrix) {

	int out_img_num = 0, i = 0, j = 0;
	bm_image *input_inner;
	int out_idx = 0;
	bm_status_t ret = BM_SUCCESS;

	ret = simple_check_bm_vpss_input_param(handle, input, in_img_num);
	if(ret != BM_SUCCESS) {
		return ret;
	}

	if (crop_rect == NULL) {
		if (crop_num_vec != NULL) {
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, \
				"crop_num_vec should be NULL err %s: %s: %d\n", __FILE__, __func__, __LINE__);
			return BM_ERR_FAILURE;
		}

		out_img_num = in_img_num;
		input_inner = input;

	} else {
		if (crop_num_vec == NULL) {
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, \
				"crop_num_vec should not be NULL err %s: %s: %d\n", __FILE__, __func__, __LINE__);
			return BM_ERR_FAILURE;
		}

		for (i = 0; i < in_img_num; i++) {
			out_img_num += crop_num_vec[i];
		}
		input_inner = (bm_image *)malloc(sizeof(bm_image) * out_img_num);

		for (i = 0; i < in_img_num; i++) {
			for (j = 0; j < crop_num_vec[i]; j++) {
				input_inner[out_idx + j]= input[i];
			}
			out_idx += crop_num_vec[i];
		}
	}

	ret = check_bm_vpss_continuity( in_img_num, input);
	if(ret != BM_SUCCESS) {
		return ret;
	}
	ret = check_bm_vpss_continuity( out_img_num, output);
	if(ret != BM_SUCCESS) {
		return ret;
	}

	ret = bm_vpss_multi_input_multi_output(
		handle, out_img_num, input_inner, output, crop_rect, padding_attr, algorithm, csc_type, matrix);

	if (crop_rect != NULL) {
		free(input_inner);
	}
	return ret;
}

bm_status_t bm_vpss_storage_convert(
	bm_handle_t      handle,
	int              image_num,
	bm_image*        input_,
	bm_image*        output_,
	csc_type_t       csc_type)
{
	bm_status_t ret = BM_SUCCESS;

	ret = bm_vpss_multi_input_multi_output(
		handle, image_num, input_, output_, NULL, NULL, BMCV_INTER_LINEAR, csc_type, NULL);

	return ret;
}

bm_status_t bm_vpss_cvt_padding(
	bm_handle_t             handle,
	int                     output_num,
	bm_image                input,
	bm_image*               output,
	bmcv_padding_attr_t*    padding_attr,
	bmcv_rect_t*            crop_rect,
	bmcv_resize_algorithm   algorithm,
	csc_matrix_t*           matrix)
{
	bm_status_t ret = BM_SUCCESS;
	csc_type_t csc_type = CSC_MAX_ENUM;

	ret = simple_check_bm_vpss_input_param(handle, output, output_num);
	if(ret != BM_SUCCESS)
	{
		return ret;
	}

	if (padding_attr == NULL) {
			bmlib_log("VPSS_PADDING", BMLIB_LOG_ERROR, "vpp padding info is nullptr");
			return BM_ERR_FAILURE;
	}

	ret = bm_vpss_single_input_multi_output(
		handle, output_num, input, output, crop_rect, padding_attr, algorithm, csc_type, matrix);

	return ret;
}

bm_status_t bm_vpss_draw_rectangle(
	bm_handle_t   handle,
	bm_image      image,
	int           rect_num,
	bmcv_rect_t * rects,
	int           line_width,
	unsigned char r,
	unsigned char g,
	unsigned char b)
{
	int i = 0;
	bm_status_t ret = BM_SUCCESS;
	int draw_num = 0;
	/*check border param*/

	draw_num = (rect_num >> 2) + 1;
	bmcv_border* border_cfg = (bmcv_border *)malloc(sizeof(bmcv_border) * draw_num);
	memset(border_cfg, 0, sizeof(bmcv_border) * draw_num);
	for(int j = 0; j < draw_num; j++){
		int draw_num_current = (j == (draw_num - 1)) ? (rect_num % 4) : 4;
		border_cfg[j].border_num = draw_num_current;
		for(int k = 0; k < draw_num_current; k++){
			int draw_idx = ((j << 2) + k);
			border_cfg[j].border_cfg[k].rect_border_enable = 1;
			border_cfg[j].border_cfg[k].st_x = rects[draw_idx].start_x;
			border_cfg[j].border_cfg[k].st_y = rects[draw_idx].start_y;
			border_cfg[j].border_cfg[k].width = rects[draw_idx].crop_w;
			border_cfg[j].border_cfg[k].height = rects[draw_idx].crop_h;
			border_cfg[j].border_cfg[k].value_r = r;
			border_cfg[j].border_cfg[k].value_g = g;
			border_cfg[j].border_cfg[k].value_b = b;
			border_cfg[j].border_cfg[k].thickness = line_width;
		}
	}

	bm_image *border_image = (bm_image *)malloc(sizeof(bm_image) * draw_num);

	for(i = 0; i< draw_num; i++) {
		border_image[i] = image;
	}

	ret = bm_vpss_multi_parameter_processing(
		handle, draw_num, border_image, border_image, NULL, NULL, BMCV_INTER_LINEAR, CSC_MAX_ENUM, NULL, NULL, border_cfg, NULL, NULL);

	free(border_image);
	free(border_cfg);
	return ret;
}

bm_status_t bm_vpss_csc_convert_to(
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
	bmcv_convert_to_attr*   convert_to_attr_){

	int out_crop_num = 0, i = 0, j = 0;
	bm_image *input_inner, *output_inner;
	bmcv_convert_to_attr *convert_to_attr;
	int out_idx = 0;
	bm_status_t ret = BM_SUCCESS;

	ret = simple_check_bm_vpss_input_param(handle, input, img_num);
	if(ret != BM_SUCCESS)
	{
		return ret;
	}

	ret = check_bm_vpss_continuity(img_num, input);
	if(ret != BM_SUCCESS)
	{
		return ret;
	}
	ret = check_bm_vpss_continuity(out_crop_num, output);
	if(ret != BM_SUCCESS)
	{
		return ret;
	}

	if (crop_rect == NULL) {
		if (crop_num_vec != NULL) {
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, \
				"crop_num_vec should be NULL err %s: %s: %d\n", __FILE__, __func__, __LINE__);
			return BM_ERR_FAILURE;
		}

		out_crop_num = img_num;
	} else {
		if (crop_num_vec == NULL) {
			bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, \
				"crop_num_vec should not be NULL err %s: %s: %d\n", __FILE__, __func__, __LINE__);
			return BM_ERR_FAILURE;
		}
		for (i = 0; i < img_num; i++) {
			out_crop_num += crop_num_vec[i];
		}
	}

	if (crop_rect != NULL) {
		input_inner = (bm_image *)malloc(sizeof(bm_image) * out_crop_num);
		output_inner = (bm_image *)malloc(sizeof(bm_image) * out_crop_num);

		for (i = 0; i < img_num; i++) {
			for (j = 0; j < crop_num_vec[i]; j++) {
					input_inner[out_idx + j]= input[i];
					output_inner[out_idx + j]= output[out_idx + j];
			}
			out_idx += crop_num_vec[i];
		}
	} else {
		input_inner = input;
		output_inner = output;
	}

	convert_to_attr = (bmcv_convert_to_attr *)malloc(sizeof(bmcv_convert_to_attr) * out_crop_num);
	for(int i=0; i<out_crop_num; i++)
		convert_to_attr[i] = convert_to_attr_[0];

	ret = bm_vpss_multi_parameter_processing(
		handle, out_crop_num, input_inner, output_inner, crop_rect, padding_attr, algorithm, csc_type, matrix, convert_to_attr, NULL, NULL, NULL);

	if (crop_rect != NULL)
	{
		free(input_inner);
		free(output_inner);
	}
	free(convert_to_attr);
	return ret;
}

bm_status_t bm_vpss_copy_to(
	bm_handle_t         handle,
	bmcv_copy_to_atrr_t copy_to_attr,
	bm_image            input,
	bm_image            output)
{
	bm_status_t ret = BM_SUCCESS;

	bmcv_padding_attr_t padding_attr;

	padding_attr.dst_crop_stx = copy_to_attr.start_x;
	padding_attr.dst_crop_sty = copy_to_attr.start_y;
	padding_attr.dst_crop_w   = input.width;
	padding_attr.dst_crop_h   = input.height;
	padding_attr.padding_r = copy_to_attr.padding_r;
	padding_attr.padding_g = copy_to_attr.padding_g;
	padding_attr.padding_b = copy_to_attr.padding_b;
	padding_attr.if_memset = copy_to_attr.if_padding;

	ret = bm_vpss_cvt_padding(handle, 1, input, &output, &padding_attr, NULL, BMCV_INTER_LINEAR, NULL);

	return ret;
}

bm_status_t bm_vpss_stitch(
	bm_handle_t             handle,
	int                     input_num,
	bm_image*               input,
	bm_image                output,
	bmcv_rect_t*            dst_crop_rect,
	bmcv_rect_t*            src_crop_rect,
	bmcv_resize_algorithm   algorithm)
{
	int i;
	bm_status_t ret = BM_SUCCESS;

	ret = simple_check_bm_vpss_input_param(handle, input, input_num);
	if(ret != BM_SUCCESS)
	{
		return ret;
	}

	if (dst_crop_rect == NULL) {
		bmlib_log("VPP-STITCH", BMLIB_LOG_ERROR, "dst_crop_rect is nullptr");
		return BM_ERR_FAILURE;
	}

	bmcv_padding_attr_t *padding_attr = (bmcv_padding_attr_t *)malloc(sizeof(bmcv_padding_attr_t) * input_num);

	memset(padding_attr, 0, input_num *sizeof(bmcv_padding_attr_t));
	for(i = 0; i < input_num; i++)
	{
		padding_attr[i].dst_crop_stx = dst_crop_rect[i].start_x;
		padding_attr[i].dst_crop_sty = dst_crop_rect[i].start_y;
		padding_attr[i].dst_crop_w   = dst_crop_rect[i].crop_w;
		padding_attr[i].dst_crop_h   = dst_crop_rect[i].crop_h;
	}
	ret = bm_vpss_multi_input_single_output(
		handle, input_num, input, output, src_crop_rect, padding_attr, algorithm, CSC_MAX_ENUM, NULL);

	free(padding_attr);
	return ret;
}

bm_status_t bm_vpss_mosaic_special(
	bm_handle_t          handle,
	int                   mosaic_num,
	bm_image              image,
	bmcv_rect_t *         mosaic_rect)
{
	bm_status_t ret = BM_SUCCESS;
	bm_image * input = (bm_image *)malloc(sizeof(bm_image) * mosaic_num);
	bm_image * output = (bm_image *)malloc(sizeof(bm_image) * mosaic_num);
	bm_image * masaic_pad = (bm_image *)malloc(sizeof(bm_image) * mosaic_num);
	bm_image * masaic_narrow = (bm_image *)malloc(sizeof(bm_image) * mosaic_num);
	bmcv_padding_attr_t * padding_pad = (bmcv_padding_attr_t *)malloc(sizeof(bmcv_padding_attr_t) * mosaic_num);
	bmcv_padding_attr_t * padding_enlarge = (bmcv_padding_attr_t *)malloc(sizeof(bmcv_padding_attr_t) * mosaic_num);
	bmcv_rect_t * crop_pad = (bmcv_rect_t *)malloc(sizeof(bmcv_rect_t) * mosaic_num);
	bmcv_rect_t * crop_enlarge = (bmcv_rect_t *)malloc(sizeof(bmcv_rect_t) * mosaic_num);
	for(int i=0; i<mosaic_num; i++){
		input[i] = image;
		crop_pad[i].start_x = mosaic_rect[i].start_x;
		crop_pad[i].start_y = mosaic_rect[i].start_y;
		crop_pad[i].crop_w = mosaic_rect[i].crop_w;
		crop_pad[i].crop_h = mosaic_rect[i].crop_h;
		padding_pad[i].dst_crop_stx = 0;
		padding_pad[i].dst_crop_sty = 0;
		padding_pad[i].dst_crop_w = crop_pad[i].crop_w;
		padding_pad[i].dst_crop_h = crop_pad[i].crop_h;
		padding_pad[i].if_memset = 1;
		padding_pad[i].padding_r = 0;
		padding_pad[i].padding_g = 0;
		padding_pad[i].padding_b = 0;
		int masaic_pad_h = mosaic_rect[i].crop_h > 128 ? mosaic_rect[i].crop_h : 128;
		int masaic_pad_w = mosaic_rect[i].crop_w > 128 ? mosaic_rect[i].crop_w : 128;
		bm_image_create(handle, masaic_pad_h, masaic_pad_w, image.image_format, image.data_type, &masaic_pad[i], NULL);
		ret = bm_image_alloc_dev_mem(masaic_pad[i], BMCV_HEAP_ANY);
		if(ret != BM_SUCCESS){
				bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
			"bm_image alloc dev mem fail %s: %s: %d\n", __FILE__, __func__, __LINE__);
			goto fail2;
		}
	}
	ret = bm_vpss_multi_parameter_processing(handle, mosaic_num, input, masaic_pad, crop_pad,
			padding_pad, BMCV_INTER_NEAREST, CSC_MAX_ENUM, NULL, NULL, NULL, NULL, NULL);
	if(ret != BM_SUCCESS)
		goto fail2;
	for(int i=0; i<mosaic_num; i++){
		output[i] = image;
		int masaic_narrow_h = masaic_pad[i].height >> 3;
		int masaic_narrow_w = masaic_pad[i].width >> 3;
		bm_image_create(handle, masaic_narrow_h, masaic_narrow_w, image.image_format, image.data_type, &masaic_narrow[i], NULL);
		ret = bm_image_alloc_dev_mem(masaic_narrow[i], BMCV_HEAP_ANY);
		if(ret != BM_SUCCESS){
				bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
			"bm_image alloc dev mem fail %s: %s: %d\n", __FILE__, __func__, __LINE__);
			goto fail1;
		}
		padding_enlarge[i].dst_crop_stx = mosaic_rect[i].start_x;
		padding_enlarge[i].dst_crop_sty = mosaic_rect[i].start_y;
		padding_enlarge[i].dst_crop_w = crop_pad[i].crop_w;
		padding_enlarge[i].dst_crop_h = crop_pad[i].crop_h;
		padding_enlarge[i].if_memset = 0;
		crop_enlarge[i].start_x = 0;
		crop_enlarge[i].start_y = 0;
		crop_enlarge[i].crop_w = crop_pad[i].crop_w;
		crop_enlarge[i].crop_h = crop_pad[i].crop_h;
	}
	ret = bm_vpss_multi_parameter_processing(handle, mosaic_num, masaic_pad, masaic_narrow, NULL,
			NULL, BMCV_INTER_NEAREST, CSC_MAX_ENUM, NULL, NULL, NULL, NULL, NULL);
	if(ret != BM_SUCCESS)
		goto fail1;
	ret = bm_vpss_multi_parameter_processing(handle, mosaic_num, masaic_narrow, masaic_pad, NULL,
			NULL, BMCV_INTER_NEAREST, CSC_MAX_ENUM, NULL, NULL, NULL, NULL, NULL);
	if(ret != BM_SUCCESS)
		goto fail1;
	ret = bm_vpss_multi_parameter_processing(handle, mosaic_num, masaic_pad, output, crop_enlarge,
			padding_enlarge, BMCV_INTER_NEAREST, CSC_MAX_ENUM, NULL, NULL, NULL, NULL, NULL);
fail1:
	for(int i=0; i<mosaic_num; i++){
		bm_image_destroy(masaic_narrow + i);
	}
fail2:
	for(int i=0; i<mosaic_num; i++){
		bm_image_destroy(masaic_pad + i);
	}
	free(masaic_narrow);
	free(masaic_pad);
	free(padding_pad);
	free(padding_enlarge);
	free(crop_pad);
	free(crop_enlarge);
	free(input);
	free(output);
	return ret;
}

bm_status_t bm_vpss_mosaic_normal(
	bm_handle_t           handle,
	int                   mosaic_num,
	bm_image              input,
	bmcv_rect_t *         mosaic_rect)
{
	bm_status_t ret = BM_SUCCESS;
	bm_image * image = (bm_image *)malloc(sizeof(bm_image) * mosaic_num);
	bm_image * masaic_narrow = (bm_image *)malloc(sizeof(bm_image) * mosaic_num);
	bmcv_rect_t * crop_narrow = (bmcv_rect_t *)malloc(sizeof(bmcv_rect_t) * mosaic_num);
	bmcv_padding_attr_t * padding_enlarge = (bmcv_padding_attr_t *)malloc(sizeof(bmcv_padding_attr_t) * mosaic_num);
	for(int i=0; i<mosaic_num; i++){
		image[i] = input;
		crop_narrow[i].start_x = mosaic_rect[i].start_x;
		crop_narrow[i].start_y = mosaic_rect[i].start_y;
		crop_narrow[i].crop_w = mosaic_rect[i].crop_w;
		crop_narrow[i].crop_h = mosaic_rect[i].crop_h;
		padding_enlarge[i].dst_crop_stx = mosaic_rect[i].start_x;
		padding_enlarge[i].dst_crop_sty = mosaic_rect[i].start_y;
		padding_enlarge[i].dst_crop_w = mosaic_rect[i].crop_w;
		padding_enlarge[i].dst_crop_h = mosaic_rect[i].crop_h;
		padding_enlarge[i].if_memset = 0;
		bm_image_create(handle, mosaic_rect[i].crop_h >> 3, mosaic_rect[i].crop_w >> 3, input.image_format, input.data_type, &masaic_narrow[i], NULL);
		ret = bm_image_alloc_dev_mem(masaic_narrow[i], BMCV_HEAP_ANY);
		if(ret != BM_SUCCESS){
				bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
			"bm_image alloc dev mem fail %s: %s: %d\n", __FILE__, __func__, __LINE__);
			goto fail;
		}
	}
	ret = bm_vpss_multi_parameter_processing(handle, mosaic_num, image, masaic_narrow, crop_narrow, NULL, BMCV_INTER_NEAREST, CSC_MAX_ENUM, NULL, NULL, NULL, NULL, NULL);
	if(ret != BM_SUCCESS)
		goto fail;
	ret = bm_vpss_multi_parameter_processing(handle, mosaic_num, masaic_narrow, image, NULL, padding_enlarge, BMCV_INTER_NEAREST, CSC_MAX_ENUM, NULL, NULL, NULL, NULL, NULL);
fail:
	for(int i=0; i<mosaic_num; i++)
		bm_image_destroy(masaic_narrow + i);
	free(image);
	free(masaic_narrow);
	free(crop_narrow);
	free(padding_enlarge);
	return ret;
}

bm_status_t bm_vpss_mosaic(
	bm_handle_t          handle,
	int                  mosaic_num,
	bm_image             image,
	bmcv_rect_t *        mosaic_rect)
{
	bm_status_t ret = BM_SUCCESS;
	int normal_num = 0, special_num = 0;
	bmcv_rect_t * normal_masaic = (bmcv_rect_t *)malloc(sizeof(bmcv_rect_t) * mosaic_num);
	bmcv_rect_t * special_masaic = (bmcv_rect_t *)malloc(sizeof(bmcv_rect_t) * mosaic_num);
	for(int i=0; i<mosaic_num; i++){
		if(mosaic_rect[i].crop_w < 128 || mosaic_rect[i].crop_h < 128){
				special_masaic[special_num] = mosaic_rect[i];
				special_num += 1;
		}
		else{
				normal_masaic[normal_num] = mosaic_rect[i];
				normal_num += 1;
		}
	}
	if(special_num > 0){
		ret = bm_vpss_mosaic_special(handle, special_num, image, special_masaic);
	}
	if(normal_num > 0){
		ret = bm_vpss_mosaic_normal(handle, normal_num, image, normal_masaic);
	}
	free(normal_masaic);
	free(special_masaic);
	return ret;
}

bm_status_t bm_vpss_fill_rectangle(
  bm_handle_t          handle,
  bm_image *           image,
  int                  rect_num,
  bmcv_rect_t *        rects,
  unsigned char        r,
  unsigned char        g,
  unsigned char        b)
{
	bm_status_t ret = BM_SUCCESS;
	ret = simple_check_bm_vpss_input_param(handle, image, 1);
	if(ret != BM_SUCCESS)
		return ret;
	coverex_cfg * coverex = (coverex_cfg *)malloc(sizeof(coverex_cfg) * rect_num);
	int execution_num = (rect_num >> 2) + 1, end_cover_num = (rect_num % 4);
	bm_image *input = (bm_image *)malloc(sizeof(bm_image) * execution_num);
	for(int i = 0; i < execution_num; i++){
		coverex[i].cover_num = (i == (execution_num - 1)) ? end_cover_num : 4;
		input[i] = image[0];
		for(int j = 0; j < coverex[i].cover_num; j++){
			int fill_idx = (i << 2) + j;
			coverex[i].coverex_param[j].enable = true;
			coverex[i].coverex_param[j].color = ((r << 16) | (g << 8) | (b));
			coverex[i].coverex_param[j].rect.left = rects[fill_idx].start_x;
			coverex[i].coverex_param[j].rect.top = rects[fill_idx].start_y;
			coverex[i].coverex_param[j].rect.width = rects[fill_idx].crop_w;
			coverex[i].coverex_param[j].rect.height = rects[fill_idx].crop_h;
		}
	}
	ret = bm_vpss_multi_parameter_processing(
		handle, execution_num, input, input, NULL, NULL, BMCV_INTER_LINEAR, CSC_MAX_ENUM, NULL, NULL, NULL, coverex, NULL);
	free(coverex);
	free(input);
	return ret;
}

bm_status_t bm_vpss_overlay(
  bm_handle_t          handle,
  bm_image             image,
  int                  rect_num,
  bmcv_rect_t *        rects,
  bm_image *           overlay_image)
{
	bm_status_t ret = BM_SUCCESS;
	unsigned char overlay_time = (rect_num + 15) >> 4;
	unsigned char overlay_num = 0;
	bmcv_rgn_cfg gop_attr;
	struct cvi_rgn_param *gop_cfg = (struct cvi_rgn_param *)malloc(sizeof(struct cvi_rgn_param) * rect_num);
	for(int i = 0; i < overlay_time; i++){
		overlay_num = (i == (overlay_time - 1)) ? (rect_num - (i << 4)) : 16;
		for(int j = 0; j < overlay_num; j++){
			int idx = ((i << 4) + j);
			switch(overlay_image[idx].image_format){
				case FORMAT_ARGB_PACKED:
				    gop_cfg[idx].fmt = CVI_RGN_FMT_ARGB8888;
					break;
				case FORMAT_ARGB4444_PACKED:
				    gop_cfg[idx].fmt = CVI_RGN_FMT_ARGB4444;
					break;
				case FORMAT_ARGB1555_PACKED:
				    gop_cfg[idx].fmt = CVI_RGN_FMT_ARGB1555;
					break;
				default:
				    printf("image format not supported \n");
					ret = BM_NOT_SUPPORTED;
					break;
			}
			if(ret != BM_SUCCESS) return ret;

			// gop_cfg[idx].fmt = CVI_RGN_FMT_ARGB8888;
			gop_cfg[idx].phy_addr = overlay_image[idx].image_private->data[0].u.device.device_addr;
			gop_cfg[idx].rect.left = rects[idx].start_x;
			gop_cfg[idx].rect.top = rects[idx].start_y;
			gop_cfg[idx].rect.width = overlay_image[idx].width;
			gop_cfg[idx].rect.height = overlay_image[idx].height;
			gop_cfg[idx].stride = overlay_image[idx].image_private->memory_layout[0].pitch_stride;
		}
		gop_attr.rgn_num = overlay_num;
		gop_attr.param = gop_cfg + (i << 4);
		ret = bm_vpss_multi_parameter_processing(
			handle, 1, &image, &image, NULL, NULL, BMCV_INTER_LINEAR, CSC_MAX_ENUM, NULL, NULL, NULL, NULL, &gop_attr);
		if(ret != BM_SUCCESS)
			break;
	}
	free(gop_cfg);
	return ret;
}