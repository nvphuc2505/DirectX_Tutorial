#include "Terrain.h"



Terrain::Terrain() :
	mQuadPatchVB(0),
	mQuadPatchIB(0),
	mLayerMapArraySRV(0),
	mBlendMapSRV(0),
	mHeightMapSRV(0),
	mNumPatchVertices(0),
	mNumPatchQuadFaces(0),
	mNumPatchVertRows(0),
	mNumPatchVertCols(0)
{
	XMStoreFloat4x4(&mWorld, XMMatrixIdentity());

	mMat.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mMat.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 64.0f);
	mMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
}
Terrain::~Terrain()
{
	SAFE_RELEASE(mQuadPatchVB);
	SAFE_RELEASE(mQuadPatchIB);
	SAFE_RELEASE(mLayerMapArraySRV);
	SAFE_RELEASE(mBlendMapSRV);
	SAFE_RELEASE(mHeightMapSRV);
}





bool Terrain::InBounds(int i, int j)
{
	return i >= 0 && i < (int)mInfo.HeightmapHeight 
		&& j >= 0 && j < (int)mInfo.HeightmapWidth;
}

float Terrain::Average(UINT i, UINT j)
{
	// ------------
	// | 1|  2 | 3|
	// ----------
	// | 4| ij | 6|
	// ----------
	// | 7|  8 | 9|
	// ------------

	float avg = 0.0f;
	float num = 0.0f;

	for (int m = i - 1; m <= i + 1; m++)
	{
		for (int n = j - 1; n <= j + 1; n++)
		{
			if (InBounds(m, n))
			{
				avg += mHeightmap[m * mInfo.HeightmapWidth + n];
				num += 1.0f;
			}
		}
	}

	return avg / num;
}

void Terrain::Smooth()
{
	std::vector<float> dest(mHeightmap.size());

	for (UINT i = 0; i < mInfo.HeightmapHeight; i++)
	{
		for (UINT j = 0; j < mInfo.HeightmapWidth; j++)
		{
			dest[i * mInfo.HeightmapWidth + j] = Average(i, j);
		}
	}

	mHeightmap = dest;
}





void Terrain::BuildHeightmapSRV(ID3D11Device* pd3dDevice)
{
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = mInfo.HeightmapWidth;
	texDesc.Height = mInfo.HeightmapHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R16_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	std::vector<HALF> hmap(mHeightmap.size());
	std::transform(mHeightmap.begin(), mHeightmap.end(), hmap.begin(), XMConvertFloatToHalf);

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &hmap[0];
	data.SysMemPitch = mInfo.HeightmapWidth * sizeof(HALF);
	data.SysMemSlicePitch = 0;

	ID3D11Texture2D* hmapTex = 0;
	HRESULT hr = pd3dDevice->CreateTexture2D(&texDesc, &data, &hmapTex);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	hr = pd3dDevice->CreateShaderResourceView(hmapTex, &srvDesc, &mHeightMapSRV);

	SAFE_RELEASE(hmapTex);
}