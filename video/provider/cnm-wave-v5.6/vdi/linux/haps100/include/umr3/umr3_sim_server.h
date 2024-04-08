/******************************************************************************
 Copyright (C) 2000 - 2017 Synopsys, Inc.
 This software and the associated documentation are confidential and
 proprietary to Synopsys, Inc. Your use or disclosure of this software
 is subject to the terms and conditions of a written license agreement
 between you, or your company, and Synopsys, Inc.
 *******************************************************************************
 Title      : UMR3 Simulation Server
 *******************************************************************************
 Description: This file is the include file for the UMR3 Simulation Server
 *******************************************************************************
 File       : umr3_sim_server.h
 *******************************************************************************
 $Id:$
******************************************************************************/ 

#include "umr3.h"
#include "umr3_status.h"
#include "svdpi.h"

#ifndef __UMR3_SIM_SERVER_H__

#define UMR3_SIM_DEFAULT_PORT 10000
#define MAX_CLIENTS 1024

#ifndef UMR3_SIM_MAX_FRAME_SIZE
#define UMR3_SIM_MAX_FRAME_SIZE 1024
#endif

#define UMR3_SIM_H_LEN    5
#define UMR3_SIM_P_LEN    UMR3_SIM_MAX_FRAME_SIZE


typedef enum {E_SH, E_CSH, E_NONE, E_BOTH} e_scriptType;
typedef enum {E_BDM, E_MCAPIM} e_CapimType;

#ifdef __cplusplus
#include <vector>

class UMR3SimServerSettings {
public:
    UMR3SimServerSettings();
    ~UMR3SimServerSettings();

    UMR3_STATUS ReadSettings();
    UMR3_STATUS GenerateScript(char* pcBasePath=NULL);
    int GetDevice();
    int GetPort();
    int GetDebugLevel();
    e_scriptType GetScriptType();

protected:
    char* FindFilePath();
    UMR3_STATUS CheckParameter(char* pcName, char* pcLine, int* parameter, int min, int max);

protected:
    int m_Device;
    int m_Port;
    int m_DebugLevel;
    e_scriptType m_scriptType;
};

/* defines to get the disassemble the UMR3 header*/
#define UMR3_SIM_GET_HID(h)     ((h[0] >> 16) & 0xFFFF)
#define UMR3_SIM_GET_OC(h)      ((h[0] >> 8) & 0xFF)
#define UMR3_SIM_GET_BM_ID(h)   ((((uint64_t) h[0] & 0xFF) << 32) | h[1])
#define UMR3_SIM_GET_TID(h)     ((h[2] >> 24) & 0xFF)
#define UMR3_SIM_GET_SC(h)      ((h[2] >> 20) & 0xF)
#define UMR3_SIM_GET_PLEN(h)    ((h[2] >> 0) & 0xFFFFF)
#define UMR3_SIM_GET_RLEN(h)    UMR3_SIM_GET_PLEN(h)
#define UMR3_SIM_GET_CRA(h)     ((h[2] >> 12) & 0xFF)
#define UMR3_SIM_GET_CWRBEN(h)  h[3]
#define UMR3_SIM_GET_CPLEN(h)   (h[2] & 0xFFF)
#define UMR3_SIM_GET_CRLEN(h)   UMR3_SIM_GET_CPLEN(h)
#define UMR3_SIM_GET_OFFSET(h)  (((uint64_t) h[4] << 32) | h[3])
#define UMR3_SIM_GET_ADDR(h)    UMR3_SIM_GET_OFFSET(h)
#define UMR3_SIM_GET_VERSION(f) ((f >> 8) & 0xFF)
#define UMR3_SIM_GET_TYPE(f)    ((f >> 16) & 0xF)

/* defines to assemble the UMR3 header*/
#define UMR3_SIM_SET_VALUE(h,m,s,v)  ((h & ~(m << s)) | (( v & m) << s))

#define UMR3_SIM_SET_HID(h,v)      UMR3_SIM_SET_VALUE(h,      0xFFFF,16, v)
#define UMR3_SIM_SET_OC(h,v)       UMR3_SIM_SET_VALUE(h,      0xFF,8, v)
#define UMR3_SIM_SET_BM_ID_H(h,v)  UMR3_SIM_SET_VALUE(h,      0xFF, 0,(v>>32))
#define UMR3_SIM_SET_BM_ID_L(h,v)  UMR3_SIM_SET_VALUE(h,0xFFFFFFFF, 0, v)
#define UMR3_SIM_SET_TID(h,v)      UMR3_SIM_SET_VALUE(h,      0xFF,24, v)
#define UMR3_SIM_SET_SC(h,v)       UMR3_SIM_SET_VALUE(h,       0xF,20, v)
#define UMR3_SIM_SET_PLEN(h,v)     UMR3_SIM_SET_VALUE(h,   0xFFFFF, 0, v)
#define UMR3_SIM_SET_RLEN(h,v)     UMR3_SIM_SET_PLEN (h,               v)
#define UMR3_SIM_SET_CRA(h,v)      UMR3_SIM_SET_VALUE(h,      0xFF,12, v)
#define UMR3_SIM_SET_CWRBEN(h,v)   UMR3_SIM_SET_VALUE(h,0xFFFFFFFF, 0, v)
#define UMR3_SIM_SET_CPLEN(h,v)    UMR3_SIM_SET_VALUE(h,     0xFFF, 0, v)
#define UMR3_SIM_SET_CRLEN(h,v)    UMR3_SIM_SET_CPLEN(h,               v)
#define UMR3_SIM_SET_OFFSET_L(h,v) UMR3_SIM_SET_VALUE(h,0xFFFFFFFF, 0, v)
#define UMR3_SIM_SET_OFFSET_H(h,v) UMR3_SIM_SET_VALUE(h,0xFFFFFFFF, 0,(v>>32))
#define UMR3_SIM_SET_ADDR_L(h,v)   UMR3_SIM_SET_OFFSET_L(h,            v)
#define UMR3_SIM_SET_ADDR_H(h,v)   UMR3_SIM_SET_OFFSET_H(h,            v)

/* timeout definition in ms*/
#ifndef UMR3_SIM_SCAN_TIMEOUT
#define UMR3_SIM_SCAN_TIMEOUT 5000
#endif

#ifndef UMR3_SIM_TIMEOUT
#define UMR3_SIM_TIMEOUT      10000
#endif

class UMR3SimScope {
public:
    UMR3SimScope(svScope scope, uint64_t addr);
    ~UMR3SimScope();

    /* store frames*/
    void SetNextFrame(uint32_t header[UMR3_SIM_H_LEN], uint32_t payload[UMR3_SIM_P_LEN]);
    void GetNextFrame(uint32_t header[UMR3_SIM_H_LEN], uint32_t payload[UMR3_SIM_P_LEN]);

    /* handle mutex*/
    void LockMutex();
    void UnlockMutex();

    /* general settings*/
    uint64_t m_Address;
    svScope  m_Scope;
    uint32_t m_Status;

    /* indicates if there is a new frame for sending to this scope*/
    bool m_bHasFrame;

protected:
    pthread_mutex_t m_SendMutex;
   
    uint32_t m_Header[UMR3_SIM_H_LEN];
    uint32_t m_Payload[UMR3_SIM_P_LEN];
};

class UMR3SimFrame {

public:
    UMR3SimFrame(uint32_t header[UMR3_SIM_H_LEN], uint32_t payload[UMR3_SIM_P_LEN], bool bDS);
    UMR3SimFrame(uint8_t HID, uint64_t BM_ID, uint8_t OC, uint8_t TID, uint8_t SC, bool bDS,
                 uint32_t* Payload = NULL, uint32_t PRLEN = 0, uint8_t CRA = 0, uint16_t CPRLEN = 0, 
                 uint32_t CWRBEN = 0, uint64_t ADDR_OFF = 0);
    UMR3SimFrame(const UMR3SimFrame &obj);
    ~UMR3SimFrame();
  
    UMR3_STATUS IsValid() {return m_bValid ? UMR3_STATUS_SUCCESS : UMR3_STATUS_FAILED;}
    UMR3_STATUS SendFrameToHw(svScope scope = NULL);

protected:
    UMR3_STATUS GetFrame(uint32_t header[UMR3_SIM_H_LEN], uint32_t payload[UMR3_SIM_P_LEN]);
  
public:
    uint8_t  m_HID;
    uint64_t m_BM_ID; /* BID and MID share the same bits in the header*/
    uint8_t  m_OC;
    uint8_t  m_TID;
    uint8_t  m_SC;
    uint32_t m_PLEN;
    uint32_t m_RLEN;
    uint8_t  m_CRA;
    uint16_t m_CPLEN;
    uint16_t m_CRLEN;
    uint32_t m_CWRBEN;
    uint64_t m_ADDR;
    uint64_t m_OFFSET;
    uint32_t m_Payload[UMR3_SIM_P_LEN];

    uint64_t m_UniqueID;

protected:
    /* indicates the direction of the frame; true for DS*/
    bool m_bDS;

    /* contains true if the frame is valid*/
    bool m_bValid;
};

class UMR3SimServerHandle {
public: 
    UMR3SimServerHandle(uint64_t uId, int iSocket);
    ~UMR3SimServerHandle();

    /* Get functions*/
    uint64_t GetId() {return m_uId;}
  
    /* functions to handle the list of complete transfers*/
    void AddToCompleteWriteList(umr3_mwrite_data resp);
    UMR3_STATUS RemoveFromCompleteWriteList(umr3_mwrite_data* resp);
    umr3_mwrite_data* GetCompleteWriteListFirstElement();

    /* functions to handle the list of acknowledgable transfers*/
    void AddToAckList(umr3_mwrite_data resp);
    UMR3_STATUS RemoveFromAckList(umr3_mwrite_data* resp);
    umr3_mwrite_data* FindInAckByTID(uint8_t TID);

protected:
    uint64_t m_uId; /* unique ID for each call to open; returned as 'handle' to the client*/
  
public:
    /* address of the associated MCAPIM*/
    uint64_t m_Address;

    /* true if MCAPIM is connected (open was performed)*/
    uint8_t  m_connected;

    /* Master struct */
    umr3_master_data m_Master;

    /* related variables for MC_WR*/
    umr3_mwrite_data m_mWrite;
    uint32_t* m_mWriteBuffer;

    /* related variables for MC_RD*/
    umr3_mread_data m_mRead;
    int       m_mReadSocket;
    
    /* supported features for the associated MCAPIM*/
    bool      m_CAPIsup;
    bool      m_INTIen;
    bool      m_INTIsup;
    bool      m_MAPIen;
    bool      m_MAPIsup;
    bool      m_MAPIConfig;
    uint32_t  m_Type;
    bool      m_MAPIenDirect;
    bool      m_MAPIsupDirect;
    bool      m_MAPIConfigDirect;
    int       m_iSocket;

    /* initial values */
    bool      m_INTIen_init;
    bool      m_MAPIen_init;
    bool      m_MAPIenDirect_init;
    
protected:
    std::vector<umr3_mwrite_data> m_CompleteMWrites;
    std::vector<umr3_mwrite_data> m_AckMWrites;
};

class UMR3SimServer {

public:
    UMR3SimServer();
    ~UMR3SimServer();

    /* start the server*/
    UMR3_STATUS StartServer(UMR3SimServerSettings Settings);

    /* handling of received frames*/
    UMR3_STATUS  AddFrame(uint32_t header[UMR3_SIM_H_LEN], uint32_t payload[UMR3_SIM_P_LEN]);
    static void* HandleMWrite(void* context);
    static void* HandleMRead(void* context);

    /* error message handling*/
    static void AppendErrormessage(int iSocket, const char* fmt, ...);
    static void DisableErrorPrint(int iSocket);
    
protected:
    /* function to handle client connection*/
    static void* ClientHandler(void* arg);

    /* functions to handle IOCTL commands*/
    static UMR3_STATUS HandleIOCTL_GENERIC_GET_DRIVER_INFO(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_GET_DEVICE_INFO(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_RESET_DEVICE(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_OPEN(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_SCAN_COUNT(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_SCAN_COUNT_MULTIPLE(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_SCAN(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_TEST(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_INTR_ENABLE(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_INTR(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_MASTER_ENABLE(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_MASTER_CONFIG(UMR3_DataPackage &Package, bool* bKeepSocketOpen);
    static UMR3_STATUS HandleIOCTL_UMR3_MWRITE(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_MREAD(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_CONTROL(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_OPEN_HANDLE(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_READ(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_WRITE(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_CLOSE(UMR3_DataPackage &Package);
    static UMR3_STATUS HandleIOCTL_UMR3_MWRITE_ACK(UMR3_DataPackage &Package);
    
    /* helper functions for UMR3 tree elements*/
    static void                     FreeTree(umr3_scan_item* element);
    static umr3_scan_item* FindTreeElementByAddr(uint64_t addr);
    static UMR3_STATUS              AddTreeElement(umr3_scan_item element);
    static UMR3_STATUS              AddTreeElementBDMn(uint32_t *frame, bool first, uint64_t bmid, uint32_t pos, uint32_t *depth, uint32_t *iterator, uint32_t *bit, uint64_t *pbmid);
    static UMR3_STATUS              InsertTreeElement(umr3_scan_item *element);
    static void                     UpdateTreeElement(umr3_scan_item *element, umr3_scan_item *new_element);
    static void                     LinkTreeElement(umr3_scan_item *element, umr3_scan_item **node);
    static bool                     IsTreeComplete(umr3_scan_item *element);
    static void                     AdjustTreePointers(uint64_t uiStart, umr3_scan_item *data);
    static void                     GetTree(uint8_t* pcData);

    /* helper functions for the UMR3 handle list*/
    static UMR3SimServerHandle* FindHandleById  (uint64_t id);
    static UMR3SimServerHandle* FindHandleByAddr(uint64_t addr);
    static UMR3_STATUS          RemoveHandleById(uint64_t id);
    static bool                 IsHandleValid(uint64_t addr);

    /* helper functions for the UMR3 received frame list*/
    static UMR3SimFrame* GetFrameByOC(uint8_t oc);
    static UMR3SimFrame* GetFrameByOCandBMID(uint8_t oc, uint64_t bm_id);
    static UMR3SimFrame* GetFrameByOCBMIDandTID(uint8_t oc, uint64_t bm_id, uint8_t tid);
    static UMR3_STATUS   RemoveFrame(UMR3SimFrame* frame, bool bDelete = true);

    /* helper functions for the UMR3 MWrite list*/
    static UMR3SimFrame* MWriteGetFirstElement();
    static void MWriteRemoveFrame(UMR3SimFrame* frame);
    
    /* helper functions for the UMR3 MRead list*/
    static void MReadRemoveFrame(UMR3SimFrame* frame);
    static UMR3SimFrame* MReadGetFirstElement();
    static UMR3_STATUS McapimLock(UMR3_DataPackage &Package, uint8_t *connected, uint64_t bmid);
    static UMR3_STATUS McapimUnLock(UMR3_DataPackage &Package, uint64_t bmid);
    
public:
    /* helper functions for the UMR3 scope list*/
    void     AddScope(svScope scope, uint64_t addr);
    uint64_t GetAddrByScope(svScope scope);
    svScope  GetScopeByAddr(uint64_t addr);
    void     SetScopeStatus(svScope scope, uint32_t status);
    uint32_t GetScopeStatus(svScope scope);
    UMR3SimScope* GetScopeObject(svScope scope);

    /* general helper functions*/
    static UMR3_STATUS GetHandleAndLock(uint32_t handle, UMR3SimServerHandle** shandle, bool* bIsLocked, bool bAppendError = true);
    static void ReleaseHandleLock(bool* bIsLocked);
    static UMR3_STATUS IsMCAPIM(uint32_t type);
    static uint64_t    GetDSAddr(uint64_t addr, int port);
    static uint64_t GetDSAddrBDMn(uint64_t address, uint32_t width, uint32_t port);
    
protected:
    /* server socket handle*/
    int m_SrvHandle;

    /* variables used for the tree structure*/
    static umr3_scan_item* m_ElementRoot;
    static uint64_t m_ElementCount;

    /* list of open handles*/
    static std::vector<UMR3SimServerHandle> m_Handles;
    static uint64_t m_HandleId;

    /* contains the received frames*/
    static std::vector<UMR3SimFrame> m_FramesRecv;
    static char* m_Errormessage;

    /* contains the last TID*/
    static uint8_t  m_LastTID;

    /* contains the list of Scopes*/
    static std::vector<UMR3SimScope> m_Scopes;

    /* contains frames for master write*/
    static std::vector<UMR3SimFrame> m_MWriteFrames;

    /* contains frames for master read*/
    static std::vector<UMR3SimFrame> m_MReadFrames;

public: 
    /* indicates if multiple scopes are used*/
    static bool m_bHasMultipleScopes;

    /* contains the ID for the last frame (ensures that each frame gets a different ID internally*/
    static uint64_t m_LastFrameId;
};

/* error handling*/
struct tErrors {
    int iSocket;
    char* pcErrorMessage;
    int iPrintErr;
};
tErrors* FindMessageBuffer(int iSocket);
void  ReserveMessageBuffer(int iSocket);
void  FreeMessageBuffer(int iSocket);

extern "C" {

    /* dpi functions*/
    int  umr3_sim_host_to_hw(uint32_t header[UMR3_SIM_H_LEN], uint32_t payload[UMR3_SIM_P_LEN]);
    int  umr3_sim_get_status(svScope scope);
    void umr3_sim_set_next_frame(uint32_t header[UMR3_SIM_H_LEN], uint32_t payload[UMR3_SIM_P_LEN], svScope scope);
    
    /* functions used for timeout*/
    uint32_t GetUniversalTickCount(void);
    uint32_t GetUniversalTimeDiff(uint32_t ulTimeStart);

}

#else /*__cplusplus*/

/* DPI functions*/
int umr3_sim_init();
int umr3_sim_handle_hw_request(uint32_t header[UMR3_SIM_H_LEN], uint32_t payload[UMR3_SIM_P_LEN]);
int umr3_sim_get_next_frame(uint32_t header[UMR3_SIM_H_LEN], uint32_t payload[UMR3_SIM_P_LEN], svScope scope);

#endif /*__cplusplus*/
#endif /*__UMR3_SIM_SERVER_H__*/
