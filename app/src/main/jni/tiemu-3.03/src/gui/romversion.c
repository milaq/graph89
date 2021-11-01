/* Hey EMACS -*- linux-c -*- */
/* $Id: romversion.c 2753 2007-12-30 23:14:15Z kevinkofler $ */

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
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details. *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */


#ifdef HAVE_CONFIG_H
#  include <tiemuconfig.h>
#endif				/*  */

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h> // unlink

#include "support.h"
#include "intl.h"
#include "paths.h"
#include "ti68k_int.h"
#include "refresh.h"
#include "struct.h"
#include "dboxes.h"
#include "fs_misc.h"
#include "rcfile.h"
#include "tie_error.h"
#include "calc.h"

GtkWidget *dbox;
gchar *chosen_file = NULL;

enum { 
	COL_FILENAME, COL_CALC, COL_VERSION, 
	COL_MEMORY, COL_SIZE, COL_TYPE, COL_HW
};
#define CLIST_NVCOLS	(7)
#define CLIST_NCOLS		(7)

static void clist_selection_changed(GtkTreeSelection * sel,
				   					gpointer user_data)
{ 
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
        if (chosen_file != NULL)
		{
	        g_free(chosen_file);
			chosen_file = NULL;
		}

        gtk_tree_model_get (model, &iter, 0, &chosen_file, -1);
		gtk_dialog_set_response_sensitive(GTK_DIALOG(dbox), GTK_RESPONSE_OK, TRUE);
    }
}

static GtkListStore* clist_create(GtkWidget *clist)
{
	GtkTreeView *view = GTK_TREE_VIEW(clist);	
	GtkListStore *list;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	gint i;
	const gchar *text[CLIST_NCOLS] = { 
		_("Filename"), _("Model"), _("Version"), 
		_("Type"), _("Size"), _("Boot"), _("Hardware") };
	
	list = gtk_list_store_new(CLIST_NCOLS,
    			G_TYPE_STRING, G_TYPE_STRING,
			    G_TYPE_STRING, G_TYPE_STRING,
			    G_TYPE_STRING, G_TYPE_STRING,
                G_TYPE_STRING,
                -1);
    model = GTK_TREE_MODEL(list);
  
    gtk_tree_view_set_model(view, model); 
    gtk_tree_view_set_headers_visible(view, TRUE);
	gtk_tree_view_set_headers_clickable(view, TRUE);
	gtk_tree_view_set_rules_hint(view, FALSE);
  
    for (i = 0; i < CLIST_NCOLS; i++) 
    {
    	renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(view, -1, text[i],
						renderer, "text", i, NULL);
    }
    
    for (i = 0; i < CLIST_NCOLS; i++) 
    {
		GtkTreeViewColumn *col;
		
		col = gtk_tree_view_get_column(view, i);
		gtk_tree_view_column_set_resizable(col, TRUE);
	}
	
	selection = gtk_tree_view_get_selection(view);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	g_signal_connect(G_OBJECT(selection), "changed",
			 G_CALLBACK(clist_selection_changed), NULL);

	return list;
}

static void clist_populate(GtkListStore *store)
{
	gchar *filename;
    FILE *fp;
    GtkTreeIter iter;    
    struct stat s;

	// scan ROM images
    filename = g_strconcat(inst_paths.img_dir, CACHE_FILE, NULL);
    ti68k_scan_images(inst_paths.img_dir, filename);

    stat(filename, &s);
    if(s.st_size == 0) 
		return;

    fp = fopen(filename, "rt");
    if(fp == NULL) 
		return;

    while(!feof(fp)) 
	{
		gchar **row_text;
        char line[256];

        if (!fgets(line, sizeof(line), fp) || feof(fp))
            break;
        line[strlen(line) - 1] = '\0';

        row_text = g_strsplit(line, ",", 7);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 
			0, row_text[0],
			1, row_text[1], 
			2, row_text[2],
			3, row_text[3], 
			4, row_text[4],
			5, row_text[5],
            6, row_text[6],
            -1);

			g_strfreev(row_text);
    } 

    fclose(fp);
}

static void clist_refresh(GtkListStore *store)
{
	gtk_list_store_clear(store);
	clist_populate(store);
}

gint display_romversion_dbox(gboolean file_only)
{
    GladeXML *xml;
	GtkWidget *data;
    gint result;
    GtkListStore *store;
	
	xml = glade_xml_new
		(tilp_paths_build_glade("romversion-2.glade"), "romversion_dbox",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);

    // display list box
	dbox = glade_xml_get_widget(xml, "romversion_dbox");
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dbox), GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_CANCEL,-1);
    
    data = glade_xml_get_widget(xml, "clist1");
    store = clist_create(data);
	clist_populate(store);
    
	// run main box
	gtk_dialog_set_response_sensitive(GTK_DIALOG(dbox), GTK_RESPONSE_OK, FALSE);
	result = gtk_dialog_run(GTK_DIALOG(dbox));
	gtk_widget_destroy(dbox);

	switch (result) 
	{
		case GTK_RESPONSE_OK:
            if(chosen_file == NULL)
                break;

			if(!ti68k_is_a_img_file(chosen_file))
				break;

            // Remove previous tib file
            g_free(params.tib_file);
			params.tib_file = g_strconcat("", NULL);

            // Set new image
			g_free(params.rom_file);
			params.rom_file = g_strconcat(inst_paths.img_dir, chosen_file, NULL);
			g_free(chosen_file);
            chosen_file = NULL;

            if(file_only) return 0;

            // Restart engine by exiting the GTK loop
			g_free(params.sav_file);
			params.sav_file = g_strdup("");

			while(gtk_events_pending()) gtk_main_iteration();
			gtk_main_quit();	
		break;
		
		default:
            if(file_only) return -1;
		break;
	}

	return 0;
}

GLADE_CB void
on_romversion_add1_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkTreeView *view = GTK_TREE_VIEW(button);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkListStore *store = GTK_LIST_STORE(model);

	display_import_romversion_dbox();
	clist_refresh(store);
}

GLADE_CB void
on_romversion_del1_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkTreeView *view = GTK_TREE_VIEW(button);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkListStore *store = GTK_LIST_STORE(model);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(view);
	GtkTreeIter iter;
	gchar *filename;
	gchar *path;

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
		gtk_tree_model_get(model, &iter, COL_FILENAME, &filename, -1);		
		path = g_strconcat(inst_paths.img_dir, filename, NULL);
		
		// delete
		unlink(path);

		g_free(filename);
		g_free(path);
	}

	clist_refresh(store);
}

GLADE_CB gboolean
on_romversion_button_press_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
#if 1
	GtkTreeView *view = GTK_TREE_VIEW(user_data);
    GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkTreeViewColumn *column;
	GtkTreePath *path;
    GtkTreeIter iter;
    gboolean ret;

    if (event->type != GDK_2BUTTON_PRESS)	// double-click ?
		return FALSE;
	else
	{
		// retrieve selection
		gint tx = (gint) event->x;
	    gint ty = (gint) event->y;
	    gint cx, cy;
		
        ret = gtk_tree_view_get_path_at_pos(view, tx, ty, &path, &column, &cx, &cy);
        if(ret == FALSE)
            return FALSE;

		if (!gtk_tree_model_get_iter(model, &iter, path))
		    return FALSE;
        gtk_tree_path_free(path);

		g_free(chosen_file);
        gtk_tree_model_get(model, &iter, COL_FILENAME, &chosen_file, -1);		
	
		gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_OK);
		return TRUE;
    }
#endif
    return FALSE;
}

