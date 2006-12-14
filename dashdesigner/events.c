#include <defines.h>
#include <events.h>
#include <getfiles.h>
#include <gauge.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#ifndef M_PI 
#define M_PI 3.1415926535897932384626433832795 
#endif

static gboolean grabbed = FALSE;
static gboolean resizing = FALSE;
static gboolean moving = FALSE;
static gint corner = -1;
static GtkWidget *dragged_widget = NULL;
static struct T
{
	gint child_x_origin;
	gint child_y_origin;
	gint child_width;
	gint child_height;
	gint rel_grab_x;
	gint rel_grab_y;
}tt = {0,0,0,0};

typedef enum 
{
	LL = 10,
	LR,
	UL,
	UR,
}CornerType;

EXPORT gboolean dashdesigner_about(GtkWidget * widget, gpointer data)
{
#if GTK_MINOR_VERSION >= 8
	gchar *authors[] = {"David Andruczyk",NULL};
	gchar *artists[] = {"Dale Anderson",NULL};
	gtk_show_about_dialog(NULL,
			"name","MegaTunix Dashboard Designer",
			"version",VERSION,
			"copyright","David J. Andruczyk(2006)",
			"comments","Dashboard Designer is a tool to design custom Dash gauge layouts for the MegaTunix Megasquirt tuning software",
			"license","GNU GPL v2",
			"website","http://megatunix.sourceforge.net",
			"authors",authors,
			"artists",artists,
			"documenters",authors,
			NULL);
#endif
	return TRUE;
}

EXPORT gboolean import_dash_xml(GtkWidget *widget, gpointer data)
{
	printf("Import Dash XML\n");
	return TRUE;
}

EXPORT gboolean add_gauge(GtkWidget *widget, gpointer data)
{

	static gboolean created = FALSE;
	static GladeXML *preview_xml = NULL;
	GtkWidget * gauge = NULL;
	GtkWidget * table = NULL;
	GtkWidget * window = NULL;
	gchar ** files = NULL;
	extern GladeXML *main_xml;
	gint i = 0;
	if (created)
	{
		window = glade_xml_get_widget(preview_xml,"preview_window");
		if (GTK_IS_WIDGET(window))
			gtk_widget_show_all(window);
		return TRUE;
	}

	preview_xml = glade_xml_new(main_xml->filename, "preview_window", NULL);

	glade_xml_signal_autoconnect(preview_xml);

	table = glade_xml_get_widget(preview_xml,"gauge_preview_table");
	files = get_files(g_strconcat(GAUGES_DIR,PSEP,NULL),g_strdup("xml"));
	if (files)
	{
		while (files[i])
		{
			gauge = mtx_gauge_face_new();
			gtk_table_attach_defaults(GTK_TABLE(table),gauge,0,1,i,i+1);
			gtk_widget_realize(gauge);
			mtx_gauge_face_import_xml(MTX_GAUGE_FACE(gauge),files[i]);
			gtk_widget_set_usize(GTK_WIDGET(gauge),150,150);
			i++;
		}
		gtk_widget_show_all(table);
		g_strfreev(files);
	}
	created = TRUE;
	return TRUE;

}

EXPORT gboolean gauge_choice_button_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GtkWidget *table = NULL;
	gint i = 0;
	gint x_cur = 0;
	gint y_cur = 0;
	gint width = 0;
	gint height = 0;
	gint origin_x = 0;
	gint origin_y = 0;
	gint total_gauges = 0;
	GtkWidget * dash = NULL;
	GtkWidget * gauge = NULL;
	GtkTableChild *child = NULL;
	gchar * filename = NULL;
	gint row = 0;
	extern GladeXML *main_xml;

	gdk_window_get_origin(widget->window,&origin_x,&origin_y);
	gdk_drawable_get_size(widget->window,&width,&height);
	
	/* Current cursor location relatuive to upper left corner */
	x_cur = (gint)event->x_root-origin_x;
	y_cur = (gint)event->y_root-origin_y;

	table = gtk_bin_get_child(GTK_BIN(widget));
	if(GTK_IS_TABLE(table))
		total_gauges = g_list_length(GTK_TABLE(table)->children);

	row = (gint)(((float)y_cur/(float)height)*(float)total_gauges);

	if (event->button == 1)
	{
		for (i=0;i<total_gauges;i++)
		{
			child = (GtkTableChild *) g_list_nth_data(GTK_TABLE(table)->children,i);
			if (row == child->top_attach)
				filename = mtx_gauge_face_get_xml_filename(MTX_GAUGE_FACE(child->widget));
		}
		dash =  glade_xml_get_widget(main_xml,"dashboard");
		gauge = mtx_gauge_face_new();
		gtk_fixed_put(GTK_FIXED(dash),gauge,130,130);
		mtx_gauge_face_import_xml(MTX_GAUGE_FACE(gauge),filename);
		gtk_widget_show_all(dash);
		g_free(filename);
	}

//	printf("button event in gauge choice window at %i,%i\n",x_cur,y_cur);
	return TRUE;

}


EXPORT gboolean dashdesigner_quit(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
	return TRUE;
}


EXPORT gboolean export_dash_xml_default(GtkWidget *widget, gpointer data)
{
	printf("Export Dash XML default\n");
	return TRUE;
}

EXPORT gboolean export_dash_xml_as(GtkWidget *widget, gpointer data)
{
	printf("Export Dash XML as...\n");
	return TRUE;
}

EXPORT gboolean motion_event(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
	gint x_cur;
	gint y_cur;
	gint origin_x;
	gint origin_y;

	gdk_window_get_origin(widget->window,&origin_x,&origin_y);
	/* Current cursor locatio nrelatuive to upper left corner */
	x_cur = (gint)event->x_root-origin_x;
	y_cur = (gint)event->y_root-origin_y;

	//printf("motion event\n");
//	printf("rel movement point %i,%i\n",x_cur,y_cur);
	if (grabbed)
	{
		if (moving)
		{
			gtk_fixed_move(GTK_FIXED(dragged_widget->parent),dragged_widget,
					x_cur-tt.rel_grab_x+tt.child_x_origin,
					y_cur-tt.rel_grab_y+tt.child_y_origin);
		}
		if (resizing)
		{
			if (corner == LR)
			{
				gtk_widget_set_usize(dragged_widget,event->x,event->y);
			}
			else if (corner == UR)
			{
				gtk_widget_set_usize(dragged_widget,
						x_cur-tt.child_x_origin,
						(tt.child_y_origin+tt.child_height)-y_cur);

				gtk_fixed_move(GTK_FIXED(dragged_widget->parent),
						dragged_widget,
						tt.child_x_origin,
						y_cur);
			}
			else if (corner == UL)
			{
				gtk_widget_set_usize(dragged_widget,
						(tt.child_x_origin+tt.child_width)-x_cur,
						(tt.child_y_origin+tt.child_height)-y_cur);

				gtk_fixed_move(GTK_FIXED(dragged_widget->parent),
						dragged_widget,
						x_cur,
						y_cur);
			}
			else if (corner == LL)
			{
				gtk_widget_set_usize(dragged_widget,
						(tt.child_x_origin+tt.child_width)-x_cur,event->y);

				gtk_fixed_move(GTK_FIXED(dragged_widget->parent),
						dragged_widget,
						x_cur,
						tt.child_y_origin);
			}
		}
	}

	return TRUE;
}



EXPORT gboolean button_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GtkWidget * fixed = gtk_bin_get_child(GTK_BIN(widget));
	GList *list = NULL;
	gint i = 0;
	gint origin_x = 0;
	gint origin_y = 0;
	gboolean found_one = FALSE;
	gint x_cur = 0;
	gint y_cur = 0;
	GtkFixedChild * fchild = NULL;
	gint len = g_list_length(GTK_FIXED(fixed)->children);

	if (event->state & GDK_BUTTON1_MASK)
	{
		grabbed = FALSE;
		moving = FALSE;
		resizing = FALSE;
		corner = -1;
		//printf("button1 released, unlocking\n");
		return TRUE;
	}

	gdk_window_get_origin(widget->window,&origin_x,&origin_y);
	/* Current cursor locatio nrelatuive to upper left corner */
	x_cur = (gint)event->x_root-origin_x;
	y_cur = (gint)event->y_root-origin_y;

	if ((event->button == 1) || (event->button == 3))
	{
		list = g_list_first(GTK_FIXED(fixed)->children);
		for (i=0;i<len;i++)
		{
			fchild = (GtkFixedChild *)  g_list_nth_data(list,i);

			tt.child_x_origin = fchild->widget->allocation.x;
			tt.child_y_origin = fchild->widget->allocation.y;
			tt.child_width = fchild->widget->allocation.width;
			tt.child_height = fchild->widget->allocation.height;

			if ((x_cur > tt.child_x_origin) && (x_cur < (tt.child_x_origin+tt.child_width)) && (y_cur > tt.child_y_origin) && (y_cur < (tt.child_y_origin+tt.child_height))) 
			{
				found_one = TRUE;
				raise_fixed_child(fchild->widget);
				mtx_gauge_face_set_show_drag_border(MTX_GAUGE_FACE(fchild->widget),found_one);
				break;
			}
			else
			{
				found_one = FALSE;
				mtx_gauge_face_set_show_drag_border(MTX_GAUGE_FACE(fchild->widget),found_one);
			}
		}
		if (found_one)
		{
			if (event->button == 3)
			{
				gtk_widget_destroy(fchild->widget);
			}
			if (event->button == 1)
			{
				//printf("grabbed it \n");
				grabbed = TRUE;
				dragged_widget = fchild->widget;
				mtx_gauge_face_set_show_drag_border(MTX_GAUGE_FACE(fchild->widget),found_one);
				tt.rel_grab_x=x_cur;
				tt.rel_grab_y=y_cur;

				if (((x_cur < (tt.child_x_origin+DRAG_BORDER)) && ((y_cur < tt.child_y_origin+DRAG_BORDER) || (y_cur > tt.child_y_origin+tt.child_height-DRAG_BORDER))) || ((x_cur > (tt.child_x_origin+tt.child_width-DRAG_BORDER)) && ((y_cur < tt.child_y_origin+DRAG_BORDER) || (y_cur > tt.child_y_origin+tt.child_height-DRAG_BORDER))))
				{
					resizing = TRUE;
					/* Left border */
					if (x_cur < (tt.child_x_origin+7))
					{
						if (y_cur < (tt.child_y_origin+(0.33*tt.child_height)))
							corner = UL;
						else if (y_cur > (tt.child_y_origin+(0.66*tt.child_height)))
							corner = LL;
						else corner = -1;
						
					}
					/* Right border */
					else if (x_cur > (tt.child_x_origin+tt.child_width-7))
					{
						if (y_cur < (tt.child_y_origin+(0.33*tt.child_height)))
							corner = UR;
						else if (y_cur > (tt.child_y_origin+(0.66*tt.child_height)))
							corner = LR;
						else
							corner = -1;
					}
					/* Top border */
					if (y_cur < (tt.child_y_origin+7))
					{
						if (x_cur < (tt.child_x_origin+(0.33*tt.child_width)))
							corner = UL;
						else if (x_cur > (tt.child_x_origin+(0.66*tt.child_width)))
							corner = UR;
						else
							corner = -1;
					}
					/* Bottom border */
					else if (y_cur > (tt.child_y_origin+tt.child_height-7))
					{
						if (x_cur < (tt.child_x_origin+(0.33*tt.child_width)))
							corner = LL;
						else if (x_cur > (tt.child_x_origin+(0.66*tt.child_width)))
							corner = LR;
						else 
							corner = -1;
					}
				}
				else
					moving = TRUE;


			}
			else
			{
				//printf("didn't grab squat\n");
				grabbed = FALSE;
				moving = FALSE;
				resizing = FALSE;
			}
		}
	}



	/*
	if (moving)
		printf ("MOVING!\n");
	if (resizing)
		printf ("RESIZING\n");
	if (grabbed)
	{
		printf("grabbed TRUE\n");
		printf("child base %i,%i, rel click point %i,%i\n",tt.child_x_origin,tt.child_y_origin,tt.rel_grab_x,tt.rel_grab_y);
	}
	*/
	
	return TRUE;

}

void raise_fixed_child (GtkWidget * widget)
{
	GtkFixed *fixed;

	g_return_if_fail (GTK_IS_FIXED (widget->parent));
	fixed = GTK_FIXED (widget->parent);
	/* If widget hasn't got a window, move it to the back of the parent fixed's
	 *      children. If it has got a window, raise it. */
	/* Note: this is slightly naughty as it changes the GtkFixed's GList of
	 *      children, but it's better than removing the widget and adding it again. */
	if (GTK_WIDGET_NO_WINDOW (widget))
	{
		GList *child;
		GtkFixedChild *data;
		child = fixed->children;
		while (child)
		{
			data = child->data;
			if (data->widget == widget)
			{
				fixed->children = g_list_remove (fixed->children, data);
				fixed->children = g_list_append (fixed->children, data);
				break;
			}
			child = child->next;
		}
	}
	else
	{
		gdk_window_raise (widget->window);
	}
}

