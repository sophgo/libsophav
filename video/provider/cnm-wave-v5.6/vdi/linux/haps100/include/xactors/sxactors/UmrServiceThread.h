//-*- C++ -*-x
#pragma once

#include "bxactors/Thread.h"
#include "sxactors/dllexport.h"

///  UMR Service thread
/// The UMRBus servioce thread which works in conjunction with the
/// UMRAdapater class and the Xactor classes.
/// This class follows the singleton pattern
class DLLEXPORT UmrServiceThread : public Thread {
public:
  /// typedef of the call back function
  typedef void (*UmrServiceCallBack) (void *context);

public:

  /// Singleton constructor -- public access
  static void createThreadIfNeeded(bool master);
  static void deleteThreadIfNeeded(bool master);

private:
  /// Constructor.
  /// Constructor for UMRService thread
  /// start() method must be called after construction
  /// \param cbfunc -- optional call-back function to be called after
  /// each iteration of the service loop
  /// \param context -- options context for the call-back fuction
  UmrServiceThread(bool master, UmrServiceCallBack cbfunc=0, void * context=0);

  ///  Destructor.
  /// Destructor calls stop() and join() on the thread.   Hence it may
  /// block.
  virtual ~UmrServiceThread();

  /// disable implicit copy constructor
  UmrServiceThread( const UmrServiceThread &);
  /// disable implicit copy constructor
  UmrServiceThread & operator= (const UmrServiceThread &);


  /// Private access to get the service thread instance,  a thread may be created during this call.
  //  static UmrServiceThread * getInstance();

  static pthread_mutex_t    s_master_mutex;
  static pthread_mutex_t    s_slave_mutex;
  static unsigned int       s_master_count;
  static unsigned int       s_slave_count;

protected:

  /// The main function for the service thread.
  /// This loops over all registered UMR Adapter objects and calls the service method.
  void main() ;

private:

  /// an optional call back function which is called after each service call
  UmrServiceCallBack m_cbfunc;
  /// the contenxt for the optional service call
  void             * m_context;
  bool               m_master;
  /// the singleton instances.
  static UmrServiceThread *s_master_instance;
  static UmrServiceThread *s_slave_instance;


};
