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

#ifndef __UMR3_INT_H__
#define __UMR3_INT_H__

#ifdef __cplusplus
extern "C" {
#endif

//! UMRBus scan object type
typedef enum Umr3ScanObjEnum {
  MCAPIM, /**< Scan object is BDM */
  BDM, /**< Scan object is MCAPIM */
  MCAPIM_DS, /**< Scan object is MCAPIM with downstream names */
} UMR3_SCAN_OBJ;

/*!
 * \struct Umr3ScanDevice
 * \brief  Structure containing umr client device information.
 */
typedef struct Umr3ScanDevice_struct {
  const char DeviceName[20];
  umr3_scan_item *Elements;
  uint64_t NumElements;
} Umr3ScanDevice;

/*!
 * \struct _Umr3ScanData
 * \brief  Structure containing umr client scan information.
 */
typedef struct _Umr3ScanData_struct {
  Umr3ScanDevice *Devices;
  uint64_t NumDevices;
} _Umr3ScanData;

//! UMRBus Test Mode
typedef enum Umr3TestMode {
  UMR3_TEST_CUSTOM, /**< Custom test pattern provided via testData */
  UMR3_TEST_PSEUDO_RANDOM, /**< Pseudo random */
  UMR3_TEST_INCREMENTAL, /**< Incremental values. */
  UMR3_TEST_TBD, /**< TBD (e.g. 0x00/0xff; 0x55/0xAA) */
} UMR3_TESTMODE;

/*!
 *  \brief Scan all active UMRBuses and MCAPIMs.
 *
 *  \param[out] scanData
 *    Address of a caller-provided variable which will be set to a pointer to the scanned UMRBus structure.
 *    If an error occurs this parameter is set to NULL.
 *
 *  \param[in] device
 *    Device name.
 *
  *  \param[in] bmid
 *    BMID for a the root node
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 *  The function returns a pointer to a struct of type _Umr3ScanData that represents the scanned UMRBus
 *  structure if no error is occured. To free the memory allocated within this function _umrbus_scan_free
 *  must be used.
 *
 */
UMR3_STATUS UMR3API_DECL _umr3_scan(_Umr3ScanData **scanData, char *device, uint64_t bmid);

/*!
 * \brief Free data returned by _umrbus_scan().
 *
 * \param[in] scanData
 *   Address of a caller-provided variable that points to the structure returned by _umrbus_scan().
 *
 * \return
 *   The function has no return value.
 *
 * The function frees the data returned by _umrbus_scan(). The scan data becomes invalid and must
 * not be accessed after calling this function.
 */
void UMR3API_DECL _umr3_scan_free(_Umr3ScanData *scanData);

/*!
 * \brief Function to check the integrity of the UMRBus
 *
 * \param[in] pathName
 *   Relative or absolute path name for the MCAPIM to test.
 *
 * \param[in] testMode
 *    Specified the pattern generation
 *    0 - Test pattern provided via testData
 *    1 - Pseudo random
 *    2 - Incremental values
 *    3 - TBD (e.g. 0x00/0xff; 0x55/0xAA)
 *
 * \param[INOUT] testData
 *   Address of a caller-provided
 buffer containing test data. If testMode 1 -3 is chosen,
 *   the buffer is updated with the test data when the function returns.
 *
 * \param[OUT] resultData
 *   Address of a caller-provided buffer containing result data. The buffer is updated
 *   with the result data for analysis.
 *
 * \param[IN] dwordsToTest
 *   Number of data words to transfer.
 *
 * \return
 *   The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 * This is a low level function to check the integrity of the UMRBus. The function sends test values to an
 * addressed MCAPIM and checks for correct return values. In case of a data error or timeout an error code
 * is returned. Depending on the testMode the function will generate the test data or accepts test data from user.
 * When the function returns it will always pass the generated test data and result data for further analysis.
 */
UMR3_STATUS UMR3API_DECL umr3_test(const char *pathName, UMR3_TESTMODE testMode, uint32_t *testData, uint32_t *resultData, uint32_t dwordsToTest);

/*!
 * \brief Function to check the integrity of the UMRBus
 *
 * \param[in] deviceName
 *   The name of the UMRBus3 device
 *
 * \param[in] mcapimAddr
 *   The numeric address of the MCAPIM to test.
 *
 * \param[in] testMode
 *    Specified the pattern generation
 *    0 - Test pattern provided via testData
 *    1 - Pseudo random
 *    2 - Incremental values
 *    3 - TBD (e.g. 0x00/0xff; 0x55/0xAA)
 *
 * \param[INOUT] testData
 *   Address of a caller-provided buffer containing test data. If testMode 1 -3 is chosen,
 *   the buffer is updated with the test data when the function returns.
 *
 * \param[OUT] resultData
 *   Address of a caller-provided buffer containing result data. The buffer is updated
 *   with the result data for analysis.
 *
 * \param[IN] dwordsToTest
 *   Number of data words to transfer.
 *
 * \return
 *   The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 * This is a low level function to check the integrity of the UMRBus. The function sends test values to an
 * addressed MCAPIM and checks for correct return values. In case of a data error or timeout an error code
 * is returned. Depending on the testMode the function will generate the test data or accepts test data from user.
 * When the function returns it will always pass the generated test data and result data for further analysis.
 */
UMR3_STATUS UMR3API_DECL umr3_ptest(const char *deviceName, uint64_t mcapimAddr, UMR3_TESTMODE testMode, uint32_t *testData, uint32_t *resultData, uint32_t dwordsToTest);
UMR3_STATUS UMR3API_DECL umr3_atest(const char *device, uint64_t address, UMR3_TESTMODE testMode, uint32_t *testData, uint32_t *resultData, uint32_t dwordsToTest);

/*!
 * \brief Function to check the integrity of the UMRBus
 *
 * \param[in] deviceName
 *   The name of the UMRBus3 device
 *
 * \param[in] mcapimAddr
 *   The numeric address of the MCAPIM to test.
 *
 * \param[in] testMode
 *    Specified the pattern generation
 *    0 - Test pattern provided via testData
 *    1 - Pseudo random
 *    2 - Incremental values
 *    3 - TBD (e.g. 0x00/0xff; 0x55/0xAA)
 *
 * \param[INOUT] testData
 *   Address of a caller-provided buffer containing test data. If testMode 1 -3 is chosen,
 *   the buffer is updated with the test data when the function returns.
 *
 * \param[OUT] resultData
 *   Address of a caller-provided buffer containing result data. The buffer is updated
 *   with the result data for analysis.
 *
 * \param[IN] dwordsToTest
 *   Number of data words to transfer.
 *
 * \param[INOUT] iterations
 *   Address to an integer that has the number of iterations.
 *   This contains the failed iteration in case of error.
 *
 * \return
 *   The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *
 * This is a low level function to check the integrity of the UMRBus. The function sends test values to an
 * addressed MCAPIM and checks for correct return values. In case of a data error or timeout an error code
 * is returned. Depending on the testMode the function will generate the test data or accepts test data from user.
 * When the function returns it will always pass the generated test data and result data for further analysis.
 */
UMR3_STATUS UMR3API_DECL umr3_ptest_iterations(const char *deviceName, uint64_t mcapimAddr, UMR3_TESTMODE testMode, uint32_t *testData, uint32_t *resultData, uint32_t dwordsToTest, size_t *iterations);

/*!
 *  \brief Write control bits to the MCAPIM or BDM control register by it's path.
 *
 * \param[in] pathName
 *   Relative or absolute path name for the MCAPIM.
 *
 *  \param[in] address
 *    Address of the Control register to be set.
 *
 *  \param[in] mask
 *    Bit mask for the control data. If the mask is 0 then the mask is assumed to by 0xFFFFFFFF.
 *
 *  \param[in] data
 *    The control data to be set.
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *    Some specific error codes are described below.
 *
 *  \retval UMR3_NOT_PRESENT
 *    The MCAPIM instance identified by \p mcapimAddr is not present in the system.
 *
 *  \retval UMR3_ALREADY_IN_USE
 *    Indicates that the MCAPIM instance is already connected.
 *
 *  The function write the control bits in the MCAPIM or BDM control register depending on the
 *  bit mask.
 *
 */
UMR3_STATUS UMR3API_DECL umr3_set_control_register(const char *pathName, uint8_t address, uint32_t mask, uint32_t data);

/*!
 *  \brief Write control bits to the MCAPIM or BDM control register by it's physical address.
 *
 *  \param[in] deviceName
 *  Identifies the device instance (umrpci0).
 *
 *  \param[in] mcapimAddr
 *    Address of the MCAPIM within a MCAPIM tree.
 *
 *  \param[in] address
 *    Address of the Control register to be set.
 *
 *  \param[in] mask
 *    Bit mask for the control data. If the mask is 0 then the mask is assumed to by 0xFFFFFFFF.
 *
 *  \param[in] data
 *    The control data to be set.
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *    Some specific error codes are described below.
 *
 *  \retval UMR3_NOT_PRESENT
 *    The MCAPIM instance identified by \p mcapimAddr is not present in the system.
 *
 *  \retval UMR3_ALREADY_IN_USE
 *    Indicates that the MCAPIM instance is already connected.
 *
 *  The function write the control bits in the MCAPIM or BDM control register depending on the
 *  bit mask.
 *
 */
UMR3_STATUS UMR3API_DECL umr3_pset_control_register(const char *deviceName, uint64_t mcapimAddr, uint8_t address, uint32_t mask, uint32_t data);

/*!
 *  \brief Read control bits to the MCAPIM or BDM control register by it's path.
 *
 * \param[in] pathName
 *   Relative or absolute path name for the MCAPIM.
 *
 *  \param[in] address
 *    Address of the Control register to be read.
 *
 *  \param[in] mask
 *    Bit mask for the control data. If the mask is 0 then the mask is assumed to by 0xFFFFFFFF.
 *
 *  \param[out] data
 *    The control data that is read.
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *    Some specific error codes are described below.
 *
 *  \retval UMR3_NOT_PRESENT
 *    The MCAPIM instance identified by \p mcapimAddr is not present in the system.
 *
 *  \retval UMR3_ALREADY_IN_USE
 *    Indicates that the MCAPIM instance is already connected.
 *
 *  The function reads the control bits in the MCAPIM or BDM control register depending on the
 *  bit mask.
 *
 */
UMR3_STATUS UMR3API_DECL umr3_get_control_register(const char *pathName, uint8_t address, uint32_t mask, uint32_t *data);

/*!
 *  \brief Read control bits to the MCAPIM or BDM control register by it's physical address.
 *
 *  \param[in] deviceName
 *  Identifies the device instance (umrpci0).
 *
 *  \param[in] mcapimAddr
 *    Address of the MCAPIM within a MCAPIM tree.
 *
 *  \param[in] address
 *    Address of the Control register to be read.
 *
 *  \param[in] mask
 *    Bit mask for the control data. If the mask is 0 then the mask is assumed to by 0xFFFFFFFF.
 *
 *  \param[out] data
 *    The control data that is read.
 *
 *  \return
 *    The function returns \c UMR3_SUCCESS if successful, an error code otherwise.
 *    Some specific error codes are described below.
 *
 *  \retval UMR3_NOT_PRESENT
 *    The MCAPIM instance identified by \p mcapimAddr is not present in the system.
 *
 *  \retval UMR3_ALREADY_IN_USE
 *    Indicates that the MCAPIM instance is already connected.
 *
 *  The function reads the control bits in the MCAPIM or BDM control register depending on the
 *  bit mask.
 *
 */
UMR3_STATUS UMR3API_DECL umr3_pget_control_register(const char *deviceName, uint64_t mcapimAddr, uint8_t address, uint32_t mask, uint32_t *data);
UMR3_STATUS UMR3API_DECL umr3_get_control_word(const char *deviceName, uint64_t mcapimAddr, uint8_t address, uint32_t mask, uint32_t *data);

/*!
 * \brief Close one or all the MCAPIMs on the given device
 *
 *  \param[in] deviceName
 *  Identifies the device instance (umrpci0).
 *
 *  \param[in] bmid
 *    Address of the MCAPIM (0 for all MCAPIMs).
 *
 *  \retval UMR3_STATUS
 *    UMR3_STATUS_SUCCESS or the corresponding error code.
 *
 *  The function closes one or all the MCAPIMs of a given device.
 */
UMR3_STATUS UMR3API_DECL umr3_close_mcapims(const char *deviceName, uint64_t bmid);

/*!
 * \brief Get the PCIe register value for a provided offset
 *
 *  \param[in] deviceName
 *  Identifies the device instance (umrpci0).
 *
 *  \param[in] address
 *    Register offset
 *
 *  \retval UMR3_STATUS
 *    UMR3_STATUS_SUCCESS or the corresponding error code.
 *
 *  The function returns the register content for a given offset.
 */
UMR3_STATUS UMR3API_DECL umr3_get_pcie_register(const char *deviceName, uint32_t *offset);

/*!
 *  \brief Get or Set the control registers of a MCAPIM or BDM
 *
 *  \param[in] deviceName
 *  Identifies the device instance (umrpci0).
 *
 *  \param[in] ctrl
 *    Pointer to the Umr3Control structure that contain function parameters
 *
 *  \retval UMR3_STATUS
 *    UMR3_STATUS_SUCCESS or the corresponding error code.
 *
 *  The function sets or gets the register content for a given MCAPIM or BDM.
 */
UMR3_STATUS UMR3API_DECL umr3_control_register_access(const char *deviceName, umr3_control_data *ctrl);

#ifdef __cplusplus
}
#endif

#endif // __UMR3_INT_H__
