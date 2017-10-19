/*
 * AudioSRC.h
 *
 *  Created on: 2017年8月24日
 *      Author: xzl
 */

#ifndef SDLAUDIOMIXER_AUDIOSRC_H_
#define SDLAUDIOMIXER_AUDIOSRC_H_

#include <memory>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif
#include "SDL2/SDL.h"
#ifdef __cplusplus
}
#endif

#if defined(_WIN32)
#pragma comment(lib,"SDL2.lib")
#endif //defined(_WIN32)

using namespace std;


class AudioSRCDelegate{
public:
	virtual ~AudioSRCDelegate(){};
	virtual void setPCMBufferSize(int bufsize) = 0;
	virtual int getPCMSampleBit() = 0;
	virtual int getPCMSampleRate() = 0;
	virtual int getPCMChannel() = 0;
	virtual int getPCMData(char *buf, int bufsize) = 0;
};
class AudioSRC
{
public:
	typedef std::shared_ptr<AudioSRC> Ptr;
	AudioSRC(AudioSRCDelegate *);
	virtual ~AudioSRC();
	void setEnableMix(bool flag);
	void setOutputAudioConfig(const SDL_AudioSpec &cfg);
	//此处buf大小务必要比bufsize大的多
	int getPCMData(char *buf, int bufsize);
private:
	AudioSRCDelegate *_delegate;
	SDL_AudioCVT _audioCvt;
	bool _enableMix = true;
	int _pcmSize = 0;
	string _pcmBuf;
};
#endif /* SDLAUDIOMIXER_AUDIOSRC_H_ */
