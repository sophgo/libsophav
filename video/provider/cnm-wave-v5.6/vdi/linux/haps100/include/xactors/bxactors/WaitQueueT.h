#pragma once

// A queue for multhreaded programs, where the process receiving from
// the queue can block until data is available.

#include <sxactors/gettimeofday1.h>
#include <deque>
#include <pthread.h>
#include <errno.h>

/// A templated Queue class which include mutex locks and methods for ease of
/// use in multi-threaded program.  The user does not need to be
/// concerned with lock handling, but only with the blocking nature of
/// the calls.
/// This class is templated on any storable type T
template <typename T>
class WaitQueueT {
 private:
  // Shared state.

  // This is dynamically allocated to avoid Windows VC++ warning C4251
  // about STL member data in DLLs.  The particular danger is the
  // inlined constructor and destructor.
  std::deque<T> *m_queue;
  // Mutex for gaining control of the shared state
  mutable pthread_mutex_t m_lock;
  // Signal to wakeup the waiting receiver
  mutable pthread_cond_t m_cond;

 public:
  /// \brief Constructor
  WaitQueueT ()
    {
      // Initialize the signals
      pthread_mutex_init(&m_lock, NULL);
      pthread_cond_init(&m_cond, NULL);
      m_queue = new std::deque<T>();
    }

  /// \brief  Destructor
  ~WaitQueueT ()
    {
      // Destroy the signals
      pthread_cond_destroy(&m_cond);
      pthread_mutex_destroy(&m_lock);
      if(m_queue)
        delete m_queue;
    }

 private:
  // Copy constructor is disabled.
  WaitQueueT ( const WaitQueueT &);

 public:

  /// Blocking push data into queue.  Pushes data into the tail of the queue; locks are checked as
  /// required.  
  /// \param t -- the data which will be copied into the queue
  void put(const T& t) {
    pthread_mutex_lock(&m_lock);
    m_queue->push_back(t);
    pthread_cond_signal(&m_cond);
    pthread_mutex_unlock(&m_lock);
  }

  /// Blocking get (pop) operation in queue.  This method blocks if there is no message to receive.
  /// This method should NOT be called from a scemi call back as
  /// deadlock will occur.
  /// \return a copy of the data from the head of the queue
  T get () {

    // grab the lock
    pthread_mutex_lock(&m_lock);

    // is there data?
    while (m_queue->empty()) {
      // this wait will release the lock atomically with the wait
      pthread_cond_wait(&m_cond, &m_lock);
      // and will give up back the lock when the wait is over
    }

    T res = m_queue->front();
    m_queue->pop_front();

    // release the lock
    pthread_mutex_unlock(&m_lock);

    return res;
  }

  /// A non-blocking get (pop) operation.
  /// \param t -- object to be populated
  /// \return true if the object has been populated, false otherwise
  bool getNonBlocking (T &t) {
    bool res = false;

    // grab the lock
    pthread_mutex_lock(&m_lock);

    // is the port ready?
    if (!m_queue->empty()) {
      t = m_queue->front();
      m_queue->pop_front();
      res = true;
    }

    // release the lock
    pthread_mutex_unlock(&m_lock);

    return res;
  }

  /// Timeout based get (pop) operation.  This method attempts to retrieve a message from the queue
  /// blocking until the message is received or the timeout expires.
  /// Time is specified as a delta-time relative to current time
  /// \param  t -- object to be populated
  /// \param delta_seconds -- timeout delay in seconds
  /// \param delta_microseconds -- delay in micro-seconds
  /// \return  returns true if the Request has been received,  false if
  /// a timeout occurred.
  bool getTimed (T &t, const time_t & delta_seconds,  const long & delta_microseconds=0) {
    struct timespec ts;
    setTimeout(ts, delta_seconds, delta_microseconds);
    return getTimed (t, &ts);
  }


  ///  Timeout based get (pop) operation.
  /// This method attempts to retrieve a message from the queue
  /// blocking until the message is received or the timeout expires.
  /// Time is specified as a delta-time relative to current time
  /// \param  t -- object to be populated
  /// \param expiration -- absolute time for the timeout
  /// \return  returns true if the Request has been received,  false if
  /// a timeout occurred.
  bool getTimed (T &t, struct timespec &expiration) {
    return getTimed (t, &expiration);
  }

  ///  Timeout based get (pop) operation.
  /// This method attempts to retrieve a message from the queue
  /// blocking until the message is received or the timeout expires.
  /// Time is specified as a delta-time relative to current time
  /// \param  t -- object to be populated
  /// \param expiration -- absolute time for the timeout
  /// \return  returns true if the Request has been received,  false if
  /// a timeout occurred.
  bool getTimed (T &t, struct timespec *expiration) {
    bool res = false;
    int status;

    // grab the lock
    pthread_mutex_lock(&m_lock);

    // is there data?
    while (m_queue->empty()) {
      // this wait will release the lock atomically with the wait
      status = pthread_cond_timedwait(&m_cond, &m_lock, expiration);
      // and will give up back the lock when the wait is over
      if (status == ETIMEDOUT) {
	break;
      }
    }

    // Even if the timer timed out, it's still possible that
    // the ready callback happened at the last moment
    if (!m_queue->empty()) {
      t = m_queue->front();
      m_queue->pop_front();
      res = true;
    }

    // release the lock
    pthread_mutex_unlock(&m_lock);

    return res;
  }

  /// Query method on queue.  Returns true if data is in the queue.
  /// Note that another thread may take the data even if this returns true.
  bool isDataAvailable() {
    bool res = false;

    // grab the lock
    pthread_mutex_lock(&m_lock);

    // is the port ready?
    if (!m_queue->empty()) {
      res = true;
    }

    // release the lock
    pthread_mutex_unlock(&m_lock);

    return res;
  }

  ///  Returns the number of items in the queue.
  /// Note that another thread may take or add to the data in the queue.
  size_t size() const {
    // grab the lock
    pthread_mutex_lock(&m_lock);
    size_t res = m_queue->size();
    // release the lock
    pthread_mutex_unlock(&m_lock);

    return res;
  }

  ///  Blocking wait until data is available.
  /// Blocking until data is available in the queue.
  void waitForData() {
     // grab the lock
    pthread_mutex_lock(&m_lock);

    // is there data?
    while (m_queue->empty()) {
      // this wait will release the lock atomically with the wait
      pthread_cond_wait(&m_cond, &m_lock);
      // and will give up back the lock when the wait is over
    }
    // release the lock
    pthread_mutex_unlock(&m_lock);

  }


  /// Timeout based until data is available in the queue.
  /// \param expiration -- absolute time for the timeout
  /// \return  returns true if the Request has been received,  false if
  /// a timeout occurred.
  bool waitForDataTimed (struct timespec *expiration) {
    bool res = false;
    int status;

    // grab the lock
    pthread_mutex_lock(&m_lock);

    // is there data?
    while (m_queue->empty()) {
      // this wait will release the lock atomically with the wait
      status = pthread_cond_timedwait(&m_cond, &m_lock, expiration);
      // and will give up back the lock when the wait is over
      if (status == ETIMEDOUT) {
	break;
      }
    }

    res = !m_queue->empty();
    // release the lock
    pthread_mutex_unlock(&m_lock);

    return res;
  }

  /// Timeout based wait until data is available in the queue.
  /// \param delta_seconds -- timeout delay in seconds
  /// \param delta_microseconds -- delay in micro-seconds
  /// \return  returns true if the Request has been received,  false if
  /// a timeout occurred.
  bool waitForDataTimed (const time_t & delta_seconds,  const long & delta_microseconds=0) {
    struct timespec ts;
    setTimeout(ts, delta_seconds, delta_microseconds);
    return waitForDataTimed(&ts);
  }

  /// Blocking peek function.
  /// Peek function examines the head of the queue without consuming data
  /// \param obj -- object to be populated
  /// \return true if the object has been populated, false otherwise
  bool peekB (T &obj) const {
    bool res = true;

    // grab the lock
    pthread_mutex_lock(&m_lock);

    // is there data?
    while (m_queue->empty()) {
      // this wait will release the lock atomically with the wait
      pthread_cond_wait(&m_cond, &m_lock);
      // and will give up back the lock when the wait is over
    }

    obj = m_queue->front();

    // release the lock
    pthread_mutex_unlock(&m_lock);

    return res;
  }

  ///  Non-blocking peek function.
  /// Peek function examines the head of the queue without consuming data
  /// \param obj -- object to be populated
  /// \return true if the object has been populated, false otherwise
  bool peekNB (T &obj) const {
    bool res = false;

    // grab the lock
    pthread_mutex_lock(&m_lock);
    if (!m_queue->empty()) {
      obj = m_queue->front() ;
      res = true;
    }

    // release the lock
    pthread_mutex_unlock(&m_lock);

    return res;
  }

  /// Timed peek function.
  /// Peek function examines the head of the queue without consuming data
  /// \param obj -- object to be populated
  /// \param expiration -- absolute time for the timeout (pointer)
  /// \return true if the object has been populated, false otherwise
  bool peekT (T &obj,  struct timespec *expiration) const {
    bool res = false;
    int status;

    // grab the lock
    pthread_mutex_lock(&m_lock);

    // is there data?
    while (m_queue->empty()) {
      // this wait will release the lock atomically with the wait
      status = pthread_cond_timedwait(&m_cond, &m_lock, expiration);
      // and will give up back the lock when the wait is over
      if (status == ETIMEDOUT) {
	break;
      }
    }

    // Even if the timer timed out, it's still possible that
    // the ready callback happened at the last moment
    if (!m_queue->empty()) {
      obj = m_queue->front();
      res = true;
    }

    // release the lock
    pthread_mutex_unlock(&m_lock);

    return res;
  }


  /// Timeout based peek function.
  /// Peek function examines the head of the queue without consuming data
  /// \param obj -- object to be populated
  /// \param expiration -- absolute time for the timeout (reference)
  /// \return true if the object has been populated, false otherwise
  bool peekT (T &obj,  struct timespec &expiration) const {
    return peekT (obj, &expiration);
  }
  ///  Timeout based peek operation.
  /// This method attempts to retrieve a message from the queue
  /// blocking until the message is received or the timeout expires.
  /// Time is specified as a delta-time relative to current time
  /// \param  t -- object to be populated
  /// \param delta_seconds -- timeout delay in seconds
  /// \param delta_microseconds -- delay in micro-seconds
  /// \return  returns true if the Request has been received,  false if
  /// a timeout occurred.
  bool peekT (T &t, const time_t & delta_seconds,  const long & delta_microseconds=0) {
    struct timespec ts;
    setTimeout(ts, delta_seconds, delta_microseconds);
    return peekT (t, &ts);
  }


  /// Utility function to convert delta time to absolute
  static void  setTimeout(struct timespec &ts
                         ,const time_t delta_seconds, const long delta_microseconds) {
    gettimeofday1(&ts);
    long overflow_s  = delta_microseconds / 1000000 ;
    long corrected_us  = delta_microseconds % 1000000 ;

    ts.tv_nsec = ts.tv_nsec + (corrected_us * 1000);
    if (ts.tv_nsec >= 1000000000) {
      ts.tv_nsec = ts.tv_nsec - 1000000000;
      overflow_s ++;
    }
    //Note that on 32-bit Windows, time_t is 64bits, long is 32bits.
    //and timespec is a struct of longs.  This will cause a warning
    //until Y2038 is dealt with (by fixing timespec).
    ts.tv_sec = ts.tv_sec + delta_seconds + overflow_s;
  }
};

