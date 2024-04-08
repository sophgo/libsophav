/******************************************************************************
 Copyright (C) 2000 - 2016 Synopsys, Inc.
 This software and the associated documentation are confidential and
 proprietary to Synopsys, Inc. Your use or disclosure of this software
 is subject to the terms and conditions of a written license agreement
 between you, or your company, and Synopsys, Inc.
 *******************************************************************************
 Title      : UMR3 API
 *******************************************************************************
 Description: This is the common header file for the UMR3 Simulation Package
 *******************************************************************************
 File       : umr3_sim_common.h
 *******************************************************************************
 $Id:$
 ******************************************************************************/

#ifndef __UMR3_SIM_COMMON_H__
#define __UMR3_SIM_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(LINUX) || defined(UNIX)
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "umr3.h"
#include "umr3_status.h"

#ifdef WIN64
#define SOCKET uint64_t
#else
#define SOCKET int
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifdef DEBUG
#define DPRINT(...) printf("%s:%d: %s: ", __FILE__, __LINE__, __FUNCTION__); printf(__VA_ARGS__); printf("\n"); fflush(stdout);
#else
#define DPRINT(...)
#endif

#define UMR3_SIM_VERSION_H 0
#define UMR3_SIM_VERSION_L 1

/* translated io controls for simulation*/
enum e_IOCTL_SIM {
    IOCTL_SIM_GENERIC_GET_DRIVER_INFO = 1,
    IOCTL_SIM_UMR3_GET_DEVICE_INFO,
    IOCTL_SIM_UMR3_RESET_DEVICE,
    IOCTL_SIM_UMR3_OPEN,
    IOCTL_SIM_UMR3_SCAN_COUNT,
    IOCTL_SIM_UMR3_SCAN,
    IOCTL_SIM_UMR3_TEST,
    IOCTL_SIM_UMR3_INTR_ENABLE,
    IOCTL_SIM_UMR3_INTR,
    IOCTL_SIM_UMR3_MASTER_ENABLE,
    IOCTL_SIM_UMR3_MASTER_CONFIG,
    IOCTL_SIM_UMR3_MWRITE,
    IOCTL_SIM_UMR3_MREAD,
    IOCTL_SIM_UMR3_CONTROL,
    IOCTL_SIM_UMR3_MWRITE_ACK
};

#define IOCTL_UMR3_OPEN_HANDLE 0x4F50454E
#define IOCTL_UMR3_READ        0x52454144
#define IOCTL_UMR3_WRITE       0x57524954
#define IOCTL_UMR3_CLOSE       0x434C4F53

#define UMR3_SIM_LEN_LEN    sizeof(uint32_t)
#define UMR3_SIM_LEN_ID     sizeof(uint32_t)
#define UMR3_SIM_LEN_IOCTL  sizeof(uint32_t)
#define UMR3_SIM_LEN_HANDLE sizeof(uint32_t)
#define UMR3_SIM_LEN_RETURN sizeof(uint32_t)
    
class UMR3_DataPackage {

public:
  UMR3_DataPackage();
  UMR3_DataPackage(uint32_t IOCTL, uint32_t Handle, uint8_t* Payload = NULL, uint32_t PayloadLenByte = 0);
  ~UMR3_DataPackage();

  /* handle SetPayload carefully. Note that the class becomes owner
     of the payload pointer. may not be deleted outside!*/
  void SetPayload(uint8_t* payload, uint32_t len);  

  /* handle GetPayload carefully. Note that the class stays owner
     of the payload pointer. it may not be deleted outside!*/  
  void GetPayload(uint8_t** payload, uint32_t* len);

  /* send this package over network*/
  UMR3_STATUS Send(SOCKET iSocket);

  /* receive package over network*/
  UMR3_STATUS Receive(SOCKET iSocket);

public:
  uint32_t m_IOCTL;
  uint32_t m_Handle;
  uint32_t m_Return;
  uint32_t m_TransactionID;
  SOCKET   m_iSocket;
  bool     m_bTransferComplete;

protected:
  uint8_t* m_Payload;
  uint32_t m_PayloadLenByte;
};

#define MIN(A,B) (A < B ? A : B)
#define MAX(A,B) (A > B ? A : B)

#endif //__UMR3_SIM_COMMON_H__
