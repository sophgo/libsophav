//-*- C++ -*-x
#pragma once

#include "Packet.h"
#include "FastQueue.h"
#include <list>
#include <pthread.h>
#include "sxactors/dllexport.h"
#include "XactorTypes.h"

///  Virtual class for the transport layers.
/// Pure virtual class to allow a common interface of the transport
/// layers (SceMi or UMRBus) to the XactorCore.
///  This class is not intended for use outside of the XactorCore class.
class DLLEXPORT XactorAdapter {
public:

  /// type name for the call-back function
  typedef void (*AdapterCallBack) (void *context);

private:
  /// Callback function for canSend
  AdapterCallBack m_canSendCB;
  /// Callback function for canReceive
  AdapterCallBack m_canRecvCB;

  /// Callback context for canSend
  void *m_canSendCBContext;
  /// Callback context for canReceive
  void *m_canRecvCBContext;

  /// The user's name of the transactor
  // This is dynamically allocated to avoid Windows VC++ warning C4251
  // about STL member data in DLLs.
  std::string* m_name;

  /// data mutex lock for send side
  mutable pthread_mutex_t m_send_mutex ;

  /// data mutex lock for receive side
  mutable pthread_mutex_t m_recv_mutex ;

  /// Signal to wakeup the waiting sender
  mutable pthread_cond_t m_sendCond;

  /// Signal to wakeup the waiting receiver
  mutable pthread_cond_t m_recvCond;

  /// List of all XactorAdapter object; used for debug
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4251)
    //this is private, so will not be accessed outside the DLL
#endif
  static std::list<XactorAdapter const *> s_adapters;
#ifdef _WIN32
#pragma warning(pop)
#endif

protected:
  /// A queue to hold data received from client
  FastQueue   m_recvQ;

  /// flag to indicate debug tracing of data
  bool        m_debug;
  /// flag to indicate debug tracing of lock operations
  bool        m_debugL;

  /// True if the Adapter was opened successfully
  bool m_isopen;

  /// The transactor type
  unsigned int m_type;

  /// Constructor
  XactorAdapter(const std::string & name);

public:
  /// master or slave transactor
  bool m_master;

  /// Destructor
  virtual ~XactorAdapter() ;

private:
  /// Disallow copy and assignement
  XactorAdapter (const XactorAdapter &) ;
  /// Disallow copy and assignement
  XactorAdapter & operator=(const XactorAdapter &);

public:
  /// Accessor for name of xactor
  inline const char *getNameStr() const {
    return m_name->c_str();
  }
  /// Accessor for name of xactor
  inline const std::string & getName() const {
    return *m_name;
  }

  /// indicator that xactor can accept data for sending
  /// \return true  if the adapter can send a packet
  virtual bool   canSend() const = 0;

  /// Accepts pack for sending to client hardware.
  /// \param p -- the packet to send
  /// \return true if the packet was sent.
  virtual bool doSend(const Packet &p) = 0;

  /// Indicates if data is available
  /// \return the number of 32-bit words available from client
  size_t canReceive() const ;

  /// Take maxread words from the receive Q returning them in Packet p.
  /// This operation is atomic either all maxread words are transferred or none
  /// \param p -- the packet where the data is placed
  /// \param maxread -- the number of word to take from the receive Q
  /// \return true if the packet has been populated
  bool doRecv(Packet &p, size_t maxread);

  /// Peek at maxread words from the receive Q returning them in Packet p.
  /// This operation is atomic either all maxread words are copied or none
  /// \param p -- the packet where the data is placed
  /// \param maxread -- the number of word to take from the receive Q
  /// \return true if the packet has been populated
  bool doPeek(Packet &p, size_t maxread) const;

  /// Set or clear the data trace debug
  /// \param val -- the value to set
  /// \return the prvious value
  bool setDebug(bool val);
  /// Set or clear the data mutex lock trace debug
  /// \param val -- the value to set
  /// \return the prvious value
  bool setLDebug(bool val);


  ///  Set a debug mask.
  /// \param mask -- the bit mask
  /// Bit position in the mask are as follows
  /// - 0 -- trace data send receive
  /// - 1 -- trace data lock
  /// \return the old value of the mask
  virtual unsigned setDebug (unsigned mask);
  
  /// place holder for debug call of concrete instances
  virtual void debug() const ;

  /// Set the user callback function and context for can send notifier.
  /// The user cbfunc is called when the canSend state changed from false to true
  /// \param cbfunc -- the user's function to call
  /// \param context -- the user's context, which is part of function call.
  void setSendCallBack(AdapterCallBack cbfunc, void *context);

  /// Set the user callback function and context for can receive notifier.
  /// The user cbfunc is called when new data arrives at the transactor
  /// \param cbfunc -- the user's function to call
  /// \param context -- the user's context, which is part of function call.
  void setRecvCallBack(AdapterCallBack cbfunc, void *context);

  /// Set the mutex lock on send data
  void lockSend() const;
  /// Unset the mutex lock on send data
  void unlockSend() const;
  /// Set the mutex lock on receive data
  void lockRecv() const;
  /// Unset the mutex lock on receive data
  void unlockRecv() const;

  /// Blocks execution until the canSend status changes
  /// This is wrapper on the pthread_cond_wait function
  void send_cond_wait();
  /// Blocks execution until the canSend status changes or a timeout expires.
  /// This is wrapper on  pthread_cond_timedwait function
  /// \param expiration -- absolute expiration time
  /// \return  value from pthread_cond_timedwait
  int  send_cond_timedwait(const struct timespec *expiration);

  /// Blocks execution until the receive status changes
  /// This is wrapper on the pthread_cond_wait function
  void recv_cond_wait();
  /// Blocks execution until the receive status changes or a timeout expires.
  /// This is wrapper on  pthread_cond_timedwait function
  /// \param expiration -- absolute expiration time
  /// \return  value from pthread_cond_timedwait
  int  recv_cond_timedwait(const struct timespec *expiration);

  /// true if the Adapter was opened successfully
  bool isOpen() { return m_isopen; }

  /// Get the transactor type. Types are define in XactorTypes.h
  unsigned int getType() { return m_type; }

  /// Get the transactor type string. Types are define in XactorTypes.h
  const char *getTypeStr() { return XactorTypesGetTypeStr(m_type); }

protected:

  /// Signal to wakeup the waiting sender.
  /// This must called with the SendLock locked
  void signalSendCondition();

  /// Signal to wakeup the waiting receiver.
  /// This must be called with the RecvLock locked
  void signalRecvCondition();


  /// wrapper function to execute the can send call back
  void executeSendCallBack();
  /// wrapper function to execute the can receive call back
  void executeRecvCallBack();

public:
  /// A debug interface which calls the debug on each Xactor Adapter object.
  /// This method may be useful when debugging within a debugger and the
  /// state of all transactor is desired.   E.g.
  /// (gdb) p XactorAdapter::xdebug();
  /// See UmrAdapter::debug and SceMiAdapter::debug methods for
  /// details on the reported information.
  static void xdebug();
};
