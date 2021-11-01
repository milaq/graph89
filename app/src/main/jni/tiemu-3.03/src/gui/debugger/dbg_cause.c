/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_cause.c 2713 2007-12-14 21:03:03Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
 *  Copyright (c) 2007, Kevin Kofler, Romain Liévin
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
#endif				/*  */

#include <gtk/gtk.h>
#include <glade/glade.h>

#include "intl.h"
#include "paths.h"
#include "ti68k_int.h"
#include "dbg_code.h"

gint dbgcause1_display_dbox()
{
	GladeXML *xml;
	GtkWidget *dbox;
	GtkWidget *label;
	gint result;
	gchar *str;
	gint type, id, mode;
	uint32_t value, min, max;

    // get context
	ti68k_bkpt_get_cause(&type, &mode, &id);
    if(!type && !mode)
        return 0;
	
    // load GUI
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_cause-2.glade"), "dbgcause1_dbox",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
	dbox = glade_xml_get_widget(xml, "dbgcause1_dbox");

	// set PC
	label = glade_xml_get_widget(xml, "label21");
	ti68k_register_get_pc(&value);
	str = g_strdup_printf("0x%06x", value);
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);

	// set type
	label = glade_xml_get_widget(xml, "label22");
	str = g_strdup_printf("%s", ti68k_bkpt_type_to_string(type));
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);

	// set mode
	label = glade_xml_get_widget(xml, "label23");
	str = g_strdup_printf("%s", ti68k_bkpt_mode_to_string(type, mode));
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);

	// set id
	label = glade_xml_get_widget(xml, "label24");
	str = g_strdup_printf("%i", id);
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);

	// set target
	label = glade_xml_get_widget(xml, "label25");
	switch(type)
	{
	case BK_TYPE_ACCESS:
		ti68k_bkpt_get_access(id, &min, mode);
		str = g_strdup_printf("0x%06x", min);
		break;
	case BK_TYPE_RANGE:
		ti68k_bkpt_get_range(id, &min, &max, mode);
		str = g_strdup_printf("0x%06x-0x%06x", min, max);
		break;
	default:
		str = g_strdup("n/a");
		break;
	}
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);
	
	result = gtk_dialog_run(GTK_DIALOG(dbox));
	switch (result) {
	case GTK_RESPONSE_OK:
		break;
	default:
		break;
	}

	gtk_widget_destroy(dbox);

	return 0;
}

gint dbgcause2_display_dbox()
{
	GladeXML *xml;
	GtkWidget *dbox;
	GtkWidget *label;
	gint result;
	gchar *str;
	gint type, id, mode;
	uint32_t sp;
	uint32_t pc;
	uint32_t sr;

    // get context
	ti68k_bkpt_get_cause(&type, &mode, &id);
    if(!type && !mode && !id)
        return 0;
	
    // load GUI
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_cause-2.glade"), "dbgcause2_dbox",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
	dbox = glade_xml_get_widget(xml, "dbgcause2_dbox");

	// set exception type
	label = glade_xml_get_widget(xml, "label41");
	gtk_label_set_text(GTK_LABEL(label), ti68k_exception_to_string(mode));

	// set id
	label = glade_xml_get_widget(xml, "label42");
	str = g_strdup_printf("%i", id);
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);

	// set pushed PC
	ti68k_register_get_sp(&sp);
	sr = mem_rd_long(sp);
	str = g_strdup_printf("%04x", sr);
	label = glade_xml_get_widget(xml, "label43");
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);

	// set pushed SR
	ti68k_register_get_sp(&sp);
	pc = mem_rd_long(sp+2);
	str = g_strdup_printf("%06x", pc);
	label = glade_xml_get_widget(xml, "label44");
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);
	
	result = gtk_dialog_run(GTK_DIALOG(dbox));
	switch (result) {
	case GTK_RESPONSE_OK:
		break;
	default:
		break;
	}

	gtk_widget_destroy(dbox);

	return 0;
}

gint display_dbgcause_dbox()
{
	gint type, id, mode;

    // get context
	ti68k_bkpt_get_cause(&type, &mode, &id);
    if(!type && !mode && !id)
        return 0;

	// exception or code/mem ?
	if(type == BK_TYPE_EXCEPTION)
		dbgcause2_display_dbox();
	else
		dbgcause1_display_dbox();

	return 0;
}

gint display_dbgcause_dbox2(GtkWidget *sb)
{
	gint type, id, mode;
	guint sb_id;
	gchar *str = NULL;

    // get context
	ti68k_bkpt_get_cause(&type, &mode, &id);
    if(!type && !mode && !id)
	{
		str = g_strdup("");
		sb_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(sb), str);
		gtk_statusbar_push(GTK_STATUSBAR(sb), sb_id, str);
		g_free(str);

        return 0;
	}

	// user break
	if(!type && !mode)
	{
		uint32_t value;

		ti68k_register_get_pc(&value);
		str = g_strdup_printf("User break (PC=$%06x)", value);
	}

	// exception or code/mem ?
	else if(type == BK_TYPE_EXCEPTION)
	{
		// exception
		uint32_t sp;
		uint32_t pc;
		uint32_t sr;

		ti68k_register_get_sp(&sp);
		sr = mem_rd_long(sp);

		ti68k_register_get_sp(&sp);
		pc = mem_rd_long(sp+2);

		str = g_strdup_printf("type=<%s>, id=#%i, SR=%04x, PC=%06x", 
			ti68k_exception_to_string(mode), id, sr, pc);
	}
	else if(type == BK_TYPE_CODE)
	{
		// code
		uint32_t value;

		ti68k_register_get_pc(&value);
		str = g_strdup_printf("type=<%s>, id=#%i, PC=$%06x", 
			ti68k_bkpt_type_to_string(type), id, value);
	}
	else if((type == BK_TYPE_ACCESS) || (type ==BK_TYPE_RANGE))
	{
		// mem access or range
		uint32_t value, min, max;
		gchar *str1, *str2;

		value = ti68k_debug_get_old_pc();
		str1 = g_strdup_printf("type=<%s>, id=#%i, mode=<%s>, PC=$%06x", 
			ti68k_bkpt_type_to_string(type), id, 
			ti68k_bkpt_mode_to_string(type, mode), value);
	
		switch(type)
		{

		case BK_TYPE_ACCESS:
			ti68k_bkpt_get_access(id, &min, mode);
			str2 = g_strdup_printf("mem=$%06x", min);
			break;
		case BK_TYPE_RANGE:
			ti68k_bkpt_get_range(id, &min, &max, mode);
			str2 = g_strdup_printf("mem=$%06x-$%06x", min, max);
			break;
		default:
			str2 = g_strdup("n/a");
			break;
		}

		str = g_strconcat(str1, ", ", str2, NULL);
		g_free(str1);
		g_free(str2);
	}
	else if(type == BK_TYPE_PROTECT)
	{
		uint32_t value;

		ti68k_register_get_pc(&value);

		switch(bkpts.id)
		{
		case 1:
			str = g_strdup_printf("hw protection violation: FLASH execution at $%06x. Execution allowed until $%06x.", value, 0x390000+tihw.archive_limit*0x10000);
			break;
		case 2:
			str = g_strdup_printf("hw protection violation: RAM execution at $%06x.", value);
			break;
		case 3:
			str = g_strdup_printf("hw protection violation: FLASH execution at $%06x. Execution allowed until $%06x.", value, tihw.rom_base + 0x10000 + tihw.io2[0x13]*0x10000);
			break;
		default: 
			str = g_strdup("bug !"); 
			break;
		}
	}
	else if(type == BK_TYPE_PGMENTRY)
	{
		uint16_t handle;
		uint32_t pc;

		ti68k_register_get_pc(&pc);		
		ti68k_bkpt_get_pgmentry(id, &handle);

		str = g_strdup_printf("type=<%s>, id=#%i, handle=$%04x, PC=$%06x", 
			ti68k_bkpt_type_to_string(type), id, handle, pc);
	}
	else if(type == BK_TYPE_BIT)
	{
		uint32_t value, addr;
		uint8_t checks, states;

		value = ti68k_debug_get_old_pc();
		ti68k_bkpt_get_bits(id, &addr, &checks, &states);
		str = g_strdup_printf("type=<%s>, id=#%i, PC=$%06x, ($%06x)=#$%02x", 
			ti68k_bkpt_type_to_string(type), id, value, addr, mem_rd_byte(addr));
	}
	else
	{
		str = g_strdup("bug !");
	}

	sb_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(sb), str);
	gtk_statusbar_push(GTK_STATUSBAR(sb), sb_id, str);
	g_free(str);

	return 0;
}
