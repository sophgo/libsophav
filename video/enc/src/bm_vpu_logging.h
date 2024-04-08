#ifndef __BM_JPEG_LOGGING_H__
#define __BM_JPEG_LOGGING_H__

#include "bm_vpuenc_interface.h"

extern BmVpuEncLogLevel bmvpu_enc_cur_log_level_threshold;

void logging_fn(BmVpuEncLogLevel level, char const *file, int const line, char const *fn, const char *format, ...);

#define BMVPU_ENC_ERROR_FULL(FILE_, LINE_, FUNCTION_, ...)   do { if (bmvpu_enc_cur_log_level_threshold >= BMVPU_ENC_LOG_LEVEL_ERROR)   { logging_fn(BMVPU_ENC_LOG_LEVEL_ERROR,   FILE_, LINE_, FUNCTION_, __VA_ARGS__); } } while(0)
#define BMVPU_ENC_WARNING_FULL(FILE_, LINE_, FUNCTION_, ...) do { if (bmvpu_enc_cur_log_level_threshold >= BMVPU_ENC_LOG_LEVEL_WARNING) { logging_fn(BMVPU_ENC_LOG_LEVEL_WARNING, FILE_, LINE_, FUNCTION_, __VA_ARGS__); } } while(0)
#define BMVPU_ENC_INFO_FULL(FILE_, LINE_, FUNCTION_, ...)    do { if (bmvpu_enc_cur_log_level_threshold >= BMVPU_ENC_LOG_LEVEL_INFO)    { logging_fn(BMVPU_ENC_LOG_LEVEL_INFO,    FILE_, LINE_, FUNCTION_, __VA_ARGS__); } } while(0)
#define BMVPU_ENC_DEBUG_FULL(FILE_, LINE_, FUNCTION_, ...)   do { if (bmvpu_enc_cur_log_level_threshold >= BMVPU_ENC_LOG_LEVEL_DEBUG)   { logging_fn(BMVPU_ENC_LOG_LEVEL_DEBUG,   FILE_, LINE_, FUNCTION_, __VA_ARGS__); } } while(0)
#define BMVPU_ENC_LOG_FULL(FILE_, LINE_, FUNCTION_, ...)     do { if (bmvpu_enc_cur_log_level_threshold >= BMVPU_ENC_LOG_LEVEL_LOG)     { logging_fn(BMVPU_ENC_LOG_LEVEL_LOG,     FILE_, LINE_, FUNCTION_, __VA_ARGS__); } } while(0)
#define BMVPU_ENC_TRACE_FULL(FILE_, LINE_, FUNCTION_, ...)   do { if (bmvpu_enc_cur_log_level_threshold >= BMVPU_ENC_LOG_LEVEL_TRACE)   { logging_fn(BMVPU_ENC_LOG_LEVEL_TRACE,   FILE_, LINE_, FUNCTION_, __VA_ARGS__); } } while(0)


#define BMVPU_ENC_ERROR(...)    BMVPU_ENC_ERROR_FULL  (__FILE__, __LINE__, __func__, __VA_ARGS__)
#define BMVPU_ENC_WARNING(...)  BMVPU_ENC_WARNING_FULL(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define BMVPU_ENC_INFO(...)     BMVPU_ENC_INFO_FULL   (__FILE__, __LINE__, __func__, __VA_ARGS__)
#define BMVPU_ENC_DEBUG(...)    BMVPU_ENC_DEBUG_FULL  (__FILE__, __LINE__, __func__, __VA_ARGS__)
#define BMVPU_ENC_LOG(...)      BMVPU_ENC_LOG_FULL    (__FILE__, __LINE__, __func__, __VA_ARGS__)
#define BMVPU_ENC_TRACE(...)    BMVPU_ENC_TRACE_FULL  (__FILE__, __LINE__, __func__, __VA_ARGS__)

#endif /* __BM_JPEG_LOGGING_H__ */
