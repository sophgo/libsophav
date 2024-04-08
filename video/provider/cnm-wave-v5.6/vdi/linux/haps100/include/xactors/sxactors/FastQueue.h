//-*- C++ -*-x
#pragma once

#include "Packet.h"
#include "sxactors/dllexport.h"

/// A Data container which implements a fast queue.
/// Optimized for inserting and dequeing data in memory blocks, that is Packets.
/// Pushes are accomplished at the end of the memory, pops at the beginning
/// Data shifting is amortized over all pop operations.
class DLLEXPORT FastQueue {
public:
  typedef SceMiU32                     FQData_t;
  typedef unsigned                     size_type;

private:
  ///  The data container
  FQData_t       *m_data;
  /// The current size of the allocated buffer
  size_type      m_capacity;
  /// the head index
  size_type      m_head;
  /// the tail index
  size_type      m_tail;
  /// The number of elements in the Q.
  /// We track the size with an integral type to allow thread safer operations
  /// on size() calls;
  size_type      m_size;

public:
  /// Constructs an empty queue
  FastQueue();
  /// Destructor
  ~FastQueue();

  /// Remove all elements from the queue
  void clear ();

  /// Return a count of the number of elements inthe Q
  /// \return the size in elements of the queue
  size_type size() const ;
  /// Test for empty queue
  /// \return true of the queue is empty
  bool empty () const ;

  /// push the content of packet onto the Q
  /// \param p -- the packet to be pushed
  /// \param startFrom -- the first element to take from
  void push (const Packet &p, size_type startFrom = 0);

  /// Copy n elements from the front of the queue to packet
  /// \param p -- the destination packet
  /// \param n -- the number of elements to copy
  void copyFromFront (Packet &p, size_type n) const;

  /// dequeue n elements from the queue
  /// \param n -- number of elements to remove
  void pop (size_type n);

  /// Peek at the first element in the queue
  const FQData_t & front() const ;

  /// Get a pointer to the data head of the queue
  /// \return point to the first element of the data
  FQData_t * getFrontPtr ();

private:
  /// Utility function to compress data to fron the of the queue
  void compress();
};
