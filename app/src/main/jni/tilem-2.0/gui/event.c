/*
 * TilEm II
 *
 * Copyright (c) 2010-2011 Thibault Duponchelle
 * Copyright (c) 2010-2012 Benjamin Moody
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
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <ticalcs.h>
#include <tilem.h>
#include <scancodes.h>

#include "gui.h"
#include "files.h"
#include "filedlg.h"

/* Table for translating skin-file key number (based on actual
   position, and defined by the VTI/TiEmu file formats) into a
   scancode.  Note that the TILEM_KEY_* constants are named according
   to the TI-83 keypad layout; other models use different names for
   the keys, but the same scancodes. */
static const int keycode_map[] =
	{ TILEM_KEY_YEQU,
	  TILEM_KEY_WINDOW,
	  TILEM_KEY_ZOOM,
	  TILEM_KEY_TRACE,
	  TILEM_KEY_GRAPH,

	  TILEM_KEY_2ND,
	  TILEM_KEY_MODE,
	  TILEM_KEY_DEL,
	  TILEM_KEY_LEFT,
	  TILEM_KEY_RIGHT,
	  TILEM_KEY_UP,
	  TILEM_KEY_DOWN,
	  TILEM_KEY_ALPHA,
	  TILEM_KEY_GRAPHVAR,
	  TILEM_KEY_STAT,

	  TILEM_KEY_MATH,
	  TILEM_KEY_MATRIX,
	  TILEM_KEY_PRGM,
	  TILEM_KEY_VARS,
	  TILEM_KEY_CLEAR,

	  TILEM_KEY_RECIP,
	  TILEM_KEY_SIN,
	  TILEM_KEY_COS,
	  TILEM_KEY_TAN,
	  TILEM_KEY_POWER,

	  TILEM_KEY_SQUARE,
	  TILEM_KEY_COMMA,
	  TILEM_KEY_LPAREN,
	  TILEM_KEY_RPAREN,
	  TILEM_KEY_DIV,

	  TILEM_KEY_LOG,
	  TILEM_KEY_7,
	  TILEM_KEY_8,
	  TILEM_KEY_9,
	  TILEM_KEY_MUL,

	  TILEM_KEY_LN,
	  TILEM_KEY_4,
	  TILEM_KEY_5,
	  TILEM_KEY_6,
	  TILEM_KEY_SUB,

	  TILEM_KEY_STORE,
	  TILEM_KEY_1,
	  TILEM_KEY_2,
	  TILEM_KEY_3,
	  TILEM_KEY_ADD,

	  TILEM_KEY_ON,
	  TILEM_KEY_0,
	  TILEM_KEY_DECPNT,
	  TILEM_KEY_CHS,
	  TILEM_KEY_ENTER };

/* Find the keycode for the key (if any) at the given position.  If
   the keys overlap, choose the "nearest" (according to Manhattan
   distance to the midpoint.) */
static int scan_click(const SKIN_INFOS* skin, double x, double y)
{
	guint ix, iy, nearest = 0, i;
	int dx, dy, d, best_d = G_MAXINT;

	if (!skin)
		return 0;

	ix = (x * skin->sx + 0.5);
	iy = (y * skin->sy + 0.5);

	for (i = 0; i < G_N_ELEMENTS(keycode_map); i++) {
		if (ix >= skin->keys_pos[i].left
		    && ix < skin->keys_pos[i].right
		    && iy >= skin->keys_pos[i].top
		    && iy < skin->keys_pos[i].bottom) {
			dx = (skin->keys_pos[i].left + skin->keys_pos[i].right
			      - 2 * ix);
			dy = (skin->keys_pos[i].top + skin->keys_pos[i].bottom
			      - 2 * iy);
			d = ABS(dx) + ABS(dy);

			if (d < best_d) {
				best_d = d;
				nearest = keycode_map[i];
			}
		}
	}

	return nearest;
}

/* Retrieve pointer coordinates for an input device. */
static void get_device_pointer(GdkWindow *win, GdkDevice *dev,
                               gdouble *x, gdouble *y, GdkModifierType *mask)
{
	gdouble *axes;
	int i;

	axes = g_new(gdouble, dev->num_axes);
	gdk_device_get_state(dev, win, axes, mask);

	for (i = 0; i < dev->num_axes; i++) {
		if (x && dev->axes[i].use == GDK_AXIS_X)
			*x = axes[i];
		else if (y && dev->axes[i].use == GDK_AXIS_Y)
			*y = axes[i];
	}

	g_free(axes);
}

/* Show a nice GtkAboutDialog */
void show_about()
{
	GtkWidget *dialog = gtk_about_dialog_new();
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), "2.0"); 
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), _("(c) Benjamin Moody\n(c) Thibault Duponchelle\n(c) Luc Bruant\n"));
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), _("TilEm is a TI Linux Emulator.\n It emulates all current z80 models.\n TI73, TI76, TI81, TI82, TI83(+)(SE), TI84+(SE), TI85 and TI86 ;D"));
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "http://lpg.ticalc.org/prj_tilem/");

	/* Add the logo */	
	char* tilem_ban = get_shared_file_path("pixs", "tilem_ban.png", NULL);
	if(tilem_ban) {
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(tilem_ban, NULL);
		gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), pixbuf);
		g_object_unref(pixbuf), pixbuf = NULL;
	}

	gtk_dialog_run(GTK_DIALOG (dialog));
	gtk_widget_destroy(dialog);

}


void launch_debugger(TilemEmulatorWindow *ewin)
{
	if (!ewin->emu->dbg)
		ewin->emu->dbg = tilem_debugger_new(ewin->emu);
	tilem_debugger_show(ewin->emu->dbg);
}

/* Press a key, ensuring that at most one key is "pressed" at a time
   due to this function (if pointer moves or is released, we don't
   want the old key held down.)

   FIXME: on multi-pointer displays, allow each input device to act
   separately */
static void press_mouse_key(TilemEmulatorWindow* ewin, int key)
{
	if (ewin->mouse_key == key)
		return;

	tilem_calc_emulator_release_key(ewin->emu, ewin->mouse_key);
	tilem_calc_emulator_press_key(ewin->emu, key);
	ewin->mouse_key = key;
}

/* Mouse button pressed */
gboolean mouse_press_event(G_GNUC_UNUSED GtkWidget* w, GdkEventButton *event,
                           gpointer data)
{  	
	TilemEmulatorWindow* ewin = data;
	int key;

	key = scan_click(ewin->skin, event->x, event->y);

	if (event->button == 1) {
		/* button 1: press key until button is released or
		   pointer moves away */
		press_mouse_key(ewin, key);
		return TRUE;
	}
	else if (event->button == 2) {
		/* button 2: hold key down permanently */
		tilem_calc_emulator_press_key(ewin->emu, key);
		return TRUE;
	}
	else if (event->button == 3) {
		/* button 3: popup menu */
		gtk_menu_popup(GTK_MENU(ewin->popup_menu),
		               NULL, NULL, NULL, NULL,
		               event->button, event->time);
		return TRUE;
	}
	else
		return FALSE;
}

/* Mouse pointer moved */
gboolean pointer_motion_event(G_GNUC_UNUSED GtkWidget* w, GdkEventMotion *event,
                              gpointer data)
{
	TilemEmulatorWindow* ewin = data;
	int key;

	if (event->is_hint)
		get_device_pointer(event->window, event->device,
		                   &event->x, &event->y, &event->state);

	if (event->state & GDK_BUTTON1_MASK)
		key = scan_click(ewin->skin, event->x, event->y);
	else
		key = 0;

	press_mouse_key(ewin, key);

	return FALSE;
}

/* Mouse button released */
gboolean mouse_release_event(G_GNUC_UNUSED GtkWidget* w, GdkEventButton *event,
                             gpointer data)
{
	TilemEmulatorWindow* ewin = data;

	if (event->button == 1)
		press_mouse_key(ewin, 0);

	return FALSE;
}

/* Find key binding for the given keysym and modifiers */
static TilemKeyBinding* find_key_binding_for_keysym(TilemCalcEmulator* emu,
                                                    guint keyval,
                                                    GdkModifierType mods)
{
	int i;

	for (i = 0; i < emu->nkeybindings; i++)
		if (keyval == emu->keybindings[i].keysym
		    && mods == emu->keybindings[i].modifiers)
			return &emu->keybindings[i];

	return NULL;
}

/* Find key binding matching the given event */
static TilemKeyBinding* find_key_binding(TilemCalcEmulator* emu,
                                         GdkEventKey* event)
{
	GdkDisplay *dpy;
	GdkKeymap *km;
	guint keyval;
	GdkModifierType consumed, mods;
	TilemKeyBinding *kb;

	dpy = gdk_drawable_get_display(event->window);
	km = gdk_keymap_get_for_display(dpy);

	/* determine the relevant set of modifiers */

	gdk_keymap_translate_keyboard_state(km, event->hardware_keycode,
	                                    event->state, event->group,
	                                    &keyval, NULL, NULL, &consumed);

	mods = (event->state & ~consumed
	        & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK));

	if (event->state & GDK_LOCK_MASK
	    && (kb = find_key_binding_for_keysym(emu, keyval,
	                                         mods | GDK_LOCK_MASK))) {
		return kb;
	}

	return find_key_binding_for_keysym(emu, keyval, mods);
}

/* Key-press event */
gboolean key_press_event(G_GNUC_UNUSED GtkWidget* w, GdkEventKey* event,
                         gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	TilemKeyBinding *kb;
	int i, key;
	unsigned int hwkey;

	/* Ignore repeating keys */
	for (i = 0; i < 64; i++)
		if (ewin->keypress_keycodes[i] == event->hardware_keycode)
			return TRUE;
	if (ewin->sequence_keycode == event->hardware_keycode)
		return TRUE;

	if (!(kb = find_key_binding(ewin->emu, event)))
		return FALSE;

	hwkey = event->hardware_keycode;

	if (kb->nscancodes == 1) {
		/* if queue is empty, just press the key; otherwise
		   add it to the queue */
		key = kb->scancodes[0];
		if (tilem_calc_emulator_press_or_queue(ewin->emu, key))
			ewin->sequence_keycode = hwkey;
		else 
			ewin->keypress_keycodes[key] = hwkey;
	}
	else {
		tilem_calc_emulator_queue_keys(ewin->emu, kb->scancodes,
		                               kb->nscancodes);
		ewin->sequence_keycode = hwkey;
	}

	return TRUE;
}

/* Keyboard key released (keycode 0 = all keys) */
static void release_keycode(TilemEmulatorWindow *ewin, int keycode)
{
	int i;

	/* Check if the key that was just released was one that
	   activated a calculator keypress.  (Do not try to look up
	   event->keyval; modifiers may have changed since the key was
	   pressed.) */
	for (i = 0; i < 64; i++) {
		if (keycode == 0
		    ? ewin->keypress_keycodes[i] != 0
		    : ewin->keypress_keycodes[i] == keycode) {
			tilem_calc_emulator_release_key(ewin->emu, i);
			ewin->keypress_keycodes[i] = 0;
		}
	}

	if (keycode == 0
	    ? ewin->sequence_keycode != 0
	    : ewin->sequence_keycode == keycode) {
		tilem_calc_emulator_release_queued_key(ewin->emu);
		ewin->sequence_keycode = 0;
	}
}

/* Key-release event */
gboolean key_release_event(G_GNUC_UNUSED GtkWidget* w, GdkEventKey* event,
                           gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	release_keycode(ewin, event->hardware_keycode);
	return FALSE;
}

/* Keyboard focus lost */
gboolean focus_out_event(G_GNUC_UNUSED GtkWidget* w,
                         G_GNUC_UNUSED GdkEventFocus *event, gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	release_keycode(ewin, 0);
	return FALSE;
}

static void place_menu(GtkMenu *menu, gint *x, gint *y,
                       gboolean *push_in, gpointer data)
{
	GtkWidget *w = data;
	GdkWindow *win;
	GdkScreen *screen;
	int n;

	win = gtk_widget_get_window(w);
	gdk_window_get_origin(win, x, y);

	screen = gdk_drawable_get_screen(win);
	n = gdk_screen_get_monitor_at_point(screen, *x, *y);
	gtk_menu_set_monitor(menu, n);

	*push_in = FALSE;
}

/* Pop up menu on main window */
gboolean popup_menu_event(GtkWidget* w, gpointer data)
{
	TilemEmulatorWindow *ewin = data;

	gtk_menu_popup(GTK_MENU(ewin->popup_menu),
	               NULL, NULL, &place_menu, w,
	               0, gtk_get_current_event_time());

	return TRUE;
}

/* Callback function for the drag and drop event */
void drag_data_received(G_GNUC_UNUSED GtkWidget *win,
                        G_GNUC_UNUSED GdkDragContext *dc,
                        G_GNUC_UNUSED gint x, G_GNUC_UNUSED gint y,
                        GtkSelectionData *seldata,
                        G_GNUC_UNUSED guint info, G_GNUC_UNUSED guint t,
                        gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	gchar **uris, **filenames;
	gint i, j, n;

	uris = gtk_selection_data_get_uris(seldata);
	if (!uris)
		return;

	n = g_strv_length(uris);
	filenames = g_new0(gchar *, n + 1);

	for (i = j = 0; i < n; i++) {
		filenames[j] = g_filename_from_uri(uris[i], NULL, NULL);
		if (filenames[j])
			j++;
	}
	filenames[j] = NULL;

	load_files(ewin, filenames);
	g_strfreev(filenames);
}
