/*
 * TilEm II
 *
 * Copyright (c) 2011-2012 Benjamin Moody
 * Copyright (c) 2011 Duponchelle Thibault
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

/* Key binding */
typedef struct _TilemKeyBinding {
	unsigned int keysym;     /* host keysym value */
	unsigned int modifiers;  /* modifier mask */
	int nscancodes;          /* number of calculator scancodes */
	byte *scancodes;         /* calculator scancodes */
} TilemKeyBinding;

/* A single action */
typedef struct _TilemMacroAtom {
	char* value;
	int type;
} TilemMacroAtom;

/* All the actions */
typedef struct _TilemMacro {
	TilemMacroAtom** actions;
	int n;
} TilemMacro;



typedef struct _TilemCalcEmulator {
	GThread *z80_thread;

	/* Mutex controlling access to the calc.  Use
	   tilem_calc_emulator_lock()/unlock() rather than
	   g_mutex_lock()/unlock() directly. */
	GMutex *calc_mutex;
	int calc_lock_waiting;

	GCond *calc_wakeup_cond;
	TilemCalc *calc;
	gboolean paused;
	gboolean exiting;
	gboolean limit_speed;   /* limit to actual speed */

	/* Timer used for speed limiting */
	GTimer *timer;
	gulong timevalue;
	int high_res_time;

	/* External link cable */
	CableHandle *ext_cable;
	CableOptions ext_cable_options;
	gboolean ext_cable_raw_mode;
	byte ext_cable_raw_in;
	byte ext_cable_raw_out;
	int ext_cable_in;
	int ext_cable_out;
	gboolean ext_cable_changed;

	/* Queue of tasks to be performed */
	GQueue *task_queue;
	gboolean task_busy;
	gboolean task_abort;
	GCond *task_finished_cond;

	/* Sequence of keys to be pressed */
	byte *key_queue;
	int key_queue_len;
	int key_queue_timer;
	int key_queue_pressed;
	int key_queue_cur;
	int key_queue_hold;

	GMutex *lcd_mutex;
	TilemLCDBuffer *lcd_buffer;
	TilemLCDBuffer *tmp_lcd_buffer;
	TilemGrayLCD *glcd;
	gboolean grayscale;
	gboolean lcd_update_pending;

	gboolean anim_grayscale; /* use grayscale in animation */

	char *rom_file_name;
	char *state_file_name;

	/* List of key bindings */
	int nkeybindings;
	
	struct _TilemMacro *macro;

	/* Link transfer state */
	gboolean ilp_active;
	CalcUpdate *link_update; /* CalcUpdate (status and callbacks for ticalcs) */
	GMutex *pbar_mutex;
	char *pbar_title;
	char *pbar_status;
	gdouble pbar_progress;
	gboolean pbar_update_pending;
	gboolean progress_changed;

	/* GUI widgets */
	struct _TilemDebugger *dbg;
	struct _TilemEmulatorWindow *ewin;
	struct _TilemScreenshotDialog *ssdlg;
	struct _TilemReceiveDialog *rcvdlg;
	struct _TilemLinkProgress *linkpb;
	

	FILE * macro_file;	/* The macro file */
	gboolean isMacroRecording; /* A flag to know everywhere that macro is recording */

} TilemCalcEmulator;

/* Errors */
#define TILEM_EMULATOR_ERROR g_quark_from_static_string("tilem-emulator-error")
enum {
	TILEM_EMULATOR_ERROR_NO_ROM,
	TILEM_EMULATOR_ERROR_INVALID_ROM,
	TILEM_EMULATOR_ERROR_INVALID_STATE
};

/* Create a new TilemCalcEmulator. */
TilemCalcEmulator *tilem_calc_emulator_new(void);

/* Free a TilemCalcEmulator. */
void tilem_calc_emulator_free(TilemCalcEmulator *emu);

/* Lock calculator so we can directly access it from outside the core
   thread. */
void tilem_calc_emulator_lock(TilemCalcEmulator *emu);

/* Unlock calculator and allow emulation to continue. */
void tilem_calc_emulator_unlock(TilemCalcEmulator *emu);

/* Load the calculator state from the given ROM file (and accompanying
   sav file, if any.) */
gboolean tilem_calc_emulator_load_state(TilemCalcEmulator *emu,
                                        const char *romfname,
                                        const char *statefname,
                                        int model, GError **err);

/* Reload the calculator state from the most recently loaded file. */
gboolean tilem_calc_emulator_revert_state(TilemCalcEmulator *emu,
                                          GError **err);

/* Save the calculator state. */
gboolean tilem_calc_emulator_save_state(TilemCalcEmulator *emu,
                                        GError **err);

/* Reset the calculator. */
void tilem_calc_emulator_reset(TilemCalcEmulator *emu);

/* Pause emulation (if currently running.) */
void tilem_calc_emulator_pause(TilemCalcEmulator *emu);

/* Resume emulation (if currently paused.) */
void tilem_calc_emulator_run(TilemCalcEmulator *emu);

/* Enable/disable speed limiting (TRUE means attempt to run at the
   actual CPU speed; FALSE means run as fast as we can.) */
void tilem_calc_emulator_set_limit_speed(TilemCalcEmulator *emu,
                                         gboolean limit);

/* Enable/disable grayscale */
void tilem_calc_emulator_set_grayscale(TilemCalcEmulator *emu,
                                       gboolean grayscale);

/* Enable/disable audio output */
void tilem_calc_emulator_set_audio(TilemCalcEmulator *emu,
                                   gboolean enable);

/* Set audio volume */
void tilem_calc_emulator_set_audio_volume(TilemCalcEmulator *emu,
                                          double volume);

/* Set audio options */
void tilem_calc_emulator_set_audio_options(TilemCalcEmulator *emu,
                                           const TilemAudioOptions *options);

/* Select an external link cable (or change cable settings.) */
void tilem_calc_emulator_set_link_cable(TilemCalcEmulator *emu,
                                        const CableOptions *options);

/* Press a single key. */
void tilem_calc_emulator_press_key(TilemCalcEmulator *emu, int key);

/* Release a single key. */
void tilem_calc_emulator_release_key(TilemCalcEmulator *emu, int key);

/* Add keys to the input queue. */
void tilem_calc_emulator_queue_keys(TilemCalcEmulator *emu,
                                    const byte *keys, int nkeys);

/* Release final key in input queue. */
void tilem_calc_emulator_release_queued_key(TilemCalcEmulator *emu);

/* If input queue is empty, press key immediately; otherwise, add to
   the input queue.  Return TRUE if key was added to the queue. */
gboolean tilem_calc_emulator_press_or_queue(TilemCalcEmulator *emu, int key);

/* Retrieve a static screenshot of current calculator screen.
   Returned object has a reference count of 1 (free it with
   g_object_unref().) */
TilemAnimation * tilem_calc_emulator_get_screenshot(TilemCalcEmulator *emu,
                                                    gboolean grayscale);

/* Begin recording an animated screenshot. */
void tilem_calc_emulator_begin_animation(TilemCalcEmulator *emu,
                                         gboolean grayscale);

/* Finish recording an animated screenshot.  Returned object has a
   reference count of 1 (free it with g_object_unref().) */
TilemAnimation * tilem_calc_emulator_end_animation(TilemCalcEmulator *emu);

/* Prompt for a ROM file to open */
int tilem_calc_emulator_prompt_open_rom(TilemCalcEmulator *emu);


/* Run slowly to play macro */
void run_with_key_slowly(TilemCalc* calc, int key);

int sync_clock_tilem(TilemCalcEmulator *emu);


/* Task handling */
 
typedef gboolean (*TilemTaskMainFunc)(TilemCalcEmulator *emu, gpointer data);
typedef void (*TilemTaskFinishedFunc)(TilemCalcEmulator *emu, gpointer data,
                                      gboolean cancelled);
 
/* Add a task to the queue.  MAINF is a function to perform in the
   core thread.  If it returns FALSE, all further tasks will be
   cancelled.  Tasks can also be cancelled early by calling
   tilem_calc_emulator_cancel_tasks().

   After the task finishes or is cancelled, FINISHEDF will be called
   in the GUI thread.  Task-finished functions might not be called in
   the same order the tasks were originally added to the queue. */
void tilem_calc_emulator_begin(TilemCalcEmulator *emu,
                               TilemTaskMainFunc taskf,
                               TilemTaskFinishedFunc finishedf,
                               gpointer data);

/* Cancel all pending tasks.  If a task is currently running, this
   will attempt to cancel it and wait for it to exit. */
void tilem_calc_emulator_cancel_tasks(TilemCalcEmulator *emu);


/* Macros */

/* Start to record a macro */
void tilem_macro_start(TilemCalcEmulator *emu);


/* Add an action to the macro */
void tilem_macro_add_action(TilemMacro* macro, int type, char * value);


/* Stop recording a macro */
void tilem_macro_stop(TilemCalcEmulator *emu);


/* Print the macro (debug) */
void tilem_macro_print(TilemMacro *macro);


/* Write a macro file */
void tilem_macro_write_file(TilemCalcEmulator *emu);


/* Play a macro (loaded or recorded before) */
void tilem_macro_play(TilemCalcEmulator *emu);


/* Load a macro from filename or if filename == NULL prompt user */
void tilem_macro_load(TilemCalcEmulator *emu, char* filename);

