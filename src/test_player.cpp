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
#include "MediaPlayer/MediaPlayerWrapper.h"


#if defined(_WIN32)
#include <windows.h>
#include <tchar.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); //¥∞ø⁄∫Ø ˝Àµ√˜
//------------ ≥ı ºªØ¥∞ø⁄¿‡----------------
int WINAPI WinMain(HINSTANCE hInstance, //WinMain∫Ø ˝Àµ√˜
	HINSTANCE hPrevInst,
	LPSTR lpszCmdLine,
	int nCmdShow){
	HWND hwnd;
	MSG Msg;
	WNDCLASS wndclass;
	TCHAR lpszClassName[] = _T("¥∞ø⁄"); //¥∞ø⁄¿‡√˚
	TCHAR lpszTitle[] = _T("My_Windows"); //¥∞ø⁄±ÍÃ‚√˚
										  //¥∞ø⁄¿‡µƒ∂®“Â
	wndclass.style = 0; //¥∞ø⁄¿‡–ÕŒ™»± °¿‡–Õ
	wndclass.lpfnWndProc = WndProc; //¥∞ø⁄¥¶¿Ì∫Ø ˝Œ™WndProc
	wndclass.cbClsExtra = 0; //¥∞ø⁄¿‡Œﬁ¿©’π
	wndclass.cbWndExtra = 0; //¥∞ø⁄ µ¿˝Œﬁ¿©’π
	wndclass.hInstance = hInstance; //µ±«∞ µ¿˝æ‰±˙
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);

	//¥∞ø⁄µƒ◊Ó–°ªØÕº±ÍŒ™»± °Õº±Í
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	//¥∞ø⁄≤…”√º˝Õ∑π‚±Í
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);

	//¥∞ø⁄±≥æ∞Œ™∞◊…´
	wndclass.lpszMenuName = NULL; //¥∞ø⁄÷–Œﬁ≤Àµ•
	wndclass.lpszClassName = lpszClassName;
	//¥∞ø⁄¿‡√˚Œ™"¥∞ø⁄ æ¿˝"
	//--------------- ¥∞ø⁄¿‡µƒ◊¢≤· -----------------
	if (!RegisterClass(&wndclass)) //»Áπ˚◊¢≤· ß∞‹‘Ú∑¢≥ˆæØ∏Ê…˘“Ù
	{
		MessageBeep(0);
		return FALSE;
	}

	//¥¥Ω®¥∞ø⁄
	hwnd = CreateWindow(lpszClassName, //¥∞ø⁄¿‡√˚
		lpszTitle, //¥∞ø⁄ µ¿˝µƒ±ÍÃ‚√˚
		WS_OVERLAPPEDWINDOW, //¥∞ø⁄µƒ∑Á∏Ò
		CW_USEDEFAULT,
		CW_USEDEFAULT, //¥∞ø⁄◊Û…œΩ«◊¯±ÍŒ™»± °÷µ
		CW_USEDEFAULT,
		CW_USEDEFAULT, //¥∞ø⁄µƒ∏ﬂ∫ÕøÌŒ™»± °÷µ
		NULL, //¥À¥∞ø⁄Œﬁ∏∏¥∞ø⁄
		NULL, //¥À¥∞ø⁄Œﬁ÷˜≤Àµ•
		hInstance, //¥¥Ω®¥À¥∞ø⁄µƒ”¶”√≥Ã–Úµƒµ±«∞æ‰±˙
		NULL); //≤ª π”√∏√÷µ

    MediaPlayerWrapperHelper::globalInitialization();
    MediaPlayerWrapperHelper::Instance().addDelegate("rtmp://live.hkstv.hk.lxdns.com/live/hks", new MediaPlayerWrapperDelegateImp(hwnd));
    MediaPlayerWrapperHelper::Instance().getPlayer("rtmp://live.hkstv.hk.lxdns.com/live/hks")->setPauseAuto(false);
	/*MediaPlayerWrapper::Ptr player(new MediaPlayerWrapper());
	player->setOption("rtp_type",1);
	player->setPauseAuto(true);
	player->addDelegate(new MediaPlayerWrapperDelegateImp(hwnd));
	player->addDelegate(new MediaPlayerWrapperDelegateImp(hwnd1));*/

	//player->play("rtmp://douyucdn.cn/live/260965rO4IFm4Mat");

	//player->play("rtmp://live.hkstv.hk.lxdns.com/live/hks");
	//player->play("rtmp://192.168.0.124/live/obs");
	//player->play("rtsp://192.168.0.233/live/test0");
	//player->play("rtsp://192.168.0.124/record/live/obs/2017-08-16/11-21-47.mp4");
	//player->play("rtsp://admin:jzan123456@192.168.0.122/live/test0");

	//œ‘ æ¥∞ø⁄
	ShowWindow(hwnd, nCmdShow);
	//ªÊ÷∆”√ªß«¯
    UpdateWindow(hwnd);

	//œ˚œ¢—≠ª∑
	while (GetMessage(&Msg, NULL, 0, 0)){
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
		//MediaPlayerWrapperHelper::Instance().getPlayer("rtsp://192.168.0.233/live/test0")->reDraw();
	}

    MediaPlayerWrapperHelper::globalUninitialization();
	return Msg.wParam; //œ˚œ¢—≠ª∑Ω· ¯º¥≥Ã–Ú÷’÷π ±Ω´–≈œ¢∑µªÿœµÕ≥
}


//¥∞ø⁄∫Ø ˝
LRESULT CALLBACK WndProc(HWND hwnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (message) {
	case WM_DESTROY:
		PostQuitMessage(0); //µ˜”√PostQuitMessage∑¢≥ˆWM_QUITœ˚œ¢
	default: //»± ° ±≤…”√œµÕ≥œ˚œ¢»± °¥¶¿Ì∫Ø ˝
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return (0);
}
#else
#include <unistd.h>
int main(int argc,char *argv[]){
	//监听SIGINT信号,在按下 Ctrl + C后将调用stopSdlLoop方法通知runSdlLoop函数退出
	signal(SIGINT, [](int) { MediaPlayerWrapperHelper::stopSdlLoop();});

	//初始化基础网络库
    MediaPlayerWrapperHelper::globalInitialization();


	for(int i = 1 ;i < argc ; i++){
		//添加播放器实例，你可以这么执行本程序: ./test_player rtmp://127.0.0.1/live/0 rtsp://127.0.0.1/live/0 ...
		MediaPlayerWrapperHelper::Instance().addDelegate(argv[i], new MediaPlayerWrapperDelegateImp(nullptr));
		MediaPlayerWrapperHelper::Instance().getPlayer(argv[i])->setPauseAuto(false);
	}

	//执行sdl渲染循环线程，该操作将导致此线程阻塞
	//在macOS Majove 系统下,sdl渲染线程必须在main函数中执行，否则会黑屏
	//windows/linux系统下可以在后台线程执行runSdlLoop
	MediaPlayerWrapperHelper::runSdlLoop();

	//释放基础网络库
    MediaPlayerWrapperHelper::globalUninitialization();
}




#endif //defined(_WIN32)

















