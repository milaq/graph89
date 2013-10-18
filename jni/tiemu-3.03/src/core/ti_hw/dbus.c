/* Hey EMACS -*- linux-c -*- */
/* $Id: dbus.c 2839 2009-05-08 19:50:07Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Li�vin
 *  Copyright (c) 2005-2007, Romain Li�vin, Kevin Kofler
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
 * Modified to run in Android OS. Dritan Hashorva 2012
 */



/*
 Linkport management (D-bus serial input/output)
 */

#include <jni.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>

#include "libuae.h"
#include "ports.h"
#include "dbus.h"
#include "m68k.h"
#include "engine.h"
#include "gettimeofday.h"

#include "tilibs.h"
#include "ti68k_err.h"
#include "ti68k_def.h"
#include "ti68k_int.h"
#include <tie_error.h>
#include <androidlog.h>

CableHandle* cable_handle = NULL;
CalcHandle* calc_handle = NULL;

JNIEnv * DbusJNIenv = NULL;

/*
 Linkport (lp) / directfile (df) / void (vd) mappers
 */

void (*hw_dbus_reinit)(void);
void (*hw_dbus_putbyte)(uint8_t arg);	// TI -> outside
uint8_t (*hw_dbus_getbyte)(void);			// outside -> TI
int (*hw_dbus_checkread)(void);

static void lp_reinit(void);
static void lp_putbyte(uint8_t arg);
static uint8_t lp_getbyte(void);
static int lp_checkread(void);

static void df_reinit(void);
static void df_putbyte(uint8_t arg);
static uint8_t df_getbyte(void);
static int df_checkread(void);

static void vd_reinit(void);
static void vd_putbyte(uint8_t arg);
static uint8_t vd_getbyte(void);
static int vd_checkread(void);

static void map_dbus_to_cable(void) {
	hw_dbus_reinit = lp_reinit;
	hw_dbus_putbyte = lp_putbyte;
	hw_dbus_getbyte = lp_getbyte;
	hw_dbus_checkread = lp_checkread;
}

static void map_dbus_to_file(void) {
	hw_dbus_reinit = df_reinit;
	hw_dbus_putbyte = df_putbyte;
	hw_dbus_getbyte = df_getbyte;
	hw_dbus_checkread = df_checkread;
}

static void map_dbus_to_void(void) {
	hw_dbus_reinit = vd_reinit;
	hw_dbus_putbyte = vd_putbyte;
	hw_dbus_getbyte = vd_getbyte;
	hw_dbus_checkread = vd_checkread;
}

/*
 D-bus management (HW linkport)
 */

static int dbus_set = 0;

int ilp_reset(CableHandle *h);
int ilp_send(CableHandle *h, uint8_t *data, uint32_t len);
int ilp_recv(CableHandle *h, uint8_t *data, uint32_t len);

int hw_dbus_init(void) {
	int err;

	// exit has to be called before init
	if (dbus_set)
		return 0;

	// don't let linkport function pointers uninitialized
	map_dbus_to_void();

	// set cable
	cable_handle = ticables_handle_new(linkp.cable_model, linkp.cable_port);
	if (cable_handle == NULL ) {
		tiemu_err(0, "Can't set cable");
		return -1;
	}

	ticables_options_set_timeout(cable_handle, linkp.cable_timeout);
	ticables_options_set_delay(cable_handle, linkp.cable_delay);

	// set calc
	linkp.calc_model = ti68k_calc_to_libti_calc();
	calc_handle = ticalcs_handle_new(linkp.calc_model);
	if (calc_handle == NULL ) {
		tiemu_err(0, "Can't set cable");
		return -1;
	}

	// attach cable to calc (open cable)
	err = ticalcs_cable_attach(calc_handle, cable_handle);
	if (err) {
		ticalcs_handle_del(calc_handle);
		ticables_handle_del(cable_handle);
		tiemu_err(err, NULL );
		return -1;
	}

	// customize cable by overriding some methods
	if (linkp.cable_model == CABLE_ILP) {
		cable_handle->cable->reset = ilp_reset;
		cable_handle->cable->send = ilp_send;
		cable_handle->cable->recv = ilp_recv;
	}

	// and set pointers
	if (linkp.cable_model == CABLE_ILP)
		map_dbus_to_file();		// set mappers to internal file loader
	else
		map_dbus_to_cable();	// set mappers to external link cable

	dbus_set = !0;

	return 0;
}

int hw_dbus_reset(void) {
	// has to be init'ed first
	if (!dbus_set)
		return 0;

	hw_dbus_reinit();

	return 0;
}

int hw_dbus_exit(void) {
	int err;

	// init has to be called before exit
	if (!dbus_set)
		return 0;

	// don't let linkport function pointers uninitialized
	map_dbus_to_void();

	// detach cable from calc (close cable)
	err = (calc_handle ? ticalcs_cable_detach(calc_handle) : 0);
	if (err) {
		tiemu_err(err, NULL );
		return -1;
	}

	// delete calc & cable handles
	if (calc_handle)
		ticalcs_handle_del(calc_handle);
	if (cable_handle)
		ticables_handle_del(cable_handle);

	dbus_set = 0;

	return 0;
}

/*
 Void cable access
 */

static void vd_reinit(void) {
}
;
static void vd_putbyte(uint8_t arg) {
}
;
static uint8_t vd_getbyte(void) {
	return 0;
}
;
static int vd_checkread(void) {
	return 0;
}
;

/*
 Link cable access
 */

static int avail = 0;

static void lp_reinit(void) {
	int err;

	avail = 0;
	err = ticables_cable_reset(cable_handle);
	if (err)
		tiemu_err(err, NULL );
}

static void lp_putbyte(uint8_t arg) {
	int err;

	err = ticables_cable_put(cable_handle, arg);
	if (err) {
		io_bit_set(0x0d, 7);
		// error
		printf("lp_putbyte error !\n");
		return;
	}

	io_bit_set(0x0d, 6);
	// tx reg empty
	io_bit_set(0x0d, 2);
	// link activity
}

static uint8_t lp_getbyte(void) {
	int err;
	uint8_t arg;

	if (!avail) {
		io_bit_set(0x0d, 7);
		printf("lp_getbyte (byte lost) !\n");
	}

	err = ticables_cable_get(cable_handle, &arg);
	if (err) {
		io_bit_set(0x0d, 7);
		// error
		printf("lp_getbyte error !\n");
		return 0x00;
	}

	avail = 0;
	return arg;
}

static int lp_checkread(void) {
	int err = 0;
	CableStatus status = 0;

	if (avail)
		return 0;

	err = ticables_cable_check(cable_handle, &status);
	if (err) {
		io_bit_set(0x0d, 7);
		// error
		printf("lp_checkread error !\n");
		return 0x00;
	}

	if (status & STATUS_RX) {
		io_bit_set(0x0d, 5);
		// rx reg full
		io_bit_set(0x0d, 2);
		// link activity
		avail = !0;
	}

	return avail;
}

/*
 Link file access
 */

static int sip = 0;	// sending in progress
static int rip = 0;	// receive in progress

int t2f_data;   // ti => file data
int t2f_flag;   // data available

int f2t_data;   // file => ti data
int f2t_flag;   // data available

int recfile_flag; // receive file at end of instruction

void df_reinit(void) {
	t2f_flag = f2t_flag = 0;
}

void df_putbyte(uint8_t arg) {
	t2f_data = arg;
	t2f_flag = 1;

	if (!sip) {
		if (params.recv_file)
			recfile_flag = 1;
		else
			io_bit_set(0x0d, 7);
		// SLE=1: error
	}
}

uint8_t df_getbyte(void) {
	f2t_flag = 0;
	return f2t_data;
}

int df_checkread(void) {
	if (f2t_flag) {
		io_bit_set(0x0d, 5);
		// rx reg full
	}

	return f2t_flag;
}

/*
 Internal link port emulation for direct sending/receiving files.
 The code below is not very obvious at first glance. This is the
 reason why I am explaining the idea and mechanisms.

 The idea is to use TiLP's libticalcs since libticalcs contains all the code
 needed for communicating with a TI (software protocol, file handling).
 The libticalcs is built on libticables, a library which handles link cables
 (hardware protocol).
 In fact, I simply reimplement the basic functions of libticables usually
 used by libticalcs for sending/receiving data. These functions exchange
 bytes with the linkport at HW level (ports.c).
 The libticalcs provides the abstraction we need for this.

 Wonderful, isn't it ?! Take a look at the 'TiLP framework' power ;-)
 */

/* libticables functions (link API) */

int ilp_reset(CableHandle *h) {
	return t2f_flag = f2t_flag = 0;
}

int ilp_send(CableHandle *h, uint8_t *data, uint32_t len) {
	unsigned int i;
	tiTIME clk;

	for (i = 0; i < len; i++) {
		f2t_data = data[i];
		f2t_flag = 1;

		io_bit_set(0x0d, 5);
		// SRX=1 (rx reg is full)
		hw_m68k_irq(4);		// this turbo-boost transfer !

		TO_START(clk);
		while (f2t_flag) {
			hw_m68k_run(1);
			//if(TO_ELAPSED(clk, linkp.cable_timeout))
			//	return ERROR_WRITE_TIMEOUT;
		};
	}

	return 0;
}

int ilp_recv(CableHandle *h, uint8_t *data, uint32_t len) {
	unsigned int i;
	tiTIME clk;

	for (i = 0; i < len; i++) {
		TO_START(clk);
		while (!t2f_flag) {
			hw_m68k_run(1);
			if (TO_ELAPSED(clk, linkp.cable_timeout*2))
				return ERROR_WRITE_TIMEOUT;
		};

		data[i] = t2f_data;
		t2f_flag = 0;

		io_bit_set(0x0d, 6);
		// STX=1 (tx reg is empty)
		hw_m68k_irq(4);		// this turbo-boost transfer (30% faster)
	}

	return 0;
}

static int do_cpu(int duration)	// thenth of seconds
{
	tiTIME clk;
	TO_START(clk);
	while (1) {
		hw_m68k_run(1);
		if (TO_ELAPSED(clk, duration))
			return !0;;
	};
	return 0;
}

int sync_clock() {
	time_t tt;
	struct tm *lt;
	CalcClock tc;
	int err;

	sip = 1;

	err = ticalcs_calc_isready(calc_handle);
	if (err) {
		goto end;
	}

	time(&tt);
	lt = localtime(&tt);
	tc.day = lt->tm_mday;
	tc.month = lt->tm_mon + 1;
	tc.year = lt->tm_year + 1900;
	tc.hours = lt->tm_hour;
	tc.minutes = lt->tm_min;
	tc.seconds = lt->tm_sec;

	err = ticalcs_calc_set_clock(calc_handle, &tc);
	if (err) {
		goto end;
	}
	end: sip = 0;
}

int send_ti_file(const char *filename) {
	gint ok = 0;
	int ret;
	struct timeval start, finish;
	double duration;

	// Check for TI file
	if (!tifiles_file_is_ti(filename))
		return ERR_NOT_TI_FILE;

	if (((tifiles_file_get_model(filename) == CALC_TI92))
			|| (tifiles_file_get_model(filename) == CALC_TI89)
			|| (tifiles_file_get_model(filename) == CALC_TI89T)
			|| (tifiles_file_get_model(filename) == CALC_TI92P)
			|| (tifiles_file_get_model(filename) == CALC_V200)
			|| (tifiles_file_is_tigroup(filename))) {
		ok = 1;
	} else
		return ERR_NOT_TI_FILE;

	// Use direct file loading
	gettimeofday(&start, NULL );
	sip = 1;

	// Check whether calc is ready... Otherwise, goes to HOME.
	ret = ticalcs_calc_isready(calc_handle);

	if (ret == 257) /* ERR_NOT_READY = 257 */
	{
		switch (tihw.calc_type) {
		case TI89:
		case TI89t:
			ret = ticalcs_calc_send_key(calc_handle, 277);
			break;
		case TI92:
		case TI92p:
		case V200:
			ret = ticalcs_calc_send_key(calc_handle, 8273);
			break;

		default:
			break;
		}

		do_cpu(5);

		if (ret) {
			sip = 0;
			goto send_ti_file_exit;
		}
	}

	// FLASH APP file ?
	if (tifiles_file_is_app(filename)) {
		// increase timeout due to excessive time for last ACK
		linkp.cable_timeout *= 10;
		ret = ticalcs_calc_send_app2(calc_handle, filename);
		linkp.cable_timeout /= 10;
	}

	// FLASH OS file ?
	if (tifiles_file_test(filename, TIFILE_OS, calc_handle->model)) {
		ret = ticalcs_calc_send_app2(calc_handle, filename);
	}

	// Group file ?
	else if (tifiles_file_is_group(filename)) {
		ret = ticalcs_calc_send_var2(calc_handle, MODE_NORMAL, filename);
	}

	// Backup file ?
	else if (tifiles_file_is_backup(filename)) {
		ret = ticalcs_calc_send_backup2(calc_handle, filename);
	}

	// Single file
	else if (tifiles_file_is_single(filename)) {
		ret = ticalcs_calc_send_var2(calc_handle, MODE_NORMAL, filename);
	}

	// TiGroup file
	else if (tifiles_file_is_tigroup(filename)) {
		// increase timeout due to excessive time for last ACK (file may contains apps)
		linkp.cable_timeout *= 10;
		ret = ticalcs_calc_send_tigroup2(calc_handle, filename, TIG_ALL);
		linkp.cable_timeout /= 10;
	}

	// Restore link cable use
	sip = 0;
	gettimeofday(&finish, NULL );
	duration = (double) (finish.tv_sec - start.tv_sec)
			+ (double) (finish.tv_usec - start.tv_usec) / 1000000.;
	printf("Duration: %2.1lf seconds.\n", duration);

	// Transfer aborted ? Set hw link error
	send_ti_file_exit: if (ret) {
		tiemu_err(ret, "error in send_ti_file");
		io_bit_set(0x0d, 7);
		// SLE=1
		df_reinit();
	}

	return 0;
}

int ti68k_linkport_ready(void) {
	int ret;
	int pedrom;

	// Some OS versions lock up if they get a readiness probe too early in the boot cycle.
	hw_m68k_run(1250000);

	// Hack: PedroM doesn't like resetting the link flag here, so handle it specially.
	//       Test this now because this might not be set yet if the OS hasn't booted up yet.
	pedrom = (mem_rd_word(0x32) == (('R' << 8) + 'O'));

	// Block sending and receiving.
	if (sip || rip)
		return 0;
	sip = 1;
	ret = ticalcs_calc_isready(calc_handle);
/*
	if (ret) {
		if (!pedrom)
			io_bit_set(0x0d, 7);
		// SLE=1
		df_reinit();

		// Try getting the calculator ready.
		// FIXME: This won't work immediately, it will at first return FALSE a few times and you'll have to retry.
		// However, it should be enough for KTIGCC to work.
		engine_stop();
		hw_m68k_irq(6);
		while (ti68k_debug_is_supervisor())
			hw_m68k_run(1);
		engine_start();

		ti68k_kbd_push_chars("\f");

		hw_m68k_run(1250000);

		ret = ticalcs_calc_isready(calc_handle);
	}*/

	if (ret) {
		if (!pedrom)
			io_bit_set(0x0d, 7);
		// SLE=1
		df_reinit();
	}

	sip = 0;
	return !ret;
}

int recfile(void) {
	int ret;
	char src_fn[1024];
	char dst_fn[1024];
	VarEntry *ve;

	if (TMP_DIR == NULL )
		return 0;

	recfile_flag = 0;

	// Make this function not re-entrant
	if (rip)
		return 0;
	else
		rip = 1;

	// Some models and AMS versions send an RDY packet when entering in
	// the VAR-Link menu or just before sending variable. We skip it !
	if (t2f_data == 0x89 && tihw.calc_type != TI92) {
		uint8_t arg[4];
		int i;

		ilp_recv(cable_handle, arg, 4);
		for (i = 0; i < 4; i++)
			printf("purging <%02x>\n", arg[i]);

		if (tihw.calc_type != TI89t)
			goto recfile_end;
	}

	// Receive variable in non-silent mode
	strcpy(src_fn, TMP_DIR);
	strcat(src_fn, "/");
	strcat(src_fn, "file.rec");

	ret = ticalcs_calc_recv_var_ns2(calc_handle, MODE_NORMAL, src_fn, &ve);

	// Check for error
	if (ret) {
		io_bit_set(0x0d, 7);
		// SLE=1
		t2f_flag = f2t_flag = 0;

		tiemu_err(ret, NULL );
		goto recfile_end;
	}

	// Construct filename
	strcpy(dst_fn, "");
	//strcat(dst_fn, "/");

	if (ve) {
		char *basename;

		//single
		basename = ticonv_varname_to_filename(calc_handle->model, ve->name,
				ve->type);
		strcat(dst_fn, basename);
		strcat(dst_fn, ".");
		strcat(dst_fn, tifiles_vartype2fext(calc_handle->model, ve->type));

		tifiles_ve_delete(ve);
	} else {
		// group
		strcat(dst_fn, "group.");
		strcat(dst_fn, tifiles_fext_of_group(linkp.calc_model));
	}

	if (DbusJNIenv != NULL )
	{
		jstring jSrc = (*DbusJNIenv)->NewStringUTF(DbusJNIenv, src_fn);
		jstring jDst = (*DbusJNIenv)->NewStringUTF(DbusJNIenv, dst_fn);

		jclass class = (*DbusJNIenv)->FindClass(DbusJNIenv,
				"com/graph89/emulationcore/TIEmuThread");
		jmethodID method = (*DbusJNIenv)->GetStaticMethodID(DbusJNIenv, class, "ReceiveFile",
				"(Ljava/lang/String;Ljava/lang/String;)V");

		(*DbusJNIenv)->CallStaticIntMethod(DbusJNIenv, class, method, jSrc, jDst);
	}

	recfile_end: rip = 0;

	return 0;
}
