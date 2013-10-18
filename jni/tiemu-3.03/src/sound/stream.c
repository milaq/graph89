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

#include "audio.h"

#undef BUFFER_FULL_WARNING

int bufpos;

void stream_audio(void *unused, Uint8 *stream, int len) {
	int spill=bufpos-len;
	if (spill < 0)
		spill = 0;

	//mix each playing voice into the audio stream len bytes at a time
	memcpy(stream,buffer,len);

	memmove(buffer,buffer+len,spill);
	memset(buffer+spill,0,bufpos-spill);

	//reset the buffer
	bufpos=spill;
}  

void stream_push_amplitudes(char left, char right) {
#ifdef BUFFER_FULL_WARNING
	static int warned=0;
#endif

	SDL_LockAudio();
	
	if(bufpos<BUFFER_SIZE) {
		*(buffer+bufpos)=left;
		*(buffer+bufpos+1)=right;
		bufpos+=2;
	}
#ifdef BUFFER_FULL_WARNING
	else if(!warned) {
		tiemu_warning(_("sound buffer full, dropping sample(s)"));
		warned=1;
	}
#endif
	
	SDL_UnlockAudio();

}

