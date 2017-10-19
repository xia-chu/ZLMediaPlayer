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
#include "MediaPlayerImp.h"
#include "H264Decoder.h"
#include "YuvDisplayer.h"
#include "AudioDec/AudioDec.h"
#include "Player/MediaPlayer.h"
#include "H264/H264Parser.h"
#include "Rtsp/UDPServer.h"
#include "Util/onceToken.h"
using namespace ZL::Util;
using namespace ZL::Rtsp;

//最大包长度 1 秒
#define MAX_BUF_SECOND (1)
//
#define RENDER_IN_MAIN_THREAD 1

class PcmPacket
{
public:
    string data;
    uint32_t timeStamp;
};

static shared_ptr<AsyncTaskThread> s_videoRenderTimer;//全局的视频渲染时钟
static shared_ptr<ThreadPool> s_videoDecodeThread; //全局的视频渲染时钟
static shared_ptr<Ticker> s_screenIdleTicker;//熄屏定时器

static onceToken s_token([]() {
#if defined(_WIN32)
    static onceToken g_token([]() {
        WORD wVersionRequested = MAKEWORD(2, 2);
        WSADATA wsaData;
        WSAStartup(wVersionRequested, &wsaData);
    }, []() {
        WSACleanup();
    });
#endif//defined(_WIN32)

    Logger::Instance().add(std::make_shared<ConsoleChannel>("stdout", LTrace));
    EventPoller::Instance(true);

    s_videoRenderTimer.reset(new AsyncTaskThread(5));//全局的视频渲染时钟
    s_videoDecodeThread.reset(new ThreadPool(1)); //全局的视频渲染时钟
    s_screenIdleTicker.reset(new Ticker);//熄屏定时器

    s_videoRenderTimer->DoTaskDelay(0, 3 * 1000, []() {
        if (s_screenIdleTicker->elapsedTime() > 3 * 1000) {
            //熄灭屏幕
        }
        return true;
    });
}, []() {
    s_videoRenderTimer.reset();
    s_videoDecodeThread.reset();
    s_screenIdleTicker.reset();

    UDPServer::Destory();
    EventPoller::Destory();
    Logger::Destory();
});

void MediaPlayerImp::setOnThreadSwitch(const onTask &cb) {
    if (cb) {
        _ontask = cb;
    }else{
        _ontask = [](const Task &tsk) {
            tsk();
        };
    }
}

MediaPlayerImp::MediaPlayerImp() {
    _delegate.reset(new MediaPlayerDelegate());
    setOnThreadSwitch(nullptr);
    _h264Parser.reset(new H264Parser);
}
MediaPlayerImp::~MediaPlayerImp(){
    teardown();
}

void MediaPlayerImp::setupEvent() {
    std::weak_ptr<MediaPlayerImp> weakSelf = shared_from_this();
    setOnShutdown([weakSelf](const SockException &ex) {
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        strongSelf->_playing = !ex;
        strongSelf->_ontask([strongSelf, ex]() {
            strongSelf->_delegate->onShutdown(strongSelf.get(), ex);
        });
    });
    setOnAudioCB([weakSelf](const AdtsFrame &data) {
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        strongSelf->onRecvAudio(data);
    });
    setOnVideoCB([weakSelf](const H264Frame &data) {
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        strongSelf->onRecvVideo(data);
    });
    setOnPlayResult([weakSelf](const SockException &ex) {
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        strongSelf->_playing = !ex;
        strongSelf->_ontask([strongSelf, ex]() {
            strongSelf->_delegate->onPlayResult(strongSelf.get(), ex);
        });
    });
}
void MediaPlayerImp::play(const char *url){
    _url = url;
    teardown();
    setupEvent();
    MediaPlayer::play(url);
}

void MediaPlayerImp::onRecvAudio(const AdtsFrame &data) {
    if (!_enableAudio || _pause) {
        return;
    }
    std::weak_ptr<MediaPlayerImp> weakSelf = shared_from_this();
    s_videoDecodeThread->async([weakSelf, data]() {
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        strongSelf->onAAC(data);
    });
    // DebugL << s_videoDecodeThread->size();
}
void MediaPlayerImp::onRecvVideo(const H264Frame &data) {
    if (_pause) {
        return;
    }
    std::weak_ptr<MediaPlayerImp> weakSelf = shared_from_this();
    s_videoDecodeThread->async([weakSelf, data]() {
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        strongSelf->onH264(data);
    });
    //DebugL << s_videoDecodeThread->size();
}

void MediaPlayerImp::tickProgress(){
    if (_onPorgressStamp.elapsedTime() > 300 && getDuration() > 0) {
        _onPorgressStamp.resetTime();
        float progress = getProgress();
        auto strongSelf = shared_from_this();
        strongSelf->_ontask([strongSelf, progress](){
            strongSelf->_delegate->onProgress(strongSelf.get(),progress);
        });
    }
}

void MediaPlayerImp::onDecoded(const std::shared_ptr<YuvFrame> &frame) {
    //display yuv;
    if (!_firstVideoStamp) {
        _firstVideoStamp = frame->pts;
        std::weak_ptr<MediaPlayerImp> weakSelf = shared_from_this();
        s_videoRenderTimer->CancelTask(reinterpret_cast<uint64_t>(this));
        s_videoRenderTimer->DoTaskDelay(reinterpret_cast<uint64_t>(this), 10, [weakSelf]() {
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return false;
            }
            strongSelf->onDrawFrame();
            return true;
        });
    }
    lock_guard<recursive_mutex> lck(_mtx_mapYuv);
    _mapYuv.emplace(frame->pts, frame);
    if (_mapYuv.rbegin()->second->pts - _mapYuv.begin()->second->pts >  1000 * MAX_BUF_SECOND) {
        _mapYuv.erase(_mapYuv.begin());
    }
}

void MediaPlayerImp::onH264(const H264Frame &data) {
    if (_pause) {
        return;
    }
    tickProgress();
    if (!_h264Decoder && isInited() && data.type == 7) {
        _h264Decoder.reset(new H264Decoder());
    }
    if (!_h264Decoder) {
        return;
    }
    //解码器已经OK
    //_h264Parser->inputH264(data.data, data.timeStamp);
    auto frame = _h264Decoder->inputVideo((uint8_t *)data.data.data(), data.data.size(), data.timeStamp,_h264Parser->getPts());
    if (!frame) {
        return;
    }
    frame->dts = frame->frame->pkt_dts;
    frame->pts = frame->frame->pts;
    onDecoded(frame);
}

void MediaPlayerImp::onAAC(const AdtsFrame &data) {
    if (_pause) {
        return;
    }
    tickProgress();
    if (!_aacDec) {
        _aacDec.reset(new AudioDec());
        _aacDec->Init(data.data);
        _audioPktMS = 1000 * _pcmBufSize / (_aacDec->getChannels()*_aacDec->getSamplerate()*_aacDec->getSamplebit() / 8);
        _audioBuffer.reset(new RingBuffer<PcmPacket>(MAX_BUF_SECOND * 1000 / _audioPktMS));
        _audioReader = _audioBuffer->attach(false);
    }
    uint8_t *pcm;
    int pcmLen = _aacDec->InputData(data.data, data.aac_frame_length, &pcm);
    if(pcmLen){
            _pcmBuf.append((char *)pcm,pcmLen);
    }
    int offset = 0;
    int i = 0;
    while (_pcmBuf.size() - offset >= _pcmBufSize) {
        PcmPacket pkt;
        pkt.data.assign(_pcmBuf.data() + offset, _pcmBufSize);
        pkt.timeStamp = data.timeStamp + _audioPktMS*(i++);
        _audioBuffer->write(pkt);
        offset += _pcmBufSize;
        if(offset > 100*1024){
            DebugL << offset << " " << _pcmBufSize << endl;
            break;
        }
    }
    if(offset){
            _pcmBuf.erase(0,offset);
    }

    if (!_audioSrc) {
            _audioSrc.reset(new AudioSRC(this));
            SDLAudioDevice::Instance().addChannel(_audioSrc.get());
    }
}

void MediaPlayerImp::teardown(){
    s_videoRenderTimer->CancelTask(reinterpret_cast<uint64_t>(this));
    SDLAudioDevice::Instance().delChannel(_audioSrc.get());
    MediaPlayer::teardown();
    _playing = false;
    _pause = true;
    _h264Decoder = nullptr;
    _aacDec.reset();
    _audioBuffer.reset();
    _pause = false;
    _playedAudioStamp = 0;
    _firstVideoStamp = 0;
    _audioPktMS = 0;
    _systemStamp.resetTime();
    lock_guard<recursive_mutex> lck(_mtx_mapYuv);
    _mapYuv.clear();
}
void MediaPlayerImp::rePlay(){
    if (!_url.empty()) {
        play(_url.data());
    }
}

void MediaPlayerImp::setDelegate(const std::shared_ptr<MediaPlayerDelegate> &delegate){
    weak_ptr<MediaPlayerImp> weakSelf = shared_from_this();
    _ontask([delegate, weakSelf]() {
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        if (delegate) {
            strongSelf->_delegate = delegate;
        }else {
            strongSelf->_delegate.reset(new MediaPlayerDelegate());;
        }
    });
}

void MediaPlayerImp::seekTo(float progress){
    {
        lock_guard<recursive_mutex> lck(_mtx_mapYuv);
        _mapYuv.clear();
        if (_audioReader) {
            _audioReader->reset(false);
        }
    }
    pause(false,false);
    _onPorgressStamp.resetTime();
    MediaPlayer::seekTo(progress);
}
void MediaPlayerImp::enableAudio(bool enableAudio){
    if (_enableAudio == enableAudio) {
        return;
    }
    _enableAudio = enableAudio;
    if (_pause && !_enableAudio) {
        //暂停中开启声音无效
        return;
    }
    if(_audioSrc){
        _audioSrc->setEnableMix(_enableAudio);
    }
}
bool MediaPlayerImp::isEnableAudio() const{
    return _enableAudio;
}
void MediaPlayerImp::pause(bool pause,bool flag){
    if (_pause == pause) {
        return;
    }
    _pause = pause;
    if (flag) {
        MediaPlayer::pause(pause);
    }

    if (_pause && !_enableAudio) {
        //暂停中开启声音无效
        return;
    }
    if(_audioSrc){
        _audioSrc->setEnableMix(_enableAudio);
    }
}
void MediaPlayerImp::pause(bool pause) {
    this->pause(pause, true);
}
bool MediaPlayerImp::isPaused() const{
    return  _pause;
}
bool MediaPlayerImp::isPlaying() const{
    return _playing;
}
void MediaPlayerImp::setPauseAuto(bool flag){
    _pauseAuto = flag;
}
const string & MediaPlayerImp::getUrl() const{
    return _url;
}

//audio
void MediaPlayerImp::setPCMBufferSize(int size) {
   _pcmBufSize = size;
   weak_ptr<MediaPlayerImp> weakSelf = shared_from_this();
   s_videoDecodeThread->async_first([weakSelf](){
       auto strongSelf = weakSelf.lock();
       if(strongSelf){
           strongSelf->_aacDec.reset();
       }
   });
}
int MediaPlayerImp::getPCMSampleBit() {
    return _aacDec->getSamplebit();

}
int MediaPlayerImp::getPCMSampleRate() {
    return (int)_aacDec->getSamplerate();

}
int MediaPlayerImp::getPCMChannel() {
    return _aacDec->getChannels();

}
int MediaPlayerImp::getPCMData(char *buf, int bufsize) {
    auto pkt = _audioReader->read();
    if (pkt) {
        _playedAudioStamp = pkt->timeStamp;
        if(bufsize >= pkt->data.size()){
            bufsize = pkt->data.size();
        }
        memcpy(buf, pkt->data.data(), bufsize);
        return (int)pkt->data.size();
    }
    return 0;
}

void MediaPlayerImp::onDrawFrame() {
    if (_pause) {
        return;
    }
    int headIndex;
    std::shared_ptr<YuvFrame> headFrame;
    {
        lock_guard<recursive_mutex> lck(_mtx_mapYuv);
        if (_mapYuv.empty()) {
            return;
        }
        headIndex = _mapYuv.begin()->first;
        headFrame = _mapYuv.begin()->second;//首帧
        _mapYuv.erase(_mapYuv.begin());// 消费第一帧
    }

    auto referencedStamp = _playedAudioStamp;
    if (abs((int32_t)(_playedAudioStamp - headFrame->pts)) > MAX_BUF_SECOND * 1000) {
        //没有音频或则
        if (_systemStamp.elapsedTime() > 5 * 1000) {
            //时间戳每5秒修正一次
            _firstVideoStamp = headFrame->pts;
            _systemStamp.resetTime();
        }
        referencedStamp = _firstVideoStamp + _systemStamp.elapsedTime();
    }
    if (headFrame->pts > referencedStamp + _audioPktMS / 2) {
        //不应该播放,重新放回列队
        lock_guard<recursive_mutex> lck(_mtx_mapYuv);
        _mapYuv.emplace(headIndex, headFrame);
        return;
    }
    //播放图像
    _frame = headFrame;
    weak_ptr<MediaPlayerImp> weakSelf = shared_from_this();
#if RENDER_IN_MAIN_THREAD
    _ontask([weakSelf, headFrame]() {
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        strongSelf->_delegate->onDrawFrame(strongSelf.get(), headFrame);
    });
#else
    _delegate->onDrawFrame(this, headFrame);
#endif

    if (_pauseAuto) {
        _pauseAuto = false;
        pause(true);
        std::weak_ptr<MediaPlayerImp> weakSelf = shared_from_this();
        _ontask([weakSelf]() {
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }
            strongSelf->_delegate->onAutoPaused(strongSelf.get());
        });
    }
    //禁止熄屏
    s_screenIdleTicker->resetTime();
    //此处亮屏
}
void MediaPlayerImp::reDraw() {
    if (_frame) {
#if RENDER_IN_MAIN_THREAD
        weak_ptr<MediaPlayerImp> weakSelf = shared_from_this();
        _ontask([weakSelf]() {
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }
            strongSelf->_delegate->onDrawFrame(strongSelf.get(), strongSelf->_frame);
        });
#else
        _delegate->onDrawFrame(this, _frame);
#endif
    }
}
