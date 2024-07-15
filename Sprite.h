#pragma once
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

#include "Matrix.h"
#include "Vector.h"

//頂点データ(構造体)
struct VertexData
{
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct Material
{
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
};

struct Transform
{
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

struct TransformationMatrix
{
	Matrix4x4 WVP;
	Matrix4x4 World;
};

class SpriteCommon;
class Sprite
{
public:
	/// \brief 初期化
	void Initialize(SpriteCommon* spriteCommon);

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

	/// \brief バッファリソースの生成
	/// \param device 
	/// \param sizeInBytes 
	/// \return 
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes);

private:
	SpriteCommon* spriteCommon_ = nullptr;

	/*--------------[ 頂点データ ]-----------------*/

	//バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	//バッファリソース内のデータを指すポインタ
	VertexData* vertexData_ = nullptr;
	uint32_t* indexData_ = nullptr;
	//バッファリソースの使い道を補足するバッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_;

	/*--------------[ マテリアル ]-----------------*/

	//バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	//マテリアルにデータを書き込む
	Material* materialData_ = nullptr;

	/*--------------[ 座標変換行列 ]-----------------*/

	//バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
	//バッファリソース内のデータを指すポインタ
	TransformationMatrix* transformationMatrixData_ = nullptr;



};

