/******************************************************************************
   Copyright (C) 2013-2020 Synopsys, Inc.
   This software and the associated documentation are confidential and
   proprietary to Synopsys, Inc. Your use or disclosure of this software is
   subject to the terms and conditions of a written license agreement
   between you, or your company, and Synopsys, Inc.
   *******************************************************************************
   Project    : memory
   *******************************************************************************
   DESCRIPTION: memory test application
   *******************************************************************************
           $Id: //chipit/chipit/main/release/systems/haps_db/ddr3_sodimm2r_ht3/check/xtor_mig_ddr3/app/src/xtor_memory_test.cpp#1 $
       $Author: mlange $
     $DateTime: 2021/02/05 09:22:10 $
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#ifdef UNIX
#include <sys/time.h>
#include <unistd.h>
#endif
#ifdef WINDOWS
#pragma warning( disable: 4251 )
//#include <windows.h>
#include <sys/timeb.h>
#endif

#include <math.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <set>

#include "umrbus.h"
#define HAVE_STRUCT_TIMESPEC
#include "xactors.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define DEBUG
//#define AXI4_M2
#ifdef AXI4_M2

#define XTOR_M1 "AXI"
#define XTOR_M1_AWIDTH 64
#define XTOR_M1_DWIDTH 32
#define XTOR_M1_BSIZE e_BITS32

#else

#define XTOR_M1 "AXI4"
#define XTOR_M1_AWIDTH 64
#define XTOR_M1_DWIDTH 128
#define XTOR_M1_BSIZE e_BITS128

#endif

#define XTOR_M2 "AXI4"
#define XTOR_M2_AWIDTH 32
#define XTOR_M2_DWIDTH 128
//#define XTOR_M2_BSIZE e_BITS256
#define XTOR_M2_BSIZE e_BITS128

#define XTOR_M3 "AXI4"
#define XTOR_M3_AWIDTH 32
#define XTOR_M3_DWIDTH 32
#define XTOR_M3_BSIZE e_BITS32

//#define MAX_BURST_LENGTH    128     /* 4K boundary */
#define MAX_BURST_LENGTH_M1  128*1024 /* MAX 128k */
#define MAX_BURST_LENGTH_M2  128*1024 /* MAX 128k */
#define MAX_BURST_LENGTH_M3  128*1024 /* MAX 128k */
#define MAX_BURST_LENGTH_SIM 256
#define ERROR_OUTPUT_RANGE   1024

  /* all capims used by this program */
#define MASTER_ID1        1
#define MASTER_ID1_TYPE   0x200E // 0x200C
#define MASTER_ID2        2
#define MASTER_ID2_TYPE   0x200E
#define MASTER_ID3        3
#define MASTER_ID3_TYPE   0x200E
#define MIG_STATUS_C      7
#define MIG_STATUS_C_TYPE 0x8001
  
  /* global variables and switches */
  unsigned int umr_dev = 0;
  unsigned int umr_bus = 1;
  int umr_cap = 1;
  int use_axi = 1;
  int umr_sim = 0;

  unsigned int umr_dev_default = umr_dev ;
  int umr_dev_fixed   = 0 ;
  unsigned int umr_bus_default = umr_bus ;
  int umr_bus_fixed   = 0 ;
  int umr_cap_fixed   = 0 ;

#define MODE_READ               1
#define MODE_WRITE              2
#define MODE_WRITE_READ_COMPARE 3
#define MODE_READ_COMPARE       4
#define MODE_WRITE_FROM_FILE    5
  int mode = MODE_WRITE_READ_COMPARE;

  char * filename = NULL;

#define DATAMODE_RANDOM        1
#define DATAMODE_NIBBLEPATTERN 2
#define DATAMODE_STRESS_00FF   3
#define DATAMODE_STRESS_55AA   4
#define DATAMODE_RANDOM_FAST   5
#ifdef WINDOWS
#define strtoll _strtoi64
#endif

  int  retval          = 1;
  char regen           = 1;
  char mig_capim_check = 1;

  int datamode  = DATAMODE_RANDOM;

  int max_burst = MAX_BURST_LENGTH_M2;

  long long offset         = 0;
  long long offset_default = offset;
  long long p_offset       = 0;
  long long p_length       = 0;

  long long length         = 268435456; // 256 MByte data
  long long length_default = length;

  int delay = -1;
  int percentage = 1;
  long long quick = 0;

  char * errormessage;

  void usage (char * program)
  {
    printf ("usage: %s [options]\n\n", program);
    printf ("\t-d <n>            UMRBus device number (default is %i)\n", umr_dev_default);
    printf ("\t-b <n>            UMRBus bus number (default is %i)\n", umr_bus_default);
    printf ("\t-c <n>            UMRBus CAPIM address of MIG Status CAPIM (default is auto scan)\n");
    printf ("\t-x <id>           transactor ID 1 (%s %i/%ibit) or ID 2 (%s %i/%ibit) (default is 1)\n", XTOR_M1, XTOR_M1_DWIDTH, XTOR_M1_AWIDTH, XTOR_M2, XTOR_M2_DWIDTH, XTOR_M2_AWIDTH);
    printf ("\t-m <mode>         access mode (default is write_read_compare)\n");
    printf ("\t     write_read_compare  write, read and compare generated data\n");
    printf ("\t     read                read data from memory and write into given file (option -f required)\n");
    printf ("\t     write               generated data pattern, write into memory and given file (option -f required)\n");
    printf ("\t     write_from_file     read data from given file and write into memory (option -f required) \n");
    printf ("\t     read_compare        read data from memory and compare to data from given file (option -f required)\n");
    printf ("\t-f <filename>     binary file (required by some modi)\n");
    printf ("\t-g <mode>         mode how data is generated (random is default)\n");
    printf ("\t     random              randomized data will be generated\n");
    printf ("\t     00ff                data like 00, FF, 00, FF, ... will be generated\n");
    printf ("\t     55aa                data like 55, AA, 55, AA, ... will be generated\n");
    printf ("\t     nibblepattern       data like 00, 11, 22, 33, ... will be generated\n");
    printf ("\t-o <addr>         byte-address offset for memory access (default is %lld)\n", offset_default);
    printf ("\t-n <capim>        Check MIG Status CAPIM (default is 1)\n");
    printf ("\t-p <0|1>          output only, show percentage feedback after each burst (default is 1)\n");
    if ( umr_sim == 1 )
      max_burst = MAX_BURST_LENGTH_SIM;
    else if ( use_axi == 2 )
      max_burst = MAX_BURST_LENGTH_M2;
    else
      max_burst = MAX_BURST_LENGTH_M1;
    printf ("\t-q <0|1>          test mode: use blocks of %i Bytes only with fix address increment (default is 0)\n", max_burst);
    printf ("\t-l <n>            number of bytes that will be processed (default is %lld = %lld MByte)\n", length_default, length_default / 1024 / 1024);
    printf ("\t-s <n>            add delay (in seconds) between UMRBus accesses (optional for simulation)\n");
  }

#ifdef WINDOWS
#define SLEEP(iValue) Sleep (iValue*1000)
#else
#define SLEEP(iValue) sleep (iValue)
#endif

#ifdef UNIX
  double GetTime(void)
  {
    struct timeval s_timeval;
    struct timezone s_timezone;
    double TimeValue;
    
    gettimeofday(&s_timeval, &s_timezone);
    
    TimeValue = ((double)(s_timeval.tv_sec)) + (((double)(s_timeval.tv_usec))/1000000.0);
    
    return TimeValue;
  }
#endif

#ifdef WINDOWS
  double GetTime(void)
  {
    struct _timeb s_timeb;
    double TimeValue;
    
    _ftime(&s_timeb);
    
    TimeValue=((double)(s_timeb.time)) + (((double)(s_timeb.millitm))/1000.0);
    
    return TimeValue;
  }
#endif
  
  double StartTime, StopTime;
  
  double dBytesPerSecond, dMBytesPerSecond;
  double elapsedTime = 0.0;

  MasterXactor<XTOR_M1_AWIDTH,XTOR_M1_DWIDTH> *m1 = NULL;
  MasterXactor<XTOR_M2_AWIDTH,XTOR_M2_DWIDTH> *m2 = NULL;
  MasterXactor<XTOR_M3_AWIDTH,XTOR_M3_DWIDTH> *m3 = NULL;

  /* UMRBus functions error message variable */ 
  UMR_HANDLE MIG_STATUS_C_HANDLE  = UMR_INVALID_HANDLE;
  UMRBUS_LONG mig_status;
  
  void CloseCapim(UMR_HANDLE handle)
  {
    if (handle != UMR_INVALID_HANDLE) {
      if ((umrbus_close(handle, &errormessage))) {
        printf("\nError : Close connection to CAPIM : %s\n", errormessage);
        umrbus_errormessage_free(&errormessage);
      }    
    }
  }
  
  void MIGStatusSet(int iValue)
  {
    if ( mig_capim_check ) {
      if (MIG_STATUS_C_HANDLE != UMR_INVALID_HANDLE) {
        mig_status = iValue;
        if ((umrbus_write(MIG_STATUS_C_HANDLE, &mig_status, (UMRBUS_LONG) 1, &errormessage))) { 
          printf("\nError : Could not write to mig_status CAPIM : %s\n\n", errormessage);
          CloseCapim(MIG_STATUS_C_HANDLE);
        }
      } else {
        printf("\nError : No open handle for mig_status CAPIM\n");
      }
    }
  }

  void MIGStatusGet()
  {
    if ( mig_capim_check ) {
      if (MIG_STATUS_C_HANDLE != UMR_INVALID_HANDLE) {
        if ((umrbus_read(MIG_STATUS_C_HANDLE, &mig_status, (UMRBUS_LONG) 1, &errormessage))) { 
          printf("\nError : Could not read from mig_status CAPIM : %s\n\n", errormessage);
          umrbus_errormessage_free(&errormessage);
          CloseCapim(MIG_STATUS_C_HANDLE);
        }
      } else {
        printf("\nError : No open handle for mig_status CAPIM\n");
      }
    }
  }

  int XtorSend_M1(MasterXactor<XTOR_M1_AWIDTH,XTOR_M1_DWIDTH> *m, long long address, unsigned char *ptSrcData, long long Size)
  {
    TLMResponse<XTOR_M1_DWIDTH>               rsp;
    TLMRequest<XTOR_M1_AWIDTH,XTOR_M1_DWIDTH> req;
    req.m_header.m_command    = TLMCommand::e_WRITE;
    req.m_header.m_b_size     = TLMBSize::XTOR_M1_BSIZE;
    req.m_header.m_burst_mode = TLMBurstMode::e_INCR;
    //req.m_header.m_spec_byte_enable = true;
    req.m_b_length            = unsigned ((address % (XTOR_M1_DWIDTH / 8) + Size + (XTOR_M1_DWIDTH / 8 - 1)) / (XTOR_M1_DWIDTH / 8));
    req.m_address             = address;

    //printf ("@D: size = %li m_b_length = %li m_address = %li\n", (long int) Size, (long int) req.m_b_length, (long int) req.m_address);

    req.setPayload();

    req.fillPayload((char*) ptSrcData, (size_t) Size * sizeof(char));
            
    m->send(req, rsp);
    if (rsp.m_header.m_status != TLMStatus::e_SUCCESS) {
      printf("@Error: %s = %s\n", rsp.m_header.m_error.getClassName(), rsp.m_header.m_error.getName());
      return 1;
    }

    //printf ("@D: 3\n");

    if (delay != -1) SLEEP (delay);

    return 0;        
  }

  int XtorSend_M2(MasterXactor<XTOR_M2_AWIDTH,XTOR_M2_DWIDTH> *m, long long address, unsigned char *ptSrcData, long long Size)
  {
    TLMResponse<XTOR_M2_DWIDTH>               rsp;
    TLMRequest<XTOR_M2_AWIDTH,XTOR_M2_DWIDTH> req;
    req.m_header.m_command    = TLMCommand::e_WRITE;
    req.m_header.m_b_size     = TLMBSize::XTOR_M2_BSIZE;
    req.m_header.m_burst_mode = TLMBurstMode::e_INCR;
    //req.m_header.m_spec_byte_enable = true;
    req.m_b_length            = unsigned ((address % (XTOR_M2_DWIDTH / 8) + Size + (XTOR_M2_DWIDTH / 8 - 1)) / (XTOR_M2_DWIDTH / 8)); // minimum is one full 8Byte (64Bit) access
    req.m_address             = address; // if address isn't multiple of 8 the lower bytes of one full 8Byte access will be masked by library

    //printf ("@D: m_b_length = %i m_address = %i\n", (int) req.m_b_length, (int) req.m_address);

    req.setPayload();

    //req.fillPayload((char*) ptSrcData, Size * sizeof(char), (const char*) ptSrcByteEn, Size * sizeof(char));
    req.fillPayload((char*) ptSrcData, (size_t) Size * sizeof(char)); // if payload size is lower than 8Bytes all remaining upper bytes of one full 8Byte access will be masked by library
            
    m->send(req, rsp);
    if (rsp.m_header.m_status != TLMStatus::e_SUCCESS) {
      printf("@Error: %s = %s\n", rsp.m_header.m_error.getClassName(), rsp.m_header.m_error.getName());
      return 1;
    }

    if (delay != -1) SLEEP (delay);

    return 0;        
  }

  int XtorSend_M3(MasterXactor<XTOR_M3_AWIDTH,XTOR_M3_DWIDTH> *m, long long address, unsigned char *ptSrcData, long long Size)
  {
    TLMResponse<XTOR_M3_DWIDTH>               rsp;
    TLMRequest<XTOR_M3_AWIDTH,XTOR_M3_DWIDTH> req;
    req.m_header.m_command    = TLMCommand::e_WRITE;
    req.m_header.m_b_size     = TLMBSize::XTOR_M3_BSIZE;
    req.m_header.m_burst_mode = TLMBurstMode::e_INCR;
    //req.m_header.m_spec_byte_enable = true;
    req.m_b_length            = unsigned ((address % (XTOR_M3_DWIDTH / 8) + Size + (XTOR_M3_DWIDTH / 8 - 1)) / (XTOR_M3_DWIDTH / 8)); // minimum is one full 8Byte (64Bit) access
    req.m_address             = address; // if address isn't multiple of 8 the lower bytes of one full 8Byte access will be masked by library

    //printf ("@D: m_b_length = %i m_address = %i\n", (int) req.m_b_length, (int) req.m_address);

    req.setPayload();

    //req.fillPayload((char*) ptSrcData, Size * sizeof(char), (const char*) ptSrcByteEn, Size * sizeof(char));
    req.fillPayload((char*) ptSrcData, (size_t) Size * sizeof(char)); // if payload size is lower than 8Bytes all remaining upper bytes of one full 8Byte access will be masked by library
            
    m->send(req, rsp);
    if (rsp.m_header.m_status != TLMStatus::e_SUCCESS) {
      printf("@Error: %s = %s\n", rsp.m_header.m_error.getClassName(), rsp.m_header.m_error.getName());
      return 1;
    }

    if (delay != -1) SLEEP (delay);

    return 0;        
  }
  int XtorRead_M1(MasterXactor<XTOR_M1_AWIDTH,XTOR_M1_DWIDTH> *m, long long address, unsigned char *ptDstData, long long Size)
  {
    TLMResponse<XTOR_M1_DWIDTH>               rsp;
    TLMRequest<XTOR_M1_AWIDTH,XTOR_M1_DWIDTH> req;
    req.m_header.m_command    = TLMCommand::e_READ;
    req.m_header.m_b_size     = TLMBSize::XTOR_M1_BSIZE;
    req.m_header.m_burst_mode = TLMBurstMode::e_INCR;
    req.m_b_length            = unsigned ((address % (XTOR_M1_DWIDTH / 8) + Size + (XTOR_M1_DWIDTH / 8 - 1)) / (XTOR_M1_DWIDTH / 8));
    req.m_address             = address;

    //printf ("@D: m_b_length = 0x%x m_address = 0x%llx\n", req.m_b_length, (long long) req.m_address);

    m->send(req, rsp);
    //printf ("@D: done\n");
    if (rsp.m_header.m_status != TLMStatus::e_SUCCESS) {
      printf("@Error: %s = %s\n", rsp.m_header.m_error.getClassName(), rsp.m_header.m_error.getName());
      return 1;
    }

    rsp.fillMemory(address, (char*) ptDstData, (size_t) Size * sizeof(char));

    if (delay != -1) SLEEP (delay);
    return 0;        
  }

  int XtorRead_M2(MasterXactor<XTOR_M2_AWIDTH,XTOR_M2_DWIDTH> *m, long long address, unsigned char *ptDstData, long long Size)
  {
    TLMResponse<XTOR_M2_DWIDTH>               rsp;
    TLMRequest<XTOR_M2_AWIDTH,XTOR_M2_DWIDTH> req;
    req.m_header.m_command    = TLMCommand::e_READ;
    req.m_header.m_b_size     = TLMBSize::XTOR_M2_BSIZE;
    req.m_header.m_burst_mode = TLMBurstMode::e_INCR;
    req.m_b_length            = unsigned ((address % (XTOR_M2_DWIDTH / 8) + Size + (XTOR_M2_DWIDTH / 8 - 1)) / (XTOR_M2_DWIDTH / 8));
    req.m_address             = address;

    //printf ("@D: m_b_length = 0x%x m_address = 0x%llx\n", req.m_b_length, (long long) req.m_address);

    m->send(req, rsp);
    if (rsp.m_header.m_status != TLMStatus::e_SUCCESS) {
      printf("@Error: %s = %s\n", rsp.m_header.m_error.getClassName(), rsp.m_header.m_error.getName());
      return 1;
    }

    rsp.fillMemory(address, (char*) ptDstData, (size_t) Size * sizeof(char));

    if (delay != -1) SLEEP (delay);

    return 0;     
  }

  int XtorRead_M3(MasterXactor<XTOR_M3_AWIDTH,XTOR_M3_DWIDTH> *m, long long address, unsigned char *ptDstData, long long Size)
  {
    TLMResponse<XTOR_M3_DWIDTH>               rsp;
    TLMRequest<XTOR_M3_AWIDTH,XTOR_M3_DWIDTH> req;
    req.m_header.m_command    = TLMCommand::e_READ;
    req.m_header.m_b_size     = TLMBSize::XTOR_M3_BSIZE;
    req.m_header.m_burst_mode = TLMBurstMode::e_INCR;
    req.m_b_length            = unsigned ((address % (XTOR_M3_DWIDTH / 8) + Size + (XTOR_M3_DWIDTH / 8 - 1)) / (XTOR_M3_DWIDTH / 8));
    req.m_address             = address;

    //printf ("@D: m_b_length = 0x%x m_address = 0x%llx\n", req.m_b_length, (long long) req.m_address);

    m->send(req, rsp);
    if (rsp.m_header.m_status != TLMStatus::e_SUCCESS) {
      printf("@Error: %s = %s\n", rsp.m_header.m_error.getClassName(), rsp.m_header.m_error.getName());
      return 1;
    }

    rsp.fillMemory(address, (char*) ptDstData, (size_t) Size * sizeof(char));

    if (delay != -1) SLEEP (delay);

    return 0;     
  }
  void GenerateData(unsigned char * outbuffer, long long c_length)
  {
    unsigned int value, value2;
    long long li;

    if ( c_length < 16 )
      c_length = 16;
    
    /* there are data to generate? */
    if ((mode == MODE_WRITE) || (mode == MODE_WRITE_READ_COMPARE))
      {
        unsigned int* outbuffer_ip;
        outbuffer_ip = (unsigned int*) outbuffer;            

        if ( regen ) {
          //printf ("OK\n");
          switch (datamode)
            {
            case DATAMODE_RANDOM:
              for (li = 0; li < c_length/4+1; li++)
                //*((unsigned int*)outbuffer+li+0) = rand ();
                *outbuffer_ip++ = rand ();
              break;
            case DATAMODE_RANDOM_FAST:
              for (li = 0; li < c_length/4+1; li+=16)
                {
                  value = rand ();
                  *((unsigned int*)outbuffer+li+0) = value;
                  *((unsigned int*)outbuffer+li+1) = ~value;
                  value2 = rand ();
                  *((unsigned int*)outbuffer+li+2) = value2;
                  *((unsigned int*)outbuffer+li+3) = ~value2;
                  *((unsigned int*)outbuffer+li+4) = value ^ value2;
                  *((unsigned int*)outbuffer+li+5) = value ^ ~value2;
                  *((unsigned int*)outbuffer+li+6) = value | value2;
                  *((unsigned int*)outbuffer+li+7) = value | ~value2;
                  *((unsigned int*)outbuffer+li+8) = ~value | value2;
                  *((unsigned int*)outbuffer+li+9) = ~value | ~value2;
                  *((unsigned int*)outbuffer+li+10) = value & value2;
                  *((unsigned int*)outbuffer+li+11) = value & ~value2;
                  *((unsigned int*)outbuffer+li+12) = ~value & value2;
                  *((unsigned int*)outbuffer+li+13) = ~value & ~value2;
                  *((unsigned int*)outbuffer+li+14) = ~value ^ value2;
                  *((unsigned int*)outbuffer+li+15) = ~value ^ ~value2;
                  
                  //if ( li == 0 ) printf ("%s %d: 0x%x %d\n", __FUNCTION__, __LINE__, value, sizeof(unsigned int));
                }
              break;
            case DATAMODE_NIBBLEPATTERN:
              for (li = 0; li < c_length/4+1; li+=4)
                {
                  *((unsigned int*)outbuffer+li+0) = 0x33221100;
                  *((unsigned int*)outbuffer+li+1) = 0x77665544;
                  *((unsigned int*)outbuffer+li+2) = 0xBBAA9988;
                  *((unsigned int*)outbuffer+li+3) = 0xFFEEDDCC;
                }
              regen=0;
              break;
            case DATAMODE_STRESS_00FF:
              for (li = 0; li < c_length/4+1; li+=4)
                {
                  *((unsigned int*)outbuffer+li+0) = 0xFF00FF00;
                  *((unsigned int*)outbuffer+li+1) = 0xFF00FF00;
                  *((unsigned int*)outbuffer+li+2) = 0xFF00FF00;
                  *((unsigned int*)outbuffer+li+3) = 0xFF00FF00;
                }
              regen=0;
              break;
            case DATAMODE_STRESS_55AA:
              for (li = 0; li < c_length/4+1; li+=4)
                {
                  *((unsigned int*)outbuffer+li+0) = 0xAA55AA55;
                  *((unsigned int*)outbuffer+li+1) = 0xAA55AA55;
                  *((unsigned int*)outbuffer+li+2) = 0xAA55AA55;
                  *((unsigned int*)outbuffer+li+3) = 0xAA55AA55;
                }
              regen=0;
            }
        }
      }    
  }

  void CompareData(unsigned char * outbuffer, unsigned char * inbuffer, long long c_length)
  {
    UMRBUS_LONG error_word_count = 0;
    UMRBUS_LONG error_bit_count = 0;
    UMRBUS_LONG ulBitErrors[32];
    UMRBUS_LONG ulPreviousBitErrors[32]; 
    unsigned int ui, i;
    UMRBUS_LONG li, start, lli, error_pattern, previous_error_pattern;
    UMRBUS_LONG ulOddErrors = 0;
    UMRBUS_LONG ulEvenErrors = 0;

    for (ui = 0; ui < 32; ui ++)
      {
        ulBitErrors[ui] = 0;
        ulPreviousBitErrors[ui] = 0;
      }
        
    for (li = 0; li < c_length; li ++)
      {
        if (* (outbuffer + li) != * (inbuffer + li))
          {
            printf ("\nFirst data error at address 0x%09lx\n\n", (unsigned long) (li + p_offset));
            printf ("address      expected values                      received values                    errors\n");
            printf ("-------------------------------------------------------------------------------------------\n");
            lli = start = (li < (ERROR_OUTPUT_RANGE >> 1) ? 0 : li - (ERROR_OUTPUT_RANGE >> 1)) & 0xFFFFFFF0;
            while (lli < start + ERROR_OUTPUT_RANGE)
              {
                printf ("0x%09llx: ", (lli + p_offset));
                for (i = 0; i < 16; i += 4)
                  if (lli + i < c_length) 
                    printf ("%02x%02x%02x%02x ", * (outbuffer + lli+i+3), * (outbuffer + lli+i+2), * (outbuffer + lli+i+1), * (outbuffer + lli+i));
                  else
                    printf ("         ");
                printf (" ");
                for (i = 0; i < 16; i += 4)
                  if (lli + i < c_length) 
                    printf ("%02x%02x%02x%02x ", * (inbuffer + lli+i+3), * (inbuffer + lli+i+2), * (inbuffer + lli+i+1), * (inbuffer + lli+i));
                  else
                    printf ("         ");
                printf (" ");
                for (i = 0; i < 16; i += 4)
                  if (lli + i < c_length) 
                    {
                      if ((* (outbuffer + lli+i+3) != * (inbuffer + lli+i+3)) | (* (outbuffer + lli+i+2) != * (inbuffer + lli+i+2)) | (* (outbuffer + lli+i+1) != * (inbuffer + lli+i+1)) | (* (outbuffer + lli+i) != * (inbuffer + lli+i)))
                        printf ("#");
                      else
                        printf ("-");
                    }
                  else
                    printf ("         ");
                printf ("\n");
                lli += 16;
                if ((lli & 0x0000003F) == 0)
                  printf ("-------------------------------------------------------------------------------------------\n");
                if (lli >= c_length) break;
              }
          
            error_word_count = error_word_count + 1;
            if (li % 2)
              ulOddErrors ++;
            else
              ulEvenErrors ++;
            
            /* get bit error count */
            error_pattern = *(outbuffer + li) ^ *(inbuffer + li);
            if (li != 0)
              previous_error_pattern = *(outbuffer + li - 1) ^ *(inbuffer + li);
            else
              previous_error_pattern = 0;
            for (i = 0; i < 32; i++) {
              if ((error_pattern & (1 << i)) != 0)
                {
                  error_bit_count = error_bit_count + 1;
                  ulBitErrors[i] ++;
                  if ((previous_error_pattern & (1 << i)) != 0)
                    ulPreviousBitErrors[i] ++;
                }
            }
            break;
          }
        else {
            if ((li % 1024)==0) printf("Address : %9llx, WR : %x, RD :%x, Matched \n",(long long)(li + p_offset ), *(outbuffer+li), *(inbuffer+li));
        }
      }

    if (error_word_count != 0) {
      //printf ("\nBit error statistic:\n");
      //for (ui = 0; ui < 32 ; ui ++)
      //  {
      //    printf ("  Bit # %2i %10lu errors %10lu errors compared to the previous value\n",
      //            ui, (unsigned long) ulBitErrors[ui], (unsigned long) ulPreviousBitErrors[ui]);
      //  }
      //printf ("\nComparing data finished with error(s).\n");
      //printf ("  Number of even data word errors : %lu\n", (unsigned long) ulEvenErrors);
      //printf ("  Number of odd  data word errors : %lu\n", (unsigned long) ulOddErrors);
      //printf ("  Number of data word errors      : %lu\n", (unsigned long) error_word_count);
      //printf ("  Number of data bit errors       : %lu\n", (unsigned long) error_bit_count);
      
      printf ("\nERROR: Test Failed.\n\n");
      retval=1;
    } else {
      retval=0;
    }
  }


  /* main routine */
  int main (int argc, char * argv[])
  {
    XactorLog::SetOutputLevel(XACTOR_MSG_ERROR); 
//    XactorLog::SetOutputLevel(XACTOR_MSG_DEBUG10);

    int i = 1;
    long long s_length;
    double pi, ppi;
    unsigned char * inbuffer = NULL;
    unsigned char * outbuffer = NULL;
    unsigned char * inbuffer_charles = NULL;
    unsigned char * outbuffer_charles = NULL;
    unsigned int global_rand_start;
    FILE * file = NULL;
    BitT<32u> gpi_data = 0;

    umrbus_scan_type **umrbusscan;
    std::set<int> umr_dev_scanlist;
    std::set<int>::iterator it;
    int umr_scan = 0;
    int umr_scan_check = 3;
    int mid1, mid2, mid3, cid1;

    mid1 = MASTER_ID1;
    mid2 = MASTER_ID2;
    mid3 = MASTER_ID3;
    cid1 = MIG_STATUS_C;
    
    fflush(stdout);

    /* check for NO_PERCENT variable */
    char * val = std::getenv("NO_PERCENT");
    if (val) percentage = 0;

    i = 1;
    /* parse command line arguments */
    while (i < argc)
      {
        if (i == argc-1) goto usage_exit;
        if (*argv[i] != '-') goto usage_exit;
        if (*(argv[i]+2) != 0) goto usage_exit;
        switch (*(argv[i]+1))
          {
          case 'd': umr_dev = strtoul (argv[i+1], NULL, 0); umr_dev_fixed = 1; break;
          case 'b': umr_bus = strtoul (argv[i+1], NULL, 0); umr_bus_fixed = 1; break;
          case 'c': umr_cap = strtoul (argv[i+1], NULL, 0); umr_cap_fixed = 1; break;
          case 'a': use_axi = strtoul (argv[i+1], NULL, 0); break;
          case 'x': use_axi = strtoul (argv[i+1], NULL, 0); break;
          case 'i': umr_scan = strtoul (argv[i+1], NULL, 0); break;
          case 'm': 
            if (strcmp (argv[i+1], "read") == 0) mode = MODE_READ;
            else if (strcmp (argv[i+1], "write") == 0) mode = MODE_WRITE;
            else if (strcmp (argv[i+1], "write_from_file") == 0) mode = MODE_WRITE_FROM_FILE;
            else if (strcmp (argv[i+1], "write_read_compare") == 0) mode = MODE_WRITE_READ_COMPARE;
            else if (strcmp (argv[i+1], "read_compare") == 0) mode = MODE_READ_COMPARE;
            else goto usage_exit;
            break;
          case 'f': filename = argv[i+1]; break;
          case 'g': 
            if (strcmp (argv[i+1], "random") == 0) datamode = DATAMODE_RANDOM;
            else if (strcmp (argv[i+1], "nibblepattern") == 0) datamode = DATAMODE_NIBBLEPATTERN;
            else if (strcmp (argv[i+1], "00ff") == 0) datamode = DATAMODE_STRESS_00FF;
            else if (strcmp (argv[i+1], "55aa") == 0) datamode = DATAMODE_STRESS_55AA;
            else if (strcmp (argv[i+1], "random_fast") == 0) datamode = DATAMODE_RANDOM_FAST;
           else goto usage_exit;
            break;
          case 'o': offset = strtoll (argv[i+1], NULL, 0); break;
          case 'n': mig_capim_check = strtoll (argv[i+1], NULL, 0); break;
          case 'l': length = strtoll (argv[i+1], NULL, 0); break;
          case 'p': percentage = atoi (argv[i+1]); break;
          case 'q': quick = strtoll (argv[i+1], NULL, 0); break;
          case 's': printf ("Simulation mode on!\n");
            umr_sim = 1 ; delay = atoi (argv[i+1]); break;
          }
        i += 2;
      }

    // use umrscan to identify used device and capim addresses
    if ( umr_scan == 0 ) {
      /* scan for all CAPIMs / Transactors */
      if((umrbusscan = umrbus_scan(&errormessage)) == NULL) {
        printf("\nError : UMRBus scan : %s\n", errormessage);
        umrbus_errormessage_free(&errormessage);
        goto exit;
      }
      
      //printf("\nDebug : %i %i %i\n", mid1, mid2, cid1);
      i=0;
      while(umrbusscan[i] != NULL) {
        if ( umr_dev_fixed && umrbusscan[i]->device != umr_dev)  { i++; continue; }
        if ( umr_bus_fixed && umrbusscan[i]->bus != umr_bus)     { i++; continue; }
        
        umr_dev_scanlist.insert(umrbusscan[i]->device);
        if (umrbusscan[i]->type == MASTER_ID1_TYPE && umr_scan == 0) {
          mid1 = umrbusscan[i]->address;
          umr_scan++;
        } else if (umrbusscan[i]->type == MASTER_ID2_TYPE) {
          mid2 = umrbusscan[i]->address;
          umr_scan++;
        } else if (umrbusscan[i]->type == MASTER_ID3_TYPE) {
          mid3 = umrbusscan[i]->address;
          umr_scan++;
        }

        printf("[DEBUG] i : %d, umr_bus type : %x\n", i, umrbusscan[i]->type);

        if ( mig_capim_check ) {
#ifdef AXI4_M2
          umr_scan_check=3;
#else
          umr_scan_check=3;
#endif
          if (umrbusscan[i]->type == MIG_STATUS_C_TYPE) {
            cid1 = umrbusscan[i]->address;
            umr_scan++;
          }
        } else {
          umr_scan_check=1;
        }
        if (umr_scan == umr_scan_check) {
          //if ( umr_dev != umrbusscan[i]->device ) {
          //  printf("Use UMRBus device <%d> instead of default <%d>\n", umrbusscan[i]->device, umr_dev_default);
          //}
          umr_dev = umrbusscan[i]->device;
          //if ( umr_bus != umrbusscan[i]->bus ) {
          //  printf("Use UMRBus bus <%d> instead of default <%d>\n", umrbusscan[i]->bus, umr_bus_default);
          //}
          umr_bus = umrbusscan[i]->bus;
          if ( mig_capim_check ) {
            if ( umr_cap_fixed && cid1 != umr_cap) { i++; umr_scan=0; continue; }
            printf("Use UMRBus device<%d> bus<%d> MIG status capim address<%d>\n", umr_dev, umr_bus, cid1);
          }
          break;
        }
        i++;
      }
      
      /* free memory of the scan results */
      umrbus_scan_free(umrbusscan);
      
      if (umr_scan < umr_scan_check) {
        if (umr_dev_scanlist.size()) {
          printf("\nError : Not all CAPIMs/Transactors found on device(s)");
          for (it = umr_dev_scanlist.begin(); it != umr_dev_scanlist.end(); ++it) {
            printf(" <%d>", *it);
          }
          printf(". Please double check HAPS configuration or used umr_device <%d>.\n", umr_dev);
          goto exit;
        } else {
          printf("No UMRBus device found. Please double check HAPS hardware setup umr_scan : %d, umr_scan_check : %d .\n", umr_scan, umr_scan_check);
          goto exit;
        }
      }
    }
//printf("No UMRBus device found. Please double check HAPS hardware setup umr_scan : %d, umr_scan_check : %d .\n", umr_scan, umr_scan_check);

    { 
// charles register write test start
	m3 = new MasterXactor<XTOR_M3_AWIDTH,XTOR_M3_DWIDTH>("master", umr_dev, umr_bus, mid2);
	if (!m3->isOpen()) {
		printf("@WARNING: Cannot open transactor 'm3'\n");
	}
  
	if ((inbuffer_charles = (unsigned char *) malloc (sizeof (char) * (max_burst + 8))) == NULL)	{
		printf ("  Error: Out of memory\n");
		goto exit;
	}
	if ((outbuffer_charles = (unsigned char *) malloc (sizeof (char) * (max_burst + 8))) == NULL) {
		printf ("  Error: Out of memory\n");
		goto exit;
	}

    unsigned int temp_reg_data;
    unsigned int temp_read_data;
    
    unsigned int reg_address = 0x0f000010;
    //unsigned int reg_address = 0x05000008;

    for (int i = 0; i<10; i++) {
        temp_reg_data = rand();
        printf("Sending Data... (%8x)\n", temp_reg_data);
	    XtorSend_M3(m3, reg_address, (unsigned char*)&temp_reg_data, 4);
        
        printf("Readding Data...\n");
	    XtorRead_M3(m3, reg_address, (unsigned char*)&temp_read_data, 4);

        printf("%3d Write : %8x, Read : %8x  ", i, temp_reg_data&0xfffffff0, temp_read_data&0xfffffff0);


        if((temp_reg_data&0xfffffff0) != (temp_read_data&0xfffffff0)) {
            printf("    Not Mached\n");
            break;
        }
        else printf("   Result : Matched\n");
    }

//    printf("Sending Data ... \n");
//    memset(outbuffer_charles, 0xab1f, 200);
//    memset(inbuffer_charles, 0xe0, 200);
//    printf("memset Done ... \n");
//
//    printf("Sending Data : (%x)\n", outbuffer_charles[0]);
//	//XtorSend_M2(m2, 0xf000010, &outbuffer_charles[0], 4);
//	XtorSend_M2(m2, 0xf000010, (unsigned char*)&temp_reg_data, 4);
//
//    printf("Sending Data Done... \n");
//
//	XtorRead_M2(m2, 0xf000010, &inbuffer_charles[0], 4);
//    printf("Reading Data Done... \n");
//    printf("Reading Data : (%x)\n", inbuffer_charles[0]);
//
//	//if (inbuffer_charles) free (inbuffer_charles);
//	//if (outbuffer_charles) free (outbuffer_charles);
//	//if (m2) delete (m2);
//	//

// charels register write test done
 
      /* check for valid mode options */
      if ((mode == MODE_READ) && (filename == NULL)) goto usage_exit;
      if ((mode == MODE_WRITE) && (filename == NULL)) goto usage_exit;
      if ((mode == MODE_WRITE_FROM_FILE) && (filename == NULL)) goto usage_exit;
      if ((mode == MODE_READ_COMPARE) && (filename == NULL)) goto usage_exit;
      
      /* trying to open all xactors */
      if ( use_axi == 1 || use_axi == 2 ) {
        if ( use_axi == 2 ) {
          m2 = new MasterXactor<XTOR_M2_AWIDTH,XTOR_M2_DWIDTH>("master", umr_dev, umr_bus, mid2);
          if (!m2->isOpen()) {
            printf("@WARNING: Cannot open transactor 'm2'\n");
            printf("          Continue with transactor 'm1'\n");
            use_axi = 1;
          }
        }
        if ( use_axi == 1 ) {
          m1 = new MasterXactor<XTOR_M1_AWIDTH,XTOR_M1_DWIDTH>("master", umr_dev, umr_bus, mid1);
          if (!m1->isOpen()) {
            printf ("  Error: Cannot open transactor 'm1'\n");   
            goto exit;
          }
        }
      } else {
        // force xactor address
        m1 = new MasterXactor<XTOR_M1_AWIDTH,XTOR_M1_DWIDTH>("master", umr_dev, umr_bus, use_axi);
        if (!m1->isOpen()) {
          printf ("  Error: Cannot open transactor 'm1'\n");   
          goto exit;
        }
      }

      if ( umr_sim == 1 )
        max_burst = MAX_BURST_LENGTH_SIM;
      else if ( use_axi == 2 )
        max_burst = MAX_BURST_LENGTH_M2;
      else
        max_burst = MAX_BURST_LENGTH_M1;

      fflush(stdout);

      /* open MIG_STATUS CAPIM */
      if ( mig_capim_check ) {
        if ((MIG_STATUS_C_HANDLE=umrbus_open(umr_dev,umr_bus,cid1,&errormessage)) == UMR_INVALID_HANDLE) {
          printf("\nError : Could not open mig_status CAPIM : %s\n\n", errormessage);
          umrbus_errormessage_free(&errormessage);
          return(1);
        }
      }

      /* readback MIG init done */
      MIGStatusGet();
      if ( mig_capim_check ) {
        if ( mig_status & 0x00000001 ) {
          printf("DDR Initialization done.\n");
        } else {
          printf("@ERROR: DDR Initialization not done!\n");
          goto exit;
        }

#ifdef DEBUG
        printf ("@D:          MIG status: 0x%04x\n", (unsigned int) mig_status);
        printf ("@D:            DDR Init: %d\n",     (unsigned int) mig_status & 0x0001);
        printf ("@D:       Clocks Locked: %d\n",     ((unsigned int) mig_status >> 1) & 0x0001);
        printf ("@D:             Reset_N: %d\n",     ((unsigned int) mig_status >> 4) & 0x0001);
        printf ("@D:        IO_Delay_RDY: %d\n",     ((unsigned int) mig_status >> 5) & 0x0001);
        printf ("@D:     Phaser_Ref_Lock: %d\n",     ((unsigned int) mig_status >> 6) & 0x0001);
        for (i = 0; i < 12; i ++) 
          printf ("@D:   Phaser_In_Lock_%02d: %d\n", i,  ((unsigned int) mig_status >> (7 + i)) & 0x0001); 
        for (i = 0; i < 12; i ++) 
          printf ("@D: Phaser_DQS_Found_%02d: %d\n", i,  ((unsigned int) mig_status >> (19 + i)) & 0x0001); 
#endif
      }
      
      /* malloc memory for data inbuffer + one extra word */
      if ((inbuffer = (unsigned char *) malloc ((size_t) (sizeof (char) * (max_burst + 8)))) == NULL)
        {
          printf ("  Error: Out of memory\n");
          goto exit;
        }
      /* malloc memory for data outbuffer + 8 extra words (required for GenerateData) */
      if ((outbuffer = (unsigned char *) malloc ((size_t) (sizeof (char) * (max_burst + 64)))) == NULL)
        {
          printf ("  Error: Out of memory\n");
          goto exit;
        }
  
      if ( quick > 0 ) {
        printf ("Use address increment by: ");
        if (quick < 1024)
          printf ("%ld Byte\n", (long int) quick );
        else if (quick < (1024 * 1024))
          printf ("%ld KByte\n", (long int) (quick / 1024) );
        else if (quick < (1024 * 1024 * 1024))
          printf ("%ld MByte\n", (long int) (quick / 1024 / 1024) );
        else
          printf ("%ld GByte\n", (long int) (quick / 1024 / 1024 / 1024) );\

        s_length = length / quick * max_burst;
      } else {
        s_length = length;
      }
      //GenerateData(outbuffer, length);

      if ( mode == MODE_WRITE || mode == MODE_WRITE_READ_COMPARE )
        switch (datamode)
          {
          case DATAMODE_RANDOM:
            printf ("Generate random data");
            break;
          case DATAMODE_RANDOM_FAST:
            printf ("Generate random (fast) data");
            break;
          case DATAMODE_NIBBLEPATTERN:
            printf ("Generate nibblepattern data");
            break;
          case DATAMODE_STRESS_00FF:
            printf ("Generate \"00ff\" pattern data");
            break;
          case DATAMODE_STRESS_55AA:
            printf ("Generate \"55aa\" pattern data");
          }

      /* there are data to write into the file? */
      if ( mode == MODE_WRITE_READ_COMPARE || mode == MODE_WRITE || mode == MODE_READ )
        {
          if (filename != NULL)          
            if ((file = fopen (filename, "wb")) == NULL)
              {
                printf ("  Error opening file for write: %s\n", strerror (errno));
                goto exit;
              }
          switch (mode)
            {
            case MODE_WRITE:
              printf (", write to memory and to output file \"%s\" ...\n", filename);
              break;
            case MODE_WRITE_READ_COMPARE:
              printf (", write to memory, readback and compare ...\n");
              break;
            case MODE_READ:
              printf ("Read data from memory and write to output file \"%s\" ...\n", filename);
            }
        }
      

      /* there are data to read from file? */
      if ( mode == MODE_READ_COMPARE || mode == MODE_WRITE_FROM_FILE )
        {
          switch (mode)
            {
            case MODE_READ_COMPARE:
              printf ("Read data from memory and compare to input file \"%s\" ...\n", filename);
              break;
            case MODE_WRITE_FROM_FILE:
              printf ("Read data from input file \"%s\" and write to memory ...\n", filename);          
            }
          if ((file = fopen (filename, "rb")) == NULL)
            {
              printf ("  Error opening file for read: %s\n", strerror (errno));
              goto exit;
            }
        }

      /* there are data to write into RAM? */
      if ((mode == MODE_WRITE_READ_COMPARE) || (mode == MODE_WRITE) || (mode == MODE_WRITE_FROM_FILE))
        {
          if ( use_axi == 2 )
            printf ("XTOR %s Master (ID%i): Send data ", XTOR_M2, use_axi);
          else
            printf ("XTOR %s Master (ID%i): Send data ", XTOR_M1, use_axi);
                    
          if (s_length < 1024)
            printf ("(%ld Byte)", (long int) (s_length) );
          else if (s_length < (1024 * 1024))
            printf ("(%ld KByte)", (long int) (s_length / ( 1024 )) );
          else if (s_length < (1024 * 1024 * 1024))
            printf ("(%ld MByte)", (long int) (s_length / ( 1024 * 1024 )) );
          else
            printf ("(%ld GByte)", (long int) (s_length / ( 1024 * 1024 * 1024 )) );
          printf (" to Memory (StartAddress: 0x%09llx) ...\n", offset);
	  
          p_length = length;
          p_offset = offset;

          if (percentage != 0) {
            printf ("\r%.0f%%", (double) (length - p_length) * 100 / length);
            fflush(stdout);
          }

          elapsedTime = 0;

          /* split length into MAX_BURST_LENGTH packages */ 
          pi=0;

          global_rand_start = (unsigned) time (NULL);

          srand (global_rand_start);
          //printf ("p_length + p_offset mod max_burst - %llx + 0x%llx mod %x\n", p_length, p_offset, max_burst);
          while ( ( p_length + p_offset % max_burst ) >= max_burst)
            {
              //printf ("%lld 0x%llx \n", p_offset, p_offset);

              /* there are data to read from file? */
              if (mode == MODE_WRITE_FROM_FILE) {
                if (fread ((void *) (outbuffer), sizeof (char), (size_t) max_burst - (p_offset % max_burst), file) != (size_t) max_burst - (p_offset % max_burst))
                  {
                    printf ("  Error reading file: %s\n", strerror (errno));
                    goto exit;
                  }
              } else {
                GenerateData(outbuffer, max_burst - (p_offset % max_burst));
              }

              MIGStatusSet(1);
              StartTime = GetTime();
              //max_burst=0x10000;
              //printf ("XtorSend offset=%lld length=0x%llx \n", p_offset, max_burst - (p_offset % max_burst));
              if ( use_axi == 2 ) {                
                XtorSend_M2(m2, p_offset, outbuffer, max_burst - (p_offset % max_burst));
              } else {
                XtorSend_M1(m1, p_offset, outbuffer, max_burst - (p_offset % max_burst));
              }
              StopTime = GetTime();
              elapsedTime += (StopTime - StartTime);
              MIGStatusSet(0);

              /* there are data to write into the file? */
              if (filename != NULL && mode != MODE_WRITE_FROM_FILE)
                {
                  //printf ("p_length - %lld \n", max_burst - (p_offset % max_burst));
                  if (fwrite ((void *) (outbuffer), sizeof (char), (size_t) max_burst - (p_offset % max_burst), file) != (size_t) max_burst - (p_offset % max_burst))
                    {
                      printf ("  Error writing file: %s\n", strerror (errno));
                      goto exit;
                    }
                }              

              if ( quick > 0 ) {
                p_length = p_length - quick + p_offset % max_burst;
                p_offset = p_offset + quick - p_offset % max_burst;
              } else {
                p_length = p_length - max_burst + p_offset % max_burst;
                p_offset = p_offset + max_burst - p_offset % max_burst;
              }
                
              if (percentage != 0) {
                ppi=(double) (length - p_length) * 100 / length;
                if ( ppi - pi > 5 ) {
                  printf ("\r%.0f%%", ppi);
                  fflush(stdout);
                  pi=ppi;
                }
              }
            }

          if ( p_length > 0 ) { 
            /* there are data to read from file? */
            if ((mode == MODE_WRITE_FROM_FILE)) {
              if (fread ((void *) (outbuffer), sizeof (char), (size_t) p_length, file) != (size_t) p_length)
                {
                  printf ("  Error reading file: %s\n", strerror (errno));
                  goto exit;
                }
            } else {
              GenerateData(outbuffer, p_length);
            }
          
            //printf ("p_length - %llx \n", p_length);
            //printf ("%lld 0x%llx \n", p_offset, p_offset);
            MIGStatusSet(1);
            StartTime = GetTime();
            //printf ("XtorSend offset=%lld length=0x%lld \n", p_offset, p_length);
            if ( use_axi == 2 ) {
              XtorSend_M2(m2, p_offset, outbuffer, p_length);
            } else {
              XtorSend_M1(m1, p_offset, outbuffer, p_length);
            }
            StopTime = GetTime();
            elapsedTime += (StopTime - StartTime);
            MIGStatusSet(0);

            /* there are data to write into the file? */
            if (filename != NULL && mode != MODE_WRITE_FROM_FILE)
              {
                //printf ("p_length - %lld \n", p_length);
                if (fwrite ((void *) (outbuffer), sizeof (char), (size_t) p_length, file) != (size_t) p_length)
                  {
                    printf ("  Error writing file: %s\n", strerror (errno));
                    goto exit;
                  }
              }              
          }

          dBytesPerSecond=s_length/((double)elapsedTime);
          dMBytesPerSecond=dBytesPerSecond/1024.0/1024.0;
          
          if (delay != -1) { 
            printf("\r100%%  ->  Byte/s: % 4.3f\tWrite time (s): % 4.3f\n",dMBytesPerSecond*1024*1024,elapsedTime);
          } else {
            printf("\r100%%  ->  MByte/s: % 4.3f\tWrite time (s): % 4.3f\n",dMBytesPerSecond,elapsedTime);
          }
          fflush(stdout);          
        }

      /* there are data to read from RAM? */
      if ((mode == MODE_WRITE_READ_COMPARE) || (mode == MODE_READ_COMPARE) || (mode == MODE_READ))
        {             
          if ( use_axi == 2 )
            printf ("XTOR %s Master (ID%i): ", XTOR_M2, use_axi);
          else
            printf ("XTOR %s Master (ID%i): ", XTOR_M1, use_axi);

          if ((mode == MODE_WRITE_READ_COMPARE) || (mode == MODE_READ_COMPARE))
            printf ("Read/Compare data ");
          else
            printf ("Read data ");
                   
          if (s_length < 1024)
            printf ("(%ld Byte)", (long int) (s_length) );
          else if (s_length < (1024 * 1024))
            printf ("(%ld KByte)", (long int) (s_length / ( 1024 )) );
          else if (s_length < (1024 * 1024 * 1024))
            printf ("(%ld MByte)", (long int) (s_length / ( 1024 * 1024 )) );
          else
            printf ("(%ld GByte)", (long int) (s_length / ( 1024 * 1024 * 1024 )) );
          printf (" from Memory (StartAddress: 0x%09llx) ...\n", offset);

          p_length = length;
          p_offset = offset;

          if (percentage != 0) {
            printf ("\r%.0f%%", (double) (length - p_length) * 100 / length);
            fflush(stdout);
          }

          elapsedTime = 0;

          /* split length into MAX_BURST_LENGTH packages */ 
          pi=0;
          srand (global_rand_start);
          //printf ("@D: XtorRead p_length=0x%llx\n", p_length + p_offset % max_burst);
          while ( ( p_length + p_offset % max_burst ) > max_burst)
            {
              MIGStatusSet(1);
              StartTime = GetTime();
              //printf ("@D: XtorRead length=0x%llx offset=0x%llx \n", max_burst - p_offset % max_burst, p_offset);
              if ( use_axi == 2 ) {
                XtorRead_M2(m2, p_offset, inbuffer, max_burst - p_offset % max_burst);
              } else {
                XtorRead_M1(m1, p_offset, inbuffer, max_burst - p_offset % max_burst);
              }
              StopTime = GetTime();
              elapsedTime += (StopTime - StartTime);
              MIGStatusSet(0);
          
              if ( (mode == MODE_WRITE_READ_COMPARE) || (mode == MODE_READ_COMPARE) )
                {
                  /* there are data to read from file? */
                  if ( mode == MODE_READ_COMPARE ) {
                    if (fread ((void *) (outbuffer), sizeof (char), (size_t) max_burst - (p_offset % max_burst), file) != (size_t) max_burst - (p_offset % max_burst))
                      {
                        printf ("  Error reading file: %s\n", strerror (errno));
                        goto exit;
                      }
                  } else {
                    GenerateData(outbuffer, max_burst - (p_offset % max_burst));
                  }
                  CompareData(outbuffer, inbuffer, max_burst - (p_offset % max_burst));
                  if ( retval != 0 ) goto exit;
                }              
              else if (mode == MODE_READ)
                {
                  /* there are data to write into the file? */
                  if (fwrite ((void *) (inbuffer), sizeof (char), (size_t) max_burst - (p_offset % max_burst), file) != (size_t) max_burst - (p_offset % max_burst))
                    {
                      printf ("  Error writing file: %s\n", strerror (errno));
                      goto exit;
                    }
                }              

              if ( quick > 0 ) {
                p_length = p_length - quick + p_offset % max_burst;
                p_offset = p_offset + quick - p_offset % max_burst;
              } else {
                p_length = p_length - max_burst + p_offset % max_burst;
                p_offset = p_offset + max_burst - p_offset % max_burst;
              }

              if (percentage != 0) {
                ppi=(double) (length - p_length) * 100 / length;
                if ( ppi - pi > 5 ) {
                  printf ("\r%.0f%%", ppi);
                  fflush(stdout);
                  pi=ppi;
                }
              }
            }

          if ( p_length > 0 ) {
            MIGStatusSet(1);
            StartTime = GetTime();
            if ( use_axi == 2 ) {
              XtorRead_M2(m2, p_offset, inbuffer, p_length);
            } else {
              XtorRead_M1(m1, p_offset, inbuffer, p_length);
            }
            StopTime = GetTime();
            elapsedTime += (StopTime - StartTime);
            MIGStatusSet(0);
            //printf ("%02x%02x%02x%02x \n", * (inbuffer + 3), * (inbuffer + 2), * (inbuffer + 1), * (inbuffer + 0));	

            //for (li = 0; li < (length); li ++)
            //  printf ("%x\n", *(inbuffer+li));
            
            if ((mode == MODE_WRITE_READ_COMPARE) || (mode == MODE_READ_COMPARE))
              {
                /* there are data to read from file? */
                if ((mode == MODE_READ_COMPARE)) {
                  if (fread ((void *) (outbuffer), sizeof (char), (size_t) p_length, file) != (size_t) p_length)
                    {
                      printf ("  Error reading file: %s\n", strerror (errno));
                      goto exit;
                    }
                } else {
                  GenerateData(outbuffer, p_length);
                }
                CompareData(outbuffer, inbuffer, p_length);
                if ( retval != 0 ) goto exit;
              }
            else if (mode == MODE_READ)
              {
                /* there are data to write into the file? */
                if (fwrite ((void *) (inbuffer), sizeof (char), (size_t) p_length, file) != (size_t) p_length)
                  {
                    printf ("  Error writing file: %s\n", strerror (errno));
                    goto exit;
                  }
              }                                       
          }	  

          dBytesPerSecond=s_length/((double)elapsedTime);
          dMBytesPerSecond=dBytesPerSecond/1024.0/1024.0;
          
          if (delay != -1) { 
            printf("\r100%%  ->  Byte/s: % 4.3f\tRead time (s): % 4.3f\n",dMBytesPerSecond*1024*1024,elapsedTime);
          } else {
            printf("\r100%%  ->  MByte/s: % 4.3f\tRead time (s): % 4.3f\n",dMBytesPerSecond,elapsedTime);
          }
          fflush(stdout);          
        }
      
      /* there are data to read/write from/into the file? */
      if ((mode == MODE_WRITE) || (mode == MODE_READ) || (mode == MODE_READ_COMPARE) || (mode == MODE_WRITE_FROM_FILE))
        {
          fclose (file);
          file = NULL;
          retval = 0;
        }
      
      if ((mode == MODE_WRITE_READ_COMPARE) || (mode == MODE_READ_COMPARE)) {
        printf ("Data comparison succeed.\n");
      }
    }
    
    printf ("\n");    
    
  exit:
    /* clean up */
    if (inbuffer) free (inbuffer);
    if (outbuffer) free (outbuffer);
    if (inbuffer_charles) free (inbuffer_charles);
    if (outbuffer_charles) free (outbuffer_charles);
    if (file) fclose (file);
    if (m1) delete (m1);
    if (m2) delete (m2);
    if (m3) delete (m3);
    if (MIG_STATUS_C_HANDLE != UMR_INVALID_HANDLE) {
      if ((umrbus_close(MIG_STATUS_C_HANDLE, &errormessage))) {
        printf("\nError : Close connection to mig_status CAPIM : %s\n", errormessage);
        umrbus_errormessage_free(&errormessage);
      }
    }
    if (errormessage) free (errormessage);    
    return (retval);
  
  usage_exit:
    usage (argv[0]);
    goto exit;
  }

#ifdef __cplusplus
}
#endif
