//-*- C++ -*-x
#include "sxactors/dllexport.h"
#include "CustomAdapter.h"

/**
 *
 */
class DLLEXPORT ClockResetControl: public CustomAdapter {
    public:
    /**
     * @brief
     *
     * @param name
     * @param device
     * @param bus
     * @param address
     * @param polling
     */
    ClockResetControl(const std::string &name, unsigned int device, unsigned int bus, unsigned int address, bool polling = false);
    ~ClockResetControl();

    /**
     * @brief
     *
     * @param cycles
     *
     * @return
     */
    bool setClockCycle(unsigned int cycles);

    /**
     * @brief
     *
     * @param divider
     *
     * @return
     */
    bool setClockDiv(unsigned int divider);

    /**
     * @brief
     *
     * @return
     */
    bool resetRCCNT();

    /**
     * @brief
     *
     * @param id
     * @param value
     *
     * @return
     */
    bool setReset(unsigned int id, unsigned int value);

    /**
     * @brief
     *
     * @param cycles
     *
     * @return
     */
    bool setNumClockCycles(unsigned int cycles);

    /**
     * @brief
     *
     */
    void startClockDriver();

    /**
     * @brief
     *
     * @param cycles
     */
    void startClockDriver(unsigned int cycles);

    /**
     * @brief
     */
    void stopClockDriver();

    /**
     * @brief
     *
     * @param mode
     */
    void setPollingMode(bool mode);

    /**
     * @brief
     *
     * @param cycle
     */
    unsigned long long waitOnCycle(unsigned int cycle);

    /**
     * @brief
     *
     * @param cycle
     */
    unsigned long long waitForCycle(unsigned int cycle);

    /**
     * @brief
     *
     */
    unsigned long long getCycle();

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

    bool m_polling;
    bool m_thread_run;
    pthread_cond_t m_cond_next_event;
    pthread_mutex_t m_mutex_next_event;
    std::string *m_last_error; /**< The last error message */
    unsigned int m_cycles;
    unsigned long long m_next_event;
    unsigned long long m_sw_cclk_cycle;
};
