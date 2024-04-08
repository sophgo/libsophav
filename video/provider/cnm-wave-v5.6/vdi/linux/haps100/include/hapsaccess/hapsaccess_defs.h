
#ifndef __HAPSACCESS_DEFS_H__
#define __HAPSACCESS_DEFS_H__

//-------------------------------------------------------------------------------------------

#define HAPSACCESS_FLAG_SET(VAL,FLAG)     (VAL |  FLAG)          /**< FLAG_SET */
#define HAPSACCESS_FLAG_CLEAR(VAL,FLAG)   (VAL & ~FLAG)          /**< FLAG_CLEAR */
#define HAPSACCESS_FLAG_IS_SET(VAL,FLAG) ((VAL &  FLAG) == FLAG) /**< FLAG_IS_SET */

//-------------------------------------------------------------------------------------------

/** @enum e_HapsPrjOpen
 *  @brief Control ProjectOpen
 */
typedef enum {
    HAPSACCESS_PRJ_OPEN_NONE      =  0,                   /**< No flags */
    HAPSACCESS_PRJ_OPEN_FORCE     =  (1 <<  0)            /**< Force opening a system even if an error is present */
} e_HapsPrjOpen;

//-------------------------------------------------------------------------------------------

/** @enum e_HapsPrjConfig
 *  @brief Include/exclude specific configuration settings
 */
typedef enum {
    HAPSACCESS_PRJ_CONFIG_NONE            =  0,                   /**< Exclude all settings */
    HAPSACCESS_PRJ_CONFIG_CLEAR           =  (1 <<  0),           /**< Include clearing of the system */
    HAPSACCESS_PRJ_CONFIG_CLOCK_SELECT    =  (1 <<  1),           /**< Include clock selection */
    HAPSACCESS_PRJ_CONFIG_CLOCK_ENABLE    =  (1 <<  2),           /**< Include clock enable settings */
    HAPSACCESS_PRJ_CONFIG_CLOCK           =  (3 <<  1),           /**< Include complete clock configuration i.e. select and enable */
    HAPSACCESS_PRJ_CONFIG_HT3             =  (1 <<  3),           /**< Include HT3 voltage settings */
    HAPSACCESS_PRJ_CONFIG_RESET           =  (1 <<  4),           /**< Include reset settings (HAPS80) */
    HAPSACCESS_PRJ_CONFIG_FPGA            =  (1 <<  5),           /**< Include FPGA configuration */
    HAPSACCESS_PRJ_CONFIG_HSTDM           =  (1 <<  6),           /**< Include HSTDM training (HAPS80) */
    HAPSACCESS_PRJ_CONFIG_CON             =  (1 <<  7),           /**< Include MGB connector settings */
    HAPSACCESS_PRJ_CONFIG_ALL             =  0xFFFFFFFF           /**< Include all settings */
} e_HapsPrjConfig;

//-------------------------------------------------------------------------------------------

/** @enum e_HapsSysStruct
 *  @brief Control StatusGetSystemStructure
 */
typedef enum {
    HAPSACCESS_SYSSTRUCT_NONE     =  0,                   /**< No flags, TCL output */
    HAPSACCESS_SYSSTRUCT_VERBOSE  =  (1 <<  0),           /**< Verbose, human readable format */
    HAPSACCESS_SYSSTRUCT_BOM      =  (1 <<  1),           /**< BOM style output */
    HAPSACCESS_SYSSTRUCT_FULL     =  (1 <<  2)            /**< full scan */
} e_HapsSysStruct;

//-------------------------------------------------------------------------------------------

/** @enum e_HapsState
 *  @brief State of the HAPS system
 */
typedef enum {
    HS_INVALID,   /**< The HAPS system state is invalid */
    HS_AVAILABLE, /**< The HAPS system is available */
    HS_LOCKED,    /**< The HAPS system is locked by another user */
    HS_ERROR      /**< The HAPS system is in an error state */
} e_HapsState;

//-------------------------------------------------------------------------------------------

/** @enum e_HapsSensorUnit
 *  @brief The unit of a sensor value
 */
typedef enum {
    HSU_INVALID,   /**< The unit is invalid */
    HSU_VOLT,      /**< Volt */
    HSU_DEGC,      /**< Degree Centigrade */
    HSU_DEGF       /**< Degree Fahrenheit */
} e_HapsSensorUnit;

//-------------------------------------------------------------------------------------------

/** @enum e_HapsBoardType
 *  @brief The type of a CHapsBoardElement
 */
typedef enum {
    HBT_INVALID,        /**< The board type is invalid */
    HBT_DAUGHTERBOARD,  /**< A daughterboard */
    HBT_HT3_CABLE       /**< An HT3 cable */
} e_HapsBoardType;

//-------------------------------------------------------------------------------------------

/** @enum e_MsgType
 *  @brief text types for messages
 */
typedef enum {
    MT_DEBUG, /**< Text type Debug message*/
    MT_WARN,  /**< Text type Warning*/
    MT_INFO,  /**< Text type Information*/
    MT_ERROR  /**< Text type Error*/
} e_MsgType;

/** @brief type definition for callback function for messages */
typedef void(*tHapsMessageCallBack) (e_MsgType mtype, char* buf);

/** @brief type definition for callback function for progress messages */
typedef void(*tHapsProgressCallBack) (int min, int max, int current, const char* msg);

//-------------------------------------------------------------------------------------------

#endif
