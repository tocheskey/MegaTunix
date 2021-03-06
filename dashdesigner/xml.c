#include <defines.h>
#include <events.h>
#include <getfiles.h>
#include <gauge.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <rtv_parser.h>
#include <xml.h>
#include <xmlbase.h>


#if defined(LIBXML_TREE_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)

extern GtkBuilder *toplevel;

void import_dash_xml(gchar * filename)
{
	GtkWidget *dash = NULL;
	GList *children = NULL;
	GtkWidget * dialog = NULL;
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;
	gint result = 0;

	g_return_if_fail(filename);

	dash = GTK_WIDGET(gtk_builder_get_object(toplevel,"dashboard"));

	children = GTK_FIXED(dash)->children;
	if (g_list_length(children) > 0)
	{
		dialog = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,
				GTK_MESSAGE_QUESTION,
				GTK_BUTTONS_YES_NO,
				"Dashboard already containts %i gauges, destroy it?",g_list_length(children));
		result = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy(dialog);
		if (result == GTK_RESPONSE_YES)
			clear_dashboard(dash);
		else
			return;
	}
	LIBXML_TEST_VERSION

		/*parse the file and get the DOM */
		doc = xmlReadFile(filename, NULL, 0);

	if (doc == NULL) 
	{
		printf("error: could not parse file %s\n",filename);
		return;
	}
	/*Get the root element node */
	root_element = xmlDocGetRootElement(doc);
	load_elements(dash,root_element);

	OBJ_SET((dash),"dash_xml_filename",g_strdup(filename));
	/*free the document */
	xmlFreeDoc(doc);

	/*
	 *Free the global variables that may
	 *have been allocated by the parser.
	 */
	xmlCleanupParser();

	return ;
}


void load_elements(GtkWidget *dash, xmlNode *a_node)
{
	xmlNode *cur_node = NULL;

	/* Iterate though all nodes... */
	for (cur_node = a_node;cur_node;cur_node = cur_node->next)
	{
		if (cur_node->type == XML_ELEMENT_NODE)
		{
			 if (g_strcasecmp((gchar *)cur_node->name,"dash_geometry") == 0)
				 load_geometry(dash,cur_node);
			 if (g_strcasecmp((gchar *)cur_node->name,"gauge") == 0)
				 load_gauge(dash,cur_node);
		}
		load_elements(dash,cur_node->children);

	}

}

void load_geometry(GtkWidget *dash, xmlNode *node)
{
	xmlNode *cur_node = NULL;
	gint width = 0;
	gint height = 0;
	if (!node->children)
	{
		printf("ERROR, load_geometry, xml node is empty!!\n");
		return;
	}
	cur_node = node->children;
	while (cur_node->next)
	{
		if (cur_node->type == XML_ELEMENT_NODE)
		{
			if (g_strcasecmp((gchar *)cur_node->name,"width") == 0)
				generic_xml_gint_import(cur_node,&width);
			if (g_strcasecmp((gchar *)cur_node->name,"height") == 0)
				generic_xml_gint_import(cur_node,&height);
		}
		cur_node = cur_node->next;

	}
	gtk_widget_set_size_request(dash,-1,-1);

}

void load_gauge(GtkWidget *dash, xmlNode *node)
{
	xmlNode *cur_node = NULL;
	GtkWidget *gauge = NULL;
	gchar * filename = NULL;
	gint width = 0;
	gint height = 0;
	gint x_offset = 0;
	gint y_offset = 0;
	gchar *xml_name = NULL;
	gchar *datasource = NULL;
	if (!node->children)
	{
		printf("ERROR, load_gauge, xml node is empty!!\n");
		return;
	}
	cur_node = node->children;
	while (cur_node->next) { if (cur_node->type == XML_ELEMENT_NODE)
		{
			if (g_strcasecmp((gchar *)cur_node->name,"width") == 0)
				generic_xml_gint_import(cur_node,&width);
			if (g_strcasecmp((gchar *)cur_node->name,"height") == 0)
				generic_xml_gint_import(cur_node,&height);
			if (g_strcasecmp((gchar *)cur_node->name,"x_offset") == 0)
				generic_xml_gint_import(cur_node,&x_offset);
			if (g_strcasecmp((gchar *)cur_node->name,"y_offset") == 0)
				generic_xml_gint_import(cur_node,&y_offset);
			if (g_strcasecmp((gchar *)cur_node->name,"gauge_xml_name") == 0)
				generic_xml_gchar_import(cur_node,&xml_name);
			if (g_strcasecmp((gchar *)cur_node->name,"datasource") == 0)
				generic_xml_gchar_import(cur_node,&datasource);
		}
		cur_node = cur_node->next;

	}
	if (xml_name && datasource)
	{
		gauge = mtx_gauge_face_new();
		gtk_fixed_put(GTK_FIXED(dash),gauge,x_offset,y_offset);
		xml_name = g_strdelimit(xml_name,"\\",'/');
		filename = get_file(g_build_filename(PSEP,GAUGES_DATA_DIR,xml_name,NULL),NULL);
		mtx_gauge_face_import_xml(MTX_GAUGE_FACE(gauge),filename);
		gtk_widget_set_usize(gauge,width,height);
		g_free(filename);
		OBJ_SET_FULL((gauge),"datasource",g_strdup(datasource),g_free);
		/* Cheat to get property window created... */
		create_preview_list(NULL,NULL);
		update_properties(gauge,GAUGE_ADD);
		g_free(xml_name);
		g_free(datasource);
		gtk_widget_show_all(dash);
	}

}


void clear_dashboard(GtkWidget *widget)
{
	GList *children = NULL;
	GtkFixedChild *child = NULL;
	gint i = 0;
	gint len = 0;

	children = g_list_copy(GTK_FIXED(widget)->children);
	len = g_list_length(children);

	for (i=0;i<len;i++)
	{
		child = g_list_nth_data(children,i);
		update_properties(child->widget,GAUGE_REMOVE);
		gtk_widget_destroy(child->widget);
	}
	g_list_free(children);
}


void export_dash_xml(gchar * filename)
{
	GtkWidget *dash = NULL;
	GList *children = NULL;
	GtkFixedChild *child = NULL;
	gchar * tmpbuf = NULL;
	guint i = 0;
	xmlDocPtr doc = NULL;       /* document pointer */
	xmlNodePtr root_node = NULL;/* node pointers */
	xmlNodePtr node = NULL;/* node pointers */
	xmlDtdPtr dtd = NULL;       /* DTD pointer */
	GtkTreeIter parent;
	GtkTreeIter iter;
	GtkTreeModel *model = NULL;
	gboolean state = FALSE;
	gchar * iname = NULL;
	gchar ** vector = NULL;

	g_return_if_fail(filename);

	doc = xmlNewDoc(BAD_CAST "1.0");
	root_node = xmlNewNode(NULL,BAD_CAST "dashboard");
	xmlDocSetRootElement(doc,root_node);
	/*
	 * Creates a DTD declaration. Isn't mandatory. 
	 */
	dtd = xmlCreateIntSubset(doc, BAD_CAST "dashboard", NULL, BAD_CAST "mtxdashboard.dtd");

	dash = GTK_WIDGET(gtk_builder_get_object(toplevel,"dashboard"));

	node = xmlNewChild(root_node,NULL,BAD_CAST "dash_geometry", NULL);
	generic_xml_gint_export(node,"width",&dash->allocation.width);
	generic_xml_gint_export(node,"height",&dash->allocation.height);

	children = GTK_FIXED(dash)->children;
	for(i=0;i<g_list_length(GTK_FIXED(dash)->children);i++)
	{
		child = g_list_nth_data(GTK_FIXED(dash)->children,i);
		node = xmlNewChild(root_node,NULL,BAD_CAST "gauge", NULL);

		generic_xml_gint_export(node,"width",&child->widget->allocation.width);
		generic_xml_gint_export(node,"height",&child->widget->allocation.height);
		generic_xml_gint_export(node,"x_offset",&child->x);
		generic_xml_gint_export(node,"y_offset",&child->y);
		tmpbuf = g_strrstr(mtx_gauge_face_get_xml_filename(MTX_GAUGE_FACE(child->widget)),"Gauges");
		vector = g_strsplit(tmpbuf,PSEP,2);
		
		generic_xml_gchar_export(node,"gauge_xml_name",&vector[1]);
		g_strfreev(vector);
		state = gtk_combo_box_get_active_iter(GTK_COMBO_BOX(OBJ_GET((child->widget),"combo")),&iter);
		model = gtk_combo_box_get_model(GTK_COMBO_BOX(OBJ_GET((child->widget),"combo")));
		gtk_tree_model_get(model,&iter,DATASOURCE_COL,&iname,-1);
		generic_xml_gchar_export(node,"datasource",&iname);
	}
	xmlSaveFormatFileEnc(filename, doc, "utf-8", 1);

	/*free the document */
	xmlFreeDoc(doc);

	/*
	 *Free the global variables that may
	 *have been allocated by the parser.
	 */
	xmlCleanupParser();

	/*
	 * this is to debug memory for regression tests
	 */
	xmlMemoryDump();

	return ;
}

#endif

