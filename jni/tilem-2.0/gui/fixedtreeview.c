/*
 * TilEm II
 *
 * Copyright (c) 2011-2012 Benjamin Moody
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gobject/gvaluecollector.h>

#include "fixedtreeview.h"
#include "gettext.h"

/* Style set on tree view; update column sizes */
static void ftv_style_set(GtkWidget *treeview,
                          G_GNUC_UNUSED GtkStyle *oldstyle,
                          G_GNUC_UNUSED gpointer data)
{
	GtkTreeModel *template;
	GtkTreeIter iter;
	GList *cols, *cp;
	GtkTreeViewColumn *col;
	int width;

	template = g_object_get_data(G_OBJECT(treeview), "ftv-template");
	if (!template)
		return;

	if (!gtk_tree_model_get_iter_first(template, &iter))
		return;

	cols = gtk_tree_view_get_columns(GTK_TREE_VIEW(treeview));
	for (cp = cols; cp; cp = cp->next) {
		col = cp->data;
		gtk_tree_view_column_cell_set_cell_data(col, template, &iter,
		                                        FALSE, FALSE);
		gtk_tree_view_column_cell_get_size(col, NULL, NULL, NULL,
		                                   &width, NULL);
		gtk_tree_view_column_set_fixed_width(col, width + 2);
	}
	g_list_free(cols);
}

/* Widget destroyed */
static void ftv_destroy(GtkWidget *treeview, G_GNUC_UNUSED gpointer data)
{
	GtkTreeModel *template;

	template = g_object_get_data(G_OBJECT(treeview), "ftv-template");
	if (template)
		g_object_unref(template);
	g_object_set_data(G_OBJECT(treeview), "ftv-template", NULL);
}

void fixed_tree_view_init_with_template(GtkWidget *treeview,
                                        GtkTreeModel *template)
{
	GtkTreeModel *oldtemplate;

	if (template)
		g_object_ref_sink(template);

	oldtemplate = g_object_get_data(G_OBJECT(treeview), "ftv-template");
	if (oldtemplate) {
		g_object_unref(oldtemplate);
	}
	else {
		g_signal_connect(treeview, "style-set",
		                 G_CALLBACK(ftv_style_set), NULL);
		g_signal_connect(treeview, "destroy",
		                 G_CALLBACK(ftv_destroy), NULL);
	}
	g_object_set_data(G_OBJECT(treeview), "ftv-template", template);

	if (template && GTK_WIDGET_REALIZED(treeview))
		ftv_style_set(treeview, NULL, NULL);
}

void fixed_tree_view_init(GtkWidget *treeview, int colgroupsize, ...)
{
	GtkTreeModel *real_model;
	int ncols, i, col;
	GType *types;
	GtkListStore *store;
	GtkTreeIter iter;
	GValue value;
	gchar *error = NULL;
	va_list ap;

	g_return_if_fail(GTK_IS_TREE_VIEW(treeview));
	g_return_if_fail(colgroupsize >= 0);

	real_model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	g_return_if_fail(real_model != NULL);

	ncols = gtk_tree_model_get_n_columns(real_model);
	g_return_if_fail(ncols > 0);

	if (colgroupsize == 0)
		colgroupsize = ncols;

	g_return_if_fail(ncols % colgroupsize == 0);

	types = g_new(GType, ncols);
	for (i = 0; i < ncols; i++) {
		types[i] = gtk_tree_model_get_column_type(real_model, i);
		if (i > colgroupsize)
			g_return_if_fail(types[i] == types[i - colgroupsize]);
	}
	store = gtk_list_store_newv(ncols, types);

	va_start(ap, colgroupsize);
	gtk_list_store_append(store, &iter);

	memset(&value, 0, sizeof(value));

	col = va_arg(ap, int);
	while (col != -1) {
		if (col < 0 || col >= colgroupsize) {
			g_critical(_("missing sentinel"));
			break;
		}

		g_value_init(&value, types[col]);

		G_VALUE_COLLECT(&value, ap, 0, &error);

		if (error) {
			g_critical("%s", error);
			g_free(error);
			break;
		}

		for (i = col; i < ncols; i += colgroupsize)
			gtk_list_store_set_value(store, &iter, i, &value);

		g_value_unset(&value);

		col = va_arg(ap, int);
	}

	va_end(ap);

	g_free(types);

	fixed_tree_view_init_with_template(treeview, GTK_TREE_MODEL(store));
}
