#pragma once
#include <d3d12.h>
#include <string>
#include <wrl.h>

#include "externals/DirectXTex/DirectXTex.h"

//deviceを使うために、DxCommonを前方宣言
class DirectXCommon;
class TextureManager
{

public:
	//シングルトンインスタンスの取得
	static TextureManager* GetInstance();
	/// \brief 初期化
	void Initialize(DirectXCommon* dxCommon);
	/// \brief 終了
	void Finalize();

	/// \brief テクスチャファイルの読み込み
	/// \param filePath 
	void LoadTexture(const std::string& filePath);

	/// \brief テクスチャリソースの生成
	/// \param device 
	/// \param metadata 
	/// \return 
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);

private:
	//テクスチャ１枚分のデータ
	struct TextureData
	{
		std::string filePath;
		DirectX::TexMetadata metadata;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
	};

	//テクスチャデータ
	std::vector<TextureData> textureDatas_;

	//DirectXCommonのポインタ
	DirectXCommon* dxCommon_;

private:	//シングルトン
	static TextureManager* instance_;

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(TextureManager&) = delete;
};

