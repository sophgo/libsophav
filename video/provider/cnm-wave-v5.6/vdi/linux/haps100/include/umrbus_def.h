/******************************************************************************
   Copyright (C) 2000 - 2014 Synopsys, Inc.
   This software and the associated documentation are confidential and
   proprietary to Synopsys, Inc. Your use or disclosure of this software
   is subject to the terms and conditions of a written license agreement
   between you, or your company, and Synopsys, Inc.
*******************************************************************************
   Title      : UMRBus Defines
*******************************************************************************
   Description: This file contains the UMRBus defines.
*******************************************************************************
   File       : umrbus.h 
*******************************************************************************
           $Id: //chipit/chipit/main/dev/umrbus/sw/driver/umrusb_lib/include/umrbus.h#4 $
******************************************************************************/ 

#ifndef __UMRBUS_DEF_H__
/* Tell doxygen that we want to document this file. */
/*! \file */

/* OS specific function decoration */
#if defined(_WIN32)
  #define UMRBUS_DECL __declspec(dllexport)
#else
  #define UMRBUS_DECL
#endif
    
/* OS specific includes */
#if defined(_WIN32)
  #include "windows.h"
  #include "BaseTsd.h"
#else
  #include <inttypes.h>
#endif

/* OS specific 32bit UMRBus types */ 
#if defined(_WIN32)
  #define UMRBUS_LONG           ULONG32
  #define UMRBUS_LONG_FORMAT    "%li"
  #define UMRBUS_ULONG_FORMAT   "%lu"
  #define UMRBUS_WCHAR          WCHAR
#else
  #define UMRBUS_LONG_FORMAT    "%i"
  #define UMRBUS_ULONG_FORMAT   "%u"
  #define UMRBUS_LONG           uint32_t
  #define UMRBUS_WCHAR          wchar_t
#endif
    
/* OS specific handles for the client applications */
#if defined(_WIN32)
  #define UMR_HANDLE             HANDLE
  #define UMR_INVALID_HANDLE     INVALID_HANDLE_VALUE
#else
  #define UMR_HANDLE             int
  #define UMR_INVALID_HANDLE     -1
#endif
    
/*!
 * \struct umrbus_scan_struct
 * \brief Structure containing scan information for a UMRBus client.
 */
struct umrbus_scan_struct
{
  unsigned int device;  /**< UMRBus device number */ 
  unsigned int bus;     /**< UMRBus number */
  unsigned int address; /**< UMRBus client address */
  unsigned int type;    /**< UMRBus client type */
  char * devicename;    /**< UMRBus device name string */
  int connected;        /**< Client connected flag */
};

/*! UMRBus client scan type */
typedef struct umrbus_scan_struct umrbus_scan_type;

/*!
 * \struct umrbus_interrupt_struct
 * \brief Structure containing interrupt information for a UMRBus client.
 */
struct umrbus_interrupt_struct
{
  int mode;        /**< UMRBus interrupt mode */
  int interrupt;   /**< UMRBus client interrupt identifier */
  int timeout;     /**< UMRBus interrupt for wait mode */
  int capiinttype; /**< UMRBus client interrupt type */
};

/*! UMRBus client interrupt type */
typedef struct umrbus_interrupt_struct umrbus_interrupt_type;

#define UMRBUS_DEFAULT_PORT 4000 /**< UMRBus default port for network connections*/

#endif //__UMRBUS_DEF_H__
