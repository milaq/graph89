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

#ifndef TIEMU_STREAM_H
#define TIEMU_STREAM_H

#include "audio.h"

extern int bufpos;

//callback to stream the audio buffer to SDL on request
extern void stream_audio(void *volume, Uint8 *stream, int len);

/*takes a pair of amplitudes (representing one stereo sample) and inserts them
into the stream buffer*/
extern void stream_push_amplitudes(char left, char right);

#endif
