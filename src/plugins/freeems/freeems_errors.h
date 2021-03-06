/*
 * Copyright (C) 2002-2011 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 *
 * Linux Megasquirt tuning software
 * 
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute, etc. this as long as all the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

#ifndef __FEEEMS_ERRORS_H__
#define __FEEEMS_ERRORS_H__

#include <gtk/gtk.h>
#include <defines.h>
#include <configfile.h>
#include <freeems_globaldefs.h>
#include <enums.h>
#include <threads.h>

typedef struct _FreeEMS_Errors FreeEMS_Errors;
static struct _FreeEMS_Errors
{
	guint code;
	gchar *message;
}Errors[] = {
	{0x0666,"No Error"},
	/* Unconfiugred Options */
	{0x1000,"IAT not Configured!"},
	{0x1001,"CHT not Configured!"},
	{0x1002,"TPS not Configured!"},
	{0x1003,"EGO not Configured!"},
	{0x1004,"BRV not Configured!"},
	{0x1005,"MAP not Configured!"},
	{0x1006,"AAP not Configured!"},
	{0x1007,"MAT not Configured!"},
	{0x1008,"EGO2 not Confiugred!"},
	{0x1009,"IAP not Configured!"},
	{0x100A,"LOAD not Configured!"},
	{0x100B,"AIRFLOW not Configured!"},
	{0x100C,"BPW not Configured!"},
	/* Badly confiugred Options */
	{0x2000,"VE-Table Main Load Length too long!"},
	{0x2001,"VE-Table Main RPM Length too long!"},
	{0x2002,"VE-Table Main Main Length too long!"},
	{0x2003,"BRV Max too large!"},
	/* Flash burning errors */
	{0x3000,"Size not multiple of sector size!"},
	{0x3001,"Size of block to burn is ZERO!"},
	{0x3002,"Small block crosses sector boundary!"},
	{0x3003,"Address not sector aligned!"},
	{0x3004,"Address not word aligned!"},
	{0x3005,"Address not Flash region!"},
	{0x3006,"Flash erase failed!"},
	{0x3007,"Flash access error!"},
	{0x3008,"Flash protection error!"},
	{0x3009,"Memory write error!"},
	/* Communications error Codes */
	{0x4000,"Unimplemented function!"},
	{0x4001,"Packet checksum mismatch!"},
	{0x4002,"Packet too short for specified fields!"},
	{0x4003,"Does not make sense to retrieve partially!"},
	{0x4004,"Payload length type mismatch!"},
	{0x4005,"Payload length header mismatch!"},
	{0x4006,"Invalid Payload ID!"},
	{0x4007,"Unrecognized Payload ID!"},
	{0x4008,"Invalid Memory action for ID!"},
	{0x4009,"Invalid ID for Main Table Action!"},
	{0x400A,"Invalud ID for 2D Table Action!"},
	{0x400B,"No such Async Datalog Type!"},
	{0x400C,"Datalog length exceeds max!"},
	{0x400D,"Location ID not found!"},
	{0x400E,"Requested RAM page invalid!"},
	{0x400F,"Requested FLASH page invalid!"},
	{0x4010,"Requested length too large!"},
	{0x4011,"Requested Address disallowed!"},
	{0x4012,"Invalid size/offset combination!"},
	{0x4013,"Unchecked table manipulation not allowd!"},
	{0x4014,"Payload not equal to specified value!"},
	{0x4015,"No such location ID list type!"},
	{0x4016,"Payload shorter than required for test!"},
	{0x4017,"No such unit test ID!"},
	{0x6000,"Error base main table RPM!"},
	{0x6001,"Invalid main table RPM order!"},
	{0x6002,"Invalid main table RPM index!"},
	{0x6003,"Invalid main table RPM length!"},
	{0x6004,"Error with base main table load!"},
	{0x6005,"Invalid main table Load order!"},
	{0x6006,"Invalid main table Load index!"},
	{0x6007,"Invalid main table Load length!"},
	{0x6008,"Invalid main table Main length!"},
	{0x6010,"Error base 2D table axis!"},
	{0x6011,"Invalid 2D table axis order!"},
	{0x6012,"Invalid 2D table index!"}
};

/* Prototypes */
const gchar * lookup_error(gint);
/* Prototypes */

#endif
