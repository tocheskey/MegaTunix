/*
 * Copyright (C) 2003 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 *
 * Linux Megasquirt tuning software
 * 
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute etc. this as long as the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

#include <comms_gui.h>
#include <config.h>
#include <conversions.h>
#include <dataio.h>
#include <datalogging_gui.h>
#include <defines.h>
#include <debugging.h>
#include <enums.h>
#include <errno.h>
#include <gui_handlers.h>
#include <interrogate.h>
#include <logviewer_gui.h>
#include <notifications.h>
#include <post_process.h>
#include <runtime_gui.h>
#include <rtv_map_loader.h>
#include <serialio.h>
#include <string.h>
#include <structures.h>
#include <sys/poll.h>
#include <tabloader.h>
#include <threads.h>
#include <unistd.h>


GThread *raw_input_thread;			/* thread handle */
GAsyncQueue *io_queue = NULL;
gboolean raw_reader_running;			/* flag for thread */
extern gboolean connected;			/* valid connection with MS */
extern GtkWidget * comms_view;
extern struct DynamicMisc misc;
extern struct Serial_Params *serial_params;
static gchar *handler_types[]={"Realtime Vars","VE-Block","Raw Memory Dump","Comms Test"};


void io_cmd(IoCommand cmd, gpointer data)
{
	struct Io_Message *message = NULL;
	extern struct IoCmds *cmds;
	extern gboolean tabs_loaded;
	extern struct Firmware_Details * firmware;
	gint tmp = -1;
	gint i = 0;

	/* This function is the bridge from the main GTK thread (gui) to
	 * the Serial I/O Handler (GThread). Communication is achieved through
	 * Asynchronous Queues. (preferred method in GLib and should be more 
	 * portable to more arch's)
	 */
	switch (cmd)
	{
		case IO_REALTIME_READ:
			message = g_new0(struct Io_Message,1);
			message->command = READ_CMD;
			message->out_str = g_strdup(cmds->realtime_cmd);
			message->out_len = cmds->rt_cmd_len;
			message->handler = REALTIME_VARS;
			message->funcs = g_array_new(FALSE,TRUE,sizeof(gint));
			tmp = UPD_REALTIME;
			g_array_append_val(message->funcs,tmp);
			tmp = UPD_LOGVIEWER;
			g_array_append_val(message->funcs,tmp);
			tmp = UPD_DATALOGGER;
			g_array_append_val(message->funcs,tmp);
			g_async_queue_push(io_queue,(gpointer)message);
			break;

		case IO_INTERROGATE_ECU:
			if (!connected)	// Attempt reconnect with comms test
				io_cmd(IO_COMMS_TEST,NULL);
			message = g_new0(struct Io_Message,1);
			message->command = INTERROGATION;
			message->funcs = g_array_new(TRUE,TRUE,sizeof(gint));
			if (!tabs_loaded)
			{
				tmp = UPD_LOAD_GUI_TABS;
				g_array_append_val(message->funcs,tmp);
				tmp = UPD_LOAD_REALTIME_MAP;
				g_array_append_val(message->funcs,tmp);
			}
			tmp = UPD_READ_VE_CONST;
			g_array_append_val(message->funcs,tmp);
			g_async_queue_push(io_queue,(gpointer)message);
			break;
		case IO_COMMS_TEST:
			message = g_new0(struct Io_Message,1);
			message->command = COMMS_TEST;
			message->page = 0;
			message->out_str = g_strdup("Q");
			message->out_len = 1;
			message->handler = C_TEST;
			g_async_queue_push(io_queue,(gpointer)message);
			break;
		case IO_READ_VE_CONST:
			if (!firmware)
				break;
			for (i=0;i<firmware->total_pages;i++)
			{
				message = g_new0(struct Io_Message,1);
				message->command = READ_CMD;
				message->page = i;
				if (firmware->page_params[i]->is_spark)
				{
					message->out_str = g_strdup(cmds->ignition_cmd);
					message->out_len = cmds->ign_cmd_len;
				}
				else
				{
					message->out_str = g_strdup(cmds->veconst_cmd);
					message->out_len = cmds->ve_cmd_len;
				}
				message->handler = VE_BLOCK;
				message->funcs = g_array_new(TRUE,TRUE,sizeof(gint));
				if (i == (firmware->total_pages-1))
				{
					tmp = UPD_VE_CONST;
					g_array_append_val(message->funcs,tmp);
					tmp = UPD_STORE_BLACK;
					g_array_append_val(message->funcs,tmp);
				}
				g_async_queue_push(io_queue,(gpointer)message);
			}
			break;
		case IO_READ_RAW_MEMORY:
			message = g_new0(struct Io_Message,1);
			message->command = READ_CMD;
			message->page = 0;
			message->offset = (gint)data;
			message->out_str = g_strdup(cmds->raw_mem_cmd);
			message->out_len = cmds->raw_mem_cmd_len;
			message->handler = RAW_MEMORY_DUMP;
			message->funcs = g_array_new(FALSE,TRUE,sizeof(gint));
			tmp = UPD_RAW_MEMORY;
			g_array_append_val(message->funcs,tmp);
			g_async_queue_push(io_queue,(gpointer)message);
			break;
		case IO_BURN_MS_FLASH:
			message = g_new0(struct Io_Message,1);
			message->command = BURN_CMD;
			message->funcs = g_array_new(FALSE,TRUE,sizeof(gint));
			tmp = UPD_STORE_BLACK;
			g_array_append_val(message->funcs,tmp);
			g_async_queue_push(io_queue,(gpointer)message);
			break;
		case IO_WRITE_DATA:
			message = g_new0(struct Io_Message,1);
			message->command = WRITE_CMD;
			message->payload = data;
			g_async_queue_push(io_queue,(gpointer)message);
			break;
	}
}

void *serial_io_handler(gpointer data)
{
	struct Io_Message *message = NULL;	
	extern struct Firmware_Details * firmware;
	gint len=0;
	gint i=0;
	gint val=-1;
	gboolean result = FALSE;
	extern gint temp_units;
	extern gboolean paused_handlers;
	extern gint mem_view_style[];
	/* Create Queue to listen for commands */
	io_queue = g_async_queue_new();

	/* Endless Loop, wiat for message, processs and repeat... */
	while (1)
	{
		message = g_async_queue_pop(io_queue);
		switch ((CmdType)message->command)
		{
			case INTERROGATION:
				dbg_func(__FILE__": serial_io_handler()\n\tInterrogate_ecu requested\n",SERIAL_RD|SERIAL_WR);
				if (connected)
					interrogate_ecu();
				break;
			case COMMS_TEST:
				dbg_func(__FILE__": serial_io_handler()\n\tcomms_test requested \n",SERIAL_RD|SERIAL_WR);
				comms_test();
				break;
			case READ_CMD:
				dbg_func(g_strdup_printf(__FILE__": serial_io_handler()\n\tread_command requested (%s)\n",handler_types[message->handler]),SERIAL_RD);
				readfrom_ecu(message);
				break;
			case WRITE_CMD:
				dbg_func(__FILE__": serial_io_handler()\n\twrite_command requested\n",SERIAL_WR);
				writeto_ecu(message);
				break;
			case BURN_CMD:
				dbg_func(__FILE__": serial_io_handler()\n\tburn_command requested\n",SERIAL_WR);
				burn_ms_flash();
				break;

		}
		/* If dispatch funcs are defined, run them from the thread
		 * context. NOTE wrapping with gdk_threads_enter/leave, as 
		 * that is NECESSARY to to do when making gtk calls inside
		 * the thread....
		 */
		if (message->funcs != NULL)
		{
			len = message->funcs->len;
			for (i=0;i<len;i++)
			{
				val = g_array_index(message->funcs,
						UpdateFunction, i);
				switch ((UpdateFunction)val)
				{
					case UPD_LOAD_REALTIME_MAP:
						if (connected)
						{
							gdk_threads_enter();
							result = load_realtime_map();
							if (result == FALSE)
							{
								gdk_threads_leave();
								goto breakout;
							}
							gdk_threads_leave();
						}
						break;
					case UPD_LOAD_GUI_TABS:
						if (connected)
						{
							gdk_threads_enter();
							result = load_gui_tabs();
							if (result == FALSE)
							{
								gdk_threads_leave();
								goto breakout;
							}
							reset_temps(GINT_TO_POINTER(temp_units));
							gdk_threads_leave();
						}
						break;
					case UPD_READ_VE_CONST:
						io_cmd(IO_READ_VE_CONST,NULL);
						break;
					case UPD_REALTIME:
						update_runtime_vars();
						break;
					case UPD_VE_CONST:
						gdk_threads_enter();
						paused_handlers = TRUE;
						update_ve_const();
						paused_handlers = FALSE;
						gdk_threads_leave();
						break;
					case UPD_STORE_BLACK:
						gdk_threads_enter();
						set_store_buttons_state(BLACK);
						for (i=0;i<firmware->total_pages;i++)
							set_reqfuel_state(BLACK,i);
						gdk_threads_leave();
						break;
					case UPD_LOGVIEWER:
						gdk_threads_enter();
						update_logview_traces();
						gdk_threads_leave();
						break;
					case UPD_RAW_MEMORY:
						gdk_threads_enter();
						update_raw_memory_view(mem_view_style[message->offset],message->offset);
						gdk_threads_leave();
						break;
					case UPD_DATALOGGER:
						run_datalog();
						break;
				}
			}
		}
breakout:
		dealloc_message(message);
	}
}

void dealloc_message(void * ptr)
{
	struct Io_Message *message = (struct Io_Message *)ptr;
	if (message->out_str)
		g_free(message->out_str);
	if (message->funcs) 
		g_array_free(message->funcs,TRUE);
	if (message->payload)
		g_free(message->payload);
	g_free(message);

}

void readfrom_ecu(void *ptr)
{
	struct pollfd ufds;
	struct Io_Message *message = (struct Io_Message *)ptr;
	gint result = 0;
	extern gint ecu_caps;

	if(serial_params->open == FALSE)
		return;

	ufds.fd = serial_params->fd;
	ufds.events = POLLIN;

	/* Flush serial port... */
	tcflush(serial_params->fd, TCIOFLUSH);

	if (ecu_caps & DUALTABLE)
		set_ms_page(message->page);
	result = write(serial_params->fd,
			message->out_str,
			message->out_len);
	if (result != message->out_len)	
		dbg_func(__FILE__": readfrom_ecu()\n\twrite command to ECU failed\n",CRITICAL);

	dbg_func(g_strdup_printf(__FILE__": readfrom_ecu()\n\tSent %s to the ECU\n",message->out_str),SERIAL_WR);
	if (message->handler == RAW_MEMORY_DUMP)
		result = write(serial_params->fd,&message->offset,1);


	/* check for data,,,, */
	result = poll (&ufds,1,5*serial_params->poll_timeout);
	if (result < 0)	   /* Error */
		dbg_func(g_strdup_printf(__FILE__": readfrom_ecu()\n\tError polling for data: %s\n",g_strerror(errno)),CRITICAL);
	else if (result == 0)   /* Timeout */
	{
		dbg_func(__FILE__": readfrom_ecu()\n\tTimeout reading Data from ECU\n",CRITICAL);
		serial_params->errcount++;
		connected = FALSE;
	}
	else            /* Data arrived */
	{
		connected = TRUE;
		dbg_func(g_strdup_printf(__FILE__": readfrom_ecu()\n\tReading %s\n",
					handler_types[message->handler]),SERIAL_RD);
		if (message->handler != -1)
			handle_ms_data(message->handler,message);

	}	
}

/* Called from a THREAD ONLY so gdk_threads_enter/leave are REQUIRED on any
 * gtk function inside.... 
 */
void comms_test()
{
	struct pollfd ufds;
	gint result = -1;
	gchar * tmpbuf = NULL;


	if (serial_params->open == FALSE)
	{
		connected = FALSE;
		dbg_func(__FILE__": comms_test()\n\tSerial Port is NOT opened can NOT check ecu comms...\n",CRITICAL);
		gdk_threads_enter();
		no_ms_connection();
		gdk_threads_leave();
		return;
	}

	ufds.fd = serial_params->fd;
	ufds.events = POLLIN;
	/* Flush the toilet.... */
	tcflush(serial_params->fd, TCIOFLUSH);	
	while (write(serial_params->fd,"C",1) != 1)
	{
		usleep(10000);
		dbg_func(__FILE__": comms_test()\n\tError writing \"C\" to the ecu in comms_test()\n",CRITICAL);
	}
	dbg_func(__FILE__": commes_test()\n\tRequesting MS Clock (\"C\" cmd)\n",SERIAL_RD);
	result = poll (&ufds,1,serial_params->poll_timeout*5);
	if (result < 0)		// Error
		dbg_func(g_strdup_printf(__FILE__": comms_test()\n\tError polling for data: %s\n",g_strerror(errno)),CRITICAL);
	else if (result == 0)	// Timeout
	{
		connected = FALSE;
		tmpbuf = g_strdup_printf("I/O with MegaSquirt Timeout\n");
		/* An I/O Error occurred with the MegaSquirt ECU */
		dbg_func(__FILE__": comms_test()\n\tI/O with ECU Timeout\n",CRITICAL);
		gdk_threads_enter();
		update_logbar(comms_view,"warning",tmpbuf,TRUE,FALSE);
		g_free(tmpbuf);
		gtk_widget_set_sensitive(misc.status[STAT_CONNECTED],
				connected);
		////		gtk_widget_set_sensitive(misc.ww_status[STAT_CONNECTED],
		//				connected);
		gdk_threads_leave();
	}
	else
	{
		handle_ms_data(C_TEST,NULL);
		connected = TRUE;

		tmpbuf = g_strdup_printf("ECU Comms Test Successfull\n");
		/* COMMS test succeeded */
		dbg_func(__FILE__": comms_test()\n\tECU Comms Test Successfull\n",SERIAL_RD);
		gdk_threads_enter();
		update_logbar(comms_view,NULL,tmpbuf,TRUE,FALSE);
		g_free(tmpbuf);
		gtk_widget_set_sensitive(misc.status[STAT_CONNECTED],
				connected);
		//		gtk_widget_set_sensitive(misc.ww_status[STAT_CONNECTED],
		//				connected);
		gdk_threads_leave();

	}
	/* Flush the toilet again.... */
	tcflush(serial_params->fd, TCIOFLUSH);
	return;
}

void write_ve_const(gint page, gint offset, gint value, gboolean ign_parm)
{
	struct OutputData *output = NULL;

	dbg_func(g_strdup_printf(__FILE__": write_ve_const()\n\t Sending page %i, offset %i, value %i, ign_parm %i\n",page,offset,value,ign_parm),SERIAL_WR);
	output = g_new0(struct OutputData,1);
	output->page = page;
	output->offset = offset;
	output->value = value;
	output->ign_parm = ign_parm;
	io_cmd(IO_WRITE_DATA,output);
	return;
}

void writeto_ecu(void *ptr)
{
	struct Io_Message *message = (struct Io_Message *)ptr;
	struct OutputData *data = message->payload;

	gint page = data->page;
	gint offset = data->offset;
	gint value = data->value;
	gboolean ign_parm = data->ign_parm;
	gint highbyte = 0;
	gint lowbyte = 0;
	gboolean twopart = 0;
	gint res = 0;
	gint count = 0;
	char lbuff[3] = {0, 0, 0};
	extern guchar *ms_data[MAX_SUPPORTED_PAGES];
	extern guchar *ms_data_last[MAX_SUPPORTED_PAGES];
	gchar * write_cmd = NULL;
	extern struct Firmware_Details *firmware;
	static GStaticMutex mutex = G_STATIC_MUTEX_INIT;

	g_static_mutex_lock(&mutex);

	if (!connected)
	{
		no_ms_connection();
		g_static_mutex_unlock(&mutex);
		return;		/* can't write anything if disconnected */
	}
	dbg_func(g_strdup_printf(__FILE__": writeto_ecu()\n\tMS Serial Write, Page, %i, Value %i, Mem Offset %i\n",page,value,offset),SERIAL_WR);

	if (value > 255)
	{
		dbg_func(g_strdup_printf(__FILE__": writeto_ecu()\n\tLarge value being sent: %i, to page %i, offset %i\n",value,page,offset),SERIAL_WR);

		highbyte = (value & 0xff00) >> 8;
		lowbyte = value & 0x00ff;
		twopart = TRUE;
		dbg_func(g_strdup_printf(__FILE__": writeto_ecu()\n\tHighbyte: %i, Lowbyte %i\n",highbyte,lowbyte),SERIAL_WR);
	}
	if (value < 0)
	{
		dbg_func(__FILE__": writeto_ecu()\n\tWARNING!!, value sent is below 0\n",CRITICAL);
		g_static_mutex_unlock(&mutex);
		return;
	}

	/* Handles variants and dualtable... */
	if (firmware->multi_page)
		if (ign_parm == FALSE)
			set_ms_page(page);

	dbg_func(g_strdup_printf(__FILE__": writeto_ecu()\n\tIgnition param %i\n",ign_parm),SERIAL_WR);

	if (ign_parm)
		write_cmd = g_strdup("J");
	else
		write_cmd = g_strdup("W");

	lbuff[0]=offset;
	if (twopart)
	{
		lbuff[1]=highbyte;
		lbuff[2]=lowbyte;
		count = 3;
		dbg_func(__FILE__": writeto_ecu()\n\tSending 16 bit value to ECU\n",SERIAL_WR);
	}
	else
	{
		lbuff[1]=value;
		count = 2;
		dbg_func(__FILE__": writeto_ecu()\n\tSending 8 bit value to ECU\n",SERIAL_WR);
	}


	res = write (serial_params->fd,write_cmd,1);	/* Send write command */
	if (res != 1 )
		dbg_func(__FILE__": writeto_ecu()\n\tSending write command FAILED!!!\n",CRITICAL);
	res = write (serial_params->fd,lbuff,count);	/* Send offset+data */
	if (res != count )
		dbg_func(__FILE__": writeto_ecu()\n\tSending offset+data FAILED!!!\n",CRITICAL);
	usleep(5000);
	if (page > 0)
		set_ms_page(0);
	g_free(write_cmd);

	/* We check to see if the last burn copy of the MS VE/constants matches 
	 * the currently set, if so take away the "burn now" notification.
	 * avoid unnecessary burns to the FLASH 
	 */
	res = memcmp(ms_data_last[page],ms_data[page],2*MS_PAGE_SIZE);
	gdk_threads_enter();
	if (res == 0)
		set_store_buttons_state(BLACK);
	else
		set_store_buttons_state(RED);
	gdk_threads_leave();

	g_static_mutex_unlock(&mutex);
	return;
}

void burn_ms_flash()
{
	extern guchar *ms_data[MAX_SUPPORTED_PAGES];
	extern guchar *ms_data_last[MAX_SUPPORTED_PAGES];
	gint res = 0;
	gint i = 0;
	extern gint ecu_caps;
	extern struct Firmware_Details * firmware;
	static GStaticMutex mutex = G_STATIC_MUTEX_INIT;

	g_static_mutex_lock(&mutex);

	tcflush(serial_params->fd, TCIOFLUSH);

	/* doing this may NOT be necessary,  but who knows... */
	if (ecu_caps & DUALTABLE)
	{
		set_ms_page(0);
		usleep(5000);
	}

	res = write (serial_params->fd,"B",1);  /* Send Burn command */
	if (res != 1)
	{
		dbg_func(g_strdup_printf(__FILE__": burn_ms_flash()\n\tBurn Failure, write command failed!!%i\n",res),CRITICAL);
	}
	usleep(5000);

	dbg_func(__FILE__": burn_ms_flash()\n\tBurn to Flash\n",SERIAL_WR);

	/* sync temp buffer with current burned settings */
	for (i=0;i<firmware->total_pages;i++)
		memcpy(ms_data_last[i],ms_data[i],2*MS_PAGE_SIZE);

	tcflush(serial_params->fd, TCIOFLUSH);

	g_static_mutex_unlock(&mutex);
	return;
}
