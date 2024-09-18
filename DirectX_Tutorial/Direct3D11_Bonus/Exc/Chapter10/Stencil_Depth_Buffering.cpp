#include <DXUT.h>
#include <SDKmisc.h>
#include <d3dx11effect.h>
#include <vector>
#include <fstream>
#include "LightHelper.h"
#include "MathHelper.h"
#include "GeometryGenerator.h"
#include "Vertex.h"
#include "RenderStates.h"
#include "Effects.h"

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------

ID3D11Buffer*		g_RoomVertexBuffer;
XMFLOAT4X4			g_RoomWorld;

ID3D11Buffer*		g_SkullVertexBuffer;
ID3D11Buffer*		g_SkullIndexBuffer;
UINT				g_SkullIndexCount;
XMFLOAT3			g_SkullTranslation;
XMFLOAT4X4			g_SkullWorld;

DirectionalLight	g_DirLights[3];
Material			g_RoomMat;
Material			g_SkullMat;
Material			g_MirrorMat;
Material			g_ShadowMat;

XMMATRIX			g_View;
XMMATRIX			g_Proj;
XMFLOAT3			g_EyeToWorld;

ID3D11ShaderResourceView*		g_FloorDiffuseMapSRV;
ID3D11ShaderResourceView*		g_WallDiffuseMapSRV;
ID3D11ShaderResourceView*		g_MirrorDiffuseMapSRV;


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo* AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo* DeviceInfo,
	DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext)
{
	return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext)
{
	return true;
}



//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	HRESULT hr = S_OK;

	//
	// Build Room Geometry Buffers
	//
	//   |--------------|
	//   |              |
	//   |----|----|----|
	//   |Wall|Mirr|Wall|
	//   |    | or |    |
	//   /--------------/
	//  /   Floor      /
	// /--------------/

	Vertex::Basic32 v[30];

	// Floor: Observe we tile texture coordinates.
	v[0] = Vertex::Basic32(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f);
	v[1] = Vertex::Basic32(-3.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	v[2] = Vertex::Basic32(7.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f);

	v[3] = Vertex::Basic32(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f);
	v[4] = Vertex::Basic32(7.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f);
	v[5] = Vertex::Basic32(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f);

	// Wall: Observe we tile texture coordinates, and that we
	// leave a gap in the middle for the mirror.
	v[6] = Vertex::Basic32(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[7] = Vertex::Basic32(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[8] = Vertex::Basic32(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f);

	v[9] = Vertex::Basic32(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[10] = Vertex::Basic32(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f);
	v[11] = Vertex::Basic32(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f);

	v[12] = Vertex::Basic32(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[13] = Vertex::Basic32(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[14] = Vertex::Basic32(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f);

	v[15] = Vertex::Basic32(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[16] = Vertex::Basic32(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f);
	v[17] = Vertex::Basic32(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f);

	v[18] = Vertex::Basic32(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[19] = Vertex::Basic32(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[20] = Vertex::Basic32(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f);

	v[21] = Vertex::Basic32(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[22] = Vertex::Basic32(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f);
	v[23] = Vertex::Basic32(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f);

	// Mirror
	v[24] = Vertex::Basic32(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[25] = Vertex::Basic32(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[26] = Vertex::Basic32(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);

	v[27] = Vertex::Basic32(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[28] = Vertex::Basic32(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[29] = Vertex::Basic32(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	D3D11_BUFFER_DESC roomBuffer = {};
	roomBuffer.ByteWidth = sizeof(Vertex::Basic32) * 30;
	roomBuffer.Usage = D3D11_USAGE_IMMUTABLE;
	roomBuffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	roomBuffer.CPUAccessFlags = 0;
	roomBuffer.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA roomVInitData = {};
	roomVInitData.pSysMem = v;
	V_RETURN(pd3dDevice->CreateBuffer(&roomBuffer, &roomVInitData, &g_RoomVertexBuffer));

	//
	// Buld Skull Geometry Buffers
	//
	std::ifstream fin("D:/Work/Direct3D11/Exc/Chapter10/Models/skull.txt");
	if (!fin)
	{
		MessageBox(0, L"skull.txt not found!", 0, 0);
		return S_FALSE;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	std::vector <Vertex::Basic32> skullVertices(vcount);
	for (UINT i = 0; i < vcount; i++)
	{
		fin >> skullVertices[i].Pos.x >> skullVertices[i].Pos.y >> skullVertices[i].Pos.z;
		fin >> skullVertices[i].Normal.x >> skullVertices[i].Normal.y >> skullVertices[i].Normal.z;
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	g_SkullIndexCount = 3 * tcount;
	std::vector<UINT> skullIndices(g_SkullIndexCount);
	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> skullIndices[i * 3 + 0] >> skullIndices[i * 3 + 1] >> skullIndices[i * 3 + 2];
	}

	fin.close();

	D3D11_BUFFER_DESC skullVBD = {};
	skullVBD.ByteWidth = sizeof(Vertex::Basic32) * vcount;
	skullVBD.Usage = D3D11_USAGE_IMMUTABLE;
	skullVBD.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	skullVBD.CPUAccessFlags = 0;
	skullVBD.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA skullVInitData;
	skullVInitData.pSysMem = &skullVertices[0];
	V_RETURN(pd3dDevice->CreateBuffer(&skullVBD, &skullVInitData, &g_SkullVertexBuffer));

	D3D11_BUFFER_DESC skullIBD = {};
	skullIBD.ByteWidth = sizeof(UINT) * g_SkullIndexCount;
	skullIBD.Usage = D3D11_USAGE_IMMUTABLE;
	skullIBD.BindFlags = D3D11_BIND_INDEX_BUFFER;
	skullIBD.CPUAccessFlags = 0;
	skullIBD.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA skullIInitData;
	skullIInitData.pSysMem = &skullIndices[0];
	V_RETURN(pd3dDevice->CreateBuffer(&skullIBD, &skullIInitData, &g_SkullIndexBuffer));

	
	
	//
	// Matrix
	//
	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&g_RoomWorld, I);
	
	static const XMVECTORF32 s_Eye = { 10.0f, 8.0f, -10.0f, 0.f };
	static const XMVECTORF32 s_At = { 0.0f, 1.0f, 0.0f, 0.f };
	static const XMVECTORF32 s_Up = { 0.0f, 1.0f, 0.0f, 0.f };
	g_View = XMMatrixLookAtLH(s_Eye, s_At, s_Up);

	//
	// Light
	//
	g_DirLights[0].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	g_DirLights[0].Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	g_DirLights[0].Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	g_DirLights[0].Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	g_DirLights[1].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	g_DirLights[1].Diffuse = XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
	g_DirLights[1].Specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	g_DirLights[1].Direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

	g_DirLights[2].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	g_DirLights[2].Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	g_DirLights[2].Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	g_DirLights[2].Direction = XMFLOAT3(0.0f, -0.707f, -0.707f);

	//
	// Matertial
	//
	g_RoomMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	g_RoomMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	g_RoomMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

	g_SkullMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	g_SkullMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	g_SkullMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

	g_MirrorMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	g_MirrorMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	g_MirrorMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

	g_ShadowMat.Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	g_ShadowMat.Diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	g_ShadowMat.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);

	Effects::InitAll(pd3dDevice);
	InputLayouts::InitAll(pd3dDevice);
	RenderStates::InitAll(pd3dDevice);

	//
	// Shader Resource View
	//
	V_RETURN(DXUTCreateShaderResourceViewFromFile(pd3dDevice, L"D:/Work/Direct3D11/Exc/Chapter10/Textures/brick01.dds", &g_WallDiffuseMapSRV));
	V_RETURN(DXUTCreateShaderResourceViewFromFile(pd3dDevice, L"D:/Work/Direct3D11/Exc/Chapter10/Textures/ice.dds", &g_MirrorDiffuseMapSRV));
	V_RETURN(DXUTCreateShaderResourceViewFromFile(pd3dDevice, L"D:/Work/Direct3D11/Exc/Chapter10/Textures/checkboard.dds", &g_FloorDiffuseMapSRV));

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext)
{
}



//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	bool* pbNoFurtherProcessing, void* pUserContext)
{
	return 0;
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	// Setup the projection parameters
	float fAspect = static_cast<float>(pBackBufferSurfaceDesc->Width) / static_cast<float>(pBackBufferSurfaceDesc->Height);
	g_Proj = XMMatrixPerspectiveFovLH(XM_PI * 0.25f, fAspect, 0.1f, 1000.0f);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	// Convert Spherical to Cartesian coordinates.
	float x = 80.0f * sinf(0.1f * XM_PI) * cosf(1.5f * XM_PI);
	float z = 80.0f * sinf(0.1f * XM_PI) * sinf(1.5f * XM_PI);
	float y = 80.0f * cosf(0.1f * XM_PI);
	g_EyeToWorld = XMFLOAT3(x, y, z);

	if (GetAsyncKeyState('A') & 0x8000)
		g_SkullTranslation.x -= 2.0f * fElapsedTime;

	if (GetAsyncKeyState('D') & 0x8000)
		g_SkullTranslation.x += 2.0f * fElapsedTime;

	if (GetAsyncKeyState('W') & 0x8000)
		g_SkullTranslation.y += 1.0f * fElapsedTime;

	if (GetAsyncKeyState('S') & 0x8000)
		g_SkullTranslation.y -= 1.0f * fElapsedTime;

	if (GetAsyncKeyState('Q') & 0x8000)
		g_SkullTranslation.z += 1.0f * fElapsedTime;

	if (GetAsyncKeyState('E') & 0x8000)
		g_SkullTranslation.z -= 1.0f * fElapsedTime;

	// Don't let user move below ground plane.
	g_SkullTranslation.y = MathHelper::Max(g_SkullTranslation.y, 0.0f);

	XMMATRIX skullRotate = XMMatrixRotationY(0.5f * MathHelper::Pi);
	XMMATRIX skullScale = XMMatrixScaling(0.45f, 0.45f, 0.45f);
	XMMATRIX skullOffset = XMMatrixTranslation(g_SkullTranslation.x, g_SkullTranslation.y, g_SkullTranslation.z);
	XMStoreFloat4x4(&g_SkullWorld, skullRotate * skullScale * skullOffset);
}

//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
	if (bKeyDown)
	{
		switch (nChar)
		{
		case VK_F1: // Change as needed                
			break;
		}
	}
}


// --------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
	double fTime, float fElapsedTime, void* pUserContext)
{
	auto pRTV = DXUTGetD3D11RenderTargetView();
	pd3dImmediateContext->ClearRenderTargetView(pRTV, Colors::Black);

	auto pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	pd3dImmediateContext->IASetInputLayout(InputLayouts::Basic32);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	Effects::BasicFX->SetDirLights(g_DirLights);
	Effects::BasicFX->SetEyePosW(g_EyeToWorld);
	Effects::BasicFX->SetFogColor(Colors::Black);
	Effects::BasicFX->SetFogStart(2.0f);
	Effects::BasicFX->SetFogRange(40.0f);

	ID3DX11EffectTechnique* activeTech = Effects::BasicFX->Light3TexTech;
	ID3DX11EffectTechnique* activeSkullTech = Effects::BasicFX->Light3Tech;
	D3DX11_TECHNIQUE_DESC techDesc;

	//
	// Draw the floor and walls to the back buffer as normal.
	//
	activeTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; p++)
	{
		ID3DX11EffectPass* pass = activeTech->GetPassByIndex(p);

		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_RoomVertexBuffer, &stride, &offset);

		// Set per object constants.
		XMMATRIX world = XMLoadFloat4x4(&g_RoomWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world * g_View * g_Proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
		Effects::BasicFX->SetMaterial(g_RoomMat);

		// Floor
		Effects::BasicFX->SetDiffuseMap(g_FloorDiffuseMapSRV);
		pass->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->Draw(6, 0);

		// Wall
		Effects::BasicFX->SetDiffuseMap(g_WallDiffuseMapSRV);
		pass->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->Draw(18, 6);
	}


	//
	// Draw the skull to the back buffer as normal.
	//
	activeSkullTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; p++)
	{
		ID3DX11EffectPass* pass = activeSkullTech->GetPassByIndex(p);

		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_SkullVertexBuffer, &stride, &offset);
		pd3dImmediateContext->IASetIndexBuffer(g_SkullIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		XMMATRIX world = XMLoadFloat4x4(&g_SkullWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world * g_View * g_Proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetMaterial(g_SkullMat);

		pass->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(g_SkullIndexCount, 0, 0);
	}


	//
	// Draw the mirror to stencil buffer only.
	//
	activeTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		ID3DX11EffectPass* pass = activeTech->GetPassByIndex(p);

		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_RoomVertexBuffer, &stride, &offset);

		XMMATRIX world = XMLoadFloat4x4(&g_RoomWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world * g_View * g_Proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());

		pd3dImmediateContext->OMSetBlendState(RenderStates::NoRenderTargetWritesBS, blendFactor, 0xffffffff);
		pd3dImmediateContext->OMSetDepthStencilState(RenderStates::MarkMirrorDSS, 1);

		pass->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->Draw(6, 24);

		// Restore states.
		pd3dImmediateContext->OMSetDepthStencilState(0, 0);
		pd3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
	}


	//
	// Draw the skull reflection.
	//
	activeSkullTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		ID3DX11EffectPass* pass = activeSkullTech->GetPassByIndex(p);

		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_SkullVertexBuffer, &stride, &offset);
		pd3dImmediateContext->IASetIndexBuffer(g_SkullIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
		XMMATRIX R = XMMatrixReflect(mirrorPlane);
		XMMATRIX world = XMLoadFloat4x4(&g_SkullWorld) * R;
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world * g_View * g_Proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetMaterial(g_SkullMat);

		// Cache the old light directions, and reflect the light directions.
		XMFLOAT3 oldLightDirections[3];
		for (int i = 0; i < 3; ++i)
		{
			oldLightDirections[i] = g_DirLights[i].Direction;

			XMVECTOR lightDir = XMLoadFloat3(&g_DirLights[i].Direction);
			XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
			XMStoreFloat3(&g_DirLights[i].Direction, reflectedLightDir);
		}

		Effects::BasicFX->SetDirLights(g_DirLights);

		// Cull clockwise triangles for reflection.
		pd3dImmediateContext->RSSetState(RenderStates::CullClockwiseRS);

		// Only draw reflection into visible mirror pixels as marked by the stencil buffer. 
		pd3dImmediateContext->OMSetDepthStencilState(RenderStates::DrawReflectionDSS, 1);
		pass->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(g_SkullIndexCount, 0, 0);

		// Restore default states.
		pd3dImmediateContext->RSSetState(0);
		pd3dImmediateContext->OMSetDepthStencilState(0, 0);

		// Restore light directions.
		for (int i = 0; i < 3; ++i)
		{
			g_DirLights[i].Direction = oldLightDirections[i];
		}

		Effects::BasicFX->SetDirLights(g_DirLights);
	}


	//
	// Draw the mirror to the back buffer as usual but with transparency
	// blending so the reflection shows through.
	// 
	activeTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		ID3DX11EffectPass* pass = activeTech->GetPassByIndex(p);

		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_RoomVertexBuffer, &stride, &offset);

		// Set per object constants.
		XMMATRIX world = XMLoadFloat4x4(&g_RoomWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world * g_View * g_Proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
		Effects::BasicFX->SetMaterial(g_MirrorMat);
		Effects::BasicFX->SetDiffuseMap(g_MirrorDiffuseMapSRV);

		// Mirror
		pd3dImmediateContext->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xffffffff);
		pass->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->Draw(6, 24);
	}


	//
	// Draw the skull shadow.
	//
	activeSkullTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; p++)
	{
		ID3DX11EffectPass* pass = activeSkullTech->GetPassByIndex(p);
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_SkullVertexBuffer, &stride, &offset);
		pd3dImmediateContext->IASetIndexBuffer(g_SkullIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		XMVECTOR shadowPlane = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);;
		XMVECTOR toMainLight = -XMLoadFloat3(&g_DirLights[0].Direction);
		XMMATRIX S = XMMatrixShadow(shadowPlane, toMainLight);
		XMMATRIX shadowOffsetY = XMMatrixTranslation(0.0f, 0.001f, 0.0f);

		// Set per object constants.
		XMMATRIX world = XMLoadFloat4x4(&g_SkullWorld) * S * shadowOffsetY;
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world * g_View * g_Proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetMaterial(g_ShadowMat);

		pd3dImmediateContext->OMSetDepthStencilState(RenderStates::NoDoubleBlendDSS, 0);
		pass->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(g_SkullIndexCount, 0, 0);

		// Restore default states.
		pd3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
		pd3dImmediateContext->OMSetDepthStencilState(0, 0);
	}
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
	SAFE_RELEASE(g_SkullVertexBuffer);
	SAFE_RELEASE(g_SkullIndexBuffer);

	SAFE_RELEASE(g_RoomVertexBuffer);

	SAFE_RELEASE(g_FloorDiffuseMapSRV);
	SAFE_RELEASE(g_WallDiffuseMapSRV)
	SAFE_RELEASE(g_MirrorDiffuseMapSRV);

	Effects::DestroyAll();
	InputLayouts::DestroyAll();
}

//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved(void* pUserContext)
{
	return true;
}


//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	// Enable run-time memory check for debug builds.
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// DXUT will create and use the best device
	// that is available on the system depending on which D3D callbacks are set below

	// Set general DXUT callbacks
	DXUTSetCallbackFrameMove(OnFrameMove);
	DXUTSetCallbackKeyboard(OnKeyboard);
	DXUTSetCallbackMsgProc(MsgProc);
	DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);
	DXUTSetCallbackDeviceRemoved(OnDeviceRemoved);

	// Set the D3D11 DXUT callbacks. Remove these sets if the app doesn't need to support D3D11
	DXUTSetCallbackD3D11DeviceAcceptable(IsD3D11DeviceAcceptable);
	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice); /****/
	DXUTSetCallbackD3D11SwapChainResized(OnD3D11ResizedSwapChain);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);
	DXUTSetCallbackD3D11SwapChainReleasing(OnD3D11ReleasingSwapChain);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);

	// Perform any application-level initialization here

	DXUTInit(true, true, nullptr); // Parse the command line, show msgboxes on error, no extra command line params
	DXUTSetCursorSettings(true, true); // Show the cursor and clip it when in full screen
	DXUTCreateWindow(L"HillsDemo");

	// Only require 10-level hardware or later
	DXUTCreateDevice(D3D_FEATURE_LEVEL_10_0, true, 800, 600);
	DXUTMainLoop(); // Enter into the DXUT render loop

	// Perform any application-level cleanup here

	return DXUTGetExitCode();
}