/*
 * TilEm II
 *
 * Copyright (c) 2010-2011 Thibault Duponchelle
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
#include <gtk/gtk.h>
#include <ticalcs.h>
#include <tilem.h>

#include "gui.h"

/* Update the progress_bar */
static void progress_bar_update_activity(TilemLinkProgress *linkpb)
{
	char *s;
	gdouble f;

	if (!linkpb->window || !linkpb->emu->pbar_title)
		return;

	gtk_window_set_title(GTK_WINDOW(linkpb->window), linkpb->emu->pbar_title);

	s = g_strdup_printf("<big><b>%s</b></big>", linkpb->emu->pbar_title);
	gtk_label_set_markup(linkpb->title_lbl, s);
	g_free(s);

	if (linkpb->emu->paused && linkpb->emu->pbar_status) {
		s = g_strconcat(linkpb->emu->pbar_status, " ", _("(paused)"), NULL);
		gtk_label_set_text(linkpb->status_lbl, s);
		g_free(s);
	}
	else if (linkpb->emu->paused)
		gtk_label_set_text(linkpb->status_lbl, _("(paused)"));
	else
		gtk_label_set_text(linkpb->status_lbl, linkpb->emu->pbar_status);

	if (linkpb->emu->pbar_progress < 0.0) {
		gtk_progress_bar_pulse(linkpb->progress_bar);
	}
	else {
		f = CLAMP(linkpb->emu->pbar_progress, 0.0, 1.0);
		gtk_progress_bar_set_fraction(linkpb->progress_bar, f);
	}
}

/* Callback to destroy the progress bar */
static void destroy_progress(G_GNUC_UNUSED GtkDialog *dlg,
                             G_GNUC_UNUSED gint response,
                             gpointer data)
{
	TilemLinkProgress* linkpb = data;
	tilem_calc_emulator_cancel_tasks(linkpb->emu);
	gtk_widget_destroy(linkpb->window);
	linkpb->window = NULL;
	linkpb->progress_bar = NULL;
	linkpb->title_lbl = NULL;
	linkpb->status_lbl = NULL;
}

/* Create the progress bar window */
static void progress_bar_init(TilemLinkProgress* linkpb)
{
	GtkWidget *pw, *parent, *vbox, *vbox2, *lbl, *pb;

	if (linkpb->emu->ewin)
		parent = linkpb->emu->ewin->window;
	else
		parent = NULL;

	pw = gtk_dialog_new_with_buttons("", GTK_WINDOW(parent), 0,
	                                 GTK_STOCK_CANCEL,
	                                 GTK_RESPONSE_CANCEL,
	                                 NULL);
	linkpb->window = pw;

	gtk_window_set_resizable(GTK_WINDOW(pw), FALSE);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(pw));

	vbox2 = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2), 6);

	lbl = gtk_label_new(NULL);
	gtk_label_set_width_chars(GTK_LABEL(lbl), 35);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox2), lbl, FALSE, FALSE, 0);
	linkpb->title_lbl = GTK_LABEL(lbl);

	pb = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox2), pb, FALSE, FALSE, 0);
	linkpb->progress_bar = GTK_PROGRESS_BAR(pb);

	lbl = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox2), lbl, FALSE, FALSE, 0);
	linkpb->status_lbl = GTK_LABEL(lbl);

	gtk_box_pack_start(GTK_BOX(vbox), vbox2, FALSE, FALSE, 6);

	g_signal_connect(pw, "response", G_CALLBACK(destroy_progress), linkpb);

	progress_bar_update_activity(linkpb);

	gtk_widget_show_all(pw);
}

/* Create or update the progress bar */
void progress_bar_update(TilemCalcEmulator* emu)
{
	g_return_if_fail(emu != NULL);

	g_mutex_lock(emu->pbar_mutex);

	if (!emu->linkpb) {
		emu->linkpb = g_slice_new0(TilemLinkProgress);
		emu->linkpb->emu = emu;
	}

	if (!emu->linkpb->window && emu->pbar_title) {
		progress_bar_init(emu->linkpb);
	}
	else if (emu->linkpb->window && !emu->pbar_title) {
		gtk_widget_destroy(emu->linkpb->window);
		emu->linkpb->window = NULL;
		emu->linkpb->title_lbl = NULL;
		emu->linkpb->status_lbl = NULL;
		emu->linkpb->progress_bar = NULL;
	}
	else {
		progress_bar_update_activity(emu->linkpb);
	}

	emu->pbar_update_pending = FALSE;
	g_mutex_unlock(emu->pbar_mutex);
}

