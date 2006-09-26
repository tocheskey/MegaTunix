/* Christopher Mire, 2006
 *
 * -------------------------------------------------------------------------
 *  Hacked and slashed to hell by David J. Andruczyk in order to bend and
 *  tweak it to my needs for MegaTunix.  Added in rendering ability using
 *  cairo and raw GDK callls for those less fortunate (OS-X)
 */



#include <config.h>
#ifdef HAVE_CAIRO
#include <cairo/cairo.h>
#endif
#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <glib-object.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <gauge.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
#define MTX_GAUGE_FACE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MTX_TYPE_GAUGE_FACE, MtxGaugeFacePrivate))

G_DEFINE_TYPE (MtxGaugeFace, mtx_gauge_face, GTK_TYPE_DRAWING_AREA);

typedef struct
{
	gint dragging;
}
MtxGaugeFacePrivate;

static void mtx_gauge_face_set_value_internal (MtxGaugeFace *, float );
static void mtx_gauge_face_class_init (MtxGaugeFaceClass *);
static void mtx_gauge_face_init (MtxGaugeFace *);
static void generate_gauge_background(GtkWidget *);
static void gdk_generate_gauge_background(GtkWidget *);
static void cairo_generate_gauge_background(GtkWidget *);
static void gdk_update_gauge_position (GtkWidget *);
static void cairo_update_gauge_position (GtkWidget *);
static void update_gauge_position (GtkWidget *);
static gboolean mtx_gauge_face_configure (GtkWidget *, GdkEventConfigure *);
static gboolean mtx_gauge_face_expose (GtkWidget *, GdkEventExpose *);
static gboolean mtx_gauge_face_button_press (GtkWidget *,GdkEventButton *);
static void mtx_gauge_face_redraw_canvas (MtxGaugeFace *);
static gboolean mtx_gauge_face_button_release (GtkWidget *,GdkEventButton *);
					       

static void mtx_gauge_face_set_value_internal (MtxGaugeFace *gauge, float value)
{
	gauge->value = value;
	mtx_gauge_face_redraw_canvas (gauge);//show new value
}

static void mtx_gauge_face_set_units_str_internal (MtxGaugeFace *gauge, gchar * units_str)
{
	if (gauge->units_str)
		g_free(gauge->units_str);
	gauge->units_str = g_strdup(units_str);;
	mtx_gauge_face_redraw_canvas (gauge);//show new value
}

/* Changes value stored in widget, and gets widget redrawn to show change */
void mtx_gauge_face_set_value (MtxGaugeFace *gauge, float value)
{
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_object_freeze_notify (G_OBJECT (gauge));
	mtx_gauge_face_set_value_internal (gauge, value);
	g_object_thaw_notify (G_OBJECT (gauge));
}

/* Sets antialias mode */
void mtx_gauge_face_set_antialias(MtxGaugeFace *gauge, gboolean state)
{
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	gauge->antialias = state;
}

/* Changes value stored in widget, and gets widget redrawn to show change */
void mtx_gauge_face_set_units_str (MtxGaugeFace *gauge, gchar * units_str)
{
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_object_freeze_notify (G_OBJECT (gauge));
	mtx_gauge_face_set_units_str_internal (gauge, units_str);
	g_object_thaw_notify (G_OBJECT (gauge));
}

/* Returns value that needle currently points to */
float mtx_gauge_face_get_value (MtxGaugeFace *gauge)
{
	g_return_val_if_fail (MTX_IS_GAUGE_FACE (gauge), -1);
	return gauge->value;
}

/* Returns antialias status */
gboolean mtx_gauge_face_get_antialias (MtxGaugeFace *gauge)
{
	g_return_val_if_fail (MTX_IS_GAUGE_FACE (gauge), FALSE);
	return gauge->antialias;
}

/* Returns value that needle currently points to */
gchar * mtx_gauge_face_get_units_str (MtxGaugeFace *gauge)
{
	g_return_val_if_fail (MTX_IS_GAUGE_FACE (gauge), NULL);
	return gauge->units_str;
}

/* Changes the lower and upper value bounds */
void mtx_gauge_face_set_bounds (MtxGaugeFace *gauge, float value1, float value2)
{
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_object_freeze_notify (G_OBJECT (gauge));
	gauge->lbound = value1;
	gauge->ubound = value2;
	gauge->range = gauge->ubound -gauge->lbound;
	g_object_thaw_notify (G_OBJECT (gauge));
}

/* returns the lower and upper value bounds */
gboolean mtx_gauge_face_get_bounds(MtxGaugeFace *gauge, float *value1, float *value2)
{
	g_return_val_if_fail (MTX_IS_GAUGE_FACE (gauge),FALSE);
	*value1 = gauge->lbound;
	*value2 = gauge->ubound;
	return TRUE;
}
/* Set number of ticks to be shown in the range of the drawn gauge */
void mtx_gauge_face_set_resolution (MtxGaugeFace *gauge, int ticks)
{
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_object_freeze_notify (G_OBJECT (gauge));
	gauge->num_ticks = ticks;
	g_object_thaw_notify (G_OBJECT (gauge));
}

/* Returns current number of ticks drawn in span of gauge */
int mtx_gauge_face_get_resolution (MtxGaugeFace *gauge)
{
	g_return_val_if_fail (MTX_IS_GAUGE_FACE (gauge), -1);
	return gauge->num_ticks;
}

/* Changes the span of the gauge, specified in radians the start and stop position. */
/* Right is 0 going clockwise to M_PI (180 Degrees) back to 0 (2 * M_PI) */
void mtx_gauge_face_set_span (MtxGaugeFace *gauge, float start_radian, float stop_radian)
{
	gint float tmp;
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_object_freeze_notify (G_OBJECT (gauge));
	gauge->start_radian = start_radian;
	gauge->stop_radian = stop_radian;
	/* For some un know ndamn reason,  GDK draws it's arcs
	 * counterclockwisein units of 1/64th fo a degree, whereas cairo
	 * doe it clockwise in radians. At least they start at the same place
	 * "3 O'clock" 
	 * */
	tmp = (start_radian/M_PI) *180
	
	g_object_thaw_notify (G_OBJECT (gauge));
}

static void mtx_gauge_face_class_init (MtxGaugeFaceClass *class_name)
{
	GObjectClass *obj_class;
	GtkWidgetClass *widget_class;

	obj_class = G_OBJECT_CLASS (class_name);
	widget_class = GTK_WIDGET_CLASS (class_name);

	/* GtkWidget signals */
	widget_class->configure_event = mtx_gauge_face_configure;
	widget_class->expose_event = mtx_gauge_face_expose;
	widget_class->button_press_event = mtx_gauge_face_button_press;
	widget_class->button_release_event = mtx_gauge_face_button_release;

	g_type_class_add_private (obj_class, sizeof (MtxGaugeFacePrivate));
}

static void mtx_gauge_face_init (MtxGaugeFace *gauge)
{
	//which events widget receives
	gtk_widget_add_events (GTK_WIDGET (gauge),GDK_BUTTON_PRESS_MASK
			       | GDK_BUTTON_RELEASE_MASK);

	gauge->value = 0.0;//default values
	gauge->lbound = -1.0;
	gauge->ubound = 1.0;
	gauge->start_radian = 0.75 * M_PI;//M_PI is left, 0 is right
	gauge->stop_radian = 2.25 * M_PI;
	gauge->num_ticks = 24;
	gauge->antialias = FALSE;
	gauge->xc = 0.0;
	gauge->yc = 0.0;
	gauge->radius = 0.0;
	gauge->cr = NULL;
	gauge->range = gauge->ubound - gauge->lbound;
#ifndef HAVE_CAIRO
	gauge->colormap = gdk_colormap_get_system();
	gauge->axis_gc = NULL;	
	gauge->needle_gc = NULL;
	gauge->font_gc = NULL;
#endif
	mtx_gauge_face_redraw_canvas (gauge);
}


static void update_gauge_position(GtkWidget *widget)
{
#ifdef HAVE_CAIRO
	cairo_update_gauge_position (widget);
#else
	gdk_update_gauge_position (widget);
#endif
}


static void cairo_update_gauge_position (GtkWidget *widget)
{
	gfloat x = 0.0;
	gfloat y = 0.0;
	gfloat arc = 0.0;
	gfloat arc_rad = 0.0;
	gint last_height = 0.0;
	gchar * message = NULL;
	gfloat current_value = 0.0;
	cairo_t *cr = NULL;
	cairo_text_extents_t extents;

	MtxGaugeFace * gauge = (MtxGaugeFace *)widget;

	/* Copy bacground pixmap to intermediaary for final rendering */
	gdk_draw_drawable(gauge->pixmap,
			widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
			gauge->bg_pixmap,
			0,0,
			0,0,
			widget->allocation.width,widget->allocation.height);


	cr = gdk_cairo_create (gauge->pixmap);
	/* gauge hands */
	arc = (gauge->stop_radian - gauge->start_radian) / (2 * M_PI);

	current_value = gauge->value;
	cairo_set_line_width (cr, 2.5 * cairo_get_line_width (cr));
	cairo_move_to (cr, gauge->xc, gauge->yc);

	arc_rad = (2 * M_PI) * arc;//angle in radians of total arc
	cairo_line_to (cr, gauge->xc + gauge->radius * sin ((current_value - gauge->lbound) * (arc_rad) /
				gauge->range +//needle neutral is 1.5 M_PI
				(gauge->start_radian) - (1.5 * M_PI)),
			gauge->yc + gauge->radius * -cos ((current_value - gauge->lbound) * (arc_rad) /
				gauge->range +//needle neutral is 1.5 M_PI
				(gauge->start_radian) - (1.5 * M_PI)));

	cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_BOLD);

	cairo_set_font_size (cr, gauge->radius / 5);
	message = g_strdup_printf("%.2f", current_value);

	cairo_text_extents (cr, message, &extents);
	x = 0.5-(extents.width/2 + extents.x_bearing);
	y = 0.5-(extents.height/2 + extents.y_bearing);

	cairo_move_to (cr, gauge->xc+x, gauge->yc+(2.5*y));
	cairo_show_text (cr, message);
	g_free(message);
	last_height = extents.height;

	if (gauge->units_str)
	{
		cairo_set_font_size (cr, gauge->radius / 8);

		cairo_text_extents (cr, gauge->units_str, &extents);
		x = 0.5-(extents.width/2 + extents.x_bearing);
		y = 0.5-(extents.height/2 + extents.y_bearing);

		cairo_move_to (cr, gauge->xc+x, gauge->yc+last_height+(4*y));
		cairo_show_text (cr, gauge->units_str);
	}
	
	cairo_stroke (cr);
	cairo_destroy(cr);
}

static gboolean mtx_gauge_face_configure (GtkWidget *widget, GdkEventConfigure *event)
{
	gint w = 0;
	gint h = 0;
	MtxGaugeFace * gauge = MTX_GAUGE_FACE(widget);

	if(widget->window)
	{
		w = widget->allocation.width;
		h = widget->allocation.height;

		if (gauge->axis_gc)
			g_object_unref(gauge->axis_gc);
		if (gauge->needle_gc)
			g_object_unref(gauge->needle_gc);
		if (gauge->font_gc)
			g_object_unref(gauge->font_gc);
		/* Backing pixmap (copy of window) */
		if (gauge->pixmap)
			g_object_unref(gauge->pixmap);
		gauge->pixmap=gdk_pixmap_new(widget->window,
				w,h,
				gtk_widget_get_visual(widget)->depth);
		gdk_draw_rectangle(gauge->pixmap,
				widget->style->black_gc,
				TRUE, 0,0,
				w,h);
		/* Static Background pixmap */
		if (gauge->bg_pixmap)
			g_object_unref(gauge->bg_pixmap);
		gauge->bg_pixmap=gdk_pixmap_new(widget->window,
				w,h,
				gtk_widget_get_visual(widget)->depth);
		gdk_draw_rectangle(gauge->bg_pixmap,
				widget->style->black_gc,
				TRUE, 0,0,
				w,h);

		gdk_window_set_back_pixmap(widget->window,gauge->pixmap,0);
#ifndef HAVE_CAIRO
		gauge->axis_gc = gdk_gc_new(gauge->bg_pixmap);
		gauge->needle_gc = gdk_gc_new(gauge->pixmap);
		gauge->font_gc = gdk_gc_new(gauge->pixmap);
		gdk_gc_set_colormap(gauge->axis_gc,gauge->colormap);
		gdk_gc_set_colormap(gauge->needle_gc,gauge->colormap);
		gdk_gc_set_colormap(gauge->font_gc,gauge->colormap);
#endif
		gauge->xc = w / 2;
		gauge->yc = h / 2;
		gauge->radius = MIN (w/2, h/2) - 5;
			

	}
	generate_gauge_background(widget);
	update_gauge_position(widget);

	return TRUE;
}


static gboolean mtx_gauge_face_expose (GtkWidget *widget, GdkEventExpose *event)
{
	MtxGaugeFace * gauge = MTX_GAUGE_FACE(widget);

	gdk_draw_drawable(widget->window,
			widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
			gauge->pixmap,
			event->area.x, event->area.y,
			event->area.x, event->area.y,
			event->area.width, event->area.height);

	return FALSE;
}


static void generate_gauge_background(GtkWidget *widget)
{
#ifdef HAVE_CAIRO
	gdk_generate_gauge_background(widget);
#else
	gdk_generate_gauge_background(widget);
#endif
}
static void cairo_generate_gauge_background(GtkWidget *widget)
{
	cairo_t *cr = NULL;
	gfloat arc = 0.0;
	gint total_ticks = 0;
	gint left_tick = 0;
	gint right_tick = 0;
	gint counter = 0;
	gint inset = 0;

	MtxGaugeFace * gauge = (MtxGaugeFace *)widget;

	/* get a cairo_t */
	cr = gdk_cairo_create (gauge->bg_pixmap);
	cairo_set_source_rgba (cr, 0, 0, 0, 1.0);
	cairo_rectangle (cr,
			0,0,
			widget->allocation.width, widget->allocation.height);
	if (gauge->antialias)
		cairo_set_antialias(cr,CAIRO_ANTIALIAS_DEFAULT);
	else
		cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);



	/* gauge back */
	cairo_set_source_rgba (cr, 1, 1, 1, 1.0);//white
	cairo_arc (cr, gauge->xc, gauge->yc, gauge->radius, gauge->start_radian,gauge->stop_radian);

	cairo_set_source_rgba (cr, 1, 1, 1, 1.0);//white
	cairo_fill_preserve (cr);
	cairo_set_source_rgba (cr, 0, 0, 0, 1.0);
	cairo_stroke (cr);

	/* gauge ticks */
	arc = (gauge->stop_radian - gauge->start_radian) / (2 * M_PI);
	total_ticks = gauge->num_ticks / arc;//amount of ticks if entire gauge was used
	left_tick = total_ticks * (gauge->start_radian / (2 * M_PI));//start tick
	right_tick = total_ticks * (gauge->stop_radian / (2 * M_PI));//end tick

	for (counter = left_tick ; counter <= right_tick; counter++)
	{
		cairo_save (cr); /* stack-pen-size */
		if (counter % 3 == 0)
		{
			inset = (gint) (0.2 * gauge->radius);
		}
		else
		{
			inset = (gint) (0.1 * gauge->radius);
			cairo_set_line_width (cr, 0.5 *
					cairo_get_line_width (cr));
		}
		cairo_move_to (cr,
				gauge->xc + (gauge->radius - inset) * cos (counter * M_PI / (total_ticks / 2)),
				gauge->yc + (gauge->radius - inset) * sin (counter * M_PI / (total_ticks / 2)));
		cairo_line_to (cr,
				gauge->xc + (gauge->radius * cos (counter * (M_PI / (total_ticks / 2)))),
				gauge->yc + (gauge->radius * sin (counter * (M_PI / (total_ticks / 2)))));
		cairo_stroke (cr);
		cairo_restore (cr); /* stack-pen-size */
	}

	cairo_destroy (cr);
}


static void gdk_generate_gauge_background(GtkWidget *widget)
{
	gint w = 0;
	gint h = 0;
	gfloat arc = 0.0;
	gint total_ticks = 0;
	gint left_tick = 0;
	gint right_tick = 0;
	gint counter = 0;
	gint inset = 0;

	MtxGaugeFace * gauge = (MtxGaugeFace *)widget;
	w = widget->allocation.width;
	h = widget->allocation.height;

	/* Wipe the display */
	gdk_draw_rectangle(gauge->bg_pixmap,
			widget->style->black_gc,
			TRUE, 0,0,
			w,h);

	/* gauge back */
	/* Filled arc in white */
	gdk_draw_arc(gauge->bg_pixmap,widget->style->white_gc,TRUE,
			0,0,
			w,h,
			gauge->start_deg,gauge->stop_deg);

	/* Ouside border line, NOT filled */
	gdk_draw_arc(gauge->bg_pixmap,widget->style->black_gc,FALSE,
			0,0,
			w,h,
			gauge->start_deg,gauge->stop_deg);


	/* gauge ticks */
	arc = (gauge->stop_radian - gauge->start_radian) / (2 * M_PI);
	total_ticks = gauge->num_ticks / arc;//amount of ticks if entire gauge was used
	left_tick = total_ticks * (gauge->start_radian / (2 * M_PI));//start tick
	right_tick = total_ticks * (gauge->stop_radian / (2 * M_PI));//end tick

	for (counter = left_tick ; counter <= right_tick; counter++)
	{
		//cairo_save (cr); /* stack-pen-size */
		if (counter % 3 == 0)
		{
			inset = (gint) (0.2 * gauge->radius);
		}
		else
		{
			inset = (gint) (0.1 * gauge->radius);
//			cairo_set_line_width (cr, 0.5 *
//					cairo_get_line_width (cr));
		}
/*		cairo_move_to (cr,
				gauge->xc + (gauge->radius - inset) * cos (counter * M_PI / (total_ticks / 2)),
				gauge->yc + (gauge->radius - inset) * sin (counter * M_PI / (total_ticks / 2)));
		cairo_line_to (cr,
				gauge->xc + (gauge->radius * cos (counter * (M_PI / (total_ticks / 2)))),
				gauge->yc + (gauge->radius * sin (counter * (M_PI / (total_ticks / 2)))));
		cairo_stroke (cr);
		cairo_restore (cr); // stack-pen-size 
*/
	}

//	cairo_destroy (cr);
}

static gboolean mtx_gauge_face_button_press (GtkWidget *gauge,
					     GdkEventButton *event)
{
	MtxGaugeFacePrivate *priv;
	priv = MTX_GAUGE_FACE_GET_PRIVATE (gauge);

	return FALSE;
}

static void mtx_gauge_face_redraw_canvas (MtxGaugeFace *gauge)
{
	GtkWidget *widget;
	GdkRegion *region;

	widget = GTK_WIDGET (gauge);

	if (!widget->window) return;

	update_gauge_position(widget);
	region = gdk_drawable_get_clip_region (widget->window);
	/* redraw the cairo canvas completely by exposing it */
	gdk_window_invalidate_region (widget->window, region, TRUE);
	gdk_window_process_updates (widget->window, TRUE);

	gdk_region_destroy (region);
}

static gboolean mtx_gauge_face_button_release (GtkWidget *gauge,
					       GdkEventButton *event)
{
	MtxGaugeFacePrivate *priv;
	printf ("button release\n");
	priv = MTX_GAUGE_FACE_GET_PRIVATE (gauge);
	return FALSE;
}

GtkWidget *mtx_gauge_face_new ()
{
	return GTK_WIDGET (g_object_new (MTX_TYPE_GAUGE_FACE, NULL));
}