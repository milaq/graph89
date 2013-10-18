/*
 * TilEm II
 *
 * Copyright (c) 2011-2012 Benjamin Moody
 * Copyright (c) 2011 Thibault Duponchelle
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

#include "gtk-compat.h"
#include "filedlg.h"
#include "gettext.h"

#ifdef GDK_WINDOWING_WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <commdlg.h>
# include <shlobj.h>
# include <gdk/gdkwin32.h>
# include <wchar.h>

# ifndef OPENFILENAME_SIZE_VERSION_400
#  define OPENFILENAME_SIZE_VERSION_400 sizeof(OPENFILENAMEA)
# endif

struct fcinfo {
	const char *title;
	gboolean save;
	HWND parent_window;
	char *filename;
	char *dirname;
	char *extension;
	const char *filters;
	unsigned int flags;
};

#define BUFFER_SIZE 32768

static char * file_chooser_main(const struct fcinfo *fci)
{
	if (G_WIN32_HAVE_WIDECHAR_API()) {
		OPENFILENAMEW ofnw;
		wchar_t *titlew, *filterw, *initdirw, *defextw;
		wchar_t filenamew[BUFFER_SIZE + 1];
		wchar_t *p;
		int result;
		int i;

		titlew = g_utf8_to_utf16(fci->title, -1, 0, 0, 0);

		filterw = g_utf8_to_utf16(fci->filters, -1, 0, 0, 0);
		for (i = 0; filterw[i]; i++)
			if (filterw[i] == '\n') filterw[i] = 0;

		memset(&ofnw, 0, sizeof(ofnw));
		ofnw.lStructSize = OPENFILENAME_SIZE_VERSION_400;

		ofnw.hwndOwner = fci->parent_window;
		ofnw.lpstrTitle = titlew;
		ofnw.lpstrFilter = filterw;
		ofnw.nFilterIndex = 1;
		ofnw.lpstrFile = filenamew;
		ofnw.nMaxFile = BUFFER_SIZE;

		memset(filenamew, 0, sizeof(filenamew));

		if (fci->filename) {
			p = g_utf8_to_utf16(fci->filename, -1, 0, 0, 0);
			if (p) {
				wcsncpy(filenamew, p, BUFFER_SIZE);
				g_free(p);
			}
		}

		if (fci->dirname)
			initdirw = g_utf8_to_utf16(fci->dirname, -1, 0, 0, 0);
		else
			initdirw = NULL;

		if (fci->extension)
			defextw = g_utf8_to_utf16(fci->extension, -1, 0, 0, 0);
		else
			defextw = NULL;

		ofnw.lpstrInitialDir = initdirw;
		ofnw.lpstrDefExt = defextw;

		ofnw.Flags = fci->flags;

		result = (fci->save
		          ? GetSaveFileNameW(&ofnw)
		          : GetOpenFileNameW(&ofnw));

		g_free(titlew);
		g_free(filterw);
		g_free(initdirw);
		g_free(defextw);

		if (!result)
			return NULL;

		if ((fci->flags & OFN_ALLOWMULTISELECT)) {
			for (i = 0; i < BUFFER_SIZE; i++) {
				if (filenamew[i] == 0 && filenamew[i + 1] == 0)
					break;
				else if (filenamew[i] == '/')
					filenamew[i] = '\\';
				else if (filenamew[i] == 0)
					filenamew[i] = '/';
			}
		}

		return g_utf16_to_utf8(filenamew, -1, 0, 0, 0);
	}
	else {
		OPENFILENAMEA ofna;
		char *titlel, *filterl, *initdirl, *defextl;
		char filenamel[BUFFER_SIZE + 1];
		char *p;
		int result;
		int i;

		titlel = g_locale_from_utf8(fci->title, -1, 0, 0, 0);

		filterl = g_locale_from_utf8(fci->filters, -1, 0, 0, 0);
		for (i = 0; filterl[i]; i++)
			if (filterl[i] == '\n') filterl[i] = 0;

		memset(&ofna, 0, sizeof(ofna));
		ofna.lStructSize = OPENFILENAME_SIZE_VERSION_400;

		ofna.hwndOwner = fci->parent_window;
		ofna.lpstrTitle = titlel;
		ofna.lpstrFilter = filterl;
		ofna.nFilterIndex = 1;
		ofna.lpstrFile = filenamel;
		ofna.nMaxFile = BUFFER_SIZE;

		memset(filenamel, 0, sizeof(filenamel));

		if (fci->filename) {
			p = g_locale_from_utf8(fci->filename, -1, 0, 0, 0);
			if (p) {
				strncpy(filenamel, p, BUFFER_SIZE);
				g_free(p);
			}
		}

		if (fci->dirname)
			initdirl = g_locale_from_utf8(fci->dirname, -1, 0, 0, 0);
		else
			initdirl = NULL;

		if (fci->extension)
			defextl = g_locale_from_utf8(fci->extension, -1, 0, 0, 0);
		else
			defextl = NULL;

		ofna.lpstrInitialDir = initdirl;
		ofna.lpstrDefExt = defextl;

		ofna.Flags = fci->flags;

		result = (fci->save
		          ? GetSaveFileNameA(&ofna)
		          : GetOpenFileNameA(&ofna));

		g_free(titlel);
		g_free(filterl);
		g_free(initdirl);
		g_free(defextl);

		if (!result)
			return NULL;

		if ((fci->flags & OFN_ALLOWMULTISELECT)) {
			for (i = 0; i < BUFFER_SIZE; i++) {
				if (filenamel[i] == 0 && filenamel[i + 1] == 0)
					break;
				else if (filenamel[i] == '/')
					filenamel[i] = '\\';
				else if (filenamel[i] == 0)
					filenamel[i] = '/';
			}
		}

		return g_locale_to_utf8(filenamel, -1, 0, 0, 0);
	}
}

static gboolean wakeup(G_GNUC_UNUSED gpointer data)
{
	gtk_main_quit();
	return FALSE;
}

static gpointer file_chooser_thread(gpointer data)
{
	struct fcinfo *fci = data;
	gpointer res = file_chooser_main(fci);
	g_idle_add(wakeup, NULL);
	return res;
}

static char * build_filter_string(const char *desc1,
                                  const char *pattern1,
                                  va_list ap)
{
	GString *str = g_string_new(NULL);

	while (desc1 && pattern1) {
		if (pattern1[0]) {
			g_string_append(str, desc1);
			g_string_append_c(str, '\n');
			g_string_append(str, pattern1);
			g_string_append_c(str, '\n');
		}

		desc1 = va_arg(ap, char *);
		if (!desc1) break;
		pattern1 = va_arg(ap, char *);
	}

	return g_string_free(str, FALSE);
}

static char ** run_file_chooser1(const char *title,
                                 GtkWindow *parent,
                                 gboolean save,
                                 gboolean multiple,
                                 const char *suggest_name,
                                 const char *suggest_dir,
                                 const char *filters)
{
	struct fcinfo fci;
	GThread *thread;
	GtkWidget *dummy;
	GdkWindow *pwin;
	char *fname, *p, *dir;
	char **result;
	int i;

	if (!g_thread_supported())
		g_thread_init(NULL);

	fci.title = title;
	fci.save = save;

	if (parent && (pwin = gtk_widget_get_window(GTK_WIDGET(parent))))
		fci.parent_window = GDK_WINDOW_HWND(pwin);
	else
		fci.parent_window = NULL;

	if (suggest_name && suggest_dir) {
		fci.filename = g_build_filename(suggest_dir,
		                                suggest_name, NULL);
		fci.dirname = NULL;
	}
	else if (suggest_name) {
		fci.filename = g_strdup(suggest_name);
		fci.dirname = NULL;
	}
	else if (suggest_dir) {
		fci.filename = NULL;
		fci.dirname = g_strdup(suggest_dir);
	}
	else {
		fci.filename = fci.dirname = NULL;
	}

	if (suggest_name && (p = strrchr(suggest_name, '.')))
		fci.extension = g_strdup(p + 1);
	else
		fci.extension = NULL;

	fci.filters = filters;

	fci.flags = (OFN_HIDEREADONLY | OFN_EXPLORER);

	if (save)
		fci.flags |= OFN_OVERWRITEPROMPT;
	else {
		fci.flags |= OFN_FILEMUSTEXIST;
		if (multiple)
			fci.flags |= OFN_ALLOWMULTISELECT;
	}

	if ((thread = g_thread_create(file_chooser_thread, &fci, TRUE, NULL))) {
		dummy = gtk_invisible_new();
		gtk_grab_add(dummy);
		gtk_main();
		fname = g_thread_join(thread);
		gtk_widget_destroy(dummy);
	}
	else {
		fname = file_chooser_main(&fci);
	}

	g_free(fci.filename);
	g_free(fci.dirname);
	g_free(fci.extension);

	if (!fname) {
		return NULL;
	}
	else if (multiple && (p = strchr(fname, '/'))) {
		dir = g_strndup(fname, p - fname);
		result = g_strsplit(p + 1, "/", -1);

		for (i = 0; result[i]; i++) {
			p = result[i];
			result[i] = g_build_filename(dir, p, NULL);
			g_free(p);
		}

		g_free(fname);
		return result;
	}
	else {
		result = g_new(char *, 2);
		result[0] = fname;
		result[1] = NULL;
		return result;
	}
}

static char ** run_file_chooser(const char *title,
                                GtkWindow *parent,
                                gboolean save,
                                gboolean multiple,
                                const char *suggest_name,
                                const char *suggest_dir,
                                const char *desc1,
                                const char *pattern1,
                                va_list ap)
{
	char *filters;
	char **result;
	filters = build_filter_string(desc1, pattern1, ap);
	result = run_file_chooser1(title, parent, save, multiple,
	                           suggest_name, suggest_dir, filters);
	g_free(filters);
	return result;
}

struct dcinfo {
	const char *title;
	HWND parent_window;
	wchar_t *suggest_dir_w;
	char *suggest_dir_l;
};

static int CALLBACK dir_chooser_callback(HWND hwnd, UINT uMsg,
                                         G_GNUC_UNUSED LPARAM lParam,
                                         LPARAM lpData)
{
	const struct dcinfo *dci = (struct dcinfo*) lpData;

	if (uMsg != BFFM_INITIALIZED)
		return 0;

	if (G_WIN32_HAVE_WIDECHAR_API())
		SendMessageW(hwnd, BFFM_SETSELECTIONW,
		             TRUE, (LPARAM) dci->suggest_dir_w);
	else
		SendMessageA(hwnd, BFFM_SETSELECTIONA,
		             TRUE, (LPARAM) dci->suggest_dir_l);
	return 0;
}

static char * dir_chooser_main(const struct dcinfo *dci)
{
	LPITEMIDLIST idl;
	char *result = NULL;

	CoInitialize(NULL);

	if (G_WIN32_HAVE_WIDECHAR_API()) {
		BROWSEINFOW bifw;
		wchar_t dirnamew[MAX_PATH + 1];

		memset(&bifw, 0, sizeof(bifw));
		bifw.hwndOwner = dci->parent_window;
		bifw.lpszTitle = g_utf8_to_utf16(dci->title, -1, 0, 0, 0);
		bifw.ulFlags = (BIF_RETURNONLYFSDIRS | BIF_USENEWUI);
		bifw.lpfn = &dir_chooser_callback;
		bifw.lParam = (LPARAM) dci;

		idl = SHBrowseForFolderW(&bifw);
		if (idl && SHGetPathFromIDListW(idl, dirnamew))
			result = g_utf16_to_utf8(dirnamew, -1, 0, 0, 0);
	}
	else {
		BROWSEINFOA bifa;
		char dirnamel[MAX_PATH + 1];

		memset(&bifa, 0, sizeof(bifa));
		bifa.hwndOwner = dci->parent_window;
		bifa.lpszTitle = g_locale_from_utf8(dci->title, -1, 0, 0, 0);
		bifa.ulFlags = (BIF_RETURNONLYFSDIRS | BIF_USENEWUI);
		bifa.lpfn = &dir_chooser_callback;
		bifa.lParam = (LPARAM) dci;

		idl = SHBrowseForFolderA(&bifa);
		if (idl && SHGetPathFromIDListA(idl, dirnamel))
			result = g_locale_to_utf8(dirnamel, -1, 0, 0, 0);
	}

	if (idl)
		CoTaskMemFree(idl);

	CoUninitialize();

	return result;
}

static gpointer dir_chooser_thread(gpointer data)
{
	struct dcinfo *dci = data;
	gpointer res = dir_chooser_main(dci);
	g_idle_add(wakeup, NULL);
	return res;
}

static char* run_dir_chooser(G_GNUC_UNUSED const char *title,
                             GtkWindow *parent,
                             G_GNUC_UNUSED gboolean save,
                             const char *suggest_dir)
{
	struct dcinfo dci;
	GdkWindow *pwin;
	GThread *thread;
	GtkWidget *dummy;
	char *dname;

	if (!g_thread_supported())
		g_thread_init(NULL);

	dci.title = _("Select a folder to save received files.");

	if (parent && (pwin = gtk_widget_get_window(GTK_WIDGET(parent))))
		dci.parent_window = GDK_WINDOW_HWND(pwin);
	else
		dci.parent_window = NULL;

	if (suggest_dir) {
		dci.suggest_dir_w = g_utf8_to_utf16(suggest_dir, -1, 0, 0, 0);
		dci.suggest_dir_l = g_locale_from_utf8(suggest_dir, -1, 0, 0, 0);
	}
	else {
		dci.suggest_dir_w = NULL;
		dci.suggest_dir_l = NULL;
	}

	if ((thread = g_thread_create(dir_chooser_thread, &dci, TRUE, NULL))) {
		dummy = gtk_invisible_new();
		gtk_grab_add(dummy);
		gtk_main();
		dname = g_thread_join(thread);
		gtk_widget_destroy(dummy);
	}
	else {
		dname = dir_chooser_main(&dci);
	}

	g_free(dci.suggest_dir_w);
	g_free(dci.suggest_dir_l);

	return dname;
}

#else  /* ! GDK_WINDOWING_WIN32 */

/* Case insensitive filter function */
static gboolean filter_lowercase(const GtkFileFilterInfo *info,
                                 gpointer data)
{
	GSList *list = data;
	const char *base;
	char *lowercase, *reversed;
	int length;
	gboolean matched = FALSE;

	if ((base = strrchr(info->filename, G_DIR_SEPARATOR)))
		base++;
	else
		base = info->filename;

	lowercase = g_ascii_strdown(base, -1);
	length = strlen(lowercase);
	reversed = g_memdup(lowercase, length + 1);
	g_strreverse(reversed);

	while (list) {
		if (g_pattern_match(list->data, length,
		                    lowercase, reversed)) {
			matched = TRUE;
			break;
		}
		list = list->next;
	}

	g_free(lowercase);
	g_free(reversed);
	return matched;
}

static void free_filter_info(gpointer data)
{
	GSList *list = data, *l;
	for (l = list; l; l = l->next)
		g_pattern_spec_free(l->data);
	g_slist_free(list);
}

static void setup_file_filters(GtkFileChooser *chooser,
                               const char *desc1,
                               const char *pattern1,
                               va_list ap)
{
	GtkFileFilter *ffilt;
	char **pats;
	GPatternSpec *pspec;
	GSList *pspeclist;
	int i;

	while (desc1 && pattern1) {
		if (pattern1[0]) {
			ffilt = gtk_file_filter_new();
			gtk_file_filter_set_name(ffilt, desc1);

			pats = g_strsplit(pattern1, ";", -1);
			pspeclist = NULL;
			for (i = 0; pats && pats[i]; i++) {
				pspec = g_pattern_spec_new(pats[i]);
				pspeclist = g_slist_prepend(pspeclist, pspec);
			}
			g_strfreev(pats);

			gtk_file_filter_add_custom(ffilt, GTK_FILE_FILTER_FILENAME,
			                           &filter_lowercase,
			                           pspeclist,
			                           &free_filter_info);

			gtk_file_chooser_add_filter(chooser, ffilt);
		}

		desc1 = va_arg(ap, char *);
		if (!desc1) break;
		pattern1 = va_arg(ap, char *);
	}
}

static gboolean prompt_overwrite(const char *fname,
                                 GtkWindow *parent)
{
	GtkWidget *dlg;
	GtkWidget *button;
	char *p, *q;

	if (!g_file_test(fname, G_FILE_TEST_EXISTS))
		return TRUE;

	if (!g_file_test(fname, G_FILE_TEST_IS_REGULAR))
		return FALSE;

	p = g_filename_display_basename(fname);
	dlg = gtk_message_dialog_new(parent,
	                             GTK_DIALOG_MODAL,
	                             GTK_MESSAGE_QUESTION,
	                             GTK_BUTTONS_NONE,
	                             _("A file named \"%s\" already exists.  "
	                               "Do you want to replace it?"),
	                             p);
	g_free(p);

	p = g_path_get_dirname(fname);
	q = g_filename_display_basename(p);
	gtk_message_dialog_format_secondary_markup
		(GTK_MESSAGE_DIALOG(dlg),
		 _("The file already exists in \"%s\".  Replacing it will "
		   "overwrite its contents."), q);
	g_free(p);
	g_free(q);

	gtk_dialog_add_button(GTK_DIALOG(dlg),
	                      GTK_STOCK_CANCEL,
	                      GTK_RESPONSE_CANCEL);

	button = gtk_button_new_with_mnemonic(_("_Replace"));
	gtk_widget_set_can_default(button, TRUE);
	gtk_button_set_image(GTK_BUTTON(button),
	                     gtk_image_new_from_stock(GTK_STOCK_SAVE,
	                                              GTK_ICON_SIZE_BUTTON));
	gtk_widget_show(button);
	gtk_dialog_add_action_widget(GTK_DIALOG(dlg), button,
	                             GTK_RESPONSE_ACCEPT);

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dlg),
	                                        GTK_RESPONSE_ACCEPT,
	                                        GTK_RESPONSE_CANCEL,
	                                        -1);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
		gtk_widget_destroy(dlg);
		return TRUE;
	}

	gtk_widget_destroy(dlg);
	return FALSE;
}

static char ** run_file_chooser(const char *title,
                                GtkWindow *parent,
                                gboolean save,
                                gboolean multiple,
                                const char *suggest_name,
                                const char *suggest_dir,
                                const char *desc1,
                                const char *pattern1,
                                va_list ap)
{
	GtkWidget *filesel;
	GSList *filelist, *l;
	char *fname;
	char **fnames;
	int i, n;

	filesel = gtk_file_chooser_dialog_new(title, parent,
	                                      (save
	                                       ? GTK_FILE_CHOOSER_ACTION_SAVE
	                                       : GTK_FILE_CHOOSER_ACTION_OPEN),
	                                      GTK_STOCK_CANCEL,
	                                      GTK_RESPONSE_CANCEL,
	                                      (save
	                                       ? GTK_STOCK_SAVE
	                                       : GTK_STOCK_OPEN),
	                                      GTK_RESPONSE_ACCEPT,
	                                      NULL);

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(filesel),
	                                        GTK_RESPONSE_ACCEPT,
	                                        GTK_RESPONSE_CANCEL,
	                                        -1);

	gtk_dialog_set_default_response(GTK_DIALOG(filesel),
	                                GTK_RESPONSE_ACCEPT);

	if (suggest_dir)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(filesel),
		                                    suggest_dir);

	if (suggest_name)
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(filesel),
		                                  suggest_name);

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(filesel),
	                                     multiple);

	setup_file_filters(GTK_FILE_CHOOSER(filesel), desc1, pattern1, ap);

	while (gtk_dialog_run(GTK_DIALOG(filesel)) == GTK_RESPONSE_ACCEPT) {
		if (save) {
			fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
			if (!fname || !prompt_overwrite(fname, GTK_WINDOW(filesel))) {
				g_free(fname);
				continue;
			}

			fnames = g_new(char *, 2);
			fnames[0] = fname;
			fnames[1] = NULL;

			gtk_widget_destroy(filesel);
			return fnames;
		}
		else {
			filelist = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(filesel));
			if (!filelist)
				continue;

			n = g_slist_length(filelist);
			fnames = g_new(char *, n + 1);
			i = 0;
			for (l = filelist; l; l = l->next)
				fnames[i++] = l->data;
			g_slist_free(filelist);
			fnames[n] = NULL;

			for (i = 0; i < n; i++)
				if (!g_file_test(fnames[i],
				                 G_FILE_TEST_IS_REGULAR))
					break;
			if (i < n) {
				g_strfreev(fnames);
				continue;
			}

			gtk_widget_destroy(filesel);
			return fnames;
		}
	}

	gtk_widget_destroy(filesel);
	return NULL;
}

static char* run_dir_chooser(const char *title,
                                GtkWindow *parent,
                                gboolean save,
                                const char *suggest_dir)
{
	GtkWidget *filesel;
	char *fname;

	filesel = gtk_file_chooser_dialog_new(title, parent,
					      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
	                                      GTK_STOCK_CANCEL,
	                                      GTK_RESPONSE_CANCEL,
	                                      (save
	                                       ? GTK_STOCK_SAVE
	                                       : GTK_STOCK_OPEN),
	                                      GTK_RESPONSE_ACCEPT,
	                                      NULL);

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(filesel),
	                                        GTK_RESPONSE_ACCEPT,
	                                        GTK_RESPONSE_CANCEL,
	                                        -1);

	gtk_dialog_set_default_response(GTK_DIALOG(filesel),
	                                GTK_RESPONSE_ACCEPT);

	if (suggest_dir)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(filesel),
		                                    suggest_dir);

	while (gtk_dialog_run(GTK_DIALOG(filesel)) == GTK_RESPONSE_ACCEPT) {
		fname = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(filesel));
		if (!fname) {
			g_free(fname);
			continue;
		}

		gtk_widget_destroy(filesel);
		return fname;
	}

	gtk_widget_destroy(filesel);
	return NULL;
}

#endif  /* ! GDK_WINDOWING_WIN32 */

char * prompt_open_file(const char *title,
                        GtkWindow *parent,
                        const char *suggest_dir,
                        const char *desc1,
                        const char *pattern1,
                        ...)
{
	char **result, *fname;
	va_list ap;

	va_start(ap, pattern1);
	result = run_file_chooser(title, parent, FALSE, FALSE,
	                          NULL, suggest_dir,
	                          desc1, pattern1, ap);
	va_end(ap);

	if (!result || !result[0] || result[1]) {
		g_strfreev(result);
		return NULL;
	}
	else {
		fname = result[0];
		g_free(result);
		return fname;
	}
}

char ** prompt_open_files(const char *title,
                          GtkWindow *parent,
                          const char *suggest_dir,
                          const char *desc1,
                          const char *pattern1,
                          ...)
{
	char **result;
	va_list ap;

	va_start(ap, pattern1);
	result = run_file_chooser(title, parent, FALSE, TRUE,
	                          NULL, suggest_dir,
	                          desc1, pattern1, ap);
	va_end(ap);
	return result;
}

char * prompt_save_file(const char *title,
                        GtkWindow *parent,
                        const char *suggest_name,
                        const char *suggest_dir,
                        const char *desc1,
                        const char *pattern1,
                        ...)
{
	char **result, *fname;
	va_list ap;

	va_start(ap, pattern1);
	result = run_file_chooser(title, parent, TRUE, FALSE,
	                          suggest_name, suggest_dir,
	                          desc1, pattern1, ap);
	va_end(ap);

	if (!result || !result[0] || result[1]) {
		g_strfreev(result);
		return NULL;
	}
	else {
		fname = result[0];
		g_free(result);
		return fname;
	}
}

char * prompt_select_dir(const char *title, GtkWindow *parent, const char *suggest_dir)
{
	char *dirname;

	dirname = run_dir_chooser(title, parent, TRUE, suggest_dir);

	if (!dirname) {
		return NULL;
	} else {
		return dirname;
	}
}



/**************** File entry ****************/

#ifdef GDK_WINDOWING_WIN32

typedef struct _FileEntry {
	GtkHBox parent;
	GtkWidget *entry;
	GtkWidget *button;
	char *title;
	char *filters;
	char *filename;
} FileEntry;

typedef struct _FileEntryClass {
	GtkHBoxClass parent;
} FileEntryClass;

static guint selection_changed_signal = 0;

G_DEFINE_TYPE(FileEntry, file_entry, GTK_TYPE_HBOX);

static void file_entry_finalize(GObject *obj)
{
	FileEntry *fe = (FileEntry*) obj;
	g_free(fe->title);
	g_free(fe->filters);
	g_free(fe->filename);
}

void file_entry_set_filename(GtkWidget *entry,
                             const char *filename)
{
	FileEntry *fe = (FileEntry*) entry;

	if (filename && filename[0]) {
		if (!fe->filename || strcmp(filename, fe->filename)) {
			g_free(fe->filename);
			fe->filename = g_strdup(filename);
			gtk_entry_set_text(GTK_ENTRY(fe->entry), filename);
			g_signal_emit(fe, selection_changed_signal, 0, NULL);
		}
	}
	else if (fe->filename) {
		g_free(fe->filename);
		fe->filename = NULL;
		g_signal_emit(fe, selection_changed_signal, 0, NULL);		
	}
}

char * file_entry_get_filename(GtkWidget *entry)
{
	FileEntry *fe = (FileEntry*) entry;
	if (fe->filename)
		return g_strdup(fe->filename);
	else
		return NULL;
}

static void focus_changed(G_GNUC_UNUSED GObject *obj,
                          G_GNUC_UNUSED GParamSpec *pspec,
                          gpointer data)
{
	FileEntry *fe = data;
	const char *text;
	text = gtk_entry_get_text(GTK_ENTRY(fe->entry));
	file_entry_set_filename(GTK_WIDGET(fe), text);
}

static void browse_for_files(G_GNUC_UNUSED GtkButton *btn, gpointer data)
{
	FileEntry *fe = data;
	GtkWidget *parent;
	char **result;
	char *bname, *dname;

	parent = gtk_widget_get_toplevel(GTK_WIDGET(fe));

	if (fe->filename) {
		bname = g_path_get_basename(fe->filename);
		dname = g_path_get_dirname(fe->filename);
	}
	else {
		bname = dname = NULL;
	}

	result = run_file_chooser1(fe->title, GTK_WINDOW(parent), FALSE, FALSE,
	                           bname, dname, fe->filters);
	g_free(bname);
	g_free(dname);

	if (result && result[0])
		file_entry_set_filename(GTK_WIDGET(fe), result[0]);

	g_strfreev(result);
}

static void file_entry_init(FileEntry *fe)
{
	gtk_box_set_spacing(GTK_BOX(fe), 6);

	fe->entry = gtk_entry_new();
	fe->button = gtk_button_new_with_label(_("Browse..."));
	gtk_box_pack_start(GTK_BOX(fe), fe->entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(fe), fe->button, FALSE, FALSE, 0);
	gtk_widget_show(fe->entry);
	gtk_widget_show(fe->button);

	g_signal_connect(fe->entry, "notify::is-focus",
	                 G_CALLBACK(focus_changed), fe);
	g_signal_connect(fe->button, "clicked",
	                 G_CALLBACK(browse_for_files), fe);
}

static void file_entry_class_init(FileEntryClass *class)
{
	GObjectClass *obj_class;

	obj_class = G_OBJECT_CLASS(class);
	obj_class->finalize = file_entry_finalize;

	selection_changed_signal =
		g_signal_new("selection-changed",
		             G_OBJECT_CLASS_TYPE(obj_class),
		             G_SIGNAL_RUN_LAST,
		             0, NULL, NULL,
		             g_cclosure_marshal_VOID__VOID,
		             G_TYPE_NONE, 0);
}

GtkWidget * file_entry_new(const char *title,
                           const char *desc1,
                           const char *pattern1,
                           ...)
{
	FileEntry *fe = g_object_new(file_entry_get_type(), NULL);
	va_list ap;

	fe->title = g_strdup(title);

	va_start(ap, pattern1);
	fe->filters = build_filter_string(desc1, pattern1, ap);
	va_end(ap);

	return GTK_WIDGET(fe);
}


#else /* ! GDK_WINDOWING_WIN32 */

GtkWidget * file_entry_new(const char *title,
                           const char *desc1,
                           const char *pattern1,
                           ...)
{
	GtkWidget *btn;
	va_list ap;

	btn = gtk_file_chooser_button_new(title, GTK_FILE_CHOOSER_ACTION_OPEN);

	va_start(ap, pattern1);
	setup_file_filters(GTK_FILE_CHOOSER(btn), desc1, pattern1, ap);
	va_end(ap);

	return btn;
}

void file_entry_set_filename(GtkWidget *fe,
                             const char *filename)
{
	gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(fe), filename);
}

char * file_entry_get_filename(GtkWidget *fe)
{
	return gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fe));
}

#endif /* ! GDK_WINDOWING_WIN32 */
