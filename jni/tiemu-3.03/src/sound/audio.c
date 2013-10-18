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
#include "stream.h"
#include "dboxes.h"

int audio_isactive;
int audioerr;

int mix_volume;
char *buffer;

SDL_AudioSpec format;


//sets up everything for use
int audio_init(void) {
	//initialize SDL for Audio
	if(SDL_Init(SDL_INIT_AUDIO)<0) {
		msg_box1(_("Unable to initialize sound"),SDL_GetError());
		return -1;
	}
	
	atexit(SDL_Quit);

	atexit(audio_disable);

	//set up the audio format
	format.freq=44100;
	format.format=AUDIO_S8;
	format.channels=2;
	format.samples=2048;
	format.callback=stream_audio;
	format.userdata=NULL;

	audio_isactive=0;

	return 0;
}


int audio_enable(void) {
	if(audio_isactive)
		return 0;
	
	buffer=malloc(BUFFER_SIZE);
	
	if(!buffer) {
		msg_box1(_("Sound Error"),_("Not enough memory"));
		return -1;
	}
	
	memset(buffer,0,BUFFER_SIZE);
	bufpos=0;

	
	//open the audio device
	if(SDL_OpenAudio(&format,NULL)<0) {
		msg_box1(_("Unable to open audio device"),SDL_GetError());
		return -1;
	}
	
	audio_isactive=1;
	
	//begin streaming audio
	SDL_PauseAudio(0);

	return 0;
}

void audio_disable(void) {
	if(audio_isactive) {
		//stop streaming audio
		SDL_CloseAudio();
		audio_isactive=0;
		free(buffer);
	}
}
