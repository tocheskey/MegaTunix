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

#include <assert.h>
#include <config.h>
#include <conversions.h>
#include <defines.h>
#include <debugging.h>
#include <enums.h>
#include <glade/glade.h>
#include <gui_handlers.h>
#include <listmgmt.h>
#include <lookuptables.h>
#include <notifications.h>
#include <mtxmatheval.h>
#include <plugin.h>
#include <rtv_processor.h>
#include <stdio.h>
#include <stdlib.h>
#include <tabloader.h>


extern gconstpointer *global_data;

/*!
 \brief convert_before_download() converts the value passed using the
 conversions bound to the widget
 \param widget (GtkWidget *) widget to extract the conversion info from
 \param value (gfloat *) the "real world" value from the tuning gui before
 translation to MS-units
 \returns the integere ms-units form after conversion
 */
G_MODULE_EXPORT gint convert_before_download(GtkWidget *widget, gfloat value)
{
	static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
	gint return_value = 0;
	gint tmpi = 0;
	gchar * conv_expr = NULL;
	void *evaluator = NULL;
	DataSize size = MTX_U08;
	float lower = 0.0;
	float upper = 0.0;
	gfloat *multiplier = NULL;
	gfloat *adder = NULL;
	guint i = 0;
	GHashTable *mhash = NULL;
	GHashTable *ahash = NULL;
	gchar *key_list = NULL;
	gchar *mult_list = NULL;
	gchar *add_list = NULL;
	gchar **keys = NULL;
	gchar **mults = NULL;
	gchar **adds = NULL;
	gint table_num = 0;
	gchar *tmpbuf = NULL;
	gchar * source_key = NULL;
	gchar * hash_key = NULL;
	gint *algorithm = NULL;
	GHashTable *sources_hash = NULL;

	sources_hash = DATA_GET(global_data,"sources_hash");
	algorithm = DATA_GET(global_data,"algorithm");

	g_static_mutex_lock(&mutex);


	if (!OBJ_GET(widget,"size"))
		printf(__FILE__"%s %s\n",_(": convert_before_download, FATAL ERROR, size undefined for widget %s "),glade_get_widget_name(widget));

	size = (DataSize)OBJ_GET(widget,"size");
	if (OBJ_GET(widget,"raw_lower"))
		lower = (gfloat)strtol(OBJ_GET(widget,"raw_lower"),NULL,10);
	else
		lower = (gfloat)get_extreme_from_size(size,LOWER);
	if (OBJ_GET(widget,"raw_upper"))
		upper = (gfloat)strtol(OBJ_GET(widget,"raw_upper"),NULL,10);
	else
		upper = (gfloat)get_extreme_from_size(size,UPPER);

	/* MULTI EXPRESSION ONLY! */
	if (OBJ_GET(widget,"multi_expr_keys"))
	{
		if ((!OBJ_GET(widget,"mhash")) && (!OBJ_GET(widget,"ahash")))
		{
			mhash = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
			ahash = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
			key_list = OBJ_GET(widget,"multi_expr_keys");
			mult_list = OBJ_GET(widget,"fromecu_mults");
			add_list = OBJ_GET(widget,"fromecu_addds");
			keys = g_strsplit(key_list,",",-1);
			mults = g_strsplit(mult_list,",",-1);
			adds = g_strsplit(add_list,",",-1);
			for (i=0;i<MIN(g_strv_length(keys),g_strv_length(mults));i++)
			{
				multiplier = g_new0(gfloat, 1);
				*multiplier = (gfloat)g_strtod(mults[i],NULL);
				g_hash_table_insert(mhash,g_strdup(keys[i]),multiplier);
				adder = g_new0(gfloat, 1);
				*adder = (gfloat)g_strtod(adds[i],NULL);
				g_hash_table_insert(ahash,g_strdup(keys[i]),adder);
			}
			g_strfreev(keys);
			g_strfreev(mults);
			g_strfreev(adds);

			OBJ_SET_FULL(widget,"mhash",mhash,g_hash_table_destroy);
			OBJ_SET_FULL(widget,"ahash",ahash,g_hash_table_destroy);
		}
		mhash = OBJ_GET(widget,"mhash");
		ahash = OBJ_GET(widget,"ahash");
		source_key = OBJ_GET(widget,"source_key");
		if (!source_key)
			printf(_("big problem, source key is undefined!!\n"));
		hash_key = (gchar *)g_hash_table_lookup(sources_hash,source_key);
		tmpbuf = (gchar *)OBJ_GET(widget,"table_num");
		if (tmpbuf)
			table_num = (GINT)strtol(tmpbuf,NULL,10);
		if (table_num == -1) /* Not a table */
		{
			if (!hash_key)
			{
				multiplier = g_hash_table_lookup(mhash,"DEFAULT");
				adder = g_hash_table_lookup(ahash,"DEFAULT");
			}
			else
			{
				multiplier = g_hash_table_lookup(mhash,(gchar *)hash_key);
				adder = g_hash_table_lookup(ahash,(gchar *)hash_key);
			}
		}
		else /* This is a 3d table */
		{
			switch (algorithm[table_num])
			{
				case SPEED_DENSITY:
					if (!hash_key)
					{
						multiplier = g_hash_table_lookup(mhash,"DEFAULT");
						adder = g_hash_table_lookup(ahash,"DEFAULT");
					}
					else
					{
						multiplier = g_hash_table_lookup(mhash,hash_key);
						adder = g_hash_table_lookup(ahash,hash_key);
					}
					break;
				case ALPHA_N:
					multiplier = g_hash_table_lookup(mhash,"DEFAULT");
					adder = g_hash_table_lookup(ahash,"DEFAULT");
					break;
				case MAF:
					multiplier = g_hash_table_lookup(mhash,"AFM_VOLTS");
					adder = g_hash_table_lookup(ahash,"AFM_VOLTS");
					break;
			}
		}
		/* Reverse calc due to this being TO the ecu */
		if ((multiplier) && (adder))
			return_value = (GINT)((value - (*adder))/(*multiplier));
		else if (multiplier)
			return_value = (GINT)(value/(*multiplier));
		else
			return_value = (GINT)value;
	}
	else /* NON Multi Expression */
	{
		conv_expr = (gchar *)OBJ_GET(widget,"toecu_conv_expr");

		/* Expression is NOT multi expression but has more complex math*/
		if (conv_expr)
		{
			evaluator = (void *)OBJ_GET(widget,"dl_evaluator");
			if (!evaluator)
			{
				evaluator = evaluator_create(conv_expr);
				assert(evaluator);
				OBJ_SET_FULL(widget,"dl_evaluator",(gpointer)evaluator,evaluator_destroy);
			}
			return_value = (GINT)evaluator_evaluate_x(evaluator,value); 
		}
		else
		{
			multiplier = (gfloat *)OBJ_GET(widget,"fromecu_mult");
			adder = (gfloat *)OBJ_GET(widget,"fromecu_add");
			/* Handle all cases of with or without multiplier/adder*/
			if ((multiplier) && (adder))
				return_value = (GINT)((value - (*adder))/(*multiplier));
			else if (multiplier)
				return_value = (GINT)(value/(*multiplier));
			else
				return_value = (GINT)value;
		}
	}
	dbg_func(CONVERSIONS,g_strdup_printf(__FILE__": convert_before_dl():\n\t widget %s raw %.2f, sent %i\n",glade_get_widget_name(widget),value,return_value));
	if (return_value > upper) 
	{
		dbg_func(CONVERSIONS|CRITICAL,g_strdup_printf(__FILE__": convert_before_download()\n\t WARNING value clamped at %f (%f <- %f -> %f)!!\n",upper,lower,value,upper));
		return_value = upper;
	}
	if (return_value < lower)
	{
		dbg_func(CONVERSIONS|CRITICAL,g_strdup_printf(__FILE__": convert_before_download()\n\t WARNING value clamped at %f (%f <- %f -> %f)!!\n",lower,lower,value,upper));
		return_value = lower;
	}

	tmpi = return_value;
	if (OBJ_GET(widget,"lookuptable"))
		return_value = (GINT)reverse_lookup_obj(G_OBJECT(widget),tmpi);

	g_static_mutex_unlock(&mutex);
	return (return_value);
}


/*!
 \brief convert_after_upload() converts the ms-units data to the real world
 units for display on the GUI
 \param widget (GtkWidget *) to extract the conversion info from to perform
 the necessary math
 \returns the real world value for the GUI
 */
G_MODULE_EXPORT gfloat convert_after_upload(GtkWidget * widget)
{
	static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
	static gint (*get_ecu_data_f)(gpointer);
	static void (*send_to_ecu_f)(gpointer, gint, gboolean) = NULL;
	gfloat return_value = 0.0;
	gchar * conv_expr = NULL;
	void *evaluator = NULL;
	gint tmpi = 0;
	DataSize size = 0;
	gfloat lower = 0.0;
	gfloat upper = 0.0;
	gboolean fromecu_complex = FALSE;
	guint i = 0;
	gint table_num = -1;
	GHashTable *mhash = NULL;
	GHashTable *ahash = NULL;
	gchar *key_list = NULL;
	gchar *mult_list = NULL;
	gchar *add_list = NULL;
	gchar **keys = NULL;
	gchar **mults = NULL;
	gchar **adds = NULL;
	gchar * tmpbuf = NULL;
	gchar * source_key = NULL;
	gchar * hash_key = NULL;
	gfloat *multiplier = NULL;
	gfloat *adder = NULL;
	gint *algorithm = NULL;
	GHashTable *sources_hash = NULL;
	extern gconstpointer *global_data;


	if (!get_ecu_data_f)
		get_symbol("get_ecu_data",(void *)&get_ecu_data_f);
	if (!send_to_ecu_f)
		get_symbol("send_to_ecu",(void *)&send_to_ecu_f);
	g_return_val_if_fail(get_ecu_data_f,0.0);
	g_return_val_if_fail(send_to_ecu_f,0.0);

	g_static_mutex_lock(&mutex);

	size = (DataSize)OBJ_GET(widget,"size");
	if (size == 0)
	{
		printf(_("BIG PROBLEM, size undefined! widget %s, default to U08 \n"),(gchar *)glade_get_widget_name(widget));
		size = MTX_U08;
	}

	if (OBJ_GET(widget,"raw_lower"))
		lower = (gfloat)strtol(OBJ_GET(widget,"raw_lower"),NULL,10);
	else
		lower = (gfloat)get_extreme_from_size(size,LOWER);
	if (OBJ_GET(widget,"raw_upper"))
		upper = (gfloat)strtol(OBJ_GET(widget,"raw_upper"),NULL,10);
	else
		upper = (gfloat)get_extreme_from_size(size,UPPER);

	fromecu_complex = (GBOOLEAN)OBJ_GET(widget,"fromecu_complex");
	if (fromecu_complex)
	{
		g_static_mutex_unlock(&mutex);
		/*printf("Complex upload conversion for widget at page %i, offset %i, name %s\n",(GINT)OBJ_GET(widget,"page"),(GINT)OBJ_GET(widget,"offset"),glade_get_widget_name(widget));
		  */
		return handle_complex_expr_obj(G_OBJECT(widget),NULL,UPLOAD);
	}

	if (OBJ_GET(widget,"lookuptable"))
		tmpi = lookup_data_obj(G_OBJECT(widget),get_ecu_data_f(widget));
	else
		tmpi = get_ecu_data_f(widget);
	if (tmpi < lower)
	{
		dbg_func(CONVERSIONS|CRITICAL,g_strdup_printf(__FILE__": convert_after_upload()\n\t WARNING RAW value  out of range for widget %s, clamped at %.1f (%.1f <- %i -> %.1f), updating ECU with valid value within limits!!\n",(gchar *)glade_get_widget_name(widget),lower,lower,tmpi,upper));
		tmpi = lower;
		send_to_ecu_f(widget,tmpi,TRUE);
	}
	if (tmpi > upper)
	{
		dbg_func(CONVERSIONS|CRITICAL,g_strdup_printf(__FILE__": convert_after_upload()\n\t WARNING RAW value out of range for widget %s, clamped at %.1f (%.1f <- %i -> %.1f), updating ECU with valid value within limits!!\n",(gchar *)glade_get_widget_name(widget),lower,lower,tmpi,upper));
		tmpi = upper;
		send_to_ecu_f(widget,tmpi,TRUE);
	}
	/* MULTI EXPRESSION ONLY! */
	if (OBJ_GET(widget,"multi_expr_keys"))
	{
		sources_hash = DATA_GET(global_data,"sources_hash");
		if ((!OBJ_GET(widget,"mhash")) && (!OBJ_GET(widget,"ahash")))
		{
			mhash = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
			ahash = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
			key_list = OBJ_GET(widget,"multi_expr_keys");
			mult_list = OBJ_GET(widget,"fromecu_mults");
			add_list = OBJ_GET(widget,"fromecu_adds");
			if (!mult_list)
				printf("BUG, widget %s is multi_expression but doesn't have fromecu_mults defined!\n",glade_get_widget_name(widget));
			if (!add_list)
				printf("BUG, widget %s is multi_expression but doesn't have fromecu_adds defined!\n",glade_get_widget_name(widget));
			keys = g_strsplit(key_list,",",-1);
			mults = g_strsplit(mult_list,",",-1);
			adds = g_strsplit(add_list,",",-1);
			for (i=0;i<MIN(g_strv_length(keys),g_strv_length(mults));i++)
			{
				multiplier = g_new0(gfloat, 1);
				*multiplier = (gfloat)g_strtod(mults[i],NULL);
				g_hash_table_insert(mhash,g_strdup(keys[i]),multiplier);
				adder = g_new0(gfloat, 1);
				*adder = (gfloat)g_strtod(adds[i],NULL);
				g_hash_table_insert(ahash,g_strdup(keys[i]),adder);
			}
			g_strfreev(keys);
			g_strfreev(mults);
			g_strfreev(adds);

			OBJ_SET_FULL(widget,"mhash",mhash,g_hash_table_destroy);
			OBJ_SET_FULL(widget,"ahash",ahash,g_hash_table_destroy);
		}
		mhash = OBJ_GET(widget,"mhash");
		ahash = OBJ_GET(widget,"ahash");
		source_key = OBJ_GET(widget,"source_key");
		if (!source_key)
			printf(_("big problem, source key is undefined!!\n"));
		hash_key = (gchar *)g_hash_table_lookup(sources_hash,source_key);
		tmpbuf = (gchar *)OBJ_GET(widget,"table_num");
		if (tmpbuf)
			table_num = (GINT)strtol(tmpbuf,NULL,10);
		if (table_num == -1)
		{
			if (!hash_key)
			{
				multiplier = g_hash_table_lookup(mhash,"DEFAULT");
				adder = g_hash_table_lookup(ahash,"DEFAULT");
			}
			else
			{
				multiplier = g_hash_table_lookup(mhash,(gchar *)hash_key);
				adder = g_hash_table_lookup(ahash,(gchar *)hash_key);
			}
		}
		else
		{
			algorithm = DATA_GET(global_data,"algorithm");
			switch (algorithm[table_num])
			{
				case SPEED_DENSITY:
					if (!hash_key)
					{
						multiplier = g_hash_table_lookup(mhash,"DEFAULT");
						adder = g_hash_table_lookup(ahash,"DEFAULT");
					}
					else
					{
						multiplier = g_hash_table_lookup(mhash,hash_key);
						adder = g_hash_table_lookup(ahash,hash_key);
					}
					break;
				case ALPHA_N:
					multiplier = g_hash_table_lookup(mhash,"DEFAULT");
					adder = g_hash_table_lookup(ahash,"DEFAULT");
					break;
				case MAF:
					multiplier = g_hash_table_lookup(mhash,"AFM_VOLTS");
					adder = g_hash_table_lookup(ahash,"AFM_VOLTS");
					break;
			}
		}
		if ((multiplier) && (adder))
			return_value = (((gfloat)tmpi * (*multiplier)) + (*adder));
		else if (multiplier)
			return_value = (gfloat)tmpi * (*multiplier);
		else
			return_value = (gfloat)tmpi;
	}
	else
	{
		conv_expr = (gchar *)OBJ_GET(widget,"fromecu_conv_expr");
		if (conv_expr)
		{
			evaluator = (void *)OBJ_GET(widget,"ul_evaluator");
			if (!evaluator)
			{
				evaluator = evaluator_create(conv_expr);
				assert(evaluator);
				OBJ_SET_FULL(widget,"ul_evaluator",(gpointer)evaluator,evaluator_destroy);
			}
			return_value = evaluator_evaluate_x(evaluator,tmpi);
		}
		else
		{
			multiplier = OBJ_GET(widget,"fromecu_mult");
			adder = OBJ_GET(widget,"fromecu_add");
			if ((multiplier) && (adder))
				return_value = (((gfloat)tmpi * (*multiplier)) + (*adder));
			else if (multiplier)
				return_value = (gfloat)tmpi * (*multiplier);
			else
				return_value = (gfloat)tmpi;
			/*dbg_func(CONVERSIONS,g_strdup_printf(__FILE__": convert_after_ul():\n\tNo/Fast CONVERSION defined for  widget %s, value %f\n",(gchar *)glade_get_widget_name(widget), return_value));*/
		}

	}
//	dbg_func(CONVERSIONS,g_strdup_printf(__FILE__": convert_after_ul()\n\t page %i,offset %i, raw %i, val %f\n",page,offset,tmpi,return_value));
	g_static_mutex_unlock(&mutex);
	return (return_value);
}


/*!
 \brief convert_temps() changes the values of controls based on the currently
 selected temperature scale.  IT works for labels, spinbuttons, etc...
 \param widget (gpointer) pointer to the widget that contains the necessary
 paramaters re temp (Alt label, etc)
 \param units (gpointer) the temp scale selected
 */
G_MODULE_EXPORT void convert_temps(gpointer widget, gpointer units)
{
	static void (*update_widget_f)(gpointer, gpointer);
	static gboolean (*check_deps)(gconstpointer *) = NULL;
	gconstpointer *dep_obj = NULL;
	gfloat upper = 0.0;
	gfloat lower = 0.0;
	gfloat value = 0.0;
	gchar * text = NULL;
	GtkAdjustment * adj = NULL;
	gboolean state = FALSE;
	gint widget_temp = -1;
	/*extern GdkColor black;*/
	extern gconstpointer *global_data;

	/* If this widgt depends on anything call check_dependancy which will
	 * return TRUE/FALSE.  True if what it depends on is in the matching
	 * state, FALSE otherwise...
	 */
	if ((!widget) || (DATA_GET(global_data,"leaving")))
		return;
	if (!check_deps)
		if (!get_symbol("check_dependancies",(void *)&check_deps))
			dbg_func(CRITICAL|CONVERSIONS,g_strdup_printf(__FILE__": convert_temps()\n\tCan NOT locate \"check_dependancies\" function pointer in plugins, BUG!\n"));
	if (!update_widget_f)
		if(!get_symbol("update_widget",(void *)&update_widget_f))
			dbg_func(CRITICAL|CONVERSIONS,g_strdup_printf(__FILE__": convert_temps()\n\tCan NOT locate \"update_widget\" function pointer in plugins, BUG!\n"));
	dep_obj = (gconstpointer *)OBJ_GET(widget,"dep_object");
	widget_temp = (GINT)OBJ_GET(widget,"widget_temp");
	if (dep_obj)
	{
		if (check_deps)
			state = check_deps(dep_obj);
		else
			dbg_func(CRITICAL|CONVERSIONS,g_strdup_printf(__FILE__": convert_temps()\n\tWidget %s has dependant object bound but can't locate function ptr for \"check_dependancies\" from plugins, BUG!\n",glade_get_widget_name(widget)));
	}

	switch ((TempUnits)units)
	{
		case FAHRENHEIT:
			/*printf("fahr %s\n",glade_get_widget_name(widget));*/
			if (GTK_IS_LABEL(widget))
			{
				if ((dep_obj) && (state))	
					text = (gchar *)OBJ_GET(widget,"alt_f_label");
				else
					text = (gchar *)OBJ_GET(widget,"f_label");
				gtk_label_set_text(GTK_LABEL(widget),text);
				//gtk_widget_modify_text(widget,GTK_STATE_NORMAL,&black);
				OBJ_SET(widget,"widget_temp",GINT_TO_POINTER(units));

			}
			if ((GTK_IS_SPIN_BUTTON(widget)) && (widget_temp != FAHRENHEIT))
			{

				adj = (GtkAdjustment *) gtk_spin_button_get_adjustment(
						GTK_SPIN_BUTTON(widget));
				upper = adj->upper;
				value = adj->value;
				lower = adj->lower;
				if (widget_temp == CELSIUS)
				{
					adj->value = c_to_f(value);
					adj->lower = c_to_f(lower);
					adj->upper = c_to_f(upper);
				}
				else /* Previous is kelvin */
				{
					adj->value = k_to_f(value);
					adj->lower = k_to_f(lower);
					adj->upper = k_to_f(upper);
				}

				gtk_adjustment_changed(adj);
				OBJ_SET(widget,"widget_temp",GINT_TO_POINTER(units));
				update_widget_f(widget,NULL);
			}
			if ((GTK_IS_ENTRY(widget)) && (widget_temp != FAHRENHEIT))
			{
				OBJ_SET(widget,"widget_temp",GINT_TO_POINTER(units));
				update_widget_f(widget,NULL);
			}
			if ((GTK_IS_RANGE(widget)) && (widget_temp != FAHRENHEIT))
			{
				adj = (GtkAdjustment *) gtk_range_get_adjustment(
						GTK_RANGE(widget));
				upper = adj->upper;
				lower = adj->lower;
				value = adj->value;
				if (widget_temp == CELSIUS)
				{
					adj->value = c_to_f(value);
					adj->lower = c_to_f(lower);
					adj->upper = c_to_f(upper);
				}
				else /* Previous is kelvin */
				{
					adj->value = k_to_f(value);
					adj->lower = k_to_f(lower);
					adj->upper = k_to_f(upper);
				}

				gtk_range_set_adjustment(GTK_RANGE(widget),adj);
				OBJ_SET(widget,"widget_temp",GINT_TO_POINTER(units));
			}
			break;
		case CELSIUS:
			/*printf("fahr %s\n",glade_get_widget_name(widget));*/
			if (GTK_IS_LABEL(widget))
			{
				if ((dep_obj) && (state))	
					text = (gchar *)OBJ_GET(widget,"alt_c_label");
				else
					text = (gchar *)OBJ_GET(widget,"c_label");
				gtk_label_set_text(GTK_LABEL(widget),text);
				//gtk_widget_modify_text(widget,GTK_STATE_NORMAL,&black);
				OBJ_SET(widget,"widget_temp",GINT_TO_POINTER(units));

			}
			if ((GTK_IS_SPIN_BUTTON(widget)) && (widget_temp != CELSIUS))
			{

				adj = (GtkAdjustment *) gtk_spin_button_get_adjustment(
						GTK_SPIN_BUTTON(widget));
				upper = adj->upper;
				value = adj->value;
				lower = adj->lower;
				if (widget_temp == FAHRENHEIT)
				{
					adj->value = f_to_c(value);
					adj->lower = f_to_c(lower);
					adj->upper = f_to_c(upper);
				}
				else /* Previous is kelvin */
				{
					adj->value = k_to_c(value);
					adj->lower = k_to_c(lower);
					adj->upper = k_to_c(upper);
				}

				gtk_adjustment_changed(adj);
				OBJ_SET(widget,"widget_temp",GINT_TO_POINTER(units));
				update_widget_f(widget,NULL);
			}
			if ((GTK_IS_ENTRY(widget)) && (widget_temp != CELSIUS))
			{
				OBJ_SET(widget,"widget_temp",GINT_TO_POINTER(units));
				update_widget_f(widget,NULL);
			}
			if ((GTK_IS_RANGE(widget)) && (widget_temp != CELSIUS))
			{
				adj = (GtkAdjustment *) gtk_range_get_adjustment(
						GTK_RANGE(widget));
				upper = adj->upper;
				lower = adj->lower;
				value = adj->value;
				if (widget_temp == FAHRENHEIT)
				{
					adj->value = f_to_c(value);
					adj->lower = f_to_c(lower);
					adj->upper = f_to_c(upper);
				}
				else /* Previous is kelvin */
				{
					adj->value = k_to_c(value);
					adj->lower = k_to_c(lower);
					adj->upper = k_to_c(upper);
				}

				gtk_range_set_adjustment(GTK_RANGE(widget),adj);
				OBJ_SET(widget,"widget_temp",GINT_TO_POINTER(units));
			}
			break;
		case KELVIN:
			/*printf("fahr %s\n",glade_get_widget_name(widget));*/
			if (GTK_IS_LABEL(widget))
			{
				if ((dep_obj) && (state))	
					text = (gchar *)OBJ_GET(widget,"alt_k_label");
				else
					text = (gchar *)OBJ_GET(widget,"k_label");
				gtk_label_set_text(GTK_LABEL(widget),text);
				//gtk_widget_modify_text(widget,GTK_STATE_NORMAL,&black);
				OBJ_SET(widget,"widget_temp",GINT_TO_POINTER(units));

			}
			if ((GTK_IS_SPIN_BUTTON(widget)) && (widget_temp != KELVIN))
			{

				adj = (GtkAdjustment *) gtk_spin_button_get_adjustment(
						GTK_SPIN_BUTTON(widget));
				upper = adj->upper;
				value = adj->value;
				lower = adj->lower;
				if (widget_temp == FAHRENHEIT)
				{
					adj->value = f_to_k(value);
					adj->lower = f_to_k(lower);
					adj->upper = f_to_k(upper);
				}
				else /* Previous is celsius */
				{
					adj->value = c_to_k(value);
					adj->lower = c_to_k(lower);
					adj->upper = c_to_k(upper);
				}

				gtk_adjustment_changed(adj);
				OBJ_SET(widget,"widget_temp",GINT_TO_POINTER(units));
				update_widget_f(widget,NULL);
			}
			if ((GTK_IS_ENTRY(widget)) && (widget_temp != KELVIN))
			{
				OBJ_SET(widget,"widget_temp",GINT_TO_POINTER(units));
				update_widget_f(widget,NULL);
			}
			if ((GTK_IS_RANGE(widget)) && (widget_temp != KELVIN))
			{
				adj = (GtkAdjustment *) gtk_range_get_adjustment(
						GTK_RANGE(widget));
				upper = adj->upper;
				lower = adj->lower;
				value = adj->value;
				if (widget_temp == FAHRENHEIT)
				{
					adj->value = f_to_k(value);
					adj->lower = f_to_k(lower);
					adj->upper = f_to_k(upper);
				}
				else /* Previous is celsius */
				{
					adj->value = c_to_k(value);
					adj->lower = c_to_k(lower);
					adj->upper = c_to_k(upper);
				}

				gtk_range_set_adjustment(GTK_RANGE(widget),adj);
				OBJ_SET(widget,"widget_temp",GINT_TO_POINTER(units));
			}
			break;
	}
}


/*!
 \brief reset_temps() calls the convert_temps function for each widget in
 the "temperature" list
 \param type (gpointer) the temp scale now selected
 */
G_MODULE_EXPORT void reset_temps(gpointer type)
{
	/* Better way.. :) */
	g_list_foreach(get_list("temperature"),convert_temps,type);

}


G_MODULE_EXPORT gdouble temp_to_host(gdouble in)
{
	static Firmware_Details *firmware = NULL;
	gdouble res = 0.0;

	TempUnits mtx_temp_units = (TempUnits)DATA_GET(global_data,"mtx_temp_units");

	if (!firmware)
		firmware = DATA_GET(global_data,"firmware");
	g_return_val_if_fail(firmware,in);
	g_return_val_if_fail(DATA_GET(global_data,"mtx_temp_units"),in);

	if (firmware->ecu_temp_units == mtx_temp_units)
		res = in;
	else if ((firmware->ecu_temp_units == CELSIUS) && (mtx_temp_units == FAHRENHEIT))
		res = c_to_f(in);
	else if ((firmware->ecu_temp_units == CELSIUS) && (mtx_temp_units == KELVIN))
		res = c_to_k(in);
	else if ((firmware->ecu_temp_units == FAHRENHEIT) && (mtx_temp_units == KELVIN))
		res = f_to_k(in);
	else if ((firmware->ecu_temp_units == FAHRENHEIT) && (mtx_temp_units == CELSIUS))
		res = f_to_c(in);
	else if ((firmware->ecu_temp_units == KELVIN) && (mtx_temp_units == CELSIUS))
		res = k_to_c(in);
	else if ((firmware->ecu_temp_units == KELVIN) && (mtx_temp_units == FAHRENHEIT))
		res = k_to_f(in);
	else 
	{
		printf("temp_to_host(): BUG couldn't figure out temp conversion!\n");
		res = in;
	}
	return res;
}

G_MODULE_EXPORT gdouble temp_to_ecu(gdouble in)
{
	gdouble res = 0.0;
	static Firmware_Details *firmware = NULL;
	TempUnits mtx_temp_units = (TempUnits)DATA_GET(global_data,"mtx_temp_units");

	if (!firmware)
		firmware = DATA_GET(global_data,"firmware");
	g_return_val_if_fail(firmware,in);
	g_return_val_if_fail(DATA_GET(global_data,"mtx_temp_units"),in);

	if(firmware->ecu_temp_units == mtx_temp_units)
		res = in;
	else if ((firmware->ecu_temp_units == CELSIUS) && (mtx_temp_units == FAHRENHEIT))
		res = f_to_c(in);
	else if ((firmware->ecu_temp_units == CELSIUS) && (mtx_temp_units == KELVIN))
		res = k_to_c(in);
	else if ((firmware->ecu_temp_units == FAHRENHEIT) && (mtx_temp_units == KELVIN))
		res = k_to_f(in);
	else if ((firmware->ecu_temp_units == FAHRENHEIT) && (mtx_temp_units == CELSIUS))
		res = c_to_f(in);
	else if ((firmware->ecu_temp_units == KELVIN) && (mtx_temp_units == CELSIUS))
		res = c_to_k(in);
	else if ((firmware->ecu_temp_units == KELVIN) && (mtx_temp_units == FAHRENHEIT))
		res = f_to_k(in);
	else
	{
		printf("temp_to_ecu(): BUG couldn't figure out temp conversion!\n");
		res = in;
	}
	return res;
}


G_MODULE_EXPORT gdouble c_to_f(gdouble in)
{
	return ((in *(9.0/5.0))+32.0)+0.001;
}


G_MODULE_EXPORT gdouble c_to_k(gdouble in)
{
	return (in+273.0);
}


G_MODULE_EXPORT gdouble f_to_c(gdouble in)
{
	return ((in-32.0)*(5.0/9.0))+0.001;
}


G_MODULE_EXPORT gdouble f_to_k(gdouble in)
{
	return ((in-32.0)*(5.0/9.0))+273.001;
}


G_MODULE_EXPORT gdouble k_to_f(gdouble in)
{
	return (((in-273) *(9.0/5.0))+32.0)+0.001;
}


G_MODULE_EXPORT gdouble k_to_c(gdouble in)
{
	return (in-273.0);
}


G_MODULE_EXPORT gfloat calc_value(gfloat in, gfloat *mult, gfloat *add, ConvDir dir)
{
	gfloat result = 0.0;

	g_return_val_if_fail(((dir == FROMECU)||(dir == TOECU)),0.0);
	if (dir == FROMECU)
	{
		if ((mult) && (add))
			result = (in * (*mult)) + (*add);
		else if (mult)
			result = (in * (*mult));
		else
			result = in;
	}
	else if (dir == TOECU)
	{
		if ((mult) && (add))
			result = (in - (*add))/(*mult);
		else if (mult)
			result = in/(*mult);
		else
			result = in;
	}
	return result;
}

