#pragma once

#include <DXUT.h>

using namespace DirectX;

namespace Vertex
{
	struct Pos
	{
		float x;
		float y;
		float z;
	};

	struct Basic32
	{
		XMFLOAT3 Pos;
		XMFLOAT3 Normal;
		XMFLOAT2 Tex;
	};
}



class InputLayoutDesc
{
public:
	static const D3D11_INPUT_ELEMENT_DESC Pos[1];
	static const D3D11_INPUT_ELEMENT_DESC Basic32[3];
};



class InputLayouts
{
public:
	static void InitAll(ID3D11Device* device);
	static void DestroyAll();

	static ID3D11InputLayout* Basic32;
	static ID3D11InputLayout* Pos;
};