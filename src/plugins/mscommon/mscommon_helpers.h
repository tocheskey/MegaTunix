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

#ifndef __MSCOMMON_HELPERS_H__
#define __MSCOMMON_HELPERS_H__

#include <defines.h>
#include <enums.h>
#include <gtk/gtk.h>
#include <threads.h>

typedef enum
{
	WRITE_VERIFY=0x290,
	MISMATCH_COUNT,
	MS1_CLOCK,
	MS2_CLOCK,
	NUM_REV,
	TEXT_REV,
	SIGNATURE,
	MS1_VECONST,
	MS2_VECONST,
	MS2_BOOTLOADER,
	MS1_RT_VARS,
	MS2_RT_VARS,
	MS1_GETERROR,
	MS1_E_TRIGMON,
	MS1_E_TOOTHMON,
	MS2_E_TRIGMON,
	MS2_E_TOOTHMON,
	MS2_E_COMPOSITEMON,
	LAST_XML_FUNC_CALL_TYPE
}FuncCall;

/* Prototypes */
void spawn_read_ve_const_pf(void);
void enable_get_data_buttons_pf(void);
void simple_read_pf(void *, FuncCall);
gboolean read_ve_const(void *, FuncCall);
gboolean burn_all_helper(void *, FuncCall);
void post_single_burn_pf(void *data);
void post_burn_pf(void);
void startup_tcpip_sockets_pf(void);
/* Prototypes */

#endif
