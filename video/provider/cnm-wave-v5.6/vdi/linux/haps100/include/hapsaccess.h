/******************************************************************************
   Title      : HAPS Access library
   Project    : Anaconda/HAPS80
*******************************************************************************
   SYNOPSYS CONFIDENTIAL - This is an unpublished, proprietary work of
   Synopsys, Inc., and is fully protected under copyright and trade secret
   laws. You may not view, use, disclose, copy, or distribute this file or
   any information contained herein except pursuant to a valid written license
   from Synopsys.
******************************************************************************/
/**
 * @file   hapsaccess.h
 *
 * @brief  The HAPS access library with all needed functions
 *
 */
/******************************************************************************
           $Id$
******************************************************************************/

#ifndef __HAPSACCESS_H__
#define __HAPSACCESS_H__

#include <string>
#include <vector>
#include <stdint.h>
#include <tcl.h>
#include "hapsaccess/hapsaccess_defs.h"

#ifdef WINDOWS
#ifndef NODLL
#ifdef _HAPSACCESS_DLL_
#define HAPS_ACCESS_EXP __declspec(dllexport)
#else // _HAPSACCESS_DLL_
#define HAPS_ACCESS_EXP
#endif
#else // NODLL
#define HAPS_ACCESS_EXP
#endif
#else // WINDOWS
#define HAPS_ACCESS_EXP
#endif

/**
 *  @page page_index Index HAPS access library
 *
 *  This is a short descrition how to use HAPS access functions.
 *
 *  @section sec_overview Overview
 *
 *  confpro uses the same commands and general syntax for VU3P as for H70/80/etc
 *  Command parameters (e.g. clock names) may be different but the help system has been improved.
 *  To get the available parameters for a command, simply replace the parameter with an '?'.
 *  This will return a list of all possible values.
 *  In contrary to HAPS80 it will only list those values which can be applied for this specific parameter combination.
 *
 *
 */


/** @brief class CHapsScanElement for system scan */
class HAPS_ACCESS_EXP CHapsScanElement {
public:
    /** @brief Default Constructor */
    CHapsScanElement();
    /** @brief Copy constructor */
    CHapsScanElement(std::string device, e_HapsState system_state, std::string serial_number, std::string system_type, int num_fpgas);
    /** @brief Copy constructor */
    CHapsScanElement(const CHapsScanElement &obj);
    /** @brief Copy constructor */
    CHapsScanElement(const CHapsScanElement *obj);
    /**
     * @brief Get device name/path
     *
     * @return The device name/path
     */
    std::string GetDevice();
    /**
     * @brief Get system state
     *
     * @return The system state
     */
    std::string GetSystemState();
    /**
     * @brief Get serial number
     *
     * @return The serial number
     */
    std::string GetSerialNumber();
    /**
     * @brief Get system type
     *
     * @return The system type
     */
    std::string GetSystemType();
    /**
     * @brief Get number of fpgas
     *
     * @return The number of fpgas
     */
    int GetNumberFpgas();

    std::string m_device;          /**< The UMRBus device name */
    e_HapsState m_system_state;    /**< The current system state */
    std::string m_serial_number;   /**< The system serial number */
    std::string m_system_type;     /**< The system type */
    int         m_num_fpgas;       /**< The number of user FPGAs in the system */
};

/** @brief class CHapsSensorElement for sensor measurements */
class HAPS_ACCESS_EXP CHapsSensorElement {
public:
    /** @brief Default Constructor */
    CHapsSensorElement();
    CHapsSensorElement(const std::string &name, e_HapsSensorUnit unit, uint32_t factor, uint32_t value);
    std::string GetName();
    std::string GetValue();

    std::string m_name;            /**< The sensor name */
    uint32_t m_value;              /**< The sensor value */
    e_HapsSensorUnit m_unit;       /**< The sensor value unit */
    uint32_t m_factor;             /**< The sensor value factor e.g. m_value = 1500, m_factor = 1000, m_unit = HSU_VOLT -> 1.5V */
};

/** @brief class CHapsBoardElement for scanning daughter boards and cables */
class HAPS_ACCESS_EXP CHapsBoardElement {
public:
    CHapsBoardElement();
    CHapsBoardElement(const CHapsBoardElement &obj);

    std::string m_location;   /**< The location ID e.g. FB1.uA.J1 */
    std::string m_stack;      /**< The stack level e.g. D1 */
    std::string m_product;    /**< The product name e.g. CON_CABLE_50_HT3 */
    std::string m_serial;     /**< The serial number e.g. X001234 */
    std::string m_connector;  /**< The board connector e.g. J1 */
    std::string m_voltage;    /**< The voltage IDs e.g. "1V5 1V8" */
    std::string m_article;    /**< The article number e.g. "SH12345678" */
    std::string m_vendor;     /**< The board vendor e.g, Synopsys */
    int m_length;             /**< The length of a cable in cm */
    e_HapsBoardType m_type;   /**< The board type */
};
    
//-------------------------------------------------------------------------------------------

/** @brief class CHaps for library functions */
class HAPS_ACCESS_EXP CHaps {
public:

    //-------------------------------------------------------------------------------------------
    /** 
     * @brief Scan the host PC for connected HAPS systems
     * 
     * @param results Receives the connected HAPS system information
     * 
     * @return true on success
     */
    static bool Scan(std::vector<CHapsScanElement> &results);
    
    //-------------------------------------------------------------------------------------------
    
    /** @brief Default Constructor */
    CHaps();
    /** @brief Destructor */
    ~CHaps();
    
    /**
     * @brief Opens the connection to a HAPS system
     *
     * @param path    UMRBus device number or UMRBus3 path to the HAPS module
     * @param descr   -- unused --
     * @param mapping Hardware mapping string or path to a hardware mapping file (HMF)
     * @param flags   OR combination of e_HapsPrjOpen flags
     *
     * @return True on success
     */
    bool Open(const std::string &path, const std::string &descr, const std::string &mapping, uint32_t flags = HAPSACCESS_PRJ_OPEN_NONE);

    /**
     * @brief Opens the connection to a list of HAPS systems
     *
     * @param path    List of UMRBus device numbers or UMRBus3 paths to the HAPS modules
     * @param descr   -- unused --
     * @param mapping Hardware mapping string or path to a hardware mapping file (HMF)
     * @param flags   OR combination of e_HapsPrjOpen flags
     *
     * @return True on success
     */
    bool Open(const std::vector<std::string> &paths, const std::string &descr, const std::string &mapping, uint32_t flags = HAPSACCESS_PRJ_OPEN_NONE);

    /**
     * @brief Opens the connection to a HAPS system
     *
     * @param path    UMRBus device number or UMRBus3 path to the HAPS module
     * @param mapping Hardware mapping string or path to a hardware mapping file (HMF)
     * @param flags   OR combination of e_HapsPrjOpen flags
     *
     * @return True on success
     */
    bool Open(const std::string &path, const std::string &mapping, uint32_t flags = HAPSACCESS_PRJ_OPEN_NONE);

    /**
     * @brief Opens the connection to a list of HAPS systems
     *
     * @param path    List of UMRBus device numbers or UMRBus3 paths to the HAPS modules
     * @param mapping Hardware mapping string or path to a hardware mapping file (HMF)
     * @param flags   OR combination of e_HapsPrjOpen flags
     *
     * @return True on success
     */
    bool Open(const std::vector<std::string> &paths, const std::string &mapping, uint32_t flags = HAPSACCESS_PRJ_OPEN_NONE);

    /**
     * @brief Opens the connection to a HAPS system
     *
     * @param path    UMRBus device number or UMRBus3 path to the HAPS module
     * @param flags   OR combination of e_HapsPrjOpen flags
     *
     * @return True on success
     */
    bool Open(const std::string &path, uint32_t flags = HAPSACCESS_PRJ_OPEN_NONE);

    /**
     * @brief Opens the connection to a list of HAPS systems
     *
     * @param path    List of UMRBus device numbers or UMRBus3 paths to the HAPS modules
     * @param flags   OR combination of e_HapsPrjOpen flags
     *
     * @return True on success
     */
    bool Open(const std::vector<std::string> &paths, uint32_t flags = HAPSACCESS_PRJ_OPEN_NONE);

    /**
     * @brief Opens the connection to a HAPS system
     *
     * @param scan    A CHapsScan item retrieved with CHaps::Scan()
     * @param flags   OR combination of e_HapsPrjOpen flags
     *
     * @return True on success
     */
    bool Open(const CHapsScanElement &scan, uint32_t flags = HAPSACCESS_PRJ_OPEN_NONE);

    /**
     * @brief Close connection
     */
    bool Close();

    /**
     * @brief Check if connection is established
     */
    bool IsOpen();

    /** 
     * @brief Get the last error message and clear the message buffer
     * 
     * @return the last error message
     */
    std::string GetErrMessage();

    /** 
     * @brief Set the maximum number of parallel jobs
     * 
     * @param n The number of jobs
'
     * @return true on success
     */
    bool SetJobCount(size_t n);

    /** 
     * @brief Get the maximum number of parallel jobs
     * 
     * @param n Receives the number of jobs
     *
     * @return true on success
     */
    bool GetJobCount(size_t &n);
    
    //-------------------------------------------------------------------------------------------
    
    /**
     * @brief Clear the system
     *
     * @param flags       Include/exclude specific settings (OR combination of e_HapsPrjConfig)
     *
     * @return true on success
     */
    bool ProjectClear(uint32_t flags = HAPSACCESS_PRJ_CONFIG_ALL);

    /**
     * @brief Configure the system
     *
     * @param projectFile The project TSS/TSD file
     * @param flags       Include/exclude specific settings (OR combination of e_HapsPrjConfig)
     *
     * @return true on success
     */
    bool ProjectConfigure(const std::string &projectFile, uint32_t flags = HAPSACCESS_PRJ_CONFIG_ALL);

    /**
     * @brief Export the current system configuration
     *
     * @param projectFile The target TSS/TSD/TCL/... file
     *
     * @return true on success
     */
    bool ProjectExport(const std::string &projectFile);

    //-------------------------------------------------------------------------------------------
    
    /**
     * @brief Get the firmware version of the connected system.
     *
     * @param version Receives the version string (Major.Minor)
     * @param id      Optional controller or board id
     *
     * @return true on success
     */
    bool StatusGetFirmwareVersion(std::string &version, const std::string &id = "");

    /**
     * @brief Get the serial number of the connected platform
     *
     * @param serial Receives the serial number
     * @param id     The optional controller or board id
     *
     * @return true on success
     */
    bool StatusGetSerialNumber(std::string &serial, const std::string &id = "");

    /**
     * @brief Get the board type of the connected platform
     *
     * @param type   Receives the board type
     * @param id     The board id
     * @param flags  OR combination of e_HapsSysStruct flags
     *
     * @return true on success
     */
    bool StatusGetBoardType(std::string &type, const std::string &id, uint32_t flags = HAPSACCESS_SYSSTRUCT_NONE);

    /**
     * @brief Get the system log messages
     *
     * @param log   Receives the system log messages
     * @param flags unused
     *
     * @return true on success
     */
    bool StatusGetSystemLog(std::vector<std::string> &log, uint32_t flags = 0);
    
    /**
     * @brief Get the system structure
     *
     * @param structure The received structure
     * @param flags     OR combination of e_HapsSysStruct flags
     *
     * @return true on success
     */
    bool StatusGetSystemStructure(std::vector<std::string> &structure, uint32_t flags = HAPSACCESS_SYSSTRUCT_NONE);

    /**
     * @brief Get the main board names
     *
     * @param boards Receives the main board names
     *
     * @return true on success
     */
    bool StatusGetMainBoards(std::vector<std::string> &boards);

    /**
     * @brief Find all user FFPGAs
     *
     * @param fpgas name of FPGAs found
     * @param board board number
     *
     */
    bool StatusGetUserFpgas(std::vector<std::string> &fpgas, const std::string &board = "");

    /**
     * @brief Get the type of the specified FPGA.
     *
     * @param ftype Receives the FPGA type
     * @param fpga  Contains the FPGA ID
     *
     * @return true on success
     */
    bool StatusGetFpgaType(std::string &ftype, const std::string &fpga);

    /**
     * @brief Get the FPGA configuration state.
     *
     * @param done Receives the done state: 0 or 1
     * @param fpga Contains the FPGA ID
     *
     * @return true on success
     */
    bool StatusGetDone(bool &done, const std::string &fpga);

    /**
     * @brief Get board description for a specified board id
     *
     * Gets the board description text for a specified Board ID
     *
     * @param descr Receives the board description
     * @param board The board id
     *
     * @return true on success
     */
    bool StatusGetDescription(std::string &descr, const std::string &board);

    /**
     * @brief Set the board description for a specified board ID.
     *
     * This function sets the description for a given HAPS System.
     *
     * @param board The board ID
     * @param descr The board description
     *
     * @return true on success
     */
    bool StatusSetDescription(const std::string &board, const std::string &descr);

    /**
     * @brief Get the value of a sensor
     *
     * The sensor can be specified in the following formats:
     *    FB1.uA.TEMP -> The temperature of FB1.uA FPGA (full path)
     *    FB1.uA      -> The sensors of FB1.uA FPGA
     *    FB1.uA.A    -> The sensors of FB1.uA.A (assistant) FPGA
     *    FB1         -> All sensors on FB1
     *
     * @param values  Receives the sensor values
     * @param sensor  The sensor name
     *
     * @return true on success
     */
    bool StatusGetValue(std::vector<CHapsSensorElement> &values, const std::string &sensor);

    //-------------------------------------------------------------------------------------------

    /**
     * @brief Configure the selected FPGA
     *
     * @param fpga  The FPGA ID
     * @param file  The full path to the binary file
     * @param afile The optional full path to the AFPGA binary file (if applicable)
     *
     * @return true on success
     */
    bool ConfigData(const std::string &fpga, const std::string &file);
    bool ConfigData(const std::string &fpga, const std::string &file, const std::string &afile);

    /**
     * @brief Clear the selected FPGA
     *
     * @param fpga  The FPGA ID
     *
     * @return true on success
     */
    bool ConfigClear(const std::string &fpga);

    /**
     * @brief Set the FPGA ID of the selected FPGA
     *
     * @param fpga  The FPGA name
     * @param id    The ID number
     *
     * @return true on success
     */
    bool ConfigSetFpgaId(const std::string &fpga, uint32_t id);

    /**
     * @brief Get the FPGA ID of the selected FPGA
     *
     * @param id    Receives the ID number
     * @param fpga  The FPGA name
     *
     * @return true on success
     */
    bool ConfigGetFpgaId(uint32_t &id, const std::string &fpga);

    /**
     * @brief Set the HANDLE ID of the selected FPGA
     *
     * @param fpga  The FPGA name
     * @param id    The ID number
     *
     * @return true on success
     */
    bool ConfigSetHandleId(const std::string &fpga, uint32_t id);

    /**
     * @brief Get the HANDLE ID of the selected FPGA
     *
     * @param id    Receives the ID number
     * @param fpga  The FPGA name
     *
     * @return true on success
     */
    bool ConfigGetHandleId(uint32_t &id, const std::string &fpga);
    
    /**
     * @brief Get the location ID of the selected FPGA
     *
     * @param id    Receives the ID number
     * @param fpga  The FPGA name
     *
     * @return true on success
     */
    bool ConfigGetLocationId(uint32_t &id, const std::string &fpga);

    /**
     * @brief Set the scratch register of the selected FPGA
     *
     * @param fpga  The FPGA name
     * @param value The scratch value
     *
     * @return true on success
     */
    bool ConfigSetScratch(const std::string &fpga, uint32_t value);

    /**
     * @brief Get the scratch register of the selected FPGA
     *
     * @param value Receives the scratch value
     * @param fpga  The FPGA name
     *
     * @return true on success
     */
    bool ConfigGetScratch(uint32_t &value, const std::string &fpga);


    //-------------------------------------------------------------------------------------------

    /**
     * @brief Select the source for a clock net
     *
     * @param clock  The full alias for clock output
     * @param source The clock source
     *
     * @return true on success
     */
    bool ClockSetSelect(const std::string &clock, const std::string &source);

    /**
     * @brief Select the source for multiple clock nets
     *
     * @param clocks The clock net names
     * @param source The clock source
     *
     * @return true on success
     */
    bool ClockSetSelect(const std::vector<std::string> &clocks, const std::string &source);

    /**
     * @brief Get the selected source for a clock net
     *
     * @param source Receives the driver of the clock net
     * @param clock  The clock net name
     *
     * @return true on success
     */
    bool ClockGetSelect(std::string &source, const std::string &clock);

    /**
     * @brief Get the driving source of the clock net
     *
     * @param source Receives the driver of the clock net
     * @param clock  The full alias for clock output
     *
     * @return true on success
     */
    bool ClockGetDriver(std::string &source, const std::string &clock);
    
    /**
     * @brief Get all nets which are driven by the specified clock net
     *
     * @param sink  Receives the nets which are driven by this clock net
     * @param clock The full alias for clock output
     *
     * @return true on success
     */
    bool ClockGetSink(std::string &sink, const std::string &clock);
    
    /**
     * @brief Get all nets which are driven by the specified clock net
     *
     * @param sinks Receives the nets which are driven by this clock net
     * @param clock The full alias for clock output
     *
     * @return true on success
     */
    bool ClockGetSinks(std::vector<std::string> &sinks, const std::string &clock);

    /**
     * @brief Set a frequency
     *
     * @param clock The full alias for clock output
     * @param freq  The frequency to set in kHz
     *
     * @return True on success
     */
    bool ClockSetFrequency(const std::string &clock, uint32_t freq);

    /**
     * @brief Get a frequency
     *
     * @param freq  The read frequency in kHz
     * @param clock The alias for clock output
     *
     * @return True on success
     */
    bool ClockGetFrequency(uint32_t &freq, const std::string &clock);

    /**
     * @brief Set a clock output active/inactive
     *
     * @param clock  The full alias for clock output
     * @param enable Set to true for active
     *
     * @return True on success
     */
    bool ClockSetEnable(const std::string &clock, bool enable);

    /**
     * @brief Get status of a clock output
     *
     * @param enable True is active
     * @param clock  The full alias for clock output
     *
     * @return True on success
     */
    bool ClockGetEnable(bool &enable, const std::string &clock);

    /**
     * @brief Write the default register values to the PLL
     *
     * @param clock The pll name
     *
     * @return True on success
     */
    bool ClockSetDefault(const std::string &clock);

    /**
     * @brief Calibrate/Reset the pll
     *
     * @param clock The pll name
     *
     * @return True on success
     */
    bool ClockCalibrate(const std::string &clock);

    /**
     * @brief Function for setting a phase shift for an output frequency
     *
     * @param clock  The full alias for clock output
     * @param degree The phase shift in degree (0-180)
     *
     * @return true on success
     */
    bool ClockSetPhase(const std::string &clock, uint16_t degree);

    /**
     * @brief Function for getting a phase shift for an output frequency
     *
     * @param degree The phase shift in degree (0-180)
     * @param clock  The full alias for clock output
     *
     * @return true on success
     */
    bool ClockGetPhase(uint16_t &degree, const std::string &clock);

    /**
     * @brief Get the names of all clock nets.
     *
     * @param nets Receives the names of the clock nets
     * @param path The board ID
     *
     * @return true on success
     */
    bool ClockGetNets(std::vector<std::string> &nets, const std::string &path);

    /**
     * @brief Get the names of all programmable clocks.
     *
     * @param nets Receives the names of the clock nets
     * @param path The board ID
     *
     * @return true on success
     */
    bool ClockGetProgrammables(std::vector<std::string> &nets, const std::string &path);

    /**
     * @brief Check if a clock source is programmable.
     *
     * @param is    The status
     * @param clock The clock ID
     *
     * @return true if the clock source is programmable
     */
    bool ClockIsProgrammable(bool &is, const std::string &clock);

    /**
     * @brief Get the clock cables of the selected connector
     *
     * @param cabless Receives the clock cables
     * @param path    The board or connector ID
     * @param flags   OR combination of e_HapsSysStruct flags
     *
     * @return true on success
     */
    bool ClockGetCables(std::vector<std::string> &cables, const std::string &path, uint32_t flags = HAPSACCESS_SYSSTRUCT_NONE);

    /**
     * @brief Get the clock cables of the selected connector
     *
     * @param cabless Receives the clock cables
     * @param path    The board or connector ID
     * @param flags   OR combination of e_HapsSysStruct flags
     *
     * @return true on success
     */
    bool ClockGetCables(std::vector<CHapsBoardElement> &cables, const std::string &path, uint32_t flags = HAPSACCESS_SYSSTRUCT_NONE);

    /**
     * @brief Get the clock connectors of the specified board
     *
     * @param cons  Receives the names of the connectors
     * @param board The board ID
     *
     * @return true on success
     */
    bool ClockGetConnectors(std::vector<std::string> &cons, const std::string &board);

    //-------------------------------------------------------------------------------------------

    /**
     * @brief Get the names of the reset nets.
     *
     * @param nets  Receives the names of the reset nets
     * @param board The board ID
     *
     * @return true on success
     */
    bool ResetGetNets(std::vector<std::string> &nets, const std::string &board);

    /**
     * @brief Set the source of a reset net.
     *
     * @param reset  The reset net
     * @param value  The value
     *
     * @return true on success
     */
    bool ResetSet(const std::string &reset, bool value);

    /**
     * @brief Set the source of multiple reset nets.
     *
     * @param resets The reset nets
     * @param value  The value
     *
     * @return true on success
     */
    bool ResetSet(const std::vector<std::string> &resets, bool value);

    /**
     * @brief Get the source of a reset net.
     *
     * @param value  The value
     * @param reset  The reset
     *
     * @return true on success
     */
    bool ResetGet(bool &value, const std::string &reset);
    
    //-------------------------------------------------------------------------------------------

    /**
     * @brief Get the HT connectors of the board or the specific HT region
     *
     * @param cons Receives the connectors
     * @param path The board ID
     *
     * @return true on success
     */
    bool HtGetConnectors(std::vector<std::string> &cons, const std::string &path);

    /**
     * @brief Get the HT regions of the selected board
     *
     * @param regions Receives the regions
     * @param path    The board ID
     *
     * @return true on success
     */
    bool HtGetRegions(std::vector<std::string> &regions, const std::string &path);

    /**
     * @brief Get the daughter boards of the selected connector site
     *
     * @param boards Receives the HT daughterboards
     * @param path   The board ID
     *
     * @return true on success
     */
    bool HtGetDaughterBoards(std::vector<std::string> &boards, const std::string &path);

    /**
     * @brief Get the daughter boards of the selected connector site
     *
     * @param boards Receives the HT daughterboards
     * @param path   The board ID
     *
     * @return true on success
     */
    bool HtGetDaughterBoards(std::vector<CHapsBoardElement> &boards, const std::string &path);

    /**
     * @brief Set the IO voltage of the specified voltage region
     *
     * @param con       The connector id
     * @param voltageId The voltage id
     *
     * @return true on success
     */
    bool HtSetVcc(const std::string &con, const std::string &voltageId);
    
    /**
     * @brief Set the IO voltage of the specified voltage regions
     *
     * @param cons      The connector ids
     * @param voltageId The voltage id
     *
     * @return true on success
     */
    bool HtSetVcc(const std::vector<std::string> &cons, const std::string &voltageId);

    /**
     * @brief Get the IO voltage of the specified voltage connector
     *
     * @param voltageId The voltage id
     * @param con       The connector id
     *
     * @return true on success
     */
    bool HtGetVcc(std::string &voltageId, const std::string &con);
    
    /**
     * @brief Check if the platform has DCI settings
     *
     * @param has   True if platform has DCI settings
     * @param board The optional board name
     *
     * @return true on success
     */
    bool HtHasDci(bool &has, const std::string &board);

    /**
     * @brief Enable or disable the connector's resistor
     *
     * @param cons   The connector ids
     * @param enable The enable/disable signal (1/0)
     *
     * @return true on success
     */
    bool HtSetDci(const std::vector<std::string> &cons, bool enable);

    /**
     * @brief Enable or disable the connector's resistor
     *
     * @param con    The connector id
     * @param enable The enable/disable signal (1/0)
     *
     * @return true on success
     */
    bool HtSetDci(const std::string &con, bool enable);

    /**
     * @brief Get the status of the connector's resistors
     *
     * @param enable The enable/disable signal (1/0)
     * @param con    The connector id
     *
     * @return true on success
     */
    bool HtGetDci(bool &enable, const std::string &con);
    
    /**
     * @brief Check if the platform has Vref settings
     *
     * @param has   True if platform has Vref settings
     * @param board The optional board name
     *
     * @return true on success
     */
    bool HtHasVref(bool &has, const std::string &board);

    /**
     * @brief Set the vref IO voltage percentage of the specified voltage region
     *
     * @param cons    The connector ids
     * @param percent The vref setting in percent of the set vccio
     *
     * @return true on success
     */
    bool HtSetVref(const std::vector<std::string> &cons, uint8_t percent);

    /**
     * @brief Set the vref IO voltage percentage of the specified voltage region
     *
     * @param con     The connector id
     * @param percent The vref setting in percent of the set vccio
     *
     * @return true on success
     */
    bool HtSetVref(const std::string &con, uint8_t percent);

    /**
     * @brief Get the vref IO voltage percentage of the specified voltage region
     *
     * @param percent The vref setting in percent of the set vccio
     * @param con     The connector id
     *
     * @return true on success
     */
    bool HtGetVref(uint8_t &percent, const std::string &con);

    /**
     * @brief Write data into the daughterboard IDPROM of the specified connector
     *
     * @param con       The connector id e.g. FB1.uA.J1
     * @param db        The daughterboard id e.g. D1
     * @param regaddr   The register address
     * @param data      The data to be written
     *
     * @return true on success
     */
    bool HtWriteProm(const std::string &con, const std::string &db, uint8_t regaddr, const std::vector<uint8_t> &data);
    
    /**
     * @brief Read data from the daughterboard IDPROM of the specified connector
     *
     * @param data      Receives the data
     * @param con       The connector id e.g. FB1.uA.J1
     * @param db        The daughterboard id e.g. D1
     * @param regaddr   The register address
     * @param count     The number of bytes to be read
     *
     * @return true on success
     */
    bool HtReadProm(std::vector<uint8_t> &data, const std::string &con, const std::string &db, uint8_t regaddr, uint8_t count);

    //-------------------------------------------------------------------------------------------

    /**
     * @brief Get the LEDs of the board or the specific HT region
     *
     * @param leds  Receives the LEDs
     * @param board The board name
     *
     * @return true on success
     */
    bool LedGetLeds(std::vector<std::string> &leds, const std::string &board);

    /**
     * @brief Set the color of the specified LED
     *
     * @param led    The LED id
     * @param color  The color as 0x00RRGGBB value
     *
     * @return true on success
     */
    bool LedSet(const std::string &led, uint32_t color);

    /**
     * @brief Set the color of the specified LED
     *
     * @param led      The LED id
     * @param color    The color as 0x00RRGGBB value
     * @param altcolor The alternating color for blink modes as 0x00RRGGBB value
     * @param flags    The blink mode flags @todo: TBD
     *
     * @return true on success
     */
    bool LedSet(const std::string &led, uint32_t color, uint32_t altcolor, uint32_t flags);

    /**
     * @brief Set the color of the specified LEDs
     *
     * @param leds   The LED ids
     * @param color  The color as 0x00RRGGBB value
     *
     * @return true on success
     */
    bool LedSet(const std::vector<std::string> &leds, uint32_t color);

    /**
     * @brief Set the color of the specified LEDs
     *
     * @param leds     The LED ids
     * @param color    The color as 0x00RRGGBB value
     * @param altcolor The alternating color for blink modes as 0x00RRGGBB value
     * @param flags    The blink mode flags @todo: TBD
     *
     * @return true on success
     */
    bool LedSet(const std::vector<std::string> &leds, uint32_t color, uint32_t altcolor, uint32_t flags);

    /**
     * @brief Get the color of the specified LED
     *
     * @param color  Receives the color as 0x00RRGGBB value
     * @param led    The LED id
     *
     * @return true on success
     */
    bool LedGet(uint32_t &color, const std::string &led);

    /**
     * @brief Get the color of the specified LED
     *
     * @param color    Receives the color as 0x00RRGGBB value
     * @param altcolor Receives the alternating color for blink modes as 0x00RRGGBB value
     * @param flags    Receives the blink mode flags @todo: TBD
     * @param led      The LED id
     *
     * @return true on success
     */
    bool LedGet(uint32_t &color, uint32_t &altcolor, uint32_t &flags, const std::string &led);

    //-------------------------------------------------------------------------------------------

    /**
     * @brief Get the temperature value of a sensor
     *
     * The sensor can be specified in the following formats:
     *    FB1.uA.TEMP -> The temperature of FB1.uA FPGA (full path)
     *    FB1.uA      -> The temperature of FB1.uA FPGA
     *    FB1.uA.A    -> The temperature of FB1.uA.A (assistant) FPGA
     *    FB1         -> All temperature sensors on FB1
     *
     * @param values  Receives the sensor values
     * @param sensor  The sensor name
     *
     * @return true on success
     */
    bool TempGet(std::vector<CHapsSensorElement> &values, const std::string &sensor);

    //-------------------------------------------------------------------------------------------

    /**
     * @brief Get the MGB connectors of the specified board
     *
     * @param cons  Receives the names of the connectors
     * @param board The board ID
     *
     * @return true on success
     */
    bool ConGetConnectors(std::vector<std::string> &cons, const std::string &board);

    /**
     * @brief Get the MGB daughter boards of the selected connector site
     *
     * @param boards Receives the MGB daughterboards
     * @param path   The board ID
     *
     * @return true on success
     */
    bool ConGetDaughterBoards(std::vector<std::string> &boards, const std::string &path);

    /**
     * @brief Get the MGB daughter boards of the selected connector site
     *
     * @param boards Receives the MGB daughterboards
     * @param path   The board ID
     *
     * @return true on success
     */
    bool ConGetDaughterBoards(std::vector<CHapsBoardElement> &boards, const std::string &path);

    //-------------------------------------------------------------------------------------------
    
private:
    void* m_pimpl;  /**< Pointer to implementation */


    //-------------------------------------------------------------------------------------------

public:
    /**
     * @brief Init
     *
     * @param tcl   tcl
     * @param argv0 The arguments
     *
     * @return true on success
     */
    static bool Init(Tcl_Interp *tcl, char* argv0);

    //-------------------------------------------------------------------------------------------

public:
    
    /**
     * @brief Enable debugging
     *
     * @param level The debug level. Higher numbers increase verbosity, 0 disables messages
     *
     */    
    static void SetDebugEnable(int level);

    /**
     * @brief Set the message callback function
     *
     * @param cb Message callback function
     *
     */
    static void SetMessageCallback(tHapsMessageCallBack cb);

    /**
     * @brief Set the progress callback function
     *
     * @param cb Progress callback function
     *
     */
    static void SetProgressCallback(tHapsProgressCallBack cb);

      
};

#endif // __HAPSACCESS_H__
