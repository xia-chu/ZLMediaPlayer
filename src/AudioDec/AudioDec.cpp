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

#include "AudioDec.h"
#include "libFaad/faad.h"
AudioDec::AudioDec() {
	// TODO Auto-generated constructor stub
	_handle=nullptr;
	samplebit=16;
}

AudioDec::~AudioDec() {
	// TODO Auto-generated destructor stub
	if(_handle != nullptr){
		NeAACDecClose((NeAACDecHandle)_handle);
		_handle =nullptr;
	}
}

bool AudioDec::Init(const void *adtshed,int hedlen) {
	_handle = NeAACDecOpen();
	if(_handle == nullptr){
		return false;
	}
	char err = NeAACDecInit((NeAACDecHandle)_handle, ( unsigned char *)adtshed, hedlen, &samplerate, &channels);
	if (err != 0)
	{
		return false;
	}
	return true;

}

int AudioDec::InputData(const void *data, int len, unsigned char** pOutBuffer) {
	NeAACDecFrameInfo hInfo;
	NeAACDecHandle handle = (NeAACDecHandle)_handle;
	* pOutBuffer=(unsigned char*)NeAACDecDecode(handle, &hInfo, (unsigned char*)data,len);
	if (!((hInfo.error == 0) && (hInfo.samples > 0))){
		return 0;
	}
	return hInfo.samples*hInfo.channels;
}

