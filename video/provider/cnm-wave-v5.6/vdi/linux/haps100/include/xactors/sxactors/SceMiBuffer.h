//-*- C++ -*-x
#pragma once

#include "SceMiMessageDataInterface.h"
#include "sized_types.h"

/// Implementation of SceMiMessageDataInterface for SceMi.
/// This class implements a SceMiMessageDataInterface interface for
/// marshalling data between SceMi and C++ objects
class SceMiBuffer : public SceMiMessageDataInterface {
public:
  std::vector<SceMiU32> m_buffer;


  SceMiBuffer(unsigned int width_in_bits)
    : m_buffer(bits_to_words(width_in_bits))
  {
  }

  ///  Set the bits from i to i (i+range) with bits within the packet.
  /// \param i -- first bit to set
  /// \param range -- number of bits following i to be set 
  /// \param bits -- its to set
  /// \param ec --  SceMi Error Handling class
  void SetBitRange(unsigned int i, unsigned int range, SceMiU32 bits, SceMiEC* ec = 0){
    uncheckedSetBitRange(i,range,bits,&m_buffer[0]);
  }


  ///  Accessor for message data.
  ///  Set the bits from i to i (i+range) with bits within the packet
  /// \param i -- first bit to se
  /// \param range -- number of bits following i to be set 
  /// \param ec --  SceMi Error Handling class
  /// \return 32 bit data word of the bits at i
  SceMiU32 GetBitRange(unsigned int i, unsigned int range, SceMiEC* ec = 0) const {
    return uncheckedGetBitRange(i,range,&m_buffer[0]);
  }
};

