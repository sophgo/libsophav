//-*- C++ -*-x
#pragma once

#include "XactorAdapter.h"
#include "umrbus.h"
#include "FastQueue.h"
#include "sxactors/dllexport.h"

///  SceMi Interface -- internal use.
///  This class is not intended for use outside of the XactorCore class.
/// This is a wrapper/adapter class over a pair of SceMi handle which
///  make up a SceMi-based transactor.
class DLLEXPORT SceMiAdapter : public XactorAdapter {
public:
  typedef void (*SceMiAdapterCallBack) (void *context);

private:
  /// SceMi handle for the put side, i.e., data to the client hardware
  void      * m_putHandle;
  /// SceMi handle for the get side, i.e., data from the client hardware
  void      * m_getHandle;

  /// A queue for the send data
  FastQueue  m_sendDataQ;

  /// Use for local error checking, delete after testing complted.
  bool m_inRCB;                   // in receive call back  for DEBUG

  /// Lock on scemi commands, this may not be needed!
  static pthread_mutex_t s_cmd_mutex ;

public:
  ///  Constructor for SceMi based client.
  /// \param name -- symbolic name
  /// \param path -- scemi path, in params file
  SceMiAdapter (const std::string & name, const std::string & path);

  /// Destructor
  ~SceMiAdapter();

  /// Print data on the state of the adapter.
  /// Specially,
  /// - name
  /// - canSend -- canSend status
  /// - SendQ -- number of words in the software buffer
  /// - canRecv --number of words available from client
  /// - RecvQ -- number of words in software receive queue.
  virtual void debug() const;

private:
  /// Disable copy constructor
  SceMiAdapter (const SceMiAdapter &);
  /// Disable copy constructor
  SceMiAdapter & operator=(const SceMiAdapter &);
public:

  /// indicator that the adapater can accept more data, IE software queue size is less than limit
  /// \return true if the adapter can send a packet
  virtual bool   canSend() const;

  /// Accepts pack for sending to client hardware.
  /// \param p -- the packet to send
  /// \return true if the packet was sent.
  virtual bool doSend (const class Packet & p);

private:
  /// Utility function to lock access to the SceMi commands
  void lockCmds (const char *) ;
  /// Utility function to unlock access to the SceMi commands
  void unlockCmds (const char *) ;

  /// Class call-back function; this is the call-back function set as
  /// the scemi can send notify call back
  static void canSendCB (void *pt) ;
  /// Class call-back function; this is the call-back function set as
  /// the scemi can send notify call back
  void canSendCB();

  /// Class call-back function; this is the call-back function set as
  /// the scemi can receive notify call back
  static void canRecvCB (void *pt) ;
  /// Class call-back function; this is the call-back function set as
  /// the scemi can receive notify call back
  void canRecvCB();

  /// A utility function which does nothing.  Useful since scemi call
  /// backs cannot be set to null.
  static void noop (void *pt);

};
