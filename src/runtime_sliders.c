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


#include <configfile.h>
#include <debugging.h>
#include <getfiles.h>
#include <glib.h>
#include <runtime_sliders.h>
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <widgetmgmt.h>

GHashTable *rt_sliders = NULL;
GHashTable *ww_sliders = NULL;
GHashTable **ve3d_sliders = NULL;
static GtkSizeGroup *size_group_left = NULL;
static GtkSizeGroup *size_group_right = NULL;

void load_sliders()
{
	ConfigFile *cfgfile = NULL;
	struct Rt_Slider *slider = NULL;
	extern struct Firmware_Details *firmware;
	gchar *filename = NULL;
	gint count = 0;
	gint table = 0;
	gint row = 0;
	gchar *ctrl_name = NULL;
	gchar *source = NULL;
	gchar *section = NULL;
	gint i = 0;
	extern gboolean tabs_loaded;
	extern gboolean rtvars_loaded;

	if (!tabs_loaded)
		return;
	if (!rtvars_loaded)
	{
		dbg_func(__FILE__": load_sliders()\n\tCRITICAL ERROR, Realtime Variable definitions NOT LOADED!!!\n\n",CRITICAL);
		return;
	}
	if (!rt_sliders)
		rt_sliders = g_hash_table_new_full(NULL,NULL,slider_key_destroy,slider_value_destroy);
	if (!ww_sliders)
		ww_sliders = g_hash_table_new_full(NULL,NULL,slider_key_destroy,slider_value_destroy);

	filename = get_file(g_strconcat(RTSLIDERS_DIR,"/",firmware->sliders_map_file,".rts_conf",NULL));
	cfgfile = cfg_open_file(filename);
	if (cfgfile)
	{
		if(!cfg_read_int(cfgfile,"global","rt_total_sliders",&count))
		{
			dbg_func(g_strdup_printf(__FILE__": load_sliders()\n\t could NOT read \"rt_total_sliders\" value from\n\t file \"%s\"\n",filename),CRITICAL);
			goto do_ww_sliders;
		}
		size_group_left = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		size_group_right = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		for (i=0;i<count;i++)
		{
			slider = NULL;
			row = -1;
			table = -1;
			section = g_strdup_printf("rt_slider_%i",i);
			if(!cfg_read_string(cfgfile,section,"slider_name",&ctrl_name))
				dbg_func(g_strdup_printf(__FILE__": load_sliders()\n\t Failed reading \"slider_name\" from section \"%s\" in file\n\t%s\n",section,filename),CRITICAL);
			if(!cfg_read_int(cfgfile,section,"table",&table))
				dbg_func(g_strdup_printf(__FILE__": load_sliders()\n\t Failed reading \"table\" from section \"%s\" in file\n\t%s\n",section,filename),CRITICAL);
			if (!cfg_read_int(cfgfile,section,"row",&row))
				dbg_func(g_strdup_printf(__FILE__": load_sliders()\n\t Failed reading \"row\" from section \"%s\" in file\n\t%s\n",section,filename),CRITICAL);
			if (!cfg_read_string(cfgfile,section,"source",&source))
				dbg_func(g_strdup_printf(__FILE__": load_sliders()\n\t Failed reading \"source\" from section \"%s\" in file\n\t%s\n",section,filename),CRITICAL);

			slider = add_slider(ctrl_name,table,0,row,source,RUNTIME_PAGE);
			if (slider)
			{
				if (g_hash_table_lookup(rt_sliders,g_strdup(ctrl_name))==NULL)
					g_hash_table_insert(rt_sliders,g_strdup(ctrl_name),(gpointer)slider);
			}
			g_free(section);
			g_free(ctrl_name);
			g_free(source);
		}
		// Now warmup wizard.....
do_ww_sliders:
		if (!cfg_read_int(cfgfile,"global","ww_total_sliders",&count))
		{
			dbg_func(g_strdup_printf(__FILE__": load_sliders()\n\t could NOT read \"ww_total_sliders\" value from\n\t file \"%s\"\n",filename),CRITICAL);
			goto finish_off;
		}
		for (i=0;i<count;i++)
		{
			slider = NULL;
			row = -1;
			table = -1;
			section = g_strdup_printf("ww_slider_%i",i);
			if(!cfg_read_string(cfgfile,section,"slider_name",&ctrl_name))
				dbg_func(g_strdup_printf(__FILE__": load_sliders()\n\t Failed reading \"slider_name\" from section \"%s\" in file\n\t%s\n",section,filename),CRITICAL);
			if(!cfg_read_int(cfgfile,section,"table",&table))
				dbg_func(g_strdup_printf(__FILE__": load_sliders()\n\t Failed reading \"table\" from section \"%s\" in file\n\t%s\n",section,filename),CRITICAL);
			if (!cfg_read_int(cfgfile,section,"row",&row))
				dbg_func(g_strdup_printf(__FILE__": load_sliders()\n\t Failed reading \"row\" from section \"%s\" in file\n\t%s\n",section,filename),CRITICAL);
			if (!cfg_read_string(cfgfile,section,"source",&source))
				dbg_func(g_strdup_printf(__FILE__": load_sliders()\n\t Failed reading \"source\" from section \"%s\" in file\n\t%s\n",section,filename),CRITICAL);

			slider = add_slider(ctrl_name,table,0,row,source,WARMUP_WIZ_PAGE);
			if (slider)
			{
				if (g_hash_table_lookup(ww_sliders,g_strdup(ctrl_name))==NULL)
					g_hash_table_insert(ww_sliders,g_strdup(ctrl_name),(gpointer)slider);
			}
			g_free(section);
			g_free(ctrl_name);
			g_free(source);
		}
finish_off:
		cfg_free(cfgfile);
	}
	else
		dbg_func(g_strdup_printf(__FILE__": load_sliders()\n\t Filename \"%s\" NOT FOUND Critical error!!\n\n",filename),CRITICAL);
	g_free(filename);
}

void load_ve3d_sliders(gint table_num)
{
	ConfigFile *cfgfile = NULL;
	struct Rt_Slider *slider = NULL;
	extern struct Firmware_Details *firmware;
	gchar *filename = NULL;
	gint count = 0;
	gint table = 0;
	gint row = 0;
	gchar *ctrl_name = NULL;
	gchar *source = NULL;
	gchar *section = NULL;
	gint i = 0;
	extern gboolean tabs_loaded;
	extern gboolean rtvars_loaded;

	if (!tabs_loaded)
		return;
	if (!rtvars_loaded)
	{
		dbg_func(__FILE__": load_sliders()\n\tCRITICAL ERROR, Realtime Variable definitions NOT LOADED!!!\n\n",CRITICAL);
		return;
	}

	if (!ve3d_sliders)
		ve3d_sliders = g_new0(GHashTable *,firmware->total_tables);

	if (!ve3d_sliders[table_num])
	{
		ve3d_sliders[table_num] = g_hash_table_new_full(NULL,NULL,slider_key_destroy,slider_value_destroy);
	}


	filename = get_file(g_strconcat(RTSLIDERS_DIR,"/",firmware->sliders_map_file,".rts_conf",NULL));
	cfgfile = cfg_open_file(filename);
	if (cfgfile)
	{
		size_group_left = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		size_group_right = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

		if (!cfg_read_int(cfgfile,"global","ve3d_total_sliders",&count))
		{
			dbg_func(g_strdup_printf(__FILE__": load_sliders()\n\t could NOT read \"ve3d_total_sliders\" value from\n\t file \"%s\"\n",filename),CRITICAL);
			goto finish_off;
		}
		for (i=0;i<count;i++)
		{
			slider = NULL;
			row = -1;
			table = -1;
			section = g_strdup_printf("ve3d_slider_%i",i);
			if(!cfg_read_string(cfgfile,section,"slider_name",&ctrl_name))
				dbg_func(g_strdup_printf(__FILE__": load_sliders()\n\t Failed reading \"slider_name\" from section \"%s\" in file\n\t%s\n",section,filename),CRITICAL);
			if(!cfg_read_int(cfgfile,section,"table",&table))
				dbg_func(g_strdup_printf(__FILE__": load_sliders()\n\t Failed reading \"table\" from section \"%s\" in file\n\t%s\n",section,filename),CRITICAL);
			if (!cfg_read_int(cfgfile,section,"row",&row))
				dbg_func(g_strdup_printf(__FILE__": load_sliders()\n\t Failed reading \"row\" from section \"%s\" in file\n\t%s\n",section,filename),CRITICAL);
			if (!cfg_read_string(cfgfile,section,"source",&source))
				dbg_func(g_strdup_printf(__FILE__": load_sliders()\n\t Failed reading \"source\" from section \"%s\" in file\n\t%s\n",section,filename),CRITICAL);

			slider = add_slider(ctrl_name,table,table_num,row,source,VE3D_VIEWER_PAGE);
			if (slider)
			{
				if (g_hash_table_lookup(ve3d_sliders[table_num],ctrl_name)==NULL)
					g_hash_table_insert(ve3d_sliders[table_num],g_strdup(ctrl_name),(gpointer)slider);
			}
					
			g_free(section);
			g_free(ctrl_name);
			g_free(source);
		}
finish_off:
		cfg_free(cfgfile);
	}
}

struct Rt_Slider *  add_slider(gchar *ctrl_name, gint tbl, gint table_num, gint row, gchar *source, PageIdent ident)
{
	struct Rt_Slider *slider = NULL;
	GtkWidget *label = NULL;
	GtkWidget *pbar = NULL;
	GtkWidget *table = NULL;
	GtkWidget *hbox = NULL;
	gchar * name = NULL;
	extern GHashTable *dynamic_widgets;
	extern struct Rtv_Map *rtv_map;
	GObject *object = NULL;

	slider = g_malloc0(sizeof(struct Rt_Slider));

	object = g_hash_table_lookup(rtv_map->rtv_hash,source);
	if (!G_IS_OBJECT(object))
	{
		dbg_func(g_strdup_printf(__FILE__": add_slider()\n\tBad things man, object doesn't exist for %s\n",source),CRITICAL);
		return NULL;
	}

	slider->ctrl_name = g_strdup(ctrl_name);
	slider->tbl = tbl;
	slider->table_num = table_num;
	slider->row = row;
	slider->friendly_name = (gchar *) g_object_get_data(object,"dlog_gui_name");
	slider->lower = (gint)g_object_get_data(object,"lower_limit");

	slider->upper = (gint)g_object_get_data(object,"upper_limit");
	slider->history = (gfloat *) g_object_get_data(object,"history");
	slider->object = object;

	if (ident == RUNTIME_PAGE)
		name = g_strdup_printf("runtime_rt_table%i",slider->tbl);
	else if (ident == WARMUP_WIZ_PAGE)
		name = g_strdup_printf("ww_rt_table%i",slider->tbl);
	else if (ident == VE3D_VIEWER_PAGE)
		name = g_strdup_printf("ve3d_rt_table%i_%i",slider->tbl,slider->table_num);
	else
	{
		dbg_func(g_strdup_printf(__FILE__": add_slider()\n\tpage ident passed is not handled, ERROR, widget add aborted\n"),CRITICAL);
		return NULL;
	}
	table = g_hash_table_lookup(dynamic_widgets,name);
	if (!table)
	{
		dbg_func(g_strdup_printf(__FILE__": add_slider()\n\t table \"%s\" was not found, RuntimeSlider map or runtime datamap has a typo\n",name),CRITICAL);
		g_free(name);
		return NULL;
	}
	g_free(name);
	hbox = gtk_hbox_new(FALSE,5);

	label = gtk_label_new(NULL);
	slider->label = label;
	gtk_label_set_markup(GTK_LABEL(label),g_strdup(slider->friendly_name));
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_box_pack_start(GTK_BOX(hbox),label,TRUE,TRUE,0);

	label = gtk_label_new(NULL);
	slider->textval = label;
	gtk_misc_set_alignment(GTK_MISC(label),1,0.5);
	gtk_box_pack_start(GTK_BOX(hbox),label,TRUE,TRUE,0);

	if ((ident == RUNTIME_PAGE) || (ident == VE3D_VIEWER_PAGE))
	{
		if (((tbl+1) % 2) == 0)
			gtk_size_group_add_widget(size_group_right,hbox);
		else
			gtk_size_group_add_widget(size_group_left,hbox);
	}

	gtk_table_attach (GTK_TABLE (table),hbox,
			0,2,slider->row,(slider->row)+1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_FILL), 0, 0);

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
	
	gtk_table_attach (GTK_TABLE (table),pbar,
			2,3,slider->row,(slider->row)+1,
			(GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
			(GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK), 0, 0);
	slider->pbar = pbar;

	slider->parent = table;
	gtk_widget_show_all(slider->parent);

	return slider;
}


gboolean free_ve3d_sliders(gint table_num)
{
	extern GHashTable **ve3d_sliders;
	g_hash_table_destroy(ve3d_sliders[table_num]);
	deregister_widget(g_strdup_printf("ve3d_rt_table0_%i",table_num));
	deregister_widget(g_strdup_printf("ve3d_rt_table1_%i",table_num));
	ve3d_sliders[table_num] = NULL;
	return FALSE;
}

void slider_key_destroy(gpointer key)
{
	g_free(key);
}

void slider_value_destroy(gpointer value)
{
	struct Rt_Slider *slider = value;
	if (slider->ctrl_name)
		g_free(slider->ctrl_name);
	/* We do NOT free the object,history and friendly_name as they
	 * are just pointers to data in another structure 
	 * and if we delete them, we hose the other struct.
	 */
	g_free(slider);
}

