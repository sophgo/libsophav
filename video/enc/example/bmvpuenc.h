/* main code used by all examples
 * Copyright (C) 2018 Solan Shang
 * Copyright (C) 2014 Carlos Rafael Giani
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#ifndef __TEST_ENC_H__
#define __TEST_ENC_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#ifdef __linux__
#include <pthread.h>
#include <unistd.h>
#elif _WIN32
#include <windows.h>
#endif

// #include "bm_vpuenc_interface.h"
#include "bm_vpuenc_interface.h"

enum
{
    RETVAL_OK = 0,
    RETVAL_ERROR = 1,
    RETVAL_EOS = 2
};

static void logging_fn(BmVpuEncLogLevel level,
                       char const *file,
                       int const line,
                       char const *fn,
                       const char *format, ...)
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
#ifdef __linux__
    fprintf(stderr, "[%zx] %s:%d (%s)   %s: ", pthread_self(), file, line, fn, lvlstr);
#endif
#ifdef _WIN32
    fprintf(stderr, "[%zx] %s:%d (%s)   %s: ", GetCurrentThreadId(), file, line, fn, lvlstr);
#endif
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
}


#endif /* __TEST_ENC_H__ */

