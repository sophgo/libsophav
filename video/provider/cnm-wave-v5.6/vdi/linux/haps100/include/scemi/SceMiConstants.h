/******************************************************************************
 Copyright © 2009 Synopsys, Inc. This Example and the associated documentation
 are confidential and proprietary to Synopsys, Inc. Your use or disclosure of
 this example is subject to the terms and conditions of a written license
 agreement between you, or your company, and Synopsys, Inc.
 *******************************************************************************
 Title      : SCE-MI Object definitions
 Project    : SCE-MI 2 Implementation
 ******************************************************************************/
/**
 * @file   SceMiConstants.h
 * @author  Roshan Antony Tauro
 * @date   2006/11/30
 *
 * @brief  Various Constants used in the Library.
 *
 *
 */
/******************************************************************************
 $Id: //chipit/chipit/main/release/tools/scemi2/sw/include/SceMiConstants.h#2 $ $Author: mlange $ $DateTime: 2017/02/02 05:27:09 $
 ******************************************************************************/

#ifndef __SceMiConstants_h__
#define __SceMiConstants_h__

/*SceMi Version Constants */
#define SCEMI_MAJOR_VERSION		2
#define SCEMI_MINOR_VERSION		0
#define SCEMI_PATCH_VERSION		0
#define SCEMI_VERSION_STRING	"2.0.0"

/* Interrupt/Data FIFO Fill Limits */
#define DATA_FIFO_FILL_MAX		2048
#define INTERRUPT_FIFO_FILL_MAX	2048

/* Constraints on the Buffers Used for SceMi 2.0 Pipes */
#define PIPE_BUFFER_MAX_DEPTH	512
#define PIPE_FIFO_FILL_MAX		512
#define PIPE_MAX_SEND_LIMIT		0x1FFFFFFF

/* SceMi Interrupt Commands */
#define I_FLUSH_SEND_PIPE			0x01
#define I_IS_READY_MESSAGE			0x02
#define I_READ_READY				0x03
#define I_EXPORT_FKT_READY_PIPE		0x04
#define I_IMPORT_FKT_CALL			0x05
#define I_RECEIVE_MESSAGE			0x06
#define I_FLUSH_ACK_RECEIVE_PIPE	0x07
#define I_SEND_PIPE_HAS_COUNT(VAL) (((VAL & 0xFFFFF000) == 0x0000F000) ? 1 : 0)
#define I_SEND_PIPE_COUNT(VAL) (((VAL & 0xFFFFF000) == 0x0000F000) ? (VAL & 0x03FF) : 0)
#define I_INT_FIFO_HAS_COUNT(VAL) (((VAL & 0xFFFFF000) == 0x0000F000) ? 1 : 0)
#define I_INT_FIFO_COUNT(VAL) (((VAL & 0xFFFFF000) == 0x0000F000) ? (VAL & 0x03FF) : 0)

/* vpi constants */
#define vpiScaledRealTime    0
#define vpiSimTime    1
#define vpiTimePrecision    1

/* Other General Purpose Constants */
#define ALLONES32	0xFFFFFFFF

#endif
