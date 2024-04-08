//-*- C++ -*-x
#include "sxactors/dllexport.h"
#include "CustomAdapter.h"

#define GPIOXACTOR_WRITE(ADDR,VAL) (0x80000000 | (ADDR * 0x10000) | VAL)
#define GPIOXACTOR_READ(ADDR)      (0x00000000 | (ADDR * 0x10000))

/**
 * Register addresses
 */
#define GPIOXACTOR_REG_GPI_WIDTH          0
#define GPIOXACTOR_REG_GPO_WIDTH         (GPIOXACTOR_REG_GPI_WIDTH   + 1)
#define GPIOXACTOR_REG_GPI_INT           (GPIOXACTOR_REG_GPO_WIDTH   + 1)

#define GPIOXACTOR_REG_START_GPI(GPI,GPO)       (GPIOXACTOR_REG_GPI_INT + 1)
#define GPIOXACTOR_REG_STOP_GPI(GPI,GPO)        (((GPI-1)/16) + GPIOXACTOR_REG_START_GPI(GPI,GPO))
#define GPIOXACTOR_REG_START_GPO(GPI,GPO)       (GPIOXACTOR_REG_STOP_GPI(GPI,GPO) + 1)
#define GPIOXACTOR_REG_STOP_GPO(GPI,GPO)        (((GPO-1)/16) + GPIOXACTOR_REG_START_GPO(GPI,GPO))
#define GPIOXACTOR_REG_GPO_REG(GPI,GPO)         (GPIOXACTOR_REG_STOP_GPO(GPI,GPO) + 1)

#define GPIOXACTOR_CHECK_TEMPLATE(I,O) \
    {                                  \
        if (I != 32 || O != 32) {      \
            XactorsLog logfile;        \
            logfile.Error ("Invalid GpioXactor template parameters, should be <32,32>"); \
        }                                                               \
    }                                                                   \

// type definition for call back function
typedef void (*GpioXactorCallBack) (int value, void *context);


/** 
 * @brief The GPIO transactor
 *
 * The GPIO transactor provides input and output ports. The lower
 * 16 input bits are capable of triggering a callback function.
 * The sensitivity has to be specified as parameters at IP instantiation
 * in the hardware design.
 *
 * Currently only 32bits are supported for GPI and GPO!
 * 
 * @param GPI Input port width
 * @param GPO Output port width
 *
 **/
template<unsigned int GPI=32, unsigned int GPO=32>
class DLLEXPORT GpioXactor : public CustomAdapter {
public:
    /** 
     * @brief The GPIO transactor
     *
     * The GPIO transactor provides input and output ports. The lower
     * 16 input bits are capable of triggering a callback function.
     * The sensitivity has to be specified as parameters at IP instantiation
     * in the hardware design.
     * 
     * @param name    Symbolic transactor name
     * @param device  UMRBus device number
     * @param bus     UMRBus bus number
     * @param address UMRBus CAPIM address
     * 
     * @return 
     */
    GpioXactor(const std::string &name, unsigned int device, unsigned int bus, unsigned int address);
    ~GpioXactor();
    /** 
     * @brief Set the output value of the transactor.
     * 
     * @param val Contains the output value
     * 
     * @return true on success
     */
    bool setOutput(const BitT<GPO> &val);
    /** 
     * @brief Get the output value of the transactor.
     * 
     * @param val Receives the output value
     * 
     * @return true on success
     */
    bool getOutput(BitT<GPO> &val);
    /** 
     * @brief Get the input value of the transactor.
     * 
     * @param val Receives the input value
     * 
     * @return true on success
     */
    bool getInput(BitT<GPI> &val);
    /** 
     * @brief Set the notify callback of the GPIO transactor
     *
     * This callback will be executed whenever an event occurs on at least one
     * input pin matching the sensitivity setup of the GPIO transactor IP.
     * The sensitivity has to be specified as parameters at IP instantiation
     * in the hardware design.
     *
     * The callback function will be called with a 'value' and a 'context' parameter
     * where context is the same as specified when calling setNotifyCallBack and value
     * contains the input bits which caused the callback to be executed.
     * 
     * @param cbfunc   The callback function of type 'void cb(int value, void *context)'
     * @param context  User data passed to the callback function which should
     *                 contain the GpioXactor object to allow identifying the
     *                 owner of the callback.
     * 
     * @return The number of bytes received
     */
    void setNotifyCallBack(GpioXactorCallBack cbfunc, void *context);
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
    GpioXactorCallBack   m_cb;         /**< The callback function */
    std::string         *m_last_error; /**< The last error message */
};

