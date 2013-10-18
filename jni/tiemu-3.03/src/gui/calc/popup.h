/* Hey EMACS -*- linux-c -*- */
/* $Id: popup.h 2822 2009-05-04 20:46:56Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
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

#include <gtk/gtk.h>

#include "support.h"

GtkWidget* display_popup_menu(void);

GLADE_CB void
on_popup_menu_cancel                   (GtkMenuShell    *menushell,
                                        gpointer         user_data);

GLADE_CB gboolean
on_popup_menu_button_press_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

GLADE_CB gboolean
on_popup_menu_key_press_event          (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

GLADE_CB void
on_popup_menu_header                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_send_file_to_tiemu1_activate     (GtkMenuItem     *menuitem,
                                     gpointer         user_data);

GLADE_CB void
on_recv_file_from_tiemu1_activate     (GtkMenuItem     *menuitem,
                                       gpointer         user_data);

GLADE_CB void
on_link_cable1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_save_config1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_load_config1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_load_state_image1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_save_state_image1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_revert_to_saved_state1_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_quick_save_state_image1_activate    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_enter_debugger1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_reset_calc1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_set_rom1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_restrict_to_actual_speed1_activate  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_sync1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_number_of_colors1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_2_colors1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_4_colors1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_7_colors1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_blurry1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_normal_view__1x_1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_large_view__x2_1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_full_screen1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_none1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_skin1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_set_skin1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_now__1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_rbm_options1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_help3_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_manpage1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_changelog1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_infos1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_exit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_exit_without_saving_state1_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_now1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
