//-*- C++ -*-x
#include "sxactors/dllexport.h"
#include "CustomAdapter.h"

/** 
 * @brief The INT Master transactor
 *
 * The INT Master transactor provides INTW interrupts going into the hardware.
 * INTW can be any multiple of 32 between 32 and 1024 and has to match the
 * hardware parameter used in the design.
 * 
 * @param INTW Interrupt port width
 *
 **/
template<unsigned int INTW=32>
class DLLEXPORT IntMasterXactor : public CustomAdapter {
public:
    /** 
     * @brief The INT Master transactor provides INTW interrupts
     *        going into the hardware.
     * 
     * @param name    Symbolic transactor name
     * @param device  UMRBus device number
     * @param bus     UMRBus bus number
     * @param address UMRBus CAPIM address
     * 
     * @return 
     */
    IntMasterXactor(const std::string &name, unsigned int device, unsigned int bus, unsigned int address);
    ~IntMasterXactor();
    /** 
     * @brief Set the output value of the transactor.
     * 
     * @param val Contains the output value
     * 
     * @return true on success
     */
    bool setOutput(const BitT<INTW> &val);
    /** 
     * @brief Get the output value of the transactor.
     * 
     * @param val Receives the output value
     * 
     * @return true on success
     */
    bool getOutput(BitT<INTW> &val);
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
    std::string         *m_last_error; /**< The last error message */
};
