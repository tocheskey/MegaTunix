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

#include <about_gui.h>
#include <comms_gui.h>
#include <config.h>
#include <configfile.h>
#include <core_gui.h>
#include <defines.h>
#include <enums.h>
#include <general_gui.h>
#include <glade/glade.h>
#include <gui_handlers.h>
#include <getfiles.h>
#include <menu_handlers.h>
#include <stdlib.h>
#include <structures.h>
#include <tabloader.h>
#include <tuning_gui.h>
#include <widgetmgmt.h>



/* Default window size and MINIMUM size as well... */
static gint def_width=620;
static gint def_height=525;
gint width = 0;
gint height = 0;
gint main_x_origin = 0;
gint main_y_origin = 0;
extern gboolean tips_in_use;
GtkWidget *main_window = NULL;
GtkTooltips *tip = NULL;
GtkWidget *notebook = NULL;
static struct 
{
	gchar *frame_name;	/* Textual name at the top of the frame */
	void (*Function) (GtkWidget *);	/* builder function */
	gchar *tab_name;	/* The Tab textual name for the main gui */
	TabIdent tab_ident;	/* Tab Identifier... */
} notebook_tabs[] = { 
{ "About MegaTunix", build_about, "_About",ABOUT_TAB},
{ "General MegaTunix Settings", build_general, "_General",GENERAL_TAB},
{ "MegaSquirt Communications Parameters", build_comms, "_Communications",COMMS_TAB},
};

static int num_tabs = sizeof(notebook_tabs) / sizeof(notebook_tabs[0]);


/*!
 \brief setup_gui() creates the main window, main notebook, and the static
 tabs and populates them with data
 */
int setup_gui()
{
	GtkWidget *frame = NULL;
	GtkWidget *label = NULL;
	GtkWidget *hbox = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *button = NULL;
	gchar *filename = NULL;
	GladeXML *xml = NULL;
	gint i=0;

	filename = get_file(g_build_filename(GUI_DATA_DIR,"main.glade",NULL),NULL);
	if (!filename)
	{
		printf("Can't locate primary glade file!!!!\n");
		exit(-1);
	}
	else
		xml = glade_xml_new(filename, "mtx_main_window",NULL);
	g_free(filename);

	setup_menu_handlers(xml);

	glade_xml_signal_autoconnect(xml);

	main_window = glade_xml_get_widget(xml,"mtx_main_window");
	gtk_window_move((GtkWindow *)main_window, main_x_origin, main_y_origin);
	gtk_widget_set_size_request(main_window,def_width,def_height);
	gtk_window_set_default_size(GTK_WINDOW(main_window),width,height);
	gtk_window_set_title(GTK_WINDOW(main_window),"MegaTunix "VERSION);
	gtk_widget_realize(main_window);

	tip = gtk_tooltips_new();

	vbox = glade_xml_get_widget(xml,"mtx_top_vbox");

	notebook = gtk_notebook_new ();
	register_widget("toplevel_notebook",notebook);
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_LEFT);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook),TRUE);
	gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));
	gtk_box_pack_start(GTK_BOX(vbox),notebook,TRUE,TRUE,0);

	for (i=0;i<num_tabs;i++)
	{
		frame = gtk_frame_new (notebook_tabs[i].frame_name);
		gtk_container_set_border_width (GTK_CONTAINER (frame), 0);

		notebook_tabs[i].Function(frame);

		label = gtk_label_new_with_mnemonic (notebook_tabs[i].tab_name);

		g_object_set_data(G_OBJECT(frame),"tab_ident",GINT_TO_POINTER(notebook_tabs[i].tab_ident));
		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);
	}
	g_signal_connect(G_OBJECT(notebook),"switch-page",
			G_CALLBACK(page_changed),NULL);

	hbox = gtk_hbox_new(FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox),5);

	button = gtk_button_new_with_label("            Exit           ");
	gtk_box_pack_start(GTK_BOX(hbox),button,FALSE,FALSE,0);
	g_signal_connect(G_OBJECT(button),"pressed",
			G_CALLBACK(leave),NULL);

	if(tips_in_use)
		gtk_tooltips_enable(tip);
	else
		gtk_tooltips_disable(tip);

	gtk_widget_show_all(main_window);

	return TRUE;
}
