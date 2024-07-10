#include "DirectXCommon.h"
#include <cassert>
#include <format>

//文字列
#include "Logger.h"
#include "StringUtility.h"

//DirectX12
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

//ImGui
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
extern  IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


using namespace Microsoft::WRL;

void DirectXCommon::Initialize(WinApp* winApp)
{
	//NULL検出
	assert(winApp);

	//借りてきたWinAppのインスタンスを記録
	winApp_ = winApp;

	//デバイスの初期化
	InitializeDevice();
	//コマンド関連の初期化
	InitializeCommand();
	//スワップチェインの生成
	CreateSwapChain();
	//深度バッファの生成
	CreateDepthBuffer();
	//各種ディスクリプタヒープの生成
	CreateAllDescriptorHeaps();
	//レンダーターゲットビューの初期化
	InitializeRenderTargetView();
	//深度ステンシルビュー
	InitializeDepthStencilView();
	//フェンスの生成
	CreateFence();
	//ビューポートの初期化
	InitializeViewPort();
	//シザリングの初期化
	InitializeScissor();
	//DXCコンパイラの生成
	CreateDXCCompiler();
	//ImGuiの初期化
	InitializeImGui();
}

void DirectXCommon::InitializeDevice()
{
	HRESULT hr;

	/*--------------[ デバッグレイヤーをオンにする ]-----------------*/

#ifdef _DEBUG
	debugController_ = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController_)))) {
		//デバッグレイヤーを有効化する
		debugController_->EnableDebugLayer();
		//さらにGPU側でもチェックを行うようにする
		debugController_->SetEnableGPUBasedValidation(TRUE);
	}
#endif

	/*--------------[ DXGIファクトリーの生成 ]-----------------*/

	//HRESULTはWindows刑のエラーコードであり、
	//関数が成功したかどうかをSUCCEEDEDマクロで判定できる
	hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
	//初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、
	//どうにもできない場合が多いのでassertにしておく
	assert(SUCCEEDED(hr));

	/*--------------[ アダプターの列挙 ]-----------------*/

	//いい順にアダプタを頼む
	for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter_)) != DXGI_ERROR_NOT_FOUND; i++)
	{
		//アダプターの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter_->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));	//取得できないのは一大事
		//ソフトウェアアダプタでなければ採用！
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			//採用したアダプタの情報をログに出力。wstringの方なので注意
			Logger::Log(StringUtility::ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter_ = nullptr;
	}
	//適切なアダプタが見つからなかったので起動できない
	assert(useAdapter_ != nullptr);

	/*--------------[ デバイスの生成 ]-----------------*/

	//機能レベルとログの出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
	//高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); ++i)
	{
		//採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(useAdapter_.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
		//指定した機能レベルでデバイスが生成できたかを確認
		if (SUCCEEDED(hr))
		{
			//生成できたのでログ出力を行ってループを抜ける
			Logger::Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
			break;
		}
	}
	//デバイスの生成がうまくいかなかったので起動できない
	assert(device_ != nullptr);
	Logger::Log("Complete create D3D12Device!!!");//初期化完了のログを出す

	/*--------------[ エラー時にブレークを発生させる設定 ]-----------------*/

#ifdef _DEBUG
	if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue_))))
	{
		//やばいエラー時に止まる
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		//エラー時に止まる
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		//警告時に止まる
		//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		//抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] = {
			//windows11までのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		//抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		//指定したメッセージの表示を抑制する
		infoQueue_->PushStorageFilter(&filter);

		//解放
		infoQueue_->Release();
	}
#endif

}

void DirectXCommon::InitializeCommand()
{

	HRESULT hr;

	/*--------------[ コマンドアロケータの生成 ]-----------------*/

	hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
	//コマンドアロケータの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	/*--------------[ コマンドリストの生成 ]-----------------*/

	
	hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));
	//コマンドリストの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	/*--------------[ コマンドキューの生成 ]-----------------*/

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
	//コマンドキューの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

}

void DirectXCommon::CreateSwapChain()
{
	HRESULT hr;

	/*--------------[ スワップチェイン生成の設定 ]-----------------*/

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = WinApp::kClientWidth;						//画面の幅。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Height = WinApp::kClientHeight;					//画面の高さ。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				//色の形式
	swapChainDesc.SampleDesc.Count = 1;								//マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	//描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2;									//ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		//モニタにうつしたら、中身を廃棄

	/*--------------[ スワップチェイン生成 ]-----------------*/

	//コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	hr = dxgiFactory_->CreateSwapChainForHwnd(commandQueue_.Get(), winApp_->GetHwnd(), &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf()));
	assert(SUCCEEDED(hr));

}

void DirectXCommon::CreateDepthBuffer()
{
	HRESULT hr;

	/*--------------[ 深度バッファーリソースの設定 ]-----------------*/

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = WinApp::kClientWidth;							//Textureの幅
	resourceDesc.Height = WinApp::kClientHeight;						//Textureの高さ
	resourceDesc.MipLevels = 1;											//mipmapの数
	resourceDesc.DepthOrArraySize = 1;									//奥行き or 配列Textureの配列数
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;				//DepthStencilとして利用可能なフォーマット
	resourceDesc.SampleDesc.Count = 1;									//サンプリングカウント。１個指定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;		//２次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;		//DepthStencilとして使う通知

	/*--------------[ 利用するヒープの設定 ]-----------------*/

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;						//VRAM上に作る

	/*--------------[ 深度値のクリア設定 ]-----------------*/
	
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;							//1.0f(最大値)でクリア
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;				//フォーマット。Resourceと合わせる

	/*--------------[ 深度バッファの生成 ]-----------------*/

	hr = device_->CreateCommittedResource(
		&heapProperties,												//Heapの設定
		D3D12_HEAP_FLAG_NONE,											//Heapの特殊な設定。特になし
		&resourceDesc,													//Resourceの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE,								//深度値を書き込む状態にしておく
		&depthClearValue,												//Clear最適値
		IID_PPV_ARGS(&depthBuffer_)											//作成するResourceポインタへのポインタ
	);
	assert(SUCCEEDED(hr));

}

void DirectXCommon::CreateAllDescriptorHeaps()
{
	/*--------------[ ディスクリプタヒープのサイズを登録する ]-----------------*/

	descriptorSizeSRV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descriptorSizeRTV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorSizeDSV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	GetCPUDescriptorHandle(rtvDescriptorHeap_, descriptorSizeRTV_, 0);


	/*--------------[ ディスクリプタヒープの生成 ]-----------------*/

	rtvDescriptorHeap_ = CreateDescriptorHeap(device_, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	//SRV用のヒープでディスクリプタの数は１２８。SRVはShader内で触るものなので、ShaderVisibleはtrue
	srvDescriptorHeap_ = CreateDescriptorHeap(device_, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	//DSV用のヒープでディスクリプタの数は１。DSVはSharder内で触るものではないので、ShaderVisibleはfalse
	dsvDescriptorHeap_ = CreateDescriptorHeap(device_, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

}

void DirectXCommon::InitializeRenderTargetView()
{
	HRESULT hr;

	/*--------------[ スワップチェインからリソースを引っ張ってくる ]-----------------*/
	
	hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(&swapChainResources_[0]));
	//うまく取得できなければ起動できない
	assert(SUCCEEDED(hr));
	hr = swapChain_->GetBuffer(1, IID_PPV_ARGS(&swapChainResources_[1]));
	assert(SUCCEEDED(hr));

	/*--------------[ RTV用の設定 ]-----------------*/

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	/*--------------[ RTVハンドルの要素数を２個に変更する ]-----------------*/

	uint32_t RTVElements = 2;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

	/*--------------[ 生成していく ]-----------------*/

	for(uint32_t i = 0; i < RTVElements;++i)
	{
		/*--------------[ RTVハンドルの取得 ]-----------------*/

		// RTVのディスクリプタハンドルを設定
		rtvHandles_[i] = rtvStartHandle;

		/*--------------[ レンダーターゲットビューの生成 ]-----------------*/

		// SwapChainResourcesから対応するリソースを取得してRTVを作成
		device_->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc, rtvHandles_[i]);

		// 次のRTVのためにディスクリプタハンドルを移動
		rtvStartHandle.ptr += device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	}
}

void DirectXCommon::InitializeDepthStencilView()
{
	//DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	//DSVHeapの先頭にDSVを作る
	device_->CreateDepthStencilView(
		depthBuffer_.Get(),
		&dsvDesc,
		dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart()
	);
}

void DirectXCommon::CreateFence()
{
	HRESULT hr;

	/*--------------[ フェンスの生成 ]-----------------*/

	uint64_t fenceValue = 0;
	hr = device_->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));

	//FenceのSignalを待つためのイベントを作成する
	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);
}

void DirectXCommon::InitializeViewPort()
{
	//クライアント領域のサイズと一緒にして画面全体に表示
	viewport_.Width = WinApp::kClientWidth;
	viewport_.Height = WinApp::kClientHeight;
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;
}

void DirectXCommon::InitializeScissor()
{
	//基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect_.left = 0;
	scissorRect_.right = WinApp::kClientWidth;
	scissorRect_.top = 0;
	scissorRect_.bottom = WinApp::kClientHeight;
}

void DirectXCommon::CreateDXCCompiler()
{
	HRESULT hr;

	/*--------------[ DXCユーティリティの生成 ]-----------------*/

	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr));

	/*--------------[ DXCコンパイラの生成 ]-----------------*/

	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	assert(SUCCEEDED(hr));

	/*--------------[ デフォルトインクルードハンドラの生成 ]-----------------*/

	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr));

}

void DirectXCommon::InitializeImGui()
{
	/*--------------[ バージョンチェック ]-----------------*/

	IMGUI_CHECKVERSION();

	/*--------------[ コンテキストの生成 ]-----------------*/

	ImGui::CreateContext();

	/*--------------[ スタイルの設定 ]-----------------*/

	ImGui::StyleColorsDark();

	/*--------------[ Win32用の初期化 ]-----------------*/

	ImGui_ImplWin32_Init(winApp_->GetHwnd());

	/*--------------[ DirectX12用の初期化 ]-----------------*/

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = WinApp::kClientWidth;						//画面の幅。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Height = WinApp::kClientHeight;					//画面の高さ。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				//色の形式
	swapChainDesc.SampleDesc.Count = 1;								//マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	//描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2;									//ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		//モニタにうつしたら、中身を廃棄

	ImGui_ImplDX12_Init(
		device_.Get(),
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptorHeap_.Get(),
		srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart()
	);
}

void DirectXCommon::PreDraw()
{
	/*--------------[ バックバッファの番号取得 ]-----------------*/

	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	/*--------------[ リソースバリアで書き込み可能に変更 ]-----------------*/

	//TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier{};
	//今回のバリアはTransition
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//Noneにしておく
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	//バリアを張る対称のリソース。現在のバックバッファに対して行う
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
	//遷移前（現在）のresourceState
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	//遷移後のResourceState
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	//transitionBarrierを張る
	commandList_->ResourceBarrier(1, &barrier);
	
	/*--------------[ 描画先のRTVとDSVを指定する ]-----------------*/

	//DSV
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

	//描画先のRTVを設定する
	commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, nullptr);
	commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, &dsvHandle);

	/*--------------[ 画面全体の色をクリア ]-----------------*/

	float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };	//青っぽい色。RGBAの順
	commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);

	/*--------------[ 画面全体の深度をクリア ]-----------------*/

	//指定した深度で画面全体をクリアする
	commandList_->ClearDepthStencilView(
		dsvHandle,
		D3D12_CLEAR_FLAG_DEPTH,
		1.0f,
		0,
		0,
		nullptr
	);

	/*--------------[ SRV用のディスクリプタヒープを指定する ]-----------------*/

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeaps_[] = { srvDescriptorHeap_ };
	commandList_->SetDescriptorHeaps(1, descriptorHeaps_->GetAddressOf());

	/*--------------[ ビューポート領域の設定 ]-----------------*/

	commandList_->RSSetViewports(1, &viewport_);

	/*--------------[ シザー矩形の設定 ]-----------------*/

	commandList_->RSSetScissorRects(1, &scissorRect_);
}

void DirectXCommon::PostDraw()
{
	/*--------------[ バックバッファの番号取得 ]-----------------*/

	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	/*--------------[ リソースバリアで表示状態に変更 ]-----------------*/

	//TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier{};
	//今回のバリアはTransition
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//Noneにしておく
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	//バリアを張る対称のリソース。現在のバックバッファに対して行う
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
	//遷移前（現在）のresourceState
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	//遷移後のResourceState
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	//transitionBarrierを張る
	commandList_->ResourceBarrier(1, &barrier);

	/*--------------[ グラフィックスコマンドをクローズ ]-----------------*/

	HRESULT hr;

	//コマンドリストの内容を確定させる。すべてのコマンドを頼んでからCloseすること
	hr = commandList_->Close();
	assert(SUCCEEDED(hr));

	/*--------------[ GPUコマンドの実行 ]-----------------*/

	//GPU二コマンドリストの実行を行わせる
	Microsoft::WRL::ComPtr<ID3D12CommandList> commandLists[] = { commandList_ };
	commandQueue_->ExecuteCommandLists(1, commandLists->GetAddressOf());

	/*--------------[ GPU画面の交換を通知 ]-----------------*/

	//GPUとOSに画面の交換を行うように通知する
	swapChain_->Present(1, 0);

	/*--------------[ Fenceの値を更新 ]-----------------*/

	//Fenceの値を更新
	fenceValue_++;

	/*--------------[ コマンドキューにシグナルを送る ]-----------------*/

	commandQueue_->Signal(fence_.Get(), fenceValue_);
	//Fenceの値が指定したSignal値にたどり着いているか確認する
	//GetCompleteValueの初期値はFence制作時に渡した初期値
	if (fence_->GetCompletedValue() < fenceValue_)
	{
		HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		//指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent);
		//イベントを待つ
		WaitForSingleObject(fenceEvent, INFINITE);
		CloseHandle(fenceEvent);
	}

	/*--------------[ コマンド完了待ち ]-----------------*/

	

	/*--------------[ コマンドアロケーターのリセット ]-----------------*/

	//次フレーム用のコマンドリストを準備
	hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));

	/*--------------[ コマンドリストのリセット ]-----------------*/

	hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
	assert(SUCCEEDED(hr));


}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXCommon::CreateDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptor, bool shaderVisible)
{
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType; descriptorHeapDesc.NumDescriptors = numDescriptor;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index)
{
	return GetCPUDescriptorHandle(srvDescriptorHeap_, descriptorSizeSRV_, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index)
{
	return GetGPUDescriptorHandle(srvDescriptorHeap_, descriptorSizeSRV_, index);
}
