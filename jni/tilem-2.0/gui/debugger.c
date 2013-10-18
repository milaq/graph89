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
#include <stdlib.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <ticalcs.h>
#include <tilem.h>
#include <tilemdb.h>

#include "gui.h"
#include "disasmview.h"
#include "files.h"
#include "msgbox.h"
#include "fixedtreeview.h"
#include "memmodel.h"

/* Stack list */
enum
{
	COL_OFFSET_STK = 0,
	COL_VALUE_STK,
  	NUM_COLS_STK
};

/* Indices in reg_entries */
enum {
	R_AF, R_BC, R_DE, R_HL, R_IX, R_SP,
	R_AF2, R_BC2, R_DE2, R_HL2, R_IY, R_PC,
	R_IM, R_I,
	NUM_REGS
};

/* Labels for the entries */
static const char * const reg_labels[] = {
	N_("A_F:"), N_("B_C:"), N_("D_E:"), N_("H_L:"), N_("I_X:"), N_("SP:"),
	N_("AF':"), N_("BC':"), N_("DE':"), N_("HL':"), N_("I_Y:"), N_("PC:"),
	N_("IM:"), N_("I:")
};

/* Labels for the flag buttons */
static const char flag_labels[][2] = {
	"C", "N", "P", "X", "H", "Y", "Z", "S"
};

/* Read a word */
static dword read_mem_word(TilemCalc *calc, dword addr)
{
	dword phys, v;

	phys = (*calc->hw.mem_ltop)(calc, addr & 0xffff);
	v = calc->mem[phys];
	phys = (*calc->hw.mem_ltop)(calc, (addr + 1) & 0xffff);
	v += calc->mem[phys] << 8;
	return v;
}

/* Determine model name for the purpose of looking up default system
   symbols */
static const char *get_sys_name(const TilemCalc *calc)
{
	g_return_val_if_fail(calc != NULL, NULL);

	switch (calc->hw.model_id) {
	case TILEM_CALC_TI83:
	case TILEM_CALC_TI76:
		return "ti83";

	case TILEM_CALC_TI83P:
	case TILEM_CALC_TI83P_SE:
	case TILEM_CALC_TI84P:
	case TILEM_CALC_TI84P_SE:
	case TILEM_CALC_TI84P_NSPIRE:
		return "ti83p";

	default:
		return calc->hw.name;
	}
}

/* Load default system symbols */
static void load_default_symbols(TilemDebugger *dbg)
{
	char *base, *path, *dname;
	const char *errstr;
	FILE *symfile;

	base = g_strdup_printf("%s.sym", get_sys_name(dbg->emu->calc));
	path = get_shared_file_path("symbols", base, NULL);
	g_free(base);
	if (!path)
		return;

	symfile = g_fopen(path, "rb");
	if (!symfile) {
		errstr = g_strerror(errno);
		dname = g_filename_display_name(path);
		messagebox02(NULL, GTK_MESSAGE_ERROR,
		             _("Unable to read symbols"),
		             _("An error occurred while reading %s: %s"),
		             dname, errstr);
		g_free(dname);
		g_free(path);
		return;
	}

	tilem_disasm_read_symbol_file(dbg->dasm, symfile);

	fclose(symfile);
	g_free(path);
}

/* Cancel temporary breakpoint */
static void cancel_step_bp(TilemDebugger *dbg)
{
	if (!dbg->step_bp)
		return;

	g_return_if_fail(dbg->emu->calc != NULL);
	tilem_calc_emulator_lock(dbg->emu);
	tilem_z80_remove_breakpoint(dbg->emu->calc, dbg->step_bp);
	tilem_calc_emulator_unlock(dbg->emu);
	dbg->step_bp = 0;
}

/* Actions */

/* Run (but leave debugger window open) */
static void action_run(G_GNUC_UNUSED GtkAction *a, gpointer data)
{
	TilemDebugger *dbg = data;
	cancel_step_bp(dbg);
	tilem_calc_emulator_run(dbg->emu);
	tilem_debugger_refresh(dbg, TRUE);
}

/* Pause */
static void action_pause(G_GNUC_UNUSED GtkAction *a, gpointer data)
{
	TilemDebugger *dbg = data;
	tilem_debugger_show(dbg);
	cancel_step_bp(dbg);
}

/* Stepping */

static int bptest_step(TilemCalc *calc, dword op, G_GNUC_UNUSED void *data)
{
	/* Single step condition: if calculator is halted, wait until
	   an interrupt occurs; otherwise, stop after any
	   instruction. */

	if (op != 0x76 && (op & ~0x2000) != 0xdd76)
		/* not a HALT instruction */
		return 1;
	else if (calc->z80.interrupts != 0 && calc->z80.r.iff1)
		return 1;
	else
		return 0;
}

static int bptest_step_over(TilemCalc *calc, dword op, void *data)
{
	TilemDebugger *dbg = data;
	dword destaddr;

	/* Step-over condition: behavior depends on what instruction
	   is executed.

	   For most instructions, stop when we reach the "next line"
	   as determined by disassembly.  This means skipping over
	   CALLs, RSTs, HALTs, and macros.

	   For jump and return instructions, stop at the current PC,
	   whatever that is.

	   In both cases, wait until we actually reach the target PC,
	   rather than halting immediately; the effect of this is that
	   if an interrupt has occurred, we also "step over" the
	   ISR. */

	if ((op & ~0x20ff) == 0xdd00)
		op &= 0xff;

	if (op == 0xc3 /* JP */
	    || op == 0xc9 /* RET */
	    || op == 0xe9 /* JP HL/IX/IY */
	    || (op & ~0x38) == 0 /* JR, DJNZ, NOP, or EX AF,AF' */
	    || (op & ~0x38) == 0xc2 /* conditional JP */
	    || (op & ~0x38) == 0xc0 /* conditional RET */
	    || (op & ~0x38) == 0xed45) /* RETI/RETN */
		destaddr = calc->z80.r.pc.d;
	else
		destaddr = dbg->step_next_addr;

	destaddr &= 0xffff;

	/* Delete this breakpoint, and replace it with a simple exec
	   breakpoint at the target address. */

	tilem_z80_remove_breakpoint(calc, dbg->step_bp);
	dbg->step_bp = tilem_z80_add_breakpoint(calc, TILEM_BREAK_MEM_EXEC,
	                                        destaddr, destaddr, 0xffff,
	                                        NULL, NULL);
	return 0;
}

static int bptest_finish(TilemCalc *calc, dword op, void *data)
{
	dword exitsp = TILEM_PTR_TO_DWORD(data);
	byte f;

	/* Finish condition: wait until stack pointer is greater than
	   a certain value, and we execute a return instruction.  JP
	   HL/IX/IY are also considered return instructions. */

	if (calc->z80.r.sp.w.l <= exitsp)
		return 0;

	if ((op & ~0x20ff) == 0xdd00)
		op &= 0xff;

	f = calc->z80.r.af.b.l;

	switch (op) {
	case 0xc9: /* RET */
	case 0xe9: /* JP HL/IX/IY */
	case 0xed45: /* RETN */
	case 0xed4d: /* RETI */
	case 0xed55:
	case 0xed5d:
	case 0xed65:
	case 0xed6d:
	case 0xed75:
	case 0xed7d:
		return 1;

	/* conditionals: check if condition was true */
	case 0xc0: return !(f & 0x40);
	case 0xc8: return (f & 0x40);
	case 0xd0: return !(f & 0x01);
	case 0xd8: return (f & 0x01);
	case 0xe0: return !(f & 0x04);
	case 0xe8: return (f & 0x04);
	case 0xf0: return !(f & 0x80);
	case 0xf8: return (f & 0x80);

	default:
		return 0;
	}
}

static gboolean post_resume_refresh(gpointer data)
{
	TilemDebugger *dbg = data;
	tilem_debugger_refresh(dbg, FALSE);
	return FALSE;
}

static void run_with_step_condition(TilemDebugger *dbg,
                                    TilemZ80BreakpointFunc func,
                                    void *data)
{
	tilem_calc_emulator_lock(dbg->emu);
	dbg->step_bp = tilem_z80_add_breakpoint(dbg->emu->calc,
	                                        TILEM_BREAK_EXECUTE, 0, 0, 0,
	                                        func, data);
	tilem_calc_emulator_unlock(dbg->emu);
	tilem_calc_emulator_run(dbg->emu);
	/* Don't refresh right away, to avoid flickering */
	g_timeout_add(10, &post_resume_refresh, dbg);
}

/* Execute one instruction */
static void action_step(G_GNUC_UNUSED GtkAction *a, gpointer data)
{
	TilemDebugger *dbg = data;

	if (!dbg->emu->paused)
		return;

	g_return_if_fail(dbg->emu->calc != NULL);

	cancel_step_bp(dbg);

	run_with_step_condition(dbg, &bptest_step, NULL);
}

/* Skip over an instruction */
static void action_step_over(G_GNUC_UNUSED GtkAction *a, gpointer data)
{
	TilemDebugger *dbg = data;

	if (!dbg->emu->paused)
		return;

	g_return_if_fail(dbg->emu->calc != NULL);

	cancel_step_bp(dbg);

	tilem_calc_emulator_lock(dbg->emu);
	tilem_disasm_disassemble(dbg->dasm, dbg->emu->calc, 0,
	                         dbg->emu->calc->z80.r.pc.w.l,
	                         &dbg->step_next_addr,
	                         NULL, 0);
	tilem_calc_emulator_unlock(dbg->emu);

	run_with_step_condition(dbg, &bptest_step_over, dbg);
}

/* Run until current subroutine finishes */
static void action_finish(G_GNUC_UNUSED GtkAction *a, gpointer data)
{
	TilemDebugger *dbg = data;
	dword sp;

	if (!dbg->emu->paused)
		return;

	g_return_if_fail(dbg->emu->calc != NULL);

	cancel_step_bp(dbg);

	tilem_calc_emulator_lock(dbg->emu);
	sp = dbg->emu->calc->z80.r.sp.w.l;
	tilem_calc_emulator_unlock(dbg->emu);

	run_with_step_condition(dbg, &bptest_finish,
	                        TILEM_DWORD_TO_PTR(sp));
}

/* Toggle breakpoint at selected line */
static void action_toggle_breakpoint(G_GNUC_UNUSED GtkAction *a, gpointer data)
{
	TilemDebugger *dbg = data;
	GtkWidget *focus;

	focus = gtk_window_get_focus(GTK_WINDOW(dbg->window));
	if (TILEM_IS_DISASM_VIEW(focus))
		tilem_disasm_view_toggle_breakpoint(TILEM_DISASM_VIEW(focus));
}

/* Edit breakpoints */
static void action_edit_breakpoints(G_GNUC_UNUSED GtkAction *a, gpointer data)
{
	TilemDebugger *dbg = data;
	tilem_debugger_edit_breakpoints(dbg);
}

/* Close debugger window */
static void action_close(G_GNUC_UNUSED GtkAction *a, gpointer data)
{
	TilemDebugger *dbg = data;
	tilem_debugger_hide(dbg);
}

static void keypad_dlg_response(G_GNUC_UNUSED GtkDialog *dlg,
                                G_GNUC_UNUSED int response,
                                gpointer data)
{
	gtk_toggle_action_set_active(data, FALSE);
}

/* Show/hide keypad dialog */
static void action_view_keypad(GtkToggleAction *action, gpointer data)
{
	TilemDebugger *dbg = data;

	if (!dbg->keypad_dialog) {
		dbg->keypad_dialog = tilem_keypad_dialog_new(dbg);
		g_signal_connect(dbg->keypad_dialog->window, "response",
		                 G_CALLBACK(keypad_dlg_response), action);
	}

	if (gtk_toggle_action_get_active(action))
		gtk_window_present(GTK_WINDOW(dbg->keypad_dialog->window));
	else
		gtk_widget_hide(dbg->keypad_dialog->window);
}

/* Set memory addressing mode */
static void action_mem_mode(GtkRadioAction *action,
                            G_GNUC_UNUSED GtkRadioAction *current,
                            gpointer data)
{
	TilemDebugger *dbg = data;

	dbg->mem_logical = gtk_radio_action_get_current_value(action);

	tilem_disasm_view_set_logical(TILEM_DISASM_VIEW(dbg->disasm_view),
	                              dbg->mem_logical);

	tilem_debugger_mem_view_configure(dbg->mem_view,
	                                  dbg->emu, dbg->mem_rowsize,
	                                  dbg->mem_start, dbg->mem_logical);

	tilem_config_set("debugger",
	                 "mem_logical/b", dbg->mem_logical,
	                 NULL);
}

/* Prompt for an address to view */
static void action_go_to_address(G_GNUC_UNUSED GtkAction *action, gpointer data)
{
	TilemDebugger *dbg = data;
	TilemDisasmView *dv = TILEM_DISASM_VIEW(dbg->disasm_view);
	dword addr;
	gboolean addr_set, logical;

	addr_set = tilem_disasm_view_get_cursor(dv, &addr, &logical);

	if (!tilem_prompt_address(dbg, GTK_WINDOW(dbg->window),
	                          _("Go to Address"), _("Address:"),
	                          &addr, !logical, addr_set))
		return;

	tilem_disasm_view_go_to_address(dv, addr, logical);
	gtk_widget_grab_focus(dbg->disasm_view);
}

/* Jump to address at given stack index */
static void go_to_stack_pos(TilemDebugger *dbg, int pos)
{
	dword addr;
	GtkTreePath *path;
	GtkTreeSelection *sel;

	dbg->stack_index = pos;

	tilem_calc_emulator_lock(dbg->emu);
	if (pos < 0)
		addr = dbg->emu->calc->z80.r.pc.d;
	else
		addr = read_mem_word(dbg->emu->calc,
		                     dbg->emu->calc->z80.r.sp.d + 2 * pos);
	tilem_calc_emulator_unlock(dbg->emu);

	tilem_disasm_view_go_to_address(TILEM_DISASM_VIEW(dbg->disasm_view),
	                                addr, TRUE);
	gtk_widget_grab_focus(dbg->disasm_view);

	if (pos >= 0) {
		path = gtk_tree_path_new_from_indices(pos, -1);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(dbg->stack_view),
		                         path, NULL, FALSE);
		gtk_tree_path_free(path);
	}
	else {
		sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(dbg->stack_view));
		gtk_tree_selection_unselect_all(sel);
	}

	gtk_action_set_sensitive(dbg->prev_stack_action, (pos >= 0));
}

/* Jump to current PC */
static void action_go_to_pc(G_GNUC_UNUSED GtkAction *action, gpointer data)
{
	TilemDebugger *dbg = data;
	go_to_stack_pos(dbg, -1);
}

/* Jump one byte backward */
static void action_scroll_prev_byte(G_GNUC_UNUSED GtkAction *action,
                                    gpointer data)
{
	TilemDebugger *dbg = data;
	TilemDisasmView *dv = TILEM_DISASM_VIEW(dbg->disasm_view);

	tilem_disasm_view_scroll_bytes(dv, -1);
	gtk_widget_grab_focus(dbg->disasm_view);
}

/* Jump one byte forward */
static void action_scroll_next_byte(G_GNUC_UNUSED GtkAction *action,
                                    gpointer data)
{
	TilemDebugger *dbg = data;
	TilemDisasmView *dv = TILEM_DISASM_VIEW(dbg->disasm_view);

	tilem_disasm_view_scroll_bytes(dv, 1);
	gtk_widget_grab_focus(dbg->disasm_view);
}

/* Jump to previous stack entry */
static void action_prev_stack_entry(G_GNUC_UNUSED GtkAction *action,
                                        gpointer data)
{
	TilemDebugger *dbg = data;
	if (dbg->stack_index >= 0)
		go_to_stack_pos(dbg, dbg->stack_index - 1);
}

/* Jump to next stack entry */
static void action_next_stack_entry(G_GNUC_UNUSED GtkAction *action,
                                    gpointer data)
{
	TilemDebugger *dbg = data;
	go_to_stack_pos(dbg, dbg->stack_index + 1);
}


static const GtkActionEntry run_action_ents[] =
	{{ "pause", GTK_STOCK_MEDIA_PAUSE, N_("_Pause"), "Escape",
	   N_("Pause emulation"), G_CALLBACK(action_pause) }};

static const GtkActionEntry paused_action_ents[] =
	{{ "run", GTK_STOCK_MEDIA_PLAY, N_("_Run"), "F5",
	   N_("Resume emulation"), G_CALLBACK(action_run) },
	 { "step", "tilem-db-step", N_("_Step"), "F7",
	   N_("Execute one instruction"), G_CALLBACK(action_step) },
	 { "step-over", "tilem-db-step-over", N_("Step _Over"), "F8",
	   N_("Run to the next line (skipping over subroutines)"),
	   G_CALLBACK(action_step_over) },
	 { "finish", "tilem-db-finish", N_("_Finish Subroutine"), "F9",
	   N_("Run to end of the current subroutine"), G_CALLBACK(action_finish) },
	 { "toggle-breakpoint", NULL, N_("Toggle Breakpoint"), "F2",
	   N_("Enable or disable breakpoint at the selected address"),
	   G_CALLBACK(action_toggle_breakpoint) },
	 { "edit-breakpoints", NULL, N_("_Breakpoints"), "<control>B",
	   N_("Add, remove, or modify breakpoints"),
	   G_CALLBACK(action_edit_breakpoints) },
	 { "go-to-address", GTK_STOCK_JUMP_TO, N_("_Address..."), "<control>L",
	   N_("Jump to an address"),
	   G_CALLBACK(action_go_to_address) },
	 { "go-to-pc", NULL, N_("Current P_C"), "<alt>Home",
	   N_("Jump to the current program counter"),
	   G_CALLBACK(action_go_to_pc) },
	 { "prev-stack-entry", GTK_STOCK_GO_UP, N_("_Previous Stack Entry"), "<alt>Page_Up",
	   N_("Jump to the previous address in the stack"),
	   G_CALLBACK(action_prev_stack_entry) },
	 { "next-stack-entry", GTK_STOCK_GO_DOWN, N_("_Next Stack Entry"), "<alt>Page_Down",
	   N_("Jump to the next address in the stack"),
	   G_CALLBACK(action_next_stack_entry) },
	 { "scroll-prev-byte", NULL, N_("One Byte _Backward"), "<control>Up",
	   N_("Scroll backward by one byte"),
	   G_CALLBACK(action_scroll_prev_byte) },
	 { "scroll-next-byte", NULL, N_("One Byte _Forward"), "<control>Down",
	   N_("Scroll forward by one byte"),
	   G_CALLBACK(action_scroll_next_byte) }};

static const GtkRadioActionEntry mem_mode_ents[] =
	{{ "view-logical", 0, N_("_Logical Addresses"), 0,
	   N_("Show contents of the current Z80 address space"), 1 },
	 { "view-absolute", 0, N_("_Absolute Addresses"), 0,
	   N_("Show all memory contents"), 0 }};

static const GtkActionEntry misc_action_ents[] =
	{{ "debug-menu", 0, N_("_Debug"), 0, 0, 0 },
	 { "view-menu", 0, N_("_View"), 0, 0, 0 },
	 { "go-menu", 0, N_("_Go"), 0, 0, 0 },
	 { "close", GTK_STOCK_CLOSE, 0, 0,
	   N_("Close the debugger"), G_CALLBACK(action_close) }};

static const GtkToggleActionEntry misc_toggle_ents[] =
	{{ "view-keypad", 0, N_("_Keypad"), 0,
	   N_("Show the calculator keypad state"),
	   G_CALLBACK(action_view_keypad), FALSE }};

/* Callbacks */

/* Register edited */
static void reg_edited(GtkEntry *ent, gpointer data)
{
	TilemDebugger *dbg = data;
	TilemCalc *calc;
	const char *text;
	char *end;
	dword value;
	int i;

	if (dbg->refreshing)
		return;

	calc = dbg->emu->calc;
	g_return_if_fail(calc != NULL);

	text = gtk_entry_get_text(ent);
	value = strtol(text, &end, 16);

	for (i = 0; i < NUM_REGS; i++)
		if (ent == (GtkEntry*) dbg->reg_entries[i])
			break;

	tilem_calc_emulator_lock(dbg->emu);
	switch (i) {
	case R_AF: calc->z80.r.af.d = value; break;
	case R_BC: calc->z80.r.bc.d = value; break;
	case R_DE: calc->z80.r.de.d = value; break;
	case R_HL: calc->z80.r.hl.d = value; break;
	case R_AF2: calc->z80.r.af2.d = value; break;
	case R_BC2: calc->z80.r.bc2.d = value; break;
	case R_DE2: calc->z80.r.de2.d = value; break;
	case R_HL2: calc->z80.r.hl2.d = value; break;
	case R_SP: calc->z80.r.sp.d = value; break;
	case R_PC: calc->z80.r.pc.d = value; break;
	case R_IX: calc->z80.r.ix.d = value; break;
	case R_IY: calc->z80.r.iy.d = value; break;
	case R_I: calc->z80.r.ir.b.h = value; break;
	}
	tilem_calc_emulator_unlock(dbg->emu);

	/* Set the value of the register immediately, but don't
	   refresh the display: refreshing the registers themselves
	   while user is trying to edit them would just be obnoxious,
	   and refreshing stack and disassembly would be at least
	   distracting.  Instead, we'll refresh only when focus
	   changes. */

	dbg->delayed_refresh = TRUE;
}

/* Flag button toggled */
static void flag_edited(GtkToggleButton *btn, gpointer data)
{
	TilemDebugger *dbg = data;
	TilemCalc *calc;
	int i;

	if (dbg->refreshing)
		return;

	calc = dbg->emu->calc;
	g_return_if_fail(calc != NULL);

	for (i = 0; i < 8; i++)
		if (btn == (GtkToggleButton*) dbg->flag_buttons[i])
			break;

	tilem_calc_emulator_lock(dbg->emu);
	if (gtk_toggle_button_get_active(btn))
		calc->z80.r.af.d |= (1 << i);
	else
		calc->z80.r.af.d &= ~(1 << i);
	tilem_calc_emulator_unlock(dbg->emu);

	/* refresh AF */
	tilem_debugger_refresh(dbg, FALSE);
}

/* IM edited */
static void im_edited(GtkEntry *ent, gpointer data)
{
	TilemDebugger *dbg = data;
	TilemCalc *calc;
	const char *text;
	char *end;
	int value;

	if (dbg->refreshing)
		return;

	calc = dbg->emu->calc;
	g_return_if_fail(calc != NULL);

	text = gtk_entry_get_text(ent);
	value = strtol(text, &end, 0);

	tilem_calc_emulator_lock(dbg->emu);
	if (value >= 0 && value <= 2)
		calc->z80.r.im = value;
	tilem_calc_emulator_unlock(dbg->emu);
	/* no need to refresh */
}

/* IFF button toggled */
static void iff_edited(GtkToggleButton *btn, gpointer data)
{
	TilemDebugger *dbg = data;
	TilemCalc *calc;

	if (dbg->refreshing)
		return;

	calc = dbg->emu->calc;
	g_return_if_fail(calc != NULL);

	tilem_calc_emulator_lock(dbg->emu);
	if (gtk_toggle_button_get_active(btn))
		calc->z80.r.iff1 = calc->z80.r.iff2 = 1;
	else
		calc->z80.r.iff1 = calc->z80.r.iff2 = 0;
	tilem_calc_emulator_unlock(dbg->emu);
	/* no need to refresh */
}

/* Main window's focus widget changed */
static void focus_changed(G_GNUC_UNUSED GtkWindow *win,
                          G_GNUC_UNUSED GtkWidget *widget,
                          gpointer data)
{
	TilemDebugger *dbg = data;

	/* delayed refresh - see reg_edited() above */
	if (dbg->delayed_refresh)
		tilem_debugger_refresh(dbg, FALSE);
}

/* Main window received a "delete" message */
static gboolean delete_win(G_GNUC_UNUSED GtkWidget *w,
                           G_GNUC_UNUSED GdkEvent *ev,
                           gpointer data)
{
	TilemDebugger *dbg = data;
	tilem_debugger_hide(dbg);
	return TRUE;
}

/* Create table of widgets for editing registers */
static GtkWidget *create_registers(TilemDebugger *dbg)
{
	GtkWidget *vbox, *tbl, *lbl, *hbox, *ent, *btn;
	int i;

	vbox = gtk_vbox_new(FALSE, 6);

	tbl = gtk_table_new(6, 4, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(tbl), 6);
	gtk_table_set_col_spacings(GTK_TABLE(tbl), 6);
	gtk_table_set_col_spacing(GTK_TABLE(tbl), 1, 12);

	for (i = 0; i < 12; i++) {
		lbl = gtk_label_new_with_mnemonic(_(reg_labels[i]));
		gtk_misc_set_alignment(GTK_MISC(lbl), LABEL_X_ALIGN, 0.5);
		gtk_table_attach(GTK_TABLE(tbl), lbl,
		                 2 * (i / 6), 2 * (i / 6) + 1,
		                 (i % 6), (i % 6) + 1,
		                 GTK_FILL, GTK_FILL, 0, 0);

		dbg->reg_entries[i] = ent = gtk_entry_new();
		gtk_entry_set_width_chars(GTK_ENTRY(ent), 5);
		g_signal_connect(ent, "changed", G_CALLBACK(reg_edited), dbg);
		gtk_table_attach(GTK_TABLE(tbl), ent,
		                 2 * (i / 6) + 1, 2 * (i / 6) + 2,
		                 (i % 6), (i % 6) + 1,
		                 GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

		gtk_label_set_mnemonic_widget(GTK_LABEL(lbl), ent);
	}

	gtk_box_pack_start(GTK_BOX(vbox), tbl, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(TRUE, 0);

	for (i = 7; i >= 0; i--) {
		btn = gtk_toggle_button_new_with_label(flag_labels[i]);
		dbg->flag_buttons[i] = btn;
		g_signal_connect(btn, "toggled", G_CALLBACK(flag_edited), dbg);
		gtk_box_pack_start(GTK_BOX(hbox), btn, TRUE, TRUE, 0);
	}

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 6);

	for (i = 12; i < 14; i++) {
		lbl = gtk_label_new(_(reg_labels[i]));
		gtk_box_pack_start(GTK_BOX(hbox), lbl, FALSE, FALSE, 0);

		dbg->reg_entries[i] = ent = gtk_entry_new();
		gtk_box_pack_start(GTK_BOX(hbox), ent, TRUE, TRUE, 0);
	}

	g_signal_connect(dbg->reg_entries[R_I], "changed",
	                 G_CALLBACK(reg_edited), dbg);
	g_signal_connect(dbg->reg_entries[R_IM], "changed",
	                 G_CALLBACK(im_edited), dbg);

	gtk_entry_set_width_chars(GTK_ENTRY(dbg->reg_entries[R_IM]), 2);
	gtk_entry_set_width_chars(GTK_ENTRY(dbg->reg_entries[R_I]), 3);

	dbg->iff_checkbox = btn = gtk_check_button_new_with_label(_("EI"));
	g_signal_connect(btn, "toggled", G_CALLBACK(iff_edited), dbg);
	gtk_box_pack_start(GTK_BOX(hbox), btn, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	return vbox;
}

/* Create the GtkTreeView to show the stack */
static GtkWidget *create_stack_view()
{
	GtkCellRenderer   *renderer;
	GtkWidget         *treeview;
	GtkTreeViewColumn *column;

	/* Create the stack list tree view and set title invisible */
	treeview = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeview),
	                                COL_VALUE_STK);

	/* Create the columns */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes
		("ADDR", renderer, "text", COL_OFFSET_STK, NULL);

	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes
		("VAL", renderer, "text", COL_VALUE_STK, NULL);

	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	return treeview;
}

/* Create a new scrolled window with sensible default settings. */
static GtkWidget *new_scrolled_window(GtkWidget *contents)
{
	GtkWidget *sw;
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
	                                    GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(sw), contents);
	return sw;
}

static const char uidesc[] =
	"<menubar name='menu-bar'>"
	" <menu action='debug-menu'>"
	"  <menuitem action='run'/>"
	"  <menuitem action='pause'/>"
	"  <separator/>"
	"  <menuitem action='step'/>"
	"  <menuitem action='step-over'/>"
	"  <menuitem action='finish'/>"
	"  <separator/>"
	"  <menuitem action='edit-breakpoints'/>"
	"  <separator/>"
	"  <menuitem action='close'/>"
	" </menu>"
	" <menu action='view-menu'>"
	"  <menuitem action='view-keypad'/>"
	"  <separator/>"
	"  <menuitem action='view-logical'/>"
	"  <menuitem action='view-absolute'/>"
	" </menu>"
	" <menu action='go-menu'>"
	"  <menuitem action='go-to-address'/>"
	"  <menuitem action='go-to-pc'/>"
	"  <menuitem action='scroll-prev-byte'/>"
	"  <menuitem action='scroll-next-byte'/>"
	"  <separator/>"
	"  <menuitem action='prev-stack-entry'/>"
	"  <menuitem action='next-stack-entry'/>"
	" </menu>"
	"</menubar>"
	"<toolbar name='toolbar'>"
	" <toolitem action='run'/>"
	" <toolitem action='pause'/>"
	" <separator/>"
	" <toolitem action='step'/>"
	" <toolitem action='step-over'/>"
	" <toolitem action='finish'/>"
	"</toolbar>"
	"<accelerator action='toggle-breakpoint'/>";

/* Create a new TilemDebugger. */
TilemDebugger *tilem_debugger_new(TilemCalcEmulator *emu)
{
	TilemDebugger *dbg;
	GtkWidget *hbox, *vbox, *vbox2, *vpaned, *sw, *menubar, *toolbar;
	GtkUIManager *uimgr;
	GtkAccelGroup *accelgrp;
	GError *err = NULL;
	int defwidth, defheight;

	g_return_val_if_fail(emu != NULL, NULL);

	dbg = g_slice_new0(TilemDebugger);
	dbg->emu = emu;
	dbg->dasm = tilem_disasm_new();

	dbg->last_bp_type = TILEM_DB_BREAK_LOGICAL;
	dbg->last_bp_mode = TILEM_DB_BREAK_EXEC;

	dbg->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(dbg->window), _("TilEm Debugger"));
	gtk_window_set_role(GTK_WINDOW(dbg->window), _("Debugger"));

	tilem_config_get("debugger",
	                 "width/i", &defwidth,
	                 "height/i", &defheight,
	                 "mem_width/i", &dbg->mem_rowsize,
	                 "mem_start/i", &dbg->mem_start,
	                 "mem_logical/b=1", &dbg->mem_logical,
	                 NULL);

	if (dbg->mem_rowsize <= 0)
		dbg->mem_rowsize = 8;
	if (dbg->mem_start < 0 || dbg->mem_start >= dbg->mem_rowsize)
		dbg->mem_start = 0;

	if (defwidth <= 0 || defheight <= 0) {
		defwidth = 600;
		defheight = 400;
	}

	gtk_window_set_default_size(GTK_WINDOW(dbg->window),
	                            defwidth, defheight);

	g_signal_connect(dbg->window, "set-focus",
	                 G_CALLBACK(focus_changed), dbg);
	g_signal_connect(dbg->window, "delete-event",
	                 G_CALLBACK(delete_win), dbg);

	vbox2 = gtk_vbox_new(FALSE, 0);

	/* Actions and menu bar */

	uimgr = gtk_ui_manager_new();
	dbg->run_actions = gtk_action_group_new("Debug");
	gtk_action_group_set_translation_domain(dbg->run_actions, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(dbg->run_actions, run_action_ents,
	                             G_N_ELEMENTS(run_action_ents), dbg);
	gtk_ui_manager_insert_action_group(uimgr, dbg->run_actions, 0);

	dbg->paused_actions = gtk_action_group_new("Debug");
	gtk_action_group_set_translation_domain(dbg->paused_actions, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(dbg->paused_actions, paused_action_ents,
	                             G_N_ELEMENTS(paused_action_ents), dbg);
	gtk_action_group_add_radio_actions(dbg->paused_actions, mem_mode_ents,
	                                   G_N_ELEMENTS(mem_mode_ents),
	                                   dbg->mem_logical,
	                                   G_CALLBACK(action_mem_mode), dbg);
	gtk_ui_manager_insert_action_group(uimgr, dbg->paused_actions, 0);

	dbg->misc_actions = gtk_action_group_new("Debug");
	gtk_action_group_set_translation_domain(dbg->misc_actions, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(dbg->misc_actions, misc_action_ents,
	                             G_N_ELEMENTS(misc_action_ents), dbg);
	gtk_action_group_add_toggle_actions(dbg->misc_actions, misc_toggle_ents,
	                                    G_N_ELEMENTS(misc_toggle_ents), dbg);
	gtk_ui_manager_insert_action_group(uimgr, dbg->misc_actions, 0);

	dbg->prev_stack_action = gtk_action_group_get_action(dbg->paused_actions,
	                                                     "prev-stack-entry");

	accelgrp = gtk_ui_manager_get_accel_group(uimgr);
	gtk_window_add_accel_group(GTK_WINDOW(dbg->window), accelgrp);

	if (!gtk_ui_manager_add_ui_from_string(uimgr, uidesc, -1, &err))
		g_error("Failed to create menus: %s", err->message);

	menubar = gtk_ui_manager_get_widget(uimgr, "/menu-bar");
	gtk_box_pack_start(GTK_BOX(vbox2), menubar, FALSE, FALSE, 0);

	toolbar = gtk_ui_manager_get_widget(uimgr, "/toolbar");
	gtk_box_pack_start(GTK_BOX(vbox2), toolbar, FALSE, FALSE, 0);

	g_object_unref(uimgr);

	hbox = gtk_hbox_new(FALSE, 6);

	vpaned = gtk_vpaned_new();

	/* Disassembly view */

	dbg->disasm_view = tilem_disasm_view_new(dbg);
	tilem_disasm_view_set_logical(TILEM_DISASM_VIEW(dbg->disasm_view),
	                              dbg->mem_logical);
	sw = new_scrolled_window(dbg->disasm_view);
	gtk_paned_pack1(GTK_PANED(vpaned), sw, TRUE, TRUE);

	/* Memory view */

	dbg->mem_view = tilem_debugger_mem_view_new(dbg);
	sw = new_scrolled_window(dbg->mem_view);
	gtk_paned_pack2(GTK_PANED(vpaned), sw, TRUE, TRUE);

	gtk_box_pack_start(GTK_BOX(hbox), vpaned, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 6);

	/* Registers */

	dbg->regbox = create_registers(dbg);
	gtk_box_pack_start(GTK_BOX(vbox), dbg->regbox, FALSE, FALSE, 0);

	/* Stack view */

	dbg->stack_view = create_stack_view();
	sw = new_scrolled_window(dbg->stack_view);
	gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox2), hbox, TRUE, TRUE, 0);
	gtk_widget_show_all(vbox2);

	gtk_container_add(GTK_CONTAINER(dbg->window), vbox2);

	tilem_debugger_calc_changed(dbg);

	return dbg;
}

/* Save the dimension for the debugger */
static void save_debugger_dimension(TilemDebugger *dbg)
{
	gint width, height;

	if (!dbg->window)
		return;

	gtk_window_get_size(GTK_WINDOW(dbg->window), &width, &height);

	if (width <= 0 || height <= 0)
		return;

	tilem_config_set("debugger",
	                 "width/i", width,
	                 "height/i", height,
	                 NULL);
}

static void free_all_breakpoints(TilemDebugger *dbg)
{
	GSList *l;
	TilemDebugBreakpoint *bp;

	for (l = dbg->breakpoints; l; l = l->next) {
		bp = l->data;
		g_slice_free(TilemDebugBreakpoint, bp);
	}

	g_slist_free(dbg->breakpoints);
	dbg->breakpoints = NULL;
}

/* Free a TilemDebugger. */
void tilem_debugger_free(TilemDebugger *dbg)
{
	g_return_if_fail(dbg != NULL);

	save_debugger_dimension(dbg);

	if (dbg->emu && dbg->emu->dbg == dbg)
		dbg->emu->dbg = NULL;
	if (dbg->window) {
		gtk_widget_destroy(dbg->window);
	}
	if (dbg->dasm)
		tilem_disasm_free(dbg->dasm);
	if (dbg->run_actions)
		g_object_unref(dbg->run_actions);
	if (dbg->paused_actions)
		g_object_unref(dbg->paused_actions);
	if (dbg->misc_actions)
		g_object_unref(dbg->misc_actions);

	free_all_breakpoints(dbg);

	g_slice_free(TilemDebugger, dbg);
}

static void entry_printf(GtkWidget *ent, const char *s, ...)
{
	char buf[20];
	va_list ap;
	va_start(ap, s);
	g_vsnprintf(buf, sizeof(buf), s, ap);
	va_end(ap);
	gtk_entry_set_text(GTK_ENTRY(ent), buf);
}

static void refresh_regs(TilemDebugger *dbg)
{
	TilemCalc *calc = dbg->emu->calc;
	int i;
	GtkToggleButton *btn;

	entry_printf(dbg->reg_entries[R_AF], "%04X", calc->z80.r.af.w.l);
	entry_printf(dbg->reg_entries[R_BC], "%04X", calc->z80.r.bc.w.l);
	entry_printf(dbg->reg_entries[R_DE], "%04X", calc->z80.r.de.w.l);
	entry_printf(dbg->reg_entries[R_HL], "%04X", calc->z80.r.hl.w.l);
	entry_printf(dbg->reg_entries[R_AF2], "%04X", calc->z80.r.af2.w.l);
	entry_printf(dbg->reg_entries[R_BC2], "%04X", calc->z80.r.bc2.w.l);
	entry_printf(dbg->reg_entries[R_DE2], "%04X", calc->z80.r.de2.w.l);
	entry_printf(dbg->reg_entries[R_HL2], "%04X", calc->z80.r.hl2.w.l);
	entry_printf(dbg->reg_entries[R_SP], "%04X", calc->z80.r.sp.w.l);
	entry_printf(dbg->reg_entries[R_PC], "%04X", calc->z80.r.pc.w.l);
	entry_printf(dbg->reg_entries[R_IX], "%04X", calc->z80.r.ix.w.l);
	entry_printf(dbg->reg_entries[R_IY], "%04X", calc->z80.r.iy.w.l);
	entry_printf(dbg->reg_entries[R_I], "%02X", calc->z80.r.ir.b.h);
	entry_printf(dbg->reg_entries[R_IM], "%d", calc->z80.r.im);

	for (i = 0; i < 8; i++) {
		btn = GTK_TOGGLE_BUTTON(dbg->flag_buttons[i]);
		gtk_toggle_button_set_active(btn, calc->z80.r.af.d & (1 << i));
	}

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dbg->iff_checkbox),
	                             calc->z80.r.iff1);
}

/* Create GtkListStore and attach it */
static GtkTreeModel* fill_stk_list(TilemDebugger *dbg)
{
	GtkListStore  *store;
	GtkTreeIter    iter;
	char stack_offset[10];
	char stack_value[10];
	dword i, v;
	int n = 0;

	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	i = dbg->emu->calc->z80.r.sp.w.l;
	while  (i < 0x10000 && n < 512) {
		g_snprintf(stack_offset, sizeof(stack_offset), _("%04X:"), i);

		v = read_mem_word(dbg->emu->calc, i);
		g_snprintf(stack_value, sizeof(stack_value), "%04X", v);

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
		                   COL_OFFSET_STK, stack_offset,
		                   COL_VALUE_STK, stack_value, -1);
 
                i += 0x0002;
                n++;
        }
    
	return GTK_TREE_MODEL (store);
}

static void refresh_stack(TilemDebugger *dbg)
{
	GtkTreeModel *model = fill_stk_list(dbg);
	gtk_tree_view_set_model(GTK_TREE_VIEW(dbg->stack_view), model);
	g_object_unref(model);

	fixed_tree_view_init(dbg->stack_view, 0,
	                     COL_OFFSET_STK, "DDDD: ",
	                     COL_VALUE_STK, "DDDD ",
	                     -1);
}

static void unselect_all(GtkTreeView *tv)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tv);
	gtk_tree_selection_unselect_all(sel);
}

static void refresh_all(TilemDebugger *dbg, gboolean updatemem)
{
	TilemCalc *calc;
	gboolean paused;
	gboolean updatedasm = FALSE;
	GtkTreeModel *model;

	dbg->refreshing = TRUE;
	dbg->delayed_refresh = FALSE;

	tilem_calc_emulator_lock(dbg->emu);
	calc = dbg->emu->calc;
	paused = dbg->emu->paused;

	if (calc) {
		refresh_regs(dbg);

		if (dbg->lastwrite != calc->z80.lastwrite)
			updatemem = TRUE;

		if (updatemem || dbg->lastsp != calc->z80.r.sp.d)
			refresh_stack(dbg);

		if (paused && dbg->lastpc != calc->z80.r.pc.d)
			updatedasm = TRUE;

		dbg->lastwrite = calc->z80.lastwrite;
		dbg->lastsp = calc->z80.r.sp.d;
		dbg->lastpc = calc->z80.r.pc.d;
	}

	tilem_calc_emulator_unlock(dbg->emu);
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(dbg->mem_view));
	tilem_mem_model_clear_cache(TILEM_MEM_MODEL(model));

	gtk_widget_queue_draw(dbg->mem_view);

	if (paused != dbg->paused) {
		dbg->paused = paused;
		gtk_widget_set_sensitive(dbg->regbox, paused);
		gtk_widget_set_sensitive(dbg->disasm_view, paused);
		gtk_widget_set_sensitive(dbg->mem_view, paused);
		gtk_widget_set_sensitive(dbg->stack_view, paused);
		gtk_action_group_set_sensitive(dbg->run_actions, !paused);
		gtk_action_group_set_sensitive(dbg->paused_actions, paused);
		updatedasm = TRUE; /* need to redraw icons */
	}

	if (updatemem || updatedasm)
		tilem_disasm_view_refresh(TILEM_DISASM_VIEW(dbg->disasm_view));

	if (!paused) {
		unselect_all(GTK_TREE_VIEW(dbg->disasm_view));
		unselect_all(GTK_TREE_VIEW(dbg->mem_view));
		unselect_all(GTK_TREE_VIEW(dbg->stack_view));
	}

	if (dbg->keypad_dialog)
		tilem_keypad_dialog_refresh(dbg->keypad_dialog);

	dbg->refreshing = FALSE;
}

/* Show debugger, and pause emulator if not already paused. */
void tilem_debugger_show(TilemDebugger *dbg)
{
	g_return_if_fail(dbg != NULL);
	g_return_if_fail(dbg->emu->calc != NULL);
	tilem_calc_emulator_pause(dbg->emu);
	cancel_step_bp(dbg);
	refresh_all(dbg, TRUE);
	go_to_stack_pos(dbg, -1);
	gtk_window_present(GTK_WINDOW(dbg->window));
}

/* Hide debugger, and resume emulation if not already running. */
void tilem_debugger_hide(TilemDebugger *dbg)
{
	g_return_if_fail(dbg != NULL);
	gtk_widget_hide(dbg->window);
	tilem_calc_emulator_run(dbg->emu);
	save_debugger_dimension(dbg);
}

/* New calculator loaded. */
void tilem_debugger_calc_changed(TilemDebugger *dbg)
{
	TilemCalc *calc;

	g_return_if_fail(dbg != NULL);

	tilem_disasm_free(dbg->dasm);
	dbg->dasm = tilem_disasm_new();

	dbg->step_bp = 0;

	free_all_breakpoints(dbg);

	calc = dbg->emu->calc;
	if (!calc)
		return;

	load_default_symbols(dbg);

	tilem_debugger_mem_view_configure(dbg->mem_view,
	                                  dbg->emu, dbg->mem_rowsize,
	                                  dbg->mem_start, dbg->mem_logical);

	tilem_debugger_refresh(dbg, TRUE);

	if (dbg->keypad_dialog)
		tilem_keypad_dialog_calc_changed(dbg->keypad_dialog);
}

/* Update display. */
void tilem_debugger_refresh(TilemDebugger *dbg, gboolean updatemem)
{
	g_return_if_fail(dbg != NULL);

	if (!gtk_widget_get_visible(dbg->window))
		return;

	refresh_all(dbg, updatemem);
}

/* Breakpoint manipulation */

/* Convert debugger type/mode into a core breakpoint type (core
   breakpoints have only a single access type, for efficiency, so a
   single TilemDebugBreakpoint may correspond to up to 3 core
   breakpoints) */
static int get_core_bp_type(int type, int mode)
{
	switch (type) {
	case TILEM_DB_BREAK_LOGICAL:
		switch (mode) {
		case TILEM_DB_BREAK_READ:
			return TILEM_BREAK_MEM_READ;
		case TILEM_DB_BREAK_WRITE:
			return TILEM_BREAK_MEM_WRITE;
		case TILEM_DB_BREAK_EXEC:
			return TILEM_BREAK_MEM_EXEC;
		}
		break;

	case TILEM_DB_BREAK_PHYSICAL:
		switch (mode) {
		case TILEM_DB_BREAK_READ:
			return TILEM_BREAK_MEM_READ | TILEM_BREAK_PHYSICAL;
		case TILEM_DB_BREAK_WRITE:
			return TILEM_BREAK_MEM_WRITE | TILEM_BREAK_PHYSICAL;
		case TILEM_DB_BREAK_EXEC:
			return TILEM_BREAK_MEM_EXEC | TILEM_BREAK_PHYSICAL;
		}
		break;

	case TILEM_DB_BREAK_PORT:
		switch (mode) {
		case TILEM_DB_BREAK_READ:
			return TILEM_BREAK_PORT_READ;
		case TILEM_DB_BREAK_WRITE:
			return TILEM_BREAK_PORT_WRITE;
		}
		break;

	case TILEM_DB_BREAK_OPCODE:
		return TILEM_BREAK_EXECUTE;
	}

	g_return_val_if_reached(0);
}

/* Install core breakpoint(s) */
static void set_bp(TilemDebugger *dbg, TilemDebugBreakpoint *bp)
{
	int i, t, n;

	tilem_calc_emulator_lock(dbg->emu);
	for (i = 0; i < 3; i++) {
		if (!bp->disabled && (bp->mode & (1 << i))) {
			t = get_core_bp_type(bp->type, (1 << i));
			n = tilem_z80_add_breakpoint(dbg->emu->calc, t,
			                             bp->start, bp->end,
			                             bp->mask,
			                             NULL, NULL);
			bp->id[i] = n;
		}
		else {
			bp->id[i] = 0;
		}
	}
	tilem_calc_emulator_unlock(dbg->emu);
}

/* Remove core breakpoint(s) */
static void unset_bp(TilemDebugger *dbg, TilemDebugBreakpoint *bp)
{
	int i;
	tilem_calc_emulator_lock(dbg->emu);
	for (i = 0; i < 3; i++) {
		if (bp->id[i])
			tilem_z80_remove_breakpoint(dbg->emu->calc, bp->id[i]);
		bp->id[i] = 0;
	}
	tilem_calc_emulator_unlock(dbg->emu);
}

static gboolean is_mem_exec_bp(const TilemDebugBreakpoint *bp)
{
	return ((bp->type == TILEM_DB_BREAK_LOGICAL
	         || bp->type == TILEM_DB_BREAK_PHYSICAL)
	        && (bp->mode & TILEM_DB_BREAK_EXEC));
}

/* Add a new debugger breakpoint */
TilemDebugBreakpoint * tilem_debugger_add_breakpoint(TilemDebugger *dbg,
                                                     const TilemDebugBreakpoint *bp)
{
	TilemDebugBreakpoint *bp2;

	g_return_val_if_fail(dbg != NULL, NULL);
	g_return_val_if_fail(bp != NULL, NULL);
	g_return_val_if_fail(bp->mode != 0, NULL);

	bp2 = g_slice_new(TilemDebugBreakpoint);
	*bp2 = *bp;
	dbg->breakpoints = g_slist_append(dbg->breakpoints, bp2);
	set_bp(dbg, bp2);

	if (is_mem_exec_bp(bp) && dbg->disasm_view)
		tilem_disasm_view_refresh(TILEM_DISASM_VIEW(dbg->disasm_view));

	return bp2;
}

/* Remove a debugger breakpoint */
void tilem_debugger_remove_breakpoint(TilemDebugger *dbg,
                                      TilemDebugBreakpoint *bp)
{
	gboolean isexec;

	g_return_if_fail(dbg != NULL);
	g_return_if_fail(bp != NULL);
	g_return_if_fail(g_slist_index(dbg->breakpoints, bp) != -1);

	isexec = is_mem_exec_bp(bp);

	unset_bp(dbg, bp);
	dbg->breakpoints = g_slist_remove(dbg->breakpoints, bp);
	g_slice_free(TilemDebugBreakpoint, bp);

	if (isexec && dbg->disasm_view)
		tilem_disasm_view_refresh(TILEM_DISASM_VIEW(dbg->disasm_view));
}

/* Modify a debugger breakpoint */
void tilem_debugger_change_breakpoint(TilemDebugger *dbg,
                                      TilemDebugBreakpoint *bp,
                                      const TilemDebugBreakpoint *newbp)
{
	gboolean isexec;

	g_return_if_fail(dbg != NULL);
	g_return_if_fail(bp != NULL);
	g_return_if_fail(newbp != NULL);
	g_return_if_fail(g_slist_index(dbg->breakpoints, bp) != -1);

	isexec = (is_mem_exec_bp(bp) || is_mem_exec_bp(newbp));

	unset_bp(dbg, bp);
	*bp = *newbp;
	set_bp(dbg, bp);

	if (isexec && dbg->disasm_view)
		tilem_disasm_view_refresh(TILEM_DISASM_VIEW(dbg->disasm_view));
}
