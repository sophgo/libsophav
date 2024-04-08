//-*- C++ -*-x
#include "sxactors/dllexport.h"
#include "CustomAdapter.h"

#define STREAM_CMD(X, Y) 			((X * 0x10000000) | Y)
#define STREAM_FIFO_DEPTH(X)		(X & 0x3FF)
#define STREAM_FIFO_IS_EMPTY(X)		((X / 0x400) & 0x1)
#define STREAM_CURRENT_INDEX(X)		(X / 0x10000)

#define STREAM_GET_FIFO_CONTENT		0x1
#define STREAM_GET_FIFO_COUNT		0x2
#define STREAM_GET_STREAM_BITWIDTH	0x3
#define STREAM_SET_TIMEOUT_VALUE	0x4
#define STREAM_INIT_INTERRUPTS   	0x5
#define STREAM_CLEAR_FIFO			0xF

// type definition for call back function
typedef void (*StreamSlaveXactorCallBack) (void *element, void *context);

/**
 * @brief The StreamSlaveXactor
 *
 * The StreamSlaveXactor transfers any changes in the input data vectors originating from
 * the hardware. INTW can be any multiple of 32 between
 * 32 and 1024 and has to match the hardware parameter used in the design.
 * The data vectors coming from the hardware trigger a
 * callback function which needs to be implemented by the user in
 * order to retrieve the state of the input lines.
 *
 * @param INTW Port width Stream Transactor Slave
 *
 **/
template<unsigned int INTW=32>
class DLLEXPORT StreamSlaveXactor : public CustomAdapter {
public:
    /**
     * @brief The StreamSlaveXactor transfers any changes in the input data vectors originating from
     * the hardware.
     *
     * @param name    Symbolic transactor name
     * @param device  UMRBus device number
     * @param bus     UMRBus bus number
     * @param address UMRBus CAPIM address
     *
     * @return
     */
    StreamSlaveXactor(const std::string &name, unsigned int device, unsigned int bus, unsigned int address);
    ~StreamSlaveXactor();
    /**
     * @brief Set the notify callback of the Stream Slave Transactor
     *
     * This callback will be executed whenever an event occurs when the Stream
     * Transactor Slave internal FIFO has at lease one input data vector.
     *
     * The callback function will be called with a 'value' and a 'context' parameter
     * where context is the same as specified when calling setNotifyCallBack and value
     * contains the input data signals.
     *
     * @param cbfunc   The callback function of type 'void cb(int value, void *context)'
     * @param context  User data passed to the callback function which should
     *                 contain the StreamSlaveXactor object to allow identifying the
     *                 owner of the callback.
     *
     * @return The number of bytes received
     */
    void setNotifyCallBack(StreamSlaveXactorCallBack cbfunc, void *context);
    /**
     * @brief Get the transactor FIFO contents for a given depth.
     *
     * This function gets the number of elements as specified by the param count from the Stream
     * Transactor Slave
     *
     * @param count Number of elements to fetch for the Transactor, The element count can be obtained
     * using the getFIFOFillCount function
     *
     * @return true on success
     */
    bool getData(BitT<INTW> *elements, unsigned int count = 1);
    /**
     * @brief Sets the number of CLK cycles after which the data vectors are transfered to the software by the Stream Slave transactor.
     *
     * The Transactor by default issues an UMRBus Interrupt everytime a new data is inserted
     * into the FIFO. With this function, a User can set a delay in order to
     * delay the interrupt by a specified number of CLK cycles.  The default value is 0.
     *
     * @param timeout The number of CLK cycles to delay the next UMR Bus interrupt
     *
     * @return the set delay value
     */
    void setDelayCycles(unsigned int timeout = 0);
    /**
     * @brief Gets the number of CLK cycles after which the data vectors are transfered to the software by the Stream Slave transactor.
     *
     * This function gets the current timeout value of the StreamSlaveXactor
     *
     * @return The delay value
     */
    unsigned int getDelayCycles();
    /**
     * @brief Get FIFO fill count of the StreamSlaveXactor and index of the word index of the element being read
     *
     * Get FIFO fill count of the StreamSlaveXactor and index of the word index of the element being read
     *
     * @param index This stores the word index of the element being read
     *
     * @return The FIFO fill count
     */
    unsigned int getFIFOFillLevel(unsigned int *index = NULL);
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
    void                    *m_context;    /**< The context passed to the callback function */
    StreamSlaveXactorCallBack    m_cb;    /**< The callback function */
    std::string             *m_last_error; /**< The last error message */
    unsigned int			m_timeout; /**< The previously set timeout value */
};
