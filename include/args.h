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

#ifndef __ARGS_H__
#define __ARGS_H__

#include <gtk/gtk.h>


typedef struct _CmdLineArgs CmdLineArgs;
/*!
 * \brief _CmdLineArgs struct is a container to hold the command line argument
 * related variables, used to make mtx quiet, suppress portions of the gui
 * and autolog to files.
 */
struct _CmdLineArgs
{
	gboolean debugargs;	/* Debug arg processing to console */
	gboolean verbose;	/* Show Version */
	gboolean version;	/* Show Version */
	gboolean be_quiet;	/* No error popups */
	gboolean autolog_dump;	/* Automatically dump full logs periodically */
	gboolean force_port;	/* Forced port, no autoscanning */
	gboolean inhibit_tabs;	/* Don't load tabs specified in interrogation profile */
	gboolean hide_rttext;	/* Hide Runtime Variable Window */
	gboolean hide_status;	/* Hide Status Window */
	gboolean hide_maingui;	/* Hide Main Gui (Dash only mode */
	gboolean offline;	/* Force offline mode */
	gint autolog_minutes;	/* How many minutes to log per file */
	gchar *dbglog;		/* Global debug log file name */
	gchar *autolog_dump_dir;/* What dir to put logs into */
	gchar *autolog_basename;/* Autolog base filename */
	gchar *network_host;	/* Network host */
	gchar *persona;		/* ECU Persona to default to */
	gchar *port;		/* Serial port override */
	gint network_port;	/* Network port */
	gboolean network_mode;	/* Network mode */
	gboolean listen_mode;	/* Listen mode */
};

/* Prototypes */
CmdLineArgs * init_args(void);
void handle_args(gint, gchar ** );
void args_free(gpointer);
/* Prototypes */

#endif
