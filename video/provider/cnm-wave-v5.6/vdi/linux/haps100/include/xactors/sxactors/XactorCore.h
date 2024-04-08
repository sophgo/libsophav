//-*- C++ -*-x
#pragma once

#include <pthread.h>
#include <time.h>
#include <errno.h>

#include "Packet.h"
#include "XactorAdapter.h"
#include "TLMUtilities.h"
#include "sxactors/dllexport.h"

/**
 * @brief Base class for all transactor.
 *
 * This is the base class for the transactor.  It provides a common
 * interface between UMRBus and SceMi transport layers, as well as
 * several common method for any derived classes.  Data
 * transfers are accomplished in generic data of the Packet class.
 */
class DLLEXPORT XactorCore {
public:
  // type definition for call back function
  typedef void (*XactorCallBack) (void *context);

  /**
   * @brief Constructor for UMRBus-based client.
   *
   * @param name -- symbolic name
   * @param device -- umr device number
   * @param bus -- umr bus number
   * @param address -- umr bus address
   */
  XactorCore(bool master, const std::string & name, unsigned int device, unsigned int bus, unsigned int address) ;

  /**
   * @brief Constructor for SceMi based client.
   *
   * @param name -- symbolic name
   * @param path -- scemi path, in params file
   */
  XactorCore (bool master, const std::string & name, const std::string & path);

private:
  // Disallow copy and assignement
  XactorCore (const XactorCore &) ;
  XactorCore & operator=(const XactorCore &);

public:
  /// Destructor
  virtual ~XactorCore();

  /**
   * @brief Indicates if a message can be sent.
   *
   * @return true if the xactor can send a message
   */
  virtual bool canSend () const;

  /**
   * @brief  Indicates how much data is able be be received.
   *
   * Indicates how many 32-bit data words are available within the
   * host side of this transactor.
   *
   * @return the number of 32-bit words which can be received
   */
  virtual size_t canReceive ();
  size_t canReceive () const;

  // these are all thread-safe and can be used intermixed

  /**
   * @brief Blocking send method.
   *
   * This methods blocks until the Packet has been
   * successfully passed to the xactor
   *
   * @param p -- the Packet to be sent
   */
  void send (const Packet &p);

  /**
   * @brief  Non-blocking send.
   *
   * This method sends the Packet if the xactor can accept its.
   *
   * @param p -- the Packet to be sent
   * @return  returns true if the Packet has been sent
   */
  bool sendNB (const Packet &p);

  /**
   * @brief Timed-blocking send.
   *
   * This method tries to send the Packet but will timeout if the
   * packet has not been sent within delta time.
   *
   * @param p -- the Packet to be sent
   * @param seconds -- timeout delay in seconds
   * @param microseconds -- delay in micro-seconds
   * @return  returns true if the Packet has been sent, false if a timeout occurred.
   */
  bool sendT  (const Packet &p, const time_t seconds, const long microseconds=0);

  /**
   * @brief Timed-blocking send.
   *
   * This method tries to send the Packet but will timeout if the
   * packet has not been sent within delta time.
   *
   * @param p -- the Packet to be sent
   * @param expiration -- absolute time for the timeout
   * @return  returns true if the Packet has been sent, false if a timeout occurred.
   */
  bool sendT  (const Packet &p, struct timespec &expiration) ;

  /**
   * @brief Blocking receive.
   *
   * This method blocks until a Packet is received from the xactor.
   * Exactly psize 32-bit words of data will be available in the packet
   *
   * @param p -- the Packet to be received
   * @param psize -- the desired size of the packet.
   */
  void receive   (Packet &p, size_t psize) ;

  /**
   * @brief  Non-Blocking receive.
   *
   * This method checks if a Packet is available from the xactor
   * and returns it if it is.
   * Exactly psize 32-bit words of data will be available in the packet
   *
   * @param p -- the Packet to be received
   * @param psize -- the desired size of the packet.
   */
  bool receiveNB (Packet &p, size_t  psize) ;

  /**
   * @brief Timeout based receive.
   *
   * This method attempts to retrieve a message from the xactor,
   * blocking until the message is received or the timeout expires.
   * Time is specified as an absolute
   * Exactly psize 32-bit words of data will be available in the packet.
   *
   * @param p -- the Packet to be received
   * @param psize -- the desired size of the packet.
   * @param expiration -- absolute time for the timeout
   * @return  returns true if the Packet has been received, false if a timeout occurred.
   */
  bool receiveT  (Packet &p, size_t psize, struct timespec &expiration) ;

  /**
   * @brief Timeout based receive.
   *
   * This method attempts to retrieve a message from the xactor,
   * blocking until the message is received or the timeout expires.
   * Time is specified as a delta-time relative to current time
   * Exactly psize 32-bit words of data will be available in the packet
   *
   * @param p -- the Packet to be received
   * @param psize -- the desired size of the packet.
   * @param seconds -- timeout delay in seconds
   * @param microseconds -- delay in micro-seconds
   * @return  returns true if the Request has been received, false if a timeout occurred.
   */
  bool receiveT  (Packet &p, size_t  psize, unsigned int seconds, unsigned int microseconds=0) ;

  /**
   * @brief  Blocking peek method.
   *
   * Peek at the data avaiable from the transactor without
   * consumption.  This blocking until all words are available.
   * Exactly psize 32-bit words of data will be available in the packet
   *
   * @param p -- the Packet to be received
   * @param psize -- the desired size of the packet.
   */
  bool peek (Packet &p, size_t psize) const;

  /**
   * @brief  Non-blocking peek method.
   *
   * Peek at the data avaiable from the transactor without
   * consumption. This returns immediately
   * Exactly psize 32-bit words of data will be available in the packet
   *
   * @param p -- the Packet to be received
   * @param psize -- the desired size of the packet.
   * @return true if data is available
   */
  bool peekNB (Packet &p, size_t psize) const;

  /**
   * @brief Timed peek function.
   *
   * This method attempts to retrieve a message from the xactor,
   * blocking until the message is received or the timeout expires.
   * Time is specified as an absolute
   * Exactly psize 32-bit words of data will be available in the packet.
   *
   * @param p -- the Packet to be received
   * @param psize -- the desired size of the packet.
   * @param expiration -- absolute time for the timeout
   * @return  returns true if the Packet has been received, false if a timeout occurred.
   */
  bool peekT (Packet &p, size_t psize, struct timespec &expiration) const;

  /**
   * @brief Timed peek function.
   *
   * This method attempts to retrieve a message from the xactor,
   * blocking until the message is received or the timeout expires.
   * Time is specified as a delta-time relative to current time
   * Exactly psize 32-bit words of data will be available in the packet.
   *
   * @param p -- the Packet to be received
   * @param psize -- the desired size of the packet.
   * @param seconds -- timeout delay in seconds
   * @param microseconds -- delay in micro-seconds
   * @return  returns true if the Packet has been received, false if a timeout occurred.
   */
  bool peekT (Packet &p, size_t  psize, unsigned int seconds, unsigned int microseconds=0) const;

  /**
   * @brief Set call-back function when data can be sent by the xactor.
   *
   * Set the call-back notify function to be called when data can be
   * sent by the xactor. This is not required to be be set, and may
   * be set to NULL.
   *
   * @param cbfunc -- a pointer to the call-back function
   * @param context -- user context used by cbfunc when called
   */
  void setCanSendNotifyCallBack   ( XactorCallBack cbfunc, void *context) ;

  /**
   * @brief Set call-back function when data has been received by xactor.
   *
   * Set the call-back notify function to be called when data has
   * been received by the xactor.  Note the call-back function is
   * called whenever one or more new words are avaialable.   Multiple
   * calls may occur if a large packet (e.g. TLMResponse from read)
   * This is not required to be be set, and may be set to NULL.
   *
   * @param cbfunc -- a pointer to the call-back function
   * @param context -- user context used by cbfunc when called
   */
  void setCanRecvNotifyCallBack( XactorCallBack cbfunc, void *context) ;

  /**
   * @brief Enable trace data for the interface.
   *
   * Enable trace mode for the transport adapter -- scemi or umr
   *
   * @param mask -- a bit mask to enable various modes,
   *                 1 -- trace data flow to xactors
   *                 2 -- trace data mutex locks
   *                 4 -- trace interrupt behavior
   *                 8 -- trace command mutex locks
   * @return revious mask status
   */
  unsigned setAdapterDebug(unsigned mask);

  /// \brief return the name as assigned by the user
  const char *getNameStr() const ;
  /// \brief return the name as assigned by the user
  const std::string & getName() const ;

  /**
   * @brief Enable the use of TLM checking in Master and Slave xactors
   *
   * @param fabric -- the underlying bus fabric
   * @param msglevel -- the reporting level for the messages
   */
  void enableTLMChecks (TLMUtilities::E_XactorType fabric, XactorMsgLevelType msglevel = XACTOR_MSG_WARN);

  /**
   * @brief Enable the use of TLM checking in Master and Slave xactors
   *
   * @param msglevel -- the reporting level for the messages
   */
  void enableTLMChecks (XactorMsgLevelType msglevel = XACTOR_MSG_WARN) { enableTLMChecks(m_fabric, msglevel); }

  /// Displays internal state for this xactor
  void debug() const ;
  /// Displays internal state for all xactors
  static void xdebug();

  /// True if the transactor was opened successfully
  bool isOpen() { return m_padapter->isOpen(); }

  /// Get the transactor type. Types are define in XactorTypes.h
  unsigned int getType() { return m_padapter->getType(); }

  /// Get the transactor type string. Types are define in XactorTypes.h
  const char* getTypeStr() { return m_padapter->getTypeStr(); }

  /// Get the transactor protocol type
  TLMUtilities::E_XactorType getFabric() { return m_fabric; }

  /// Check if the transactor is an APB transactor
  bool typeIsAPB() { return XactorTypesIsAPB(m_padapter->getType()); }
  /// Check if the transactor is an AHB transactor
  bool typeIsAHB() { return XactorTypesIsAHB(m_padapter->getType()); }
  /// Check if the transactor is an AXI transactor
  bool typeIsAXI() { return XactorTypesIsAXI(m_padapter->getType()); }
  /// Check if the transactor is an AXI4 transactor
  bool typeIsAXI4() { return XactorTypesIsAXI4(m_padapter->getType()); }
  /// Check if the transactor is an AXI4L transactor
  bool typeIsAXI4L() { return XactorTypesIsAXI4L(m_padapter->getType()); }
  /// Return the last error message
  std::string getLastError() { std::string msg = m_lastError; m_lastError = ""; return msg; }

protected:
  
  /// wrapper function to execute the can send call back
  void executeSendCallBack();
  /// wrapper function to execute the can receive call back
  void executeRecvCallBack();

  /**
   * @brief Accepts pack for sending to client hardware.
   *
   * @param p -- the packet to send
   * @return true if the packet was sent.
   */
  bool doSend    (const Packet &p);

  /**
   * @brief Take psize words from the receive Q returning them in Packet p.
   *
   * This operation is atomic either all maxread words are transferred or none
   *
   * @param p -- the packet where the data is placed
   * @param psize -- the number of word to take from the receive Q
   * @return true if the packet has been populated
   */
  bool doReceive (Packet &p, size_t  psize) ;

  /**
   * @brief Peek at psize words from the receive Q returning them in Packet p.
   *
   * This operation is atomic either all maxread words are copied or none
   *
   * @param p -- the packet where the data is placed
   * @param psize -- the number of word to take from the receive Q
   * @return true if the packet has been populated
   */
  bool doPeek    (Packet &p, size_t  psize) const;

  /**
   * @brief Utility function useful as a call back
   *
   * @param pv -- the context really the class object
   */
  static void signalCanSend(void *pv);
  /// member function to call indicating that data can be sent to the xactor
  void signalCanSend () ;

  /**
   * @brief Utility function useful as a call back
   *
   * @param pv -- the context really the class object
   */
  static void signalCanReceive(void *pv);
  /// member function to call indicating that new data is available from the client
  void signalCanReceive () ;

  /**
   * @brief Utility function to convert a delta time to absolute time
   *
   * The delta time is given 2 part, seconds and micro seconds, where the total time is the 
   * sum of the two parts.
   *
   * @param ts -- timespec update to absolute time
   * @param delta_seconds -- 
   * @param delta_microseconds -- 
   */
   static void  setTimeout(struct timespec &ts, const time_t delta_seconds, const long delta_microseconds);

private:
  /// Callback function for canSend
  XactorCallBack m_sendCallBack;
  /// Callback function for canReceive
  XactorCallBack m_recvCallBack;
  /// Callback context for canSend
  void *m_sendCBContext;
  /// Callback context for canReceive
  void *m_recvCBContext;

protected:
  /// A holder for the adapter class -- either UmrAdapter or SceMiAdapter
  XactorAdapter *m_padapter;
  /// The type of the bus fabric as set by the user, e.g. AHB, AXI, etc.
  TLMUtilities::E_XactorType m_fabric;
  /// The reporting level for error messages
  XactorMsgLevelType m_reportLevel;
  /// Collects error messages
  std::string m_lastError;
  /// master or slave transactor
  bool m_master;
};
