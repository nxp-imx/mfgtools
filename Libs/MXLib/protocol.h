/*****************************************************************************
** protocol.h
**
** Copyright 2007 Freescale Semiconductor, Inc. All Rights Reserved.
**
** This file contains copyrighted material. Use of this file is
** restricted by the provisions of a Freescale Software License
** Agreement, which has either been electronically accepted by
** you or has been expressly executed between the parties.
**
** Description: Explanation for the usage of this file.
**
** Revision History:
** -----------------
*****************************************************************************/

/*!
 * @file protocol.h
 *
 * @brief the RAM Kernel protocol header file.
 *
 * @ingroup RAM Kernel
 */
#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*****************************************************************************
* <Includes>
*****************************************************************************/

/*****************************************************************************
* <Macros>
*****************************************************************************/
/*!
 * RAM Kernel protocol defines
 */
#define RKL_COMMAND_MAGIC	0x0606
#define RKL_COMMAND_LEN		16 	/* 16bytes */
#define RKL_RESPONSE_LEN	8	/* 8bytes */

#define ERROR_COMMAND		0xffff
#define RET_SUCCESS		0
#define FLASH_PARTLY		1	/* response each dump/program size */
#define FUSE_PARTLY		1
#define FLASH_ERASE		2	/* response each erase size */
#define FLASH_VERIFY		3	/* response each verified bytes count */

/* flash failed define */
#define FLASH_FAILED		-4
#define FLASH_ECC_FAILED	-5

#define FLASH_ERROR_NO		0
#define FLASH_ERROR_READ   	-100
#define FLASH_ERROR_ECC    	-101
#define FLASH_ERROR_PROG   	-102
#define FLASH_ERROR_ERASE  	-103
#define FLASH_ERROR_VERIFY 	-104
#define FLASH_ERROR_INIT   	-105
#define FLASH_ERROR_OVER_ADDR	-106
#define FLASH_ERROR_PART_ERASE	-107
#define FLASH_ERROR_EOF 		-108

/* fuse failed define */
#define FUSE_FAILED		-4

#define FUSE_READ_PROTECT	-5
#define FUSE_SENSE_PROTECT	-6
#define FUSE_OVERRIDE_PROTECT	-7
#define FUSE_WRITE_PROTECT	-8
#define FUSE_VERIFY_FAILED	-9

/* ram kernel error define */

#define INVALID_CHANNEL		-256
#define INVALID_CHECKSUM	-257
#define INVALID_PARAM		-258


/* command id define */
#define CMD_FLASH	(0x00)
#define CMD_FUSE	(0x01)
#define CMD_COMMON	(0x02)
#define CMD_EXTEND	(0x03)


#define MAX_MODEL_LEN	128

#define OP_BLOCK_SIZE	(64*1024)

/*****************************************************************************
* <Typedefs>
*****************************************************************************/

typedef enum {
	CMD_FLASH_INITIAL	= ((CMD_FLASH << 8)  | 0x01), 
	CMD_FLASH_ERASE		= ((CMD_FLASH << 8)  | 0x02), 
	CMD_FLASH_DUMP		= ((CMD_FLASH << 8)  | 0x03),
	CMD_FLASH_PRORAM	= ((CMD_FLASH << 8)  | 0x04),
	CMD_FLASH_PRORAM_UB	= ((CMD_FLASH << 8)  | 0x05),
	CMD_FLASH_GET_CAPACITY	= ((CMD_FLASH << 8)  | 0x06),
	CMD_FUSE_READ		= ((CMD_FUSE << 8)   | 0x01),
	CMD_FUSE_SENSE		= ((CMD_FUSE << 8)   | 0x02),
	CMD_FUSE_OVERRIDE	= ((CMD_FUSE << 8)   | 0x03),
	CMD_FUSE_PROGRAM	= ((CMD_FUSE << 8)   | 0x04),
	CMD_RESET		= ((CMD_COMMON << 8) | 0x01),
	CMD_DOWNLOAD		= ((CMD_COMMON << 8) | 0x02),
	CMD_EXECUTE		= ((CMD_COMMON << 8) | 0x03),
	CMD_GETVER		= ((CMD_COMMON << 8) | 0x04),
	CMD_COM2USB		= ((CMD_EXTEND << 8) | 0x01),
	CMD_SWAP_BI		= ((CMD_EXTEND << 8) | 0x02),
	CMD_FL_BBT		= ((CMD_EXTEND << 8) | 0x03),
	CMD_FL_INTLV		= ((CMD_EXTEND << 8) | 0x04),
	CMD_FL_LBA		= ((CMD_EXTEND << 8) | 0x05),
} cmd_t;

/*****************************************************************************
* <Global Variables>
*****************************************************************************/


/*****************************************************************************
* <Local Variables>
*****************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PROTOCOL_H_ */
