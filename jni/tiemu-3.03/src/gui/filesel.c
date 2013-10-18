/* Hey EMACS -*- linux-c -*- */
/* $Id: filesel.c 2730 2007-12-16 15:54:33Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005-2007, Romain Liévin, Kevin Kofler
 *
 *  Carbon file dialog code lifted from Systool (LGPL):
 *  Copyright (c) 2006 Asger Ottar Alstrup, Nicolas Cannasse, Edwin van Rijkom
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

/* 
	Some informations about these file selectors: starting at tifiles2-v0.0.6, we
	use the 'glib filename encoding' scheme for charset encoding of filenames:
	- UTF-8 charset on Windows,
	- locale charset on Linux (usually UTF-8 but this is not always true).

	GTK+ always uses UTF-8 for widgets (label, file selectors, ...) thus some conversions
	may be needed.
*/

#ifdef HAVE_CONFIG_H
#  include <tiemuconfig.h>
#endif				/*  */

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>

#ifdef __WIN32__
#include <windows.h>
#include <wchar.h>
#if defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 3)))
	typedef OPENFILENAME OPENFILENAME_MAYALIAS __attribute__((may_alias));
#else
	typedef OPENFILENAME OPENFILENAME_MAYALIAS;
#endif
#endif

#ifdef HAVE_CARBON_CARBON_H
#include <Carbon/Carbon.h>
#endif

#if WITH_KDE
#include "kde.h"
#endif

#include "intl.h"
#include "filesel.h"
#include "refresh.h"
#include "struct.h"

/* In case some header happens to define this... */
#undef HAVE_CARBON_CARBON_H

#ifdef HAVE_CARBON_CARBON_H
/* Helpers */
#define PATH_SIZE 2048

static OSStatus GetFSRefFromAEDesc( FSRef *fsRef, AEDesc* theItem ) {
        OSStatus err = noErr;
        AEDesc coerceDesc= { 0, NULL };
        if ( theItem->descriptorType != typeFSRef )     {
                err = AECoerceDesc( theItem, typeFSRef, &coerceDesc );
                if ( err == noErr )
                theItem = &coerceDesc;
        }
        if ( err == noErr )
        err = AEGetDescData( theItem, fsRef, sizeof(*fsRef) );
        AEDisposeDesc( &coerceDesc );

        if ( err != noErr )     {
                FSSpec fsSpec;
                AEDesc coerceDesc2 = {0, NULL};
                if ( theItem->descriptorType != typeFSS ) {
                        err = AECoerceDesc( theItem, typeFSS, &coerceDesc2 );
                        theItem = &coerceDesc2;
                }
                if ( err == noErr )
                err = AEGetDescData( theItem, &fsSpec, sizeof(fsSpec) );
                AEDisposeDesc( &coerceDesc2 );
                if ( err == noErr )
                err = FSpMakeFSRef( &fsSpec, fsRef );
        }
        return(err);
}

static Boolean filterProc(AEDesc * theItem, void * info, void * callBackUD, NavFilterModes filterMode) {
        const gchar **filters = (const gchar **) callBackUD;
        if (!filters)
                return 1;

        NavFileOrFolderInfo *i = (NavFileOrFolderInfo*) info;
        if (i->isFolder)
                return 1;

        if (theItem->descriptorType==typeFSRef) {
                FSRef f;
                UInt8 path[PATH_SIZE];

                GetFSRefFromAEDesc(&f,theItem);
                if (FSRefMakePath (&f,path,PATH_SIZE)==noErr) {
                        char *ext = NULL;
                        char *next = (char*) path;
                        while(next) {
                                next = strchr(next,'.');
                                if (next)
                                        ext = ++next;
                        }
                        if(ext) {
                                while(*filters) {
                                        const gchar *filter=*(filters++);
                                        if (*(filter++)=='*' && *filter=='.') {
                                                next = ext;
                                                while (*(++filter)) {
                                                        if (!*next) break;
                                                        if (*filter=='?') {
                                                                next++;
                                                        } else if (*filter!=*(next++)) break;
                                                }
                                                if (!*filter && !*next)
                                                        return 1;
                                        }
                                }
                        }
                }
                return 0;
        }
        return 1;
}
#endif

/* Single file selectors */

static gchar *fname = NULL;
static gint action = 0;

// GTK >= 2.4
static const gchar* create_fsel_2(gchar *dirname, gchar *filename, gchar *ext, gboolean save)
{
	GtkWidget *dialog;
	GtkFileFilter *filter;
	gchar *path, *tmp;
	gchar **sarray;
	gint i;
	gchar *sfilename, *sext;

	// gtk_file_chooser_set_current_name and gtk_file_filter_add_pattern ALWAYS want UTF-8.
	sfilename = filename ? g_filename_to_utf8(filename,-1,NULL,NULL,NULL) : NULL;
	sext = ext ? g_filename_to_utf8(ext,-1,NULL,NULL,NULL) : NULL;
    
	// create box
	dialog = gtk_file_chooser_dialog_new (
					  save ? "Save File" : "Open File",
				      NULL,
					  save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      NULL);

	// set default folder
	tmp = g_strconcat(dirname, G_DIR_SEPARATOR_S, NULL);	// add trailing '/' otherwise get_dirname is confused
	path = g_path_get_dirname(tmp);
	g_free(tmp);

	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), path);
	g_free(path);

	// set default name
	if(filename)
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), sfilename);

	// set wildcards
	filter = gtk_file_filter_new();
	sarray = g_strsplit(sext, ";", -1);
	for(i = 0; sarray[i] != NULL; i++)
		gtk_file_filter_add_pattern (filter, sarray[i]);
	g_strfreev(sarray);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

	// get result
	g_free(fname);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
		fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	else
		fname = NULL;
	gtk_widget_destroy (dialog);

	g_free(sfilename);
	g_free(sext);

	return fname;
}

// WIN32
static const gchar* create_fsel_3(gchar *dirname, gchar *filename, gchar *ext, gboolean save)
{
#if defined(__WIN32__)
	OPENFILENAME_MAYALIAS o;
	char lpstrFile[2048] = "\0";
	char lpstrFilter[512];
	char *p;
	gchar **sarray;
	int i, n;
	int have_widechar = G_WIN32_HAVE_WIDECHAR_API();
	void *sdirname;

	// clear structure
	memset (&o, 0, sizeof(OPENFILENAME));

	// set default filename
	if(filename)
	{
		void *temp;
		if (have_widechar)
		{
			temp = g_utf8_to_utf16(filename,-1,NULL,NULL,NULL);
			if(!temp) return NULL;
			wcsncpy((wchar_t *)lpstrFile, temp, sizeof(lpstrFile)>>1);
		}
		else
		{
			temp = g_locale_from_utf8(filename,-1,NULL,NULL,NULL);
			if(!temp) return NULL;
			strncpy(lpstrFile, temp, sizeof(lpstrFile));
		}
		g_free(temp);
	}

	// format filter
	sarray = g_strsplit(ext, "|", -1);
	for(n = 0; sarray[n] != NULL; n++);

	for(i = 0, p = lpstrFilter; i < n; i++)
	{
		void *temp;
		if (have_widechar)
		{
			temp = g_utf8_to_utf16(sarray[i],-1,NULL,NULL,NULL);
			wcscpy((wchar_t *)p,temp);
			p += (wcslen(temp)<<1);
			*p++ = '\0';
			*p++ = '\0';
			wcscpy((wchar_t *)p,temp);
			p += (wcslen(temp)<<1);
			*p++ = '\0';
			*p++ = '\0';
		}
		else
		{
			temp = g_locale_from_utf8(sarray[i],-1,NULL,NULL,NULL);
			strcpy(p,temp);
			p += strlen(temp);
			*p++ = '\0';
			strcpy(p,temp);
			p += strlen(temp);
			*p++ = '\0';
		}
		g_free(temp);
	}
	*p++ = '\0';
	if (have_widechar)
		*p++ = '\0';
	g_strfreev(sarray);

	// set structure
	o.lStructSize = sizeof (o);	
	o.lpstrFilter = lpstrFilter;	//"All\0*.*\0Text\0*.TXT\0";
	o.lpstrFile = lpstrFile;
	if (have_widechar)
	{
		o.nMaxFile = sizeof(lpstrFile) >> 1;
		sdirname = g_utf8_to_utf16(dirname,-1,NULL,NULL,NULL);
	}
	else
	{
		o.nMaxFile = sizeof(lpstrFile);
		sdirname = g_locale_from_utf8(dirname,-1,NULL,NULL,NULL);
	}
	o.lpstrInitialDir = sdirname;
	o.Flags = 0x02000000 | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
				 OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_LONGNAMES | OFN_NONETWORKBUTTON;

	// open/close
	if(save)
	{
		if(!(have_widechar ? GetSaveFileNameW((OPENFILENAMEW *)&o) : GetSaveFileName((OPENFILENAME *)&o)))
		{
			g_free(sdirname);
			return fname = NULL;
		}
	}
	else
	{
		if(!(have_widechar ? GetOpenFileNameW((OPENFILENAMEW *)&o) : GetOpenFileName((OPENFILENAME *)&o)))
		{
			g_free(sdirname);
			return fname = NULL;
		}
	}

	g_free(sdirname);

	if (have_widechar)
		fname = g_utf16_to_utf8((wchar_t *)lpstrFile,-1,NULL,NULL,NULL);
	else
		fname = g_locale_to_utf8(lpstrFile,-1,NULL,NULL,NULL);
	return fname;
#elif defined(HAVE_CARBON_CARBON_H)
        NavDialogRef ref;
        NavDialogCreationOptions opt;
        OSStatus ret;
        gchar **sarray=NULL;

        fname = NULL;
        NavGetDefaultDialogCreationOptions(&opt);
        opt.clientName = CFSTR("TiEmu");
        opt.modality = kWindowModalityAppModal;
        opt.optionFlags = kNavDefaultNavDlogOptions | kNavAllFilesInPopup;

        if (save) {
                ret = NavCreatePutFileDialog(&opt,0,kNavGenericSignature,NULL,NULL,&ref);
        } else {
                sarray = g_strsplit(ext, ";", -1);
                ret = NavCreateChooseFileDialog(&opt,NULL,NULL,NULL,filterProc,sarray,&ref);
        }
        if (ret == noErr) {
                if (NavDialogRun(ref) == noErr) {
                        if (NavDialogGetUserAction(ref) == save?kNavUserActionSaveAs:kNavUserActionChoose) {
                                NavReplyRecord reply;
                                if (NavDialogGetReply(ref,&reply)  == kNavNormalState) {
                                        FSRef fsref;
                                        AEGetNthPtr(&reply.selection,1,typeFSRef,0,0,&fsref,sizeof(FSRef),0);
                                        fname = g_malloc(PATH_SIZE);
                                        memset(fname,0,PATH_SIZE);
                                        if (FSRefMakePath (&fsref,(UInt8*)fname,PATH_SIZE-(save?1:0))==noErr) {
                                                if (save) {
                                                        strcat(fname,"/");
                                                        CFStringGetCString(reply.saveFileName,fname+strlen(fname),PATH_SIZE-strlen(fname),kCFStringEncodingUTF8);
                                                }
                                        } else {
                                                g_free(fname);
                                                fname = NULL;
                                        }
                                        NavDisposeReply(&reply);
                                }
                        }
                }
                NavDialogDispose(ref);
        }
        if (!save) g_strfreev(sarray);
        return fname;
#endif

	return NULL;
}

// KDE
static const gchar* create_fsel_4(gchar *dirname, gchar *filename, gchar *ext, gboolean save)
{
#if WITH_KDE
	gchar *p;
	gchar *extspaces = g_strdup(ext);

	p = extspaces;
	while ((p = strchr(p, ';'))) *p = ' ';

	if(save)
	{
		if (filename)
			dirname = g_strconcat(dirname, "/", filename, NULL);
		fname = sp_kde_get_write_filename(dirname, extspaces, _("Save file"));
	}
	else
		fname = sp_kde_get_open_filename(dirname, extspaces, _("Open file"));

	g_free(extspaces);
	return fname;
#endif

	return NULL;
}

// Front-end
const gchar *create_fsel(gchar *dirname, gchar *filename, gchar *ext, gboolean save)
{
#if !defined(__WIN32__) && !defined(HAVE_CARBON_CARBON_H)
	if(options.fs_type == 2)
	{
#if WITH_KDE
		const char *p = getenv("KDE_FULL_SESSION");
		if (p && *p) // KDE is running
			options.fs_type = 3;
		else
#endif
			options.fs_type = 1;
	}
#endif
#if !WITH_KDE
	if(options.fs_type == 3)
		options.fs_type = 1;
#endif
	//printf("%i: <%s> <%s> <%s> %i\n", options.fs_type, dirname, filename, ext, save);

	switch(options.fs_type)
	{
	case 0:
	case 1:	return create_fsel_2(dirname, filename, ext, save);
	case 2: return create_fsel_3(dirname, filename, ext, save);
	case 3: return create_fsel_4(dirname, filename, ext, save);
	default: return NULL;
	}

	return NULL;
}

/* Multiple files selectors */

static gchar** filenames = NULL;
static gint actions = 0;

// GTK >= 2.4
static gchar** create_fsels_2(gchar *dirname, gchar *filename, gchar *ext)
{
	GtkWidget *dialog;
	GtkFileFilter *filter;
	gchar *path, *tmp;
	gchar **sarray;
	gint i;
	gchar *sfilename, *sext;

	// gtk_file_chooser_set_current_name and gtk_file_filter_add_pattern ALWAYS want UTF-8.
	sfilename = filename ? g_filename_to_utf8(filename,-1,NULL,NULL,NULL) : NULL;
	sext = ext ? g_filename_to_utf8(ext,-1,NULL,NULL,NULL) : NULL;
    
	// create box
	dialog = gtk_file_chooser_dialog_new ("Open File",
				      NULL,
					  GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      NULL);

	// set default folder
	tmp = g_strconcat(dirname, G_DIR_SEPARATOR_S, NULL);	// add trailing '/' otherwise get_dirname is confused
	path = g_path_get_dirname(tmp);
	g_free(tmp);

	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), path);
	g_free(path);

	// set multiple selection
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

	// set default name
	if(filename)
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), sfilename);

	// set wildcards
	filter = gtk_file_filter_new();
	sarray = g_strsplit(sext, ";", -1);
	for(i = 0; sarray[i] != NULL; i++)
		gtk_file_filter_add_pattern (filter, sarray[i]);
	g_strfreev(sarray);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

	// get result
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GSList *list, *p;
		gchar **q;

		// convert list into string array
		list=gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER (dialog));
	      
		filenames = (gchar **)g_malloc0((g_slist_length(list)+1) * 
						sizeof(gchar *));
		for(p = list, q = filenames; p; p = g_slist_next(p), q++)
			*q = p->data;
		*q = NULL;
		     
		g_slist_free(list);
	}
	else
		filenames = NULL;
	gtk_widget_destroy (dialog);

	g_free(sfilename);
	g_free(sext);

	return filenames;
}

// WIN32
static gchar** create_fsels_3(gchar *dirname, gchar *filename, gchar *ext)
{
#if defined(__WIN32__)
	OPENFILENAME_MAYALIAS o;
	char lpstrFile[2048] = "\0";
	char lpstrFilter[512];
	char *p;
	gchar **sarray;
	int i, n;
	int have_widechar = G_WIN32_HAVE_WIDECHAR_API();
	void *sdirname;
	gchar *temp1;

	// clear structure
	memset (&o, 0, sizeof(OPENFILENAME));

	// set default filename
	if(filename)
	{
		void *temp;
		if (have_widechar)
		{
			temp = g_utf8_to_utf16(filename,-1,NULL,NULL,NULL);
			if(!temp) return NULL;
			wcsncpy((wchar_t *)lpstrFile, temp, sizeof(lpstrFile)>>1);
		}
		else
		{
			temp = g_locale_from_utf8(filename,-1,NULL,NULL,NULL);
			if(!temp) return NULL;
			strncpy(lpstrFile, temp, sizeof(lpstrFile));
		}
		g_free(temp);
	}

	// format filter
	sarray = g_strsplit(ext, "|", -1);
	for(n = 0; sarray[n] != NULL; n++);

	for(i = 0, p = lpstrFilter; i < n; i++)
	{
		void *temp;
		if (have_widechar)
		{
			temp = g_utf8_to_utf16(sarray[i],-1,NULL,NULL,NULL);
			wcscpy((wchar_t *)p,temp);
			p += (wcslen(temp)<<1);
			*p++ = '\0';
			*p++ = '\0';
			wcscpy((wchar_t *)p,temp);
			p += (wcslen(temp)<<1);
			*p++ = '\0';
			*p++ = '\0';
		}
		else
		{
			temp = g_locale_from_utf8(sarray[i],-1,NULL,NULL,NULL);
			strcpy(p,temp);
			p += strlen(temp);
			*p++ = '\0';
			strcpy(p,temp);
			p += strlen(temp);
			*p++ = '\0';
		}
		g_free(temp);
	}
	*p++ = '\0';
	if (have_widechar)
		*p++ = '\0';
	g_strfreev(sarray);

	// set structure
	o.lStructSize = sizeof (o);	
	o.lpstrFilter = lpstrFilter;	//"All\0*.*\0Text\0*.TXT\0";
	o.lpstrFile = lpstrFile;		//"C:\msvc\tilp\0foo.txt\0bar.txt"
	if (have_widechar)
	{
		o.nMaxFile = sizeof(lpstrFile) >> 1;
		sdirname = g_utf8_to_utf16(dirname,-1,NULL,NULL,NULL);
	}
	else
	{
		o.nMaxFile = sizeof(lpstrFile);
		sdirname = g_locale_from_utf8(dirname,-1,NULL,NULL,NULL);
	}
	o.lpstrInitialDir = sdirname;
	o.Flags = 0x02000000 | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
				 OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_LONGNAMES | OFN_NONETWORKBUTTON |
				 OFN_ALLOWMULTISELECT;

	// open selector
	if(!(have_widechar ? GetOpenFileNameW((OPENFILENAMEW *)&o) : GetOpenFileName((OPENFILENAME *)&o)))
	{
		g_free(sdirname);
		return NULL;
	}
	filenames = NULL;

	// converts resulting string
	if (have_widechar)
		temp1 = g_utf16_to_utf8((wchar_t *)lpstrFile,-1,NULL,NULL,NULL);
	else
		temp1 = g_locale_to_utf8(lpstrFile,-1,NULL,NULL,NULL);
	for(p = lpstrFile, i=0; *p;
	    p += have_widechar?((wcslen((wchar_t *)p)+1)<<1):(strlen(p)+1), i++)
	{
		if(i)	// skip directory
		{
			gchar *temp;
			filenames = g_realloc(filenames, (i+1) * sizeof(gchar *));
			if (have_widechar)
				temp = g_utf16_to_utf8((wchar_t *)p,-1,NULL,NULL,NULL);
			else
				temp = g_locale_to_utf8(p,-1,NULL,NULL,NULL);
			filenames[i-1] = g_strconcat(temp1, G_DIR_SEPARATOR_S, temp, NULL);
			g_free(temp);
		}
	}
	g_free(temp1);

	// one file selected ?
	if(i == 1)
	{
		filenames = g_malloc(2 * sizeof(gchar *));
		if (have_widechar)
			filenames[0] = g_utf16_to_utf8((wchar_t *)lpstrFile,-1,NULL,NULL,NULL);
		else
			filenames[0] = g_locale_to_utf8(lpstrFile,-1,NULL,NULL,NULL);
		filenames[1] = NULL;
	}
	else
		filenames[i-1] = NULL;

	g_free(sdirname);

	return filenames;
#elif defined(HAVE_CARBON_CARBON_H)
        NavDialogRef ref;
        NavDialogCreationOptions opt;
        gchar **sarray;

        filenames = NULL;
        NavGetDefaultDialogCreationOptions(&opt);
        opt.clientName = CFSTR("TiEmu");
        opt.modality = kWindowModalityAppModal;
        opt.optionFlags = kNavDefaultNavDlogOptions | kNavAllFilesInPopup;

        sarray = g_strsplit(ext, ";", -1);
        if (NavCreateGetFileDialog(&opt,NULL,NULL,NULL,filterProc,sarray,&ref) == noErr) {

                if (NavDialogRun(ref) == noErr) {
                        if (NavDialogGetUserAction(ref)==kNavUserActionOpen) {
                                NavReplyRecord reply;
                                if (NavDialogGetReply(ref,&reply) == kNavNormalState) {
                                        long count;
                                        AECountItems(&reply.selection, &count);
                                        if (count) {
                                                filenames = (gchar **) g_malloc0((count+1)*sizeof(gchar*));
                                                while(count>0) {
                                                        FSRef fsref;
                                                        AEGetNthPtr(&reply.selection,count--,typeFSRef,0,0,&fsref,sizeof(FSRef),0);
                                                        filenames[count] = g_malloc(PATH_SIZE);
                                                        memset(filenames[count],0,PATH_SIZE);
                                                        if (FSRefMakePath (&fsref,(UInt8*)filenames[count],PATH_SIZE)!=noErr) {
                                                               g_strfreev(filenames);
                                                               filenames = NULL;
                                                        }
                                                }
                                        }
                                        NavDisposeReply(&reply);
                                }
                        }
                }
                NavDialogDispose(ref);
        }
        g_strfreev(sarray);
        return filenames;
#endif

	return NULL;
}

static gchar** create_fsels_4(gchar *dirname, gchar *filename, gchar *ext)
{
#if WITH_KDE
	gchar *p;
	gchar *extspaces = g_strdup(ext);
	p = extspaces;
	while ((p = strchr(p, ';'))) *p = ' ';
	filenames = sp_kde_get_open_filenames(dirname, extspaces, _("Open file"));
	g_free(extspaces);
	return filenames;
#endif

	return NULL;
}

// Front-end
gchar** create_fsels(gchar *dirname, gchar *filename, gchar *ext)
{
#if !defined(__WIN32__) && !defined(HAVE_CARBON_CARBON_H)
	if(options.fs_type == 2)
	{
#if WITH_KDE
		const char *p = getenv("KDE_FULL_SESSION");
		if (p && *p) // KDE is running
			options.fs_type = 3;
		else
#endif
			options.fs_type = 1;
	}
#endif
#if !WITH_KDE
	if(options.fs_type == 3)
		options.fs_type = 1;
#endif
	//printf("%i: <%s> <%s> <%s>\n", options.fs_type, dirname, filename, ext);

	switch(options.fs_type)
	{
	case 0:
	case 1:	return create_fsels_2(dirname, filename, ext);
	case 2: return create_fsels_3(dirname, filename, ext);
	case 3: return create_fsels_4(dirname, filename, ext);
	default: return NULL;
	}

	return NULL;
}


