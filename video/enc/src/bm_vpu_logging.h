#ifndef __BM_JPEG_LOGGING_H__
#define __BM_JPEG_LOGGING_H__

#include "bm_vpuenc_interface.h"

extern BmVpuEncLogLevel bmvpu_enc_cur_log_level_threshold;

extern BmVpuEncLoggingFunc bmvpu_cur_logging_fn;

#define BMVPUAPI_UNUSED_PARAM(x) ((void)(x))

#define BMVPU_ENC_ERROR_FULL(FILE_, LINE_, FUNCTION_, ...)   do { if (bmvpu_enc_cur_log_level_threshold >= BMVPU_ENC_LOG_LEVEL_ERROR)   { bmvpu_cur_logging_fn(BMVPU_ENC_LOG_LEVEL_ERROR,   FILE_, LINE_, FUNCTION_, __VA_ARGS__); } } while(0)
#define BMVPU_ENC_WARNING_FULL(FILE_, LINE_, FUNCTION_, ...) do { if (bmvpu_enc_cur_log_level_threshold >= BMVPU_ENC_LOG_LEVEL_WARNING) { bmvpu_cur_logging_fn(BMVPU_ENC_LOG_LEVEL_WARNING, FILE_, LINE_, FUNCTION_, __VA_ARGS__); } } while(0)
#define BMVPU_ENC_INFO_FULL(FILE_, LINE_, FUNCTION_, ...)    do { if (bmvpu_enc_cur_log_level_threshold >= BMVPU_ENC_LOG_LEVEL_INFO)    { bmvpu_cur_logging_fn(BMVPU_ENC_LOG_LEVEL_INFO,    FILE_, LINE_, FUNCTION_, __VA_ARGS__); } } while(0)
#define BMVPU_ENC_DEBUG_FULL(FILE_, LINE_, FUNCTION_, ...)   do { if (bmvpu_enc_cur_log_level_threshold >= BMVPU_ENC_LOG_LEVEL_DEBUG)   { bmvpu_cur_logging_fn(BMVPU_ENC_LOG_LEVEL_DEBUG,   FILE_, LINE_, FUNCTION_, __VA_ARGS__); } } while(0)
#define BMVPU_ENC_LOG_FULL(FILE_, LINE_, FUNCTION_, ...)     do { if (bmvpu_enc_cur_log_level_threshold >= BMVPU_ENC_LOG_LEVEL_LOG)     { bmvpu_cur_logging_fn(BMVPU_ENC_LOG_LEVEL_LOG,     FILE_, LINE_, FUNCTION_, __VA_ARGS__); } } while(0)
#define BMVPU_ENC_TRACE_FULL(FILE_, LINE_, FUNCTION_, ...)   do { if (bmvpu_enc_cur_log_level_threshold >= BMVPU_ENC_LOG_LEVEL_TRACE)   { bmvpu_cur_logging_fn(BMVPU_ENC_LOG_LEVEL_TRACE,   FILE_, LINE_, FUNCTION_, __VA_ARGS__); } } while(0)


#define BMVPU_ENC_ERROR(...)    BMVPU_ENC_ERROR_FULL  (__FILE__, __LINE__, __func__, __VA_ARGS__)
#define BMVPU_ENC_WARNING(...)  BMVPU_ENC_WARNING_FULL(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define BMVPU_ENC_INFO(...)     BMVPU_ENC_INFO_FULL   (__FILE__, __LINE__, __func__, __VA_ARGS__)
#define BMVPU_ENC_DEBUG(...)    BMVPU_ENC_DEBUG_FULL  (__FILE__, __LINE__, __func__, __VA_ARGS__)
#define BMVPU_ENC_LOG(...)      BMVPU_ENC_LOG_FULL    (__FILE__, __LINE__, __func__, __VA_ARGS__)
#define BMVPU_ENC_TRACE(...)    BMVPU_ENC_TRACE_FULL  (__FILE__, __LINE__, __func__, __VA_ARGS__)

#endif /* __BM_JPEG_LOGGING_H__ */
