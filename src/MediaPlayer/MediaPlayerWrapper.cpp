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
#include "MediaPlayerWrapper.h"
#include "YuvDisplayer.h"
#include "H264Decoder.h"
#include "MediaPlayerImp.h"
#include <unordered_set>
#include <mutex>
#include <functional>
#include <memory>
#include <deque>
#include "Util/logger.h"
#include "Poller/EventPoller.h"

#define REFRESH_EVENT   (SDL_USEREVENT + 1)
#define EXIT_EVENT   (SDL_USEREVENT + 2)

using namespace std;
using namespace ZL::Util;
using namespace ZL::Poller;


class SDLDisplayerHelper
{
public:
    static SDLDisplayerHelper &Instance(){
        static SDLDisplayerHelper *instance(new SDLDisplayerHelper);
        return *instance;
    }
    static void Destory(){
        delete &Instance();
    }
    template<typename FUN>
    void doTask(FUN &&f){
        {
            lock_guard<mutex> lck(_mtxTask);
            _taskList.emplace_back(f);
        }
        SDL_Event event;
        event.type = REFRESH_EVENT;
        SDL_PushEvent(&event);
    }

    void stopLoop(){
        doTask([](){return false;});
    }

    void runLoop(){
        lock_guard<mutex> lck(_mtxLoop);
        bool flag = true;
        std::function<bool ()> task;
        SDL_Event event;
        while(flag){
            SDL_WaitEvent(&event);
            switch(event.type){
                case REFRESH_EVENT:{
                    {
                        lock_guard<mutex> lck(_mtxTask);
                        if(_taskList.empty()){
                            //not reachable
                            continue;
                        }
                        task = _taskList.front();
                        _taskList.pop_front();
                    }
                    flag = task();
                }
                    break;
            }
        }
    }
private:
    SDLDisplayerHelper(){
    };
    ~SDLDisplayerHelper(){
        stopLoop();
        lock_guard<mutex> lck(_mtxLoop);
    };
private:
    std::deque<std::function<bool ()> > _taskList;
    std::mutex _mtxTask;
    std::mutex _mtxLoop;
};

class MediaPlayerDelegateHelper : public MediaPlayerDelegate
{
public:
	MediaPlayerDelegateHelper(MediaPlayerWrapper *wraper) {
		_wraper = wraper;
	}
	virtual ~MediaPlayerDelegateHelper() {}
	void  onPlayResult(MediaPlayerImp *sender, const SockException &err) override { 
		lock_guard<recursive_mutex> lck(_mutex);
		for (auto delegate : _delegateList) {
			delegate->onPlayResult(_wraper, err.getErrCode(), err.what());
		}
	}
	void  onProgress(MediaPlayerImp *sender, float progress) override {
		lock_guard<recursive_mutex> lck(_mutex);
		for (auto delegate : _delegateList) {
			delegate->onProgress(_wraper, progress);
		}
	}
	void  onAutoPaused(MediaPlayerImp *sender) override {
		lock_guard<recursive_mutex> lck(_mutex);
		for (auto delegate : _delegateList) {
			delegate->onAutoPaused(_wraper);
		}
	}
	void  onShutdown(MediaPlayerImp *sender, const SockException &err) override {
		lock_guard<recursive_mutex> lck(_mutex);
		for (auto delegate : _delegateList) {
			delegate->onShutdown(_wraper, err.getErrCode(), err.what());
		}
	}
	void  onDrawFrame(MediaPlayerImp *sender, const std::shared_ptr<YuvFrame> &frame) override {
		lock_guard<recursive_mutex> lck(_mutex);
		for (auto delegate : _delegateList) {
			delegate->onDrawFrame(_wraper, frame);
		}
	}
	void addDelegate(MediaPlayerWrapperDelegate * delegate) {
		lock_guard<recursive_mutex> lck(_mutex);
		_delegateList.emplace(delegate);
	}
	void delDelegate(MediaPlayerWrapperDelegate * delegate) {
		lock_guard<recursive_mutex> lck(_mutex);
		_delegateList.erase(delegate);
	}
	int delegateSize() const {
		lock_guard<recursive_mutex> lck(_mutex);
		return _delegateList.size();
	}
private:
	MediaPlayerWrapper *_wraper;
	mutable recursive_mutex _mutex;
	unordered_set<MediaPlayerWrapperDelegate *> _delegateList;
};


void MediaPlayerWrapperDelegate::onDrawFrame(MediaPlayerWrapper *sender, const std::shared_ptr<YuvFrame> &frame) {
	if (!_display) {
		auto win = onGetWinID(sender);
        _display.reset(new YuvDisplayer(win));
	}
	if (_display) {
	    //切换到sdl线程去渲染画面
        auto displayTmp = _display;
        SDLDisplayerHelper::Instance().doTask([displayTmp,frame](){
            displayTmp->displayYUV(frame->frame.get());
            return true;
        });
	}
}


MediaPlayerWrapper::MediaPlayerWrapper()
{
	_delegate.reset(new MediaPlayerDelegateHelper(this));
	_player.reset(new MediaPlayerImp());
	_player->setDelegate(_delegate);
}

MediaPlayerWrapper::~MediaPlayerWrapper()
{
}

void MediaPlayerWrapper::setOnThreadSwitch(const onTask &cb) {
	_player->setOnThreadSwitch(cb);
}
void MediaPlayerWrapper::setOption(const char *key, int val) {
	(*_player)[key] = val;
}
void MediaPlayerWrapper::addDelegate(MediaPlayerWrapperDelegate * delegate) {
	_delegate->addDelegate(delegate);
}
void MediaPlayerWrapper::delDelegate(MediaPlayerWrapperDelegate * delegate) {
	_delegate->delDelegate(delegate);
}
int MediaPlayerWrapper::delegateSize() const {
	return _delegate->delegateSize();
}
void MediaPlayerWrapper::play(const char *url) {
	_player->play(url);

}
void MediaPlayerWrapper::teardown() {
	_player->teardown();
}
void MediaPlayerWrapper::rePlay() {
	_player->rePlay();
}

const string & MediaPlayerWrapper::getUrl() const {
	return _player->getUrl();
}
bool MediaPlayerWrapper::isPlaying() const {
	return _player->isPlaying();
}

void MediaPlayerWrapper::enableAudio(bool flag) {
	_player->enableAudio(flag);
}
bool MediaPlayerWrapper::isEnableAudio() const {
	return _player->isEnableAudio();
}

void MediaPlayerWrapper::pause(bool flag) {
	_player->pause(flag);
}
bool MediaPlayerWrapper::isPaused() const {
	return _player->isPaused();
}
void MediaPlayerWrapper::setPauseAuto(bool flag) {
	_player->setPauseAuto(flag);
}

int MediaPlayerWrapper::getVideoHeight() const {
	return _player->getVideoHeight();
}
int MediaPlayerWrapper::getVideoWidth() const {
	return _player->getVideoWidth();
}
float MediaPlayerWrapper::getVideoFps() const {
	return _player->getVideoFps();
}
int MediaPlayerWrapper::getAudioSampleRate() const {
	return _player->getAudioSampleRate();
}
int MediaPlayerWrapper::getAudioSampleBit() const {
	return _player->getAudioSampleBit();
}
int MediaPlayerWrapper::getAudioChannel() const {
	return _player->getAudioChannel();
}
float MediaPlayerWrapper::getRtpLossRate(int iTrackId) {
	return _player->getRtpLossRate(iTrackId);
}
const string& MediaPlayerWrapper::getPps() const {
	return _player->getPps();
}
const string& MediaPlayerWrapper::getSps() const {
	return _player->getSps();
}
const string& MediaPlayerWrapper::getAudioCfg() const {
	return _player->getAudioCfg();
}
bool MediaPlayerWrapper::containAudio() const {
	return _player->containAudio();
}
bool MediaPlayerWrapper::containVideo() const {
	return _player->containVideo();
}
bool MediaPlayerWrapper::isInited() const {
	return _player->isInited();
}
float MediaPlayerWrapper::getDuration() const {
	return _player->getDuration();
}
float MediaPlayerWrapper::getProgress() const {
	return _player->getProgress();
}
void MediaPlayerWrapper::seekTo(float fProgress) {
	_player->seekTo(fProgress);
}
void MediaPlayerWrapper::reDraw() {
	_player->reDraw();
}

 void MediaPlayerWrapperHelper::setOnThreadSwitch(const MediaPlayerWrapper::onTask &cb){
     lock_guard<recursive_mutex> lck(_mutex);
    _ontask = cb;
 }

void MediaPlayerWrapperHelper::addDelegate(const char *url, MediaPlayerWrapperDelegate * delegate) {
    if(url == nullptr){
        WarnL << "nullptr url";
        return;
    }
    lock_guard<recursive_mutex> lck(_mutex);
    auto &val = _mapPlayer[url];
    if (!val) {
        val.reset(new MediaPlayerWrapper());
        val->setOnThreadSwitch(_ontask);
        val->play(url);
    }
    val->addDelegate(delegate);

}
void MediaPlayerWrapperHelper::delDelegate(const char *url, MediaPlayerWrapperDelegate * delegate) {
    if(url == nullptr){
        WarnL << "nullptr url";
        return;
    }
    lock_guard<recursive_mutex> lck(_mutex);
    auto it = _mapPlayer.find(url);
    if (it == _mapPlayer.end()) {
        return;
    }
    it->second->delDelegate(delegate);
    if (!it->second->delegateSize()) {
        it->second->pause(true);
        _mapPlayer.erase(it);
    }
}
std::shared_ptr<MediaPlayerWrapper> MediaPlayerWrapperHelper::getPlayer(const char *url) {
    if(url == nullptr){
        WarnL << "nullptr url";
        return nullptr;
    }
    lock_guard<recursive_mutex> lck(_mutex);
    auto it = _mapPlayer.find(url);
    if (it == _mapPlayer.end()) {
        return nullptr;
    }
    return it->second;
}




MediaPlayerWrapperDelegate::MediaPlayerWrapperDelegate() {
}
MediaPlayerWrapperDelegate::~MediaPlayerWrapperDelegate() {
}


MediaPlayerWrapperDelegateImp::MediaPlayerWrapperDelegateImp(void * win) {
	_win = win;
}
MediaPlayerWrapperDelegateImp::~MediaPlayerWrapperDelegateImp() {
}
void* MediaPlayerWrapperDelegateImp::onGetWinID(MediaPlayerWrapper *sender) const {
    return _win;
};

MediaPlayerWrapperHelper::MediaPlayerWrapperHelper() {
}
MediaPlayerWrapperHelper:: ~MediaPlayerWrapperHelper() {
}

static MediaPlayerWrapperHelper *g_instance;
MediaPlayerWrapperHelper& MediaPlayerWrapperHelper::Instance() {
	return *g_instance;
}

void MediaPlayerWrapperHelper::globalInitialization() {
    static onceToken token([]() {
        {
            //初始化sdl相关资源
            if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == -1) {
                string err = "初始化SDL失败:";
                err += SDL_GetError();
                ErrorL << err;
                throw std::runtime_error(err);
            }
            SDL_LogSetAllPriority(SDL_LOG_PRIORITY_CRITICAL);
            SDL_LogSetOutputFunction([](void *userdata, int category, SDL_LogPriority priority, const char *message) {
                DebugL << category << " " << priority << message;
            }, nullptr);
            SDLDisplayerHelper::Instance();
        }

        //初始化ZLMediaKit库
        MediaPlayerImp::globalInitialization();
        //新建MediaPlayerWrapperHelper单例
        g_instance = new MediaPlayerWrapperHelper();
    });

}

void MediaPlayerWrapperHelper::globalUninitialization() {
    static onceToken token(nullptr,[]() {
        //释放MediaPlayerWrapperHelper单例
        delete g_instance;
        //反初始化ZLMediaKit库
        MediaPlayerImp::globalUninitialization();
        //释放sdl相关资源
        SDLDisplayerHelper::Destory();
        SDL_Quit();
    });
}

void MediaPlayerWrapperHelper::runSdlLoop() {
    SDLDisplayerHelper::Instance().runLoop();
}

void MediaPlayerWrapperHelper::stopSdlLoop() {
    SDLDisplayerHelper::Instance().stopLoop();
}
