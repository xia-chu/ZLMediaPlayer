/*
 * SDLAudioDevice.h
 *
 *  Created on: 2017年8月24日
 *      Author: xzl
 */

#ifndef SDLAUDIOMIXER_SDLAUDIODEVICE_H_
#define SDLAUDIOMIXER_SDLAUDIODEVICE_H_

#include <mutex>
#include <memory>
#include <stdexcept>
#include <unordered_set>

#include "AudioSRC.h"
#include "Util/logger.h"
#include "Util/onceToken.h"
using namespace std;
using namespace ZL::Util;

#define DEFAULT_SAMPLERATE 32000
#define DEFAULT_SAMPLEBIT 16
#define DEFAULT_CHANNEL 2
#define DEFAULT_PCMSIZE 4096

class SDLAudioDevice{
public:
	void addChannel(AudioSRC *chn);
	void delChannel(AudioSRC *chn);
	static SDLAudioDevice &Instance();
	virtual ~SDLAudioDevice();
protected:
private:
	SDLAudioDevice();
	static void onReqPCM (void *userdata, Uint8 * stream,int len);
	void onReqPCM (char * stream,int len);

	unordered_set<AudioSRC *> _channelSet;
	recursive_mutex _mutexChannel;
	SDL_AudioSpec _audioConfig;
	std::shared_ptr<char> _pcmBuf;
};

#endif /* SDLAUDIOMIXER_SDLAUDIODEVICE_H_ */
