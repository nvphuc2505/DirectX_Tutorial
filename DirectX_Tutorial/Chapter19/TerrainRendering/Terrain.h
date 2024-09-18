#pragma once

#include <DXUT.h>
#include <SDKmisc.h>
#include <DXUTcamera.h>
#include <string>
#include <fstream>
#include <sstream>
#include "LightHelper.h"

using namespace DirectX;

class Terrain
{
public:
	struct InitInfo
	{
		std::wstring HeightMapFilename;
		std::wstring LayerMapFilename0;
		std::wstring LayerMapFilename1;
		std::wstring LayerMapFilename2;
		std::wstring LayerMapFilename3;
		std::wstring LayerMapFilename4;
		std::wstring BlendMapFilename;

		float HeightScale;

		UINT HeightmapWidth;
		UINT HeightmapHeight;

		// The cell spacing along the x- and z-axes
		float CellSpacing;
	};

public:
	Terrain();
	~Terrain();

	float GetWidth()const;
	float GetDepth()const;
	float GetHeight(float x, float z)const;

	void SetWorld(CXMMATRIX M);
	XMMATRIX GetWorld()const;

	void Init(ID3D11Device* device, ID3D11DeviceContext* dc, const InitInfo& initInfo);
	void Draw(ID3D11DeviceContext* dc, CModelViewerCamera camera, DirectionalLight lights[3]);

private:
	void LoadHeightmap();
	void Smooth();
	bool InBounds(int i, int j);
	float Average(UINT i, UINT j);
	void BuildQuadPatchVB(ID3D11Device* device);
	void BuildQuadPatchIB(ID3D11Device* device);
	void BuildHeightmapSRV(ID3D11Device* pd3dDevice);



private:
	
	static const int CellsPerPatch = 64;
	
	std::vector<float> mHeightmap;
	std::vector<XMFLOAT2> mPatchBoundsY;

	InitInfo mInfo;

	ID3D11Buffer* mQuadPatchVB;
	ID3D11Buffer* mQuadPatchIB;

	UINT mNumPatchVertices;
	UINT mNumPatchQuadFaces;

	UINT mNumPatchVertRows;
	UINT mNumPatchVertCols;

	XMFLOAT4X4 mWorld;
	Material mMat;

	ID3D11ShaderResourceView* mLayerMapArraySRV;
	ID3D11ShaderResourceView* mBlendMapSRV;
	ID3D11ShaderResourceView* mHeightMapSRV;
};