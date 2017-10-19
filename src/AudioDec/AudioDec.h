/*
 * AudioDec.h
 *
 *  Created on: 2014-7-1
 *      Author: root
 */

#ifndef AUDIODEC_H_
#define AUDIODEC_H_

class AudioDec {
public:
	AudioDec(void);
	virtual ~AudioDec(void);
	bool Init(const void *adtshed, int hedlen = 7);
	int InputData(const void *data, int len, unsigned char **pOutBuffer);
	unsigned char getChannels() const {
		return channels;
	}
	unsigned long getSamplerate() const {
		return samplerate;
	}
	unsigned char getSamplebit() const {
		return samplebit;
	}
private:
	void *_handle;
	unsigned long samplerate;
	unsigned char channels;
	unsigned char samplebit;
};

#endif /* AUDIODEC_H_ */
