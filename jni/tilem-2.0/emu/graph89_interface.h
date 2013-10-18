#ifndef _TILEM_G89_H
#define _TILEM_G89_H

#include <glib.h>
#include <ticalcs.h>
#include "tilemint.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GETTEXT_PACKAGE "tilem2"

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(str) gettext((str))
# define N_(str) (str)
# define _n(sg, pl, ct) ngettext((sg), (pl), (ct))
#else
# define _(str) (str)
# define N_(str) (str)
# define _n(sg, pl, ct) (((ct) == 1) ? (sg) : (pl))
#endif


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


struct TilemSendFileInfo {
	char *filename;
	char *display_name;
	int slot;
	int first;
	int last;
	char *error_message;
};

/* This structure is used to receive a file */
struct TilemReceiveFileInfo {
	GSList *entries;
	char* destination;
	char *error_message;
	gboolean output_tig;
};

typedef struct {
	int model;

	VarEntry *ve;   	/* Original variable info retrieved
	                	   from calculator */
	int slot;       	/* Slot number */

	/* Strings for display (UTF-8) */
	char *name_str; 	/* Variable name */
	char *type_str; 	/* Variable type */
	char *slot_str; 	/* Program slot */
	char *file_ext; 	/* Default file extension */
	char *filetype_desc; 	/* File format description */

	int size;            	/* Variable size */
	gboolean archived;   	/* Is archived */
	gboolean can_group;  	/* Can be stored in group file */

} TilemVarEntry;



#ifdef __cplusplus
}
#endif

#endif
