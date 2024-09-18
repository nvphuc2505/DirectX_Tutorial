// Using Gauss Blur Algorithm

#pragma once

#include <DXUT.h>
#include <Windows.h>

class BlurFilter
{
public:
	BlurFilter();
	~BlurFilter();

	void Init(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format);
	void BlurInPlace(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* inputSRV, ID3D11UnorderedAccessView* inputUAV, int blurCount);

private:
	UINT mWidth;
	UINT mHeight;
	DXGI_FORMAT mFormat;

	ID3D11ShaderResourceView* mBlurredOutputTexSRV;
	ID3D11UnorderedAccessView* mBlurredOutputTexUAV;

public:
	ID3D11ShaderResourceView* GetBlurredOutput();

	void SetGaussianWeights(float sigma);
	void SetWeights(const float weights[9]);
};