#pragma once
#include <Windows.h>


class WinApp
{
public:	//静的メンバ関数
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public:
	void Initialize();

	void Update();

};

