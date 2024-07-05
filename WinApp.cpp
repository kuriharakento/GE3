#include "WinApp.h"
#include "externals/imgui/imgui.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if(ImGui_ImplWin32_WndProcHandler(hwnd,msg,wparam,lparam))
	{
		return true;
	}

	//メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{
		//ウィンドウが破棄された
	case WM_DESTROY:
		//OSに対してアプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);

}


void WinApp::Initialize()
{
	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	wc.lpfnWndProc = WindowProc;
	wc.lpszClassName = L"GE3WindowClass";
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	RegisterClass(&wc);

	RECT wrc = { 0,0,kClientWidth,kClientHeight };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	hwnd = CreateWindow(
		wc.lpszClassName,
		L"GE3",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr
	);
	
	ShowWindow(hwnd,SW_SHOW);
}

void WinApp::Update()
{
	

}

