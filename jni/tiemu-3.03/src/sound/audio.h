/*Sound capability library for TI-Emu
Copyright (C) 2007  Peter Fernandes  supersonicandtails@gmail.com
Copyright (C) 2007  Kevin Kofler

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.*/

#ifndef TIEMU_AUDIO_H
#define TIEMU_AUDIO_H

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <glib.h>

#include <SDL.h>
#include <SDL_audio.h>
#include <math.h>

#include "logging.h"

#define AMP_MAX 127

#define BUFFER_SIZE 8192

extern int audio_isactive;
extern int audioerr;

extern int mix_volume;
extern char *buffer;

extern SDL_AudioSpec format;


//performs all the sound initialization
extern int audio_init(void);

//allcates buffer space and opens the audio device.  Must be called before push_amplitudes.
extern int audio_enable(void);

//frees the audio device, stops stream.  MUST be called before exiting
extern void audio_disable(void);

#endif
