#ifndef _HAPS_H_
#define _HAPS_H_

#include <stdio.h>
#include <stdlib.h>

#include <math.h>
#include <string.h>
#include <errno.h>
#include <time.h>
//#include <set>

//#include "umrbus.h"
#define HAVE_STRUCT_TIMESPEC
//#include "xactors.h"

/* Function for linux build */
typedef unsigned long       ULONG;
typedef unsigned long long  ULONGLONG;
typedef long long           LONGLONG;
typedef unsigned int        UINT32;

typedef union _LARGE_INTEGER {
    struct {
        double LowPart;
        long HighPart;
    } DUMMYSTRUCTNAME;
    struct {
        double LowPart;
        long HighPart;
    } u;
    long long QuadPart;
} LARGE_INTEGER;

#ifdef __cplusplus
extern "C" {
#endif
    
#define XTOR_M1 "AXI4"
#define XTOR_M1_AWIDTH 64
#define XTOR_M1_DWIDTH 128
#define XTOR_M1_BSIZE e_BITS128

#define XTOR_M3 "AXI4"
#define XTOR_M3_AWIDTH 32
#define XTOR_M3_DWIDTH 32
#define XTOR_M3_BSIZE e_BITS32

#define MAX_BURST_LENGTH_M1  128*1024 /* MAX 128k */
#define MAX_BURST_LENGTH_M2  128*1024 /* MAX 128k */
#define MAX_BURST_LENGTH_M3  128*1024 /* MAX 128k */
#define MAX_BURST_LENGTH     4096
#define MAX_BURST_LENGTH_SIM 256
#define ERROR_OUTPUT_RANGE   1024

  /* all capims used by this program */
#define MASTER_ID1        1
#define MASTER_ID1_TYPE   0x200E // 0x200C
#define MASTER_ID2        3
#define MASTER_ID2_TYPE   0x200E
#define MIG_STATUS_C      7
#define MIG_STATUS_C_TYPE 0x8001

/* APB & AXI setting */
#define SUPPORT_AXI1
#define SUPPORT_APB

#define ID_NUM_AXI1 1 //DDR 1 = MASTER_ID1
#define ID_NUM_APB3 3 //APB   = APB_ID3

int XtorSend_32(int mid, unsigned long address, unsigned char *ptSrcData, long Size);
int XtorRead_32(int mid, unsigned long address, unsigned char *ptDstData, long Size);
int XtorSend_128(int mid, unsigned long address, unsigned char *ptSrcData, long Size);
int XtorRead_128(int mid, unsigned long address, unsigned char *ptDstData, long Size);

void *haps_init(unsigned long core_idx, unsigned long dram_base, unsigned long dram_size);
void haps_release(unsigned long core_idx);
int haps_write_register(unsigned long core_idx, void * base, unsigned int addr, unsigned int data);
unsigned int haps_read_register(unsigned long core_idx, void * base, unsigned int addr);
int haps_write_memory(unsigned long core_idx,void * base, unsigned int addr, unsigned char *data, int len, int endian);
int haps_read_memory(unsigned long core_idx, void * base, unsigned int addr, unsigned char *data, int len, int endian);
int haps_hw_reset(unsigned long core_idx, void* base);
int haps_vpu_reset(unsigned long core_idx, void* base);

int haps_io_lock(int* dev_io_lock);
void haps_io_unlock(int* dev_io_lock);
int haps_vpu_lock(int* dev_vpu_lock);
void haps_vpu_unlock(int* dev_vpu_lock);
int haps_vmem_lock(int* dev_vmem_lock);
void haps_vmem_unlock(int* dev_vmem_lock);
int haps_vpu_disp_lock(int* dev_vpu_disp_lock);
void haps_vpu_disp_unlock(int* dev_vpu_disp_lock);
int haps_reset_lock(int* dev_vpu_reset_lock);
void haps_reset_unlock(int* dev_vpu_reset_lock);

int haps_set_timing_opt(unsigned long core_idx, void * base);
int haps_ics307m_set_clock_freg(void * base, int Device, int OutFreqMHz);
void haps_write_reg_timing(unsigned long addr, unsigned int data);
int haps_set_clock_freg( void *base, int Device, int OutFreqMHz);
long long haps_get_phys_base_addr(void);

#ifdef __cplusplus
}
#endif

#endif //#ifndef _HAPS_H_

