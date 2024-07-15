#include "Sprite.h"
#include "SpriteCommon.h"

void Sprite::Initialize(SpriteCommon* spriteCommon)
{
	//引数で受け取ってメンバ変数に記録する
	spriteCommon_ = spriteCommon;

	/*--------------[ 頂点データ ]-----------------*/

	//バッファリソース
	vertexResource_ = CreateBufferResource(spriteCommon_->GetDxCommon()->GetDevice(), sizeof(VertexData) * 4);
	indexResource_ = CreateBufferResource(spriteCommon_->GetDxCommon()->GetDevice(), sizeof(uint32_t) * 6);

	//バッファリソース内のデータ
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	////使用するリソースのサイズは頂点３つ分のサイズ
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * 1536;
	////1頂点当たりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	//使用するリソースのサイズをインデックス6つ分のサイズ
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
	//インデックスはuint32_tとする
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	//データを書き込むためのアドレスを取得して割り当てる
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));

	/*--------------[ マテリアル ]-----------------*/

	//バッファリソース
	materialResource_ = CreateBufferResource(spriteCommon_->GetDxCommon()->GetDevice(), sizeof(Material));
	//データを書き込むためのアドレスを取得して割り当てる
	materialResource_->Map(0,nullptr, reinterpret_cast<void**>(&materialData_));

	//マテリアルデータの初期値を書き込む
	materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData_->enableLighting = false;
	materialData_->uvTransform = MatrixUtils::MakeIdentity4x4();

	/*--------------[ 座標変換行列 ]-----------------*/

	//リソースを作る
	transformationMatrixResource_ = CreateBufferResource(spriteCommon_->GetDxCommon()->GetDevice(), sizeof(TransformationMatrix));
	//リソースにデータを書き込むためのアドレスを取得して割り当てる
	transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));

	//単位行列を書き込んでおく
	transformationMatrixData_->WVP = MatrixUtils::MakeIdentity4x4();
	transformationMatrixData_->World = MatrixUtils::MakeIdentity4x4();

}

void Sprite::Update()
{
	Transform transformSprite{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f}
	};

	Matrix4x4 worldMatrix = MatrixUtils::MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
	Matrix4x4 viewMatrix = MatrixUtils::MakeIdentity4x4();
	Matrix4x4 projectionMatrix = MatrixUtils::MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
	
	transformationMatrixData_->WVP = MatrixUtils::Multiply(worldMatrix, MatrixUtils::Multiply(viewMatrix, projectionMatrix));;
	transformationMatrixData_->World = worldMatrix;
}

void Sprite::Draw()
{
	/*--------------[ バッファービューの設定 ]-----------------*/

	spriteCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
	//IBVを設定
	spriteCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&indexBufferView_);

	/*--------------[ マテリアルの設定 ]-----------------*/

	//CBuffer
	spriteCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

	/*--------------[ 座標変換行列の設定 ]-----------------*/

	//TransformationMatrixCBufferの場所を設定
	spriteCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());

	/*--------------[ SRVのDescriptorTableの先頭を設定 ]-----------------*/

	//SRVのDescriptorTableの先頭を設定。2はrootPatameter[2]である。
	spriteCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(2,spriteCommon_->GetDxCommon()->GetSRVGPUDescriptorHandle(2));

	/*--------------[ ！！描画！！ ]-----------------*/

	spriteCommon_->GetDxCommon()->GetCommandList()->DrawInstanced(6, 1, 0, 0);
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes)
{
	Microsoft::WRL::ComPtr<ID3D12Resource> bufferResource = nullptr;
	//頂点リソース用のヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	//頂点リソースの設定
	D3D12_RESOURCE_DESC bufferResourceDesc{};
	//バッファリソース。テクスチャの場合はまた別の設定をする
	bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferResourceDesc.Width = sizeInBytes;
	//バッファの場合はこれらは１にする決まり
	bufferResourceDesc.Height = 1;
	bufferResourceDesc.DepthOrArraySize = 1;
	bufferResourceDesc.MipLevels = 1;
	bufferResourceDesc.SampleDesc.Count = 1;
	//バッファの場合はこれにする決まり
	bufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&bufferResource));
	assert(SUCCEEDED(hr));

	return bufferResource;
}
