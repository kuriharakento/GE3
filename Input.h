#pragma once
#include <Windows.h>
#include <wrl.h>
#define DIRECTINPUT_VERSION	0x0800	//DirectInputのバージョン指定
#include  <dinput.h>


class Input
{
public:
	//namespace省略
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:		//メンバ関数
	//初期化
	void Initialize(HINSTANCE hInstance,HWND hwnd);

	//更新
	void Update();

public:		//キー状態取得関数
	/// \brief キーの押下をチェック
	/// \param keyNumber キー番号
	/// \return 押されているか
	bool PushKey(BYTE keyNumber);

	/// \brief キーのトリガーをチェック
	/// \param keynumber キー番号
	/// \return トリガーか
	bool TriggerKey(BYTE keynumber);

private:
	//キーボードデバイス生成
	ComPtr<IDirectInputDevice8> keyboard;

	//全キーの状態
	BYTE key[256] = {};

	//前回の全キーの状態
	BYTE keyPre[256] = {};
};

