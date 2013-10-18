/* Hey EMACS -*- linux-c -*- */
/* $Id: rcfile.c 2802 2009-01-03 08:44:48Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005-2007, Romain Liévin, Kevin Kofler
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

/*
  This unit manage configurable options and config file
*/

#ifdef HAVE_CONFIG_H
# include <tiemuconfig.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


#include "intl.h"
#include "tilibs.h"
#include "paths.h"
#include "struct.h"
#include "version.h"
#include "paths.h"
#include "files.h"
#include "ti68k_int.h"
#include "screenshot.h"
#include "logging.h"

static char *rc_file; 

#define MAXCHARS 256

#define DEFAULT_FONT_NAME	"Courier"

/*
  Return the path where the config file is stored
*/
int rcfile_get_path(char **path)
{
	*path = g_strconcat(inst_paths.home_dir, 
			    G_DIR_SEPARATOR_S, CONF_DIR, 
			    G_DIR_SEPARATOR_S, INI_FILE, NULL); 
	return 0;
}

/* 
   Print an error msg 
*/
static void stop (int line)
{
	tiemu_warning(_("Configuration file error at line %i."), line);
}

/*
  Find a token in a string and returns pointer after token
*/
static char *find_str(char *s, const char *t)
{
	char *p = strstr(s, t);

	if(p==NULL) 
		return NULL;
	else 
		return p+strlen(t);
}

/* Chech whether a RC file exists */
int rcfile_exist(void)
{
	int result;

	rcfile_get_path(&rc_file);
  
	result = !access(rc_file, F_OK);
	g_free(rc_file);
	return result;
}

/* Delete the RC file */
int rcfile_delete(void)
{
	int result;

	rcfile_get_path(&rc_file);

	result = unlink(rc_file);
	g_free(rc_file);
	return result;
}

/* Return TiLP version number */
int rcfile_get_version(char *version)
{
	FILE *txt;
	char buffer[256];
	int i = 0;
	char *p;

	strcpy(version, "");
	if (rcfile_exist() == 0) {
		return -1;
	}

	rcfile_get_path(&rc_file);

	txt = fopen(rc_file, "rt");
	g_free(rc_file);
	if (txt == NULL)
		return -1;

	for (i = 0; i < 5; i++)
		if (!fgets(buffer, 256, txt))
		{
			fclose(txt);
			return -1;
		}

	p = strchr(buffer, '=');
	if (p == NULL)
	{
		fclose(txt);
		return -1;
	}

	strcpy(version, ++p);

	p = strchr(version, '\r');
	if (p)
		*p = '\0';

	p = strchr(version, '\n');
	if (p)
		*p = '\0';

	if (fclose(txt))
		return -1;

	return 0;
}

/* 
   Read the RC file and configure the corresponding variables 
*/
void rcfile_read(void)
{
	FILE *txt;
	char buffer[256];
	char *p;
	int l=0;

	rcfile_get_path(&rc_file);

	txt=fopen(rc_file, "rt");
	g_free(rc_file);
	if(txt == NULL) {
		tiemu_info(_("Configuration file not found, use default values. You can create one by the 'File|Save config' command menu."));
		return;
	}

	while(!feof(txt)) {
		if (!fgets(buffer, 256, txt))
			break;
		l++;
		buffer[strlen(buffer)-1]='\0';

	if(!strcmp(buffer, "RC_END")) {
	  fclose(txt);
	  return;
	}

	if(buffer[0]=='#' || !strlen(buffer)) continue;
      
	/* Common part with TiLP: hardware section */
	if ((p = find_str(buffer, "cable_model="))) 
	{
		linkp.cable_model = ticables_string_to_model(p);
		continue;
	}
	
	if ((p = find_str(buffer, "cable_port="))) 
	{
		linkp.cable_port = ticables_string_to_port(p);
		continue;
	}
	
	if ((p = find_str(buffer, "cable_timeout="))) 
	{
		sscanf(p, "%i", &(linkp.cable_timeout));
		continue;
	}
	
	if ((p = find_str(buffer, "cable_delay="))) {
		sscanf(p, "%i", &(linkp.cable_delay));
		continue;
	}

	if( (p=find_str(buffer, "qs_file=")) ) 
	{
	  g_free(options.qs_file);
	  options.qs_file = g_strdup(p);
	  continue;
	}

	if( (p=find_str(buffer, "qs_enabled=")) ) 
	{
	  sscanf(p, "%i", &(options.qs_enabled));
	  continue;
	}

      /* GtkTiEmu specific part: emulator section */
	if( (p=find_str(buffer, "rom_file=")) ) 
	{
	  g_free(params.rom_file);
	  params.rom_file = g_strdup(p);
	  continue;
	}

	if( (p=find_str(buffer, "img_file=")) ) 
	{
	  g_free(params.rom_file);
	  params.rom_file = g_strdup(p);
	  continue;
	}

	if( (p=find_str(buffer, "tib_file=")) )
	{
	  g_free(params.tib_file);
	  params.tib_file = g_strdup(p);
	  continue;
	}

    if( (p=find_str(buffer, "sav_file=")) )
	{
	  g_free(params.sav_file);
	  params.sav_file = g_strdup(p);
	  continue;
	}

	if( (p=find_str(buffer, "skin=")) ) 
	{
	  sscanf(p, "%i", &(options.skin));
	  continue;
	}

	if( (p=find_str(buffer, "view_mode=")) )
	{
	  if(!strcmp(p, "normal")) options.view = VIEW_NORMAL;
	  else if(!strcmp(p, "large")) options.view = VIEW_LARGE;
	  else if(!strcmp(p, "full")) options.view = VIEW_FULL;
	  else if(!strncmp(p, "custom", strlen("custom"))) 
	  {
		  //char *q;
		  options.view = VIEW_CUSTOM;
		  p=find_str(buffer, "view_mode=custom");
		  /*q = strchr(p, ',');
		  if(q) *q = '.';*/
		  sscanf(p, " (%f)", &(options.scale));
		  if(options.scale < 0.01)
			  options.scale = 1.0;
	  }
	  else stop(l);
	  continue;
	}

	if( (p=find_str(buffer, "cpu_rate=")) )
	{
	  sscanf(p, "%u", &(params.cpu_rate));
	  continue;
	}

	if( (p=find_str(buffer, "hw_rate=")) )
	{
	  sscanf(p, "%u", &(params.hw_rate));
	  continue;
	}

	if( (p=find_str(buffer, "lcd_rate=")) )
	{
	  sscanf(p, "%i", &(params.lcd_rate));
	  continue;
	}

	if( (p=find_str(buffer, "hw_protect=")) )
	{
	  sscanf(p, "%i", &(params.hw_protect));
	  continue;
	}

	if( (p=find_str(buffer, "img_format=")) )
	{
	  if(!strcmp(p, "jpg")) options2.format=IMG_JPG;
	  else if(!strcmp(p, "png")) options2.format=IMG_PNG;
	  else if(!strcmp(p, "ico")) options2.format=IMG_ICO;
	  else if(!strcmp(p, "eps")) options2.format=IMG_EPS;
	  else if(!strcmp(p, "pdf")) options2.format=IMG_PDF;
	  else if(!strcmp(p, "bmp")) options2.format=IMG_BMP;
	  else stop(l);
	  continue;
	}
      
	if( (p=find_str(buffer, "img_type=")) )
	{
	  if(!strcmp(p, "bw")) options2.type = IMG_BW;
	  else if(!strcmp(p, "color")) options2.type = IMG_COL;
	  else stop(l);
	  continue;
	}
      
	if( (p=find_str(buffer, "img_size=")) )
	{
	  if(!strcmp(p, "lcd")) options2.size = IMG_LCD;
	  else if(!strcmp(p, "skin")) options2.size = IMG_SKIN;
	  else stop(l);
	  continue;
	}

	if( (p=find_str(buffer, "screen_folder=")) )
	{
	  g_free(options2.folder);
	  options2.folder = g_strdup(p);
	  continue;
	}
      
	if( (p=find_str(buffer, "screen_file=")) )
	{
	  g_free(options2.file);
	  options2.file = g_strdup(p);
	  continue;
	}
      
	if( (p=find_str(buffer, "screen_counter=")) )
	{
	  sscanf(p, "%i", &(options2.counter));
	  continue;
	}

	if( (p=find_str(buffer, "screen_shots=")) )
	{
	  sscanf(p, "%i", &(options2.shots));
	  continue;
	}

	if( (p=find_str(buffer, "screen_skips=")) )
	{
	  sscanf(p, "%i", &(options2.skips));
	  continue;
	}

	if( (p=find_str(buffer, "clipboard=")) )
	{
	  if(!strcmp(p, "no")) options2.clipboard = 0;
	  else if(!strcmp(p, "yes")) options2.clipboard = 1;

	  continue;
	}
	
	if( (p=find_str(buffer, "console=")) )
	{
	  if(!strcmp(p, "no")) options.console = 0;
	  else if(!strcmp(p, "yes")) options.console = 1;
	  else if(!strcmp(p, "boot")) options.console = 2;
	  else stop(l);
	  continue;
	}

	if( (p=find_str(buffer, "kbd_dbg=")) )
	{
	  if(!strcmp(p, "no")) options.kbd_dbg = 0;
	  else if(!strcmp(p, "yes")) options.kbd_dbg = 1;
	}

	if( (p=find_str(buffer, "fs_type=")) )
	{
	  if(!strcmp(p, "old")) options.fs_type = 0;
	  else if(!strcmp(p, "new")) options.fs_type = 1;
	  else if(!strcmp(p, "native") || !strcmp(p, "win32")) options.fs_type = 2;
	  else if(!strcmp(p, "kde")) options.fs_type = 3;
	}
	
	if( (p=find_str(buffer, "skin_file=")) )
	{
	  g_free(options.skin_file);
	  options.skin_file = g_strdup(p);
	  continue;
	}

    if( (p=find_str(buffer, "keys_file=")) )
	{
	  g_free(options.keys_file);
	  options.keys_file = g_strdup(p);
	  continue;
	}

	if( (p=find_str(buffer, "calc_wnd=")) )
	{
	  sscanf(p, "(%i;%i;%i;%i;%i;%i)", 
		  &(options3.calc.rect.x), &(options3.calc.rect.y), 
		  &(options3.calc.rect.w), &(options3.calc.rect.h),
		  &(options3.calc.minimized), &(options3.calc.closed)
		  );
	  continue;
	}

	if( (p=find_str(buffer, "bkpts_wnd=")) )
	{
	  sscanf(p, "(%i;%i;%i;%i;%i;%i)", 
		  &(options3.bkpts.rect.x), &(options3.bkpts.rect.y), 
		  &(options3.bkpts.rect.w), &(options3.bkpts.rect.h),
		  &(options3.bkpts.minimized), &(options3.bkpts.closed)
		  );
	  continue;
	}
	if( (p=find_str(buffer, "code_wnd=")) )
	{
	  sscanf(p, "(%i;%i;%i;%i;%i;%i)", 
		  &(options3.code.rect.x), &(options3.code.rect.y), 
		  &(options3.code.rect.w), &(options3.code.rect.h),
		  &(options3.code.minimized), &(options3.code.closed));
	  continue;
	}
	if( (p=find_str(buffer, "mem_wnd=")) )
	{
	  sscanf(p, "(%i;%i;%i;%i;%i;%i)", 
		  &(options3.mem.rect.x), &(options3.mem.rect.y), 
		  &(options3.mem.rect.w), &(options3.mem.rect.h),
		  &(options3.mem.minimized), &(options3.mem.closed));
	  continue;
	}
	if( (p=find_str(buffer, "regs_wnd=")) )
	{
	  sscanf(p, "(%i;%i;%i;%i;%i;%i)", 
		  &(options3.regs.rect.x), &(options3.regs.rect.y), 
		  &(options3.regs.rect.w), &(options3.regs.rect.h),
		  &(options3.regs.minimized), &(options3.regs.closed));
	  continue;
	}
	if( (p=find_str(buffer, "pclog_wnd=")) )
	{
	  sscanf(p, "(%i;%i;%i;%i;%i;%i)", 
		  &(options3.pclog.rect.x), &(options3.pclog.rect.y), 
		  &(options3.pclog.rect.w), &(options3.pclog.rect.h),
		  &(options3.pclog.minimized), &(options3.pclog.closed));
	  continue;
	}
	if( (p=find_str(buffer, "stack_wnd=")) )
	{
	  sscanf(p, "(%i;%i;%i;%i;%i;%i)", 
		  &(options3.stack.rect.x), &(options3.stack.rect.y), 
		  &(options3.stack.rect.w), &(options3.stack.rect.h),
		  &(options3.stack.minimized), &(options3.stack.closed));
	  continue;
	}
	if( (p=find_str(buffer, "heap_wnd=")) )
	{
	  sscanf(p, "(%i;%i;%i;%i;%i;%i)", 
		  &(options3.heap.rect.x), &(options3.heap.rect.y), 
		  &(options3.heap.rect.w), &(options3.heap.rect.h),
		  &(options3.heap.minimized), &(options3.heap.closed));
	  continue;
	}	
	if( (p=find_str(buffer, "iop_wnd=")) )
	{
	  sscanf(p, "(%i;%i;%i;%i;%i;%i)", 
		  &(options3.iop.rect.x), &(options3.iop.rect.y), 
		  &(options3.iop.rect.w), &(options3.iop.rect.h),
		  &(options3.iop.minimized), &(options3.iop.closed));
	  continue;
	}
	if( (p=find_str(buffer, "dock_wnd=")) )
	{
	  sscanf(p, "(%i;%i;%i;%i;%i;%i)", 
		  &(options3.dock.rect.x), &(options3.dock.rect.y), 
		  &(options3.dock.rect.w), &(options3.dock.rect.h),
		  &(options3.dock.minimized), &(options3.dock.closed));
	  continue;
	}
	if( (p=find_str(buffer, "wnd_transient=")) )
	{
	  if(!strcmp(p, "no")) options3.transient = 0;
	  else if(!strcmp(p, "yes")) options3.transient = 1;
	  else stop(l);
	  continue;
	}

	if( (p=find_str(buffer, "dbg_font_type=")) )
	{
	  if(!strcmp(p, "default")) options3.dbg_font_type = 0;
	  else if(!strcmp(p, "custom")) options3.dbg_font_type = 1;
	  else stop(l);
	  continue;
	}
	if( (p=find_str(buffer, "dbg_font_name=")) )
	{
		g_free(options3.dbg_font_name);
		options3.dbg_font_name = g_strdup(p);
		continue;
	}

	if( (p=find_str(buffer, "dbg_dock=")) )
	{
	  if(!strcmp(p, "no")) options3.dbg_dock = 0;
	  else if(!strcmp(p, "yes")) options3.dbg_dock = 1;
	  else stop(l);
	  continue;
	}

    }
  fclose(txt);

  return;
}

/* 
   Write the contents of some variables to the RC file 
   in a plain text format.
*/
void rcfile_write(void)
{
	FILE *txt;

	rcfile_get_path(&rc_file);
#ifndef __WIN32__
	mkdir(CONF_DIR, 0755);
#else
	_mkdir(CONF_DIR);
#endif
	txt=fopen(rc_file, "wt");
	g_free(rc_file);

	if(txt==NULL) {
		//gif->msg_box(_("Error"), _("Unable to write the config file (~/.tilp or tilp.ini).\n"));
		return;
	}

	fprintf(txt, "# Config file for TiEmu 3\n");
	fprintf(txt, "# (C) Romain Lievin & Thomas Corvazier  2000-2001\n");
	fprintf(txt, "# (C) Romain Lievin 2001-2003\n");
	fprintf(txt, "# (C) Julien Blache 2003\n");
	fprintf(txt, "# (C) Romain Liévin 2004-2005\n");
	fprintf(txt, "# Warning: any comments that you add to this file WILL be overwritten\n");
	fprintf(txt, "\n");

	/* Common part with TiLP */
	fprintf(txt, "version=%s\n", TIEMU_VERSION);
	fprintf(txt, "\n");

	fprintf(txt, "#\n");
	fprintf(txt, "# LINK SECTION\n");
	fprintf(txt, "#\n");
	fprintf(txt, "\n");

	fprintf(txt, "# Link cable type\n");
	fprintf(txt, "cable_model=%s\n", ticables_model_to_string(linkp.cable_model));
	fprintf(txt, "\n");

	fprintf(txt, "# Port to use (serial, parallel, ...\n");
	fprintf(txt, "cable_port=%s\n", ticables_port_to_string(linkp.cable_port));
	fprintf(txt, "\n");

	fprintf(txt, "# Timeout value in 0.1 seconds\n");
	fprintf(txt, "cable_timeout=%i\n", linkp.cable_timeout);
	fprintf(txt, "\n");

	fprintf(txt, "# Delay value\n");
	fprintf(txt, "cable_delay=%i\n", linkp.cable_delay);
	fprintf(txt, "\n");

	fprintf(txt, "# Quick-send feature\n");
	fprintf(txt, "qs_file=%s\n", options.qs_file);
	fprintf(txt, "qs_enabled=%i\n", options.qs_enabled);
	fprintf(txt, "\n");

  /* Specific part to TiEmu */
	fprintf(txt, "#\n");
	fprintf(txt, "# EMULATOR SECTION\n");
	fprintf(txt, "#\n");
	fprintf(txt, "\n");

	fprintf(txt, "# SKIN file\n");
	fprintf(txt, "skin_file=%s\n", options.skin_file);
	fprintf(txt, "\n");

    fprintf(txt, "# KEYS file\n");
	fprintf(txt, "keys_file=%s\n", options.keys_file);
	fprintf(txt, "\n");

	fprintf(txt, "# IMG file\n");
	fprintf(txt, "img_file=%s\n", params.rom_file);
	fprintf(txt, "\n");

    fprintf(txt, "# TIB file\n");
    fprintf(txt, "tib_file=%s\n", params.tib_file);
    fprintf(txt, "\n");

    fprintf(txt, "# SAV file\n");
	fprintf(txt, "sav_file=%s\n", params.sav_file);
    fprintf(txt, "\n");

	fprintf(txt, "# Skin (0 for LCD only, 1 with skin)\n");
	fprintf(txt, "skin=%i\n", options.skin);
	fprintf(txt, "\n");

	fprintf(txt, "# View mode (normal/large/full/custom)\n");
	fprintf(txt, "view_mode=");
	switch(options.view)
    {
    case VIEW_NORMAL: fprintf(txt, "normal\n"); break;
    case VIEW_LARGE: fprintf(txt, "large\n"); break;
    case VIEW_FULL: fprintf(txt, "full\n"); break;
	case VIEW_CUSTOM: fprintf(txt, "custom (%1.2f)\n", options.scale); break;
    }
	fprintf(txt, "\n");

	fprintf(txt, "#\n");
	fprintf(txt, "# HARDWARE SECTION\n");
	fprintf(txt, "#\n");
	fprintf(txt, "\n");

  fprintf(txt, "# CPU cycles rate (360000 per 30ms) = OSC1\n");
  fprintf(txt, "cpu_rate=%i\n", params.cpu_rate);
  fprintf(txt, "\n");

  fprintf(txt, "# Hardware update rate (625 (hw1) or 1172 (hw2) = OSC2/OSC3\n");
  fprintf(txt, "hw_rate=%i\n", params.hw_rate);
  fprintf(txt, "\n");

  fprintf(txt, "# LCD update rate (100 fps max.) = OSC3\n");
  fprintf(txt, "lcd_rate=%i\n", params.lcd_rate);
  fprintf(txt, "\n");

  fprintf(txt, "# HW2 protection\n");
  fprintf(txt, "hw_protect=%i\n", params.hw_protect);
  fprintf(txt, "\n");

  fprintf(txt, "#\n");
  fprintf(txt, "# SCREENSHOT SECTION\n");
  fprintf(txt, "#\n");
  fprintf(txt, "\n");

  fprintf(txt, "# Screenshot: image format (jpg, bmp, ico, eps, pdf, bmp)\n");
  fprintf(txt, "img_format=");
  switch(options2.format)
    {
    case IMG_JPG: fprintf(txt, "jpg\n"); break;
    case IMG_PNG: fprintf(txt, "png\n"); break;
    case IMG_ICO: fprintf(txt, "ico\n"); break;
    case IMG_EPS: fprintf(txt, "eps\n"); break;
    case IMG_PDF: fprintf(txt, "pdf\n"); break;
	case IMG_BMP: fprintf(txt, "bmp\n"); break;
    }
  fprintf(txt, "\n");

  fprintf(txt, "# Screenshot: image type (bw, color)\n");
  fprintf(txt, "img_type=%s\n", (options2.type == IMG_BW) ? "bw" : "color");
  fprintf(txt, "\n");

  fprintf(txt, "# Screenshot: image size (lcd, skin)\n");
  fprintf(txt, "img_size=%s\n", (options2.size == IMG_LCD) ? "lcd" : "skin");
  fprintf(txt, "\n");

  fprintf(txt, "# Screenshot folder\n");
  fprintf(txt, "screen_folder=%s\n", options2.folder);
  fprintf(txt, "\n");

  fprintf(txt, "# Screenshot base file\n");
  fprintf(txt, "screen_file=%s\n", options2.file);
  fprintf(txt, "\n");

  fprintf(txt, "# Screenshot counter\n");
  fprintf(txt, "screen_counter=%i\n", options2.counter);
  fprintf(txt, "\n");

  fprintf(txt, "# Screenshot shots\n");
  fprintf(txt, "screen_shots=%i\n", options2.shots);
  fprintf(txt, "\n");

  fprintf(txt, "# Screenshot skips\n");
  fprintf(txt, "screen_skips=%i\n", options2.skips);
  fprintf(txt, "\n");

  fprintf(txt, "# Screenshot copied to clipboard (no, yes)\n");
  fprintf(txt, "clipboard=");
  switch(options2.clipboard)
  {
  case 0: fprintf(txt, "no\n"); break;
  case 1: fprintf(txt, "yes\n"); break;
  default: fprintf(txt, "no\n"); break;
  }
  fprintf(txt, "\n");

  fprintf(txt, "#\n");
  fprintf(txt, "# MISC SECTION\n");
  fprintf(txt, "#\n");
  fprintf(txt, "\n");

  fprintf(txt, "# Console mode (no, yes, boot)\n");
  fprintf(txt, "console=");
  switch(options.console)
  {
  case 0: fprintf(txt, "no\n"); break;
  case 1: fprintf(txt, "yes\n"); break;
  case 2: fprintf(txt, "boot\n"); break;
  default: fprintf(txt, "no\n"); break;
  }
  fprintf(txt, "\n");

  fprintf(txt, "# Display key bindings (keyboard map files)\n");
  fprintf(txt, "kbd_dbg=%s\n", options.kbd_dbg ? "yes" : "no");
  fprintf(txt, "\n");

  fprintf(txt, "# File selector to use (native or old (GTK1/2) or new (GTK 2.4+) or win32 (Windows) or kde (KDE))\n");
  fprintf(txt, "fs_type=");
  switch(options.fs_type)
  {
  case 0: fprintf(txt, "old\n"); break;
  case 1: fprintf(txt, "new\n"); break;
  case 2: fprintf(txt, "native\n"); break;
  case 3: fprintf(txt, "kde\n"); break;
  default: fprintf(txt, "old\n"); break;
  }
  fprintf(txt, "\n");

  fprintf(txt, "#\n");
  fprintf(txt, "# DEBUGGER SECTION\n");
  fprintf(txt, "#\n");
  fprintf(txt, "\n");

	fprintf(txt, "# Geometry hints of debugger windows (x,y,w,h,m,c)\n");

	fprintf(txt, "calc_wnd=(%i;%i;%i;%i;%i;%i)", 
		options3.calc.rect.x, options3.calc.rect.y, 
		options3.calc.rect.w, options3.calc.rect.h,
		options3.calc.minimized, options3.calc.closed);
	fprintf(txt, "\n");

	fprintf(txt, "bkpts_wnd=(%i;%i;%i;%i;%i;%i)", 
		options3.bkpts.rect.x, options3.bkpts.rect.y, 
		options3.bkpts.rect.w, options3.bkpts.rect.h,
		options3.bkpts.minimized, options3.bkpts.closed);
	fprintf(txt, "\n");

	fprintf(txt, "code_wnd=(%i;%i;%i;%i;%i;%i)", 
		options3.code.rect.x, options3.code.rect.y, 
		options3.code.rect.w, options3.code.rect.h,
		options3.code.minimized, options3.code.closed);
	fprintf(txt, "\n");

	fprintf(txt, "mem_wnd=(%i;%i;%i;%i;%i;%i)", 
		options3.mem.rect.x, options3.mem.rect.y, 
		options3.mem.rect.w, options3.mem.rect.h,
		options3.mem.minimized, options3.mem.closed);
	fprintf(txt, "\n");

	fprintf(txt, "regs_wnd=(%i;%i;%i;%i;%i;%i)", 
		options3.regs.rect.x, options3.regs.rect.y, 
		options3.regs.rect.w, options3.regs.rect.h,
		options3.regs.minimized, options3.regs.closed);
	fprintf(txt, "\n");

	fprintf(txt, "pclog_wnd=(%i;%i;%i;%i;%i;%i)", 
		options3.pclog.rect.x, options3.pclog.rect.y, 
		options3.pclog.rect.w, options3.pclog.rect.h,
		options3.pclog.minimized, options3.pclog.closed);
	fprintf(txt, "\n");

	fprintf(txt, "stack_wnd=(%i;%i;%i;%i;%i;%i)", 
		options3.stack.rect.x, options3.stack.rect.y, 
		options3.stack.rect.w, options3.stack.rect.h,
		options3.stack.minimized, options3.stack.closed);
	fprintf(txt, "\n");

	fprintf(txt, "heap_wnd=(%i;%i;%i;%i;%i;%i)", 
		options3.heap.rect.x, options3.heap.rect.y, 
		options3.heap.rect.w, options3.heap.rect.h,
		options3.heap.minimized, options3.heap.closed);
	fprintf(txt, "\n");

	fprintf(txt, "iop_wnd=(%i;%i;%i;%i;%i;%i)", 
		options3.iop.rect.x, options3.iop.rect.y, 
		options3.iop.rect.w, options3.iop.rect.h,
		options3.iop.minimized, options3.iop.closed);
	fprintf(txt, "\n");

	fprintf(txt, "dock_wnd=(%i;%i;%i;%i;%i;%i)", 
		options3.dock.rect.x, options3.dock.rect.y, 
		options3.dock.rect.w, options3.dock.rect.h,
		options3.dock.minimized, options3.dock.closed);
	fprintf(txt, "\n");

	fprintf(txt, "wnd_transient=%s\n", options3.transient ? "yes" : "no");
 	fprintf(txt, "\n");

	fprintf(txt, "# Font type\n");
	fprintf(txt, "dbg_font_type=");
	switch(options3.dbg_font_type)
	{
	case 0: fprintf(txt, "default\n"); break;
	case 1: fprintf(txt, "custom\n"); break;
	default: fprintf(txt, "default\n"); break;
	}
	if(options3.dbg_font_name)
	{
		fprintf(txt, "dbg_font_name=%s\n", options3.dbg_font_name);
 		fprintf(txt, "\n");
	}

	fprintf(txt, "# Use dock (yes) or multi-window (no)\n");
	fprintf(txt, "dbg_dock=");
	switch(options3.dbg_dock)
	{
	case 0: fprintf(txt, "no\n"); break;
	case 1: fprintf(txt, "yes\n"); break;
	default: fprintf(txt, "no\n"); break;
	}
	fprintf(txt, "\n");

	fprintf(txt, "\n");
  fprintf(txt, "RC_END\n");
  fflush(txt);

  fclose(txt);
}

extern void options3_set_default(void);

/*
  Initialize the options structure with some default values
*/
int rcfile_default()
{
	ti68k_config_load_default();

	// m68k options
	params.rom_file = g_strconcat(inst_paths.img_dir, "", NULL);
	params.sav_file = g_strdup("");
	params.tib_file = g_strdup("");

	// emulator options
	options.skin_file = g_strconcat(inst_paths.skin_dir, "", NULL);
    options.keys_file = g_strconcat(inst_paths.skin_dir, "", NULL);
#if defined(__WIN32__) && defined(_CONSOLE)
	options.console = !0;
#else
	options.console = 0;
#endif
#ifdef __IPAQ__
    options.skin = 0;
#else
    options.skin = 1;
#endif
	options.view = VIEW_NORMAL;
	options.scale = 1.0;
	options.kbd_dbg = 0;
	options.fs_type = 2;

	options.qs_file = g_strdup("");
	options.qs_enabled = 0;

	// screenshot options
	options2.folder = g_strdup(inst_paths.screen_dir);
	options2.file = g_strdup("screenshot");
	options2.counter = 0;
	options2.type = IMG_COL;
	options2.format = IMG_PNG;
	options2.size = IMG_SKIN;
	options2.shots = 1;
	options2.skips = 4;
	options2.clipboard = 0;

	// debugger options
	options3_set_default();	

	return 0;
}

void options3_set_default(void)
{
	// Optimized for 1024x768
	options3.calc.rect.x = 0;
	options3.calc.rect.y = 0;
	options3.calc.rect.w = -1;	// unused yet
	options3.calc.rect.h = -1;	// unused yet
	options3.calc.closed = 0;
	options3.calc.minimized = 0;

	options3.bkpts.rect.x = 700;
	options3.bkpts.rect.y = 560;
	options3.bkpts.rect.w = 320;
	options3.bkpts.rect.h = 145;
	options3.bkpts.closed = !0;
	options3.bkpts.minimized = 0;

	options3.code.rect.x = 510;
	options3.code.rect.y = 10;
	options3.code.rect.w = 320;
	options3.code.rect.h = 290;
	options3.code.closed = 0;
	options3.code.minimized = 0;

	options3.mem.rect.x = 0;
	options3.mem.rect.y = 470;
	options3.mem.rect.w = 520;
	options3.mem.rect.h = 240;
	options3.mem.closed = 0;
	options3.mem.minimized = 0;

	options3.regs.rect.x = 840;
	options3.regs.rect.y = 10;
	options3.regs.rect.w = 180;
	options3.regs.rect.h = 540;
	options3.regs.closed = 0;
	options3.regs.minimized = 0;

	options3.pclog.rect.x = 260;
	options3.pclog.rect.y = 20;
	options3.pclog.rect.w = 160;
	options3.pclog.rect.h = 240;
	options3.pclog.closed = !0;
	options3.pclog.minimized = 0;

    options3.stack.rect.x = 300;
    options3.stack.rect.y = 180;
    options3.stack.rect.w = 160;
    options3.stack.rect.h = 250;
	options3.stack.closed = !0;
	options3.stack.minimized = 0;

	options3.heap.rect.x = 530;
	options3.heap.rect.y = 470;
	options3.heap.rect.w = 160;
	options3.heap.rect.h = 240;
	options3.heap.closed = !0;
	options3.heap.minimized = 0;

	options3.iop.rect.x = 530;
	options3.iop.rect.y = 470;
	options3.iop.rect.w = 360;
	options3.iop.rect.h = 240;
	options3.iop.closed = !0;
	options3.iop.minimized = 0;

	options3.dock.rect.x = 0;
	options3.dock.rect.y = 0;
	options3.dock.rect.w = -1;	// unused yet
	options3.dock.rect.h = -1;	// unused yet
	options3.dock.closed = 0;
	options3.dock.minimized = 0;

	options3.transient = 0;
	
	options3.dbg_font_type = 0;
	options3.dbg_font_name = g_strdup(DEFAULT_FONT_NAME);

	options3.dbg_dock = 1;
}
