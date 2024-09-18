#include <DXUT.h>
#include <SDKmisc.h>
#include <array>
#include "Waves.h"
#include "Vertex.h"
#include "LightHelper.h"
#include "MathHelper.h"
#include "GeometryGenerator.h"
#include "../Chapter11_GeometryShader/Effects.h"
#include "RenderStates.h"

#define WATER_TEXTURE L"D:/Work/DirectX/Chapter11_GeometryShader/Textures/water2.dds"
#define TREE0_TEXTURE L"D:/Work/DirectX/Chapter11_GeometryShader/Textures/tree0.dds"
#define TREE1_TEXTURE L"D:/Work/DirectX/Chapter11_GeometryShader/Textures/tree1.dds"
#define TREE2_TEXTURE L"D:/Work/DirectX/Chapter11_GeometryShader/Textures/tree2.dds"
#define TREE3_TEXTURE L"D:/Work/DirectX/Chapter11_GeometryShader/Textures/tree3.dds"

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------

ID3D11Buffer*			  g_TreeSpriteVertexBuffer;
ID3D11Buffer*			  g_TreeSpriteIndexBuffer;
UINT					  g_TreeSpriteCount;
XMFLOAT4X4				  g_TreeSpriteWorld;
ID3D11ShaderResourceView* g_TreeSpriteArrayMapSRV;

Waves					  g_Wave;
UINT					  g_WaveIndexCount;
ID3D11Buffer*			  g_WaveVertexBuffer;
ID3D11Buffer*			  g_WaveIndexBuffer;
XMFLOAT4X4				  g_WaveWorld;
ID3D11ShaderResourceView* g_WaveMapSRV;
XMFLOAT4X4				  g_WaveTextureTransform;
Material				  g_WaveMaterial;
XMFLOAT4X4				  g_WaterTexTransform;
XMFLOAT2A				  g_WaterTexOffset;

DirectionalLight		  g_DirectionLight[3];

XMMATRIX g_View;
XMMATRIX g_Proj;
XMFLOAT3 g_EyePosW;





//--------------------------------------------------------------------------------------
// Function Helper
//--------------------------------------------------------------------------------------
float GetHillHeight(float x, float z)
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 GetHillNormal(float x, float z)
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}

void BuildWavesGeometry(ID3D11Device* pd3dDevice)
{
	HRESULT hr = S_OK;

	g_Wave.Init(300, 300, 1.0f, 0.03f, 3.25f, 0.4f);

	// ===== WAVE VERTEX =====
	D3D11_BUFFER_DESC waveVBD = {};
	waveVBD.ByteWidth = sizeof(Vertex::Basic32) * g_Wave.VertexCount();
	waveVBD.Usage = D3D11_USAGE_DYNAMIC;
	waveVBD.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	waveVBD.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	waveVBD.MiscFlags = 0;
	hr = pd3dDevice->CreateBuffer(&waveVBD, 0, &g_WaveVertexBuffer);

	// ===== WAVE VERTEX =====
	std::vector<UINT> waveIndices(3 * g_Wave.TriangleCount());

	UINT m = g_Wave.RowCount();
	UINT n = g_Wave.ColumnCount();
	int k = 0;
	for (UINT i = 0; i < m - 1; ++i)
	{
		for (DWORD j = 0; j < n - 1; ++j)
		{
			waveIndices[k] = i * n + j;
			waveIndices[k + 1] = i * n + j + 1;
			waveIndices[k + 2] = (i + 1) * n + j;

			waveIndices[k + 3] = (i + 1) * n + j;
			waveIndices[k + 4] = i * n + j + 1;
			waveIndices[k + 5] = (i + 1) * n + j + 1;

			k += 6;
		}
	}
	D3D11_BUFFER_DESC waveIBD = {};
	waveIBD.Usage = D3D11_USAGE_IMMUTABLE;
	waveIBD.ByteWidth = sizeof(UINT) * waveIndices.size();
	waveIBD.BindFlags = D3D11_BIND_INDEX_BUFFER;
	waveIBD.CPUAccessFlags = 0;
	waveIBD.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA waveInitialData;
	waveInitialData.pSysMem = &waveIndices[0];
	hr = pd3dDevice->CreateBuffer(&waveIBD, &waveInitialData, &g_WaveIndexBuffer);
}

void BuildTreeSpriteBillboard(ID3D11Device* pd3dDevice)
{
	HRESULT hr = S_OK;

	struct TreeSpriteVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	//
	//	Vertex
	//
	g_TreeSpriteCount = 16;
	std::array <TreeSpriteVertex, 16> vertices;
	for (UINT i = 0; i < g_TreeSpriteCount; i++)
	{
		float x = MathHelper::RandF(-45.0f, 45.0f);
		float z = MathHelper::RandF(-45.0f, 45.0f);
		float y = GetHillHeight(x, z);
		y += 8.0f;

		vertices[i].Pos = XMFLOAT3(x, y, z);
		vertices[i].Size = XMFLOAT2(20.0f, 20.0f);
	}

	D3D11_BUFFER_DESC treeVBD = {};
	treeVBD.ByteWidth = sizeof(TreeSpriteVertex) * vertices.size();
	treeVBD.Usage = D3D11_USAGE_IMMUTABLE;
	treeVBD.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	treeVBD.CPUAccessFlags = 0;
	treeVBD.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA treeVInitData;
	treeVInitData.pSysMem = &vertices[0];
	hr = pd3dDevice->CreateBuffer(&treeVBD, &treeVInitData, &g_TreeSpriteVertexBuffer);

	//
	// INDEX
	//
	std::array <UINT, 16> indices = 
	{
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15
	};

	D3D11_BUFFER_DESC treeIBD = {};
	treeIBD.ByteWidth = sizeof(TreeSpriteVertex) * vertices.size();
	treeIBD.Usage = D3D11_USAGE_IMMUTABLE;
	treeIBD.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	treeIBD.CPUAccessFlags = 0;
	treeIBD.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA treeIInitData;
	treeIInitData.pSysMem = &indices[0];
	hr = pd3dDevice->CreateBuffer(&treeIBD, &treeIInitData, &g_TreeSpriteVertexBuffer);
}

ID3D11ShaderResourceView* CreateTexture2DArraySRV(
	ID3D11Device* device, ID3D11DeviceContext* context,
	std::vector<std::wstring>& filenames,
	DXGI_FORMAT format,
	UINT filter,
	UINT mipFilter)
{
	UINT size = filenames.size();

	std::vector <ID3D11Texture2D*> srcTex(size);
	for (int i = 0; i < size; i++)
	{
		
	}
}





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

	Effects::InitAll(pd3dDevice);
	InputLayouts::InitAll(pd3dDevice);
	// RenderStates::InitAll(pd3dDevice);

	//
	// Generate Lighting
	//
	g_DirectionLight[0].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	g_DirectionLight[0].Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	g_DirectionLight[0].Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	g_DirectionLight[0].Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	g_DirectionLight[1].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	g_DirectionLight[1].Diffuse = XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
	g_DirectionLight[1].Specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	g_DirectionLight[1].Direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

	g_DirectionLight[2].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	g_DirectionLight[2].Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	g_DirectionLight[2].Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	g_DirectionLight[2].Direction = XMFLOAT3(0.0f, -0.707f, -0.707f);



	//
	// Generate Waves
	//
	hr = DXUTCreateShaderResourceViewFromFile(pd3dDevice, WATER_TEXTURE, &g_WaveMapSRV);
	
	g_WaveMaterial.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	g_WaveMaterial.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	g_WaveMaterial.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);

	XMStoreFloat4x4(&g_WaveWorld, XMMatrixIdentity());

	BuildWavesGeometry(pd3dDevice);



	//
	// Generate Waves
	//
	BuildTreeSpriteBillboard(pd3dDevice);



	static const XMVECTORF32 s_Eye = { 0.0f, 150.0f, 200.0f, 0.f };
	static const XMVECTORF32 s_At = { 0.0f, 1.0f, 0.0f, 0.f };
	static const XMVECTORF32 s_Up = { 0.0f, 1.0f, 0.0f, 0.f };
	g_View = XMMatrixLookAtLH(s_Eye, s_At, s_Up);

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

	g_EyePosW = XMFLOAT3(x, y, z);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	// Every quarter second, generate a random wave.
	static float t_base = 0.0f;
	if ((fTime - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		DWORD i = 5 + rand() % (g_Wave.RowCount() - 10);
		DWORD j = 5 + rand() % (g_Wave.ColumnCount() - 10);

		float r = MathHelper::RandF(1.0f, 2.0f);

		g_Wave.Disturb(i, j, r);
	}

	// Update the wave simulation.
	g_Wave.Update(fElapsedTime);

	// Update the wave vertex buffer with the new solution.
	D3D11_MAPPED_SUBRESOURCE mappedData;
	auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
	pd3dImmediateContext->Map(g_WaveVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);

	Vertex::Basic32* v = reinterpret_cast<Vertex::Basic32*>(mappedData.pData);
	for (UINT i = 0; i < g_Wave.VertexCount(); ++i)
	{
		v[i].Pos = g_Wave[i];
		v[i].Normal = g_Wave.Normal(i);

		v[i].Tex.x = 0.5f + g_Wave[i].x / g_Wave.Width();
		v[i].Tex.y = 0.5f - g_Wave[i].z / g_Wave.Depth();
	}

	pd3dImmediateContext->Unmap(g_WaveVertexBuffer, 0);


	//
	// Animate water texture coordinates.
	//
	XMMATRIX wavesScale = XMMatrixScaling(5.0f, 5.0f, 0.0f);

	// Translate texture over time.
	g_WaterTexOffset.y += 0.05f * fElapsedTime;
	g_WaterTexOffset.x += 0.1f * fElapsedTime;
	XMMATRIX wavesOffset = XMMatrixTranslation(g_WaterTexOffset.x, g_WaterTexOffset.y, 0.0f);

	// Combine scale and translation.
	XMStoreFloat4x4(&g_WaterTexTransform, wavesScale * wavesOffset);
}





//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{

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
	pd3dImmediateContext->ClearRenderTargetView(pRTV, Colors::MidnightBlue);

	auto pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);

	//
	//	Input Layout
	//
	pd3dImmediateContext->IASetInputLayout(InputLayouts::Basic32);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;


	//
	// Set per frame constants.
	//
	Effects::BasicFX->SetDirLights(g_DirectionLight);
	Effects::BasicFX->SetEyePosW(g_EyePosW);
	Effects::BasicFX->SetFogColor(Colors::Silver);
	Effects::BasicFX->SetFogStart(100.0f);
	Effects::BasicFX->SetFogRange(200.0f);

	ID3DX11EffectTechnique* landAndWavesTech = Effects::BasicFX->Light3TexTech;

	D3DX11_TECHNIQUE_DESC techDesc;
	landAndWavesTech->GetDesc(&techDesc);

	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		//
		// Draw the waves.
		//
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_WaveVertexBuffer, &stride, &offset);
		pd3dImmediateContext->IASetIndexBuffer(g_WaveIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		XMMATRIX world = XMLoadFloat4x4(&g_WaveWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world * g_View * g_Proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&g_WaterTexTransform));
		Effects::BasicFX->SetMaterial(g_WaveMaterial);
		Effects::BasicFX->SetDiffuseMap(g_WaveMapSRV);

		// Set the blend state of the output-merger stage.
		// pd3dImmediateContext->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xffffffff);
		landAndWavesTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(3 * g_Wave.TriangleCount(), 0, 0);
		
		// Restore default blend state
		pd3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
	}
}





//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved(void* pUserContext)
{
	return true;
}





//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	float fAspect = static_cast<float>(pBackBufferSurfaceDesc->Width) / static_cast<float>(pBackBufferSurfaceDesc->Height);
	g_Proj = XMMatrixPerspectiveFovLH(XM_PI * 0.25f, fAspect, 0.1f, 1000.0f);

	return S_OK;
}





//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif


	DXUTSetCallbackFrameMove(OnFrameMove);
	DXUTSetCallbackKeyboard(OnKeyboard);
	DXUTSetCallbackMsgProc(MsgProc);
	DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);
	DXUTSetCallbackDeviceRemoved(OnDeviceRemoved);

	DXUTSetCallbackD3D11DeviceAcceptable(IsD3D11DeviceAcceptable);
	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
	DXUTSetCallbackD3D11SwapChainResized(OnD3D11ResizedSwapChain);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);
	DXUTSetCallbackD3D11SwapChainReleasing(OnD3D11ReleasingSwapChain);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);


	DXUTInit(true, true, nullptr);
	DXUTSetCursorSettings(true, true);
	DXUTCreateWindow(L"Billboard Demo");

	DXUTCreateDevice(D3D_FEATURE_LEVEL_10_0, true, 800, 600);
	DXUTMainLoop();


	return DXUTGetExitCode();
}