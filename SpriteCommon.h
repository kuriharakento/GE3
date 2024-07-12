#pragma once
#include <d3d12.h>
#include <wrl.h>

#include "DirectXCommon.h"

/// \brief スプライト共通部
class SpriteCommon
{
public:
	/// \brief 初期化
	void Initialize(DirectXCommon* dxCommon);

	/// \brief ルートシグネチャの生成
	void CreateRootSignature();

	/// \brief グラフィックスパイプラインの生成
	void CreateGraphicsPipelineState();

	/// \brief 共通描画設定
	void CommonRenderingsettings();

	DirectXCommon* GetDxCommon() const { return dxCommon_; }

private:
	/*--------------[ 描画ルールを司るもの ]-----------------*/

	//ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;

	//グラフィックスパイプラインステート
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;

	DirectXCommon* dxCommon_;

};

