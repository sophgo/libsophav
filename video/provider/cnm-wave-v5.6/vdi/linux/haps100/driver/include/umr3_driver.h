/******************************************************************************
Copyright (C) 2000 - 2016 Synopsys, Inc.
This software and the associated documentation are confidential and
proprietary to Synopsys, Inc. Your use or disclosure of this software
is subject to the terms and conditions of a written license agreement
between you, or your company, and Synopsys, Inc.
*******************************************************************************
Title      : umr3 status codes
*******************************************************************************
Description: UMRBus 3.0 Driver header
*******************************************************************************
File       : umr3_driver.h
*******************************************************************************
$Id:$
******************************************************************************/

#pragma once

#define UMR3_STRINGIFY_2(s) #s
#define UMR3_STRINGIFY(s) UMR3_STRINGIFY_2(s)

#define UMR3_USB_DEVICE_NAME "umr3usb"
#define UMR3_PCI_DEVICE_NAME "umr3pci"

#define UMR3_MAX_PCI_DEVICES 16
#define UMR3_MAX_USB_DEVICES 128

#define UMR3_INTERFACE_VERSION_MJ 93
#define UMR3_INTERFACE_VERSION_MN 0
#define UMR3_DRIVER_INTERFACE_VERSION UMR3_STRINGIFY(UMR3_INTERFACE_VERSION_MJ) "." UMR3_STRINGIFY(UMR3_INTERFACE_VERSION_MN)

/* WINDOWS DRIVER INTERFACE IDs */
#define UMR3_USB_IID { 0x86b53dda, 0x930a, 0x4b0e, { 0x80, 0x44, 0x20, 0xf4, 0x3f, 0xca, 0x31, 0x52 } }
#define UMR3_PCI_IID { 0x23380a07, 0x1713, 0x44da, { 0x90, 0x26, 0x34, 0xd8, 0x49, 0xc1, 0x05, 0x5b } }

#define UMR3_ASSERT(flag, check) ((((flag) & (check)) != 0) ? 1 : 0)

enum {
  UMR3_TYPE_MCAPIM = 0x0,
  UMR3_TYPE_BDM    = 0x1
};

enum {
  UMR3_DEVICE_PCI = 0x1,
  UMR3_DEVICE_USB = 0x2
};

enum {
  UMR3_IN      = 0x1,
  UMR3_OUT     = 0x2,
  UMR3_OFFSET  = 0x4,
  UMR3_DIRECT  = 0x8,
  UMR3_MREAD   = 0x10,
  UMR3_MWRITE  = 0x20,
  UMR3_FREE    = 0x40,
  UMR3_ALLOC   = 0x80,
  UMR3_CONFIG  = 0x100,
  UMR3_SET     = 0x200,
  UMR3_GET     = 0x400,
  UMR3_ENABLE  = 0x800,
  UMR3_DISABLE = 0x1000
};

typedef struct s_umr3_driver_data {
  uint16_t interface_major;
  uint16_t interface_minor;
  uint16_t version_major;
  uint16_t version_minor;
  uint16_t version_sub;
  uint32_t id;
  uint32_t flags;
} umr3_driver_data, *pumr3_driver_data;

// Scan configuration
typedef struct s_umr3_scan_data {
  uint64_t scan_data; /**< Pointer to scan buffer */
  uint64_t count; /**< Mcapims/BDMs found */
} umr3_scan_data, *pumr3_scan_data;

// MCAPIM information
typedef struct s_umr3_scan_item {
  uint64_t bmid;
  uint64_t parent_bmid;
  uint64_t pid;
  uint64_t rfu64;
  uint8_t  rfu8;
  union {
    uint32_t status_capability;
    struct {
      uint32_t odma_sup      :1;
      uint32_t ddma_sup      :1;
      uint32_t capi_sup      :1;
      uint32_t inti_sup      :1;
      uint32_t data_bw       :4;
      uint32_t version       :8;
      uint32_t type          :4;
      uint32_t valid         :1;
      uint32_t compatibility :1;
      uint32_t rfu           :6;
      uint32_t ds0_bdm_prsnt :1;
      uint32_t ds1_bdm_prsnt :1;
      uint32_t ds0_mc_prsnt  :1;
      uint32_t ds1_mc_prsnt  :1;
    } SC;
  };
  union {
    struct {
      union {
        uint32_t control;
        struct {
          uint32_t odma_en   :1;
          uint32_t ddma_en   :1;
          uint32_t int_en    :1;
          uint32_t connected :1;
          uint32_t xfer_size :14;
          uint32_t rfu2      :6;
          uint32_t hid       :8;
        } MCL;
      };
      uint8_t locked;
      uint64_t mapi_trf_addr;
      int8_t name[17];
      int8_t ds_name[2][17];
    } MCAPIM;
    struct {
      union {
        uint32_t control;
        struct {
          uint32_t ds0_en :1;
          uint32_t ds1_en :1;
          uint32_t rfu    :30;
        } BCL;
      };
      int8_t ds_name[2][17];
    } BDM;
  };
#if (defined(_WINDOWS_) && !defined(UMR3_SIM_LIB))
  struct s_umr3_scan_item *POINTER_64 us;
  struct s_umr3_scan_item *POINTER_64 ds[2];
#else
  struct s_umr3_scan_item *us;
  struct s_umr3_scan_item *ds[2];
#endif
} umr3_scan_item, *pumr3_scan_item;

// Test configuration and result data
typedef struct s_umr3_test_data {
  uint64_t ds; /**< Pointer to test data */
  uint64_t us; /**< Pointer to result data */
  uint32_t count; /**< Buffer size in 32bit words */
  uint64_t bmid; /**< Address of the MCAPIM */
} umr3_test_data, *pumr3_test_data;

// Interrupt configuration and result data
typedef struct s_umr3_interrupt_data {
  uint32_t config; /**< Interrupt configuration flags */
  uint32_t timeout; /**< Interrupt timeout in seconds */
  uint32_t active; /**< 1 if interrupt is active */
  uint32_t type; /**< Interrupt type returned if interrupt is active */
} umr3_interrupt_data, *pumr3_interrupt_data;

// Master configuration
typedef struct s_umr3_master_data {
  uint64_t buffer; /**< Base address of the allocated coherent buffer */
  uint32_t size; /**< Size in DWORDS of the buffer */
  uint32_t config; /**< Configuration data like Mode */
} umr3_master_data, *pumr3_master_data;

// Master send configuration and result data
typedef struct s_umr3_mread_data {
  int32_t status; /**< Errors if any */
  uint32_t config; /**< Config data / Mode */
  uint32_t count; /**< Number of 32bit words sent */
  uint32_t last_length; /**< Length of the last read in DWORDS */
  uint32_t timeout; /**< Msend timeout in seconds */
  uint64_t address; /**< Pointer to first sent word */
  uint32_t tid; /**< The transaction ID */
} umr3_mread_data, *pumr3_mread_data;

// Master receive configuration and result data
typedef struct umr3_mwrite_data {
  int status; /**< Errors if any */
  uint32_t config; /**< Config data / Mode */
  uint32_t count; /**< Number of 32bit words received */
  uint32_t last_length; /**< Length of the last write in DWORDS */
  uint32_t timeout; /**< Mrecv timeout in seconds */
  uint64_t address; /**< Pointer to first received word */
  uint32_t tid; /**< The transaction ID */
} umr3_mwrite_data, *pumr3_mwrite_data;

// MCAPIM/BDM Control register
typedef struct s_umr3_control_data {
  uint32_t data; /**< The 32 bit control word */
  uint32_t mask; /**< Mask for the bits to be read or written */
  uint64_t bmid; /**< Address of the MCAPIM */
  uint8_t rw; /**< If set to 1 just read control register and return. if 0 then write to register*/
  uint8_t address; /**< Address of the control register */
} umr3_control_data, *pumr3_control_data;

// Device Types
enum {
  UMR3_USB_DEVICE = 1,
  UMR3_PCI_DEVICE
};

// Host initiated BDM operations
#define UMR3_OC_H_BSCAN  0x20  //Scan for available BDM
#define UMR3_OC_H_BCWR   0x25  //BDM control register write
#define UMR3_OC_H_BCRD   0x26  //BDM control register read
// Host initiated MCAPIM operations
#define UMR3_OC_H_SCAN   0x10  // Scan for available MCAPIM
#define UMR3_OC_H_WR     0x11  // Write to MCAPIM
#define UMR3_OC_H_RD     0x12  // Read from MCAPIM
#define UMR3_OC_H_TEST   0x13  // MCAPIM data transfer test
#define UMR3_OC_H_CWR    0x15  // MCAPIM control register write
#define UMR3_OC_H_CRD    0x16  // MCAPIM control register read
#define UMR3_OC_H_CWRRD  0x17  // MCAPIM control register write/read (atomic operation)
// MCAPIM initiated operations
#define UMR3_OC_MC_WR    0x81  // MCAPIM initiated write to host
#define UMR3_OC_MC_RD    0x82  // MCAPIM initiated read from host
#define UMR3_OC_MC_DWR   0x85  // MCAPIM initiated write to host
#define UMR3_OC_MC_DRD   0x86  // MCAPIM initiated read from host
#define UMR3_OC_MC_INT   0x88  // MCAPIM interrupt request
// Register read operations only for USB
#define UMR3_OC_H_REG_RD 0xFE  // Register read request for USB
#define UMR3_OC_H_REG_WR 0xFF  // Register write request for USB

#define UMR3_MCAPIM_LOCK_BIT_POS       3
#define UMR3_MCAPIM_MAX_XFER_SIZE_POS  4
#define UMR3_MCAPIM_LOCK_BIT_MASK      0x1
#define UMR3_MCAPIM_MAX_XFER_SIZE_MASK 0x3FFF
#define UMR3_MCAPIM_CONTROL_ADDRESS    0x1
#define UMR3_MCAPIM_CONTROL_LENGTH     1

#define UMR3_SET_BITS(var, val, mask, pos)  var = (var & ~((uint64_t) mask << pos)) | ((uint64_t) (val & mask) << pos)
#define UMR3_GET_BITS(var, mask, pos)       (((uint64_t) (var) >> pos) & mask)
#define UMR3_FMT64(in)                      (uint32_t) ((((uint64_t) (in)) >> 32) & 0xFFFFFFFF), (uint32_t) ((in) & 0xFFFFFFFF)

#define UMR3_GET_CPRLEN(in)          UMR3_GET_BITS(in, 0xFFF, 0)
#define UMR3_MCAPIM_TYPE(p)          (((p) >> 16) & 0xf)
#define UMR3_BDM_TYPE(p)             UMR3_MCAPIM_TYPE(p)
#define UMR3_MCAPIM_VERSION(p)       (((p) >> 8) & 0xff)
#define UMR3_BDM_VERSION(p)          UMR3_MCAPIM_VERSION(p)
#define UMR3_MCAPIM_DS0_BDM_PRSNT(p)  ((p) & (1 << 28))
#define UMR3_BDM_DS0_BDM_PRSNT(p)     UMR3_MCAPIM_DS0_BDM_PRSNT(p)
#define UMR3_MCAPIM_DS1_BDM_PRSNT(p)  ((p) & (1 << 29))
#define UMR3_BDM_DS1_BDM_PRSNT(p)     UMR3_MCAPIM_DS1_BDM_PRSNT(p)
#define UMR3_MCAPIM_DS0_MC_PRSNT(p)  ((p) & (1 << 30))
#define UMR3_BDM_DS0_MC_PRSNT(p)     UMR3_MCAPIM_DS0_MC_PRSNT(p)
#define UMR3_MCAPIM_DS1_MC_PRSNT(p)  ((p) & (1 << 31))
#define UMR3_BDM_DS1_MC_PRSNT(p)     UMR3_MCAPIM_DS1_MC_PRSNT(p)

/* PCI device controller offsets */
#define VERSION     0x0000
#define SPARAM      0x0004
#define CLKCFG      0x0008
#define HCFG        0x000C
#define HCTL        0x0010
#define HSTAT       0x0014
#define HIREQ       0x0018
#define HICTL       0x001C
#define SQDMABA     0x0020
#define SQWRPA      0x0028
#define SQWRP       0x0030
#define SQRDP       0x0034
#define IQDMABA     0x0038
#define IQWRPA      0x0040
#define DBGTXDP     0x0040
#define DBGRXDP     0x0044
#define IQWRP       0x0048
#define IQRDP       0x004C
#define MQDMABA     0x0050
#define HRSPDMABA   0x0058
#define MQWRP       0x0060
#define MQRDP       0x0064
#define MPQDMABA    0x0068
#define MPQWRPA     0x0070
#define MPQWRP      0x0078
#define MPQRDP      0x007C
#define HREQDMABA_B 0x0080
#define SREQB       0x1000
#define SRSPB       0x10E0
#define HREQB_B     0x1000

#define HREQDMABA(offset) HREQDMABA_B + ((offset) * 0x8)
#define HREQB(offset)     HREQB_B + ((offset) * 0x100)
#define HRSPB(offset)     HREQB(offset) + 0xE0
#define UMR3_MAX_CLIENTS  0xFFFFFFFFFFLL

#ifdef _WIN32

// IOCTL command codes
#define CMD_GENERIC_GET_DRIVER_INFO    0x801
#define CMD_UMR3_GET_DEVICE_INFO       0x810
#define CMD_UMR3_RESET_DEVICE          0x811
#define CMD_UMR3_OPEN                  0x812
#define CMD_UMR3_SCAN_COUNT            0x813
#define CMD_UMR3_SCAN                  0x814
#define CMD_UMR3_TEST                  0x815
#define CMD_UMR3_INTR_ENABLE           0x816
#define CMD_UMR3_INTR                  0x818
#define CMD_UMR3_MASTER_ENABLE         0x81a
#define CMD_UMR3_MASTER_CONFIG         0x81b
#define CMD_UMR3_MWRITE                0x81c
#define CMD_UMR3_MWRITE_ACK            0x81d
#define CMD_UMR3_MREAD                 0x81e
#define CMD_UMR3_CONTROL               0x81f
#define CMD_UMR3_REGISTER_READ         0x824 // WINDOWS ONLY
#define CMD_UMR3_SCAN_DATA             0x831
#define CMD_UMR3_TEST_DATA             0x832

#define UMR3_FILE_DEVICE               0x8842

// IOCTL to retrieve driver versions
// Generic IOCTL Requests
#define IOCTL_GENERIC_GET_DRIVER_INFO    CTL_CODE(UMR3_FILE_DEVICE, CMD_GENERIC_GET_DRIVER_INFO, METHOD_BUFFERED, FILE_ANY_ACCESS)

// IOCTL to retrieve additional umr3 device information
#define IOCTL_UMR3_GET_DEVICE_INFO       CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_GET_DEVICE_INFO, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL to reset umr3 device
#define IOCTL_UMR3_RESET_DEVICE          CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_RESET_DEVICE, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL to open connection to MCAPIM
#define IOCTL_UMR3_OPEN                  CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_OPEN, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL to get the count of scan elements to allocate memory in user space
#define IOCTL_UMR3_SCAN_COUNT            CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_SCAN_COUNT, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL to scan the device structure
#define IOCTL_UMR3_SCAN                  CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_SCAN, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL to set MCAPIM scan data
#define IOCTL_UMR3_SCAN_DATA             CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_SCAN_DATA, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL to set MCAPIM tree data
#define IOCTL_UMR3_TEST_DATA             CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_TEST_DATA, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL to run MCAPIM tree testing
#define IOCTL_UMR3_TEST                  CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_TEST, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL to enable interrupt
#define IOCTL_UMR3_INTR_ENABLE           CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_INTR_ENABLE, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL to check or wait on interrupt
#define IOCTL_UMR3_INTR                  CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_INTR, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL to setup DMA buffers for mread
#define IOCTL_UMR3_MASTER_ENABLE         CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_MASTER_ENABLE, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL to setup DMA buffers for mread
#define IOCTL_UMR3_MASTER_CONFIG         CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_MASTER_CONFIG, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL to check or wait on mread completion
#define IOCTL_UMR3_MWRITE                CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_MWRITE, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL to check or wait on mread completion
#define IOCTL_UMR3_MWRITE_ACK            CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_MWRITE_ACK, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL to check or wait on mwrite completion
#define IOCTL_UMR3_MREAD                 CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_MREAD, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL to check or get the control register
#define IOCTL_UMR3_CONTROL               CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_CONTROL, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL to check or get the HIM registers
#define IOCTL_UMR3_REGISTER_READ         CTL_CODE(UMR3_FILE_DEVICE, CMD_UMR3_REGISTER_READ, METHOD_BUFFERED, FILE_ANY_ACCESS)

#else // Linux
#include <linux/ioctl.h>

// IOCTL command codes
#define CMD_GENERIC_GET_DRIVER_INFO    0x001

#define CMD_UMR3_GET_DEVICE_INFO       0x010
#define CMD_UMR3_RESET_DEVICE          0x011
#define CMD_UMR3_OPEN                  0x012
#define CMD_UMR3_SCAN_COUNT            0x013
#define CMD_UMR3_SCAN                  0x014
#define CMD_UMR3_TEST                  0x015
#define CMD_UMR3_INTR_ENABLE           0x016
#define CMD_UMR3_INTR                  0x018
#define CMD_UMR3_MASTER_ENABLE         0x01a
#define CMD_UMR3_MASTER_CONFIG         0x01b
#define CMD_UMR3_MWRITE                0x01c
#define CMD_UMR3_MWRITE_ACK            0x01d
#define CMD_UMR3_MREAD                 0x01e
#define CMD_UMR3_CONTROL               0x01f

#define CMD_UMR3_REGISTER_READ         0x024

#define CMD_UMR3_TEST_DATA             0x032 // UNUSED FOR LINUX

// Generic IOCTL Requests
#define IOCTL_GENERIC_GET_DRIVER_INFO _IOR(UMR3_INTERFACE_VERSION_MJ, CMD_GENERIC_GET_DRIVER_INFO, umr3_driver_data)

// IOCTL to retrieve additional umr3 device information
#define IOCTL_UMR3_GET_DEVICE_INFO  _IOR(UMR3_INTERFACE_VERSION_MJ, CMD_UMR3_GET_DEVICE_INFO, umr3_driver_data)
// IOCTL to reset umr3 device
#define IOCTL_UMR3_RESET_DEVICE     _IO(UMR3_INTERFACE_VERSION_MJ, CMD_UMR3_RESET_DEVICE)
// IOCTL to open connection to MCAPIM
#define IOCTL_UMR3_OPEN             _IOW(UMR3_INTERFACE_VERSION_MJ, CMD_UMR3_OPEN, uint64_t)
// IOCTL to get the count of scan elements to allocate memory in user space
#define IOCTL_UMR3_SCAN_COUNT       _IOR(UMR3_INTERFACE_VERSION_MJ, CMD_UMR3_SCAN_COUNT, uint64_t)
// IOCTL to scan the device structure
#define IOCTL_UMR3_SCAN             _IOWR(UMR3_INTERFACE_VERSION_MJ, CMD_UMR3_SCAN, umr3_scan_data)
// IOCTL to run MCAPIM tree testing
#define IOCTL_UMR3_TEST             _IOWR(UMR3_INTERFACE_VERSION_MJ, CMD_UMR3_TEST, umr3_test_data)
// IOCTL to enable interrupt
#define IOCTL_UMR3_INTR_ENABLE      _IOW(UMR3_INTERFACE_VERSION_MJ, CMD_UMR3_INTR_ENABLE, umr3_interrupt_data)
// IOCTL to check or wait on interrupt
#define IOCTL_UMR3_INTR             _IOWR(UMR3_INTERFACE_VERSION_MJ, CMD_UMR3_INTR, umr3_interrupt_data)
// IOCTL to setup DMA buffers for mread
#define IOCTL_UMR3_MASTER_ENABLE    _IOW(UMR3_INTERFACE_VERSION_MJ, CMD_UMR3_MASTER_ENABLE, umr3_master_data)
// IOCTL to setup DMA buffers for mread
#define IOCTL_UMR3_MASTER_CONFIG    _IOWR(UMR3_INTERFACE_VERSION_MJ, CMD_UMR3_MASTER_CONFIG, umr3_master_data)
// IOCTL to check or wait on mread completion
#define IOCTL_UMR3_MWRITE           _IOWR(UMR3_INTERFACE_VERSION_MJ, CMD_UMR3_MWRITE, umr3_mwrite_data)
// IOCTL to check or wait on mwrite completion
#define IOCTL_UMR3_MREAD            _IOWR(UMR3_INTERFACE_VERSION_MJ, CMD_UMR3_MREAD, umr3_mread_data)
// IOCTL to get the control register
#define IOCTL_UMR3_CONTROL          _IOWR(UMR3_INTERFACE_VERSION_MJ, CMD_UMR3_CONTROL, umr3_control_data)
// IOCTL for mwrite acknowledge
#define IOCTL_UMR3_MWRITE_ACK       _IOWR(UMR3_INTERFACE_VERSION_MJ, CMD_UMR3_MWRITE_ACK, umr3_mwrite_data)
// IOCTL to read HIM register
#define IOCTL_UMR3_REGISTER_READ    _IOWR(UMR3_INTERFACE_VERSION_MJ, CMD_UMR3_REGISTER_READ, uint32_t)

#endif // Linux
