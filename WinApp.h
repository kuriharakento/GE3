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

	HWND GetHwnd() const { return hwnd; }

	HINSTANCE GetHInstance() const { return wc.hInstance; }

public:	//定数
	//クライアント領域のサイズ
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

private: //メンバ変数
	//ウィンドウハンドル
	HWND hwnd = nullptr;

	//ウィンドウクラスの設定
	WNDCLASS wc{};
};

