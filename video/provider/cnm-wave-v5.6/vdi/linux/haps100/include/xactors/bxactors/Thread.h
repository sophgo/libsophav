#pragma once

// C++ class surrounding Pthreads
#include <string>
#include <stdexcept>
#include <pthread.h>
#include <signal.h>
#include "sxactors/dllexport.h"

#ifdef _WIN32
#include <Windows.h>
#define _sleep_ Sleep
#else
#include <unistd.h>
#define _sleep_ usleep
#endif

/// Pure virtual Thread Class.  Abstraction of a thread to a C++ object
class DLLEXPORT Thread
{
protected:
  pthread_t      m_threadid;
  bool           m_stop;
  bool           m_running;

protected:
  Thread ()
    : m_stop(false)
    , m_running(false)
    {
#ifndef _WIN32
      m_threadid = (pthread_t) 0;
#endif
    }
public:
  virtual   ~Thread()
    {
    };
protected:
  virtual void main () = 0;
public:

  /// Start the thread based on the specific implementation. Note that
  /// some implementions may start the thread in the construtor
  void start()
  {
    if (m_running) {
      throw std::logic_error("Thread is already running.");
    }
    m_running = true;
    if ( pthread_create( &m_threadid, 0, Thread::startThread, (void*)this ) ) {
      m_running = false;
      throw std::runtime_error("Failed to create thread.");
    }
  }

  /// Set the stop flag to signal the thread to stop.  It is the
  /// caller responsibility to periodically check this flag and ends
  /// it execution
  virtual void stop() {
    m_stop = true ;
  }
  int cancel() {
      return pthread_cancel(m_threadid);
  }
  void testcancel() {
      pthread_testcancel();
  }
#ifndef _WIN32
  /// Send a unix signal to this thread.  See man kill for possible signals.
  /// \param sig -- unix signal
  /// This member function is not supported on windows platforms
  int signal(int sig) {
    m_running = false;
    return pthread_kill( m_threadid, sig );
  }
#endif

  /// Block until this thread has completed. Be sure that you have
  /// stopped the thread.
  void join() {
    /* if (m_running) { */
    /*   pthread_join( m_threadid, 0 ); */
    /*   m_running = false; */
    /* } */
      while (m_running) _sleep_(1);
  }

protected:
  // static member function for pthread_create call
  static void *startThread( void * obj ) {
    Thread *ptr = (Thread *) obj;
    ptr->main();
    return 0;
  }
};
