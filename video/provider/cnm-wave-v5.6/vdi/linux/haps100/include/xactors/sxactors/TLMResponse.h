//-*- C++ -*-x
// C++ Class with SceMi Message passing for  TLM3Api::TLMRequest

#pragma once

#include "bxactors/xat_scemi.h"
#include "sxactors/sized_types.h"
#include "gxactors/TLMResponseParams.h"
#include "TLMPayload.h"
#include <vector>

/**
 * @brief Data class representing a TLMResponse.
 *
 * This class represents a TLMResponse.  A request consists of 3
 * fields, the header, the burst lenght and the payload.
 * This class is templated on data width, which must match
 * the widths of the underlying hardware xactor.
 * Valid data widths are 8, 16, 32, 64, 128, 256, 512, and 1024.
 * Valid USERDSIZE are: 0, 32, and 64.
 *
 * @param ADDRSIZE   The AMBA address width
 * @param DATASIZE   The AMBA data width
 * @param USERDSIZE  The AMBA user data width
 */
template <unsigned int DATASIZE=32, unsigned int USERDSIZE=0>
class TLMResponse : public XATType {
public:
  /// The response header
  TLMResponseParams m_header;
  /// user data
  BitT<USERDSIZE> m_userdata;
  /// The response payload
  mutable TLMPayload<DATASIZE,USERDSIZE>  m_payload;
  /// The burst lenght for the reponse
  unsigned m_b_length;

  /// \brief Empty constructor
  TLMResponse ()
    : m_header()
    , m_userdata()
    , m_payload()
    , m_b_length(1)
  {}

  /**
   * @brief Constructor from Message stream.
   *
   * A TLMResponse object is constructed from the message buffer
   * with data taken from the bit offset.
   *
   * @param msg -- pointer to message buffer class
   * @param off -- initial bit offset into the message buffer (modified by call)
   */
  TLMResponse ( const SceMiMessageDataInterface *msg, unsigned int &off )
    : m_header()
    , m_userdata()
    , m_payload()
    , m_b_length(1)
  {
    m_header = TLMResponseParams(msg, off);
    TLMBurstLong blength(msg, off);
    m_b_length = 1 + (unsigned) blength;
    off = next32(off);

    // Only write responses have user data
    if (m_header.m_command == TLMCommand::e_WRITE) {
      m_userdata = BitT<USERDSIZE> (msg, off);
    }

    unpackPayload(msg, off);
    off = next32(off);
  }

  /**
   * @brief Piece-wise constructor for payload from message buffer.
   *
   * This method unpacks the message data populating the payload
   * field. It is not expected that will be be called 
   * outside of the class.
   *
   * @param msg -- pointer to message buffer class
   * @param off -- initial bit offset into the message buffer (modified by call)
   * @return the bit off for the next item in message buffer.
   */
  void unpackPayload(const SceMiMessageDataInterface *msg, unsigned int &off ){
    // insure that payload is sized correctly
    setPayload();

    m_payload.unpackPayload(msg, off);
  }

  /**
   * @brief  Populates message buffer from object.
   *
   * A portion of the message buffer is populated from the contents
   * of the class object.
   *
   * @param msg -- message buffer to be populated
   * @param off -- initial bit offset into the message buffer
   * @return the bit off for the next item in message buffer.
   */
  unsigned int setMessageData (SceMiMessageDataInterface &msg, const unsigned int off=0) const {
    setPayload();
    unsigned int running = off;

    TLMBurstLong blength = (int) (m_b_length-1);

    running = m_header.setMessageData( msg, running );
    running = blength.setMessageData (msg, running);

    running = next32(running);

    // Only write response have user data
    if (m_header.m_command == TLMCommand::e_WRITE) {
      running = m_userdata.setMessageData(msg,running);
    }

    running = m_payload.setMessageData(msg, running );
    return next32(running);
  }

    /**
     * @brief Copy data into the response's payload object.
     *
     * This method takes into account the address of the request which needs
     * to be passed as parameter. In case of an unaligned response, the necessary
     * padding bytes will be added to the front of the payload. These padding
     * bytes will be disabled by assigning approppriate byte enables. As a
     * consequence, the m_b_length parameter will be modified to account for
     * the additional bytes.
     * Using this method it is possible to assemble requests of any length and
     * target address.
     * 
     * @param addr   -- The target address
     * @param src_pl -- The source pointer for the payload data
     * @param sz_pl  -- The number of payload data bytes
     * 
     * @return The number of bytes copied.
     */
    size_t fillPayload (long long addr, const char *src_pl, size_t sz_pl)
    {
        size_t retval=0;
        unsigned int zeros = log((double)DATASIZE/8)/log((double)2);
        long long mask = ~((-1LL) << zeros);
        if ((mask & addr) != 0 || (sz_pl % (DATASIZE/8)) != 0) {
            // if not aligned we need to assemble a valid payload
            unsigned int padcnt = (mask & addr & 0xFF);
            
            // calculate new length
            m_b_length = 1 + ((sz_pl + padcnt - 1) / (DATASIZE/8));
            
            unsigned int i;
            char *dst_pl = (char*) calloc(m_b_length+1, DATASIZE/8);
            if(!dst_pl) return 0;

            // pad data bytes at the lower addresses
            for (i = 0; i < padcnt; i++) {
                dst_pl[i] = 0;
            }
            memcpy(dst_pl+padcnt, src_pl, sz_pl);
            
            // fill payload in TLMPayload            
            retval = m_payload.fillPayload(dst_pl, m_b_length * DATASIZE/8);
            
            free(dst_pl);
        } else {
            retval = m_payload.fillPayload(src_pl, sz_pl);
        }
        return retval;
    }

    /** 
     * @brief Fill the memory with the data contained in the payload.
     *
     * This method takes into account the address of the request which needs
     * to be passed as parameter. In case of an unaligned request, the memory
     * will be filled starting from the offset.
     *
     * @param addr   -- The target address
     * @param dst_pl -- The destination pointer for payload data
     * @param sz_pl  -- The size of the destination buffer
     * @param dst_be -- The byte enable pointer
     * @param sz_be  -- The number of byte enable bytes
     * 
     * @return The number of bytes copied
     */
    size_t fillMemory(long long addr, char *dst_pl, size_t sz_pl, char *dst_be = NULL, size_t sz_be = 0) {
        size_t      retval = 0;
        unsigned int zeros = log((double)DATASIZE/8)/log((double)2);
        long long     mask = ~((-1LL) << zeros);
        if ((mask & addr) != 0) {
            // if not aligned we need to extract data from payload
            unsigned int padcnt = (mask & addr & 0xFF);

            // TODO: byte enables ? Lets fake them ...
            char *src_be = (char*) malloc((m_b_length+1) * DATASIZE/8);
            memset(src_be, 0xFF, (m_b_length+1) * DATASIZE/8);
            if (dst_be && sz_be) {
                memcpy(dst_be, src_be, stdmin(m_b_length * DATASIZE/8, sz_be));
            }

            retval = m_payload.fillMemory(dst_pl, sz_pl, padcnt);
            if(src_be) free(src_be);
        } else {
            m_payload.fillMemoryFromByteEnables(dst_be, sz_be);
            retval = m_payload.fillMemory(dst_pl, sz_pl);
        }
        return retval;
    }

  /**
   * @brief Get the payload size in words.
   *
   * Returns the size of the payload, in "datasize" words for the
   * TLMRequest based on the command and burst length set in the header.
   *
   * @return size of data words for the request
   */
  unsigned int getPayloadSize() const {
    unsigned int burstl = m_b_length ;
    return (m_header.m_command == TLMCommand::e_READ) ? (burstl) : 0;
  }

  /**
   * @brief Resizes the payload to match the header fields.
   *
   * Resizes the payload field according to the command and burst
   * lenght information in the header field.   Payload data may be
   * dropped; new data is set to 0.
   */
  void setPayload() const {
    // insure that payload is sized correctly
    m_payload.resize(getPayloadSize(), false);
  }

  /// PutTo operation, tcl friendly
  friend std::ostream & operator<< (std::ostream &os, const TLMResponse &obj) {
    // insure that payload is sized correctly
    obj.setPayload();

    XATType::PutTo * override = lookupPutToOverride ( obj.getClassName() );
    if ( override != 0 ) {
       return override(os, obj );
    }
    os << "{" ;
    os << "header " << obj.m_header ;os << " " ;
    os << "b_length " << std::dec << obj.m_b_length; os << " ";

    if ( (USERDSIZE != 0) && (obj.m_header.m_command == TLMCommand::e_WRITE)) {
        os << "userdata " << obj.m_userdata << " " ;
    }
    os << "payload " << obj.m_payload ;
    os << "}" ;
    return os;
  }

  /// return a portion of the data
  virtual std::ostream & getBitString (std::ostream & os) const {

    // insure that payload is sized correctly
    setPayload();
    std::string pad;
    TLMBurstLong blength = (int) (m_b_length - 1);

    // Payload to header -- this is backwards...
    m_payload.getBitString(os);

    if (m_header.m_command == TLMCommand::e_WRITE) {
      m_userdata.getBitString(os);
    }

    // Pad string out to 32 bits
    unsigned int hsize = m_header.getBitSize() + blength.getBitSize();
    pad.resize(32-hsize,'_');
    os << pad ;

    blength.getBitString(os);

    m_header.getBitString (os);

    return os;
  }


  virtual std::ostream & getXATType (std::ostream & os) const {
    os << std::dec << "TLMResponse #(" << DATASIZE << "," << USERDSIZE << " )";
    return os;
  }

  /// returns the size in bit of the packed object
  virtual unsigned int getBitSize () const {
    return getHeaderBitSize() + getPayloadBitSize();
  }

  /// returns the size in 32-bit words of the packed object
  virtual unsigned int getWordSize() const {
    return bits_to_words(getBitSize());
  }

  ///  returns the size in bits of the packed header
  unsigned int getHeaderBitSize() const {
    unsigned usr = (m_header.m_command == TLMCommand::e_WRITE) ? USERDSIZE : 0;
    return (32 + usr);
  }

  /// returns the size in 32-bit words of the packed header
  unsigned int getHeaderWordSize() const {
    return  bits_to_words(getHeaderBitSize());
  }

  /// returns the size in bits of the packed payload field
  unsigned int getPayloadBitSize() const {
    // insure that payload is sized correctly
    setPayload();
    return m_payload.getBitSize() ;
  }
  /// returns the size in 32-bit words) of the packed payload field
  unsigned int getPayloadWordSize() const {
    return  bits_to_words(getPayloadBitSize());
  }

 /**
  * @brief Calculates the packed 32-bit word size if the payload is psize
  *
  * Utility function used for expected response size; there are never byte enables with 
  * a response.
  */
  unsigned int getWordSizeByPayload (size_t psize) {
    if (psize == 0) {
      // write response -- header + user 
      return bits_to_words(32 + USERDSIZE);
    }
    return bits_to_words(32 + (psize * (next32(DATASIZE) + USERDSIZE)));
  }

  /// returns the class name
  virtual const char * getClassName() const {
    return "TLMResponse";
    //os << std::dec << "TLMResponse<" << DATASIZE << "," << USERDSIZE << " >";
  }

  /// returns the BSVKind for this object
  virtual XATKind getKind() const {
    return XAT_Struct ;
  }

  /// Accessor for the count of members in object
  virtual unsigned int getMemberCount() const {
    return 3;
  };

  /**
   * @brief Accessor to member objects
   *
   * @param idx -- member index
   * @return BSVType * to this object or null
   */
  virtual XATType * getMember (unsigned int idx) {
    // insure that payload is sized correctly
    setPayload();
    switch (idx) {
    case 0: return & m_header;
    case 1: return & m_payload;
    case 2: return & m_userdata;
    default:
      std::cerr << "Index error in getMember for class TLMResponse" << std::endl ;
    };
    return 0;
  };

  /**
   * @brief Accessor for symbolic member names
   *
   * @param idx -- member index
   * @return char* to this name or null
   */
  virtual const char * getMemberName (unsigned int idx) const {
    // insure that payload is sized correctly
    setPayload();

    switch (idx) {
    case 0: return "header";
    case 1: return "payload" ;
    case 2: return "userdata" ;
    default:  std::cerr << "Index error in getMemberName for class TLMResponse" << std::endl ;
    };
    return 0;
  };

private:

  /**
   * @brief Utility function to raise x to the next bit boundary.
   *
   * note that next32(32) == 32.
   *
   * @param x -- initial value
   * @return x increase
   */
  static unsigned int next32(unsigned int x) {
    return 32*((x+31)/32);
  }
};
