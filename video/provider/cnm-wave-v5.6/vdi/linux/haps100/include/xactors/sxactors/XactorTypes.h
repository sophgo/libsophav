//-*- C++ -*-x
#pragma once
#include "sxactors/dllexport.h"

#define XACTOR_TYPE_INVALID             0

#define XACTOR_TYPE_HAS_CAPIM(ID) ((ID & 0x40000000) == 0)

// The count of the following types is XACTOR_TYPES_COUNT

// UART is a combination of different xactors
#define XACTOR_TYPE_UART_MASTER         0x40000001

// Single CAPIM transactors
#define XACTOR_TYPE_APB_MASTER          0x2000
#define XACTOR_TYPE_APB_SLAVE           0x2001
#define XACTOR_TYPE_AHB_MASTER          0x2002
#define XACTOR_TYPE_AHB_SLAVE           0x2003
#define XACTOR_TYPE_AXI_RD_MASTER       0x2004
#define XACTOR_TYPE_AXI_RD_SLAVE        0x2005
#define XACTOR_TYPE_AXI_WR_MASTER       0x2006
#define XACTOR_TYPE_AXI_WR_SLAVE        0x2007
#define XACTOR_TYPE_AXI4_RD_MASTER      0x2008
#define XACTOR_TYPE_AXI4_RD_SLAVE       0x2009
#define XACTOR_TYPE_AXI4_WR_MASTER      0x200A
#define XACTOR_TYPE_AXI4_WR_SLAVE       0x200B
#define XACTOR_TYPE_AXI_MASTER          0x200C
#define XACTOR_TYPE_AXI_SLAVE           0x200D
#define XACTOR_TYPE_AXI4_MASTER         0x200E
#define XACTOR_TYPE_AXI4_SLAVE          0x200F

#define XACTOR_TYPE_GPIO_MASTER         0x2010
// reserved 0x2011 - 201F

#define XACTOR_TYPE_AXI4L_RD_MASTER     0x2021
#define XACTOR_TYPE_AXI4L_RD_SLAVE      0x2022
#define XACTOR_TYPE_AXI4L_WR_MASTER     0x2023
#define XACTOR_TYPE_AXI4L_WR_SLAVE      0x2024
#define XACTOR_TYPE_AXI4L_MASTER        0x2025
#define XACTOR_TYPE_AXI4L_SLAVE         0x2026

#define XACTOR_TYPE_INT_MASTER          0x2027
#define XACTOR_TYPE_INT_SLAVE           0x2028
#define XACTOR_TYPE_STREAM_SLAVE        0x2029

#define XACTOR_CLOCK_RESET_CONTROL      0x202A

#define XACTOR_TYPE_COUNT               27


const char DLLEXPORT *XactorTypesGetTypeStr(unsigned int id);
unsigned int DLLEXPORT XactorTypesGetType(unsigned int id);

bool DLLEXPORT XactorTypesIsAPB(unsigned int id);
bool DLLEXPORT XactorTypesIsAHB(unsigned int id);
bool DLLEXPORT XactorTypesIsAXI(unsigned int id);
bool DLLEXPORT XactorTypesIsAXI4(unsigned int id);
bool DLLEXPORT XactorTypesIsAXI4L(unsigned int id);
bool DLLEXPORT XactorTypesIsCoreXactor(unsigned int id);
bool DLLEXPORT XactorTypesIsCoreMaster(unsigned int id);
bool DLLEXPORT XactorTypesIsCoreSlave(unsigned int id);
bool DLLEXPORT XactorTypesIsIntMaster(unsigned int id);
bool DLLEXPORT XactorTypesIsIntSlave(unsigned int id);
bool DLLEXPORT XactorTypesIsStreamSlave(unsigned int id);







