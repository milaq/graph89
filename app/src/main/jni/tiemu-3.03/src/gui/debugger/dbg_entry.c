/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_entry.c 2753 2007-12-30 23:14:15Z kevinkofler $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin, Kevin Kofler
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
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>

#include "intl.h"
#include "paths.h"
#include "support.h"
#include "ti68k_int.h"
#include "dbg_entry.h"
#include "dbg_bkpts.h"

enum { 
	    COL_NAME, COL_HANDLE, COL_CHECK, COL_VISIBLE,
};
#define CTREE_NVCOLS	(1)		// 1 visible columns
#define CTREE_NCOLS		(4)		// 1 real columns

static GList *sel = NULL;

static void renderer_toggled			   (GtkCellRendererToggle *cell_renderer,
                                            const gchar *path_string,
                                            gpointer user_data)
{
	GtkWidget *tree = user_data;
	GtkTreeView *view = GTK_TREE_VIEW(tree);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkTreeStore *store = GTK_TREE_STORE(model);
	GtkTreePath *path;
	GtkTreeIter iter;
	gboolean check;
	gint handle;

	path = gtk_tree_path_new_from_string(path_string);
	if(!path)
		return;

	if (!gtk_tree_model_get_iter(model, &iter, path))
		return;

	gtk_tree_model_get(model, &iter, COL_CHECK, &check, COL_HANDLE, &handle, -1);	
	check = !check;
	gtk_tree_store_set(store, &iter, COL_CHECK, check, COL_HANDLE, &handle, -1);

	if(check)
		sel = g_list_append(sel, GINT_TO_POINTER((uint32_t)handle));
	else
		sel = g_list_remove(sel, GINT_TO_POINTER((uint32_t)handle));

	gtk_tree_path_free(path);
}

static GtkTreeStore* ctree_create(GtkWidget *widget)
{
	GtkTreeView *view = GTK_TREE_VIEW(widget);
	GtkTreeStore *store;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

    const gchar *text[CTREE_NVCOLS] = { _("Name") };
    gint i;
	
	store = gtk_tree_store_new(CTREE_NCOLS, G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, -1);
    model = GTK_TREE_MODEL(store);
	
    gtk_tree_view_set_model(view, model); 
    gtk_tree_view_set_headers_visible(view, FALSE);
	gtk_tree_view_set_rules_hint(view, FALSE);

	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column(view, column);
	gtk_tree_view_column_set_title(column, text[COL_NAME]);
  
	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_column_pack_start(GTK_TREE_VIEW_COLUMN(column), renderer, FALSE);
	gtk_tree_view_column_set_attributes(GTK_TREE_VIEW_COLUMN(column),
					    renderer, "active", COL_CHECK, "visible", COL_VISIBLE, NULL);
	g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(renderer_toggled), widget);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(GTK_TREE_VIEW_COLUMN(column), renderer, FALSE);	
	gtk_tree_view_column_set_attributes(GTK_TREE_VIEW_COLUMN(column),
					    renderer, "text", COL_NAME, NULL);

   
    for (i = 0; i < CTREE_NVCOLS; i++) 
    {
		GtkTreeViewColumn *col;
		
		col = gtk_tree_view_get_column(view, i);
		gtk_tree_view_column_set_resizable(col, TRUE);
	}
	
	selection = gtk_tree_view_get_selection(view);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	return store;
}

static void ctree_populate(GtkWidget *widget)
{
	GtkTreeView *view = GTK_TREE_VIEW(widget);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkTreeStore *store = GTK_TREE_STORE(model);
	gint i, j;
	GNode *tree;
	uint16_t handle;

	// Parse VAT
	vat_parse(&tree);

	// Retrieve breakpoint
	if(ti68k_bkpt_get_pgmentry(0, &handle))
		handle = -1;

	// and show it
	for (i = 0; i < (int)g_node_n_children(tree); i++) 
	{
		GNode *fol_node = g_node_nth_child(tree, i);
		VatSymEntry *vse = (VatSymEntry *)fol_node->data;
		GtkTreeIter fol_iter;

		gtk_tree_store_append(store, &fol_iter, NULL);
		gtk_tree_store_set(store, &fol_iter, 
			COL_NAME, vse->name, COL_HANDLE, vse->handle, 
			COL_CHECK, FALSE, COL_VISIBLE, FALSE,
			-1);

		for(j = 0; j < (int)g_node_n_children(fol_node); j++)
		{
			GNode *var_node = g_node_nth_child(fol_node, j);
			VatSymEntry *vse = (VatSymEntry *)var_node->data;
			GtkTreeIter var_iter;

			gtk_tree_store_append(store, &var_iter, &fol_iter);
			gtk_tree_store_set(store, &var_iter, 
				COL_NAME, vse->name, COL_HANDLE, vse->handle, 
				COL_CHECK, (vse->handle == handle), COL_VISIBLE, TRUE,
				-1);
		}
	}

	gtk_tree_view_expand_all(view);

	// Free copy of VAT
	vat_free(&tree);
}

static void ctree_get_selection(void)
{
	GList *ptr;

	// clear bkpt list
	ti68k_bkpt_clear_pgmentry();

	// create new one
	for(ptr = sel; ptr != NULL; ptr = g_list_next(ptr))
		ti68k_bkpt_add_pgmentry((uint16_t)(GPOINTER_TO_INT(ptr->data)));

	// free data
	g_list_free(sel);
	sel = NULL;
}

gint dbgentry_display_dbox(void)
{
	GladeXML *xml;
	GtkWidget *dbox;
	GtkWidget *data;
	gint result;
	
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_entry-2.glade"), "dbgentry_dbox",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
	dbox = glade_xml_get_widget(xml, "dbgentry_dbox");
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dbox), GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_CANCEL,-1);
	//gtk_window_resize(GTK_WINDOW(dbox), 320, 240);
		
	data = glade_xml_get_widget(xml, "treeview1");
    ctree_create(data);
	ctree_populate(data);	
	
	result = gtk_dialog_run(GTK_DIALOG(dbox));
	switch (result) 
	{
	case GTK_RESPONSE_OK:
		ctree_get_selection();
		dbgbkpts_refresh_window();
		break;
	default:
		break;
	}

	gtk_widget_destroy(dbox);

	return 0;
}
