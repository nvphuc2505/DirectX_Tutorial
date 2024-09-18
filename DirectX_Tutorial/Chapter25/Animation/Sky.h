#pragma once

#include <DXUT.h>
#include <SDKmisc.h>
#include <DXUTcamera.h>
#include "Vertex.h"
#include "Effects.h"
#include "GeometryGenerator.h"
#include <vector>

using namespace DirectX;

class Sky
{
public:
	Sky(ID3D11Device* pd3dDevice, LPCWCHAR cubemapFilename, float skySphereRadius);
	~Sky();

	ID3D11ShaderResourceView* GetSkyCubeMap();

	void Draw(ID3D11DeviceContext* pd3dImmediateContext, CModelViewerCamera camera);


private:
	ID3D11Buffer* m_SkyVertexBuffer;
	ID3D11Buffer* m_SkyIndexBuffer;
	UINT		  m_SkyIndexCount;

	ID3D11ShaderResourceView* m_SkyMapSRV;
};