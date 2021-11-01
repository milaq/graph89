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

#define DEFAULT_AUDIO_RATE      48000
#define DEFAULT_AUDIO_CHANNELS  2
#define DEFAULT_AUDIO_FORMAT    TILEM_AUDIO_S16
#ifdef G_OS_WIN32
# define DEFAULT_AUDIO_LATENCY   0.1
#else
# define DEFAULT_AUDIO_LATENCY   0.03
#endif
#define DEFAULT_AUDIO_VOLUME    0.5

typedef struct {
	char *driver;
	int rate;		/* Sampling rate (Hz) */
	int channels;		/* Number of channels */
	int format;		/* Sample format (TILEM_AUDIO_xx) */
	double latency;		/* Approximate latency (seconds) */
} TilemAudioOptions;

typedef struct _TilemAudioDevice TilemAudioDevice;

#define TILEM_AUDIO_ERROR g_quark_from_static_string("tilem-audio-error")
enum {
	TILEM_AUDIO_ERROR_BAD_DRIVER,
	TILEM_AUDIO_ERROR_BAD_DEVICE,
	TILEM_AUDIO_ERROR_BAD_FORMAT,
	TILEM_AUDIO_ERROR_UNKNOWN
};

/* Initialize audio support */
void tilem_audio_device_init();

/* Shut down audio support */
void tilem_audio_device_exit();

/* Get a list of available audio drivers */
const char * const * tilem_audio_device_list_drivers();

/* Open an audio device.  OPTIONS is a suggested configuration; the
   actual configuration used will, of course, depend on the driver and
   hardware. */
TilemAudioDevice * tilem_audio_device_open(const TilemAudioOptions *options,
                                           GError **err);

/* Close an audio device. */
void tilem_audio_device_close(TilemAudioDevice *dev);

/* Get actual configuration of an audio device */
void tilem_audio_device_get_options(const TilemAudioDevice *dev,
                                    TilemAudioOptions *options);

/* Get next output buffer. */
void * tilem_audio_device_get_buffer(TilemAudioDevice *dev,
                                     int *size, GError **err);

/* Play contents of output buffer. */
gboolean tilem_audio_device_play_buffer(TilemAudioDevice *dev,
                                        GError **err);
