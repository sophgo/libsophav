//-*- C++ -*-x
#pragma once

#include <pthread.h>
#include <list>
#include "umrbus.h"
#include "sxactors/dllexport.h"

class DLLEXPORT CustomAdapter {

 public:
    /// mutex lock for accessing the CAPIM
    pthread_mutex_t m_mutex ;

 protected:
    /// CAPIM handle, access should be locked with m_mutex
    UMR_HANDLE  m_hUmr;

    /// The transactor type
    unsigned int m_type;

    /// pointer to any error messages from umr_bus calls
    char        *m_errmsg ;

    /// master or slave transactor
    bool m_master;

 private:

    /// The user's name of the transactor
    // This is dynamically allocated to avoid Windows VC++ warning C4251
    // about STL member data in DLLs.
    std::string* m_name;

    /// mutex lock for updating service list information
    static pthread_mutex_t s_service_master_mutex ;
    static pthread_mutex_t s_service_slave_mutex ;

    /// Container type for all instantiated adapter objects
    typedef std::list<CustomAdapter *> ServiceList;
    /// list of all registered adapters
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4251)
         //this is private, so will not be accessed outside the DLL
#endif
    static ServiceList  s_registeredMasterAdapters;
    static ServiceList  s_registeredSlaveAdapters;
#ifdef _WIN32
#pragma warning(pop)
#endif


 public:
    /// Constructor
    /// \param name -- name used in error messages and debug
    /// \param device -- umr device
    /// \param bus -- umr bus
    /// \param address -- umr address
    /// \param xtype -- the CAPIM type
    CustomAdapter(bool master, const std::string &name, unsigned int device, unsigned int bus, unsigned int address, unsigned int xtype = 0);
     
    /// Destructor
    virtual ~CustomAdapter();

    /// Accessor for name of xactor
    inline const char *getNameStr() const {
        return m_name->c_str();
    }
    /// Accessor for name of xactor
    inline const std::string & getName() const {
        return *m_name;
    }

    /** 
     * @brief Check if the transactor was opened successfully
     * 
     * @return true if the transactor was opened successfully
     */
    inline bool isOpen() {
        return m_hUmr != UMR_INVALID_HANDLE;
    }

    /// Service function for this adapter.
    /// This function is periodically called by the UmrServiceThread.
    /// Interrupts from the client are processed and the user callback
    /// will be executed.
    /// \return 1 if state of object has changes, 0 otherwise
    virtual int service() { return 0; }

private:
    /// register an adapter object to be part of the servive loop
    /// \param a -- the adapter object
    static void registerAdapter (bool master, CustomAdapter *a);

    /// unregister an adapter object
    /// \param a -- the adapter object
    static void unregisterAdapter (bool master, CustomAdapter *a);

public:
    /// static member function which services all registered adapters
    static int serviceAll (bool master) ;

};
