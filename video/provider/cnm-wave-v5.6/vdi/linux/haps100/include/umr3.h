/******************************************************************************
 Copyright (C) 2000 - 2016 Synopsys, Inc.
 This software and the associated documentation are confidential and
 proprietary to Synopsys, Inc. Your use or disclosure of this software
 is subject to the terms and conditions of a written license agreement
 between you, or your company, and Synopsys, Inc.
 *******************************************************************************
 Title      : UMR3 API
 *******************************************************************************
 Description: This file implements the UMR3 API functions.
 *******************************************************************************
 File       : umr3.h
 *******************************************************************************
 $Id:$
 ******************************************************************************/
/*! \file */

#ifndef __UMR3_H__
#define __UMR3_H__

// Includes
#include <stdbool.h>
#include <stdint.h>
#if defined(_WIN32)
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Windows function decoration
#if defined(_WIN32)
#if !defined(NODLL)
#if defined(UMR3API_EXPORTS)
// API function declaration used if the .h file is included by the implementation itself.
#define UMR3API_DECL __declspec(dllexport)
#else
// API function declaration used if the .h file is included (imported) by client code.
#define UMR3API_DECL __declspec(dllimport)
#endif
#else
#define UMR3API_DECL
#endif
#else
// Linux
#define UMR3API_DECL
#endif

/*!
 *  \defgroup constants_types Constants and types
 *  @{
 *  \brief Constants and types used in the programming interface.
 */

///////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
// Types
///////////////////////////////////////////////////////////////////
/* OS specific handles for the client applications */

#if defined(_WIN32)
typedef HANDLE UMR3_HANDLE;
#define UMR3_INVALID_HANDLE INVALID_HANDLE_VALUE
#else
typedef int UMR3_HANDLE; /**< UMRBus handle to client/master instance */
#define UMR3_INVALID_HANDLE -1    /**< Invalid UMR3_HANDLE value */
#endif

#define UMR3_STATUS int /**< Status code returned by API functions. */
#define UMR3_SUCCESS 0   /**< Return value for successful operation */

//! UMRBus master dma type
typedef enum Umr3DmaModeEnum {
  OFFSET, /**< CPU copies transferred data from/to host memory. */
  DIRECT /**< Direct DMA transfer from/to host memory. */
} UMR3_DMAMODE;

//! UMRBus log message level
typedef enum Umr3LogLevelEnum {
  UMR3_LOG_NONE, /**< No message is reported (default). */
  UMR3_LOG_ERROR, /**< Only errors are reported. */
  UMR3_LOG_WARN, /**< Errors and warnings are reported. */
  UMR3_LOG_INFO, /**< Error messages and warnings are printed to stderr
   and informational messages are printed to stdout */
  UMR3_LOG_DEBUG /**< Error messages and warnings are printed to stderr
   and informational and debug messages are printed to stdout. */
} UMR3_LOG_LEVEL; /**< UMRBus log message level */

/*!
 *  \struct Umr3VersionStruct
 *  \brief Structure containing umr library and driver version information.
 */
typedef struct Umr3VersionStruct {
  uint32_t LibraryMajor; /**< library version major part */
  uint32_t LibraryMinor; /**< library version minor part */
  uint32_t DriverMajor;  /**< driver version major part */
  uint32_t DriverMinor;  /**< driver version minor part */
  uint32_t DriverSub;    /**< driver version sub part */
  uint32_t Flags;        /**< reserved for future use */
} UMR3_VERSION; /**< Structure containing umr library and driver version information.*/

/*!
 *  \struct Umr3MasterResponse_struct
 *  \brief Structure containing master access callback parameters
 */
typedef struct Umr3MasterResponse_struct {
  uint32_t *address; /**< points to the transaction data */
  uint32_t size; /**< the number of uint32_t data elements available */
  uint8_t tid; /**< the transaction ID */
  int status; /**< the transaction status */
  uint32_t last_length; /**< the length of the last read or write */
  uint32_t mode; /**< DIRECT or COPY mode */
  uint64_t bmid; /**< Physical address */
} Umr3MasterResponse; /**< Structure containing master access callback parameters */

/*!
 *  \struct Umr3ScanData_struct
 *  \brief Structure containing umr3_scan results
 */
typedef struct Umr3ScanData_struct {
  char *device; /**< Device name */
  char *name; /**< MCAPIM name */
  char *path; /**< Path name including device but without name */
  uint64_t address; /**< Physical address */
  uint8_t connected; /**< Connection state */
} Umr3ScanData; /**< Structure containing umr3_scan results */

/*!
 *  @}
 */

/*!
 *  \page api API Index
 *
 *  This is the complete list of UMRBus 3.0 functions, types and enumerations.
 *
 *  \section sec_api_functions API Functions
 *   - umr3_version_get()
 *   - umr3_strerror()
 *   - umr3_log_set()
 *   - umr3_scan()
 *   - umr3_scan_device()
 *   - umr3_scan_free()
 *   - umr3_open()
 *   - umr3_popen()
 *   - umr3_write()
 *   - umr3_read()
 *  \section sec_intr_functions Interrupt Functions
 *   - umr3_interrupt_enable()
 *   - umr3_interrupt()
 *   - umr3_interrupt_register_callback()
 *  \section sec_master_functions Master Functions
 *   - umr3_master_alloc()
 *   - umr3_master_enable()
 *   - umr3_mwrite_register_callback()
 *   - umr3_mwrite()
 *   - umr3_mwrite_ack()
 *   - umr3_mread()
 *   - umr3_mread_register_callback()
 *
 *  \section sec_api_structs Structures
 *   - UmrVersionStruct
 *   - UmrScanStruct
 *   - UmrContextStruct
 *
 *  \section sec_api_enums Enumerations
 *   - \ref UMR3_LOG_LEVEL
 *   - \ref UMR3_DMAMODE
 */

/*!
 *  \page basic_lib_usage Basic Library Usage
 *
 *  This page documents the usage of the basic library functions.
 *
 *  \section sec_intr_cb_init Initialize Event Handler
 *
 *
 *  The following pseudo code shows how an application should use the basic library functions.
 *  This is not a complete C source code example. Error handling is not shown.
 *
 *  \code
 *
 *    UMR3_HANDLE    hUmr
 *    UMR3_STATUS    st;
 *    Umr3ScanData **scan_data;
 *
 *    // scan UMRBus
 *    st = umr3_scan(&scan_data);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling with umr3_strerror(st)
 *    }
 *
 *    // print all MCAPIMs
 *    uint64_t i = 0;
 *    while (scan_data[i]) {
 *        printf("MCAPIM  %s, %s, 0x%lX %s %d\n",
 *                        scan_data[i]->device,
 *                        scan_data[i]->name,
 *                        scan_data[i]->address,
 *                        scan_data[i]->path,
 *                        scan_data[i]->connected);
 *        i++;
 *    }
 *    umr3_scan_free(&scan_data);
 *  
 *    // open an MCAPIM
 *    st = umr3_popen(deviceName, address, &hUmr);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling with umr3_strerror(st)
 *    }
 *
 *    // enable interrupt requests
 *    st = umr3_interrupt_enable(hUmr, 1);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling with umr3_strerror(st)
 *    }
 *
 *    // write 2 data words
 *    uint32_t data[2] = {0x12345678, 0x5555aaaa};
 *    st = umr3_write(hUmr, data, 2);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling with umr3_strerror(st)
 *    }
 *
 *    // wait 1 second for interrupt
 *    uint32_t int_req, int_type;
 *    st = umr3_interrupt(hUmr, 1, &int_req, &int_type);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling with umr3_strerror(st)
 *    }
 *
 *    // read 2 data words
 *    st = umr3_read(hUmr, &data, 2);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling with umr3_strerror(st)
 *    }
 *
 *    // close MCAPIM
 *    st = umr3_close(hUmr);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling with umr3_strerror(st)
 *    }
 *
 *  \endcode
 *
 */

/*!
 *  \page intr_cb_usage Interrupt Event Handling
 *
 *  This page documents the interrupt function event handling and initialization.
 *
 *  \section sec_intr_cb_init Initialize Event Handler
 *
 *  The interrupt event handler needs to be initialized propperly before it can be used first time.
 *  - Registering the event handler
 *  - Enabling interrupt requests in the MCAPIM
 *
 *  The following pseudo code shows how an application should implement the interrupt event handler
 *  initialization. This is not a complete C source code example. Error handling is not shown.
 *
 *  \code
 *    // Interrupt event handler called once per interrupt
 *    void InterruptEventHandler(void *context, uint32_t intrType)
 *    {
 *        // context can be any user pointer e.g. UMR3 handle
 *        UMR3_HANDLE *hUmr = (UMR3_HANDLE*) context;
 *
 *        // handle interrupt
 *        switch (intrType) {
 *            case ...
 *        }
 *    }
 *
 *    UMR3_STATUS  st;
 *
 *    // register event handler
 *    st = umr3_interrupt_register_callback(hUmr, (void*) &hUmr, InterruptEventHandler);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling
 *    }
 *
 *    // enable interrupt requests
 *    st = umr3_interrupt_enable(hUmr, 1);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling
 *    }
 *  \endcode
 *
 *  \note It is possible to call a UMRBus function in the handler. The UMR handle needs to be passed as context pointer.
 *
 *  \section sec_intr_cb_cleanup Cleanup Event Handler
 *
 *  The interrupt handler needs to be cleaned up propperly when it is not used anymore by:
 *  - Disabling interrupts requests
 *  - Un-registering event handler
 *
 *  \code
 *    // disable interrupt requests
 *    st = umr3_interrupt_enable(hUmr, 0);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling
 *    }
 *
 *    // unregister event handler
 *    st = umr3_interrupt_register_callback(hUmr, NULL, NULL);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling
 *    }
 *
 *  \endcode
 *
 *  \note When the connection to the MCAPIM is closed by using umr3_close() the library will automatically disable
 *  interrupt requests and de-registered the event handler for this MCAPIM.
 */

/*!
 *  \page master_cb_usage Master Event Handling
 *
 *  This page documents the master function event handling and initialization.
 *
 *  \section sec_master_cb_init Initialize Event Handler
 *
 *  Before the master application can start the first data transfer, the DMA memory and
 *  event handler needs to be initialized in the correct order:
 *  - Allocating the data buffer
 *  - Configuring master access by spcifying the DMA mode
 *  - Registering the event handler
 *  - Enabling master transfers
 *
 *  The initialization starts with allocating the buffer used in the data transfer. This is
 *  done by calling \ref umr3_master_alloc.
 *  Subsequently the callback has to be registered by calling umr3_mwrite_register_callback().
 *  A pointer to a user context structure can be given for later usage in the event handler.
 *  When the callback is registered successfully the master transfer needs to be enabled by
 *  umr3_master_enable(). Once enabled successfully, the callback is executed when a new master access
 *  is completed. The following pseudo code shows how an application should implement master
 *  event handler initialization. This is not a complete C source code example. Error handling
 *  is not shown.
 *
 *  \code
 *    // master write event handler
 *    void MwriteEventHandler(void *context, Umr3MasterResponse *response)
 *    {
 *        // context can be any user pointer e.g. UMR3 handle
 *        UMR3_HANDLE *hUmr = (UMR3_HANDLE*) context;
 *
 *        // handle mWrite completion
 *        for (i = 0; i < response->size; i++) {
 *            data[i] = response->address[i];
 *        }
 *        // acknowledge callback execution
 *        umr3_mwrite_ack(hUmr, response);
 *    }
 *
 *    uint32_t*  mwriteData   = NULL;
 *    uint32_t   sizeInDwords = 128;
 *    UMR3_STATUS  st;
 *
 *    // allocating data buffer
 *    st = umr3_master_alloc(hUmr, &mwriteData, NULL, sizeInDwords, OFFSET);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling
 *    }
 *
 *    // register event handler
 *    st = umr3_mwrite_register_callback(hUmr, (void *) &hUmr, MwriteEventHandler, OFFSET);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling
 *    }
 *
 *    // enable master transfer
 *    st = umr3_master_enable(hUmr, 1, OFFSET);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling
 *    }
 *  \endcode
 *
 *  \note The master transaction needs to be acknowledged in the callback function. If this is not
 *        done, no further callback executions will be triggered. It is possible to call other UMRBus
 *        functions in the handler.
 *
 *  \section sec_master_cb_cleanup Cleanup Event Handler
 *
 *  After finishing the data transfer the even handler needs to be cleaned up in the
 *  correct order:
 *  - Disabling master transfers
 *  - Un-registering event handler
 *  - Freeing DMA memory
 *
 *  \code
 *    // disable master transfer
 *    st = umr3_master_enable(hUmr, 0, OFFSET);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling
 *    }
 *
 *    // unregister event handler
 *    st = umr3_mwrite_register_callback(hUmr, NULL, NULL, OFFSET);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling
 *    }
 *
 *    // free dma buffer
 *    st = umr3_master_free(hUmr, OFFSET);
 *    if (st != UMR3_SUCCESS) {
 *        // Error handling
 *    }
 *
 *  \endcode
 *
 *  \note When the connection to the MCAPIM is closed by using umr3_close() the library will automatically disable
 *  master requests, de-registered the event handler and free the DMA memory.
 *
 */

/*!
 *  \defgroup cb_functions Callback functions
 *  @{
 *  \brief Functions provided by the application.
 */

/*!
 *  \brief Callback function executed on on UMRBus interrupt request.
 *
 *  \param[in] context
 *    The pointer value that was passed to \ref umr3_interrupt_register_callback when the callback was registered.
 *
 *  \param[in] intrType
 *    The interrupt type assigned to the MCAPIM on interrupt request.
 *
 *  The callback function executes in the context of an internal thread.
 *  So the callback is asynchronous to any other thread of the application.
 *
 *  The callback function should return as quickly as possible and should not block internally waiting
 *  for external events to occur. If the function blocks internally then this prevents subsequent
 *  notifications from being delivered.
 */
typedef void UMR3_INTERRUPT_CALLBACK(void *context, uint32_t intrType);

/*!
 *  \brief Callback function executed on on UMRBus mwrite completion.
 *
 *  \param[in] context
 *    The pointer value that was passed to \ref umr3_mwrite_register_callback when the callback was registered.
 *
 *  \param[in] response
 *    Pointer to a Umr3MasterResponse structure containing information like address where the data has been
 *    written, size of the written data, the transaction ID and a status (in case of any error during write).
 *
 *  The callback function executes in the context of an internal thread.
 *  So the callback is asynchronous to any other thread of the application.
 *
 *  The callback function should return as quickly as possible and should not block internally waiting
 *  for external events to occur. If the function blocks internally then this prevents subsequent
 *  notifications from being delivered.
 */
typedef void UMR3_MWRITE_CALLBACK(void *context, Umr3MasterResponse *response);

/*!
 *  \brief Callback function executed on on UMRBus mread completion.
 *
 *  \param[in] context
 *    The pointer value that was passed to \ref umr3_mread_register_callback when the callback was registered.
 *
 *  \param[in] response
 *    Response struct holding the data pointer, number of elements and transaction id.
 *
 *  The callback function executes in the context of an internal thread.
 *  So the callback is asynchronous to any other thread of the application.
 *
 *  The callback function should return as quickly as possible and should not block internally waiting
 *  for external events to occur. If the function blocks internally then this prevents subsequent
 *  notifications from being delivered.
 */
typedef void UMR3_MREAD_CALLBACK(void *context, Umr3MasterResponse *response);

/*!
 *  @}
 */

///////////////////////////////////////////////////////////////////////////////
/*!
 *  \defgroup api_functions General API functions
 *  @{
 * \brief Functions called by the application.
 */

/*!
 *  \brief Get library and driver version.
 *
 *  \param[in] deviceName
 *    Identifies the device instance (umr3pci0).
 *
 *  \param[in] version
 *    Address of a caller-provided variable that points to the UMR3_VERSION structure.
 *
 *  \return
 *    none
 *
 *  This function returns a struct containing library and driver versions. If a NULL pointer
 *  is passed for deviceName only the library version is returned.
 *
 */
UMR3_STATUS UMR3API_DECL umr3_version_get(const char *deviceName, UMR3_VERSION *version);

/*!
 *  \brief Return pointer to status string.
 *
 *  \param[in] statusCode
 *    Specifies the status code.
 *
 *  \return
 *    The function returns a pointer to a string that describes the status code or a NULL pointer
 *    if no status strting is available.
 *
 *  The function returns a pointer to a string that describes the statusCode passed in the argument.
 */
const char UMR3API_DECL* umr3_strerror(UMR3_STATUS statusCode);

/*!
 *  \brief Set the debug log level.
 *
 *  \param[in] level
 *    Specifies the runtime library logging level.
 *
 *  \param[in] logFileName
 *    Specifies filename and path of library log file.
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function sets the log message verbosity of the library.
 *  By default, the logging level is set to UMR3_LOG_NONE and no log messages are generated.
 *  To enable message logging during runtime the UMRLIB_LOG environment variable can be used.
 *  On library load the variable is analyzed and the logging level is set to this value. It uses
 *  the same level number as for umr3_log_set. If the environment variable is assigned the umr3_log_set
 *  function cannot overwrite this setting. If the environment variable UMRLIB_LOG_FILE is set all message
 *  logging is written to the specified file.
 */
UMR3_STATUS UMR3API_DECL umr3_log_set(UMR3_LOG_LEVEL level, char *logFileName);

/*!
 *  \brief Scan all active UMRBuses and MCAPIMs.
 *
 *  \param[out] scanData
 *    Address of a caller-provided variable which will be set to a pointer to the scanned UMRBus structure.
 *    If an error occurs this parameter is set to NULL.
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function returns a pointer to a struct of type Umr3ScanData that represents the scanned UMRBus
 *  structure if no error is occured. To free the memory allocated within this function umrbus_scan_free
 *  must be used.
 *
 */
UMR3_STATUS UMR3API_DECL umr3_scan(Umr3ScanData ***scanData);

/*!
 *  \brief Scan UMRBuses and MCAPIMs for a specific device.
 *
 *  \param[out] scanData
 *    Address of a caller-provided variable which will be set to a pointer to the scanned UMRBus structure.
 *    If an error occurs this parameter is set to NULL.
 *
 * \param[in] device
 *   Name of the device to be scanned.
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function returns a pointer to a struct of type Umr3ScanData that represents the scanned UMRBus
 *  structure if no error is occured. To free the memory allocated within this function umrbus_scan_free
 *  must be used.
 *
 */
UMR3_STATUS UMR3API_DECL umr3_scan_device(Umr3ScanData ***scanData, char *device);

/*!
 *  \brief Scan UMRBuses and MCAPIMs for a specific device and a BDM node.
 *
 *  \param[out] scanData
 *    Address of a caller-provided variable which will be set to a pointer to the scanned UMRBus structure.
 *    If an error occurs this parameter is set to NULL.
 *
 * \param[in] device
 *   Name of the device to be scanned.
 *
  * \param[in] bdm_address
 *   Address of the BDM node to be scanned.
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function returns a pointer to a struct of type Umr3ScanData that represents the scanned UMRBus
 *  structure if no error is occured. To free the memory allocated within this function umrbus_scan_free
 *  must be used.
 *
 */
UMR3_STATUS UMR3API_DECL umr3_scan_bdm(Umr3ScanData ***scanData, char *device, uint64_t bdm_address);

/*!
 * \brief Free data returned by umrbus_scan().
 *
 * \param[in] scanData
 *   Address of a caller-provided variable that points to the Umr3ScanData structure returned by umrbus_scan().
 *
 * \return
 *   The function has no return value.
 *
 * The function frees the data returned by umrbus_scan(). The scan data becomes invalid and must
 * not be accessed after calling this function.
 */
void UMR3API_DECL umr3_scan_free(Umr3ScanData ***scanData);

/*!
 *  @}
 */

///////////////////////////////////////////////////////////////////////////////
/*!
 *  \defgroup client_functions Client API functions
 *  @{
 *  \brief Functions called by the application.
 */

/*!
 *  \brief Opens the connection to an MCAPIM instance based on its logical name
 *
 * \param[in] pathName
 *   Relative or absolute path name for the MCAPIM.
 *
 *  \param[out] handle
 *    Address of a caller-provided variable which will be set to a handle value if the function succeeds.
 *    The handle represents the MCAPIM connection and is required for all subsequent API calls.
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function opens the connection to an MCAPIM instance based on its logical name.
 *  It is not allowed to open more than one connection to an MCAPIM..
 *
 */
UMR3_STATUS UMR3API_DECL umr3_open(const char *pathName, UMR3_HANDLE *handle);

/*!
 *  \brief Open an MCAPIM connection by its physical address.
 *
 *  \param[in] deviceName
 *  Identifies the device instance (umrpci0).
 *
 *  \param[in] mcapimAddr
 *    Address of the MCAPIM within a MCAPIM tree.
 *
 *  \param[out] handle
 *    Address of a caller-provided variable which will be set to a handle value if the function succeeds.
 *    The handle represents the MCAPIM connection and is required for all subsequent API calls.
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *    Some specific error codes are described below.
 *
 *  The function opens a connection to an MCAPIM instance by it's physical address. It is not allowed to open
 *  more than one connection to a MCAPIM. The function will return UMR3_ALREADY_IN_USE if there is already
 *  an open connection to this MCAPIM.
 *
 */
UMR3_STATUS UMR3API_DECL umr3_popen(const char *deviceName, uint64_t mcapimAddr, UMR3_HANDLE *handle);

/*!
 *  \brief Close connection to MCAPIM.
 *
 *  \param[in] handle
 *    Identifies the connection to the client instance.
 *
 *  \return
 *  The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *  If a valid handle is specified, the function returns always \c UMR3_SUCCESS.
 *
 *  The specified connection will be removed and the provided handle becomes invalid with this call.
 */
UMR3_STATUS UMR3API_DECL umr3_close(UMR3_HANDLE handle);

/*!
 *  \brief Write data to a UMRBus client.
 *
 *  \param[in] handle
 *    Identifies the connection to the client instance.
 *
 *  \param[in] writeData
 *    Caller-provided buffer that contains the data to be written.
 *
 *  \param[in,out] dwordsToWrite
 *    Specifies the number of dwords to write.
 *    The number must not exceed the size of the \p writeData buffer.
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function writes a specified number of dwords to an UMRBus client.
 */
UMR3_STATUS UMR3API_DECL umr3_write(UMR3_HANDLE handle, uint32_t *writeData, uint32_t dwordsToWrite);

/*!
 *  \brief Write data to a UMRBus client and wait for completion.
 *
 *  \param[in] handle
 *    Identifies the connection to the client instance.
 *
 *  \param[in] writeData
 *    Caller-provided buffer that contains the data to be written.
 *
 *  \param[in,out] dwordsToWrite
 *    Specifies the number of dwords to write.
 *    The number must not exceed the size of the \p writeData buffer.
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function writes a specified number of dwords to an UMRBus client and
 *  waits for the operation to complete.
 */
UMR3_STATUS UMR3API_DECL umr3_bwrite(UMR3_HANDLE handle, uint32_t *writeData, uint32_t dwordsToWrite);

/*!
 *  \brief Read data from an UMRBus client.
 *
 *  \param[in] handle
 *    Identifies the connection to the client instance.
 *
 *  \param[out] readData
 *    Caller-provided buffer that will receive the read data.
 *
 *  \param[in,out] dwordsToRead
 *    Specifies the number of 32-bit words that should be read.
 *    The number must not exceed the size of the \p readData buffer.
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function reads a specified number of DWORDS from an UMRBus client.
 */
UMR3_STATUS UMR3API_DECL umr3_read(UMR3_HANDLE handle, uint32_t *readData, uint32_t dwordsToRead);

/*!
 * @}
 */

///////////////////////////////////////////////////////////////////////////////
/*!
 *  \defgroup intr_functions Interrupt API functions
 *  @{
 *
 *  \brief Functions called by the application.
 */

/*!
 *  \brief Enable client interrupts
 *
 *  \param[in] handle
 *    Identifies the connection to the client instance.
 *
 *  \param[in] enable
 *    Enable (1) or disable (0) client interrupts 
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function enables the interrupt mechanism directly in the MCAPIM. Once the interrupts
 *  are enabled the incoming interrupt requests are stored in a driver internal interrupt queue
 *  associated to the handle. If the callback event mechanism is used the function must be called
 *  after the interrupt callback is registered successfully.
 */
UMR3_STATUS UMR3API_DECL umr3_interrupt_enable(UMR3_HANDLE handle, uint32_t enable);

/*!
 *  \brief Check if a client has requested an interrupt.
 *
 *  \param[in] handle
 *    Identifies the connection to the client instance.
 *
 *  \param[in]  waitTimeoutSec
 *    The interrupt timeout in seconds.
 *
 *  \param[out] intrActive
 *    Address of a caller-provided variable which will be set to 1 if an interrupt is pending,
 *    else set to 0.
 *
 *  \param[out] intrType
 *    Address of a caller-provided variable which will be set to the MCAPIM interrupt type on
 *    pending interrupt else 0.
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function waits up to \p waitTimeoutSec seconds for an interrupt from UMRBus client.
 */
UMR3_STATUS UMR3API_DECL umr3_interrupt(UMR3_HANDLE handle, uint32_t waitTimeoutSec, uint32_t *intrActive, uint32_t *intrType);

/*!
 *  \brief Register UMRBus interrupt callback.
 *
 *  \param[in] handle
 *    Identifies the connection to the client instance.
 *
 *  \param[in] context
 *    Pointer to context data for use in the callback function.
 *
 *  \param[in] callback
 *    Pointer to the callback function to be executed on UMRBus interrupt.
 *    Passing a NULL pointer deregisters the callback function.
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function registers the UMRBus interrupt callback. After the callback has
 *  been registered it is disabled by default and need to be enabled using this function.
 */
UMR3_STATUS UMR3API_DECL umr3_interrupt_register_callback(UMR3_HANDLE handle, void *context, UMR3_INTERRUPT_CALLBACK *callback);

/*!
 *  @}
 */

///////////////////////////////////////////////////////////////////////////////
/*!
 *  \defgroup master_functions Master API functions
 *  @{
 *
 *  \brief Functions called by the application.
 */

/*!
 *  \brief Enable master DMA access
 *
 *  \param[in] handle
 *    Identifies the connection to the client instance.
 *
 *  \param[in] enable
 *    Enable (1) or disable (0) master write transfers
 *
 *  \param[in] mode
 *    Specifies the DMA mode (DIRECT or OFFSET).
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function enables or disables master DMA access for the specific MCAPIM.
 *  The master buffer has to be allocated with \ref umr3_master_alloc and a callback
 *  function should be registered using \ref umr3_mwrite_register_callback  or
 *  \ref umr3_mread_register_callback before enabling DMA access.
 */
UMR3_STATUS UMR3API_DECL umr3_master_enable(UMR3_HANDLE handle, uint32_t enable, UMR3_DMAMODE mode);

/*!
 *  \brief Allocate the buffer for master write DMA access
 *
 *  \param[in] handle
 *    Identifies the connection to the client instance.
 *
 *  \param[out] buffer
 *    Address of a caller-provided variable which will receive a pointer to the allocated
 *    DMA buffer.
 *
 *  \param[in] size
 *    Specifies the requested buffer size in DWORDs.
 *
 *  \param[in] mode
 *    Specifies the DMA mode (DIRECT or OFFSET).
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function prepares the data buffer for DMA transfers. Depending on \p mode
 *  it will either allocate a small coherent DMA buffer and send the physical address and buffer
 *  length to the MCAPIM or lock the whole user space buffer in the physical memory and send physical
 *  page addresses, start offset and buffer length to the MCAPIM.
 *  These values are presented on the MAPI and must be handled by the master application. If the buffer
 *  is not used anymore it needs to be freed using \ref umr3_master_free.
 */
UMR3_STATUS UMR3API_DECL umr3_master_alloc(UMR3_HANDLE handle, uint32_t **buffer, uint32_t size, UMR3_DMAMODE mode);

/*!
 *  \brief Free the allocated master DMA buffer
 *
 *  \param[in] handle
 *    Identifies the connection to the client instance.
 *
 *  \param[in] mode
 *    Specifies the DMA mode (DIRECT or OFFSET).
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function frees the allocated master DMA buffer.
 */
UMR3_STATUS UMR3API_DECL umr3_master_free(UMR3_HANDLE handle, UMR3_DMAMODE mode);

/*!
 *  \brief Check or wait for completed master write transfers
 *
 *  \param[in] handle
 *    Identifies the connection to the client instance.
 *
 *  \param[in] waitTimeoutSec
 *    Specifies the timeout to wait in seconds. If waitTimeoutSec = 0 the function will only
 *    check if the transfer is finished and return immediately.
 *
 *  \param[out] response
 *    Pointer to a Umr3MasterResponse structure receiving information like address where the data has been
 *    written, size of the written data, the transaction ID and a status (in case of any error during write).
 *
 *  \param[in] mode
 *    Specifies the DMA mode (DIRECT or OFFSET).
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function checks or waits on completed mwrite transfers. Once all data is written into the host memory
 *  the function returns the transferred number of DWORDs.
 *  The function can be used in polling or wait mode.
 *  In polling mode, the function checks for receive data. If the data is not fully received dwordsReceived
 *  is set to zero. Otherwise the number of received DWORDs is returned.
 *  In wait mode, the function waits not more than a specified timeout until all data is received.
 *  If the data is not fully received the function returns with timeout dwordsReceived is set to zero.
 */
UMR3_STATUS UMR3API_DECL umr3_mwrite(UMR3_HANDLE handle, uint32_t waitTimeoutSec, Umr3MasterResponse *response, UMR3_DMAMODE mode);

/*!
 *  \brief Send an acknowledgment after handling a master write transfer
 *
 *  \param[in] handle
 *    Identifies the connection to the client instance.
 *
 *  \param[in] response
 *    Pointer to the Umr3MasterResponse which was returned by the umr3_mwrite function.
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, \c UMR3_TIMEOUT in wait mode
 *    if the data transfer is not completed or an error code otherwise.
 *
 *  The function sends the transfer response to the MAPI interface (\c mapi_rsp_id_valid).
 */
UMR3_STATUS UMR3API_DECL umr3_mwrite_ack(UMR3_HANDLE handle, Umr3MasterResponse *response);

/*!
 *  \brief Register mwrite callback.
 *
 *  \param[in] handle
 *    Identifies the connection to the client instance.
 *
 *  \param[in] context
 *    Pointer to context data for use in the callback function.
 *
 *  \param[in] callback
 *    Pointer to the callback function to be executed on mwrite completion.
 *    Passing a NULL pointer deregisters the callback function.
 *
 *  \param[in] mode
 *    Specifies the DMA mode (DIRECT or OFFSET).
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function registers a callback function. It starts an internal thread that runs whenever
 *  a full data packet is written into the host memory. The callback function executes in the context
 *  of an internal thread. It should return as quickly as possible and should not block internally
 *  waiting for external events to occur. Only one mwrite callback function can be registered for a
 *  given MCAPIM.
 *
 */
UMR3_STATUS UMR3API_DECL umr3_mwrite_register_callback(UMR3_HANDLE handle, void *context, UMR3_MWRITE_CALLBACK *callback, UMR3_DMAMODE mode);

/*!
 *  \brief Check or wait on completed master read
 *
 *  \param[in] handle
 *    Identifies the connection to the client instance.
 *
 *  \param[in] waitTimeoutSec
 *    Specifies the timeout to wait in seconds. If waitTimeoutSec = 0 the function will only
 *    check if the transfer is finished and return immediately.
 *
 *  \param[out] response
 *    Pointer to a Umr3MasterResponse structure receiving information like address where the data has been
 *    written, size of the written data, the transaction ID and a status (in case of any error during write).
 *
 *  \param[in] mode
 *    Specifies the DMA mode (DIRECT or OFFSET).
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function checks or waits on completed mread transfers. Once all read data is passed to the
 *  MCAPIM the function returns the transferred number of DWORDs. The function can be used in polling
 *  or wait mode.
 *  In polling mode, the function checks for receive data. If the data is not fully received dwordsReceived
 *  is set to zero. Otherwise the number of received DWORDs is returned.
 *  In wait mode, the function waits not more than a specified timeout until all data is received.
 */
UMR3_STATUS UMR3API_DECL umr3_mread(UMR3_HANDLE handle, uint32_t waitTimeoutSec, Umr3MasterResponse *response, UMR3_DMAMODE mode);

/*!
 *  \brief Register mread callback.
 *
 *  \param[in] handle
 *    Identifies the connection to the client instance.
 *
 *  \param[in] context
 *    Pointer to context data for use in the callback function.
 *
 *  \param[in] callback
 *    Pointer to the callback function to be executed on mread completion.
 *    Passing a NULL pointer deregisters the callback function.
 *
 *  \param[in] mode
 *    Specifies the DMA mode (DIRECT or OFFSET).
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function registers a callback function. It starts an internal thread that runs whenever
 *  a full data packet is read from the host memory. The callback function executes in the context
 *  of an internal thread. It should return as quickly as possible and should not block internally
 *  waiting for external events to occur. Only one mread callback function can be registered for a
 *  given MCAPIM.
 *
 */
UMR3_STATUS UMR3API_DECL umr3_mread_register_callback(UMR3_HANDLE handle, void *context, UMR3_MREAD_CALLBACK *callback, UMR3_DMAMODE mode);

/*!
 *  \brief Get the full path of the MCAPIM
 *
 *  \retval UMR3_STATUS
 *    Returns the full path of the MCAPIM or NULL if not found
 *
 *  The function gets the full path to the MCAPIM for a given handle
 */
UMR3API_DECL char *umr3_get_path(UMR3_HANDLE handle);

/*!
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif // __UMR3_H__
