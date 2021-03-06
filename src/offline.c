/*
 * Copyright (C) 2002-2011 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
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

#include <apicheck.h>
#include <api-versions.h>
#include <config.h>
#include <defines.h>
#include <debugging.h>
#include <getfiles.h>
#include <gui_handlers.h>
#include <init.h>
#include <listmgmt.h>
#include <mode_select.h>
#include <notifications.h>
#include <offline.h>
#include <personalities.h>
#include <plugin.h>
#include <string.h>
#include <stdlib.h>
#include <tabloader.h>
#include <threads.h>
#include <widgetmgmt.h>


extern gconstpointer *global_data;


/*!
 \brief set_offline_mode() is called when the "Offline Mode" button is clicked
 in the general tab and is used to present the user with list of firmware 
 choices to select one for loading to work in offline mode (no connection to
 an ECU)
 */
G_MODULE_EXPORT gboolean set_offline_mode(void)
{
	GtkWidget * widget = NULL;
	gchar * filename = NULL;
	gboolean tmp = TRUE;
	GArray *pfuncs = NULL;
	PostFunction *pf = NULL;
	GAsyncQueue *io_repair_queue = NULL;
	Firmware_Details *firmware = NULL;
	void (*load_firmware_details)(void *,const gchar *) = NULL;

	firmware = DATA_GET(global_data,"firmware");
	io_repair_queue = DATA_GET(global_data,"io_repair_queue");

	/* Cause Serial Searcher thread to abort.... */
	if (io_repair_queue)
		g_async_queue_push(io_repair_queue,&tmp);

	gdk_threads_enter();
	filename = present_firmware_choices();
	if (!filename)
	{
		DATA_SET(global_data,"offline",GINT_TO_POINTER(FALSE));
		DATA_SET(global_data,"interrogated",GINT_TO_POINTER(FALSE));
		widget = lookup_widget("interrogate_button");
		if (GTK_IS_WIDGET(widget))
			gtk_widget_set_sensitive(GTK_WIDGET(widget),TRUE);
		widget = lookup_widget("offline_button");
		if (GTK_IS_WIDGET(widget))
			gtk_widget_set_sensitive(GTK_WIDGET(widget),TRUE);
		gdk_threads_leave();
		plugins_shutdown();
		gdk_threads_add_timeout(500,(GSourceFunc)personality_choice,NULL);

		return FALSE;
	}

	DATA_SET_FULL(global_data,"last_offline_profile",g_strdup(filename),g_free);
	DATA_SET(global_data,"offline",GINT_TO_POINTER(TRUE));
	DATA_SET(global_data,"interrogated",GINT_TO_POINTER(TRUE));

	/* Disable interrogation button */
	widget = lookup_widget("interrogate_button");
	if (GTK_IS_WIDGET(widget))
		gtk_widget_set_sensitive(GTK_WIDGET(widget),FALSE);

	queue_function("kill_conn_warning");

	if (!firmware)
	{
		firmware = g_new0(Firmware_Details,1);
		DATA_SET(global_data,"firmware",firmware);
	}
	if (get_symbol("load_firmware_details",(void*)&load_firmware_details))
		load_firmware_details(firmware,filename);

	pfuncs = g_array_new(FALSE,TRUE,sizeof(PostFunction *));

	pf = g_new0(PostFunction,1);
	pf->name = g_strdup("update_interrogation_gui_pf");
	get_symbol(pf->name,(void *)&pf->function);
	pf->w_arg = FALSE;
	pfuncs = g_array_append_val(pfuncs,pf);

	pf = g_new0(PostFunction,1);
	pf->name = g_strdup("load_realtime_map_pf");
	get_symbol(pf->name,(void *)&pf->function);
	pf->w_arg = FALSE;
	pfuncs = g_array_append_val(pfuncs,pf);

	pf = g_new0(PostFunction,1);
	pf->name = g_strdup("initialize_dashboards_pf");
	get_symbol(pf->name,(void *)&pf->function);
	pf->w_arg = FALSE;
	pfuncs = g_array_append_val(pfuncs,pf);

	pf = g_new0(PostFunction,1);
	pf->name = g_strdup("load_status_pf");
	get_symbol(pf->name,(void *)&pf->function);
	pf->w_arg = FALSE;
	pfuncs = g_array_append_val(pfuncs,pf);

	pf = g_new0(PostFunction,1);
	pf->name = g_strdup("load_rt_text_pf");
	get_symbol(pf->name,(void *)&pf->function);
	pf->w_arg = FALSE;
	pfuncs = g_array_append_val(pfuncs,pf);

	pf = g_new0(PostFunction,1);
	pf->name = g_strdup("load_gui_tabs_pf");
	get_symbol(pf->name,(void *)&pf->function);
	pf->w_arg = FALSE;
	pfuncs = g_array_append_val(pfuncs,pf);

	pf = g_new0(PostFunction,1);
	pf->name = g_strdup("load_sliders_pf");
	get_symbol(pf->name,(void *)&pf->function);
	pf->w_arg = FALSE;
	pfuncs = g_array_append_val(pfuncs,pf);

	pf = g_new0(PostFunction,1);
	pf->name = g_strdup("start_statuscounts_pf");
	get_symbol(pf->name,(void *)&pf->function);
	pf->w_arg = FALSE;
	pfuncs = g_array_append_val(pfuncs,pf);

	pf = g_new0(PostFunction,1);
	pf->name = g_strdup("disable_burner_buttons_pf");
	get_symbol(pf->name,(void *)&pf->function);
	pf->w_arg = FALSE;
	pfuncs = g_array_append_val(pfuncs,pf);

	pf = g_new0(PostFunction,1);
	pf->name = g_strdup("offline_ecu_restore_pf");
	get_symbol(pf->name,(void *)&pf->function);
	pf->w_arg = FALSE;
	pfuncs = g_array_append_val(pfuncs,pf);

	pf = g_new0(PostFunction,1);
	pf->name = g_strdup("setup_menu_handlers_pf");
	get_symbol(pf->name,(void *)&pf->function);
	pf->w_arg = FALSE;
	pfuncs = g_array_append_val(pfuncs,pf);

	pf = g_new0(PostFunction,1);
	pf->name = g_strdup("enable_3d_buttons_pf");
	get_symbol(pf->name,(void *)&pf->function);
	pf->w_arg = FALSE;
	pfuncs = g_array_append_val(pfuncs,pf);

	pf = g_new0(PostFunction,1);
	pf->name = g_strdup("ready_msg_pf");
	get_symbol(pf->name,(void *)&pf->function);
	pf->w_arg = FALSE;
	pfuncs = g_array_append_val(pfuncs,pf);


	io_cmd(NULL,pfuncs);

	/*
	   io_cmd(firmware->get_all_command,NULL);
	 */

	widget = lookup_widget("interrogate_button");
	if (GTK_IS_WIDGET(widget))
		gtk_widget_set_sensitive(GTK_WIDGET(widget),FALSE);
	widget = lookup_widget("offline_button");
	if (GTK_IS_WIDGET(widget))
		gtk_widget_set_sensitive(GTK_WIDGET(widget),FALSE);
	g_list_foreach(get_list("get_data_buttons"),set_widget_sensitive,GINT_TO_POINTER(FALSE));

	pfuncs = g_array_new(FALSE,TRUE,sizeof(PostFunction *));

	pf = g_new0(PostFunction,1);
	pf->name = g_strdup("reset_temps_pf");
	get_symbol(pf->name,(void *)&pf->function);
	pf->w_arg = FALSE;
	pfuncs = g_array_append_val(pfuncs,pf);

	io_cmd(NULL,pfuncs);
	gdk_threads_leave();
	return FALSE;
}


/*!
 \brief present_firmware_choices() presents a dialog box with the firmware
 choices.
 \returns the name of the chosen firmware
 */
G_MODULE_EXPORT gchar * present_firmware_choices(void)
{
	gchar ** filenames = NULL;
	GtkWidget *dialog = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *hbox = NULL;
	GtkWidget *ebox = NULL;
	GtkWidget *sep = NULL;
	GtkWidget *dummybutton = NULL;
	GtkWidget *button = NULL;
	GtkWidget *label = NULL;
	ListElement *element = NULL;
	gchar *tmpbuf = NULL;
	GArray *classes = NULL;
	GSList *group = NULL;
	GList *p_list = NULL;
	GList *s_list = NULL;
	ConfigFile *cfgfile = NULL;
	const gchar * last_file = NULL;
	gint major = 0;
	gint minor = 0;
	guint i = 0;
	gint result = 0;
	extern gconstpointer *global_data;



	filenames = get_files(g_build_filename(INTERROGATOR_DATA_DIR,"Profiles",DATA_GET(global_data,"ecu_family"),NULL),g_strdup("prof"),&classes);
	if (!filenames)
	{
		dbg_func(CRITICAL,g_strdup_printf(__FILE__": present_firmware_choices()\n\t NO Interrogation profiles found, was MegaTunix installed properly?\n\n"));
		return NULL;
	}
	i = 0;
	while (filenames[i]) 
	{
		cfgfile = cfg_open_file(filenames[i]);
		if (!cfgfile)
		{
			dbg_func(CRITICAL,g_strdup_printf(__FILE__": present_firmware_choices()\n\t Interrogation profile damaged!, was MegaTunix installed properly?\n\n"));
			i++;
			continue;
		}
		get_file_api(cfgfile,&major,&minor);
		if ((major != INTERROGATE_MAJOR_API) || (minor != INTERROGATE_MINOR_API))
		{
			thread_update_logbar("interr_view","warning",g_strdup_printf(_("Interrogation profile API mismatch (%i.%i != %i.%i):\n\tFile %s will be skipped\n"),major,minor,INTERROGATE_MAJOR_API,INTERROGATE_MINOR_API,cfgfile->filename),FALSE,FALSE);
			i++;
			cfg_free(cfgfile);
			continue;
		}
		cfg_read_string(cfgfile,"interrogation_profile","name",&tmpbuf);
		cfg_free(cfgfile);
		last_file = DATA_GET(global_data,"last_offline_profile");

		if (g_array_index(classes,FileClass,i) == PERSONAL)
		{
			element = g_new0(ListElement, 1);
			element->filename = g_strdup(filenames[i]);
			if (g_strcasecmp(element->filename,last_file) == 0)
				element->def = TRUE;

			element->name = g_strdup(tmpbuf);
			p_list = g_list_append(p_list,(gpointer)element);
		}
		if (g_array_index(classes,FileClass,i) == SYSTEM)
		{
			element = g_new0(ListElement, 1);
			element->filename = g_strdup(filenames[i]);
			if (g_strcasecmp(element->filename,last_file) == 0)
				element->def = TRUE;
			element->name = g_strdup(tmpbuf);
			s_list = g_list_append(s_list,(gpointer)element);
		}
		g_free(tmpbuf);
		i++;
	}
	p_list = g_list_sort(p_list,list_sort);
	s_list = g_list_sort(s_list,list_sort);


	dialog = gtk_dialog_new_with_buttons("Select Firmware",
			GTK_WINDOW(lookup_widget("main_window")),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			"Abort",
			GTK_RESPONSE_CANCEL,
			"Load",
			GTK_RESPONSE_OK,
			NULL);

	vbox = gtk_vbox_new(TRUE,2);
	gtk_container_set_border_width(GTK_CONTAINER(vbox),5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),vbox,TRUE,TRUE,0);
	/* Dummies */
	dummybutton = gtk_radio_button_new(NULL);
	g_object_ref_sink(dummybutton);
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(dummybutton));
	if (g_list_length(p_list) > 0)
	{
		label = gtk_label_new("Custom (personal) Profiles");
		gtk_box_pack_start(GTK_BOX(vbox),label,TRUE,TRUE,0);

		/* Cycle list for PERSONAL interogation files */
		for (i=0;i<g_list_length(p_list);i++)
		{
			element = g_list_nth_data(p_list,i);

			ebox = gtk_event_box_new();
			gtk_box_pack_start(GTK_BOX(vbox),ebox,TRUE,TRUE,0);
			hbox = gtk_hbox_new(FALSE,10);
			gtk_container_add(GTK_CONTAINER(ebox),hbox);
			label = gtk_label_new(element->name);
			gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,TRUE,0);
			button = gtk_radio_button_new(group);
			g_free(OBJ_GET(button,"filename"));
			OBJ_SET_FULL(button,"filename",g_strdup(element->filename),g_free);
			OBJ_SET(button,"handler",
					GINT_TO_POINTER(OFFLINE_FIRMWARE_CHOICE));
			g_signal_connect(button,
					"toggled",
					G_CALLBACK(toggle_button_handler),
					NULL);
			gtk_box_pack_end(GTK_BOX(hbox),button,FALSE,TRUE,0);
			if (element->def)
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),TRUE);
			else
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),FALSE);
			group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
		}

		sep = gtk_hseparator_new();
		gtk_box_pack_start(GTK_BOX(vbox),sep,TRUE,TRUE,0);
	}
	label = gtk_label_new("System Wide ECU Profiles");
	gtk_box_pack_start(GTK_BOX(vbox),label,TRUE,TRUE,0);
	/* Cycle list for System interogation files */
	for (i=0;i<g_list_length(s_list);i++)
	{
		element = g_list_nth_data(s_list,i);
		ebox = gtk_event_box_new();
		gtk_box_pack_start(GTK_BOX(vbox),ebox,TRUE,TRUE,0);
		hbox = gtk_hbox_new(FALSE,10);
		gtk_container_add(GTK_CONTAINER(ebox),hbox);
		label = gtk_label_new(element->name);
		gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,TRUE,0);
		button = gtk_radio_button_new(group);
		g_free(OBJ_GET(button,"filename"));
		OBJ_SET_FULL(button,"filename",g_strdup(element->filename),g_free);
		OBJ_SET(button,"handler",
				GINT_TO_POINTER(OFFLINE_FIRMWARE_CHOICE));
		g_signal_connect(button,
				"toggled",
				G_CALLBACK(toggle_button_handler),
				NULL);
		gtk_box_pack_end(GTK_BOX(hbox),button,FALSE,TRUE,0);
		if (element->def)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),TRUE);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),FALSE);
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
	}
	/*
	   if (i==1)
	   gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(button));
	   else
	   gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button),TRUE);
	 */

	g_strfreev(filenames);
	g_array_free(classes,TRUE);

	gtk_widget_show_all(dialog);

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	g_object_unref(dummybutton);
	g_list_foreach(p_list,free_element,NULL);
	g_list_foreach(s_list,free_element,NULL);
	g_list_free(p_list);
	g_list_free(s_list);
	switch (result)
	{
		case GTK_RESPONSE_ACCEPT:
		case GTK_RESPONSE_OK:
			return DATA_GET(global_data,"offline_firmware_choice");
			break;
		case GTK_RESPONSE_CANCEL:
		default:
			return NULL;
	}
	return NULL;
}

G_MODULE_EXPORT gint ptr_sort(gconstpointer a, gconstpointer b)
{
	return strcmp((gchar *)a, (gchar *) b);
}


G_MODULE_EXPORT void offline_ecu_restore_pf(void)
{
	MtxFileIO *fileio = NULL;
	gchar *filename = NULL;
	void (*restore_all_f)(const gchar *);
	Firmware_Details *firmware = NULL;

	firmware = DATA_GET(global_data,"firmware");
	get_symbol("restore_all_ecu_settings",(void *)&restore_all_f);

	g_return_if_fail(firmware);
	g_return_if_fail(restore_all_f);

	if (!DATA_GET(global_data,"interrogated"))
		return;

	fileio = g_new0(MtxFileIO ,1);
	fileio->external_path = g_strdup("MTX_ecu_snapshots");
	fileio->default_path = g_strdup("ecu_snapshots");
	fileio->parent = lookup_widget("main_window");
	fileio->on_top = TRUE;
	fileio->title = g_strdup("You should load an ECU backup from a file");
	fileio->action = GTK_FILE_CHOOSER_ACTION_OPEN;
	fileio->shortcut_folders = g_strdup("ecu_snapshots,../MTX_ecu_snapshots");
	fileio->default_filename = g_strdup(DATA_GET(global_data,"last_offline_filename"));

	gdk_threads_enter();
	filename = choose_file(fileio);
	free_mtxfileio(fileio);
	if (filename)
	{
		DATA_SET_FULL(global_data,"last_offline_filename",g_strdup(filename),g_free);
		update_logbar("tools_view",NULL,_("Full Restore of ECU Initiated\n"),FALSE,FALSE,FALSE);
		restore_all_f(filename);
		g_free(filename);
	}
	else
		io_cmd(firmware->get_all_command,NULL);

	gdk_threads_leave();
	return;
}
