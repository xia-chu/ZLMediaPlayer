/*
 * MIT License
 *
 * Copyright (c) 2017 xiongziliang <771730766@qq.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "SDLAudioDevice.h"

SDLAudioDevice& SDLAudioDevice::Instance() {
    static SDLAudioDevice *instance(new SDLAudioDevice);
	return *instance;
}

SDLAudioDevice::SDLAudioDevice() {
	SDL_AudioSpec wanted_spec;
	wanted_spec.freq = DEFAULT_SAMPLERATE;
	wanted_spec.format = DEFAULT_SAMPLEBIT == 16 ? AUDIO_S16 : AUDIO_S8;
	wanted_spec.channels = DEFAULT_CHANNEL;
	wanted_spec.silence = 0;
	wanted_spec.samples = DEFAULT_PCMSIZE / (DEFAULT_CHANNEL * DEFAULT_SAMPLEBIT / 8) ;
	wanted_spec.size = DEFAULT_PCMSIZE;
	wanted_spec.callback = SDLAudioDevice::onReqPCM;
	wanted_spec.userdata = this;
	if (SDL_OpenAudio(&wanted_spec, &_audioConfig)<0) {
//		throw std::runtime_error("SDL_OpenAudio failed");
        InfoL << "SDL_OpenAudio failed";
	}
	_pcmBuf.reset(new char[_audioConfig.size * 10],[](char *ptr){
		delete [] ptr;
	});
}

SDLAudioDevice::~SDLAudioDevice() {
}

void SDLAudioDevice::addChannel(AudioSRC* chn) {
	lock_guard<recursive_mutex> lck(_mutexChannel);
	if(_channelSet.empty()){
		SDL_PauseAudio(false);
	}
	chn->setOutputAudioConfig(_audioConfig);
	_channelSet.emplace(chn);
}

void SDLAudioDevice::delChannel(AudioSRC* chn) {
	lock_guard<recursive_mutex> lck(_mutexChannel);
	_channelSet.erase(chn);
	if(_channelSet.empty()){
		SDL_PauseAudio(true);
	}
}

void SDLAudioDevice::onReqPCM(void* userdata, Uint8* stream, int len) {
	SDLAudioDevice *_this = (SDLAudioDevice *)userdata;
	_this->onReqPCM((char *)stream,len);
}

void SDLAudioDevice::onReqPCM(char* stream, int len) {
	lock_guard<recursive_mutex> lck(_mutexChannel);
	int size;
	int channel = 0;
	for(auto &chn : _channelSet){
		if(channel ==0 ){
			size = chn->getPCMData(_pcmBuf.get(),len);
			if(size){
                //InfoL << len << " " << size;
				memcpy(stream,_pcmBuf.get(),size);
			}
		}else{
			size = chn->getPCMData(_pcmBuf.get(),len);
			if(size){
                //InfoL << len << " " << size;
				SDL_MixAudio((Uint8*)stream,(Uint8*)_pcmBuf.get(),size,SDL_MIX_MAXVOLUME);
			}
		}
		if(size){
			channel++;
		}
	}

	if(!channel){
		memset(stream,0,len);
	}
}















