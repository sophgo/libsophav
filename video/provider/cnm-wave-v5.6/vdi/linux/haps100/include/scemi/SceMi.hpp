/******************************************************************************
  Copyright © 2009 Synopsys, Inc. This Example and the associated documentation
  are confidential and proprietary to Synopsys, Inc. Your use or disclosure of
  this example is subject to the terms and conditions of a written license
  agreement between you, or your company, and Synopsys, Inc.
*******************************************************************************
   Title      : SCE-MI Primary Routines
   Project    : SCE-MI 2 Implementation
******************************************************************************/
/**
 * @file   SceMi.hpp
 * @author  Roshan Antony Tauro
 * @date   2006/10/17
 *
 * @brief  The Main Header File for C++ Sources
 *
 * This is the extended description
 *
 */
/******************************************************************************
           $Id: //chipit/chipit/main/release/tools/scemi2/sw/include/SceMi.hpp#1 $ $Author: mlange $ $DateTime: 2014/06/05 03:13:44 $

          $Log: SceMi.hpp,v $
          Revision 1.5  2009/02/20 13:20:03  rtauro
          Added copyright info.

          Revision 1.4  2008/12/16 15:52:00  rtauro
          Fixed documentation.

          Revision 1.3  2008/12/16 13:59:24  rtauro
          Added Documentation.

          Revision 1.2  2007/11/05 15:08:08  rtauro
          First Revision



******************************************************************************/

#ifndef __Scemi_hpp__
#define __Scemi_hpp__

#ifndef SCEMICPP_INTERFACE
#define SCEMICPP_INTERFACE
#include "scemi.h"
#endif

#include <iostream>

class SceMi;
class SceMiMessageData;
class SceMiMessageInPortProxy;
class SceMiMessageOutPortProxy;
class SceMiParameters;

/**
* @brief Structure represents a MessageOutPortProxy Binding for C++
*/
typedef struct S_SceMiMessageOutPortBinding
{
	void  *Context; /*!< context passed to the call-back functions by the user */
	void (*Receive)(void *context, const SceMiMessageData *data);
	void (*Close)(void *context); /*!< function called when message is received from the MessageOutPort */
}	SceMiMessageOutPortBinding;  /*!< function called to close the connection */

/**
   @brief C++ Wrapper class for the SceMi Structure.
*/
class SceMi
{
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

		@param version	The return value from SceMiVersion call (a version handle).
		@param parameters Pointer to the created SceMiParameters object.
		@param ec Pointer to a additional error handler.

		Action flow:
		\li Init the library internal data.
		\li Open all UMRBus communication ports to the hardware interface.
		\li Save all internal used and needed data to the SceMi object.
		\li Set the softReset to high over SCE-MI time reset control CAPIM (Wait until the reset is really set at hardware side).
		\li Clear the interrupt fifo ? read all interrupts over SCE-MI time reset control CAPIM and chuck away this data.
		\li Clear the read fifos ? read fifo size elements over SCE-MI data CAPIM and chunk away this data.
		\li Set the softReset to low over SCE-MI time reset control CAPIM (wait until the reset is really set at hardware side).
		\li Start the ServiceLoopThread.
		\li Return the object pointer.

		@return \li Pointer to the Initialized SCE-MI Object on succes.
				\li NULL Pointer on Failure.
	*/
	static SceMi* Init(int version, const SceMiParameters* parameters, SceMiEC* ec = 0);
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

		@return	\li Returns the pointer to the SceMiMessageOutPortProxy Structure
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
	int ServiceLoop(
			SceMiServiceLoopHandler g,
			void* context = 0,
			SceMiEC* ec = 0);
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
	static void	RegisterInfoHandler(
			SceMiInfoHandler infoHandler,
			void* context);
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
class SceMiMessageData
{
	friend class SceMiMessageOutPortProxy;
	friend class SceMiMessageInPortProxy;
public:
	/**
		@brief c'tor

		@param messageInPortProxy The SceMiMessageInPortProxy object.
		@param ec The error object.
	*/
	SceMiMessageData(const SceMiMessageInPortProxy &messageInPortProxy, SceMiEC* ec = 0);
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

		@return \li Returns the widht of the message port in words
				\li 0 on failure
	*/
	unsigned int WidthInWords() const;
	/**
		@brief Set data for the SceMiMessage Port by word.  For setting
		i + range must be smaller than the message bit width and range must be
		smaller than 32

		@param i word element to be set
		@param word	The value to be set in word
		@param ec Pointer to a additional error handler
	*/
	void Set( unsigned i, SceMiU32 word, SceMiEC* ec = 0);
	/**
		@brief Set data for the SceMiMessage Port by bit.  For setting
		i + range must be smaller than the message bit width and range must be
		smaller than 32

		@param i bit element to be set
		@param bit The value to be set in bit
		@param ec Pointer to a additional error handle
	*/
	void SetBit( unsigned i, int bit, SceMiEC* ec = 0);
	/**
		@brief Set data for the SceMiMessage Port by range.  For setting
		i + range must be smaller than the message bit width and range must be
		smaller than 32

		@param i starting pos of the bit range to be set
		@param range range of bits to be set
		@param bits	The value to be set in bits
		@param ec Pointer to a additional error handler
	*/
	void SetBitRange(unsigned int i, unsigned int range, SceMiU32 bits, SceMiEC* ec = 0);
	/**
		@brief Get data for the SceMiMessage Port by word.  For ange setting
		i + range must be smaller than the message bit width and range must be
		smaller than 32

		@param i word element to get
		@param ec Pointer to a additional error handler

		@return \li The word at position i
		 		\li 0 on failure
	*/
	SceMiU32 Get( unsigned i, SceMiEC* ec = 0) const;
	/**
		@brief Get data for the SceMiMessage Port by word, bit or range.  For ange setting
		i + range must be smaller than the message bit width and range must be
		smaller than 32

		@param i	bit element to get
		@param ec	Pointer to a additional error handler

		@return \li The bit at position i
				\li 0 on failure
	*/
	int GetBit( unsigned i, SceMiEC* ec = 0) const;
	/**
		@brief Get data for the SceMiMessagePort by range.  For ange setting
	 	i + range must be smaller than the message bit width and range must be
		smaller than 32

		@param i bit range to get
		@param range range of bits
		@param ec Pointer to a additional error handler

		@return \li The word at position i
		 		\li 0 on failure
	*/
	SceMiU32 GetBitRange(unsigned int i, unsigned int range, SceMiEC* ec = 0) const;
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
	bool m_bDestroy; /*!< flag to indicate that the object shuold be destroyed */
	SceMiC::SceMiMessageData *sceMiMessageData; /*!< pointer to the SceMiC::SceMiMessageData object */
};

/**
   @brief C++ Wrapper class for the SceMiMessageInPortProxy Structure.
*/
class SceMiMessageInPortProxy
{
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

		@param data	The pointer to the SceMiMessageData object.
		@param ec	Pointer to a additional error handler
	*/
	void Send(const SceMiMessageData &data, SceMiEC* ec = 0);
	/**
		@brief wrapper function for the SceMiC::SceMiMessageInPortProxyReplaceBinding Function.

		@param binding Pointer to the SceMiMessageInPortBinding object.
		@param ec Error object.
	*/
	void ReplaceBinding(const SceMiMessageInPortBinding* binding = 0, SceMiEC* ec = 0);
protected:
	SceMiC::SceMiMessageInPortProxy *sceMiMessageInPortProxy; /*!< pointer to the SceMiC::SceMiMessageInPortProxy structure */
private:
	SceMi *m_sceMiObject; /*!< pointer to the SCE-MI object */
};

/**
   @brief C++ Wrapper class for the SceMiMessageOutPortProxy Structure.
*/
class SceMiMessageOutPortProxy
{
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

		@return	\li The SCE-MI MessageOutPortProxy Transactor name
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
	void ReplaceBinding(const SceMiMessageOutPortBinding* binding = 0, SceMiEC* ec = 0);
	/**
		@brief Wrapper function for the Receive Call of the Port Proxy.

		@param context The user's context.
		@param data The receive data structure.
	*/
	static void SceMiReceiveWrapperFkt(void *context, const SceMiC::SceMiMessageData *data);
private:
	SceMiC::SceMiMessageOutPortProxy *sceMiMessageOutPortProxy; /*!< pointer to the SceMiC::SceMiMessageOutPortProxy structure */
	SceMiC::S_SceMiMessageOutPortBinding m_BinderInternal;  /*!< port binding to an internal C function */
	S_SceMiMessageOutPortBinding m_BinderInternalCpp;  /*!< port binding to an internal C++ function */
	SceMi *m_sceMiObject;  /*!< pointer to the SceMi object */
};

/**
   @brief C++ Wrapper class for the SceMiParameters Structure.
*/
class SceMiParameters
{
	friend class SceMi;
public:
	/**
		@brief c'tor

		@param paramsfile The parameter file path.
		@param ec The error object.
	*/
	SceMiParameters(
			const char* paramsfile,
			SceMiEC* ec = 0);
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
	unsigned int NumberOfObjects(
			const char* objectKind,
			SceMiEC* ec = 0) const;
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
	SceMiC::SceMiParameters *sceMiParameters;  /*!< pointer to the SceMiParameters structure */
};

#endif
