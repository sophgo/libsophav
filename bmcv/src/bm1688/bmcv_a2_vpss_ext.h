#ifdef _WIN32
#define DECL_EXPORT __declspec(dllexport)
#define DECL_IMPORT __declspec(dllimport)
#else
#define DECL_EXPORT
#define DECL_IMPORT
#endif
#include "bmcv_api_ext_c.h"

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
  csc_matrix_t*           matrix);

bm_status_t bm_vpss_convert_internal(
  bm_handle_t             handle,
  int                     output_num,
  bm_image                input,
  bm_image*               output,
  bmcv_rect_t*            crop_rect_,
  bmcv_resize_algorithm   algorithm,
  csc_matrix_t *          matrix);

bm_status_t bm_vpss_csc_matrix_convert(
  bm_handle_t           handle,
  int                   output_num,
  bm_image              input,
  bm_image*             output,
  csc_type_t            csc,
  csc_matrix_t*         matrix,
  bmcv_resize_algorithm algorithm,
  bmcv_rect_t *         crop_rect);

bm_status_t bm_vpss_convert_to(
  bm_handle_t          handle,
  int                  input_num,
  bmcv_convert_to_attr convert_to_attr,
  bm_image *           input,
  bm_image *           output);

bm_status_t bm_vpss_storage_convert(
  bm_handle_t      handle,
  int              image_num,
  bm_image*        input_,
  bm_image*        output_,
  csc_type_t       csc_type);

bm_status_t bm_vpss_cvt_padding(
  bm_handle_t             handle,
  int                     output_num,
  bm_image                input,
  bm_image*               output,
  bmcv_padding_attr_t*    padding_attr,
  bmcv_rect_t*            crop_rect,
  bmcv_resize_algorithm   algorithm,
  csc_matrix_t*           matrix);

bm_status_t bm_vpss_draw_rectangle(
  bm_handle_t   handle,
  bm_image      image,
  int           rect_num,
  bmcv_rect_t * rects,
  int           line_width,
  unsigned char r,
  unsigned char g,
  unsigned char b);

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
  bmcv_convert_to_attr*   convert_to_attr_);

bm_status_t bm_vpss_copy_to(
  bm_handle_t         handle,
  bmcv_copy_to_atrr_t copy_to_attr,
  bm_image            input,
  bm_image            output);

bm_status_t bm_vpss_stitch(
  bm_handle_t             handle,
  int                     input_num,
  bm_image*               input,
  bm_image                output,
  bmcv_rect_t*            dst_crop_rect,
  bmcv_rect_t*            src_crop_rect,
  bmcv_resize_algorithm   algorithm);

bm_status_t bm_vpss_mosaic(
  bm_handle_t           handle,
  int                   mosaic_num,
  bm_image              image,
  bmcv_rect_t *         mosaic_rect);

bm_status_t bm_vpss_fill_rectangle(
  bm_handle_t          handle,
  bm_image *           image,
  int                  rect_num,
  bmcv_rect_t *        rects,
  unsigned char        r,
  unsigned char        g,
  unsigned char        b);

bm_status_t bm_vpss_overlay(
  bm_handle_t          handle,
  bm_image             image,
  int                  rect_num,
  bmcv_rect_t *        rects,
  bm_image *           overlay_image);

bm_status_t bm_vpss_flip(
  bm_handle_t          handle,
  bm_image             input,
  bm_image             output,
  bmcv_flip_mode       flip_mode);

bm_status_t bm_vpss_circle(
  bm_handle_t          handle,
  bm_image             image,
  bmcv_cir_mode        cir_mode,
  bmcv_point_t         center,
  int                  radius,
  unsigned char        line_width,
  unsigned char        r,
  unsigned char        g,
  unsigned char        b);

bm_status_t bm_vpss_all_func(
  bm_handle_t             handle,
  int                     crop_num,
  bm_image                input,
  bm_image*               output,
  bmcv_rect_t*            crop_rect,
  bmcv_padding_attr_t*    padding_attr,
  bmcv_resize_algorithm   algorithm,
  csc_type_t              csc_type,
  bmcv_flip_mode          flip_mode,
  bmcv_convert_to_attr*   convert_to_attr,
  bmcv_overlay_attr*      overlay_attr,
  bmcv_draw_rect_attr*    draw_rect_attr,
  bmcv_fill_rect_attr*    fill_rect_attr,
  bmcv_circle_attr*       circle_attr);