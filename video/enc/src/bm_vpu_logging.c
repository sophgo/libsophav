#include <stdio.h>
#include <stdarg.h>
#include "bm_vpu_logging.h"

BmVpuEncLogLevel bmvpu_enc_cur_log_level_threshold = BMVPU_ENC_LOG_LEVEL_ERROR;

void bmvpu_enc_set_logging_threshold(BmVpuEncLogLevel threshold)
{
    bmvpu_enc_cur_log_level_threshold = threshold;
}

void logging_fn(BmVpuEncLogLevel level, char const *file, int const line, char const *fn, const char *format, ...)
{
    va_list args;

    char const *lvlstr = "";
    switch (level)
    {
        case BMVPU_ENC_LOG_LEVEL_ERROR:   lvlstr = "ERROR";   break;
        case BMVPU_ENC_LOG_LEVEL_WARNING: lvlstr = "WARNING"; break;
        case BMVPU_ENC_LOG_LEVEL_INFO:    lvlstr = "INFO";    break;
        case BMVPU_ENC_LOG_LEVEL_DEBUG:   lvlstr = "DEBUG";   break;
        case BMVPU_ENC_LOG_LEVEL_TRACE:   lvlstr = "TRACE";   break;
        case BMVPU_ENC_LOG_LEVEL_LOG:     lvlstr = "LOG";     break;
        default: break;
    }

    fprintf(stderr, "%s:%d (%s)   %s: ", file, line, fn, lvlstr);

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
}

