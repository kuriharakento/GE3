#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <array>
#include <chrono>

#include <dxcapi.h>
#pragma comment(lib,"dxcompiler.lib")

#include "Logger.h"
#include "WinApp.h"
#include "externals/DirectXTex/d3dx12.h"
#include "externals/DirectXTex/DirectXTex.h"

class DirectXCommon
{
public:	//メンバ関数
	//初期化
	void Initialize(WinApp* winApp);

	/// \brief デバイスの初期化
	void InitializeDevice();

	/// \brief コマンド関連の初期化
	void InitializeCommand();

	/// \brief スワップチェインの生成
	void CreateSwapChain();

	/// \brief 深度バッファの生成
	void CreateDepthBuffer();

	/// \brief 各種ディスクリプタヒープ
	void CreateAllDescriptorHeaps();

	/// \brief レンダーターゲットビューの初期化
	void InitializeRenderTargetView();

	/// \brief 深度ステンシルビューの初期化
	void InitializeDepthStencilView();

	/// \brief フェンスの生成
	void CreateFence();

	/// \brief　ビューポート矩形の初期化
	void InitializeViewPort();

	/// \brief シザー矩形の初期化
	void InitializeScissor();

	/// \brief DXCコンパイラの生成
	void CreateDXCCompiler();

	/// \brief ImGuiの初期化
	void InitializeImGui();

	//描画前処理
	void PreDraw();

	//描画後処理
	void PostDraw();

	/// \brief ディスクリプタヒープを生成する
	/// \param device デバイス
	/// \param heapType 
	/// \param numDescriptor 
	/// \param shaderVisible 
	/// \return 
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptor, bool shaderVisible);

	/// \brief CPUのディスクリプタハンドルを取得
	/// \param descriptorHeap 
	/// \param descriptorSize 
	/// \param index 
	/// \return 
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

	/// \brief GPUのディスクリプタハンドルを取得
	/// \param descriptorHeap 
	/// \param descriptorSize 
	/// \param index 
	/// \return 
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

	/// \brief SRV用の指定のCPUディスクリプタハンドルを取得する
	/// \param index 
	/// \return 
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);

	/// \brief SRV用の指定のGPUディスクリプタハンドルを取得する
	/// \param index 
	/// \return 
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

	/// \brief シェーダーコンパイル関数
	/// \param filePath 
	/// \param profile 
	/// \param dxcUtils 
	/// \param dxcCompiler 
	/// \param includeHandler 
	/// \return 
	Microsoft::WRL::ComPtr<IDxcBlob> CompileSharder(const std::wstring& filePath, const wchar_t* profile);

	/// \brief リソース生成関数
	/// \param device 
	/// \param sizeInBytes 
	/// \return 
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);

	/// \brief テクスチャリソースの生成関数
	/// \param metadata 
	/// \return 
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);

	/// \brief テクスチャデータの転送
	/// \param texture 
	/// \param mipImages 
	/// \return 
	[[nodiscard]]
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages);

	/// \brief テクスチャファイルの読み込み
	/// \param filePath 
	/// \return 
	static DirectX::ScratchImage LoadTexture(const std::string& filePath);

	/*--------------[ ゲッター ]-----------------*/

	/// \brief デバイスの取得
	/// \return 
	ID3D12Device* GetDevice() const { return device_.Get(); }

	/// \brief コマンドリストの取得
	/// \return 
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }

private:
	//FPS固定初期化
	void InitializeFixFPS();
	//FPS固定更新
	void UpdateFixFPS();
	//記録時間(FPS固定用)
	std::chrono::steady_clock::time_point reference_;


private:	//メンバ変数

	WinApp* winApp_ = nullptr;

	/*--------------[ デバイスの変数 ]-----------------*/

	//DirectX12デバイス
	Microsoft::WRL::ComPtr<ID3D12Device> device_ = nullptr;
	//DXGIファクトリ
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
	//使用するアダプタ用の変数
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_ = nullptr;

	/*--------------[ コマンドの変数 ]-----------------*/

	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue_ = nullptr;
	//コマンドアロケータ
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
	//コマンドリスト
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;
	//コマンドキュー
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;

	/*--------------[ スワップチェインの変数 ]-----------------*/

	//スワップチェイン
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
	//スワップチェインリソース
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;

	/*--------------[ 深度バッファの変数 ]-----------------*/

	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer_ = nullptr;

	/*--------------[ ディスクリプタヒープの変数 ]-----------------*/

	//RTV用のヒープでディスクリプタの数は２。RTVはShader内で触るものではないので、ShaderVisibleはfalse
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_ = nullptr;

	//SRV用のヒープでディスクリプタの数は１２８。SRVはShader内で触るものなので、ShaderVisibleはtrue
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_ = nullptr;

	//DSV用のヒープでディスクリプタの数は１。DSVはSharder内で触るものではないので、ShaderVisibleはfalse
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_ = nullptr;

	//ディスクリプタヒープのサイズ
	uint32_t descriptorSizeSRV_;
	uint32_t descriptorSizeRTV_;
	uint32_t descriptorSizeDSV_;

	/*--------------[ レンダーターゲットビューの変数 ]-----------------*/

	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[2] = { nullptr };
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2]{};

	/*--------------[ フェンスの変数 ]-----------------*/

	Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;

	/*--------------[ ビューポートの変数 ]-----------------*/

	D3D12_VIEWPORT viewport_{};

	/*--------------[ シザリングの変数 ]-----------------*/

	D3D12_RECT scissorRect_{};

	/*--------------[ DXCコンパイラの変数 ]-----------------*/

	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_ = nullptr;

	/*--------------[ 描画後処理の変数 ]-----------------*/

	UINT64 fenceValue_ = 0;

};

