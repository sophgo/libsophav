#pragma once

#include <string>
#include <list>

#include "gxactors/TLMBSize.h"
#include "sxactors/dllexport.h"


// forward declaration
template <unsigned int ADDRSIZE, unsigned int DATASIZE, unsigned int USERDSIZE> class TLMRequest ;

namespace TLMUtilities {

  /// Enum describing the hardware type for the transactor.  The value are obvious.
  enum E_XactorType {
    e_UNKNOWN
    , e_AHB
    , e_APB
    , e_AXI3
    , e_AXI = e_AXI3
    , e_AXI4
  };


  /// \brief Short name for the error information returned
  typedef std::list <std::string>  ErrorList_t;


  /// \brief get the maximum bsize for the given data size
  DLLEXPORT TLMBSize::E_TLMBSize getBSize(unsigned dsize) ;

  /// \brief list the number of lsb zeros which must be present for a given bsize
  DLLEXPORT unsigned lsbZeros (TLMBSize bsize) ;
  /// \brief list the number of lsb zeros which must be present for a given bsize
  DLLEXPORT unsigned lsbZeros (TLMBSize::E_TLMBSize bsize) ;

  /// \brief return the address increment according to TLMBSize
  DLLEXPORT unsigned addrIncr (TLMBSize bsize) ;
  /// \brief return the address increment according to TLMBSize
  DLLEXPORT unsigned addrIncr (TLMBSize::E_TLMBSize bsize) ;

  /// \brief returns a user friendly string of the xactor type
  DLLEXPORT const char * getXactorTypeName ( const E_XactorType xtype) ;


  /// \brief Checks that the TLMRequest is valid for the xactor type
  template <unsigned int A, unsigned int D, unsigned int U>
    DLLEXPORT unsigned isValid (TLMUtilities::E_XactorType xtype, const TLMRequest<A,D,U> &req, TLMUtilities::ErrorList_t &errs) ;



};

