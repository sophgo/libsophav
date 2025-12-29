#include <stdio.h>
#include <stdarg.h>
#include "bm_jpeg_internal.h"

BmJpuLogLevel bm_jpu_cur_log_level_threshold = BM_JPU_LOG_LEVEL_ERROR;

void bm_jpu_set_logging_threshold(BmJpuLogLevel threshold)
{
    bm_jpu_cur_log_level_threshold = threshold;
}

void bm_jpu_logging_fn(BmJpuLogLevel level, char const *file, int const line, char const *fn, const char *format, ...)
{
    va_list args;

    char const *lvlstr = "";
    switch (level)
    {
        case BM_JPU_LOG_LEVEL_ERROR:   lvlstr = "ERROR";   break;
        case BM_JPU_LOG_LEVEL_WARNING: lvlstr = "WARNING"; break;
        case BM_JPU_LOG_LEVEL_INFO:    lvlstr = "INFO";    break;
        case BM_JPU_LOG_LEVEL_DEBUG:   lvlstr = "DEBUG";   break;
        case BM_JPU_LOG_LEVEL_TRACE:   lvlstr = "TRACE";   break;
        case BM_JPU_LOG_LEVEL_LOG:     lvlstr = "LOG";     break;
        default: break;
    }

    fprintf(stderr, "%s:%d (%s)   %s: ", file, line, fn, lvlstr);

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
}


char const *bm_jpu_color_format_string(BmJpuColorFormat color_format)
{
    switch (color_format)
    {
        case BM_JPU_COLOR_FORMAT_YUV420: return "YUV 4:2:0";
        case BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL: return "YUV 4:2:2 horizontal";
        case BM_JPU_COLOR_FORMAT_YUV422_VERTICAL: return "YUV 2:2:4 vertical";
        case BM_JPU_COLOR_FORMAT_YUV444: return "YUV 4:4:4";
        case BM_JPU_COLOR_FORMAT_YUV400: return "YUV 4:0:0 (8-bit grayscale)";
        default:
            return "<unknown>";
    }
}

char const *bm_jpu_image_format_string(BmJpuImageFormat image_format)
{
    switch (image_format) {
        case BM_JPU_IMAGE_FORMAT_YUV420P:
            return "YUV 4:2:0";
        case BM_JPU_IMAGE_FORMAT_YUV422P:
            return "YUV 4:2:2";
        case BM_JPU_IMAGE_FORMAT_YUV444P:
            return "YUV 4:4:4";
        case BM_JPU_IMAGE_FORMAT_NV12:
            return "NV12";
        case BM_JPU_IMAGE_FORMAT_NV21:
            return "NV21";
        case BM_JPU_IMAGE_FORMAT_NV16:
            return "NV16";
        case BM_JPU_IMAGE_FORMAT_NV61:
            return "NV61";
        case BM_JPU_IMAGE_FORMAT_GRAY:
            return "YUV 4:0:0";
        case BM_JPU_IMAGE_FORMAT_RGB:
            return "RGB";
        default:
            return "<unknown>";
    }
}

char const *bm_jpu_dec_error_string(BmJpuDecReturnCodes code)
{
    switch (code) {
        case BM_JPU_DEC_RETURN_CODE_OK:
            return "ok";
        case BM_JPU_DEC_RETURN_CODE_ERROR:
            return "unspecified error";
        case BM_JPU_DEC_RETURN_CODE_INVALID_PARAMS:
            return "invalid params";
        case BM_JPU_DEC_RETURN_CODE_INVALID_HANDLE:
            return "invalid handle";
        case BM_JPU_DEC_RETURN_CODE_INVALID_FRAMEBUFFER:
            return "invalid framebuffer";
        case BM_JPU_DEC_RETURN_CODE_INSUFFICIENT_FRAMEBUFFERS:
            return "insufficient framebuffers";
        case BM_JPU_DEC_RETURN_CODE_WRONG_CALL_SEQUENCE:
            return "wrong call sequence";
        case BM_JPU_DEC_RETURN_CODE_TIMEOUT:
            return "timeout";
        case BM_JPU_DEC_RETURN_CODE_ALREADY_CALLED:
            return "already called";
        case BM_JPU_DEC_RETURN_ALLOC_MEM_ERROR:
            return "error allocating memory";
        default:
            return "<unknown>";
    }
}

char const *bm_jpu_enc_error_string(BmJpuEncReturnCodes code)
{
    switch (code) {
        case BM_JPU_ENC_RETURN_CODE_OK:
            return "ok";
        case BM_JPU_ENC_RETURN_CODE_ERROR:
            return "unspecified error";
        case BM_JPU_ENC_RETURN_CODE_INVALID_PARAMS:
            return "invalid params";
        case BM_JPU_ENC_RETURN_CODE_INVALID_HANDLE:
            return "invalid handle";
        case BM_JPU_ENC_RETURN_CODE_INVALID_FRAMEBUFFER:
            return "invalid framebuffer";
        case BM_JPU_ENC_RETURN_CODE_INSUFFICIENT_FRAMEBUFFERS:
            return "insufficient framebuffers";
        case BM_JPU_ENC_RETURN_CODE_INVALID_STRIDE:
            return "invalid stride";
        case BM_JPU_ENC_RETURN_CODE_WRONG_CALL_SEQUENCE:
            return "wrong call sequence";
        case BM_JPU_ENC_RETURN_CODE_TIMEOUT:
            return "timeout";
        case BM_JPU_ENC_RETURN_CODE_WRITE_CALLBACK_FAILED:
            return "write callback failed";
        case BM_JPU_ENC_RETURN_ALLOC_MEM_ERROR:
            return "error allocating memory";
        default:
            return "<unknown>";
    }
}
