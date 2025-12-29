#ifdef _WIN32
#define DECL_EXPORT __declspec(dllexport)
#define DECL_IMPORT __declspec(dllimport)
#else
#define DECL_EXPORT
#define DECL_IMPORT
#endif
#include "bmcv_api_ext_c.h"

bm_status_t bm_ive_add(
    bm_handle_t          handle,
    bm_image             input1,
    bm_image             input2,
    bm_image             output,
    bmcv_ive_add_attr    attr);

bm_status_t bm_ive_and(
    bm_handle_t         handle,
    bm_image            input1,
    bm_image            input2,
    bm_image            output);

bm_status_t bm_ive_or(
    bm_handle_t          handle,
    bm_image             input1,
    bm_image             input2,
    bm_image             output);

bm_status_t bm_ive_xor(
    bm_handle_t          handle,
    bm_image             input1,
    bm_image             input2,
    bm_image             output);

bm_status_t bm_ive_sub(
    bm_handle_t          handle,
    bm_image             input1,
    bm_image             input2,
    bm_image             output,
    bmcv_ive_sub_attr    attr);

bm_status_t bm_ive_thresh(
    bm_handle_t             handle,
    bm_image                input,
    bm_image                output,
    bmcv_ive_thresh_mode    threshMode,
    bmcv_ive_thresh_attr    attr);

bm_status_t bm_ive_dma_set(
    bm_handle_t                      handle,
    bm_image                         image,
    bmcv_ive_dma_set_mode            dma_set_mode,
    unsigned long long               val);

bm_status_t bm_ive_dma(
    bm_handle_t                     handle,
    bm_image                        input,
    bm_image                        output,
    bmcv_ive_dma_mode               dma_mode,
    bmcv_ive_interval_dma_attr *    attr);

bm_status_t bm_ive_map(
    bm_handle_t             handle,
    bm_image                input,
    bm_image                output,
    bm_device_mem_t         map_table);

bm_status_t bm_ive_hist(
    bm_handle_t      handle,
    bm_image         image,
    bm_device_mem_t  output);

bm_status_t bm_ive_integ(
    bm_handle_t              handle,
    bm_image                 input,
    bm_device_mem_t          output,
    bmcv_ive_integ_ctrl_s    integ_attr);

bm_status_t bm_ive_ncc(
    bm_handle_t          handle,
    bm_image             input1,
    bm_image             input2,
    bm_device_mem_t      output);

bm_status_t bm_ive_ord_stat_filter(
    bm_handle_t                   handle,
    bm_image *                    input,
    bm_image *                    output,
    bmcv_ive_ord_stat_filter_mode mode);

bm_status_t bm_ive_lbp(
    bm_handle_t              handle,
    bm_image *               input,
    bm_image *               output,
    bmcv_ive_lbp_ctrl_attr * lbp_attr);

bm_status_t bm_ive_dilate(
    bm_handle_t           handle,
    bm_image *            input,
    bm_image *            output,
    unsigned char *       dilate_mask);

bm_status_t bm_ive_erode(
    bm_handle_t           handle,
    bm_image *            input,
    bm_image *            output,
    unsigned char *       erode_mask);

bm_status_t bm_ive_mag_and_ang(
    bm_handle_t                   handle,
    bm_image *                    input,
    bm_image *                    mag_output,
    bm_image *                    ang_output,
    bmcv_ive_mag_and_ang_ctrl *   attr);

bm_status_t bm_ive_sobel(
    bm_handle_t           handle,
    bm_image *            input,
    bm_image *            outputH,
    bm_image *            outputV,
    bmcv_ive_sobel_ctrl * sobel_attr);

bm_status_t bm_ive_normgrad(
    bm_handle_t              handle,
    bm_image *               input,
    bm_image *               output_h,
    bm_image *               output_v,
    bm_image *               output_hv,
    bmcv_ive_normgrad_ctrl * normgrad_attr);

bm_status_t bm_ive_gmm(
    bm_handle_t            handle,
    bm_image *             input,
    bm_image *             output_fg,
    bm_image *             output_bg,
    bm_device_mem_t *      output_model,
    bmcv_ive_gmm_ctrl *    gmm_attr);

bm_status_t bm_ive_gmm2(
    bm_handle_t            handle,
    bm_image *             input,
    bm_image *             input_factor,
    bm_image *             output_fg,
    bm_image *             output_bg,
    bm_image *             output_match_model_info,
    bm_device_mem_t *      output_model,
    bmcv_ive_gmm2_ctrl *   gmm2_attr);

bm_status_t bm_ive_canny_hsy_edge(
    bm_handle_t                    handle,
    bm_image *                     input,
    bm_image *                     output_edge,
    bm_device_mem_t *              output_stack,
    bmcv_ive_canny_hys_edge_ctrl * canny_hys_edge_attr);

bm_status_t bm_ive_filter(
    bm_handle_t                  handle,
    bm_image                     input,
    bm_image                     output,
    bmcv_ive_filter_ctrl         filter_attr);

bm_status_t bm_ive_csc(
    bm_handle_t     handle,
    bm_image *      input,
    bm_image *      output,
    csc_type_t      csc_type);

bm_status_t bm_ive_resize(
    bm_handle_t               handle,
    bm_image *                input,
    bm_image *                output,
    bmcv_resize_algorithm     resize_mode);

bm_status_t bm_ive_stCandiCorner(
    bm_handle_t                   handle,
    bm_image *                    input,
    bm_image *                    output,
    bmcv_ive_stcandicorner_attr * stcandicorner_attr);

bm_status_t bm_ive_gradFg(
    bm_handle_t             handle,
    bm_image *              input_bgDiffFg,
    bm_image *              input_fgGrad,
    bm_image *              input_bgGrad,
    bm_image *              output_gradFg,
    bmcv_ive_gradfg_attr *  gradfg_attr);

bm_status_t bm_ive_sad(
    bm_handle_t                handle,
    bm_image *                 input,
    bm_image *                 output_sad,
    bm_image *                 output_thr,
    bmcv_ive_sad_attr *        sad_attr,
    bmcv_ive_sad_thresh_attr*  thresh_attr);

bm_status_t bm_ive_match_bgmodel(
    bm_handle_t                   handle,
    bm_image *                    cur_img,
    bm_image *                    bgmodel_img,
    bm_image *                    fgflag_img,
    bm_image *                    diff_fg_img,
    bm_device_mem_t *             stat_data_mem,
    bmcv_ive_match_bgmodel_attr * attr);

bm_status_t bm_ive_update_bgmodel(
    bm_handle_t                    handle,
    bm_image *                     cur_img,
    bm_image *                     bgmodel_img,
    bm_image *                     fgflag_img,
    bm_image *                     bg_img,
    bm_image *                     chgsta_img,
    bm_device_mem_t  *             stat_data_mem,
    bmcv_ive_update_bgmodel_attr * attr);

bm_status_t bm_ive_ccl(
    bm_handle_t          handle,
    bm_image *           src_dst_image,
    bm_device_mem_t *    ccblob_output,
    bmcv_ive_ccl_attr *  ccl_attr);

bm_status_t bm_ive_bernsen(
    bm_handle_t           handle,
    bm_image              input,
    bm_image              output,
    bmcv_ive_bernsen_attr attr);

bm_status_t bm_ive_filterAndCsc(
    bm_handle_t             handle,
    bm_image                input,
    bm_image                output,
    bmcv_ive_filter_ctrl    attr,
    csc_type_t              csc_type);

bm_status_t bm_ive_16bit_to_8bit(
    bm_handle_t                 handle,
    bm_image                    input,
    bm_image                    output,
    bmcv_ive_16bit_to_8bit_attr attr);

bm_status_t bm_ive_frame_diff_motion(
    bm_handle_t                     handle,
    bm_image                        input1,
    bm_image                        input2,
    bm_image                        output,
    bmcv_ive_frame_diff_motion_attr attr);