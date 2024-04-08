//-*- C++ -*-x
#include <deque>
#include "sxactors/dllexport.h"

/// Type definition for UART transactor callback function
typedef void (*UartXactorCallBack) (int count, void *context);

/** 
 * @brief Container for interrupt thread data
 */
class DLLEXPORT UartThreadContext {
public:
    /** 
     * @brief Container for interrupt thread data
     */
    UartThreadContext();
    virtual ~UartThreadContext();
    UartXactorCallBack m_cb;      /**< @brief  The callback function */
    void*              m_context; /**< @brief  The context pointer */
    pthread_mutex_t    m_mutex;   /**< @brief  Mutex */
    pthread_cond_t     m_cond;    /**< @brief  Condition */
    int                m_recvcnt; /**< @brief  The number of bytes received */
};

/**
 * @brief The interrupt thread executes the notify user callback
 **/
class DLLEXPORT UartInterruptThread : public Thread {
public :
    /** 
     * @brief The interrupt thread executes the notify user callback
     * 
     * The user callback is triggered by the callback_irq of the
     * UartXactor.
     *
     * @param context The thread context
     */
    UartInterruptThread(void* context);
    virtual ~UartInterruptThread();
protected:
    void main();
private:
    void *m_context;  /**< @brief  The thread context pointer */
};


/** 
 * @brief The UART transactor
 *
 * The UART transactor provides a high level interface to the uart_master which
 * is a combination of an AHB/AXI master and GPIO transactor connected
 * to a dw_apb_uart core.
 *
 **/
class DLLEXPORT UartXactor {
public:
    /** 
     * @brief The UART transactor
     *
     * The UART transactor provides a high level interface to the uart_master which
     * is a combination of an AHB/AXI master and GPIO transactor connected
     * to a dw_apb_uart core.
     * 
     * @param name    Symbolic transactor name
     * @param device  UMRBus device number
     * @param bus     UMRBus bus number
     * @param address UMRBus CAPIM address
     * 
     * @return 
     */
    UartXactor(const std::string &name, unsigned int device, unsigned int bus, unsigned int address);
    ~UartXactor();

    /** 
     * @brief Read a register of the dw_apb_uart.
     * 
     * @param address  The register address
     * @param data     The register data
     * 
     * @return true on success
     */
    bool getRegister(const unsigned char address, unsigned char &data);
    /** 
     * @brief Write a register of the dw_apb_uart.
     * 
     * @param address  The register address
     * @param data     The register data
     * 
     * @return true on success
     */
    bool setRegister(const unsigned char address, const unsigned char data);
    /** 
     * @brief Set the baudrate of the dw_apb_uart.
     * 
     * @param uart_clock The uart base clock frequency in Hz
     * @param baudrate   The desired baudrate
     * 
     * @return true on success
     */
    bool setBaudrate(const unsigned int uart_clock, const unsigned int baudrate);
    /** 
     * @brief Read data from the dw_apb_uart.
     * 
     * @param count  The number of bytes to be read
     * @param data   Receives the data
     * 
     * @return The number of bytes received
     */
    size_t receive(const size_t count, unsigned char* data);
    /** 
     * @brief Send data to the dw_apb_uart.
     * 
     * @param count  The number of bytes to be written
     * @param data   Contains the data
     * 
     * @return The number of bytes written
     */
    size_t send(const size_t count, unsigned char* data);
    /** 
     * @brief Set the notify callback of the UART transactor
     *
     * This callback will be executed whenever data has been
     * received.
     * 
     * @param cbfunc   The callback function
     * @param context  User data passed to the callback function
     * 
     * @return The number of bytes received
     */
    void setNotifyCallBack(UartXactorCallBack cbfunc, void *context);
    /** 
     * @brief Get the name of the transactor
     * 
     * @return The name of the transactor
     */
    inline const char *getNameStr() const {
        return m_name.c_str();
    }
    /** 
     * @brief Get the name of the transactor
     * 
     * @return The name of the transactor
     */
    inline const std::string & getName() const {
        return m_name;
    }
    /** 
     * @brief Get the last error message of the transactor
     * 
     * @return The last error message of the transactor
     */
    inline const std::string getLastError() {
        std::string s = m_last_error;
        m_last_error.clear();
        return s;
    }
    /** 
     * @brief Check if the transactor was opened successfully
     * 
     * @return true if the transactor was opened successfully
     */
    inline const bool isOpen() const {
        return m_valid;
    }

private:
    /** 
     * @brief Static interrupt callback
     *
     * This function is executed whenever the uart core generates an
     * interrupt. It will read the received data and put it into the
     * receive queue. It will trigger the notify user callback which is
     * executed in the interrupt thread.
     * 
     * @param data Interrupt data
     * @param p    Context pointer
     */
    static void callback_irq(int data, void *p);
    /** 
     * @brief Read data from the uart core fifo
     * 
     * @param count  The number of bytes to read
     * @param data   Receives the data
     * 
     * @return true on success
     */
    bool getFifo(const size_t count, unsigned char *data);
    /** 
     * @brief Read all data from the uart core
     * 
     * @param fifoenable Enable fifo if true
     * 
     * @return true on success
     */
    bool readData(bool fifoenable = true);

    MasterXactor<32,32> *m_bus;            /**< @brief The bus master transactor */
    GpioXactor<32,32>   *m_irq;            /**< @brief The GPIO transactor */
    char                *m_errmsg ;        /**< @brief Temporary error message */
    std::string          m_name;           /**< @brief The transactor name */
    std::string          m_last_error;     /**< @brief The last error */
    bool                 m_valid;          /**< @brief True if transactor was opened successfully */
    bool                 m_is_dw_uart;     /**< @brief True if dw_apb_uart is used */
    bool                 m_is_oc_16550;    /**< @brief True if OpenCores 16550is used */
    unsigned int         m_uart_clock;     /**< @brief The uart base clock frequency in Hz */
    unsigned int         m_baud;           /**< @brief The baudrate */
    std::deque<char>     m_recvQ;          /**< @brief The receive data queue */
    UartThreadContext    m_context;        /**< @brief The thread context */
    UartInterruptThread *m_intThread;      /**< @brief The interrupt thread */
};
