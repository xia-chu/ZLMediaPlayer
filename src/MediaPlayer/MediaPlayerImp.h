#pragma once

#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <functional>
#include "Player/MediaPlayer.h"
#include "SDLAudioMixer/SDLAudioDevice.h"
using namespace std;

class PcmPacket;
class AudioDec;
class YuvFrame;
class YuvDisplayer;
class H264Parser;
class H264Decoder;
class MediaPlayerImp;


class MediaPlayerDelegate
{
public:
	MediaPlayerDelegate() {}
	virtual ~MediaPlayerDelegate() {}
	virtual void  onPlayResult(MediaPlayerImp *sender, const SockException &err) {};
	virtual void  onProgress(MediaPlayerImp *sender, float progress) {};
	virtual void  onAutoPaused(MediaPlayerImp *sender) {};
	virtual void  onShutdown(MediaPlayerImp *sender, const SockException &err) {};
	virtual void  onDrawFrame(MediaPlayerImp *sender, const std::shared_ptr<YuvFrame> &frame) {};
};


class MediaPlayerImp : public MediaPlayer,public AudioSRCDelegate, public std::enable_shared_from_this<MediaPlayerImp>
{
public:
	typedef std::shared_ptr<MediaPlayerImp> Ptr;
	typedef std::function<void()> Task;
	typedef std::function<void(const Task &)> onTask;

	MediaPlayerImp();
	virtual ~MediaPlayerImp();

	void setOnThreadSwitch(const onTask &);
	void play(const char *url) override;
	void teardown() override;
	void rePlay();
	void seekTo(float progress) override;
	const string & getUrl() const;
	bool isPlaying() const;

	void enableAudio(bool flag);
	bool isEnableAudio() const;

	void pause(bool flag) override;
	bool isPaused() const;
	void setPauseAuto(bool flag);

	void setDelegate(const std::shared_ptr<MediaPlayerDelegate> &delegate);
	void reDraw();

	//audio
	void setPCMBufferSize(int size) override;
	int getPCMSampleBit() override;
	int getPCMSampleRate() override;
	int getPCMChannel() override;
	int getPCMData(char *buf, int bufsize) override;
private:
	std::shared_ptr<MediaPlayerDelegate> _delegate;
	uint32_t _audioPktMS = 0;//每个音频包的时长（单位毫秒）
	uint32_t _playedAudioStamp = 0;//被播放音频的时间戳
	uint32_t _firstVideoStamp = 0;//起始视频时间戳

	Ticker _systemStamp;//系统时间戳
	Ticker _onPorgressStamp;//记录上次触发onPorgress回调的时间戳

	std::shared_ptr<AudioDec> _aacDec;////aac软件解码器
	std::shared_ptr<AudioSRC> _audioSrc;
	RingBuffer<PcmPacket>::Ptr _audioBuffer;//音频环形缓存
	RingBuffer<PcmPacket>::RingReader::Ptr _audioReader;//音频环形缓存读取器

	recursive_mutex _mtx_mapYuv;//yuv视频缓存锁
	std::shared_ptr<YuvFrame> _frame;//当前yuv
	multimap<uint32_t, std::shared_ptr<YuvFrame> > _mapYuv;//yuv视频缓存
	std::shared_ptr<H264Parser> _h264Parser;//h264 解析器，用于生产pts
	std::shared_ptr<H264Decoder> _h264Decoder;//h264硬件解码器

	string _url;
	string _pcmBuf;
	bool _playing = false;
	bool _enableAudio = true;
	bool _pause = false;
	bool _pauseAuto = true;
	onTask _ontask;
	int _pcmBufSize = 2048;

	void onRecvAudio(const AdtsFrame &data);
	void onRecvVideo(const H264Frame &data);
	void onH264(const H264Frame &data);
	void onAAC(const AdtsFrame &data);
	void tickProgress();
	void onDrawFrame();
	void pause(bool pause, bool flag);
	void setupEvent();
	void onDecoded(const std::shared_ptr<YuvFrame> &frame);
};
