//-*- C++ -*-x
#pragma once

#include "TLMRequest.h"
#include "../gxactors/TLMBurstMode.h"
#include <list>
#include <sstream>
#include "sxactors/dllexport.h"

/**
 * @brief A templated class for checking TLMRequests.
 *
 * The public interface is a single static function -- isValid. All
 * other members are protected.
 *
 * @param A  The AMBA address width
 * @param D  The AMBA data width
 * @param U  The AMBA user data width
 */
template<unsigned int A, unsigned int D, unsigned int U=0>
class DLLEXPORT ValidRequest {
public:

  /**
   * @brief Check that the TLMRequest is valid for the specific transactor type.
   *
   * @param xtype -- the type of the hardware xactor
   * @param req -- the TLMRequest object to be checked
   * @param errs -- a list of errors found
   * @return count of errors found
   */
  static unsigned isValid (  TLMUtilities::E_XactorType xtype, const TLMRequest<A,D,U> &req, TLMUtilities::ErrorList_t &errs) ;


protected:
  /// type of transactor for checking
  TLMUtilities::E_XactorType      m_type;
  /// The request to be check
  const TLMRequest<A,D,U>  & m_request;
  /// A list of errors to report
  TLMUtilities::ErrorList_t      & m_errors;
  /// The count of the number of error for this request
  unsigned          m_errCnt;

protected:
  /// Protected constructor
  ValidRequest ( TLMUtilities::E_XactorType xtype,  const TLMRequest<A,D,U> &req, TLMUtilities::ErrorList_t &errs);

  /// Accessor to return the error count
  inline unsigned errCount() { return m_errCnt; }

  /// error report for address alignment error
  void errorAddressAlignment () {
    m_errCnt++;
    std::ostringstream msg;
    msg << "Unaligned address found. Word size: " << (m_request.m_header.m_b_size)
        << " Address: " << m_request.m_address;
    m_errors.push_back( msg.str() );
  }

  /// error message for burst transaction crossing a boundary
  void errorBurstCrossing (unsigned boundary) {
    m_errCnt++;
    std::ostringstream msg;
    msg << "Burst will cross a " << std::dec << (boundary/1024) << "K boundary"
        << " Word size: " << (m_request.m_header.m_b_size)
        << " Burst mode: " << m_request.m_header.m_burst_mode
        << " Burst length: " << m_request.m_b_length
        << " Address: " << m_request.m_address;
    m_errors.push_back( msg.str() );
  }

  /// Error messagee for incorrect burst size for wrapped transaction
  void errorBurstLength () {
    ++m_errCnt;
    std::ostringstream msg;
    msg << "Burst length is not a power of 2 for a WRAP transaction"
        << " Burst length: " << m_request.m_b_length ;
    m_errors.push_back (msg.str());
  }

  /// Error message when exclusive access is not a power of 2
  void errorExclusiveSize(unsigned total) {
    ++m_errCnt;
    std::ostringstream msg;
    msg << "Number of bytes transferred in an exclusive access must be a power or 2"
        << " Burst length: " << m_request.m_b_length
        << " Word size: " << (m_request.m_header.m_b_size)
        << " total bytes: " << std::dec << total
      ;
    m_errors.push_back (msg.str());
  }

  /// Error message when exclusive access is > 128
  void errorExclusiveTooBig(unsigned total) {
    ++m_errCnt;
    std::ostringstream msg;
    msg << "Number of bytes transferred in an exclusive access must <= 128"
        << " Burst length: " << m_request.m_b_length
        << " Word size: " << (m_request.m_header.m_b_size)
        << " total bytes: " << std::dec << total
      ;
    m_errors.push_back (msg.str());
  }

  /// Error message when an exclusive transaction is not aligned.
  void errorExclusiveUnaligned(unsigned total) {
    ++m_errCnt;
    std::ostringstream msg;
    msg << "For exclusive access transaction, the address must be aligned for the total bytes to transfer"
        << " Burst length: " << m_request.m_b_length
        << " Word size: " << (m_request.m_header.m_b_size)
        << " total bytes: " << std::dec << total
        << " Address: " << m_request.m_address
      ;
    m_errors.push_back (msg.str());
  }
  
  /// Error message when the bus fabric does not support the requests lock mode.
  void errorWrongLockMode () {
    ++m_errCnt;
    std::ostringstream msg;
    msg << "" << TLMUtilities::getXactorTypeName(m_type)
        << " does not support "
        <<  m_request.m_header.m_lock
        << " transfers"
      ;
    m_errors.push_back (msg.str());
  }

  /// Error message when the bus fabric does not support the requested
  /// burst size. Note that the transactor break the burst into legal sizes.
  void errorBurstSize(unsigned maxb) {
    ++m_errCnt;
    std::ostringstream msg;
    msg << "Burst size is too large for a "
        << TLMUtilities::getXactorTypeName(m_type)
        << " transaction' maximum size is: "
        << std::dec << maxb
        << " Burst length: " << m_request.m_b_length
      ;
    m_errors.push_back (msg.str());  }

private:
  /// Check the current request and type
  void check() ;

  /// Check the request for legal AHB bus transfer
  void checkAhb() {
    checkAddressAligned();
    checkBurstCross(1024);
    checkBurstLenght();
  }
  /// Check the request for legal APB bus transfer
  void checkApb()  {
    checkAddressAligned();
    checkBurstSize(1);
  }
  /// Check the request for legal AXI bus transfer
  void checkAxi3()  {
    checkBurstCross(4*1024);
    checkExclusizeAccessRestrictions();
  }
  /// Check the request for legal AXI4 bus transfer
  void checkAxi4()  {
    checkBurstCross(4*1024);
    checkExclusizeAccessRestrictions();
  }

  ///////////////////////////////////////////////////////////////////////////
  /// Check that the address is aligned for the given word size
  void checkAddressAligned() ;

    /**
     * @brief Check that the burst does not cross the specified boundary.
     * @param boundary -- must be power of 2
     */
  void checkBurstCross(unsigned boundary) ;

  /// Check that burst lenght is valid. wrap must be a power of 2 (-1)
  void checkBurstLenght() ;

  /// Check that that exclusive access restrictions are met for AXI transactions.
  /// Address must be aligned withe total byte transfered
  /// number of byte must be power of 2 upto 128
  void checkExclusizeAccessRestrictions();

  /// Check that the lock field is supported for the xtype
  void checkLock() ;

  /// check that b_lenght is valid
  void checkBurstSize(unsigned maxb) ;

  ////////////////////////////////////////////////////////////////////
  /// An obvious utility function
  static bool isPowerOfTwo (unsigned x) {
      return ((x > 0) ? (((x-1) & x) == 0) : 0);
  }
};
