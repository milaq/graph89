/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_vectors.c 2753 2007-12-30 23:14:15Z kevinkofler $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
 *  Copyright (c) 2007, Kevin Kofler
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <tiemuconfig.h>
#endif				/*  */

#include <gtk/gtk.h>
#include <glade/glade.h>

#include "intl.h"
#include "paths.h"
#include "ti68k_int.h"
#include "dbg_bkpts.h"

enum { 
	    COL_NUMBER, COL_NAME,
};
#define CLIST_NVCOLS	(2)		// 2 visible columns
#define CLIST_NCOLS		(2)		// 2 real columns

static GtkListStore* clist_create(GtkWidget *widget)
{
	GtkTreeView *view = GTK_TREE_VIEW(widget);
	GtkListStore *store;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
    const gchar *text[CLIST_NVCOLS] = { _("Number"), _("Name") };
    gint i;
	
	store = gtk_list_store_new(CLIST_NCOLS,
				G_TYPE_INT, G_TYPE_STRING,
				-1
            );
    model = GTK_TREE_MODEL(store);
	
    gtk_tree_view_set_model(view, model); 
    gtk_tree_view_set_headers_visible(view, TRUE);
	gtk_tree_view_set_rules_hint(view, FALSE);
  
	for(i = COL_NUMBER; i <= COL_NAME; i++)
	{
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_insert_column_with_attributes(view, -1, 
            text[i], renderer, 
            "text", i,
			NULL);
	}
    
    for (i = 0; i < CLIST_NVCOLS; i++) 
    {
		GtkTreeViewColumn *col;
		
		col = gtk_tree_view_get_column(view, i);
		gtk_tree_view_column_set_resizable(col, TRUE);
	}
	
	selection = gtk_tree_view_get_selection(view);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

	return store;
}

static void clist_populate(GtkWidget *widget)
{
	GtkTreeView *view = GTK_TREE_VIEW(widget);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkListStore *store = GTK_LIST_STORE(model);
    GtkTreeIter iter;
	gint i;

	for(i = 0; i < 128; i++)
	{
	    gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
		COL_NUMBER, i, 
		COL_NAME, ti68k_exception_to_string(i),
		-1);
	}
}

static void clist_get_selection(GtkWidget *list)
{
	GtkTreeView *view = GTK_TREE_VIEW(list);
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *l;
	
	// get selection
	selection = gtk_tree_view_get_selection(view);
	for (l = gtk_tree_selection_get_selected_rows(selection, &model);
	     l != NULL; l = l->next) 
	{
		GtkTreeIter iter;
		GtkTreePath *path = l->data;
		gint n;
			
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, COL_NUMBER, &n, -1);
		
		ti68k_bkpt_add_exception(n);
	}

	// free selection
	g_list_foreach (l, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (l);
}

gint dbgvectors_display_dbox(void)
{
	GladeXML *xml;
	GtkWidget *dbox;
	GtkWidget *data;
	gint result;
	
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_vectors-2.glade"), "dbgvectors_dbox",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
	dbox = glade_xml_get_widget(xml, "dbgvectors_dbox");
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dbox), GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_CANCEL,-1);
	gtk_window_resize(GTK_WINDOW(dbox), 320, 240);
		
	data = glade_xml_get_widget(xml, "treeview1");
    clist_create(data);
	clist_populate(data);	
	
	result = gtk_dialog_run(GTK_DIALOG(dbox));
	switch (result) 
	{
	case GTK_RESPONSE_OK:
		clist_get_selection(data);
		dbgbkpts_refresh_window();
		break;
	default:
		break;
	}

	gtk_widget_destroy(dbox);

	return 0;
}




