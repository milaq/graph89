/* Hey EMACS -*- linux-c -*- */
/* $Id: refresh.c 2349 2007-02-09 11:21:47Z kevinkofler $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
 *  Copyright (c) 2007, Kevin Kofler
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

#include <stdio.h>
#include <gtk/gtk.h>

#include "intl.h"
#include "dboxes.h"
#include "tilibs.h"
#include "refresh.h"
#include "pbars.h"

extern CalcUpdate calc_update;

static void gt_start(void)
{
	calc_update.cnt1 = calc_update.max1 = 0;
	calc_update.cnt2 = calc_update.max2 = 0;
	calc_update.cnt3 = calc_update.max3 = 0;
}

static void gt_stop(void)
{
	calc_update.cnt1 = calc_update.max1 = 0;
	calc_update.cnt2 = calc_update.max2 = 0;
	calc_update.cnt3 = calc_update.max3 = 0;
}

static void filter_shift(void);
static gfloat filter_compute(gfloat input);

static void refresh_pbar1(void)
{
	gchar buffer[32];
	gfloat rate, avg;
  
	if (p_win.pbar1 != NULL) 
	{
		if(calc_update.cnt1 > calc_update.max1)
			calc_update.cnt1 = calc_update.max1;

		if(calc_update.max1 != 0)
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(p_win.pbar1), 
				(gdouble)calc_update.cnt1 / calc_update.max1);

		if(p_win.label_rate != NULL)
		{
			rate = calc_update.rate;
			filter_shift();
			avg = filter_compute(rate);

			g_snprintf(buffer, 32, "Rate: %1.1f Kbytes/s", avg);
			gtk_label_set_text(GTK_LABEL(p_win.label_rate), buffer);
		}
		GTK_REFRESH();
	}
}

static void refresh_pbar2(void)
{
	if (p_win.pbar2 != NULL) 
	{
		if(calc_update.cnt2 > calc_update.max2)
			calc_update.cnt2 = calc_update.max2;

		if(calc_update.max2 != 0)
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(p_win.pbar2), 
				(gdouble)calc_update.cnt2 / calc_update.max2);

		GTK_REFRESH();
	}
}

static void refresh_pbar3(void)
{
	if(p_win.label_part != NULL)
	{
		gchar *str;

		str = g_strdup_printf("%i/%i: ", calc_update.cnt3, calc_update.max3);
		gtk_label_set_text(GTK_LABEL(p_win.label_part), str);

		GTK_REFRESH();
	}
}

static void gt_pbar(void)
{
  refresh_pbar1();
  refresh_pbar2();
  refresh_pbar3();
}

static void gt_label(void)
{
	if (p_win.label == NULL)
		return;

	gtk_label_set_text(GTK_LABEL(p_win.label), calc_update.text);

	GTK_REFRESH();
}

static void gt_refresh(void)
{
	GTK_REFRESH();
}

CalcUpdate calc_update =
{
	"", 0, 0.0, 
	0, 0, 0, 0, 0, 0,
	0, 0,
	gt_start,
	gt_stop,
	gt_refresh,
	gt_pbar,
	gt_label,
};

void tiemu_update_set_gtk(void)
{
	extern CalcHandle *calc_handle;	// defined in dbus.c

	ticalcs_update_set(calc_handle, &calc_update);
}

///// misc

static gfloat filter[8] = { 0 };

static void filter_shift(void)
{
	int i;

	for(i=7; i>0; i--)
		filter[i] = filter[i-1];
}

static gfloat filter_compute(gfloat input)
{
	int i;
	gfloat avg, min, max;
	
	avg = min = max = 0.0;

	filter[0] = input;
	for(i=0; i<7; i++) {
		if(filter[i] < min) min = filter[i];
		if(filter[i] > max) max = filter[i];

		avg += filter[i];
	}

	avg -= min;
	avg -= max;
	
	return (avg / 6);
}





