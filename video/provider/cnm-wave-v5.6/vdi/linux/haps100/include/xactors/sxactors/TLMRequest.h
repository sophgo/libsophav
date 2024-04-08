//-*- C++ -*-x
#pragma once

#include "bxactors/xat_scemi.h"
#include "sized_types.h"
#include "gxactors/TLMRequestParams.h"
#include "gxactors/TLMBurstLong.h"
#include "TLMUtilities.h"
#include "TLMPayload.h"
#include "XactorLog.h"
#include <vector>
#include <stdlib.h>
#include <math.h>


/**
 * @brief Data class representing a TLMRequest.
 *
 * This class represents a TLMRequest.  A request consists of 4
 * fields, the header, the address, the burst length, and the payload.  This class is
 * templated on the address width and the data width, both must match
 * the widths of the underlying hardware xactor.
 * Valid address widths are 32 and 64
 * Valid data widths are 8, 16, 32, 64, 128, 256, 512, 1024.
 *
 * @param ADDRSIZE   The AMBA address width
 * @param DATASIZE   The AMBA data width
 * @param USERDSIZE  The AMBA user data width
 */
template <unsigned int ADDRSIZE=32, unsigned int DATASIZE=32, unsigned int USERDSIZE=0>
class TLMRequest : public XATType {
public:
  /// The request header
  TLMRequestParams m_header;
  /// The address for the request
  BitT<ADDRSIZE>   m_address;

  /// The address for the request
  BitT<USERDSIZE>   m_userdata;

  /// The payload (data plus byte enables)
  mutable TLMPayload<DATASIZE,USERDSIZE>   m_payload;
  /// The burst lenght
  unsigned         m_b_length;

  /// Empty constructor
  TLMRequest ()
    : m_header()
    , m_address()
    , m_userdata()
    , m_payload()
    , m_b_length(1)
  {
    m_header.m_b_size = getBSize();
  }

  /**
   * @brief Constructor from Message stream.
   *
   * A TLMRequest object is constructed from the message buffer
   * with data taken from the bit offset.
   *
   * @param msg -- pointer to message buffer class
   * @param off -- initial bit offset into the message buffer (modified by call)
   */
  TLMRequest ( const SceMiMessageDataInterface *msg, unsigned int &off )
    : m_header()
    , m_address()
    , m_userdata()
    , m_payload()
    , m_b_length(1)
  {
    off = unpackHeaderAddress(msg, off);
    off = next32(off);

    unpackPayload(msg, off);

    if ((int) m_header.m_b_size.m_val > (int) getBSize()) {
      XactorLog logfile;
      logfile.Warn ("TLMRequest constructed with incorrect bsize");
    }

  }

  /**
   * @brief Piece-wise constructor for header and address from message buffer.
   *
   * This method unpacks the message data populating the header and
   * address fields.   It is not expected that will be be called
   * outside of the class.
   *
   * @param msg -- pointer to message buffer class
   * @param off -- initial bit offset into the message buffer (modified by call)
   * @return the bit off for the next item in message buffer.
   */
  unsigned int unpackHeaderAddress (const SceMiMessageDataInterface *msg, unsigned int &off) {
    unsigned int addroff = off + 64;
    m_header = TLMRequestParams(msg, off);

    TLMBurstLong blength(msg, off);
    m_b_length = 1 + (unsigned) blength;

    m_address = BitT<ADDRSIZE>(msg, addroff);
    setPayload();

    if (USERDSIZE != 0) {
      m_userdata = BitT<USERDSIZE> (msg, addroff);
    }

    return addroff;
  }

  /**
   * @brief Piece-wise constructor for payload from message buffer.
   *
   * This method unpacks the message data populating payload field.
   * It is not expected that will be be called
   * outside of the class.
   *
   * @param msg -- pointer to message buffer class
   * @param off -- initial bit offset into the message buffer (modified by call)
   * @return the bit off for the next item in message buffer.
   */
  unsigned int unpackPayload (const SceMiMessageDataInterface *msg, unsigned int &off) {
    setPayload();
    off = m_payload.unpackPayload(msg, off);
    return off;
  }

  /**
   * @brief Populates message buffer from object.
   *
   * A portion of the message buffer is populated from the contents
   * of the class object.
   *
   * @param msg -- message buffer to be populated
   * @param off -- initial bit offset into the message buffer
   * @return the bit off for the next item in message buffer.
   */
  unsigned int setMessageData (SceMiMessageDataInterface &msg, const unsigned int off=0) const {
    // insure that payload is sized correctly
    setPayload();
    unsigned int running = off;

    TLMBurstLong blength = (int) (m_b_length-1);
    
    running = m_header.setMessageData  ( msg, running );
    running = blength.setMessageData   ( msg, running );

    running = m_address.setMessageData ( msg, off+64 );
    running = m_userdata.setMessageData (msg, running );
    running = m_payload.setMessageData ( msg, running);

    return running;
  }

    /** 
     * @brief Copy data into the request's payload object.
     *
     * This method takes into account the address of the request (m_address)
     * which needs to be assigned prior to the fillPayload call. In case of 
     * an unaligned request, the necessary padding bytes will be added to the
     * front of the payload. These padding bytes will be disabled by assigning
     * approppriate byte enables. As a consequence, the m_b_length parameter
     * will be modified to account for the additional bytes.
     * Using this method it is possible to assemble requests of any length and
     * target address.
     *
     * Note: Byte enables are expected in tlm format where each byte enable
     *       byte represents one byte lane. Any non-zero value enables that
     *       specific byte lane.
     *
     * @param src_pl -- The source pointer for the payload data
     * @param sz_pl  -- The number of payload data bytes
     * @param src_be -- The source pointer for byte enables
     * @param sz_be  -- The number of byte enable bytes
     * 
     * @return The number of bytes copied.
     */
    size_t fillPayload (const char *src_pl, size_t sz_pl, const char *src_be = NULL, size_t sz_be = 0) {
        size_t       retval = 0;
        unsigned int zeros  = log((double)DATASIZE/8)/log((double)2);
        long long addr      = (long long) m_address;
        long long mask      = ~((-1LL) << zeros);
        if ((mask & addr) != 0 || (sz_pl % (DATASIZE/8)) != 0) {
            // if not aligned we need to assemble a valid payload
            unsigned int padcnt = (mask & addr & 0xFF);
            
            // calculate new length
            m_b_length = 1 + ((sz_pl + padcnt - 1) / (DATASIZE/8));

            unsigned int i;
            char *dst_pl = (char*) calloc(m_b_length+1, DATASIZE/8);
            char *dst_be = (char*) calloc(m_b_length+1, DATASIZE/8);

            if(!dst_be || !dst_pl) {
              if(dst_be) free(dst_be);
              if(dst_pl) free(dst_pl);
              return 0;
            }

            // enable byte enable usage            
            m_header.m_spec_byte_enable = true;
            setPayload();

            // pad data and byte enables at the lower addresses            
            for (i = 0; i < padcnt; i++) {
                dst_pl[i] = 0;
                dst_be[i] = 0;
            }

            memcpy(dst_pl+padcnt, src_pl, sz_pl);
            memset(dst_be+padcnt, 0xFF,   sz_pl);
            
            // copy already existing byte enables            
            if (src_be && sz_be) {
                memcpy(dst_be+padcnt, src_be, stdmin(m_b_length * DATASIZE/8 - padcnt, sz_be));
            }

            // fill payload and byte enables in TLMPayload            
            m_payload.fillByteEnables(     dst_be, m_b_length * DATASIZE/8);
            retval = m_payload.fillPayload(dst_pl, m_b_length * DATASIZE/8);

            free(dst_pl);
            free(dst_be);
        } else {
            m_payload.fillByteEnables(src_be, sz_be);
            retval = m_payload.fillPayload(src_pl, sz_pl);
        }
        return retval;
    }

    /** 
     * @brief Fill the memory with the data contained in the payload.
     *
     * This method takes into account the address of the request (m_address)
     * which needs to be assigned prior to the fillMemory call. In case of 
     * an unaligned request, the memory will be filled starting from the offset.
     *
     * @param dst_pl -- The destination pointer for payload data
     * @param sz_pl  -- The size of the destination buffer
     * @param dst_be -- The byte enable pointer
     * @param sz_be  -- The number of byte enable bytes
     * 
     * @return The number of bytes copied
     */
    size_t fillMemory(char *dst_pl, size_t sz_pl, char *dst_be = NULL, size_t sz_be = 0)
    {
        long long addr     = (long long) m_address;
        size_t retval      = 0;
        unsigned int zeros = log((double)DATASIZE/8)/log((double)2);
        long long mask     = ~((-1LL) << zeros);
        if ((mask & addr) != 0) {
            // if not aligned we need to extract data from payload
            unsigned int padcnt = (mask & addr & 0xFF);
            m_payload.fillMemoryFromByteEnables(dst_be, sz_be, padcnt);
            retval = m_payload.fillMemory(dst_pl, sz_pl, padcnt);
            
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
    return (m_header.m_command == TLMCommand::e_WRITE) ? (burstl) : 0;
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
    m_payload.resize(getPayloadSize(), (bool) m_header.m_spec_byte_enable);
  }

  /**
   * @brief Determine the payload size for the reponse.
   *
   * Returns the expected response payload size for this request.
   *
   * @return the payload size in "datasize" words for the response
   */
  unsigned int determineResponseSize() const {
    unsigned int burstl = m_b_length ;
    return (m_header.m_command == TLMCommand::e_READ) ? (burstl) : 0;
  }

  /// PutTo operation, tcl friendly.
  friend std::ostream & operator<< (std::ostream &os, const TLMRequest &obj) {
    // insure that payload is sized correctly
    obj.setPayload();

    XATType::PutTo * override = lookupPutToOverride ( obj.getClassName() );
    if ( override != 0 ) {
       return override(os, obj );
    }
    os << "{" ;
    os << "header " << obj.m_header ;os << " " ;
    os << "b_length " << std::dec << obj.m_b_length ; os << " " ;
    os << "address " << obj.m_address ;os << " " ;
    if (USERDSIZE != 0) {
      os << "userdata " << obj.m_userdata << " " ;
    }
    os << "payload " << obj.m_payload ;
    os << "}" ;
    return os;
  }

  /// returns the bit stream as series of bits
  virtual std::ostream & getBitString (std::ostream & os) const {
    // insure that payload is sized correctly
    setPayload();
    std::string pad;

    // Payload to header -- this is backwards...
    m_payload.getBitString(os);

    m_userdata.getBitString(os);
    m_address.getBitString (os);

    // Pad string out to 64 bits
    unsigned int hsize = m_header.getBitSize() + 8 + 1; // XXX
    pad.resize(64-hsize,'_');
    os << pad ;

    TLMBurstLong blength = (int) (m_b_length - 1);
    blength.getBitString(os);

    m_header.getBitString (os);
    // Pad string out to next word boundary

    return os;
  }

  /// generates an ostream listing the type of the hardware xactor
  virtual std::ostream & getXATType (std::ostream & os) const {
    os << std::dec << "TLMRequest #(" << ADDRSIZE << "," << DATASIZE << "," << USERDSIZE << " )";
    return os;
  }

  /// returns the size in bit of the packed object
  virtual unsigned int getBitSize () const {
    // data than 32 bits in not packed.
    return getHeaderBitSize() + m_payload.getBitSize();
  }
  /// returns the size in 32-bit words of the packed object
  virtual unsigned int getWordSize () const {
    return getHeaderWordSize() + getPayloadWordSize();
  }

  ///  returns the size in bits of the packed header and address field
  unsigned int getHeaderBitSize() const {
    return (64 + ADDRSIZE  + USERDSIZE);
  }
  ///  returns the size in 32-bit words of the packed header and address field 
  unsigned int getHeaderWordSize() const {
    return  bits_to_words(getHeaderBitSize());
  }

  /// returns the size in bits of the packed payload field
  unsigned int getPayloadBitSize() const {
    // insure that payload is sized correctly
    setPayload();
    return m_payload.getBitSize();
  }
  ///  returns the size in 32-bit words) of the packed payload field
  unsigned int getPayloadWordSize() const {
    return  bits_to_words(getPayloadBitSize());
  }

  /// returns the class name
  virtual const char * getClassName() const {
    return "TLMRequest";
    //os << std::dec << "TLMRequest<" << ADDRSIZE << "," << DATASIZE << "," << USERDSIZE << " >";
  }

  /// returns the BSVKind for this object
  virtual XATKind getKind() const {
    return XAT_Struct ;
  }

  /// Accessor for the count of members in object
  virtual unsigned int getMemberCount() const {
    return 4;
  };

  virtual XATType * getMember (unsigned int idx) {
    // insure that payload is sized correctly
    setPayload();
    switch (idx) {
    case 0: return & m_header;
    case 1: return & m_address;
    case 2: return & m_userdata;
    case 3: return & m_payload;
    default: 
        std::cerr << "Index error in getMember for class TLMRequest" << std::endl ;
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
    case 1: return "address";
    case 2: return "userdata";
    default:       if (idx - 3 > m_payload.size()) {
        return "payload";
      }
      else  {
        std::cerr << "Index error in getMemberName for class TLMRequest" << std::endl ;
      }
    };
    return 0;
  };

  /**
   * @brief Returns the TLMBSize enum for the DATASIZE.
   *
   * Used to set the bsize field in the header.
   */
  TLMBSize::E_TLMBSize getBSize() const {
    return TLMUtilities::getBSize(DATASIZE);
  }

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

  /**
   * @brief Utility function to get the bit index of the data in the request
   * @return bit index of the data payload
   dos2un */
  unsigned int dataOffSet () {
    return 64 + next32(ADDRSIZE);
  }
};
