#ifndef BMCV_A2_DPU_INTERNAL_H
#define BMCV_A2_DPU_INTERNAL_H

#ifdef _WIN32
#define DECL_EXPORT __declspec(dllexport)
#define DECL_IMPORT __declspec(dllimport)
#else
#define DECL_EXPORT
#define DECL_IMPORT
#endif

#include "bmcv_a2_common_internal.h"

#endif