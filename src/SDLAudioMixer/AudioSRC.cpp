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

#include "Util/logger.h"
#include "AudioSRC.h"
#include <stdexcept>

using namespace ZL::Util;

AudioSRC::AudioSRC(AudioSRCDelegate *del) {
	_delegate = del;
}
AudioSRC::~AudioSRC() {
}
void AudioSRC::setOutputAudioConfig(const SDL_AudioSpec& cfg) {
	int freq = _delegate->getPCMSampleRate();
	int format = _delegate->getPCMSampleBit() == 16 ? AUDIO_S16 : AUDIO_S8;
	int channels = _delegate->getPCMChannel();
	if(-1 == SDL_BuildAudioCVT(&_audioCvt,format,channels,freq,cfg.format,cfg.channels,cfg.freq)){
		throw std::runtime_error("the format conversion is not supported");
	}
	InfoL << freq << " " << format << " " << channels ;
	InfoL 	<< _audioCvt.needed << " "
			<< _audioCvt.src_format << " "
			<< _audioCvt.dst_format << " "
			<< _audioCvt.rate_incr << " "
			<< _audioCvt.len_mult << " "
			<< _audioCvt.len_ratio << " ";
}
void AudioSRC::setEnableMix(bool flag){
	_enableMix = flag;
}
int AudioSRC::getPCMData(char* buf, int bufsize) {
	if(!_enableMix){
		return 0;
	}
	if(!_pcmSize){
		_pcmSize =	bufsize / _audioCvt.len_ratio;
		_delegate->setPCMBufferSize(_pcmSize);
		InfoL << _pcmSize;
	}
	if(!_delegate->getPCMData(buf,_pcmSize)){
		return 0;
	}
	_audioCvt.buf = (Uint8 *)buf;
	_audioCvt.len = _pcmSize;
	if(0 != SDL_ConvertAudio(&_audioCvt)){
		_audioCvt.len_cvt = 0;
		TraceL << "SDL_ConvertAudio falied";
	}
    //return _audioCvt.len_cvt;
    if(_audioCvt.len_cvt == bufsize)
    {
		return bufsize;
	}
	if(_audioCvt.len_cvt){
		_pcmBuf.append(buf,_audioCvt.len_cvt);
	}
	if(_pcmBuf.size() < bufsize){
		return 0;
	}
	memcpy(buf,_pcmBuf.data(),bufsize);
	_pcmBuf.erase(0,bufsize);
	return bufsize;
}
