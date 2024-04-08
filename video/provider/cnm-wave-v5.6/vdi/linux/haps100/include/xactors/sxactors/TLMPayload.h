//-*- C++ -*-x
#pragma once

#include "bxactors/xat_scemi.h"

#include "DBuffer.h"
#include <iostream>
#include "sxactors/dllexport.h"

/**
 * @brief Data storage class representing a TLMPayload holding data of
 *        a particular size DATASIZE.
 *
 * The underlying model is a dynamically sized vector of BitT<D>
 * objects which can be used as a data payload.   The implementation
 * is simplier vector of char to allow memcop operations to and from
 * the payload object.
 *
 * @param DATASIZE   The AMBA data width
 * @param USERDSIZE  The AMBA user data width
 *
 */
template <unsigned int DATASIZE, unsigned int USERDSIZE=0>
class DLLEXPORT TLMPayload : public XATType {
private:
  /// The size in DATASIZE words of the payload
  unsigned m_size;
  /// container to store payload data
  DBuffer< char > m_localData;
  /// Alternate to store data as pointer to outside memory to avoid copy
  const char *  m_externalData;

  /// container to store payload byte enable
  DBuffer< char > m_localBE;
  /// Alternate to store data as pointer to outside memory to avoid copy
  const char *  m_externalBE;

  /// container for user data
  DBuffer< char > m_localUD;
  /// optional external pointer for user data
  const char *  m_externalUD;


  /// indicator is payload is to have byte enables
  bool                 m_hasBE;

#ifdef _WIN32
  // This is a workaround for VC++ 2005 in which friend functions
  // seem not to be able to see protected static inherited
  static PutTo* XATType_lookupPutToOverride(const char*cname) { 
    return XATType::lookupPutToOverride(cname);
  }
#endif

public:

  /** 
   * Empty constructor, building n size data
   * @param n -- size of the payload
   * @param hasByteEnables -- true to send byte enables
   */
  TLMPayload(size_t n=0, bool hasByteEnables=false);

  /**
   * Constructor using external memory reference
   * @param n -- size of the payload
   * @param data -- pointer to block of data for payload
   * @param beData -- pointer to block of data for payload
   */
  TLMPayload(size_t n, const char * data,  const char *beData=0);

  /**
   * @brief   Move the data from the msg to this object starting at bit i.
   *
   * Populate the payload object from the msg object data starting at bit off.
   *
   * @param msg -- the message object
   * @param off -- the initial bit offset into the message
   * @return the next bit position in the messag buffer
   */
  unsigned int unpackPayload (const SceMiMessageDataInterface *msg, unsigned int &off) ;

  /**
   * @brief Populates message buffer from object's payload field.
   *
   * A portion of the message buffer is populated from the payload
   * field of the class object.
   *
   * @param msg -- message buffer to be populated
   * @param off -- initial bit offset into the message buffer
   * @return the bit off for the next item in message buffer.
   */
  unsigned int setMessageData (SceMiMessageDataInterface &msg, const unsigned int off=0) const ;

  /**
   * @brief Copy to the payload object from a memory pointer.
   *
   * Copy upto sz bytes of data from src into the payload object.  Not that if the
   * payload is smaller than sz, fewer bytes are copied.  This does
   * not change the size of the payload.
   *
   * Note that the payload data must be address aligned to the native AMBA
   * data bitwidth. Please use TLMRequest::fillPayload if non-aligned data
   * is to be transfered.
   *
   * @param src -- the source pointer for the data
   * @param sz -- the max
   * @return returns the number of bytes copied
   */
  size_t fillPayload (const char *src, size_t sz) ;

  /**
   * @brief Set the payload object from a memory pointer.
   *
   * This method stores the pointer to the original data avoiding a copy of the data into
   * the payload object.  This does not change the size of the payload.
   *
   * @param src -- the source pointer for the data
   */
  void setPayloadData (const char *src) ;

  /**
   * Copies data from the payload to a memory buffer.
   *
   * @param dst -- the destination memory location
   * @param sz -- maximum number of bytes to copy
   * @return the number of bytes copies to dst.
   */
  size_t fillMemory (char *dst, size_t sz, size_t off = 0) const ;

  /**
   * @brief Load the byte enable portion of payload object from a memory pointer.
   *
   * Copy upto sz bytes of data from src into the payload object.  Not that if the
   * payload is smaller than sz, fewer bytes are copied.  This does
   * not change the size of the payload.
   * if byte enables are not expected 0 is returned
   * In contrary to tlm_generic_payload this implementation uses 1 bit to enable one
   * byte while tlm_generic_payload uses 8 bit. This method takes tlm_generic_payload
   * compatible byte enable information and converts it into the actual byte enables.
   *
   * @param src -- the source pointer for the data
   * @param sz -- the max
   * @return returns the number of bytes copied
   */
  size_t fillByteEnables (const char *src, size_t sz) ;

  /**
   * @brief Set the payload object from a memory pointer.
   *
   * This method stores the pointer to the original data avoiding a copy of the data into
   * the payload object. This does not change the size of the payload.
   * In contrary to tlm_generic_payload this implementation uses 1 bit to enable one
   * byte while tlm_generic_payload uses 8 bit. This method relies on src being in the
   * correct, not tlm_generic_payload compatible format. Use fillByteEnables() to
   * pass tlm_generic_payload compatible byte enables.
   *
   * @param src -- the source pointer for the data
   */
  void setByteEnables (const char *src) ;

  /**
   * @brief Copies data from the payload byte enables to a memory buffer.
   *
   * In contrary to tlm_generic_payload this implementation uses 1 bit to enable one
   * byte while tlm_generic_payload uses 8 bit. This method converts the internally
   * used byte enables into tlm_generic_payload compatible format.
   *
   * @param dst -- the destination memory location
   * @param sz -- maximum number of bytes to copy
   * @return the number of bytes copies to dst.
   */
  size_t fillMemoryFromByteEnables (char *dst, size_t sz, size_t off = 0) const ;

  //////////////////////////////////////////////////////////////////////////////////////
  /// Utility member function
  //////////////////////////////////////////////////////////////////////////////////////

  /// returns the number of D-sized words in the payload.
  size_t size() const  ;
  /// returns the number of byte enable bit
  size_t byteEnableBits() const;
  /// returns the number of byte enable bytes
  size_t byteEnableBytes() const;
  /// returns true if the payload size is 0
  bool empty() const ;
  /// clears the data in the payload --setting the size to 0
  void clear() ;
  /// resizes the payload to the sz words each D bits wide
  void resize (size_t sz, bool useByteEnables) ;
  /// return true if the data payload included byte enables
  bool hasByteEnables() const ;

  /// returns the size of the data payload in bytes
  size_t getDataSizeInBytes() const;

  /// returns the size of the BE payload in true (unpacked) bytes
  size_t getBESizeInBytes() const;

  /// returns the size of the BE payload in true (unpacked) bytes
  size_t getUDSizeInBytes() const;

  /// Get a pointer to the payload data at element idx
  char *getDataPtr (size_t idx) ;
  /// Get a pointer to the payload data at element idx
  const char *getDataPtr (size_t idx) const ;

  /// Get a pointer to the byte enable data at element 0
  const char *getBEPtr () const ;

  /// Get a pointer to the byte enable data at element 0
  const char *getUDPtr () const ;

  /// sets the idx word with d
  void setFromBitT(size_t idx, const BitT<DATASIZE> & d) ;
  /**
   * @brief Create a BitT object from the idx position in the payload.
   *
   * Note that an object is returned, and not a reference, since the
   * payload class does not store BitT objects, only their data.
   */
  BitT<DATASIZE> getBitT(size_t idx) const;

  /// \brief sets the byte enable for the idx word with d
  void setBEFromBitT(size_t idx, const BitT<DATASIZE/8> & d) ;

  /**
   * @brief Create a BitT object from the idx position of the payload byte enable
   *
   * Note that an object is returned, and not a reference, since the
   * payload class does not store BitT objects, only their data.
   */
  BitT<DATASIZE/8> getBEBitT(size_t idx) const;

  BitT<USERDSIZE> getUDBitT(size_t idx) const ;
  void setUDFromBitT(size_t idx, const BitT<USERDSIZE> & d) ;

  ///////////////////////////////////////////////////////////////////////////////////
  template<unsigned int D, unsigned int U>
    friend DLLEXPORT std::ostream & operator<< (std::ostream &os, const TLMPayload<D,U> &obj) ;

  virtual std::ostream & getBitString (std::ostream & os) const ;
  virtual std::ostream & getXATType (std::ostream & os) const ;
  virtual unsigned int getBitSize () const ;
  virtual const char * getClassName() const ;
  virtual XATKind getKind() const ;
  virtual unsigned int getMemberCount() const ;
  virtual XATType * getMember (unsigned int idx) ;
  virtual const char * getMemberName (unsigned int idx) const ;

  /// prints the payload to the log file
  void debug () const;
};
