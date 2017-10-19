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
#pragma once
#include <mutex>
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
using namespace std;

#if !defined(__MINGW32__) && defined(_WIN32)
#ifdef MediaPlayer_shared_EXPORTS
#define MediaPlayer_API __declspec(dllexport)
#else
#define MediaPlayer_API __declspec(dllimport)
#endif
#else
#define MediaPlayer_API
#endif //!defined(__MINGW32__)


class YuvFrame;
class YuvDisplayer;
class MediaPlayerImp;
class MediaPlayerWrapper;
class MediaPlayerDelegateHelper;

class MediaPlayer_API MediaPlayerWrapperDelegate
{
public:
	friend class MediaPlayerDelegateHelper;
	MediaPlayerWrapperDelegate();
	virtual ~MediaPlayerWrapperDelegate();
	virtual void  onPlayResult(MediaPlayerWrapper *sender, int errCode,const char *errMsg) {};
	virtual void  onProgress(MediaPlayerWrapper *sender, float progress) {};
	virtual void  onAutoPaused(MediaPlayerWrapper *sender) {};
	virtual void  onShutdown(MediaPlayerWrapper *sender, int errCode, const char *errMsg) {};
	virtual void* onGetWinID(MediaPlayerWrapper *sender) const { return nullptr; };
protected:
	std::shared_ptr<YuvDisplayer> _display;
	void onDrawFrame(MediaPlayerWrapper *sender, const std::shared_ptr<YuvFrame> &frame);
};

class MediaPlayer_API MediaPlayerWrapperDelegateImp : public MediaPlayerWrapperDelegate
{
public:
	MediaPlayerWrapperDelegateImp(void * win);
	virtual ~MediaPlayerWrapperDelegateImp();
private:
	void *_win;
    void* onGetWinID(MediaPlayerWrapper *sender) const  override;
};

class MediaPlayer_API MediaPlayerWrapper
{
public:
	friend class MediaPlayerWrapperHelper;
	typedef std::shared_ptr<MediaPlayerWrapper> Ptr;
	typedef std::function<void()> Task;
	typedef std::function<void(const Task &)> onTask;

	MediaPlayerWrapper();
	virtual ~MediaPlayerWrapper();

	void setOnThreadSwitch(const onTask &);
	void setOption(const char *key, int val);

	void play(const char *url);
	void teardown();
	void rePlay();
	const string & getUrl() const;
	bool isPlaying() const;

	void enableAudio(bool flag);
	bool isEnableAudio() const;

	void pause(bool flag) ;
	bool isPaused() const;
	void setPauseAuto(bool flag);

	int getVideoHeight() const ;
	int getVideoWidth() const ;
	float getVideoFps() const ;
	int getAudioSampleRate() const ;
	int getAudioSampleBit() const ;
	int getAudioChannel() const ;
	float getRtpLossRate(int iTrackId = -1) ;
	const string& getPps() const ;
	const string& getSps() const ;
	const string& getAudioCfg() const ;
	bool containAudio() const ;
	bool containVideo() const ;
	bool isInited() const ;
	float getDuration() const ;
	float getProgress() const ;
	void seekTo(float fProgress);
	void reDraw();
private:
	std::shared_ptr<MediaPlayerImp> _player;
	std::shared_ptr<MediaPlayerDelegateHelper> _delegate;

	void addDelegate(MediaPlayerWrapperDelegate * delegate);
	void delDelegate(MediaPlayerWrapperDelegate * delegate);
	int delegateSize() const;
};

class MediaPlayer_API MediaPlayerWrapperHelper
{
public:
	MediaPlayerWrapperHelper() ;
	virtual ~MediaPlayerWrapperHelper() ;
	
	static MediaPlayerWrapperHelper &Instance();
    void setOnThreadSwitch(const MediaPlayerWrapper::onTask &);
	void addDelegate(const char *url,MediaPlayerWrapperDelegate * delegate);
	void delDelegate(const char *url, MediaPlayerWrapperDelegate * delegate);
	std::shared_ptr<MediaPlayerWrapper> getPlayer(const char *url);
private:
	recursive_mutex _mutex;
    MediaPlayerWrapper::onTask _ontask;
	unordered_map<string, std::shared_ptr<MediaPlayerWrapper> > _mapPlayer;
};
