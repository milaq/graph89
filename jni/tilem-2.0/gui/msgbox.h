/*
 * Tilem II
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

/* Message box macros */

#define messagebox00(prnt, mtyp, tprm, tsec) \
  do {									\
    GtkWidget* mbx;							\
    mbx = gtk_message_dialog_new(GTK_WINDOW(prnt), GTK_DIALOG_MODAL,	\
				 mtyp, GTK_BUTTONS_OK, tprm);		\
    gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(mbx),	\
					       tsec);			\
    gtk_dialog_set_default_response(GTK_DIALOG(mbx), GTK_RESPONSE_OK);	\
    gtk_dialog_run(GTK_DIALOG(mbx));					\
    gtk_widget_destroy(mbx);						\
  } while(0)

#define messagebox01(prnt, mtyp, tprm, tsec, aaaa) \
  do {									\
    GtkWidget* mbx;							\
    mbx = gtk_message_dialog_new(GTK_WINDOW(prnt), GTK_DIALOG_MODAL,	\
				 mtyp, GTK_BUTTONS_OK, tprm);		\
    gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(mbx),	\
					       tsec, (aaaa));		\
    gtk_dialog_set_default_response(GTK_DIALOG(mbx), GTK_RESPONSE_OK);	\
    gtk_dialog_run(GTK_DIALOG(mbx));					\
    gtk_widget_destroy(mbx);						\
  } while(0)

#define messagebox02(prnt, mtyp, tprm, tsec, aaaa, bbbb) \
  do {									\
    GtkWidget* mbx;							\
    mbx = gtk_message_dialog_new(GTK_WINDOW(prnt), GTK_DIALOG_MODAL,	\
				 mtyp, GTK_BUTTONS_OK, tprm);		\
    gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(mbx),	\
					       tsec, (aaaa), (bbbb));	\
    gtk_dialog_set_default_response(GTK_DIALOG(mbx), GTK_RESPONSE_OK);	\
    gtk_dialog_run(GTK_DIALOG(mbx));					\
    gtk_widget_destroy(mbx);						\
  } while(0)

#define messagebox11(prnt, mtyp, tprm, aaaa, tsec, bbbb) \
  do {									\
    GtkWidget* mbx;							\
    mbx = gtk_message_dialog_new(GTK_WINDOW(prnt), GTK_DIALOG_MODAL,	\
				 mtyp, GTK_BUTTONS_OK, tprm, (aaaa));	\
    gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(mbx),	\
					       tsec, (bbbb));		\
    gtk_dialog_set_default_response(GTK_DIALOG(mbx), GTK_RESPONSE_OK);	\
    gtk_dialog_run(GTK_DIALOG(mbx));					\
    gtk_widget_destroy(mbx);						\
  } while(0)
