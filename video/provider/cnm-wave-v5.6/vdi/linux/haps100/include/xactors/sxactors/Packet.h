//-*- C++ -*-x
#pragma once

#include "sized_types.h"
#include "SceMiMessageDataInterface.h"
#include <iostream>
#include "sxactors/dllexport.h"
#include "DBuffer.h"

/**
 * @brief A Data container class implementing SceMiMessageDataInterface.
 *
 * This class implements a SceMiMessageDataInterface interface for
 * marshalling data between SceMi and C++ objects.
 */
class DLLEXPORT Packet : public SceMiMessageDataInterface {
public:
  typedef SceMiU32                     PacketData_t;
  typedef unsigned                     size_type;

private:
  /// The main data container a vector of 32-bit data
  DBuffer<SceMiU32>                      m_data;

public:
  /// Simple constructor without data
  Packet();

  /**
   * @brief Initializing contructor, all words are zeroed
   *
   * @param width_in_word -- number of 32-bit words to initialize
   */
  Packet(unsigned int width_in_words);

  /// copy constructor
  Packet(const Packet &p);

  /// Destructor
  ~Packet();

  // Allow access to some vector-like members.

  ///  Returns the number of words in the Packet.
  size_type size() const ;
  /// returns true if the packet size is 0
  bool empty () const ;
  /**
   * @brief Resize the Packet to hold x words of data.
   *
   * @param x -- number of 32-bit words to initialize
   */
  void resize( size_type x ) ;

  /// Add an element to the end
  void push_back( const PacketData_t &x);

  /// Reserve storage in the vector
  void reserve( const size_type n) ;

  /// Clear the data, size is set to 0
  void clear ();

  /// access word at n
  PacketData_t & operator[] (size_type n) ; 
  /// access word at n
  const PacketData_t &  operator[] (size_type n) const ;

  /// equality test
  bool operator== (const Packet &p) const ;
  /// inequality test
  bool operator!= (const Packet &p) const ;

  /**
   * @brief Set the bits from i to i (i+range) with bits within the packet.
   *
   * @param i -- first bit to set
   * @param range -- number of bits following i to be set 
   * @param bits -- its to set
   * @param ec --  SceMi Error Handling class
   */
  virtual void SetBitRange(unsigned int i, unsigned int range, PacketData_t bits, SceMiEC* ec = 0);

  /**
   * @brief Accessor for message data.
   *
   * Set the bits from i to i (i+range) with bits within the packet
   *
   * @param i -- first bit to set
   * @param range -- number of bits following i to be set 
   * @param ec --  SceMi Error Handling class
   * @return 32 bit data word of the bits at i
   */
  PacketData_t GetBitRange(unsigned int i, unsigned int range, SceMiEC* ec = 0) const;

  /**
   * @brief  Memory block set method.
   *
   * This copies data from data to this object starting at bit i, and upto range.
   * This is more efficient for large block provided the data is aligned.
   * that is, i is a multiple of 8
   *
   * @param i -- the first bit to set.
   * @param range --  number of bits following i to be set 
   * @param data -- the data source
   */
  virtual void SetBlockRange( unsigned i, unsigned range, const char *data);

  /**
   * @brief Memory block set method.
   *
   * This copies data from data to this object starting at bit i, and upto range.
   * This is more efficient for large block provided the data is aligned.
   * that is, i is a multiple of 8
   *
   * @param i -- the first bit to set.
   * @param range --  number of bits following i to be set 
   * @param data -- the data source
   */
  virtual void GetBlockRange (unsigned i, unsigned range, char *data) const;

  /// access the byte (8-bit)  at idx
  unsigned char getByte(size_t idx) const ;
  /// set the byte at idx to val
  void setByte(size_t idx, unsigned char val) ;

  /// Overload the put-to operator for packets
  friend std::ostream & operator<< (std::ostream &os, const Packet &p) ;
};
