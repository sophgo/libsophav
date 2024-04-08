
/******************************************************************************
   Title      : HAPS Access library
   *******************************************************************************
   SYNOPSYS CONFIDENTIAL - This is an unpublished, proprietary work of
   Synopsys, Inc., and is fully protected under copyright and trade secret
   laws. You may not view, use, disclose, copy, or distribute this file or
   any information contained herein except pursuant to a valid written license
   from Synopsys.
******************************************************************************/
/**
 * @file   hapsaccessc.h
 *
 * @brief  The HAPS access C-library with all needed functions
 *
 */
/******************************************************************************
           $Id$
******************************************************************************/

#ifndef __HAPSACCESSC_H__
#define __HAPSACCESSC_H__

#include <stdint.h>
#include <tcl.h>
#include "hapsaccess/hapsaccess_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WINDOWS
#ifndef NODLL
#ifdef _HAPSACCESSC_DLL_
#define HAPS_ACCESSC_EXP __declspec(dllexport)
#else // _HAPSACCESS_DLL_
#define HAPS_ACCESSC_EXP
#endif
#else // NODLL
#define HAPS_ACCESSC_EXP
#endif
#else // WINDOWS
#define HAPS_ACCESSC_EXP
#endif

#define HAPS_HANDLE size_t
#define HAPS_OK     0
#define HAPS_ERROR  1


    //-----------------------------------------------------------------------------
    // Library initialization functions
    //-----------------------------------------------------------------------------
    
    /** 
     * @brief Initialize the hapsaccess C-library
     * 
     * @param tcl      The TCL interpreter has to be created in the main application
     * @param argv0    The argv0 argument of the main application
     */
    extern void HAPS_ACCESSC_EXP haps_init(Tcl_Interp *tcl, char *argv0);

    /**
     * @brief Enable debugging
     *
     * @param level The debug level. Higher numbers increase verbosity, 0 disables messages
     */    
    extern void HAPS_ACCESSC_EXP haps_set_debug_enable(int level);

    /**
     * @brief Set the message callback function
     *
     * @param cb Message callback function
     *
     */
    extern void HAPS_ACCESSC_EXP haps_set_message_callback(tHapsMessageCallBack cb);

    /** 
     * @brief Free the message buffer returned by a haps_* function
     * 
     * @param msg The message buffer
     */
    extern void HAPS_ACCESSC_EXP haps_msg_free(char** msg);

    //-----------------------------------------------------------------------------
    // Open/Close functions
    //-----------------------------------------------------------------------------

    /** 
     * @brief Open a connection to a HAPS system
     * 
     * @param cfg      Receives the configuration handle
     * @param path     The UMR3 path or device of the system
     * @param mapping  The optional system mapping
     * @param flags    OR combination of e_HapsPrjOpen flags
     * @param retmsg   Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_open(HAPS_HANDLE *cfg, char *path, char *mapping, uint32_t flags, char **retmsg);
    
    /** 
     * @brief Close a connection to a HAPS system
     * 
     * @param cfg      The configuration handle
     * @param retmsg   Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */    
    extern int HAPS_ACCESSC_EXP haps_close(HAPS_HANDLE cfg, char **retmsg);
    

    //-----------------------------------------------------------------------------
    // Project functions
    //-----------------------------------------------------------------------------

    /**
     * @brief Clear the system
     *
     * @param cfg      The configuration handle
     * @param flags    Include/exclude specific settings (OR combination of e_HapsPrjConfig)
     * @param retmsg   Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_project_clear(HAPS_HANDLE cfg, uint32_t flags, char **retmsg);
    
    /**
     * @brief Configure the system
     *
     * @param cfg         The configuration handle
     * @param projectFile The project TSS/TSD file
     * @param flags       Include/exclude specific settings (OR combination of e_HapsPrjConfig)
     * @param retmsg      Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_project_configure(HAPS_HANDLE cfg, char* projectFile, uint32_t flags, char **retmsg);

    /**
     * @brief Export the current system configuration
     *
     * @param cfg         The configuration handle
     * @param projectFile The target project TSS/TSD/TCL/... file
     * @param retmsg      Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_project_export(HAPS_HANDLE cfg, char* projectFile, char **retmsg);

    //-----------------------------------------------------------------------------
    // Status functions
    //-----------------------------------------------------------------------------

    /**
     * @brief Get the firmware version of the connected system.
     *
     * @param cfg     The configuration handle
     * @param id      Optional controller or board id
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_status_get_firmware_version(HAPS_HANDLE cfg, char* id, char **retmsg);

    /**
     * @brief Get the firmware version of the connected system.
     *
     * @param cfg     The configuration handle
     * @param id      Optional controller or board id
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_status_get_serial_number(HAPS_HANDLE cfg, char* id, char **retmsg);

    /**
     * @brief Get the board type of the connected system.
     *
     * @param cfg     The configuration handle
     * @param id      The board id
     * @param flags   OR combination of e_HapsSysStruct flags

     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_status_get_board_type(HAPS_HANDLE cfg, char* id, uint32_t flags, char **retmsg);

    /**
     * @brief Get the system log messages
     *
     * @param cfg     The configuration handle
     * @param flags   currently unused
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_status_get_system_log(HAPS_HANDLE cfg, uint32_t flags, char **retmsg);

    /**
     * @brief Get the system structure
     *
     * @param cfg     The configuration handle
     * @param flags   OR combination of e_HapsSysStruct flags
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_status_get_system_structure(HAPS_HANDLE cfg, uint32_t flags, char **retmsg);

    /**
     * @brief Get the main boards of the system
     *
     * @param cfg     The configuration handle
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_status_get_main_boards(HAPS_HANDLE cfg, char **retmsg);

    /**
     * @brief Get the user FPGAs of the system
     *
     * @param cfg     The configuration handle
     * @param board   The optional board ID
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_status_get_user_fpgas(HAPS_HANDLE cfg, char *board, char **retmsg);

    /**
     * @brief Get the FPGA type
     *
     * @param cfg     The configuration handle
     * @param fpga    The FPGA ID
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_status_get_fpga_type(HAPS_HANDLE cfg, char *fpga, char **retmsg);

    /**
     * @brief Get the FPGA configuration state
     *
     * @param cfg     The configuration handle
     * @param fpga    The FPGA ID
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_status_get_done(HAPS_HANDLE cfg, char *fpga, char **retmsg);

    /**
     * @brief Get the board description
     *
     * @param cfg     The configuration handle
     * @param board   The board ID
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_status_get_description(HAPS_HANDLE cfg, char *board, char **retmsg);

    /**
     * @brief Set the board description
     *
     * @param cfg     The configuration handle
     * @param board   The board ID
     * @param descr   The description sctring
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_status_set_description(HAPS_HANDLE cfg, char *board, char *descr, char **retmsg);

    /**
     * @brief Get the sensor values
     *
     * @param cfg     The configuration handle
     * @param id      The board, fpga or sensor ID
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_status_get_value(HAPS_HANDLE cfg, char* id, char **retmsg);

    //-----------------------------------------------------------------------------
    // Config functions
    //-----------------------------------------------------------------------------

    /**
     * @brief Configure the selected FPGA
     *
     * @param cfg       The configuration handle
     * @param fpga      The FPGA ID
     * @param filename  The path to the configuration file
     * @param afilename The optional full path to the AFPGA binary file (if applicable)
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_config_data(HAPS_HANDLE cfg, char *fpga, char *filename, char *afilename, char **retmsg);

    /**
     * @brief Clear the selected FPGA
     *
     * @param cfg       The configuration handle
     * @param fpga      The FPGA ID
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_config_clear(HAPS_HANDLE cfg, char *fpga, char **retmsg);

    /**
     * @brief Set the FPGA ID of the selected FPGA
     *
     * @param cfg       The configuration handle
     * @param fpga      The FPGA name
     * @param id        The id to be written
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_config_set_fpga_id(HAPS_HANDLE cfg, char *fpga, uint32_t id, char **retmsg);

    /**
     * @brief Get the FPGA ID of the selected FPGA
     *
     * @param cfg       The configuration handle
     * @param fpga      The FPGA name
     * @param id        Receives the id
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_config_get_fpga_id(HAPS_HANDLE cfg, char *fpga, uint32_t *id, char **retmsg);

    /**
     * @brief Set the handle ID of the selected FPGA
     *
     * @param cfg       The configuration handle
     * @param fpga      The FPGA name
     * @param id        The id to be written
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_config_set_handle_id(HAPS_HANDLE cfg, char *fpga, uint32_t id, char **retmsg);

    /**
     * @brief Get the handle ID of the selected FPGA
     *
     * @param cfg       The configuration handle
     * @param fpga      The FPGA name
     * @param id        Receives the id
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_config_get_handle_id(HAPS_HANDLE cfg, char *fpga, uint32_t *id, char **retmsg);

    /**
     * @brief Get the location ID of the selected FPGA
     *
     * @param cfg       The configuration handle
     * @param fpga      The FPGA name
     * @param id        Receives the id
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_config_get_location_id(HAPS_HANDLE cfg, char *fpga, uint32_t *id, char **retmsg);

    /**
     * @brief Set the scratch register of the selected FPGA
     *
     * @param cfg       The configuration handle
     * @param fpga      The FPGA name
     * @param value     The value to be written
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_config_set_scratch(HAPS_HANDLE cfg, char *fpga, uint32_t value, char **retmsg);

    /**
     * @brief Get the scratch register of the selected FPGA
     *
     * @param cfg       The configuration handle
     * @param fpga      The FPGA name
     * @param value     Receives the value
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_config_get_scratch(HAPS_HANDLE cfg, char *fpga, uint32_t *value, char **retmsg);



    //-----------------------------------------------------------------------------
    // Clock functions
    //-----------------------------------------------------------------------------

    /**
     * @brief Select the source for multiple clock nets
     *
     * @param cfg       The configuration handle
     * @param clocks    The clock net list seperated by spaces
     * @param source    The clock source
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_set_select(HAPS_HANDLE cfg, char *clocks, char *source, char **retmsg);

    /**
     * @brief Get the selected source of a clock net
     *
     * @param cfg       The configuration handle
     * @param clocks    The clock net
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_get_select(HAPS_HANDLE cfg, char *clock, char **retmsg);

    /**
     * @brief Get the driver of a clock net
     *
     * @param cfg       The configuration handle
     * @param clocks    The clock net
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_get_driver(HAPS_HANDLE cfg, char *clock, char **retmsg);

    /**
     * @brief Get all nets which are driven by the specified clock net
     *
     * @param cfg       The configuration handle
     * @param clocks    The clock net
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_get_sinks(HAPS_HANDLE cfg, char *clock, char **retmsg);

    /**
     * @brief Set the clock frequency
     *
     * @param cfg       The configuration handle
     * @param clock     The clock name
     * @param value     The frequency value in kHz
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_set_frequency(HAPS_HANDLE cfg, char *clock, uint32_t value, char **retmsg);

    /**
     * @brief Get the clock frequency
     *
     * @param cfg       The configuration handle
     * @param clock     The clock name
     * @param value     Receives the frequency value in kHz
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_get_frequency(HAPS_HANDLE cfg, char *clock, uint32_t *value, char **retmsg);

    /**
     * @brief Set the clock enable value
     *
     * @param cfg       The configuration handle
     * @param clock     The clock name
     * @param value     The enable value
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_set_enable(HAPS_HANDLE cfg, char *clock, uint32_t value, char **retmsg);

    /**
     * @brief Get the clock enable value
     *
     * @param cfg       The configuration handle
     * @param clock     The clock name
     * @param value     Receives the enable value
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_get_enable(HAPS_HANDLE cfg, char *clock, uint32_t *value, char **retmsg);

    /**
     * @brief Write the default register settings to the PLL
     *
     * @param cfg       The configuration handle
     * @param clock     The PLL name
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_set_default(HAPS_HANDLE cfg, char *clock, char **retmsg);

    /**
     * @brief Calibrate the PLL
     *
     * @param cfg       The configuration handle
     * @param clock     The PLL name
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_calibrate(HAPS_HANDLE cfg, char *clock, char **retmsg);

    /**
     * @brief Set the clock phase shift value
     *
     * @param cfg       The configuration handle
     * @param clock     The clock name
     * @param value     The phase shift value in degree (0-180)
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_set_phase(HAPS_HANDLE cfg, char *clock, uint32_t value, char **retmsg);

    /**
     * @brief Get the clock phase shift value
     *
     * @param cfg       The configuration handle
     * @param clock     The clock name
     * @param value     Receives the phase shift value in degree
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_get_phase(HAPS_HANDLE cfg, char *clock, uint32_t *value, char **retmsg);

    /**
     * @brief Get the names of all clock nets
     *
     * @param cfg       The configuration handle
     * @param path      The search path
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_get_nets(HAPS_HANDLE cfg, char *path, char **retmsg);

    /**
     * @brief Get the names of all programmable clocks
     *
     * @param cfg       The configuration handle
     * @param path      The search path
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_get_programmables(HAPS_HANDLE cfg, char *path, char **retmsg);

    /**
     * @brief Check if a clock source is programmable
     *
     * @param cfg       The configuration handle
     * @param clock     The clock name
     * @param value     Receives the result value, 1 or 0
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_is_programmable(HAPS_HANDLE cfg, char *clock, uint32_t *value, char **retmsg);

    /**
     * @brief Get the clock cables
     *
     * @param cfg     The configuration handle
     * @param path    The search path
     * @param flags   OR combination of e_HapsSysStruct flags
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_get_cables(HAPS_HANDLE cfg, char *path, uint32_t flags, char **retmsg);

    /**
     * @brief Get the clock connectors
     *
     * @param cfg     The configuration handle
     * @param board   The board name
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_clock_get_connectors(HAPS_HANDLE cfg, char *board, char **retmsg);

    //-----------------------------------------------------------------------------
    // Reset functions
    //-----------------------------------------------------------------------------

    /**
     * @brief Get the names of all reset nets
     *
     * @param cfg     The configuration handle
     * @param board   The board name
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_reset_get_nets(HAPS_HANDLE cfg, char *board, char **retmsg);

    /**
     * @brief Set the reset value
     *
     * @param cfg       The configuration handle
     * @param reset     The reset name
     * @param value     The reset value
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_reset_set(HAPS_HANDLE cfg, char *reset, uint32_t value, char **retmsg);

    /**
     * @brief Get the reset value
     *
     * @param cfg       The configuration handle
     * @param reset     The reset name
     * @param value     Receives the reset value
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_reset_get(HAPS_HANDLE cfg, char *reset, uint32_t *value, char **retmsg);


    //-----------------------------------------------------------------------------
    // HT functions
    //-----------------------------------------------------------------------------

    /**
     * @brief Get all HT connectors
     *
     * @param cfg     The configuration handle
     * @param board   The board name
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_ht_get_connectors(HAPS_HANDLE cfg, char *board, char **retmsg);

    /**
     * @brief Get all HT voltage regions
     *
     * @param cfg     The configuration handle
     * @param board   The board name
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_ht_get_regions(HAPS_HANDLE cfg, char *board, char **retmsg);

    /**
     * @brief Get all HT daughter boards
     *
     * @param cfg     The configuration handle
     * @param path    The search path
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_ht_get_boards(HAPS_HANDLE cfg, char *path, char **retmsg);

    /**
     * @brief Set the HT IO voltage
     *
     * @param cfg     The configuration handle
     * @param cons    The list of connectors
     * @param vcc     The voltage setting
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_ht_set_vcc(HAPS_HANDLE cfg, char *cons, char *vcc, char **retmsg);

    /**
     * @brief Get the HT IO voltage
     *
     * @param cfg     The configuration handle
     * @param con     The HT connector
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_ht_get_vcc(HAPS_HANDLE cfg, char *con, char **retmsg);

    /**
     * @brief Check if the board has DCI settings
     *
     * @param cfg       The configuration handle
     * @param board     The board name
     * @param value     Receives the result value, 0 or 1
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_ht_has_dci(HAPS_HANDLE cfg, char *board, uint32_t *value, char **retmsg);

    /**
     * @brief Set the HT DCI resistor setting
     *
     * @param cfg     The configuration handle
     * @param cons    The list of connectors
     * @param value   The DCI enable value, 0 or 1
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_ht_set_dci(HAPS_HANDLE cfg, char *cons, uint32_t value, char **retmsg);

    /**
     * @brief Get the HT DCI resistor setting
     *
     * @param cfg     The configuration handle
     * @param con     The HT connector
     * @param value   Receives the DCI enable value, 0 or 1
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_ht_get_dci(HAPS_HANDLE cfg, char *con, uint32_t *value, char **retmsg);

    /**
     * @brief Check if the board has VREF settings
     *
     * @param cfg       The configuration handle
     * @param board     The board name
     * @param value     Receives the result value, 0 or 1
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_ht_has_vref(HAPS_HANDLE cfg, char *board, uint32_t *value, char **retmsg);

    /**
     * @brief Set the HT VREF setting
     *
     * @param cfg     The configuration handle
     * @param cons    The list of connectors
     * @param value   The VREF value in percent
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_ht_set_vref(HAPS_HANDLE cfg, char *cons, uint32_t value, char **retmsg);

    /**
     * @brief Get the HT VREF setting
     *
     * @param cfg     The configuration handle
     * @param con     The HT connector
     * @param value   Receives the VREF value in percent
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_ht_get_vref(HAPS_HANDLE cfg, char *con, uint32_t *value, char **retmsg);

    /**
     * @brief Write data into the daughterboard IDPROM of the specified connector
     *
     * @param cfg       The configuration handle
     * @param con       The connector id e.g. FB1.uA.J1
     * @param db        The daughterboard id e.g. D1
     * @param regaddr   The register address
     * @param count     The number of bytes to be written
     * @param data      The data to be written
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_ht_write_prom(HAPS_HANDLE cfg, char *con, char *db, uint8_t regaddr, uint8_t count, uint8_t *data, char **retmsg);

    /**
     * @brief Read data from the daughterboard IDPROM of the specified connector
     *
     * @param cfg       The configuration handle
     * @param con       The connector id e.g. FB1.uA.J1
     * @param db        The daughterboard id e.g. D1
     * @param regaddr   The register address
     * @param count     The number of bytes to be read
     * @param data      Receives the read data. Buffer must be large enough to receive count bytes.
     * @param retmsg    Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */    
    extern int HAPS_ACCESSC_EXP haps_ht_read_prom(HAPS_HANDLE cfg, char *con, char *db, uint8_t regaddr, uint8_t count, uint8_t *data, char **retmsg);
    
    
    //-----------------------------------------------------------------------------
    // CON functions
    //-----------------------------------------------------------------------------

    /**
     * @brief Get all CON daughter boards
     *
     * @param cfg     The configuration handle
     * @param path    The search path
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_con_get_boards(HAPS_HANDLE cfg, char *path, char **retmsg);

    /**
     * @brief Get all CON connectors
     *
     * @param cfg     The configuration handle
     * @param board   The board name
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_con_get_connectors(HAPS_HANDLE cfg, char *board, char **retmsg);

    //-----------------------------------------------------------------------------
    // LED functions
    //-----------------------------------------------------------------------------

    /**
     * @brief Get all LEDs
     *
     * @param cfg     The configuration handle
     * @param path    The search path
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_led_get_leds(HAPS_HANDLE cfg, char *path, char **retmsg);

    /**
     * @brief Set the color of the selected LEDs
     *
     * @param cfg     The configuration handle
     * @param leds    The list of LEDs
     * @param value   The LED color as RGB value
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_led_set(HAPS_HANDLE cfg, char *leds, uint32_t value, char **retmsg);

    /**
     * @brief Get the color of the selected LED
     *
     * @param cfg     The configuration handle
     * @param led     The LED
     * @param value   Receives the LED color as RGB value
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_led_get(HAPS_HANDLE cfg, char *led, uint32_t *value, char **retmsg);


    //-----------------------------------------------------------------------------
    // Temp functions
    //-----------------------------------------------------------------------------

    /**
     * @brief Get the temperature sensor values
     *
     * @param cfg     The configuration handle
     * @param id      The board, fpga or sensor ID
     * @param retmsg  Receives the return or error message
     * 
     * @return HAPS_OK on success, HAPS_ERROR otherwise
     */
    extern int HAPS_ACCESSC_EXP haps_temp_get(HAPS_HANDLE cfg, char* id, char **retmsg);


#ifdef __cplusplus
}
#endif

#endif

