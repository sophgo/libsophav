//-*- C++ -*-x
#pragma once

#include "XactorAdapter.h"
#include "UmrServiceThread.h"
#include "Packet.h"
#include "FastQueue.h"
#include "umrbus.h"
#include <list>

#include <stdio.h>
#include <stdlib.h>
#include "sxactors/dllexport.h"

///  UMRBus Interface -- internal use.
///  This class is not intended for use outside of the XactorCore class.
///  When using the mutex lock, always local local data before the umrcommand lock.
class DLLEXPORT UmrAdapter : public XactorAdapter {
public:

private:
  /// Handle for underlying emr
  UMR_HANDLE  m_hUmr;
  /// pointer to any error messages from umr_bus calls
  char        *m_errmsg ;
  /// flag to indicate debug tracing of interrrupts
  bool        m_debugIR;
  /// flag to indicate debug tracing of locks
  bool        m_debugLU;

  /// The known current number of free words in the hardware inport side
  unsigned    m_freeCredits;
  /// The size of the hardware buffer (set once after reset)
  unsigned    m_inHwBufferSize;
  /// The maximum size of the software deque for send operations
  unsigned    m_sendQSizeLimit;
  /// The number of bits of free credits which are not sent during an interrupt.
  /// This is based on the m_inHwBufferSize set during reset.
  unsigned    m_creditBitOffset; 

  // Statistics
  /// Count of number of sent word
  unsigned long m_sendC;
  /// Count of number of send commands
  unsigned long m_sendE;
  /// Count of number of received word
  unsigned long m_recvC;
  /// Count of number of receive commands
  unsigned long m_recvE;

  /// Software queue to hold data before sending
  FastQueue   m_sendQ;

  /// master or slave transactor
  bool m_master;

  /// mutex lock for umr commands
  static pthread_mutex_t s_umr_cmd_mutex ;
  /// mutex lock for updating service list information
  static pthread_mutex_t s_service_master_mutex ;
  static pthread_mutex_t s_service_slave_mutex ;

  /// Container type for all instantiated umr adapter objects
  typedef std::list<UmrAdapter *> ServiceList;
  /// list of all registered umrs
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4251)
    //this is private, so will not be accessed outside the DLL
#endif
  static ServiceList  s_registeredMasterUmrs;
  static ServiceList  s_registeredSlaveUmrs;
#ifdef _WIN32
#pragma warning(pop)
#endif

public:
  /// umr instruction code to reset
  const static UMRBUS_LONG ResetCmd      = 0x80000000;
  /// umr instuction code to report fifo sizes
  const static UMRBUS_LONG ReportSz      = 0x80000001;
  /// umr instruction code for interrup acknowledge
  const static UMRBUS_LONG IRAckResponse = 0x80000002;
  /// umr instruction code for reporting version information
  const static UMRBUS_LONG ReportVersion = 0x80000003;
  /// length of the umr reset sequence
  const static unsigned    resetSequence = 33;

  /// Constructor
  /// \param name -- name used in error messages and debug
  /// \param device -- umr device
  /// \param bus -- umr bus
  /// \param address -- umr address
  UmrAdapter(bool master, const std::string & name
             ,unsigned int device
             ,unsigned int bus
             ,unsigned int address) ;

  /// Destructor
  ~UmrAdapter();

private:
  /// disabled copy constructor
  UmrAdapter (const UmrAdapter &);
  /// disabled copy constructor
  UmrAdapter & operator=(const UmrAdapter &);
public:

  /// indicator that the adapater can accept more data, IE software queue size is less than limit
  /// \return true if the adapter can send a packet
  virtual bool canSend() const;

  /// Accepts pack for sending to client hardware.
  /// \param p -- the packet to send
  /// \return true if the packet was sent.
  virtual bool doSend (const Packet & p);

  /// Service function for this adapter.
  /// This function is periodically called by the UmrServiceThread. Interrupts from the client are processed,
  /// data is sent and retrieved from the hardware client as requested.
  /// \return 1 if state of object has changes, 0 otherwise
  int service() ;

  /// Print data on the state of the adapter.  
  /// Specially,
  /// - name
  /// - HWB -- size of hardware inport buffer
  /// - canSend -- canSend status
  /// - freeCredits -- available space in the hardare buffer
  /// - SendQ -- number of words in the software buffer
  /// - canRecv --number of words available from client
  /// - RecvQ -- number of words in software receive queue.
  virtual void debug() const;

  ///  Set a debug mask.
  /// \param mask -- the bit mask
  /// Bit position in the mask are as follows
  /// - 0 -- trace data send receive
  /// - 1 -- trace data lock
  /// - 2 -- trace interrupt activity
  /// - 3 -- trance umr lock activity
  /// \return the old value of the mask
  virtual unsigned setDebug(unsigned mask);

  /// reports the current statistic on data sent and received.
  void reportStats() const;

  /// Apply the reset sequence to client hardware and configure this object
  /// \return trus if the sequence has been sent
  bool reset();

private:
  /// Send the interrupt acknowledge code
  /// \return true if the sequence has been sent 
  bool interruptAck();

  /// Sends avaiable data from the software to the client hardware
  /// \param sendIrAck -- indicates if an interrupt acknowledge should be sent as well
  /// \return number of elements sent
  size_t sendCore(bool &sendIrAck);

  /// Utility  function to handle the sending of data to the client 
  /// \param sendIrAck -- indicates if an interrupt acknowledge should be sent as well
  /// \return true if the send call back function should be called
  bool handleSendService(bool &sendirAck);

  /// Utility function to handle interrupt indicating that data is available from client
  /// \param count -- number of data words avaialble from client
  /// \return true if the send call back function should be called
  bool handleHasDataIR(UMRBUS_LONG count);

  /// Utility function to lock access to the umr commands
  void lockUmrCmds (const char *msg) ;
  /// Utility function to unlock access for the umr commands
  void unlockUmrCmds (const char *msg) ;

  /// Utility function to calculate the bit offset for free credit information
  void setCreditBias ();

  /// Check the version of the hardware with the software
  bool checkVersion ();

protected:
  /// register a UMRAdapater object to be part of the servive loop
  /// \param ua -- the umar adapter object
  static void registerUmr (bool master, UmrAdapter *ua) ;

  /// unregister a UMRAdapater object
  /// \param ua -- the umar adapter object
  static void unregisterUmr (bool master, UmrAdapter *ua) ;

public:
  /// static member function which services all registered UMRs
  static int serviceUmrs (bool master) ;


public:
  /// Utility function to scan and print information about the umr bus
  static void scan_umrbus () ;

};


