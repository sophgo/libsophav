//-*- C++ -*-x
#include "sxactors/dllexport.h"
#include "CustomAdapter.h"

// type definition for call back function
typedef void (*IntSlaveXactorCallBack) (void* current, void* rising, void* falling, void *context);

/** 
 * @brief The INT Slave transactor
 *
 * The INT Slave transactor provides INTW interrupts coming from
 * the hardware. INTW can be any multiple of 32 between
 * 32 and 1024 and has to match the hardware parameter used in the design.
 * The interrupts coming from the hardware trigger a
 * callback function which needs to be implemented by the user in
 * order to retrieve the state of the interrupt lines.
 * 
 * @param INTW Interrupt port width
 *
 **/
template<unsigned int INTW=32>
class DLLEXPORT IntSlaveXactor : public CustomAdapter {
public:
    /** 
     * @brief The INT Slave transactor provides INTW interrupts coming
     *        from the hardware.
     * 
     * @param name    Symbolic transactor name
     * @param device  UMRBus device number
     * @param bus     UMRBus bus number
     * @param address UMRBus CAPIM address
     * 
     * @return 
     */
    IntSlaveXactor(const std::string &name, unsigned int device, unsigned int bus, unsigned int address);
    ~IntSlaveXactor();
    /** 
     * @brief Get the input value of the transactor.
     * 
     * @param val Receives the output value
     * 
     * @return true on success
     */
    bool getInput(BitT<INTW> &val);
    /** 
     * @brief Set the notify callback of the INT transactor
     *
     * This callback will be executed whenever an event occurs on one of the
     * interrupts coming from the hardware.
     *
     * The callback function will be called with a 'value' and a 'context' parameter
     * where context is the same as specified when calling setNotifyCallBack and value
     * contains the rising and falling edge identifiers for each interrupt bit.
     * 
     * @param cbfunc   The callback function of type 'void cb(int value, void *context)'
     * @param context  User data passed to the callback function which should
     *                 contain the IntSlaveXactor object to allow identifying the
     *                 owner of the callback.
     * 
     * @return The number of bytes received
     */
    void setNotifyCallBack(IntSlaveXactorCallBack cbfunc, void *context);
    /** 
     * @brief Get the last error message of the transactor
     * 
     * @return The last error message of the transactor
     */
    inline const std::string getLastError() {
        std::string s = m_last_error ? *m_last_error : "";
        if (m_last_error) m_last_error->clear();
        return s;
    }
private:
    int service();
    
private:
    void*                m_context;    /**< The context passed to the callback function */
    IntSlaveXactorCallBack    m_cb;    /**< The callback function */
    std::string         *m_last_error; /**< The last error message */
};
