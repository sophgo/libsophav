
/******************************************************************************
Copyright (C) 2000 - 2016 Synopsys, Inc.
This software and the associated documentation are confidential and
proprietary to Synopsys, Inc. Your use or disclosure of this software
is subject to the terms and conditions of a written license agreement
between you, or your company, and Synopsys, Inc.
*******************************************************************************
Title      : umr3 status codes
*******************************************************************************
Description: This file implements umr3 status codes and info strings
*******************************************************************************
File       : umr3_status.h
*******************************************************************************
$Id:$
******************************************************************************/

#ifndef __UMR3_STATUS_H__
#define __UMR3_STATUS_H__

#if !defined(_WIN32)
#include <stdbool.h>
#endif

#if !defined(UMR3_STATUS)
#define UMR3_STATUS int
#endif

#ifdef UMR3_STATUS_INFO
// UMR3_STATUS_INFO case define is already set.
#else
// not defined, only the status codes are used
#define UMR3_STATUS_INFO(value,info)
#endif // UMR3_STATUS_INFO

// Windows specific status code mapping
#if defined(_WIN32)
// custom bit, severity=error, facility code
// for private status codes we use the range 0xEE000001 ... 0xEE00FFFF.
#define UMR3_STATUS_CODE(x) ((UMR3_STATUS)( 0xEE000000L | ((x) & 0xFFFF)))
#else
// Linux specific status code mapping
// for private status codes we use the range 0xE0000001 ... 0xE0001FFF.
#define UMR3_STATUS_CODE(x) ((UMR3_STATUS)( 0xE0000000L | ((x) & 0x1FFF) ))
#endif

//////////////////////////////////////////////////////////////////////////////
// umr3 status codes and info strings
//////////////////////////////////////////////////////////////////////////////

#define UMR3_STATUS_SUCCESS 0
UMR3_STATUS_INFO(UMR3_STATUS_SUCCESS, "The operation was a success.")

#define UMR3_STATUS_FAILED UMR3_STATUS_CODE(0x00000001)
UMR3_STATUS_INFO(UMR3_STATUS_FAILED, "The operation failed.")

#define UMR3_STATUS_ALREADY_EXISTS UMR3_STATUS_CODE(0x00000002)
UMR3_STATUS_INFO(UMR3_STATUS_ALREADY_EXISTS, "The MCAPIM already exists in the scan tree.")

#define UMR3_STATUS_BUSY UMR3_STATUS_CODE(0x00000007)
UMR3_STATUS_INFO(UMR3_STATUS_BUSY, "The device is busy.")

#define UMR3_STATUS_DMA_BUFFER_NOT_CREATED UMR3_STATUS_CODE(0x0000000D)
UMR3_STATUS_INFO(UMR3_STATUS_DMA_BUFFER_NOT_CREATED, "A DMA buffer needs to be allocated before calling this function.")

#define UMR3_STATUS_INVALID_BUFFER_SIZE UMR3_STATUS_CODE(0x00000012)
UMR3_STATUS_INFO(UMR3_STATUS_INVALID_BUFFER_SIZE, "An invalid buffer size was detected.  It is either small or large for a a memcpy.")

#define UMR3_STATUS_INVALID_DEVICE_CONFIG UMR3_STATUS_CODE(0x00000014)
UMR3_STATUS_INFO(UMR3_STATUS_INVALID_DEVICE_CONFIG, "An invalid device configration was detected.")

#define UMR3_STATUS_INVALID_INDEX UMR3_STATUS_CODE(0x00000015)
UMR3_STATUS_INFO(UMR3_STATUS_INVALID_INDEX, "The index is invalid for the provided list.")

#define UMR3_STATUS_INVALID_PARAMETER UMR3_STATUS_CODE(0x0000001A)
UMR3_STATUS_INFO(UMR3_STATUS_INVALID_PARAMETER, "An invalid parameter was passed to the function.")

#define UMR3_STATUS_INVALID_TYPE UMR3_STATUS_CODE(0x0000001C)
UMR3_STATUS_INFO(UMR3_STATUS_INVALID_TYPE, "An invalid type was specified.")

#define UMR3_STATUS_INVALID_UMRHANDLE UMR3_STATUS_CODE(0x0000001D)
UMR3_STATUS_INFO(UMR3_STATUS_INVALID_UMRHANDLE, "The UMRBus 3.0 handle is invalid.")

#define UMR3_STATUS_OUT_OF_MEMORY UMR3_STATUS_CODE(0x00000020)
UMR3_STATUS_INFO(UMR3_STATUS_OUT_OF_MEMORY, "Memory could not be allocated on the host.")

#define UMR3_STATUS_NO_MORE_ITEMS UMR3_STATUS_CODE(0x00000022)
UMR3_STATUS_INFO(UMR3_STATUS_NO_MORE_ITEMS, "There are no more items in the list.")

#define UMR3_STATUS_NO_DEVICES UMR3_STATUS_CODE(0x00000023)
UMR3_STATUS_INFO(UMR3_STATUS_NO_DEVICES, "There are no UMRBus 3.0 devices connected to the host.")

#define UMR3_STATUS_NOT_POSSIBLE UMR3_STATUS_CODE(0x00000025)
UMR3_STATUS_INFO(UMR3_STATUS_NOT_POSSIBLE, "The operation is not possible on the target interface.")

#define UMR3_STATUS_NOT_SUPPORTED UMR3_STATUS_CODE(0x00000026)
UMR3_STATUS_INFO(UMR3_STATUS_NOT_SUPPORTED, "The operation is not supported on the target interface.")

#define UMR3_STATUS_OBJECT_NOT_FOUND UMR3_STATUS_CODE(0x00000029)
UMR3_STATUS_INFO(UMR3_STATUS_OBJECT_NOT_FOUND, "The requested object was not found.")

#define UMR3_STATUS_OPEN_FILE_FAILED UMR3_STATUS_CODE(0x0000002A)
UMR3_STATUS_INFO(UMR3_STATUS_OPEN_FILE_FAILED, "Unable to open the requested file.")

#define UMR3_STATUS_TIMEOUT UMR3_STATUS_CODE(0x00000035)
UMR3_STATUS_INFO(UMR3_STATUS_TIMEOUT, "A timeout occurred while performing an operation on the UMRBus 3.0.")

#define UMR3_STATUS_UNEXPECTED_DEVICE_RESPONSE UMR3_STATUS_CODE(0x00000036)
UMR3_STATUS_INFO(UMR3_STATUS_UNEXPECTED_DEVICE_RESPONSE, "An unexpected or invalid response was received from the device.")

#define UMR3_STATUS_HIM_RESET_TIMEOUT UMR3_STATUS_CODE(0x0000003E)
UMR3_STATUS_INFO(UMR3_STATUS_HIM_RESET_TIMEOUT, "Driver initiated reset of the USB HIM timed out.")

#define UMR3_STATUS_INVALID_IOCTL UMR3_STATUS_CODE(0x00000040)
UMR3_STATUS_INFO(UMR3_STATUS_INVALID_IOCTL, "The iocontrol command is not supported by the driver.")

#define UMR3_STATUS_INVALID_LENGTH UMR3_STATUS_CODE(0x00000042)
UMR3_STATUS_INFO(UMR3_STATUS_INVALID_LENGTH, "The provided length is invalid.")

#define UMR3_STATUS_VERSION_MISMATCH UMR3_STATUS_CODE(0x00000060)
UMR3_STATUS_INFO(UMR3_STATUS_VERSION_MISMATCH, "The version of the drriver does not match with the library.")

#define UMR3_STATUS_LENGTH_MISMATCH UMR3_STATUS_CODE(0x00000061)
UMR3_STATUS_INFO(UMR3_STATUS_LENGTH_MISMATCH, "Requested and response sizes do not match.")

#define UMR3_STATUS_DEVICE_REMOVED UMR3_STATUS_CODE(0x00000071)
UMR3_STATUS_INFO(UMR3_STATUS_DEVICE_REMOVED, "The UMRBus 3.0 device is removed.")

#define UMR3_STATUS_BUFFER_TOO_SMALL UMR3_STATUS_CODE(0x00000074)
UMR3_STATUS_INFO(UMR3_STATUS_BUFFER_TOO_SMALL, "The buffer is too small for the requested operation.")

#define UMR3_STATUS_CALLBACK_ALREADY_IN_USE UMR3_STATUS_CODE(0x00000086)
UMR3_STATUS_INFO(UMR3_STATUS_CALLBACK_ALREADY_IN_USE, "A callback function is already registered for this operation.")

#define UMR3_STATUS_SIM_ENV_NOT_SET UMR3_STATUS_CODE(0x00000093)
UMR3_STATUS_INFO(UMR3_STATUS_SIM_ENV_NOT_SET, "Expected environment variables UMR3_SIM_SERVER_ADDR and UMR3_SIM_SERVER_PORT not set.")

#define UMR3_STATUS_SIM_SERVER_NOT_RUNNING UMR3_STATUS_CODE(0x00000094)
UMR3_STATUS_INFO(UMR3_STATUS_SIM_SERVER_NOT_RUNNING, "Cannot connect to the simulation server.")

#define UMR3_STATUS_SIM_FAILED_SEND UMR3_STATUS_CODE(0x00000096)
UMR3_STATUS_INFO(UMR3_STATUS_SIM_FAILED_SEND, "Failed to send data to server. Probably lost the connection.")

#define UMR3_STATUS_SIM_FAILED_RECV UMR3_STATUS_CODE(0x00000097)
UMR3_STATUS_INFO(UMR3_STATUS_SIM_FAILED_RECV, "Failed to receive data from server. Probably lost the connection.")

#define UMR3_STATUS_CAPIM_NOT_CONNECTED UMR3_STATUS_CODE(0x0000009A)
UMR3_STATUS_INFO(UMR3_STATUS_CAPIM_NOT_CONNECTED, "The MCAPIM is no longer available.")

#define UMR3_STATUS_COPY_TO_USER_FAILED UMR3_STATUS_CODE(0x0000009B)
UMR3_STATUS_INFO(UMR3_STATUS_COPY_TO_USER_FAILED, "Unable to copy data from the driver to the user memory.")

#define UMR3_STATUS_COPY_FROM_USER_FAILED UMR3_STATUS_CODE(0x0000009C)
UMR3_STATUS_INFO(UMR3_STATUS_COPY_FROM_USER_FAILED, "Unable to copy data from the user memory to the driver.")

#define UMR3_STATUS_INVALID_CAPIM_ADDRESS UMR3_STATUS_CODE(0x0000009E)
UMR3_STATUS_INFO(UMR3_STATUS_INVALID_CAPIM_ADDRESS, "An invalid MCAPIM address was provided.")

#define UMR3_STATUS_CAPIM_NOT_PRESENT UMR3_STATUS_CODE(0x0000009F)
UMR3_STATUS_INFO(UMR3_STATUS_CAPIM_NOT_PRESENT, "The MCAPIM wth the provided address does not exist on the device.")

#define UMR3_STATUS_CAPIM_ALREADY_CONNECTED UMR3_STATUS_CODE(0x000000A0)
UMR3_STATUS_INFO(UMR3_STATUS_CAPIM_ALREADY_CONNECTED, "The MCAPIM is already open.")

#define UMR3_STATUS_SIM_PARAMETER_INVALID UMR3_STATUS_CODE(0x000000A2)
UMR3_STATUS_INFO(UMR3_STATUS_SIM_PARAMETER_INVALID, "An invalid parameter was passed to the function.")

#define UMR3_STATUS_SIM_SERVER_FAILED UMR3_STATUS_CODE(0x000000A3)
UMR3_STATUS_INFO(UMR3_STATUS_SIM_SERVER_FAILED, "Error in server thread.")

#define UMR3_STATUS_INTI_NOT_SUPPORTED UMR3_STATUS_CODE(0x000000A4)
UMR3_STATUS_INFO(UMR3_STATUS_INTI_NOT_SUPPORTED, "INTI is not supported on the target MCAPIM.")

#define UMR3_STATUS_MAPI_NOT_SUPPORTED UMR3_STATUS_CODE(0x000000A5)
UMR3_STATUS_INFO(UMR3_STATUS_MAPI_NOT_SUPPORTED, "MAPI is not supported on the target MCAPIM.")

#define UMR3_STATUS_CAPI_NOT_SUPPORTED UMR3_STATUS_CODE(0x000000A6)
UMR3_STATUS_INFO(UMR3_STATUS_CAPI_NOT_SUPPORTED, "CAPI is not supported on the target MCAPIM.")

#define UMR3_STATUS_MAPI_NOT_ENABLED UMR3_STATUS_CODE(0x000000A7)
UMR3_STATUS_INFO(UMR3_STATUS_MAPI_NOT_ENABLED, "MAPI is not enabled by the user.")

#define UMR3_STATUS_INTI_NOT_ENABLED UMR3_STATUS_CODE(0x000000A8)
UMR3_STATUS_INFO(UMR3_STATUS_INTI_NOT_ENABLED, "INTI is not enabled by the user.")

#define UMR3_STATUS_MCAPIM_NOT_FOUND UMR3_STATUS_CODE(0x000000A9)
UMR3_STATUS_INFO(UMR3_STATUS_MCAPIM_NOT_FOUND, "The MCAPIM was not found on the device.")

#define UMR3_STATUS_TEST_FAILED UMR3_STATUS_CODE(0x000000AA)
UMR3_STATUS_INFO(UMR3_STATUS_TEST_FAILED, "UMRBus 3.0 test failed.")

#define UMR3_STATUS_WRITE_FAILED UMR3_STATUS_CODE(0x000000AB)
UMR3_STATUS_INFO(UMR3_STATUS_WRITE_FAILED, "Write operation failed on the target interface.")

#define UMR3_STATUS_READ_FAILED UMR3_STATUS_CODE(0x000000AC)
UMR3_STATUS_INFO(UMR3_STATUS_READ_FAILED, "The read operation failed on the target interface.")

#define UMR3_STATUS_MWRITE_ACKNOWLEDGE_ERROR UMR3_STATUS_CODE(0x000000AD)
UMR3_STATUS_INFO(UMR3_STATUS_MWRITE_ACKNOWLEDGE_ERROR, "An error ocurred while acknowleding a MAPI data frame.")

#define UMR3_STATUS_EMPTY_SCAN_RESULT UMR3_STATUS_CODE(0x000000AE)
UMR3_STATUS_INFO(UMR3_STATUS_EMPTY_SCAN_RESULT, "No MCAPIMs found in the device.")

#define UMR3_STATUS_INVALID_IOCTL_PARAMETER UMR3_STATUS_CODE(0x000000AF)
UMR3_STATUS_INFO(UMR3_STATUS_INVALID_IOCTL_PARAMETER, "An invalid parameter was passed to the iocontrol.")

#define UMR3_STATUS_SCAN_TIMEOUT UMR3_STATUS_CODE(0x000000B2)
UMR3_STATUS_INFO(UMR3_STATUS_SCAN_TIMEOUT, "Scan timed out while waiting for a complete scan tree.  A BDM or MCAPIM did not respond to a scan request.")

#define UMR3_STATUS_BUFFER_REQUEST_TIMEOUT UMR3_STATUS_CODE(0x000000B5)
UMR3_STATUS_INFO(UMR3_STATUS_BUFFER_REQUEST_TIMEOUT, "Unable to get a free HCC/URB buffer for host initiated transactions.")

#define UMR3_STATUS_MASTER_BUFFER_TOO_SMALL UMR3_STATUS_CODE(0x000000B6)
UMR3_STATUS_INFO(UMR3_STATUS_MASTER_BUFFER_TOO_SMALL, "The allocated MAPI buffer on the host is too small for the transaction.")

#define UMR3_STATUS_HCC_RESPONSE_TIMEOUT UMR3_STATUS_CODE(0x000000B7)
UMR3_STATUS_INFO(UMR3_STATUS_HCC_RESPONSE_TIMEOUT, "UMRBus 3.0 upstream communication failed. Could be 1.) Bad PCIe Link 2.) Missing frame 3.) No valid data")

#define UMR3_STATUS_MASTER_DISBLED UMR3_STATUS_CODE(0x000000B8)
UMR3_STATUS_INFO(UMR3_STATUS_MASTER_DISBLED, "MAPI is disabled.")

#define UMR3_STATUS_MASTER_WRITE_TIMEOUT UMR3_STATUS_CODE(0x000000B9)
UMR3_STATUS_INFO(UMR3_STATUS_MASTER_WRITE_TIMEOUT, "A master write request was not detected during the specified timeout time.")

#define UMR3_STATUS_MASTER_READ_TIMEOUT UMR3_STATUS_CODE(0x000000BA)
UMR3_STATUS_INFO(UMR3_STATUS_MASTER_READ_TIMEOUT, "A master read request was not detected during the specified timeout time.")

#define UMR3_STATUS_INTERRUPT_TIMEOUT UMR3_STATUS_CODE(0x000000BB)
UMR3_STATUS_INFO(UMR3_STATUS_INTERRUPT_TIMEOUT, "Missing UMRBus 3.0 interrupt. Could be 1.) Bad Link 2.) Not triggered 3.) Missing frame.")

#define UMR3_STATUS_INVALID_SCAN_RESPONSE UMR3_STATUS_CODE(0x000000BC)
UMR3_STATUS_INFO(UMR3_STATUS_INVALID_SCAN_RESPONSE, "A corrupted scan response was detected from the device.")

#define UMR3_STATUS_INVALID_MCAPIM_TYPE UMR3_STATUS_CODE(0x000000BD)
UMR3_STATUS_INFO(UMR3_STATUS_INVALID_MCAPIM_TYPE, "An invalid MCAPIM type was detected in the scan frame.")

#define UMR3_STATUS_URB_RESPONSE_TIMEOUT UMR3_STATUS_CODE(0x000000BF)
UMR3_STATUS_INFO(UMR3_STATUS_URB_RESPONSE_TIMEOUT, "UMRBus 3.0 upstream communication failed. Could be 1.) Bad USB Link 2.) Missing frame 3.) No valid data")

#endif // __UMR3_STATUS_H__
