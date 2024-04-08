//-*- C++ -*-x
#pragma once

#include "scemi.h"
#include "sxactors/dllexport.h"

/// Pure virtual class which abstracts the marshalling of data.
/// This class is a pure virtual class which abstracts the marshalling
/// of data between C object and the SceMi interface.  Based on
/// SceMi's SceMiMessageData class specification.
class DLLEXPORT SceMiMessageDataInterface {
public:
  /// Destructor
  virtual ~SceMiMessageDataInterface() {} ;

  /// set the bits  from i to i (i+range) with bits
  /// param i -- the lsb bit of the data
  /// param range -- the number of bit after i,  data size is range + 1
  /// param bits -- the data to assign to the message
  /// param ec -- optional scemi error handler
  virtual void SetBitRange(unsigned int i, unsigned int range, SceMiU32 bits, SceMiEC* ec = 0) =0;

  /// return upto 32 bits from the message from bit i to (i+range)
  /// param i -- the lsb bit of the data
  /// param range -- the number of bit after i,  data size is range + 1
  /// param ec -- optional scemi error handler
  /// \return data at bit i;
  virtual SceMiU32 GetBitRange(unsigned int i, unsigned int range, SceMiEC* ec = 0) const =0;

  /// Set of range of data from bit i to i+range copying from data.
  /// param i -- the lsb bit of the data
  /// param range -- the number of bit after i,  data size is range + 1
  /// param data -- memory location where data is copied from
  virtual void SetBlockRange( unsigned i, unsigned range, const char *data) = 0;

  /// Get range of data from bit i to i+range copying to memory at data
  /// param i -- the lsb bit of the data
  /// param range -- the number of bit after i,  data size is range + 1
  /// param data -- memory location where data is copied in to
  virtual void GetBlockRange (unsigned i, unsigned range, char *data) const  = 0;
protected:

  /// param i -- the lsb bit of the data
  /// param range -- the number of bit after i,  data size is range + 1
  /// param bits -- the data to assign to the message
  /// param buffer -- buffer where data is set
  static void uncheckedSetBitRange(unsigned int i,
				   unsigned int range,
				   SceMiU32 bits,
                                   SceMiU32* buffer);

  /// param i -- the lsb bit of the data
  /// param range -- the number of bit after i,  data size is range + 1
  /// param buffer -- buffer where data is accessed
  static SceMiU32 uncheckedGetBitRange(unsigned int i, unsigned int range, const SceMiU32* buffer);
};

