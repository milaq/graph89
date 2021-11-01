/*
 * libtilemcore - Graphing calculator emulation library
 *
 * Copyright (c) 2012 Benjamin Moody
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "tilem.h"
#include "gettext.h"

#ifndef HAVE_LROUND
# define lround(x) ((long) floor((x) + 0.5))
#endif

/*

 OVERVIEW

   This file contains the audio filter, which monitors the state of
   the link port and produces a sequence of audio samples in the
   format specified by the application.

   In general, the API works as follows:

    - Call tilem_audio_filter_new() to create a TilemAudioFilter.

    - Set its properties to match the output device:
        tilem_audio_filter_set_rate()
        tilem_audio_filter_set_channels()
        tilem_audio_filter_set_format()

    - Allocate an appropriately-sized sample buffer and attach it to
      the filter using tilem_audio_filter_set_buffer().

    - Begin filling the buffer by calling tilem_audio_filter_on().

    - Repeat:

      - Wait for the buffer to be filled, either using a callback
        (tilem_audio_filter_set_callback) or by waiting for
        tilem_z80_run() to return TILEM_STOP_AUDIO_BUFFER.

      - Once the buffer is filled, ship it off to the sound card.

      - Wait for the sound card to catch up (exactly what this means
        will depend on the API being used, and on your desired
        latency.)

      - Allocate a new buffer (or, more likely, recycle one that was
        previously used), call tilem_audio_filter_set_buffer() again,
        and repeat.

    - When finished:

      - Call tilem_audio_filter_off() to stop filling the buffer.

      - Call tilem_audio_filter_flush() to clear the remainder of the
        buffer, and send that to the sound card as well.

   Note that whenever the output buffer is filled, you must provide a
   new output buffer, by calling tilem_audio_filter_set_buffer(),
   before resuming emulation.  This can be done within the callback
   function if desired.

   Remember that you will want to disable any existing speed
   regulation (e.g., using the system clock or usleep()) while playing
   audio.  The sound card is the only clock that should be used.


 IMPLEMENTATION

   The input to the audio filter may be thought of as a one-bit signal
   sampled at the calculator CPU's clock speed.  (Because the clock
   speed can change, we actually start out by translating clock ticks
   into "subticks" - currently 1/30 microsecond, which happens to be
   the gcd of 1/6 and 1/15.)  We need to apply a low-pass filter to
   this signal before resampling it to the rate needed by the output
   device.

   The output of a general FIR filter at time t is given by

     y_t  =  a_0 x_t + a_1 x_{t-1} + a_2 x_{t-2} + ... + a_n x_{t-n}

   where x_i are the input samples and a_i are constants.  In this
   case, however, two observations allow us to optimize the
   calculation:

     - Every x_t is either zero or one; and

     - x_t is almost always equal to x_{t+1}.  In fact, our input
       isn't really a sequence of x_t -- rather, our input is a
       sequence of t-values at which the value of x changes.

   Instead of actually computing the value of x_t for every t,
   multiplying and adding the appropriate coefficients, we can
   pre-compute k_i, the sum of a_0 ... a_i, for every i between 0 and
   n.  Then, the value of y_t is obtained by adding the k_i for each
   point in time when x changes from 0 to 1, and subtracting the k_i
   for each point in time when x changes from 1 to 0.

   Note that because n (the size of the filter) is much larger than
   the interval between two consecutive output samples, we need to be
   computing many output samples at the same time.  The state for each
   output frame (pair of samples) is stored in a TilemAudioAccum
   structure.


   link_callback() is called when the link port state changes (as an
   I/O breakpoint, so it has single-clock-tick precision.)  It checks
   which of the two lines have changed, and updates the
   TilemAudioAccums as appropriate.

   timer_callback() is called for each output sample.  As a timer, it
   only has roughly-one-microsecond precision.  It converts the
   samples from the oldest TilemAudioAccum into the desired output
   format and stores them in the output buffer.  It then reinitializes
   that TilemAudioAccum to be used for the next frame.

*/

#define SUBTICKRATE 30000000
#define SUBTICKS_PER_MS (SUBTICKRATE / 1000)
#define SUBTICKS_PER_US (SUBTICKRATE / 1000000)

#define DEFAULT_VOLUME 0.5

#define VSHIFT 14

typedef struct _TilemAudioAccum {
	word sample[2];   /* Accumulated samples for L and R channels */
	dword t_offset;   /* Number of sub-ticks from the time the
	                     output sample timer fires to the actual
	                     sample time */
} TilemAudioAccum;

struct _TilemAudioFilter {
	TilemCalc *calc;

	TilemAudioCallbackFunc callback;
	void *callback_data;

	int output_rate;
	int output_channels;
	int output_format;
	double volume;
	int vscale;
	dword dc_offset;

	int sample_tmr;         /* Timer ID */
	int link_bp;            /* Breakpoint ID */

	unsigned old_lines;     /* Previous state of link port */

	int num_accum;          /* Number of sample accumulators */
	int cur_accum;          /* Index of current accumulator */
	TilemAudioAccum *accum;

	/* Array of partial sums of filter coefficients.
	   filter_sum[t] is the sum (mod 65536) of coefficients from 0
	   to t, or in other words, it is the amount by which the
	   output sample should be increased if line remains low for
	   the t subticks preceding the sample time.

	   Output samples are computed by starting with init_low or
	   init_high as appropriate, then adding the filter_sum value
	   for each point when the line changes from high to low, and
	   subtracting the filter_sum value for each point when the
	   line changes from low to high.

	   This array contains padding elements on both ends, because
	   the output timer will not necessarily fire at exactly the
	   time we would like it to.
	*/
	word *filter_sum;       /* Array of partial sums */
	int filter_size;        /* Total size of the filter */

	/* Number of padding values at start/end of filter (retained
	   for debugging) */
	int filter_padnear;
	int filter_padfar;

	/* Initial output values.  The output is scaled so that the
	   smallest possible output value is mapped to -32768 and the
	   largest to 32767 - note that because the filter has some
	   negative coefficients, the smallest possible value is not
	   the same as the value output by a constant zero input.

	   Note that init_low = init_high + filter_sum[filter_size-1]. */
	word init_low;       /* Initial sample value when line low */
	word init_high;      /* Initial sample value when line high */

	int timer_period;       /* Current period of sample timer */
	int timer_period_f;     /* floor(1e6 / fs) */
	int timer_period_c;     /* ceil(1e6 / fs) */
	int timer_err_f;        /* fs * period_f - 1e6  (negative) */
	int timer_err_c;        /* fs * period_c - 1e6  (positive) */
	int timer_err;          /* accumulated error */

	int sample_period_f;    /* floor(SUBTICKRATE / fs) */
	int sample_period_c;    /* ceil(SUBTICKRATE / fs) */
	int sample_err_f;       /* fs * period_f - SUBTICKRATE  (negative) */
	int sample_err_c;       /* fs * period_c - SUBTICKRATE  (positive) */
	int sample_err;         /* accumulated error */

	byte *buffer_start;
	int buffer_size;
	byte *buffer_pos;
	int buffer_remaining;

	int overrun_warned;
};

static const char * format_to_string(int format)
{
	switch (format) {
	case TILEM_AUDIO_U8:     return _("8-bit, unsigned");
	case TILEM_AUDIO_S8:     return _("8-bit, signed");
	case TILEM_AUDIO_U16_LE: return _("16-bit, unsigned, little-endian");
	case TILEM_AUDIO_S16_LE: return _("16-bit, signed, little-endian");
	case TILEM_AUDIO_U16_BE: return _("16-bit, unsigned, big-endian");
	case TILEM_AUDIO_S16_BE: return _("16-bit, signed, big-endian");
	default: return NULL;
	}
}

static void msg(TilemAudioFilter *af, const char *desc, ...)
{
	va_list ap;
	char buf[1024];
	va_start(ap, desc);
	vsprintf(buf, desc, ap);
	va_end(ap);
	tilem_message(af->calc, _("audio: %s"), buf);
}

static void warn(TilemAudioFilter *af, const char *desc, ...)
{
	va_list ap;
	char buf[1024];
	va_start(ap, desc);
	vsprintf(buf, desc, ap);
	va_end(ap);
	tilem_internal(af->calc, _("audio: %s"), buf);
}

/***************** Emulation callbacks *****************/

/* Breakpoint callback (called when program writes to port 0/7) */
static int link_callback(TilemCalc *calc, dword portaddr TILEM_ATTR_UNUSED,
                         void *data)
{
	TilemAudioFilter * restrict af = data;
	TilemAudioAccum * restrict accum;
	const word * restrict filter_sum;
	byte oldstate, newstate;
	word fsum;
	int i, t, st, chan;

	/* if line state hasn't changed, nothing to do */

	oldstate = af->old_lines;
	newstate = calc->linkport.lines;
	if (oldstate == newstate)
		return 0;
	af->old_lines = newstate;

	/* get time relative to output sample timer */

	t = tilem_z80_get_timer_clocks(calc, af->sample_tmr) - 1;
	t = (t * SUBTICKS_PER_MS) / calc->z80.clockspeed;

	accum = af->accum;
	filter_sum = af->filter_sum;

	for (i = 0; i < af->num_accum; i++) {

		/* compute time relative to the (real) time of this sample */
		st = t + accum[i].t_offset;

		/* get filter sum */
		if (TILEM_UNLIKELY(st < 0)) {
			warn(af, "st out of bounds (%d)", st);
			fsum = 0;
		}
		else if (TILEM_UNLIKELY(st >= af->filter_size)) {
			warn(af, "st out of bounds (%d)", st);
			fsum = af->init_low - af->init_high;
		}
		else {
			fsum = filter_sum[st];
		}

		/* if a line has changed from high (inactive) to low
		   (active), add the filter sum to the current sample;
		   if a line has changed from low to high, subtract
		   the filter sum from the sample. */
		for (chan = 0; chan < 2; chan++) {
			if (!((oldstate ^ newstate) & (1 << chan)))
				continue;

			if (newstate & (1 << chan))
				accum[i].sample[chan] += fsum;
			else
				accum[i].sample[chan] -= fsum;
		}
	}

	return 0;
}

static inline int dither(int step)
{
#ifdef DISABLE_AUDIO_DITHERING
	return 0;
#else
	int r1, r2;
	if (step > RAND_MAX) {
		int f = step / ((unsigned) RAND_MAX + 1);
		r1 = rand() * f;
		r2 = rand() * f;
	}
	else {
		int d = ((unsigned) RAND_MAX + 1) / step;
		r1 = (unsigned) rand() / d;
		r2 = (unsigned) rand() / d;
	}
	return (r1 - r2);
#endif
}

/* Convert scaled sample to output format */

static inline byte conv8(dword samp)
{
	samp += dither(1 << (VSHIFT + 8));
	samp >>= VSHIFT + 8;

	if (TILEM_UNLIKELY(samp < 0x180))
		return 0x80;
	else if (TILEM_UNLIKELY(samp > 0x27f))
		return 0x7f;
	else
		return (byte) samp;
}

static inline word conv16(dword samp)
{
	samp += dither(1 << VSHIFT);
	samp >>= VSHIFT;

	if (TILEM_UNLIKELY(samp < 0x18000))
		return 0x8000;
	else if (TILEM_UNLIKELY(samp > 0x27fff))
		return 0x7fff;
	else
		return (word) samp;
}

static inline word byteswap(word v)
{
	return ((v << 8) | (v >> 8));
}

static inline void put_sample(byte * restrict buf, int format, dword samp)
{
	switch (format) {
	case TILEM_AUDIO_U8:
		*buf = conv8(samp) ^ 0x80;
		break;

	case TILEM_AUDIO_S8:
		*buf = conv8(samp);
		break;

	case TILEM_AUDIO_U16:
		* ((word *) buf) = conv16(samp) ^ 0x8000;
		break;

	case TILEM_AUDIO_S16:
		* ((word *) buf) = conv16(samp);
		break;

	case TILEM_AUDIO_U16_SWAP:
		* ((word *) buf) = byteswap(conv16(samp) ^ 0x8000);
		break;

	case TILEM_AUDIO_S16_SWAP:
		* ((word *) buf) = byteswap(conv16(samp));
		break;
	}
}

static void put_frame(TilemAudioFilter *af, word raw_l, word raw_r)
{
	dword l, r;
	int fmt = af->output_format;
	int bps = ((fmt & TILEM_AUDIO_16_BIT) ? 2 : 1);

	if (TILEM_UNLIKELY(af->buffer_remaining == 0)) {
		if (!af->overrun_warned)
			warn(af, _("buffer overrun"));
		af->overrun_warned = 1;
		return;
	}

	l = ((int16_t) raw_l * af->vscale);
	r = ((int16_t) raw_r * af->vscale);

	if (af->output_channels == 1) {
		put_sample(af->buffer_pos, fmt, l + r + af->dc_offset);
		af->buffer_pos += bps;
		af->buffer_remaining -= bps;
	}
	else {
		put_sample(af->buffer_pos, fmt, l + af->dc_offset);
		put_sample(af->buffer_pos + bps, fmt, r + af->dc_offset);
		af->buffer_pos += 2 * bps;
		af->buffer_remaining -= 2 * bps;
	}

	if (af->buffer_remaining == 0) {
		if (af->callback)
			(*af->callback)(af->calc, af,
			                af->buffer_start, af->buffer_size,
			                af->callback_data);
		tilem_z80_stop(af->calc, TILEM_STOP_AUDIO_BUFFER);
	}
}

/*
static void dumpaccum(TilemAudioFilter *af)
{
	int i;
	for (i = 0; i < af->num_accum; i++)
		printf("%8d", af->accum[i].t_offset);
	printf("  (%d, %d)\n", af->timer_err, af->sample_err);
	getchar();
}
*/

/* Timer callback (called at every output sample, plus or minus 1
   microsecond) */
static void timer_callback(TilemCalc *calc, void *data)
{
	TilemAudioFilter * restrict af = data;
	TilemAudioAccum * restrict accum = af->accum;
	byte state;
	int i, t, cur, prev;

	cur = af->cur_accum;
	prev = (cur ? cur - 1 : af->num_accum - 1);

	/*
	t = accum[cur].t_offset;
	if (TILEM_UNLIKELY(t < 0))
		warn(af, "final t_offset too small (%d)", t);
	else if (TILEM_UNLIKELY(t > af->filter_padnear))
		warn(af, "final t_offset too large (%d)", t);
	*/

	/* Adjust t_offsets by the amount that this timer was just
	   moved forward */

	t = (af->timer_period * calc->z80.clockspeed + 500) / 1000;
	t = (t * SUBTICKS_PER_MS) / calc->z80.clockspeed;
	for (i = 0; i < af->num_accum; i++)
		accum[i].t_offset -= t;

	/* Adjust this timer's period to synchronize with output
	   sample rate (needed because output sample rate may not
	   divide 1,000,000) */
	if (af->timer_err > 0) {
		af->timer_period = af->timer_period_f;
		af->timer_err += af->timer_err_f;
	}
	else {
		af->timer_period = af->timer_period_c;
		af->timer_err += af->timer_err_c;
	}
	tilem_z80_set_timer_period(calc, af->sample_tmr, af->timer_period);

	/* Write out the current frame */

	put_frame(af, accum[cur].sample[0], accum[cur].sample[1]);

	/* Initialize the next frame in the pipeline */

	state = af->old_lines;
	if (state & 1)
		accum[cur].sample[0] = af->init_low;
	else
		accum[cur].sample[0] = af->init_high;

	if (state & 2)
		accum[cur].sample[1] = af->init_low;
	else
		accum[cur].sample[1] = af->init_high;

	/* Adjust inter-sample delay (needed because output sample
	   rate may not divide SUBTICKRATE) */
	if (af->sample_err > 0) {
		t = af->sample_period_f;
		af->sample_err += af->sample_err_f;
	}
	else {
		t = af->sample_period_c;
		af->sample_err += af->sample_err_c;
	}

	accum[cur].t_offset = accum[prev].t_offset + t;

	/*
	t = accum[cur].t_offset + (af->timer_period_c * SUBTICKS_PER_US);
	if (TILEM_UNLIKELY(t >= af->filter_size))
		warn(af, "initial t_offset too large (%d)", t);
	else if (TILEM_UNLIKELY(t < af->filter_size - af->filter_padfar - 1))
		warn(af, "initial t_offset too small (%d)", t);
	*/

	if (cur + 1 == af->num_accum)
		af->cur_accum = 0;
	else
		af->cur_accum = cur + 1;
}


/**************** Initialization ****************/

#ifndef M_PI
# define M_PI 3.141592653589793238462643383
#endif

/* Compute filter coefficients (sinc filter with Blackman window) */
static void gen_filter(double *coeff, int length, int cutoff)
{
	int hsize = (length + 1) / 2;
	int i;
	double x, f, w;

	for (i = 0; i < hsize; i++) {
		if (i == 0) {
			f = 1.0;
		}
		else {
			x = i * 2.0 * cutoff / SUBTICKRATE;
			f = sin(x * M_PI) / (x * M_PI);
		}

		w = (0.42 + 0.5 * cos(i * M_PI / hsize)
		     + 0.08 * cos(i * M_PI * 2 / hsize));

		coeff[hsize - 1 - i] = coeff[hsize - 1 + i] = f * w;
	}

	if (!(length % 2)) coeff[length - 1] = 0.0;
}

/* Set up filter and other constant data as necessary for a given
   output sampling frequency */
static void filter_setup(TilemAudioFilter *af, int rate)
{
	/* length of filter = 2 ms
	   cutoff = 2 khz below Nyquist limit
	   more tweaking might be helpful */
	int length = SUBTICKRATE / 500;
	int cutoff = rate / 2 - 2000;
	int padnear, padfar;
	double *coeff;
	double sum_plus, sum_minus, scale, x;
	int i;

	if (rate == af->output_rate)
		return;

	af->output_rate = rate;

	af->timer_period_f = 1000000 / rate;
	af->timer_period_c = (1000000 + rate - 1) / rate;
	af->timer_err_f = rate * af->timer_period_f - 1000000;
	af->timer_err_c = rate * af->timer_period_c - 1000000;

	af->sample_period_f = SUBTICKRATE / rate;
	af->sample_period_c = (SUBTICKRATE + rate - 1) / rate;
	af->sample_err_f = rate * af->sample_period_f - SUBTICKRATE;
	af->sample_err_c = rate * af->sample_period_c - SUBTICKRATE;

	/* Add 1 microsec of padding to allow for imprecision in the
	   sample timer, plus 1 subtick to allow for varying
	   intersample period */
	padnear = SUBTICKS_PER_US + 1;

	/* Number of accumulators = ceil(number of sample intervals in
	   (length+padnear) subticks) */
	af->num_accum = ((((qword) length + padnear) * rate
	                  + SUBTICKRATE - 1)
	                 / SUBTICKRATE);

	/* Padding on far end sufficient so that filter_sum contains
	   valid entries for the earliest possible transitions */
	padfar = padnear + af->num_accum * af->sample_period_c - length;
	af->filter_padnear = padnear;
	af->filter_padfar = padfar;

	tilem_free(af->accum);
	af->accum = tilem_new_atomic(TilemAudioAccum, af->num_accum);

	/* Generate filter */
	coeff = tilem_new_atomic(double, length);
	gen_filter(coeff, length, cutoff);

	/* Add up all positive coefficients to determine the largest
	   value the filter can output, and likewise for negative
	   coefficients. */
	sum_plus = sum_minus = 0.0;
	for (i = 0; i < length; i++) {
		if (coeff[i] >= 0.0)
			sum_plus += coeff[i];
		else
			sum_minus += coeff[i];
	}

	/* Scale coefficients so that the maximum possible output is
	   mapped to zero and the maximum to 65,535. */
	scale = 65535.0 / (sum_plus - sum_minus);

	/* Generate filter_sum array. */
	tilem_free(af->filter_sum);
	af->filter_size = length + padnear + padfar;
	af->filter_sum = tilem_new_atomic(word, af->filter_size);

	af->init_high = -32768 - sum_minus * scale;

	x = 0.0;
	for (i = 0; i < padnear; i++) {
		af->filter_sum[i] = 0;
	}
	for (i = 0; i < length; i++) {
		x += coeff[i];
		af->filter_sum[i + padnear] = lround(x * scale);
	}
	for (i = 0; i < padfar; i++) {
		af->filter_sum[i + padnear + length] = lround(x * scale);
	}

	af->init_low = af->init_high + lround(x * scale);

	tilem_free(coeff);
}

/* Enable sampling timer (and reset filter state) */
static void timer_on(TilemAudioFilter *af)
{
	word init0, init1;
	int i;

	if (TILEM_UNLIKELY(!af->output_rate)) {
		warn(af, _("output sampling rate undefined"));
		return;
	}
	if (TILEM_UNLIKELY(!af->output_channels)) {
		warn(af, _("number of output channels undefined"));
		return;
	}
	if (TILEM_UNLIKELY(af->output_format < 0)) {
		warn(af, _("output sample format undefined"));
		return;
	}

	msg(af, _("Starting playback:"));
	msg(af, _(" rate        = %d"), af->output_rate);
	msg(af, _(" channels    = %d"), af->output_channels);
	msg(af, _(" format      = %s"), format_to_string(af->output_format));
	if (af->buffer_size)
		msg(af, _(" buffer size = %d"), af->buffer_size);

	af->timer_period = af->timer_period_f;
	af->timer_err = af->timer_err_f + af->timer_err_c;
	af->sample_err = 0;

	af->sample_tmr = tilem_z80_add_timer(af->calc, af->timer_period,
	                                     af->timer_period, 1,
	                                     &timer_callback, af);
	af->link_bp = tilem_z80_add_breakpoint(af->calc, TILEM_BREAK_PORT_WRITE,
	                                       af->calc->hw.linkportaddr,
	                                       af->calc->hw.linkportaddr,
	                                       af->calc->hw.portmask,
	                                       &link_callback, af);

	af->old_lines = af->calc->linkport.lines;
	init0 = (af->old_lines & 1 ? af->init_low : af->init_high);
	init1 = (af->old_lines & 2 ? af->init_low : af->init_high);

	for (i = 0; i < af->num_accum; i++) {
		af->accum[i].sample[0] = init0;
		af->accum[i].sample[1] = init1;
		af->accum[i].t_offset = ((float) i * SUBTICKRATE
		                         / af->output_rate) + 1;
	}

	af->cur_accum = 0;
}

/* Disable sampling timer */
static void timer_off(TilemAudioFilter *af)
{
	if (af->sample_tmr)
		tilem_z80_remove_timer(af->calc, af->sample_tmr);
	if (af->link_bp)
		tilem_z80_remove_breakpoint(af->calc, af->link_bp);

	af->sample_tmr = 0;
	af->link_bp = 0;
}

static void flush_buffer(TilemAudioFilter *af)
{
	while (af->buffer_remaining != 0)
		put_frame(af, af->init_high, af->init_high);
}

TilemAudioFilter * tilem_audio_filter_new(TilemCalc *calc)
{
	TilemAudioFilter *af;
	af = tilem_new0(TilemAudioFilter, 1);
	af->calc = calc;
	af->volume = DEFAULT_VOLUME;
	af->output_format = -1;
	return af;
}

void tilem_audio_filter_free(TilemAudioFilter *af)
{
	flush_buffer(af);
	timer_off(af);

	tilem_free(af->accum);
	tilem_free(af->filter_sum);
	tilem_free(af);
}

void tilem_audio_filter_set_volume(TilemAudioFilter *af, double v)
{
	int cscale, max_scaled, null_scaled;

	af->volume = v;
	af->vscale = (af->volume
	              * (1 << VSHIFT)
	              * af->output_channels / 2);

	if (!af->output_channels)
		return;

	cscale = af->vscale * (2 / af->output_channels);
	max_scaled = 32767 * cscale;
	null_scaled = (int16_t) af->init_high * cscale;

	/* if possible, set DC offset so that the inactive state
	   (init_high) is translated to zero output */
	if (max_scaled - null_scaled <= 32767 << VSHIFT)
		af->dc_offset = 0x80000000 - null_scaled;
	/* otherwise, get as close as we can without clipping */
	else if (cscale <= 1 << VSHIFT)
		af->dc_offset = 0x80000000 + (32767 << VSHIFT) - max_scaled;
	/* if clipping is unavoidable, clip peaks and troughs
	   symmetrically */
	else
		af->dc_offset = 0x80000000;
}

void tilem_audio_filter_set_callback(TilemAudioFilter *af,
                                     TilemAudioCallbackFunc func,
                                     void *data)
{
	af->callback = func;
	af->callback_data = data;
}

void tilem_audio_filter_set_rate(TilemAudioFilter *af, int rate)
{
	if (TILEM_UNLIKELY(rate < 0 || rate > 500000)) {
		warn(af, _("invalid sampling rate (%d)"), rate);
		return;
	}

	if (rate == af->output_rate)
		return;

	flush_buffer(af);
	filter_setup(af, rate);
	tilem_audio_filter_set_volume(af, af->volume);
	tilem_audio_filter_reset(af);
}

void tilem_audio_filter_set_channels(TilemAudioFilter *af, int channels)
{
	if (TILEM_UNLIKELY(channels < 1 || channels > 2)) {
		warn(af, _("invalid number of channels (%d)"), channels);
		return;
	}

	if (channels == af->output_channels)
		return;

	flush_buffer(af);
	af->output_channels = channels;
	tilem_audio_filter_set_volume(af, af->volume);
}

void tilem_audio_filter_set_format(TilemAudioFilter *af, int format)
{
	if (TILEM_UNLIKELY(!format_to_string(format))) {
		warn(af, _("invalid sample format (%d)"), format);
		return;
	}

	if (format == af->output_format)
		return;

	flush_buffer(af);
	af->output_format = format;
}

void tilem_audio_filter_set_buffer(TilemAudioFilter *af,
                                   void *buffer, int size)
{
	int frame_size;

	if (TILEM_UNLIKELY(!af->output_rate)) {
		warn(af, _("output sampling rate undefined"));
		return;
	}
	if (TILEM_UNLIKELY(!af->output_channels)) {
		warn(af, _("number of output channels undefined"));
		return;
	}
	if (TILEM_UNLIKELY(af->output_format < 0)) {
		warn(af, _("output sample format undefined"));
		return;
	}

	if (af->output_format & TILEM_AUDIO_16_BIT)
		frame_size = 2 * af->output_channels;
	else
		frame_size = 2 * af->output_channels;

	if (TILEM_UNLIKELY(size < 0 || (size & (frame_size - 1)))) {
		warn(af, _("invalid buffer size (%d)"), size);
		return;
	}

	af->buffer_start = af->buffer_pos = buffer;
	af->buffer_size = af->buffer_remaining = size;
	af->overrun_warned = 0;
}

void tilem_audio_filter_on(TilemAudioFilter *af)
{
	if (!af->sample_tmr)
		timer_on(af);
}

void tilem_audio_filter_off(TilemAudioFilter *af)
{
	timer_off(af);
}

void tilem_audio_filter_reset(TilemAudioFilter *af)
{
	if (af->sample_tmr) {
		timer_off(af);
		timer_on(af);
	}
}

void tilem_audio_filter_flush(TilemAudioFilter *af)
{
	flush_buffer(af);
}

int tilem_audio_filter_buffer_count(TilemAudioFilter *af)
{
	return af->buffer_size - af->buffer_remaining;
}

int tilem_audio_filter_buffer_remaining(TilemAudioFilter *af)
{
	return af->buffer_remaining;
}

