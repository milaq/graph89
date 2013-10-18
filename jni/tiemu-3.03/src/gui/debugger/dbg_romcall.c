/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_romcall.c 2591 2007-07-05 11:36:10Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
 *  Copyright (c) 2006, Kevin Kofler
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
#include <string.h>

#include "intl.h"
#include "support.h"
#include "ti68k_int.h"
#include "struct.h"
#include "romcalls.h"
#include "dbg_code.h"
#include "dbg_mem.h"

enum {
		COL_ID, COL_NAME, COL_ADDR, COL_FULL
};
#define CLIST_NVCOLS	(4)		// 3 visible columns
#define CLIST_NCOLS		(4)		// 3 real columns

#define FONT_NAME	"courier"

enum {
	SORT_BY_NAME, SORT_BY_ADDR, SORT_BY_ID,
};
static gint order = SORT_BY_NAME;

GLADE_CB gboolean    
on_combo_entry1_match_selected             (GtkEntryCompletion *widget,
                                            GtkTreeModel *model,
                                            GtkTreeIter *iter,
                                            gpointer user_data);

static GtkProgressBar *pbar = NULL;

static void clist_populate(GtkListStore *store)
{
	GList *lst, *ptr;
	gchar *path;
	gint result;
	gint i, n;
	static int old_order = SORT_BY_NAME;

	// (re)load symbols
	path = g_strconcat(inst_paths.misc_dir, "romcalls.txt", NULL);
	result = ti68k_debug_load_symbols(path);
	g_free(path);

	if((result == -4) && (order == old_order))
		return;				// already loaded
	else if(result == -3)	// no rom calls
		gtk_list_store_clear(store);

	// sort rom calls
	switch(order)
	{
	case SORT_BY_NAME: lst = romcalls_sort_by_iname(); break;
	case SORT_BY_ADDR: lst = romcalls_sort_by_addr(); break;
	case SORT_BY_ID:   lst = romcalls_sort_by_id(); break;
	default:lst = romcalls_sort_by_iname(); break;
	}
	if(lst == NULL)	
		return;

	// show progress bar
	n = g_list_length(lst);
	if(pbar != NULL)
		gtk_widget_show(GTK_WIDGET(pbar));

	// fill storage
	gtk_list_store_clear(store);
	for(ptr = lst, i=0; ptr != NULL; ptr = g_list_next(ptr), i++)
	{
		uint32_t addr = ROMCALL_ADDR(ptr);
		const gchar *name = ROMCALL_NAME(ptr);
		int id = ROMCALL_ID(ptr);
		gchar** row_text = g_malloc0((CLIST_NVCOLS + 1) * sizeof(gchar *));
		GtkTreeIter iter;

		if(!strcmp(name, "unknown") || (name == NULL))
			continue;

		row_text[0] = g_strdup_printf("#%03x", id);
		row_text[1] = g_strdup(name);
		row_text[2] = g_strdup_printf("[$%x]", addr);
		row_text[3] = g_strdup_printf("%s [$%x] - #%03x", name, addr, id);
		//printf("<%s>\n", row_text[3]);

		gtk_list_store_append(store, &iter);
	    gtk_list_store_set(store, &iter, 
	    COL_ID, row_text[0], 
		COL_NAME, row_text[1],
        COL_ADDR, row_text[2],
		COL_FULL, row_text[3],
		-1);

		if(!(i % 50))
		{
			if(pbar != NULL)
			{
				gtk_progress_bar_set_fraction(pbar, (gdouble)i / n);
				while(gtk_events_pending())	gtk_main_iteration();
			}
		}

		g_strfreev(row_text);
	}

	if(pbar != NULL)
		gtk_widget_hide(GTK_WIDGET(pbar));
	old_order = order;
}

static GtkListStore *store = NULL;

void dbgromcall_create_window(GladeXML *xml)
{
	GtkTreeModel *model;
	GtkComboBox *combo;
	GtkEntry *entry;
	GtkEntryCompletion* completion;
	gpointer data;

	pbar = data = glade_xml_get_widget(xml, "progressbar1");
	combo = data = glade_xml_get_widget(xml, "comboboxentry1");
	entry = GTK_ENTRY(GTK_BIN(combo)->child);	

	// create storage
	store = gtk_list_store_new(CLIST_NCOLS,
				G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
				-1
            );
    model = GTK_TREE_MODEL(store);

	// and set storage
	gtk_combo_box_set_model(combo, model);
	gtk_combo_box_entry_set_text_column(GTK_COMBO_BOX_ENTRY(combo), COL_FULL);

	/* --- */

	// set auto-completion
	completion = gtk_entry_completion_new();
	gtk_entry_set_completion(entry, completion);
	gtk_entry_completion_set_model(completion, model);
	gtk_entry_completion_set_text_column (completion, COL_FULL);
	g_signal_connect(G_OBJECT(completion), "match-selected", 
		G_CALLBACK(on_combo_entry1_match_selected), NULL);
	//gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
}

void dbgromcall_refresh_window(void)
{
	clist_populate(store);
}

void dbgromcall_erase_window(GtkWidget *widget)
{
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
	GtkListStore *store = GTK_LIST_STORE(model);
	
	gtk_list_store_clear(store);
}

static void goto_romcall(const char *str)
{
	gchar *ptr;
	uint32_t addr;
	int id;

	ptr = strchr(str, '[');
	if(ptr != NULL)
	{
		int ret = sscanf(ptr, "[$%x] - #%03x ", &addr, &id);
		if(ret == 2)
		{
			if(addr < 0x200000)
				dbgmem_add_tab(addr & 0xffffff);
			else
				dbgcode_disasm_at(addr & 0xffffff);
		}
	}
}

GLADE_CB void
on_combo_entry1_changed                    (GtkComboBox *combobox,
                                            gpointer user_data)
{
	GtkEntry *entry = GTK_ENTRY(GTK_BIN(combobox)->child);

	gchar *str = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
	goto_romcall(str);
	g_free(str);
}

GLADE_CB gboolean    
on_combo_entry1_match_selected             (GtkEntryCompletion *completion,
                                            GtkTreeModel *model,
                                            GtkTreeIter *iter,
                                            gpointer user_data)
{
	gchar *str;

	gtk_tree_model_get(model, iter, COL_FULL, &str, -1);
	goto_romcall(str);
	g_free(str);
	return FALSE;
}


GLADE_CB void
on_dbgcode_radiobutton1_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	// by name
	order = SORT_BY_NAME;
	dbgromcall_refresh_window();
}

GLADE_CB void
on_dbgcode_radiobutton2_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	// by addr
	order = SORT_BY_ADDR;
	dbgromcall_refresh_window();
}

GLADE_CB void
on_dbgcode_radiobutton3_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	// by id
	order = SORT_BY_ID;
	dbgromcall_refresh_window();
}
