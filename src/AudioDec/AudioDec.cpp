/*
 * AudioDec.cpp
 *
 *  Created on: 2014-7-1
 *      Author: root
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

