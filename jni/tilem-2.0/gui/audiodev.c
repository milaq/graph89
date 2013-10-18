/*
 * TilEm II
 *
 * Copyright (c) 2012 Benjamin Moody
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
#include <string.h>
#include <gtk/gtk.h>
#include <ticalcs.h>
#include <tilem.h>
#ifdef HAVE_LIBSDL
# include <SDL_audio.h>
#endif

#include "gui.h"

#ifdef HAVE___SYNC_SYNCHRONIZE
# define BARRIER() __sync_synchronize()
#else
# define BARRIER() do { gint x; g_atomic_int_get(&x); } while (0)
#endif
#define READ_BARRIER()  BARRIER()
#define WRITE_BARRIER() BARRIER()

#ifdef HAVE_LIBSDL

#define MIN_FRAMES_PER_CHUNK 64

struct _TilemAudioDevice {
	TilemAudioOptions options;

	guint8 **chunks;
	volatile int *chunk_state;
	guint chunk_size;
	guint nchunks;
	guint read_pos;
	int read_chunk;
	int write_chunk;

	gboolean buffer_pending;
	int initial_fill;
	int underrun_bytes;

	SDL_sem *buffer_count;
};

static void message(const char *s, ...)
{
	va_list ap;
	char buf[256];
	va_start(ap, s);
	g_vsnprintf(buf, sizeof(buf), s, ap);
	g_printerr(_("audiodev-sdl: %s\n"), buf);
	va_end(ap);
}

/* All known SDL audio drivers */
static const char * const known_drivers[] = {
	"AL", "alsa", "arts", "audio", "coreaudio", "dart", "dcaudio",
	/*"disk",*/ "dma", "dsound", "dsp", /*"dummy",*/ "esd",
	"mint_dma8", "mint_gsxb", "mint_mcsn", "mint_stfa", "mint_xbios",
	"nas", "nds", "netbsd", "openbsd", "paud", "pulse", "qsa-nto",
	"sndmgr", "ums", "waveout" };

static const char **driver_list;

static gboolean check_driver(const char *name)
{
	char buf[256];

	/* NAS driver takes several seconds to decide that it's not
	   working - so simply assume it's available iff it was
	   present at compile time */
	if (!strcmp(name, "nas")) {
#ifdef SDL_AUDIO_DRIVER_NAS
		return TRUE;
#else
		return FALSE;
#endif
	}

	if (!SDL_AudioInit(name)
	    && SDL_AudioDriverName(buf, sizeof(buf))
	    && !g_ascii_strcasecmp(buf, name)) {
		SDL_AudioQuit();
		return TRUE;
	}
	else {
		SDL_AudioQuit();
		return FALSE;
	}
}

void tilem_audio_device_init()
{
	int i, j;

	driver_list = g_new(const char *, G_N_ELEMENTS(known_drivers) + 1);
	for (i = j = 0; i < (int) G_N_ELEMENTS(known_drivers); i++) {
		if (check_driver(known_drivers[i])) {
			driver_list[j] = known_drivers[i];
			j++;
		}
	}
	driver_list[j] = NULL;
}

const char * const * tilem_audio_device_list_drivers()
{
	return driver_list;
}

void tilem_audio_device_exit()
{
	g_free(driver_list);
	driver_list = NULL;
}

void * tilem_audio_device_get_buffer(TilemAudioDevice *dev, int *size,
                                     GError **err)
{
	g_return_val_if_fail(dev != NULL, NULL);
	g_return_val_if_fail(size != NULL, NULL);
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	if (!dev->buffer_pending) {
		dev->buffer_pending = TRUE;
		/* wait for next buffer in queue to be ready */
		if (SDL_SemWaitTimeout(dev->buffer_count, 1000)) {
			/* (I have occasionally seen the SDL audio
			   thread die for no apparent reason, and SDL
			   provides no way of reporting errors that
			   occur there...) */
			g_set_error(err, TILEM_AUDIO_ERROR,
			            TILEM_AUDIO_ERROR_UNKNOWN,
			            _("Playback stopped unexpectedly"));
			return NULL;
		}
	}

	*size = dev->chunk_size;
	return dev->chunks[dev->write_chunk];
}

gboolean tilem_audio_device_play_buffer(TilemAudioDevice *dev, GError **err)
{
	g_return_val_if_fail(dev != NULL, FALSE);
	g_return_val_if_fail(dev->buffer_pending, FALSE);
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
	dev->buffer_pending = FALSE;

	WRITE_BARRIER();
	dev->chunk_state[dev->write_chunk] = 1; /* mark buffer as filled */

	dev->write_chunk = (dev->write_chunk + 1) & (dev->nchunks - 1);

	/* start playing once the desired number of buffers have been
	   filled */
	if (dev->initial_fill > 0) {
		dev->initial_fill--;
		if (dev->initial_fill == 0)
			SDL_PauseAudio(0);
	}

	if (dev->underrun_bytes) {
		message(_("WARNING: Audio buffer underrun by %d bytes"),
		        dev->underrun_bytes);
		dev->underrun_bytes = 0;
	}

	return TRUE;
}

static void io_callback(void *data, Uint8 *buffer, int len)
{
	TilemAudioDevice *dev = data;
	int rchunk, rpos, n;
	guint8 * restrict buf;
	guint8 ** restrict chunks;

	rchunk = dev->read_chunk;
	rpos = dev->read_pos;
	buf = buffer;
	chunks = dev->chunks;

	while (len > 0) {
		if (!dev->chunk_state[rchunk]) {
			/* don't bother attempting to synchronize this
			   variable, it's for informational purposes
			   only */
			dev->underrun_bytes += len;
			memset(buf, 0, len);
			break;
		}
		READ_BARRIER();

		n = (dev->chunk_size - rpos);
		if (G_LIKELY(len >= n)) {
			memcpy(buf, chunks[rchunk] + rpos, n);
			dev->chunk_state[rchunk] = 0;
			buf += n;
			len -= n;
			rchunk = (rchunk + 1) & (dev->nchunks - 1);
			rpos = 0;
			SDL_SemPost(dev->buffer_count);
		}
		else {
			rpos += len;
			memcpy(buf, chunks[rchunk] + rpos, len);
			break;
		}
	}

	dev->read_chunk = rchunk;
	dev->read_pos = rpos;
}

TilemAudioDevice * tilem_audio_device_open(const TilemAudioOptions *options,
                                           GError **err)
{
	TilemAudioDevice *dev;
	SDL_AudioSpec aspec, real_aspec;
	guint framesize, frames_per_chunk, latency, latency_chunks;
	char name[256];
	const char *devname;
	int fmt;
	guint i;

	g_return_val_if_fail(options != NULL, NULL);
	g_return_val_if_fail(options->rate > 0, NULL);
	g_return_val_if_fail(options->channels > 0, NULL);
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	/* initialize driver */

	if (SDL_AudioInit(options->driver)) {
		g_set_error(err, TILEM_AUDIO_ERROR,
		            TILEM_AUDIO_ERROR_BAD_DRIVER,
		            "%s", SDL_GetError());
		return NULL;
	}

	if (SDL_AudioDriverName(name, sizeof(name)))
		message(_("Initialized \"%s\" driver"), name);
	else
		strcpy(name, "");

	if ((devname = g_getenv("AUDIODEV")))
		message(_("AUDIODEV = \"%s\""), devname);

	/* open output device */

	frames_per_chunk = MIN_FRAMES_PER_CHUNK;
	while (frames_per_chunk * 7 < options->latency * options->rate * 2)
		frames_per_chunk *= 2;

	dev = g_slice_new0(TilemAudioDevice);

	aspec.freq = options->rate;
	aspec.channels = options->channels;
	aspec.format = AUDIO_S16SYS;
	aspec.samples = frames_per_chunk;
	aspec.callback = &io_callback;
	aspec.userdata = dev;

	if (SDL_OpenAudio(&aspec, &real_aspec)) {
		g_set_error(err, TILEM_AUDIO_ERROR,
		            TILEM_AUDIO_ERROR_BAD_DEVICE,
		            "%s", SDL_GetError());
		tilem_audio_device_close(dev);
		return NULL;
	}

	/* convert real_aspec into TilemAudioOptions */

	dev->options.driver = g_strdup(name);
	dev->options.rate = real_aspec.freq;
	dev->options.channels = real_aspec.channels;

	switch (real_aspec.format) {
	case AUDIO_U8:     fmt = TILEM_AUDIO_U8; break;
	case AUDIO_S8:     fmt = TILEM_AUDIO_S8; break;
	case AUDIO_U16LSB: fmt = TILEM_AUDIO_U16_LE; break;
	case AUDIO_S16LSB: fmt = TILEM_AUDIO_S16_LE; break;
	case AUDIO_U16MSB: fmt = TILEM_AUDIO_U16_BE; break;
	case AUDIO_S16MSB: fmt = TILEM_AUDIO_S16_BE; break;
	default:
		g_set_error(err, TILEM_AUDIO_ERROR,
		            TILEM_AUDIO_ERROR_BAD_FORMAT,
		            _("Unknown sample format"));
		tilem_audio_device_close(dev);
		return NULL;
	}
	dev->options.format = fmt;

	framesize = (fmt & TILEM_AUDIO_16_BIT ? 2 : 1) * dev->options.channels;
	dev->chunk_size = framesize * frames_per_chunk;

	/* compute latency and allocate sample buffers */

	latency = MAX(real_aspec.samples + frames_per_chunk,
	              options->latency * real_aspec.freq);

	latency_chunks = ((latency + frames_per_chunk / 2) / frames_per_chunk);
	latency = latency_chunks * frames_per_chunk;
	dev->options.latency = (double) latency / real_aspec.freq;

	dev->nchunks = 2;
	while (dev->nchunks <= latency_chunks)
		dev->nchunks *= 2;

	dev->chunks = g_new(guint8 *, dev->nchunks);
	for (i = 0; i < dev->nchunks; i++)
		dev->chunks[i] = g_new0(guint8, dev->chunk_size);

	dev->chunk_state = g_new0(int, dev->nchunks);

	dev->buffer_count = SDL_CreateSemaphore(latency_chunks);
	if (!dev->buffer_count) {
		g_set_error(err, TILEM_AUDIO_ERROR,
		            TILEM_AUDIO_ERROR_BAD_DRIVER,
		            _("Cannot create semaphore: %s"),
		            SDL_GetError());
		tilem_audio_device_close(dev);
		return NULL;
	}

	dev->initial_fill = latency_chunks;

	message(_("buffer size = %d, latency = %.3g"),
	        real_aspec.size, dev->options.latency);

	return dev;
}

void tilem_audio_device_get_options(const TilemAudioDevice *dev,
                                    TilemAudioOptions *options)
{
	g_return_if_fail(dev != NULL);
	g_return_if_fail(options != NULL);
	*options = dev->options;
}

void tilem_audio_device_close(TilemAudioDevice *dev)
{
	guint i;

	g_return_if_fail(dev != NULL);

	SDL_AudioQuit();

	if (dev->chunks) {
		for (i = 0; i < dev->nchunks; i++)
			g_free(dev->chunks[i]);
		g_free(dev->chunks);
	}

	g_free((int *) dev->chunk_state);

	if (dev->buffer_count)
		SDL_DestroySemaphore(dev->buffer_count);

	g_free(dev->options.driver);
	g_slice_free(TilemAudioDevice, dev);
}

#else /* !HAVE_LIBSDL */

void tilem_audio_device_init()
{
}

const char * const * tilem_audio_device_list_drivers()
{
	return NULL;
}

void tilem_audio_device_exit()
{
}

TilemAudioDevice * tilem_audio_device_open(G_GNUC_UNUSED const TilemAudioOptions *options,
                                           GError **err)
{
	g_set_error(err, TILEM_AUDIO_ERROR,
	            TILEM_AUDIO_ERROR_BAD_DRIVER,
	            _("TilEm has not been compiled with sound support."));
	return NULL;
}

void tilem_audio_device_close(G_GNUC_UNUSED TilemAudioDevice *dev)
{
}

void tilem_audio_device_get_options(G_GNUC_UNUSED const TilemAudioDevice *dev,
                                    G_GNUC_UNUSED TilemAudioOptions *options)
{
}

void * tilem_audio_device_get_buffer(G_GNUC_UNUSED TilemAudioDevice *dev,
                                     G_GNUC_UNUSED int *size,
                                     G_GNUC_UNUSED GError **err)
{
	return NULL;
}

gboolean tilem_audio_device_play_buffer(G_GNUC_UNUSED TilemAudioDevice *dev,
                                        G_GNUC_UNUSED GError **err)
{
	return FALSE;
}

#endif
