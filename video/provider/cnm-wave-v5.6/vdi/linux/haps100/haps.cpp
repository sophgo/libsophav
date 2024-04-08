#include <stdio.h>

#include "xactors.h"
#include "haps.h"
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _DP_ printf("%s:%d\n", __FUNCTION__, __LINE__);

/* global variables and switches */
int umr_device = 300;
int umr_bus = 1;

UMRBUS_LONG max_burst  = MAX_BURST_LENGTH;
#if __WORDSIZE == 64
long long offset     = 0;
long long offset_def = offset;
long long p_offset   = 0;
#else
UMRBUS_LONG offset     = 0;
UMRBUS_LONG offset_def = offset;
UMRBUS_LONG p_offset   = 0;
#endif
UMRBUS_LONG length     = 1024;
UMRBUS_LONG length_def = length;

UMRBUS_LONG delay      = 0;

long long p_index  = 0;
long long p_length = 0;

#define XTOR_A_WIDTH 32
#define XTOR_D32_WIDTH 32
#define XTOR_D64_WIDTH 64
#define XTOR_D128_WIDTH 128

#define MAX_XTOR_MASTER_ID 3

#define HEXDUMP_COLS 16

#if 0
static void hexdump(void *mem, uint32_t len)
{
    uint32_t i, j;

    for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++) {
        /* print offset */
        if(i % HEXDUMP_COLS == 0) {
            printf("0x%06x: ", i);
        }

        /* print hex data */
        if(i < len) {
            printf("%02x ", 0xFF & ((char*)mem)[i]);
        }
        else /* end of block, just aligning for ASCII dump */ {
            printf("   ");
        }

        /* print ASCII dump */
        if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1)) {
            for(j = i - (HEXDUMP_COLS - 1); j <= i; j++) {
                if(j >= len) { /* end of block, not really printing */
                    printf(" ");
                }
                else if(isprint(((char*)mem)[j])) { /* printable char */ 
                    printf("%d", 0xFF & ((char*)mem)[j]);
                }
                else { /* other char */
                    printf(".");
                }
            }
            printf("\n");
        }
    }
}
#endif

UMRBUS_LONG mig_status;
UMR_HANDLE MIG_STATUS_C_HANDLE = UMR_INVALID_HANDLE ;

char * errormessage;

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
  if ( 1 ) {
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
  if ( 1 ) {
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
//MasterXactor<XTOR_D64_WIDTH,XTOR_D128_WIDTH> *m128[MAX_XTOR_MASTER_ID] = {NULL, };
//MasterXactor<XTOR_A_WIDTH,XTOR_D32_WIDTH>  *m32[MAX_XTOR_MASTER_ID]  = {NULL, };
MasterXactor<XTOR_D64_WIDTH,XTOR_D128_WIDTH> *m128 = NULL ;
MasterXactor<XTOR_A_WIDTH,XTOR_D32_WIDTH>  *m32 = NULL ;



    int XtorSend_32(int mid, unsigned long address, unsigned char *ptSrcData, long Size) {
        TLMResponse<XTOR_D32_WIDTH>             rsp;
        TLMRequest<XTOR_A_WIDTH,XTOR_D32_WIDTH> req;
        req.m_header.m_command    = TLMCommand::e_WRITE;
        req.m_header.m_b_size     = TLMBSize::e_BITS32;
        req.m_header.m_burst_mode = TLMBurstMode::e_INCR;
        req.m_b_length            = unsigned ((address % 4 + Size + 3) / 4);
        req.m_address             = address;

        req.setPayload();

        req.fillPayload((char*) ptSrcData, Size * sizeof(char));

        //m32[mid]->send(req, rsp);
        m32->send(req, rsp);
        if (rsp.m_header.m_status != TLMStatus::e_SUCCESS) {
            printf("@Error: %s = %s\n", rsp.m_header.m_error.getClassName(), rsp.m_header.m_error.getName());
            return 1;
        }

        return 0;
    }

    int XtorRead_32(int mid, unsigned long address, unsigned char *ptDstData, long Size){
        TLMResponse<XTOR_D32_WIDTH>             rsp;
        TLMRequest<XTOR_A_WIDTH,XTOR_D32_WIDTH> req;
        req.m_header.m_command    = TLMCommand::e_READ;
        req.m_header.m_b_size     = TLMBSize::e_BITS32;
        req.m_header.m_burst_mode = TLMBurstMode::e_INCR;
        req.m_b_length            = unsigned ((address % 4 + Size + 3) / 4);
        req.m_address             = address;

        //m32[mid]->send(req, rsp);
        m32->send(req, rsp);
        if (rsp.m_header.m_status != TLMStatus::e_SUCCESS) {
            printf("@Error: %s = %s\n", rsp.m_header.m_error.getClassName(), rsp.m_header.m_error.getName());
            return 1;
        }

        rsp.fillMemory(address, (char*) ptDstData, Size * sizeof(char));
        
        return 0;
    }

    int XtorSend_128(int mid, unsigned long address, unsigned char *ptSrcData, long Size) {
        TLMResponse<XTOR_D128_WIDTH>             rsp;
        TLMRequest<XTOR_D64_WIDTH,XTOR_D128_WIDTH> req;
        req.m_header.m_command    = TLMCommand::e_WRITE;
        req.m_header.m_b_size     = TLMBSize::e_BITS128;
        req.m_header.m_burst_mode = TLMBurstMode::e_INCR;
        req.m_b_length            = unsigned ((address % 16 + Size + 15) / 16);	// 128
        req.m_address             = address;

        req.setPayload();

        req.fillPayload((char*) ptSrcData, Size * sizeof(char));

        //printf("%s:%d addr = %x, size = %d\n", __FUNCTION__, __LINE__, address, Size);
        //for (int i = 0 ; i< 21; i+=4)
        //    printf("%d=%d, %d=%d, %d=%d, %d=%d\n", i, ptSrcData[i], i+1, ptSrcData[i+1], i+2, ptSrcData[i+2], i+3, ptSrcData[i+3]);

        //m128[mid]->send(req, rsp);
        m128->send(req, rsp);
        if (rsp.m_header.m_status != TLMStatus::e_SUCCESS) {
            printf("@Error: %s = %s\n", rsp.m_header.m_error.getClassName(), rsp.m_header.m_error.getName());
            return 1;
        }
        
        return 0;
    }

    int XtorRead_128(int mid, unsigned long address, unsigned char *ptDstData, long Size) {
        TLMResponse<XTOR_D128_WIDTH>             rsp;
        TLMRequest<XTOR_D64_WIDTH,XTOR_D128_WIDTH> req;
        req.m_header.m_command    = TLMCommand::e_READ;
        req.m_header.m_b_size     = TLMBSize::e_BITS128;
        req.m_header.m_burst_mode = TLMBurstMode::e_INCR;
        req.m_b_length            = unsigned ((address % 16 + Size + 15) / 16);	// 128
        req.m_address             = address;

        //m128[mid]->send(req, rsp);
        m128->send(req, rsp);
        if (rsp.m_header.m_status != TLMStatus::e_SUCCESS) {
            printf("@Error: %s = %s\n", rsp.m_header.m_error.getClassName(), rsp.m_header.m_error.getName());
            return 1;
        }
        rsp.fillMemory(address, (char*) ptDstData, Size * sizeof(char));

        //printf("%s:%d addr = %x, size = %d\n", __FUNCTION__, __LINE__, address, Size);
        //for (int i = 0 ; i< 21; i+=4)
        //    printf("%d=%d, %d=%d, %d=%d, %d=%d\n", i, ptDstData[i], i+1, ptDstData[i+1], i+2, ptDstData[i+2], i+3, ptDstData[i+3]); 
        
        return 0;
    }

    void * haps_init(unsigned long core_idx, unsigned long dram_base, unsigned long dram_size)
    {
        printf ("haps_init start 1\n");

        int umr_scan = 0;
        int umr_scan_check = 3;
        int cid1;
        int i;
        int umr_dev = 0;
        umrbus_scan_type **umrbusscan;
        if((umrbusscan = umrbus_scan(&errormessage)) == NULL) {
            printf("\nError : UMRBus scan : %s\n", errormessage);
            umrbus_errormessage_free(&errormessage);             
        }

        i=0;                                                                                                       
        while(umrbusscan[i] != NULL) {                                                                             
            printf("[DEBUG] i : %d, umr_bus type : %x\n", i, umrbusscan[i]->type);                                   
                                                                                                           
            if ( 1 ) {                                                                                                                                                                                                                                               
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

                printf("Use UMRDev <%d>, UMRBus <%d>\n", umr_dev, umr_bus);                                                                        
                                                                                                    
                break;                                                                                                 
                }                                                                                                        
            i++;                                                                                                     
        }                                                                                                          


#ifdef SUPPORT_AXI1
        m128 = new MasterXactor<XTOR_D64_WIDTH,XTOR_D128_WIDTH>("master", umr_device, umr_bus, MASTER_ID1);
        //m128[0] = new MasterXactor<XTOR_D64_WIDTH,XTOR_D128_WIDTH>("master", umr_device, umr_bus, 1);
        //if (!m128[0]->isOpen()) {
        if (!m128->isOpen()) {
            printf ("Error: Cannot open transactor 'm128'\n");   
            return NULL;
        }
        else
            printf ("Open transactor 'm128[0]'\n");
#endif
#ifdef SUPPORT_APB
        //m32 = new MasterXactor<XTOR_A_WIDTH,XTOR_D32_WIDTH>("master", umr_device, umr_bus, APB_ID3);
        m32 = new MasterXactor<XTOR_A_WIDTH,XTOR_D32_WIDTH>("master", umr_device, umr_bus, ID_NUM_APB3);
        //m32[0] = new MasterXactor<XTOR_A_WIDTH,XTOR_D32_WIDTH>("master", umr_device, umr_bus, 3);
        //if (!m32[0]->isOpen()) {
        if (!m32->isOpen()) {
            printf ("Error: Cannot open transactor 'm32[0]'\n");   
            return NULL;
        }
        else
            printf ("Open transactor 'm32[0]'\n");
#endif

        //int cid1 = MIG_STATUS_C;

        if ((MIG_STATUS_C_HANDLE = umrbus_open(umr_device, umr_bus, cid1, &errormessage)) == UMR_INVALID_HANDLE) {
            printf("\nError : Could not open mig_status CAPIM : %s\n\n", errormessage);
        }
        //return m128[0];
        MIGStatusSet(1);
        return m128;
    }

    void haps_release(unsigned long core_idx)
    {
        /*
#ifdef SUPPORT_AXI1
        if (m128[0])  {
            delete (m128[0]);
            m128[0] = NULL;
            printf("if : Release transactor 'm128[0]'\n");
        }
        printf("Release transactor 'm128[0]'\n");
#endif
#ifdef SUPPORT_APB
        if (m32[0]) {
            delete (m32[0]);
            m32[0] = NULL;
            printf("if : Release transactor 'm32[0]'\n");
        }
        printf("Release transactor 'm32[0]'\n");
#endif
*/
        if (m128)  {
            delete (m128);
            //m128 = NULL;
            printf("if : Release transactor 'm128'\n");
        }

        if (m32) {
            delete (m32);
           //m32 = NULL;
            printf("if : Release transactor 'm32'\n");
        }

        MIGStatusSet(0);

        if (MIG_STATUS_C_HANDLE != UMR_INVALID_HANDLE) {         
            if (umrbus_close(MIG_STATUS_C_HANDLE, &errormessage)) {
                printf("\nError : Close connection to mig_status CAPIM : %s\n", errormessage);
            }
        }

    }

    int haps_write_axi(unsigned long axiNum,void * base, unsigned int addr, unsigned char *data, int len, int endian)
    {
        long long p_index = 0;
        long long p_length = len;
        //long long p_offset = addr - VDI_DRAM_PHYSICAL_BASE;
        long long p_offset = addr;

        //printf("%s:%d - addr=0x%x p_index=%d, data=%d, p_length=%d\n", __FUNCTION__, __LINE__, addr, p_index, data[p_index], p_length);
        /* split length into MAX_BURST_LENGTH packages */ 
        while ( ( p_length + p_offset % max_burst ) >= max_burst)	{
            // dummy single send because of bug in transactor lib
            if ( p_offset % 16 > 0 ) {
                XtorSend_128(axiNum, p_offset, &data[p_index], 16 - p_offset % 16);	
                p_index  = p_index  - p_offset % 16 + 16;
                p_length = p_length + p_offset % 16 - 16;
                p_offset = p_offset - p_offset % 16 + 16;
            }
            //printf("%s:%d - addr=0x%x p_index=%d, %d, p_length=%d\n", __FUNCTION__, __LINE__, addr, p_index, data[p_index], p_length);
            XtorSend_128(axiNum, p_offset, &data[p_index], max_burst - (p_offset % max_burst));

            p_length = p_length - max_burst + p_offset % max_burst;
            p_index  = p_index  + max_burst - p_offset % max_burst;
            p_offset = p_offset + max_burst - p_offset % max_burst;
        }
        if ( p_length > 0 ) { 
            //printf("%s:%d - addr=0x%x p_index=%d, %d, p_length=%d\n", __FUNCTION__, __LINE__, addr, p_index, data[p_index], p_length);
            // dummy single send because of bug in transactor lib
            if ( p_offset % 16 > 0 && p_offset % 16 + p_length > 16 ) {
                XtorSend_128(axiNum, p_offset, &data[p_index], 16 - p_offset % 16);	
                p_index  = p_index  - p_offset % 16 + 16;
                p_length = p_length + p_offset % 16 - 16;
                p_offset = p_offset - p_offset % 16 + 16;
            }
            XtorSend_128(axiNum, p_offset, &data[p_index], p_length);
        }
        return len;
    }

    int haps_read_axi(unsigned long axiNum, void * base, unsigned int addr, unsigned char *data, int len, int endian)
    {
        long long p_index = 0;
        long long p_length = len;
        //long long p_offset = addr - VDI_DRAM_PHYSICAL_BASE;
        long long p_offset = addr;

        /* split length into MAX_BURST_LENGTH packages */ 
        //printf("%s:%d - addr=0x%x p_index=%d, data=%d, p_length=%d\n", __FUNCTION__, __LINE__, addr, p_index, data[p_index], p_length);
        while ( ( p_length + p_offset % max_burst ) > max_burst) {
            // dummy single read because of bug in transactor lib
            if ( p_offset % 16 > 0 ) {
                XtorRead_128(axiNum, p_offset, &data[p_index], 16 - p_offset % 16);	
                p_index  = p_index  - p_offset % 16 + 16;
                p_length = p_length + p_offset % 16 - 16;
                p_offset = p_offset - p_offset % 16 + 16;
            }
            XtorRead_128(axiNum, p_offset, &data[p_index], max_burst - p_offset % max_burst);

            p_length = p_length - max_burst + p_offset % max_burst;
            p_index  = p_index  + max_burst - p_offset % max_burst;
            p_offset = p_offset + max_burst - p_offset % max_burst;
        }
        if ( p_length > 0 ) { 
            // dummy single read because of bug in transactor lib
            if ( p_offset % 16 > 0 && p_offset % 16 + p_length > 16 ) { 
                XtorRead_128(axiNum, p_offset, &data[p_index], 16 - p_offset % 16); 
                p_index  = p_index  - p_offset % 16 + 16; 
                p_length = p_length + p_offset % 16 - 16; 
                p_offset = p_offset - p_offset % 16 + 16; 
            }   
            XtorRead_128(axiNum, p_offset, &data[p_index], p_length);
        }    
        return len;
    }

    int haps_write_axi32(unsigned long axiNum,void * base, unsigned int addr, unsigned char *data, int len, int endian)
    {
        long long p_index = 0;
        long long p_length = len;
        //long long p_offset = addr - VDI_DRAM_PHYSICAL_BASE;
        long long p_offset = addr;

        //printf("%s:%d - addr=0x%x p_index=%d, data=%d, p_length=%d\n", __FUNCTION__, __LINE__, addr, p_index, data[p_index], p_length);
        /* split length into MAX_BURST_LENGTH packages */ 
        while ( ( p_length + p_offset % max_burst ) >= max_burst)	{
            usleep(0);
            // dummy single send because of bug in transactor lib
            if ( p_offset % 4 > 0 ) {
                XtorSend_32(axiNum, p_offset, &data[p_index], 4 - p_offset % 4);
                p_index  = p_index  - p_offset % 4 + 4;
                p_length = p_length + p_offset % 4 - 4;
                p_offset = p_offset - p_offset % 4 + 4;
            }
            //printf("%s:%d - addr=0x%x p_index=%d, %d, p_length=%d\n", __FUNCTION__, __LINE__, addr, p_index, data[p_index], p_length);
            XtorSend_32(axiNum, p_offset, &data[p_index], max_burst - (p_offset % max_burst));

            p_length = p_length - max_burst + p_offset % max_burst;
            p_index  = p_index  + max_burst - p_offset % max_burst;
            p_offset = p_offset + max_burst - p_offset % max_burst;
        }
        if ( p_length > 0 ) { 
            //printf("%s:%d - addr=0x%x p_index=%d, %d, p_length=%d\n", __FUNCTION__, __LINE__, addr, p_index, data[p_index], p_length);
            // dummy single send because of bug in transactor lib
            if ( p_offset % 4 > 0 && p_offset % 4 + p_length > 4 ) {
                XtorSend_32(axiNum, p_offset, &data[p_index], 4 - p_offset % 4);	
                p_index  = p_index  - p_offset % 4 + 4;
                p_length = p_length + p_offset % 4 - 4;
                p_offset = p_offset - p_offset % 4 + 4;
            }
            XtorSend_32(axiNum, p_offset, &data[p_index], p_length);
        }
        return len;
    }

    int haps_read_axi32(unsigned long axiNum, void * base, unsigned int addr, unsigned char *data, int len, int endian)
    {
        long long p_index = 0;
        long long p_length = len;
        //long long p_offset = addr - VDI_DRAM_PHYSICAL_BASE;
        long long p_offset = addr;

        /* split length into MAX_BURST_LENGTH packages */ 
        //printf("%s:%d - addr=0x%x p_index=%d, data=%d, p_length=%d\n", __FUNCTION__, __LINE__, addr, p_index, data[p_index], p_length);
        while ( ( p_length + p_offset % max_burst ) > max_burst) {
            usleep(0);
            // dummy single read because of bug in transactor lib
            if ( p_offset % 4 > 0 ) {
                XtorRead_32(axiNum, p_offset, &data[p_index], 4 - p_offset % 4);	
                p_index  = p_index  - p_offset % 4 + 4;
                p_length = p_length + p_offset % 4 - 4;
                p_offset = p_offset - p_offset % 4 + 4;
            }
            XtorRead_32(axiNum, p_offset, &data[p_index], max_burst - p_offset % max_burst);

            p_length = p_length - max_burst + p_offset % max_burst;
            p_index  = p_index  + max_burst - p_offset % max_burst;
            p_offset = p_offset + max_burst - p_offset % max_burst;
        }
        if ( p_length > 0 ) { 
            // dummy single read because of bug in transactor lib
            if ( p_offset % 4 > 0 && p_offset % 4 + p_length > 4 ) { 
                XtorRead_32(axiNum, p_offset, &data[p_index], 4 - p_offset % 4); 
                p_index  = p_index  - p_offset % 4 + 4; 
                p_length = p_length + p_offset % 4 - 4; 
                p_offset = p_offset - p_offset % 4 + 4; 
            }   
            XtorRead_32(axiNum, p_offset, &data[p_index], p_length);
        }
        return len;
    }

    int haps_write_register(unsigned long core_idx, void * base, unsigned int addr, unsigned int data) {
        unsigned int axiNum = 0;//ID_NUM_APB3;

        //printf("%s:%d - %d %x\n", __FUNCTION__, __LINE__, addr, data);
        haps_write_axi32(axiNum, base, addr, (unsigned char*)&data, 4, 0x31);
        return 4;
    }

    unsigned int haps_read_register(unsigned long core_idx, void * base, unsigned int addr) {
        unsigned int ptDstData = 0;
        unsigned int axiNum = 0;

        //printf("%d - %s:%d - %d %x\n",axiNum,  __FUNCTION__, __LINE__, addr, ptDstData);
        haps_read_axi32(axiNum, base, addr, (unsigned char*)&ptDstData, 4, 0x31);
        return ptDstData;
    }
    int haps_set_timing_opt(unsigned long core_idx, void * base) {
        return -1;
    }
    int haps_ics307m_set_clock_freg(void * base, int Device, int OutFreqMHz) {
        return -1;
    }
    void haps_write_reg_timing(unsigned long addr, unsigned int data) {
        return ;
    }
    int haps_set_clock_freg( void *base, int Device, int OutFreqMHz) {
        return -1;
    }
    long long haps_get_phys_base_addr(void) {
        return 0;
    }
#ifdef __cplusplus
}
#endif
