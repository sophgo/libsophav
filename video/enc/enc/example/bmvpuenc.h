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

#endif /* __TEST_ENC_H__ */

