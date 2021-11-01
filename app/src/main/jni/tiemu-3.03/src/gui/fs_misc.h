/* Hey EMACS -*- linux-c -*- */
/* $Id$ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005-2006, Romain Liévin, Kevin Kofler
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

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS
gint display_skin_dbox(void);
gint display_load_state_dbox(void);
gint display_save_state_dbox(void);
gint display_recv_files_dbox(const char *src, const char *dst);
gint display_send_files_dbox(void);

gint display_debug_dbox(void);
gint display_set_rom_dbox(void);
gint display_set_tib_dbox(void);
gint display_import_romversion_dbox(void);

void fs_send_file(const gchar *filename);
int  fs_send_files(gchar **filenames);
void fs_send_file_and_debug_info(const gchar *filename);
int  fs_load_state(const char *filename);

G_END_DECLS
