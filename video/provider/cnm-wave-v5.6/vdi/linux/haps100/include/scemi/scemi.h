/******************************************************************************
 Copyright © 2009 Synopsys, Inc. This Example and the associated documentation
 are confidential and proprietary to Synopsys, Inc. Your use or disclosure of
 this example is subject to the terms and conditions of a written license
 agreement between you, or your company, and Synopsys, Inc.
 *******************************************************************************
 Title      : SCE-MI Object definitions
 Project    : SCE-MI 2 Implementation
 ******************************************************************************/
/**
 * @file   scemi.h
 * @author  Roshan Antony Tauro
 * @date   2006/10/17
 *
 * @brief  This file contains the Declarations of Structures used in the Library.
 *
 */
/******************************************************************************
 $Id: //chipit/chipit/main/release/tools/scemi2/sw/include/scemi.h#2 $ $Author: mlange $ $DateTime: 2016/10/05 05:33:44 $
 ******************************************************************************/
#ifndef __Scemi_h__
#define __Scemi_h__

/* Include standard Header files */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include "pthread.h"
#include "umrbus.h"

/* Include SceMi Header Files */
#include "SceMiMacros.h"
#include "SceMiTypes.h"
#include "SceMiConstants.h"

#undef svGetScope
#undef svSetScope
#undef svPutUserData
#undef svGetUserData
#undef svGetNameFromScope
#undef svGetScopeFromName
#undef svGetCallerInfo
#undef svDpiVersion
#undef svGetBitselBit
#undef svPutBitselBit
#undef svGetPartselBit
#undef svPutPartselBit
#undef svScope

#define svGetScope scemiGetScope
#define svSetScope scemiSetScope
#define svPutUserData scemiPutUserData
#define svGetUserData scemiGetUserData
#define svGetNameFromScope scemiGetNameFromScope
#define svGetScopeFromName scemiGetScopeFromName
#define svGetCallerInfo scemiGetCallerInfo
#define svDpiVersion scemiDpiVersion
#define svGetBitselBit scemiGetBitselBit
#define svPutBitselBit scemiPutBitselBit
#define svGetPartselBit scemiGetPartselBit
#define svPutPartselBit scemiPutPartselBit
#define svScope scemiScope

#ifdef __cplusplus
namespace SceMiC {
#endif
/**
 * @brief Structure represents a received message data
 *        or a message to send.
 */
typedef struct S_SceMiMessageData {
	SceMiU32 *pulMessage; /*!< pointer to the message */
	SceMiU16 uwBitWidth; /*!< number of bits in the message */
	SceMiU16 uwWordWidth; /*!< number of words needed for the message */
	SceMiU64 ul64CycleStamp;/*!< the cycle stamp only used for received message */
} SceMiMessageData;
#ifdef __cplusplus
}
#endif

/**
 * @brief Structure represents a MessageInPortProxy Binding
 */
typedef struct S_SceMiMessageInPortBinding {
	void *Context; /*!< context passed to the call-back functions by the user */
	void (*IsReady)(void *context); /*!< function called when ISREADYMESSAGE interrupt received */
	void (*Close)(void *context); /*!< function called to close the connection */
} SceMiMessageInPortBinding;

#ifdef __cplusplus
namespace SceMiC {
#endif
/**
 * @brief Structure represents a MessageOutPortProxy Binding
 */
typedef struct S_SceMiMessageOutPortBinding {
	void *Context; /*!< context passed to the call-back functions by the user */
	void (*Receive)(void *context, const SceMiMessageData *data);
	void (*Close)(void *context); /*!< function called when message is received from the MessageOutPort */
} SceMiMessageOutPortBinding; /*!< function called to close the connection */

/**
 * @brief The next structures are needed for the
 *        parameter data base
 */
typedef struct S_SceMiParamAttribute {
	const char *szName; /*!< name of the attribute */
	const char *szValue;/*!< value of the attribute held as string */
} SceMiParamAttribute;

DECLARE_LIST(SceMiParamAttributeList, SceMiParamAttribute)
typedef struct S_SceMiParamInstance {
	SceMiParamAttributeList *psAttrList;/*!< list of attributes */
} SceMiParamInstance;

DECLARE_LIST(SceMiParamInstanceList, SceMiParamInstance)
typedef struct S_SceMiParamObjectKind {
	char *scObjectKindName; /*!< name of the object kind */
	SceMiParamInstanceList *psInsList;/*!< the list of instances */
} SceMiParamObjectKind;

DECLARE_LIST(SceMiParamObjectKindList, SceMiParamObjectKind)

typedef struct S_SceMiParameters {
	SceMiParamObjectKindList *psObjKindList;/*!< the list with object kinds */
} SceMiParameters;

/**
 * @brief Structure represents message in/out port proxy
 */
typedef struct S_SceMiMessageInPortProxy {
	SceMiMessageInPortBinding sPortBinding;/*!< the binding to the callback functions */
	const char *szTransName; /*!< name of the transactor */
	const char *szPortName; /*!< port name */
	SceMiU16 uwPortWidth; /*!< the bit width of the port */
	BOOL bPortReady; /*!< flag signaled that this port is ready for new data */
	SceMiU32 ulPortAddress; /*!< port address */
	UMR_HANDLE hUmrbusHandle; /*!< handle to umrbus */
} SceMiMessageInPortProxy;

typedef struct S_SceMiMessageOutPortProxy {
	SceMiMessageOutPortBinding sPortBinding;/*!< the binding to the call-back functions */
	const char *szTransName; /*!< name of the transactor */
	const char *szPortName; /*!< port name */
	SceMiU16 uwPortWidth; /*!< the bit width of the port */
	SceMiU32 ulPortAddress; /*!< port address */
	UMR_HANDLE hUmrbusHandle; /*!< handle to umrbus */
} SceMiMessageOutPortProxy;

typedef struct t_vpi_time {
	int type; /*!< One of vpiScaledRealTime and vpiSimTime */
	SceMiU32 high, low; /*!< The sim time values for vpiSimTime types */
	double real; /*!< The sim time values for vpiScaledRealTime types */
} s_vpi_time;

/**
 * @brief Structure represents a SCE-MI2 DPI function
 */
typedef struct S_SceMiDPIFunction {
	const char* szFunctName; /*!< name of the function */
	const char *szInstancePath; /*!< instance path of the function */
	SceMiU16 uwInputWidth; /*!< bit width of input data */
	SceMiU16 uwOutputWidth; /*!< bit width of output data */
	SceMiU16 uwInputWordWidth; /*!< word width of input data */
	SceMiU16 uwOutputWordWidth; /*!< word width of output data */
	BOOL bHasMultipleInstances; /*< flag indicating multiple instances */
	/**
	 * @brief specify the type of the DPI function
	 *        \li 0 for SceMiExportFunction
	 *        \li 1 for SceMiImportFunction
	 */
	unsigned char ubDPIType;
	SceMiU32 ulPortAddress; /*!< port address */
	UMR_HANDLE hUmrbusHandle; /*!< handle to umrbus */
} SceMiDPIFunction;

/* The pipe call back function handle */
typedef void (*scemi_pipe_notify_callback)(void *context);

/* insert pipe buffer struct */
typedef struct S_SceMiPipeBuffer {
	SceMiU32 iReadPos; /*!< read position */
	SceMiU32 iWritePos; /*!< write position */
	SceMiU32 uiBlocksAvailable; /*!< bytes available for new data */
	SceMiU32 *ucData; /*!< data array */
	SceMiU32 uiFlushRequest; /*!< flush request */
	SceMiU32 uiPackageInProgress; /*!< package in progress indicator */
	SceMiU32 uiBytesFromLastPackage; /*!< 32 bit words from the last package to be read */
	unsigned char cEom; /*!< the eom flag */
	svBit ucAutoFlush; /*!< the auto flush flag */
	pthread_mutex_t tWaitForBufferData; /*!< mutex to wait till buffer data is available */
	pthread_cond_t tOKToContinue; /*!< ok to continue condition signal */
	pthread_mutex_t tWaitForFlushAck; /*!< mutex to wait till flush acknowledgment is received */
	pthread_mutex_t tPipeBufferMutex;/*!< Mutex for the Pipe Buffer Read Write functions */
	pthread_cond_t tFlushed; /*!< flushed conditional signal */
	SceMiU32 iOKToContinue; /*!< ok to continue condition signal */
	SceMiU32 iIsOKToContinue; /*!< is ok to continue flag */
	SceMiU32 iFlushed; /*!< flushed conditional signal */
	SceMiU32 iPipeDepth; /*!< depth of the pipe */
} SceMiPipeBuffer;

typedef struct S_SceMiPipe {
	const char *uwPipeId; /*!< the pipe id */
	SceMiU16 uwPipeEBytes; /*!< the number of bytes for a element at the hardware side */
	SceMiU16 uwPipeENumber; /*!< the number of elements for each transfer at the hardware side. */
	/**
	 * @brief specify the type of the pipe
	 *        \li 0 for SceMiSendPipe
	 *        \li 1 for SceMiReceivePipe
	 */
	unsigned char ubPipeType;
	SceMiU32 ulPortAddress; /*!< port address */
	UMR_HANDLE hUmrbusHandle; /*!< handle to umrbus */
	SceMiMessageData *psPipeDataHandle; /*!< handle to the message data */
	scemi_pipe_notify_callback pfNotifyCallback; /*!< handle to the notify call-back function */
	void *pvNotifyContext; /*!< context to the notify call back function */
	SceMiU32 iPipeDepth; /*!< depth of the pipe */
	SceMiPipeBuffer sPipeBuffer; /*!< the pipe buffer structure */
} SceMiPipe;

#ifdef __cplusplus
}
#endif

/* The SCE-MI Service Loop Handler */
typedef int (*SceMiServiceLoopHandler)(void *context, int pending);

/**
 * @brief Structure that stores the User's ServiceLoop call-back function
 */
typedef struct S_SceMiServiceLoopBinding {
	SceMiServiceLoopHandler sceMiServiceLoopHandler; /*!< pointer to the call-back function g */
	void *context; /*!< context passed to the function */
} SceMiServiceLoopBinding;

#ifdef __cplusplus
namespace SceMiC {
#endif

DECLARE_LIST(UMRBusHandleList, UMR_HANDLE)
DECLARE_LIST(SceMiU32List, SceMiU32)
DECLARE_LIST(SceMiMessageInPortProxyList, SceMiMessageInPortProxy)
DECLARE_LIST(SceMiMessageOutPortProxyList, SceMiMessageOutPortProxy)
DECLARE_LIST(SceMiDPIFunctionList, SceMiDPIFunction)
DECLARE_LIST(SceMiPipeList, SceMiPipe)
DECLARE_LIST(SceMiPipeNotifierList, SceMiPipe*)

/**
 * @brief Structure represents a svScope User Data
 */
typedef struct S_scemiUserData {
	void *key; /*!< key*/
	void *value; /*!< value*/
} scemiUserData;
DECLARE_LIST(SceMiUserDataList, scemiUserData)

/**
 * @brief Structure represents a svScope object
 */
typedef struct S_scemiScope {
	const char *szInstancePath; /*!< the instance path string */
	SceMiUserDataList *psUserDataList; /*!< pointer to the user data list */
} scemiScope;
DECLARE_LIST(SceMiScopeList, scemiScope)

/**
 * @brief Structure represents a SCE-MI Structure
 */
typedef struct S_SceMi {
	/**
	 @brief lists for message in / out proxies
	 */
	SceMiMessageInPortProxyList *psInPortProxyList; /*!< list of message in port */
	SceMiMessageOutPortProxyList *psOutPortProxyList; /*!< list of message out ports */

	const SceMiParameters* psParams; /*!< pointer to the parameter data base */

    SceMiU32 uiScemiHwVersion; /* the hardware version */
    BOOL     bSendPipeHasInterrupt; /* TRUE if the send pipes support FILL LEVEL interrupts */
    BOOL     bInterruptFifoHasCount; /* TRUE if the interrupt FIFO supports FILL LEVEL interrupts */

	SceMiU32 uiUmrbusDevice;/*!< the umrbus device */
	UMR_HANDLE hTimeResetControlUmrbusHandle;/*!< handle to the time reset control CAPIM */
	int iPrecisionCode;/*!< the precision code of the cycle stamp */

	BOOL bServiceLoopCalled; /*!< Flag to indicate first call of ServiceLoop() */

	/**
	 @brief lists of UMR handles and interrupt list
	 */
	UMRBusHandleList *psInterruptHandleList; /*!< list of handles to the interrupt CAPIMs */
	UMRBusHandleList *psDataHandleList; /*!< list of handles to the data CAPIMs */
	SceMiU32List *psInterruptList; /*!< interrupt list */

	/**
	 @brief lists for DPI functions and pipes
	 */
	SceMiDPIFunctionList *psDPIFuncList; /*!< list of DPI functions */
	SceMiPipeList *psPipeList; /*!< list of SCE-MI pipes */
    SceMiPipeNotifierList *psPipeNotifierList;

	/**
	 * @brief list of scope objects
	 */
	SceMiScopeList *psSvScopeList; /*!< list of scope objects */

	scemiScope psSvCurrentScope; /*!< the current svScope object*/
	scemiScope psSvDefaultScope; /*!< the default svScope object*/

	/**
	 @brief Thread Handles
	 */
    pthread_mutex_t tCommunicationMutex;/*!< Mutex for the Read Write functions */
	pthread_mutex_t tServiceLoopMutex;/*!< Mutex for the service loop */
	pthread_mutex_t tServiceLoopDataMutex; /*!< Mutex to synchronize Service Loop and the ServiceLoopThread*/
	BOOL bServiceLoopActive; /*!< Mutex to synchronize Service Loop and the ServiceLoopThread*/
	BOOL bServiceLoopThreadCancel;
	pthread_t hServiceLoopThread;/*!< Handle to the Service Loop Thread */
	pthread_cond_t tOKToReturn;/*!< Condition VAriable to signal end of SceMi 1 Processing */
	pthread_mutex_t tPollInterruptMutex;/*!< Mutex for the Pipe Buffer Read Write functions */
	pthread_mutex_t tPipeNotifierMutex;/*!< Mutex for the pipe notifiers */
	pthread_mutex_t tScopeMutex;/*!< Mutex for the scope functions */
	char cOKToReturn;
	pthread_t tid; /*!< the thread id */

	/**
	 @brief Service Loop Handler
	 */
	SceMiServiceLoopBinding *psSceMiServiceLoopBinding;

	/**
	 @brief Service Loop Handler
	 */
	int iSceMiServiceRequestsArrivedCount;
} SceMi;

/**
 @brief Global pointer to the SceMi Object
 */
extern SceMi *psGlobalSceMiObjectPointer;
extern SceMiU32 uiGlobalSceMiDebugIndentLevel;
extern SceMiU32 uiGlobalSceMiDebugIndentCount;
extern SceMiU32 uiGlobalSceMiDebugLevel;
extern FILE *uiGlobaSceMilDebugFile;
#ifdef __cplusplus
}
#endif

/**
 @brief Enumeration of Info Types
 */
typedef enum e_SceMiInfoType {
	SceMiInfo, /*!< indicates info */
	SceMiWarning, /*!< indicates warning */
	SceMiNonFatalError
/*!< indicates an fatal error */
} SceMiInfoType;

/**
 @brief Enumeration of Error Types
 */
typedef enum e_SceMiErrorType {
	SceMiOK, /*!< Indicates OK */
	SceMiError
/*!< Indicates an Error */
} SceMiErrorType;

/**
 @brief Structure that define a SceMi Error
 */
typedef struct S_SceMiEC {
	const char *Culprit; /*!< The erring function */
	const char *Message; /*!< The error message */
	SceMiErrorType Type; /*!< The error type */
	int Id; /*!< an optional error id */
} SceMiEC;

typedef enum e_PipeMode {
	PIPETRYSEND,
	PIPETRYRECEIVE,
	PIPECANSEND,
	PIPECANRECEIVE,
	PIPESEND,
	PIPERECEIVE
} PipeMode;

/* Pointer to the Error HAndler */
typedef void (*SceMiErrorHandler)(void *context, SceMiEC *ec);
extern SceMiErrorHandler sceMiErrorHandler;
extern void * sceMiErrorContext;

/**
 @brief structure that defines the SceMi info
 */
typedef struct S_SceMiIC {
	const char *Originator; /*!< The originating function */
	const char *Message; /*!< The info message */
	SceMiInfoType Type; /*!< The info type */
	int Id; /*!< an optional info id */
} SceMiIC;

/* The Pointer to an Info Handler */
typedef void (*SceMiInfoHandler)(void *context, SceMiIC *ic);
extern SceMiInfoHandler sceMiInfoHandler;
extern void *sceMiInfoContext;

#ifdef __cplusplus
namespace SceMiC {
#endif
/* Initialize all SceMi Functions */
int SceMiServiceLoop(SceMi *, SceMiServiceLoopHandler, void *, SceMiEC *);
int SceMiVersion(const char *versionString);
SceMi *SceMiInit(int, const SceMiParameters *, SceMiEC *);
SceMi *SceMiPointer(SceMiEC *);
void SceMiShutdown(SceMi *, SceMiEC *);
SceMiMessageInPortProxy *SceMiBindMessageInPort(
		SceMi *,
		const char *,
		const char *,
		const SceMiMessageInPortBinding *,
		SceMiEC *);
SceMiMessageOutPortProxy *SceMiBindMessageOutPort(
		SceMi *,
		const char *,
		const char *,
		const SceMiMessageOutPortBinding *,
		SceMiEC *);
SceMiMessageData *SceMiMessageDataNew(
		const SceMiMessageInPortProxy *,
		SceMiEC *);
void SceMiMessageDataDelete(SceMiMessageData *);
unsigned int SceMiMessageDataWidthInBits(const SceMiMessageData *);
unsigned int SceMiMessageDataWidthInWords(const SceMiMessageData *);
void SceMiMessageDataSet(SceMiMessageData *, unsigned int, SceMiU32, SceMiEC *);
void SceMiMessageDataSetBit(SceMiMessageData *, unsigned int, int, SceMiEC *);
void SceMiMessageDataSetBitRange(
		SceMiMessageData *,
		unsigned int,
		unsigned int,
		SceMiU32,
		SceMiEC *);
SceMiU32 SceMiMessageDataGet(const SceMiMessageData *, unsigned int, SceMiEC *);
SceMiU32 SceMiMessageDataGetBit(
		const SceMiMessageData *,
		unsigned int,
		SceMiEC *);
SceMiU32 SceMiMessageDataGetBitRange(
		const SceMiMessageData *,
		unsigned int,
		unsigned int,
		SceMiEC *);
const char *SceMiMessageInPortProxyTransactorName(
		const SceMiMessageInPortProxy *);
const char *SceMiMessageOutPortProxyTransactorName(
		const SceMiMessageOutPortProxy *);
const char *SceMiMessageInPortProxyPortName(const SceMiMessageInPortProxy *);
const char *SceMiMessageOutPortProxyPortName(const SceMiMessageOutPortProxy *);
unsigned int SceMiMessageInPortProxyPortWidth(const SceMiMessageInPortProxy *);
unsigned int
		SceMiMessageOutPortProxyPortWidth(const SceMiMessageOutPortProxy *);
SceMiU64 SceMiMessageDataCycleStamp(const SceMiMessageData *);
void SceMiMessageInPortProxySend(
		SceMiMessageInPortProxy *,
		const SceMiMessageData *,
		SceMiEC *);
void SceMiRegisterErrorHandler(SceMiErrorHandler, void *);
void SceMiRegisterInfoHandler(SceMiInfoHandler, void *);
SceMiParameters *SceMiParametersNew(const char *, SceMiEC *);
void SceMiParametersDelete(SceMiParameters *);
unsigned int SceMiParametersNumberOfObjects(
		const SceMiParameters *,
		const char *,
		SceMiEC *);
int SceMiParametersAttributeIntegerValue(
		const SceMiParameters *,
		const char *,
		unsigned int,
		const char *,
		SceMiEC *);
const char *SceMiParametersAttributeStringValue(
		const SceMiParameters *,
		const char *,
		unsigned int,
		const char *,
		SceMiEC *);
void SceMiParametersOverrideAttributeIntegerValue(
		SceMiParameters *,
		const char *,
		unsigned int,
		const char *,
		int,
		SceMiEC *);
void SceMiParametersOverrideAttributeStringValue(
		SceMiParameters *,
		const char *,
		unsigned int,
		const char *,
		const char *,
		SceMiEC *);
SceMiEC *setSceMiErrorObject(
		SceMiEC *,
		const char *,
		const char *,
		SceMiErrorType);
SceMiIC *setSceMiInfoObject(
		SceMiIC *,
		const char *,
		const char *,
		SceMiInfoType);
void vpi_get_time(vpiHandle, s_vpi_time *);
int vpi_get(int, vpiHandle);
void scemi_pipe_c_receive(void *, int, int *, svBitVecVal *, svBit *);
int scemi_pipe_c_try_receive(void *, int, int, int *, svBitVecVal *, svBit *);
int scemi_pipe_c_can_receive(void *);
void scemi_pipe_c_send(void *, int, const svBitVecVal *, svBit);
int scemi_pipe_c_try_send(void *, int, int, svBitVecVal *, svBit);
int scemi_pipe_c_can_send(void *);
svBit scemi_pipe_c_set_eom_auto_flush(void *, svBit);
void scemi_pipe_set_notify_callback(void *, scemi_pipe_notify_callback, void *);
void *scemi_pipe_get_notify_context(void *);
int scemi_pipe_get_bytes_per_element(void *);
svBit scemi_pipe_get_direction(void *);
void scemi_pipe_c_flush(void *);
int scemi_pipe_c_try_flush(void *);
void *scemi_pipe_c_handle(const char *);
int scemi_pipe_get_depth(void *);
void scemi_pipe_set_depth(void *, int);
int SceMiRequest(
	SceMiDPIFunction *,
	SceMiU32 *,
	SceMiU32,
	SceMiU32 *,
	SceMiU32);
extern int SceMi_CallCFromHdl(
	SceMiU32,
	SceMiU32 *,
	SceMiU32,
	SceMiU32 *,
	SceMiU32);

/* System Verilog Scope Functions */
scemiScope scemiGetScope();
scemiScope scemiSetScope(const scemiScope);
int scemiPutUserData(const scemiScope, void *, void *);
void *scemiGetUserData(const scemiScope, void *);
const char *scemiGetNameFromScope(const scemiScope);
scemiScope scemiGetScopeFromName(const char *);
int scemiGetCallerInfo(char **, int *);
const char *scemiDpiVersion();
svBit scemiGetBitselBit(const svBitVecVal *, int);
void scemiPutBitselBit(svBitVecVal *, int, svBit);
void scemiGetPartselBit(svBitVecVal *, const svBitVecVal *, int, int);
void scemiPutPartselBit(svBitVecVal *, const svBitVecVal, int, int);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <iostream>

class SceMi;
class SceMiMessageData;
class SceMiMessageInPortProxy;
class SceMiMessageOutPortProxy;
class SceMiParameters;

/**
 * @brief Structure represents a MessageOutPortProxy Binding for C++
 */
typedef struct S_SceMiMessageOutPortBinding {
	void *Context; /*!< context passed to the call-back functions by the user */
	void (*Receive)(void *context, const SceMiMessageData *data);
	void (*Close)(void *context); /*!< function called when message is received from the MessageOutPort */
} SceMiMessageOutPortBinding; /*!< function called to close the connection */

/**
 @brief C++ Wrapper class for the SceMi Structure.
 */
class SceMi {
	friend class SceMiMessageInPortProxy;
	friend class SceMiMessageOutPortProxy;
public:
	/**
	 @brief c'tor
	 */
	SceMi();
	/**
	 @brief standard d'tor
	 */
	~SceMi();
	/**
	 @brief This function is used to Initialize the SceMi Object

	 @param version  The return value from SceMiVersion call (a version handle).
	 @param parameters Pointer to the created SceMiParameters object.
	 @param ec Pointer to a additional error handler.

	 Action flow:
	 \li Init the library internal data.
	 \li Open all UMRBus communication ports to the hardware interface.
	 \li Save all internal used and needed data to the SceMi object.
	 \li Set the softReset to high over SCE-MI time reset control CAPIM (Wait until the reset is really set at hardware side).
	 \li Clear the interrupt FIFO ? read all interrupts over SCE-MI time reset control CAPIM and chuck away this data.
	 \li Clear the read FIFOs ? read FIFO size elements over SCE-MI data CAPIM and chunk away this data.
	 \li Set the softReset to low over SCE-MI time reset control CAPIM (wait until the reset is really set at hardware side).
	 \li Start the ServiceLoopThread.
	 \li Return the object pointer.

	 @return \li Pointer to the Initialized SCE-MI Object on success.
	 \li NULL Pointer on Failure.
	 */
	static SceMi* Init(
			int version,
			const SceMiParameters* parameters,
			SceMiEC* ec = 0);
	/**
	 @brief This function is used to Stop the ServiceLoopThread

	 @param mct Pointer the SceMi object created with the SceMiInit function
	 @param ec Pointer to a additional error handler

	 Action flow:
	 \li Stop the <i>ServiceLoopThread</i> and wait until the thread return
	 \li Close all UMRBus communication ports to the hardware
	 \li Delete all internal data
	 */
	static void Shutdown(SceMi* mct, SceMiEC* ec = 0);
	/**
	 @brief Binds call back functions to the specified SceMiMessageIn Port

	 @param transactorName Hierarchical transactor name of the SceMiMessageIn(Out)Port in the HDL design (see section 4.3)
	 @param portName Instance name of the SceMiMessageIn(Out)Port in the HDL design (see section 5.3)
	 @param binding Structure hold user call-back function and user data
	 @param ec Pointer to a additional error handler

	 @return \li Returns the pointer to the SceMiMessageInPortProxy Structure
	 \li NULL on failure
	 */
	SceMiMessageInPortProxy* BindMessageInPort(
			const char* transactorName,
			const char* portName,
			const SceMiMessageInPortBinding* binding = 0,
			SceMiEC* ec = 0);
	/**
	 @brief Creates a new message data with specified SceMiMessageInPortProxy bit width
	 size of data

	 @param transactorName Hierarchical transactor name of the SceMiMessageIn(Out)Port in the HDL design (see section 4.3)
	 @param portName Instance name of the SceMiMessageIn(Out)Port in the HDL design (see section 5.3)
	 @param binding Structure hold user call-back function and user data
	 @param ec Pointer to a additional error handler

	 @return \li Returns the pointer to the SceMiMessageOutPortProxy Structure
	 \li NULL on failure
	 */
	SceMiMessageOutPortProxy* BindMessageOutPort(
			const char* transactorName,
			const char* portName,
			const SceMiMessageOutPortBinding* binding = 0,
			SceMiEC* ec = 0);
	/**
	 @brief This is the implementation of the ServiceLoop

	 @param g The “g” call-back function pointer must be called at each message port transfer (send or receive).
	 @param context User data for each “g” function call.
	 @param ec Pointer to a additional error handler..

	 Action flow:
	 \li The SceMiServiceLoop calls allow the library to service all the port proxies.
	 \li The g function must be registered and the ServiceLoopThread must be signaled to enable SCE-MI1 functionality.
	 \li The function returns first, after ServiceLoopThread has signaled the “ok to return”.
	 \li See reference 2 section 5.4.3.7 for more details.

	 @return The number of SCE-MI1 service requests that arrived from the HDL side and were processed since the last call to SceMiServiceLoop.
	 */
	int ServiceLoop(SceMiServiceLoopHandler g = NULL, void* context = 0, SceMiEC* ec =
			0);
	/**
	 @brief This function is used to Register Error Handlers

	 @param errorHandler The user defined error handler function for SCE-MI
	 @param context The user context

	 */
	static void RegisterErrorHandler(
			SceMiErrorHandler errorHandler,
			void* context);
	/**
	 @brief This function is used to Register Info Handlers

	 @param infoHandler The user defined info handler function for SCE-MI
	 @param context The user context
	 */
	static void
			RegisterInfoHandler(SceMiInfoHandler infoHandler, void* context);
	/**
	 @brief This function is used to Return the SCE-MI Version Handle

	 @param versionString The requested SCE-MI version string

	 @return \li 100 for SCE-MI version string "1.0.0"
	 \li 110 for SCE-MI version string "1.1.0"
	 \li 200 for SCE-MI version string "2.0.0"
	 \li -1 on failure
	 */
	static int Version(const char *versionString);
protected:
	SceMiC::SceMi *sceMiStruct; /*!< the SCE-MI structure */
};

/**
 @brief C++ Wrapper class for the SceMiMessageData Structure.
 */
class SceMiMessageData {
	friend class SceMiMessageOutPortProxy;
	friend class SceMiMessageInPortProxy;
public:
	/**
	 @brief c'tor

	 @param messageInPortProxy The SceMiMessageInPortProxy object.
	 @param ec The error object.
	 */
	SceMiMessageData(
			const SceMiMessageInPortProxy &messageInPortProxy,
			SceMiEC* ec = 0);
	/**
	 @brief standard d'tor
	 */
	~SceMiMessageData();
	/**
	 Return the width of the message port in bits

	 @return \li On success returns the Width of the message Port in bits.
	 \li 0 on failure
	 */
	unsigned int WidthInBits() const;
	/**
	 @brief Return the width of the message port in words

	 @return \li Returns the width of the message port in words
	 \li 0 on failure
	 */
	unsigned int WidthInWords() const;
	/**
	 @brief Set data for the SceMiMessage Port by word.  For setting
	 i + range must be smaller than the message bit width and range must be
	 smaller than 32

	 @param i word element to be set
	 @param word The value to be set in word
	 @param ec Pointer to a additional error handler
	 */
	void Set(unsigned i, SceMiU32 word, SceMiEC* ec = 0);
	/**
	 @brief Set data for the SceMiMessage Port by bit.  For setting
	 i + range must be smaller than the message bit width and range must be
	 smaller than 32

	 @param i bit element to be set
	 @param bit The value to be set in bit
	 @param ec Pointer to a additional error handle
	 */
	void SetBit(unsigned i, int bit, SceMiEC* ec = 0);
	/**
	 @brief Set data for the SceMiMessage Port by range.  For setting
	 i + range must be smaller than the message bit width and range must be
	 smaller than 32

	 @param i starting pos of the bit range to be set
	 @param range range of bits to be set
	 @param bits The value to be set in bits
	 @param ec Pointer to a additional error handler
	 */
	void SetBitRange(
			unsigned int i,
			unsigned int range,
			SceMiU32 bits,
			SceMiEC* ec = 0);
	/**
	 @brief Get data for the SceMiMessage Port by word.  For range setting
	 i + range must be smaller than the message bit width and range must be
	 smaller than 32

	 @param i word element to get
	 @param ec Pointer to a additional error handler

	 @return \li The word at position i
	 \li 0 on failure
	 */
	SceMiU32 Get(unsigned i, SceMiEC* ec = 0) const;
	/**
	 @brief Get data for the SceMiMessage Port by word, bit or range.  For range setting
	 i + range must be smaller than the message bit width and range must be
	 smaller than 32

	 @param i  bit element to get
	 @param ec Pointer to a additional error handler

	 @return \li The bit at position i
	 \li 0 on failure
	 */
	int GetBit(unsigned i, SceMiEC* ec = 0) const;
	/**
	 @brief Get data for the SceMiMessagePort by range.  For range setting
	 i + range must be smaller than the message bit width and range must be
	 smaller than 32

	 @param i bit range to get
	 @param range range of bits
	 @param ec Pointer to a additional error handler

	 @return \li The word at position i
	 \li 0 on failure
	 */
	SceMiU32
			GetBitRange(unsigned int i, unsigned int range, SceMiEC* ec = 0) const;
	/**
	 @brief Cycle stamping is a feature of SCE-MI standard.  Each output message sent
	 to the software side is stamped with the number of cycles of the 1/1 controlled
	 clock since the end of CReset at the time the message is accepted by the
	 infrastructure.  the returned value is an unsigned 64 bit value.
	 smaller than 32

	 @return \li 64 Bit cycle stamp of the received message from the DceMiMessageOutPort
	 \li 0 on Error
	 */
	SceMiU64 CycleStamp() const;
protected:
	SceMiMessageData(const SceMiC::SceMiMessageData *psSceMiMessageData);

	/**
	 @brief The function to get the C SceMiC::SceMiMessageData structure.

	 @return The C SceMiC::SceMiMessageData structure.
	 */
	SceMiC::SceMiMessageData *getSceMiMessageDataStruct() const;
private:
	bool m_bDestroy; /*!< flag to indicate that the object should be destroyed */
	SceMiC::SceMiMessageData *sceMiMessageData; /*!< pointer to the SceMiC::SceMiMessageData object */
};

/**
 @brief C++ Wrapper class for the SceMiMessageInPortProxy Structure.
 */
class SceMiMessageInPortProxy {
	friend class SceMiMessageData;
public:
	/**
	 @brief c'tor

	 @param sceMiHandle Pointer to the SceMi object.
	 @param transactorName The transactor name.
	 @param portName The port name.
	 @param binding Pointer to the SceMiMessageInPortBinding object.
	 @param ec The error object.
	 */
	SceMiMessageInPortProxy(
			SceMi *sceMiHandle,
			const char *transactorName,
			const char *portName,
			const SceMiMessageInPortBinding *binding = 0,
			SceMiEC* ec = 0);
	/**
	 @brief standard d'tor
	 */
	~SceMiMessageInPortProxy();
	/**
	 @brief Return the transactor name of the binded SceMiMessageInPort from the param
	 file.

	 @return \li The SCE-MI MessageInPortProxy Transactor name
	 \li NULL on error
	 */
	const char* TransactorName() const;
	/**
	 @briefReturn the port name of the binded SceMiMessageInPort from the param
	 file

	 @return \li The SCE-MI MessageInPortProxy Port Name
	 \li NULL on error
	 */
	const char* PortName() const;
	/**
	 @brief Return the port width of the binded SceMiMessageInPort from the param
	 file

	 @return \li The SCE-MI MessageInPortProxy Port Width
	 \li 0 on error
	 */
	unsigned int PortWidth() const;
	/**
	 @brief Function sends used messageDataHandle to the specified message in port proxy.
	 The whole message must be sent with one write data command.

	 @param data The pointer to the SceMiMessageData object.
	 @param ec Pointer to a additional error handler
	 */
	void Send(const SceMiMessageData &data, SceMiEC* ec = 0);
	/**
	 @brief wrapper function for the SceMiC::SceMiMessageInPortProxyReplaceBinding Function.

	 @param binding Pointer to the SceMiMessageInPortBinding object.
	 @param ec Error object.
	 */
	void ReplaceBinding(
			const SceMiMessageInPortBinding* binding = 0,
			SceMiEC* ec = 0);
protected:
	SceMiC::SceMiMessageInPortProxy *sceMiMessageInPortProxy; /*!< pointer to the SceMiC::SceMiMessageInPortProxy structure */
private:
	SceMi *m_sceMiObject; /*!< pointer to the SCE-MI object */
};

/**
 @brief C++ Wrapper class for the SceMiMessageOutPortProxy Structure.
 */
class SceMiMessageOutPortProxy {
public:
	/**
	 @brief c'tor

	 @param sceMiHandle The SceMi Object.
	 @param transactorName The transactor name.
	 @param portName The port name.
	 @param binding Message port binding object.
	 @param ec The error object.
	 */
	SceMiMessageOutPortProxy(
			SceMi *sceMiHandle,
			const char *transactorName,
			const char *portName,
			const SceMiMessageOutPortBinding *binding,
			SceMiEC* ec);
	/**
	 @brief standard d'tor
	 */
	~SceMiMessageOutPortProxy();
	/**
	 @brief Return the transactor name of the binded SceMiMessageOutPort from the param
	 file

	 @return \li The SCE-MI MessageOutPortProxy Transactor name
	 \li NULL on error
	 */
	const char* TransactorName() const;
	/**
	 @brief Return the port name of the binded SceMiMessageOutPort from the param
	 file

	 @return \li The SCE-MI MessageOutPortProxy Port name
	 \li NULL on error
	 \li Non zero indicates an error
	 */
	const char* PortName() const;
	/**
	 @brief Return the port Width of the binded SceMiMessageOutPort from the param
	 file

	 @return \li The SCE-MI MessageOutPortProxy Port Width
	 \li 0 on error
	 */
	unsigned int PortWidth() const;
	/**
	 @brief This function replaces the Out port binding.
	 file

	 @param binding The replacement SceMiMessageOutPortBinding object.
	 @param ec The error object.

	 @return \li The SCE-MI MessageOutPortProxy Port Width
	 \li 0 on error
	 */
	void ReplaceBinding(
			const SceMiMessageOutPortBinding* binding = 0,
			SceMiEC* ec = 0);
	/**
	 @brief Wrapper function for the Receive Call of the Port Proxy.

	 @param context The user's context.
	 @param data The receive data structure.
	 */
	static void SceMiReceiveWrapperFkt(
			void *context,
			const SceMiC::SceMiMessageData *data);
private:
	SceMiC::SceMiMessageOutPortProxy *sceMiMessageOutPortProxy; /*!< pointer to the SceMiC::SceMiMessageOutPortProxy structure */
	SceMiC::S_SceMiMessageOutPortBinding m_BinderInternal; /*!< port binding to an internal C function */
	S_SceMiMessageOutPortBinding m_BinderInternalCpp; /*!< port binding to an internal C++ function */
	SceMi *m_sceMiObject; /*!< pointer to the SceMi object */
};

/**
 @brief C++ Wrapper class for the SceMiParameters Structure.
 */
class SceMiParameters {
	friend class SceMi;
public:
	/**
	 @brief c'tor

	 @param paramsfile The parameter file path.
	 @param ec The error object.
	 */
	SceMiParameters(const char* paramsfile, SceMiEC* ec = 0);
	/**
	 @brief standard d'tor
	 */
	~SceMiParameters();
	/**
	 @brief This function gets the number of objects in the parameter file for a given object kind.

	 @param objectKind Name of the Object.
	 @param ec The error object.

	 @return The no. of objects in the parameter file.
	 */
	unsigned int NumberOfObjects(const char* objectKind, SceMiEC* ec = 0) const;
	/**
	 @brief This function gets the integer value for a given Object Kind and Attribute Name.

	 @param objectKind Name of the Object.
	 @param index Index of the Object in case the Name is not specified.
	 @param attributeName Name of the Attribute.
	 @param ec The Error Object.

	 @return The value of a given Attribute and Object Kind in integer.
	 */
	int AttributeIntegerValue(
			const char* objectKind,
			unsigned int index,
			const char* attributeName,
			SceMiEC* ec = 0) const;
	/**
	 @brief This function gets the string value for a given Object Kind and Attribute Name.

	 @param objectKind Name of the Object.
	 @param index Index of the Object in case the Name is not specified.
	 @param attributeName Name of the Attribute.
	 @param ec The Error Object.

	 @return The value of a given Attribute and Object Kind in the string format.
	 */
	const char* AttributeStringValue(
			const char* objectKind,
			unsigned int index,
			const char* attributeName,
			SceMiEC* ec = 0) const;
	/**
	 @brief This function is used to replace the integer value for a given Object and Attribute name.

	 @param objectKind Name of the Object.
	 @param index Index of the Object in case the Name is not specified.
	 @param attributeName Name of the Attribute.
	 @param value Replacement integer value.
	 @param ec The Error Object.
	 */
	void OverrideAttributeIntegerValue(
			const char* objectKind,
			unsigned int index,
			const char* attributeName,
			int value,
			SceMiEC* ec = 0);
	/**
	 @brief This function is used to replace the string value for a given Object and Attribute name.

	 @param objectKind Name of the Object.
	 @param index Index of the Object in case the Name is not specified.
	 @param attributeName Name of the Attribute.
	 @param value Replacement string value.
	 @param ec The Error Object.
	 */
	void OverrideAttributeStringValue(
			const char* objectKind,
			unsigned int index,
			const char* attributeName,
			const char* value,
			SceMiEC* ec = 0);
protected:
	SceMiC::SceMiParameters *sceMiParameters; /*!< pointer to the SceMiParameters structure */
};
#endif
#ifndef CHIPIT_SCEMI_INTERNAL_DEFINE
#ifndef NO_SCEMI_WRAPPER_INCLUDE
#include "scemi_wrapper.h"
#endif
#endif
#endif
