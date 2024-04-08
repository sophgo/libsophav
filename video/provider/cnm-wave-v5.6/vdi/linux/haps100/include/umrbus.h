/******************************************************************************
   Copyright (C) 2000 - 2014 Synopsys, Inc.
   This software and the associated documentation are confidential and
   proprietary to Synopsys, Inc. Your use or disclosure of this software
   is subject to the terms and conditions of a written license agreement
   between you, or your company, and Synopsys, Inc.
*******************************************************************************
   Title      : UMRBus C++ API 
*******************************************************************************
   Description: This file defines the UMRBus C++ API functions.
*******************************************************************************
   File       : umrbus.h 
*******************************************************************************
           $Id: //chipit/chipit/main/release/umrbus/sw/driver/umrusb_lib/include/umrbus.h#3 $
******************************************************************************/ 

/* Tell doxygen that we want to document this file. */
/*! \file */

#ifndef __umrbus_h__
#define __umrbus_h__

#include "umrbus_def.h"

#if defined USE_UMRBUS_NET_ACCESS && defined __cplusplus

namespace umr
{

  /*!
   * \brief Open a connection to a UMRBus client.
   *   
   * \param[in] device
   * Specifies the UMRBus device number. 
   * \li 0-3 : PCI(e) master devices
   * \li 4-7 : PCI(e) slave devices
   * \li > 8 : USB devices
   * 
   * \param[in] bus
   * Specifies the number of the UMRBus (0..7). 
   * 
   * \param[in] address
   * Specifies the address of the CAPIM to open (1..63).
   * 
   * \param[out] errormessage
   * Address of a caller-provided variable which will be set to a error message pointer if the function fails.
   * In case if the function succeeds a NULL pointer is returned. 
   * 
   * \return
   * The function returns a \c UMR_HANDLE if successful, an error code otherwise.
   * Some specific error codes are described below.
   * 
   * \retval UMR_INVALID_HANDLE
   * The device instance identified by \p device, \p bus and \p address is not present in the system.
   * The errormessage variable points to an error description. The error description must be freed after error 
   * with umrbus_errormessage_free().
   * 
   * The function opens a connection to a specific UMRBus client.
   * It is allowed to open several handles for the same UMRBus device.
   * The handle, that is returned by the function, identifies the UMRBus device and must be specified
   * when calling other functions of this interface.
   */
  UMRBUS_DECL 
  UMR_HANDLE  
  umrbus_open(unsigned int device,
              unsigned int bus,
              unsigned int address,
              char **errormessage);

  /*!
   * \brief Open a connection to a UMRBus client.
   *   
   * \param[in] pcHostname
   * Specifies the hostname to connect to (host:port)
   *
   * \param[in] device
   * Specifies the UMRBus device number. 
   * \li 0-3 : PCI(e) master devices
   * \li 4-7 : PCI(e) slave devices
   * \li > 8 : USB devices
   * 
   * \param[in] bus
   * Specifies the number of the UMRBus (0..7). 
   * 
   * \param[in] address
   * Specifies the address of the CAPIM to open (1..63).
   * 
   * \param[out] errormessage
   * Address of a caller-provided variable which will be set to a error message pointer if the function fails.
   * In case if the function succeeds a NULL pointer is returned. 
   * 
   * \return
   * The function returns a \c UMR_HANDLE if successful, an error code otherwise.
   * Some specific error codes are described below.
   * 
   * \retval UMR_INVALID_HANDLE
   * The device instance identified by \p device, \p bus and \p address is not present in the system.
   * The errormessage variable points to an error description. The error description must be freed after error 
   * with umrbus_errormessage_free().
   * 
   * The function opens a connection to a specific UMRBus client.
   * It is allowed to open several handles for the same UMRBus device.
   * The handle, that is returned by the function, identifies the UMRBus device and must be specified
   * when calling other functions of this interface.
   */
  UMRBUS_DECL 
  UMR_HANDLE  
  umrbus_open(const char* pcHostname, 
              unsigned int device,
              unsigned int bus,
              unsigned int address,
              char **errormessage);

  /*!
   * \brief Close connection to UMRBus client.
   * 
   * \param[in] handle
   * Identifies the client instance.
   * The specified handle becomes invalid after this call and must not be used in any subsequent API calls.
   * 
   * \param[out] errormessage
   * Address of a caller-provided variable which will be set to a error message pointer if the function fails.
   * In case if the function succeeds a NULL pointer is returned. 
   * 
   * \return
   * The function returns \c 0 if successful, \c 1 otherwise.
   * In case of an error the errormessage variable points to an error description. The error description must be freed 
   * after error with umrbus_errormessage_free().
   * 
   * The specified client connection will be removed and the provided handle becomes invalid with this call.
   */
  UMRBUS_DECL
  int 
  umrbus_close(
               UMR_HANDLE handle,
               char **errormessage);

  /*!
   * \brief Write data to a UMRBus client.
   * 
   * \param[in] handle
   * Identifies the client instance.
   * 
   * \param[in] data
   * Caller-provided buffer that contains the data to be written.
   * 
   * \param[in] size
   * Specifies the number of 32-bit words that should be written.
   * The number must not exceed the size of the \p data buffer.
   * 
   * \param[out] errormessage
   * Address of a caller-provided variable which will be set to a error message pointer if the function fails.
   * In case if the function succeeds a NULL pointer is returned. 
   * 
   * \return
   * The function returns \c 0 if successful, \c 1 otherwise.
   * In case of an error the errormessage variable points to an error description. The error description must be freed 
   * after error with umrbus_errormessage_free().
   * 
   * The function writes a specified number of 32-bit words to a UMRBus client.
   */
  UMRBUS_DECL
  int 
  umrbus_write(
               UMR_HANDLE handle,
               UMRBUS_LONG *data,
               UMRBUS_LONG size,
               char **errormessage);


  /*!
   * \brief Read data from a UMRBus client.
   * 
   * \param[in] handle
   * Identifies the client instance.
   * 
   * \param[out] data
   * Caller-provided buffer that will receive the read data.
   * 
   * \param[in] size
   * Specifies the number of 32-bit words that should be read.
   * The number must not exceed the size of the \p readBuffer buffer.
   *
   * \param[out] errormessage
   * Address of a caller-provided variable which will be set to a error message pointer if the function fails.
   * In case if the function succeeds a NULL pointer is returned. 
   * 
   * \return
   * The function returns \c 0 if successful, \c 1 otherwise.
   * In case of an error the errormessage variable points to an error description. 
   * The error description must be freed after error with umrbus_errormessage_free().
   * 
   * The function reads a specified number of 32-bit words from a UMRBus client.
   */
  UMRBUS_DECL
  int 
  umrbus_read(
              UMR_HANDLE handle,
              UMRBUS_LONG *data,
              UMRBUS_LONG size,
              char **errormessage);
  

  /*!
   * \brief Check UMRBus client for interrupt.
   * 
   * \param[in] handle
   * Identifies the client instance.
   * 
   * \param[in,out] interrupt
   * Pointer to a structure that specifies interrupt parameters.
   * Before the function is called the elements \p mode and \p timeout of the structure must 
   * be initialized as follows:
   * \li mode = 0 : The function checks if an interrupt was requested and returns to the 
   * calling application immediately (polling).
   * \li mode = 1 : The function waits until an interrupt is requested or a timeout has occurred. 
   * The timeout value must be initialized in the element timeout as a number of seconds.
   * If an interrupt occurred the element interrupt is nonzero and the element capiinttype contains 
   * the interrupt type.
   * 
   * \param[out] errormessage
   * Address of a caller-provided variable which will be set to a error message pointer if the function fails.
   * In case if the function succeeds a NULL pointer is returned. 
   * 
   * \return
   * The function returns \c 0 on success, or \c 1 otherwise.
   * If an error occurred errormessage points to an error description.
   * After an error occurred the error description must be freed with umrbus_errormessage_free().
   *
   * The function checks if a client application has requested an interrupt.
   * If a non-zero timeout value is specified  and wait mode is choosen the function waits for
   * an interrupt request for the given interval.
   */
  UMRBUS_DECL
  int 
  umrbus_interrupt(
                   UMR_HANDLE handle,
                   umrbus_interrupt_type *interrupt,
                   char **errormessage);
  
  /*!
   * \brief Search for all active UMRBusses and clients.
   * 
   * \param[out] errormessage
   * Address of a caller-provided variable which will be set to a error message pointer if the function fails.
   * In case if the function succeeds a NULL pointer is returned. 
   * 
   * \return
   * The function returns a pointer to a structure of type \p umrbus_scan_type on success. The last element of the 
   * structure is a NULL pointer. In case of an error a NULL pointer is returned. If an error occurred errormessage 
   * points to an error description. After an error occurred the error description must be freed with 
   * umrbus_errormessage_free().
   *
   * The function scans all active UMRBusses for UMRBus clients on the specified device and returns a structure
   * with retrieved scan information of the device.
   */
  UMRBUS_DECL
  umrbus_scan_type **  
  umrbus_scan(
              char **errormessage);

  /*!
   * \brief Search for all active UMRBusses and clients.
   *    
   * \param[in] pcHostname
   * Specifies the hostname to scan (host:port)
   *
   * \param[out] errormessage
   * Address of a caller-provided variable which will be set to a error message pointer if the function fails.
   * In case if the function succeeds a NULL pointer is returned. 
   * 
   * \return
   * The function returns a pointer to a structure of type \p umrbus_scan_type on success. The last element of the 
   * structure is a NULL pointer. In case of an error a NULL pointer is returned. If an error occurred errormessage 
   * points to an error description. After an error occurred the error description must be freed with 
   * umrbus_errormessage_free().
   *
   * The function scans all active UMRBusses for UMRBus clients on the specified device and returns a structure
   * with retrieved scan information of the device.
   */
  UMRBUS_DECL
  umrbus_scan_type **  
  umrbus_scan(
              const char* pcHostname,
              char **errormessage);

  /*!
   * \brief Free data returned by umrbus_scan().
   * 
   * \param[in] scandata
   * Pointer to scandata structure returned by umrbus_scan().
   * 
   * \return
   * The function has no return value.
   * 
   * The function frees the data returned by umrbus_scan(). The scan data becomes invalid and must
   * not be accessed after calling this function.
   */
  UMRBUS_DECL
  void
  umrbus_scan_free(
                   umrbus_scan_type **scandata);
  
  /*!
   * \brief Check the UMRBus integrity.
   * 
   * \param[in] device
   * Specifies the UMRBus device number. 
   * \li 0-3 : PCI(e) master devices
   * \li 4-7 : PCI(e) slave devices
   * \li > 8 : USB devices
   * 
   * \param[in] bus
   * Specifies the number of the UMRBus (0..7). 
   * 
   * \param[in] size
   * Specifies the number of 32-bit words which are to be transferred as test 1..65535.
   * 
   * \param[in] cycles
   * Specifies the number test cycles.
   * 
   * \param[out] errormessage
   * Address of a caller-provided variable which will be set to a error message pointer if the function fails.
   * In case if the function succeeds a NULL pointer is returned. 
   * 
   * \return
   * The function returns \c 0 if successful or \c 1 otherwise.
   * The errormessage variable points to an error description. The error description must be freed after error 
   * with umrbus_errormessage_free().
   * 
   * The function check the data transfer on the specified UMRBus using the transfer of 32-bit random words and 
   * comparing the transferred values.
   */
  UMRBUS_DECL
  int 
  umrbus_testframe(
                   unsigned int device,
                   unsigned int bus,
                   UMRBUS_LONG size,
                   UMRBUS_LONG cycles,
                   char **errormessage);

  /*!
   * \brief Check the UMRBus integrity.
   * 
   * \param[in] pcHostname
   * Specifies the hostname to connect to (host:port)
   *
   * \param[in] device
   * Specifies the UMRBus device number. 
   * \li 0-3 : PCI(e) master devices
   * \li 4-7 : PCI(e) slave devices
   * \li > 8 : USB devices
   * 
   * \param[in] bus
   * Specifies the number of the UMRBus (0..7). 
   * 
   * \param[in] size
   * Specifies the number of 32-bit words which are to be transferred as test 1..65535.
   * 
   * \param[in] cycles
   * Specifies the number test cycles.
   * 
   * \param[out] errormessage
   * Address of a caller-provided variable which will be set to a error message pointer if the function fails.
   * In case if the function succeeds a NULL pointer is returned. 
   * 
   * \return
   * The function returns \c 0 if successful or \c 1 otherwise.
   * The errormessage variable points to an error description. The error description must be freed after error 
   * with umrbus_errormessage_free().
   * 
   * The function check the data transfer on the specified UMRBus using the transfer of 32-bit random words and 
   * comparing the transferred values.
   */
  UMRBUS_DECL
  int 
  umrbus_testframe(
                   const char* pcHostname,
                   unsigned int device,
                   unsigned int bus,
                   UMRBUS_LONG size,
                   UMRBUS_LONG cycles,
                   char **errormessage);
  
  /*!
   * \brief Free error message returned by some API functions.
   * 
   * \param[in] errormessage
   * Address of the variable which holds the pointer to the error message string. 
   * 
   * \return
   * The function has no return value.
   * 
   * The function frees the memory used to store the error message. The error message string becomes invalid 
   * and must not be accessed after calling this function.
   */
  UMRBUS_DECL
  void 
  umrbus_errormessage_free(
                           char **errormessage);


/*!
 * \brief Enables or Disables UMRBus 3.0 compatibility mode.
 *
 * \param[in] enable
 * Set this to 0 to disable compatibility mode and 1 to enable.
 * Compatibility mode is enabled by default.
 *
 * \return
 * The function has no return value.
 *
 * The function enables or disables the UMRBus 3.0 compatibility mode
 * for umrbus_scan..
 */
  UMRBUS_DECL
  void
  umrbus_enable_compatibility_mode(
      unsigned int enable);

}
using namespace umr;

#else
#include "umrbus_int.h"
#endif //USE_UMRBUS_NET_ACCESS && __cplusplus

#endif // __umrbus_h__
