//-*- C++ -*-x
#pragma once

#include "sized_types.h"
#include "SceMiMessageDataInterface.h"
#include <iostream>
#include <algorithm>
#include "sxactors/dllexport.h"


/// A Dynamic buffer utility class to encapsulate memory allocation.
/// This class manages a dynamic buffer area, by encapsulating memory allocation,
/// deallocation, clearing, and resizing.  This utility class is used in the
/// Packet and TLMPayload classes.
template < class T>
class DLLEXPORT DBuffer {
public:
  typedef unsigned int                  size_type;

private:
  /// The main data container an array of T data
  T                                       * m_data;
  /// Size of the allocated area in T sized objects
  size_type                               m_capacity;
  /// Size of data used in the array in T sized objects
  size_type                               m_size;

public:

  /// Constructor 
  /// \param initSize -- the initial size of the buffer in T words
  DBuffer(size_type initSize = 0)
    : m_data(0)
    , m_capacity(initSize)
    , m_size(initSize)
  {
    if (m_capacity) {
      m_data = new T [m_capacity];
      memset(m_data, 0, byteCapacity() );
    }
  }

  /// Copy constructor
  DBuffer (const DBuffer & b)
    : m_data(0)
    , m_capacity(b.capacity())
    , m_size(b.size())
  {
    if (m_capacity) {
      m_data = new T [m_capacity];
      memcpy (m_data, &b[0], byteSize());
    }
  }

  /// Destructor
  ~DBuffer() {
    if (m_capacity != 0) {
      delete [] m_data;
    }
  }

  /// Assignment operator
  DBuffer & operator= (const DBuffer & x) {
    // avoid self copies
    if (x.m_data != m_data) {
      resize(0);
      resize(x.size());
      memcpy (m_data, &(x[0]), byteSize() ) ;
    }
    return *this;
  }

  /// Accessor to return the number of T words in the buffer
  /// The size of the buffer in T sized words
  size_type size() const { return m_size; }
  /// The size of the buffer in bytes
  size_type byteSize() const { return m_size * sizeof(T);}
  /// Returns true if the size if size is 0
  bool  empty () const { return m_size == 0 ; }

  /// return the capacity of the buffer in T sized words, i.e., the allocated space
  size_type capacity () const { return m_capacity; }
  /// return the capacity of the buffer in bytes, i.e., the allocated space
  size_type byteCapacity () const { return m_capacity * sizeof(T); }


  /// resize the buffer to hold at least n words
  /// \param n -- the new size of the buffer
  void resize (size_type n) {
    if (m_capacity == 0 && n != 0) {
      m_capacity = n;
      m_data = new T [m_capacity];
      memset(m_data, 0, byteCapacity() );
    }
    else if (n > m_capacity) {
      // reallocate
      m_capacity = (std::max) (m_capacity * 2, n);

      T * newData = new T [m_capacity];
      memcpy ( newData, m_data, byteSize() ) ;
      memset ( byteSize() + (char *) newData, 0, byteCapacity() - byteSize());
      delete [] m_data;
      m_data = newData;
    }
    else if (n > m_size) {
      // Initial out new area
      memset ( byteSize() + (char *) m_data, 0, byteCapacity() - byteSize());
    }
    m_size = n;
  }

  /// Clear the contents by setting the size to 0.
  void clear () { resize(0); }

  /// Reserve storage in the buffer
  /// \param n -- the number of words to reserve
  void reserve (const size_type n) {
    if (m_capacity == 0 && n != 0) {
      m_capacity = n;
      m_data = new T [m_capacity];
      memset(m_data, 0, byteCapacity() );
    }
    else if (n > m_capacity) {
      // reallocate
      m_capacity = (std::max) (m_capacity * 2, n);

      T * newData = new T [m_capacity];
      memcpy ( newData, m_data, byteSize() ) ;
      delete [] m_data;
      m_data = newData;
    }
  }


  /// Element accessor
  T & operator[] (size_type n) { return m_data[n] ; }
  /// Element accessor, const version
  const T & operator[] (size_type n) const { return m_data[n] ; }

  /// Equality operator
  bool operator== (const DBuffer<T> &p) const {
    if (m_size == p.size()) {
      int stat = memcmp (m_data, p.m_data, byteSize() ) ;
      return stat == 0;
    }
    return false;
  }

  /// Inequality operator
  bool operator!= (const DBuffer<T> &p) const {
    return ! (*this == p);
  }

  /// Add an element to the end of the block
  /// \param x -- element to add
  void push_back (const T &x) {
    reserve(m_size + 1);
    m_data[m_size] = x;
    m_size += 1;
  }

};
