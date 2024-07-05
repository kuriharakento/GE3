#pragma once
#include <cstdint>
#include <Windows.h>


class WinApp
{
public:	//静的メンバ関数
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public:	//メンバ関数
	void Initialize();

	void Update();

	void Finalize();

	HWND GetHwnd() const { return hwnd_; }

	HINSTANCE GetHInstance() const { return wc_.hInstance; }

public:	//定数
	//クライアント領域のサイズ
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

private: //メンバ変数
	//ウィンドウハンドル
	HWND hwnd_ = nullptr;

	//ウィンドウクラスの設定
	WNDCLASS wc_{};
};

