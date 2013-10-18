/*
 * TilEm II
 *
 * Copyright (c) 2011 Benjamin Moody
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

/* Set up a tree view with fixed-size columns, where the column sizes
   are determined automatically based on a template.  The template
   model must contain the same number and types of data columns as the
   tree view's data model.  The first row of the template model will
   be used to compute column sizes; any additional rows are
   ignored. */
void fixed_tree_view_init_with_template(GtkWidget *treeview,
                                        GtkTreeModel *template);

/* As above, but the template model is constructed automatically.  The
   tree view must have a data model attached already.

   Arguments following COLGROUPSIZE are a sequence of (column, data)
   pairs, as you would pass to gtk_list_store_set().  The list must be
   terminated with -1.

   If COLGROUPSIZE is a positive integer N, then the template will be
   constructed by repeating the first N columns as many times as
   necessary.  In this case, columns K and K+N must always have the
   same type. */
void fixed_tree_view_init(GtkWidget *treeview, int colgroupsize, ...);
