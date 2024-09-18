#include <DXUT.h>
#include <SDKmisc.h>
#include "MathHelper.h"
#include "LightHelper.h"
#include "Waves.h"
#include "Vertex.h"
#include "Effects.h"
#include "BlurFilter.h"
#include "GeometryGenerator.h"
#include "RenderStates.h"

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------

ID3D11Buffer*			  g_ScreenQuadVertexBuffer;
ID3D11Buffer*			  g_ScreenQuadIndexBuffer;

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

UINT					  g_LandIndexCount;
ID3D11Buffer*			  g_LandVertexBuffer;
ID3D11Buffer*			  g_LandIndexBuffer;
XMFLOAT4X4				  g_LandWorld;
ID3D11ShaderResourceView* g_LandMapSRV;
XMFLOAT4X4				  g_LandTextureTransform;
Material				  g_LandMaterial;

ID3D11Buffer*			  g_BoxVertexBuffer;
ID3D11Buffer*			  g_BoxIndexBuffer;
XMFLOAT4X4				  g_BoxWorld;
ID3D11ShaderResourceView* g_CrateMapSRV;
Material				  g_BoxMaterial;
UINT					  g_BoxVertexOffset;
UINT					  g_BoxIndexOffset;
UINT					  g_BoxIndexCount;
XMFLOAT4X4				  g_CrateTextureTransform;

DirectionalLight		  g_DirectionLight[3];

XMMATRIX g_View;
XMMATRIX g_Proj;
XMFLOAT3 g_EyePosW;

BlurFilter				    g_Blur;
ID3D11ShaderResourceView*	g_OffscreenSRV;
ID3D11UnorderedAccessView*	g_OffscreenUAV;
ID3D11RenderTargetView*		g_OffscreenRTV;






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
	//
	//
	// ===== MATRIX =====
	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&g_LandWorld, I);
	XMStoreFloat4x4(&g_WaveWorld, I);
	XMStoreFloat4x4(&g_CrateTextureTransform, I);

	XMMATRIX grassTexScale = XMMatrixScaling(5.0f, 5.0f, 0.0f);
	XMStoreFloat4x4(&g_LandTextureTransform, grassTexScale);

	XMMATRIX boxScale = XMMatrixScaling(15.0f, 15.0f, 15.0f);
	XMMATRIX boxOffset = XMMatrixTranslation(10.0f, 20.0f, -10.0f);
	XMStoreFloat4x4(&g_BoxWorld, boxScale * boxOffset);

	static const XMVECTORF32 s_Eye = { 0.0f, 150.0f, 200.0f, 0.f };
	static const XMVECTORF32 s_At = { 0.0f, 1.0f, 0.0f, 0.f };
	static const XMVECTORF32 s_Up = { 0.0f, 1.0f, 0.0f, 0.f };
	g_View = XMMatrixLookAtLH(s_Eye, s_At, s_Up);

	Effects::InitAll(pd3dDevice);
	InputLayouts::InitAll(pd3dDevice);
	RenderStates::InitAll(pd3dDevice);

	hr = DXUTCreateShaderResourceViewFromFile(pd3dDevice, L"D:/Work/Direct3D11/Exc/Chapter12/Blur/Textures/grass.dds", &g_LandMapSRV);
	hr = DXUTCreateShaderResourceViewFromFile(pd3dDevice, L"D:/Work/Direct3D11/Exc/Chapter12/Blur/Textures/water2.dds", &g_WaveMapSRV);
	hr = DXUTCreateShaderResourceViewFromFile(pd3dDevice, L"D:/Work/Direct3D11/Exc/Chapter12/Blur/Textures/WireFence.dds", &g_CrateMapSRV);

	g_LandMaterial.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	g_LandMaterial.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	g_LandMaterial.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	g_WaveMaterial.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	g_WaveMaterial.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	g_WaveMaterial.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);

	g_BoxMaterial.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	g_BoxMaterial.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	g_BoxMaterial.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

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
	// Build Crate Geometry
	//
	GeometryGenerator::MeshData box;
	GeometryGenerator geoGeneration;
	geoGeneration.CreateBox(1.0f, 1.0f, 1.0f, box);
	g_BoxVertexOffset = 0;
	g_BoxIndexOffset = 0;
	g_BoxIndexCount = box.Indices.size();
	UINT boxTotalVertexCount = box.Vertices.size();
	UINT boxTotalIndexCount = g_BoxIndexCount;

	std::vector<Vertex::Basic32> boxVertices(box.Vertices.size());
	for (size_t i = 0; i < box.Vertices.size(); i++)
	{
		boxVertices[i].Pos = box.Vertices[i].Position;
		boxVertices[i].Normal = box.Vertices[i].Normal;
		boxVertices[i].Tex = box.Vertices[i].TexC;
	}
	D3D11_BUFFER_DESC boxVBD = {};
	boxVBD.ByteWidth = box.Vertices.size() * sizeof(Vertex::Basic32);
	boxVBD.Usage = D3D11_USAGE_IMMUTABLE;
	boxVBD.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	boxVBD.CPUAccessFlags = 0;
	boxVBD.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA boxVInitData = {};
	boxVInitData.pSysMem = &boxVertices[0];
	V_RETURN(pd3dDevice->CreateBuffer(&boxVBD, &boxVInitData, &g_BoxVertexBuffer));

	D3D11_BUFFER_DESC boxIBD = {};
	boxIBD.ByteWidth = g_BoxIndexCount * sizeof(UINT);
	boxIBD.Usage = D3D11_USAGE_IMMUTABLE;
	boxIBD.BindFlags = D3D11_BIND_INDEX_BUFFER;
	boxIBD.CPUAccessFlags = 0;
	boxIBD.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA boxIInitData = {};
	boxIInitData.pSysMem = &box.Indices[0];
	V_RETURN(pd3dDevice->CreateBuffer(&boxIBD, &boxIInitData, &g_BoxIndexBuffer));





	//
	// Build Wave Geometry 
	//
	g_Wave.Init(300, 300, 1.0f, 0.03f, 3.25f, 0.4f);

	// ===== WAVE VERTEX =====
	D3D11_BUFFER_DESC waveVBD = {};
	waveVBD.ByteWidth = sizeof(Vertex::Basic32) * g_Wave.VertexCount();
	waveVBD.Usage = D3D11_USAGE_DYNAMIC;
	waveVBD.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	waveVBD.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	waveVBD.MiscFlags = 0;
	V_RETURN(pd3dDevice->CreateBuffer(&waveVBD, 0, &g_WaveVertexBuffer));

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
	V_RETURN(pd3dDevice->CreateBuffer(&waveIBD, &waveInitialData, &g_WaveIndexBuffer));





	//
	// Build Land Geometry 
	//
	GeometryGenerator::MeshData grid;
	GeometryGenerator geoGen;
	geoGen.CreateGrid(300.0f, 300.0f, 200, 200, grid);
	g_LandIndexCount = grid.Indices.size();

	// ===== LAND VERTEX =====
	std::vector<Vertex::Basic32> landVertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		XMFLOAT3 p = grid.Vertices[i].Position;

		p.y = GetHillHeight(p.x, p.z);

		landVertices[i].Pos = p;
		landVertices[i].Normal = GetHillNormal(p.x, p.z);
		landVertices[i].Tex = grid.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC landVBD = {};
	landVBD.ByteWidth = sizeof(Vertex::Basic32) * grid.Vertices.size();
	landVBD.Usage = D3D11_USAGE_IMMUTABLE;
	landVBD.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	landVBD.CPUAccessFlags = 0;
	landVBD.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA landVInitData = {};
	landVInitData.pSysMem = &landVertices[0];
	V_RETURN(pd3dDevice->CreateBuffer(&landVBD, &landVInitData, &g_LandVertexBuffer));

	// ===== LAND INDEX =====
	D3D11_BUFFER_DESC landIBD = { };
	landIBD.Usage = D3D11_USAGE_IMMUTABLE;
	landIBD.ByteWidth = sizeof(UINT) * g_LandIndexCount;
	landIBD.CPUAccessFlags = 0;
	landIBD.MiscFlags = 0;
	landIBD.BindFlags = D3D10_BIND_INDEX_BUFFER;
	D3D11_SUBRESOURCE_DATA landIInitialData = {};
	landIInitialData.pSysMem = &grid.Indices[0];
	V_RETURN(pd3dDevice->CreateBuffer(&landIBD, &landIInitialData, &g_LandIndexBuffer));





	//
	// Build Screen Quad Geometry Buffers
	//
	GeometryGenerator::MeshData quad;
	GeometryGenerator screenQuadGeoGen;
	screenQuadGeoGen.CreateFullscreenQuad(quad);

	std::vector<Vertex::Basic32> screenQuadVertices(quad.Vertices.size());
	for (UINT i = 0; i < quad.Vertices.size(); ++i)
	{
		screenQuadVertices[i].Pos = quad.Vertices[i].Position;
		screenQuadVertices[i].Normal = quad.Vertices[i].Normal;
		screenQuadVertices[i].Tex = quad.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC screenQuadVBD;
	screenQuadVBD.Usage = D3D11_USAGE_IMMUTABLE;
	screenQuadVBD.ByteWidth = sizeof(Vertex::Basic32) * quad.Vertices.size();
	screenQuadVBD.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	screenQuadVBD.CPUAccessFlags = 0;
	screenQuadVBD.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA screenQuadVInitData;
	screenQuadVInitData.pSysMem = &screenQuadVertices[0];
	V_RETURN(pd3dDevice->CreateBuffer(&screenQuadVBD, &screenQuadVInitData, &g_ScreenQuadVertexBuffer));

	D3D11_BUFFER_DESC screenQuadIBD;
	screenQuadIBD.Usage = D3D11_USAGE_IMMUTABLE;
	screenQuadIBD.ByteWidth = sizeof(UINT) * quad.Indices.size();
	screenQuadIBD.BindFlags = D3D11_BIND_INDEX_BUFFER;
	screenQuadIBD.CPUAccessFlags = 0;
	screenQuadIBD.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA screenQuadIInitData;
	screenQuadIInitData.pSysMem = &quad.Indices[0];
	V_RETURN(pd3dDevice->CreateBuffer(&screenQuadIBD, &screenQuadIInitData, &g_ScreenQuadIndexBuffer));





	//
	// Build Offscreen Views
	//
	SAFE_RELEASE(g_OffscreenSRV);
	SAFE_RELEASE(g_OffscreenRTV);
	SAFE_RELEASE(g_OffscreenUAV);

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = 800;
	texDesc.Height = 600;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	ID3D11Texture2D* offscreenTex = 0;
	V_RETURN(pd3dDevice->CreateTexture2D(&texDesc, 0, &offscreenTex));

	V_RETURN(pd3dDevice->CreateShaderResourceView(offscreenTex, 0, &g_OffscreenSRV));
	V_RETURN(pd3dDevice->CreateRenderTargetView(offscreenTex, 0, &g_OffscreenRTV));
	V_RETURN(pd3dDevice->CreateUnorderedAccessView(offscreenTex, 0, &g_OffscreenUAV));

	SAFE_RELEASE(offscreenTex);





	return S_OK;
}





//--------------------------------------------------------------------------------------
// Handle updates to the scene.
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	/*
	XMMATRIX boxRotate = XMMatrixRotationY(45.0f * XMConvertToRadians((float)fTime));
	XMMATRIX boxScale = XMMatrixScaling(45.0f, 45.0f, 45.0f);
	XMStoreFloat4x4(&g_BoxWorld, boxRotate * boxScale);
	*/

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



	// Switch the render mode based in key input.
	/*
	if (GetAsyncKeyState('1') & 0x8000)
		g_RenderOptions = RenderOptions::Lighting;
	if (GetAsyncKeyState('2') & 0x8000)
		g_RenderOptions = RenderOptions::Textures;
	if (GetAsyncKeyState('3') & 0x8000)
		g_RenderOptions = RenderOptions::TexturesAndFog;
	*/
}

// --------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
	double fTime, float fElapsedTime, void* pUserContext)
{
	HRESULT hr = S_OK;

	auto pDSV	   = DXUTGetD3D11DepthStencilView();
	auto pRTV	   = DXUTGetD3D11RenderTargetView();
	auto swapChain = DXUTGetDXGISwapChain();

	ID3D11RenderTargetView* renderTargetView[1] = { g_OffscreenRTV };
	pd3dImmediateContext->OMSetRenderTargets(1, renderTargetView, pDSV);
	pd3dImmediateContext->ClearRenderTargetView(g_OffscreenRTV, reinterpret_cast<const float*>(&Colors::Silver));
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);



	//
	// DrawWrapper();
	//
	pd3dImmediateContext->IASetInputLayout(InputLayouts::Basic32);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	XMMATRIX viewProj = g_View * g_Proj;
	Effects::BasicFX->SetDirLights(g_DirectionLight);
	Effects::BasicFX->SetEyePosW(g_EyePosW);
	Effects::BasicFX->SetFogColor(Colors::Silver);
	Effects::BasicFX->SetFogStart(15.0f);
	Effects::BasicFX->SetFogRange(175.0f);

	ID3DX11EffectTechnique* boxTech = Effects::BasicFX->Light3TexAlphaClipTech;
	ID3DX11EffectTechnique* landAndWavesTech = Effects::BasicFX->Light3TexTech;

	D3DX11_TECHNIQUE_DESC techDesc;

	boxTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; p++)
	{
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_BoxVertexBuffer, &stride, &offset);
		pd3dImmediateContext->IASetIndexBuffer(g_BoxIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		XMMATRIX world = XMLoadFloat4x4(&g_BoxWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world * g_View * g_Proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
		Effects::BasicFX->SetMaterial(g_BoxMaterial);
		Effects::BasicFX->SetDiffuseMap(g_CrateMapSRV);

		pd3dImmediateContext->RSSetState(RenderStates::NoCullRS);
		boxTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(36, 0, 0);

		// Restore default render state.
		pd3dImmediateContext->RSSetState(0);
	}

	landAndWavesTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; p++)
	{
		//
		// Draw the hills.
		//
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_LandVertexBuffer, &stride, &offset);
		pd3dImmediateContext->IASetIndexBuffer(g_LandIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		XMMATRIX world = XMLoadFloat4x4(&g_LandWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world * g_View * g_Proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&g_LandTextureTransform));
		Effects::BasicFX->SetMaterial(g_LandMaterial);
		Effects::BasicFX->SetDiffuseMap(g_LandMapSRV);

		landAndWavesTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(g_LandIndexCount, 0, 0);

		//
		// Draw the waves.
		//
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_WaveVertexBuffer, &stride, &offset);
		pd3dImmediateContext->IASetIndexBuffer(g_WaveIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		world = XMLoadFloat4x4(&g_WaveWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * g_View * g_Proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&g_WaterTexTransform));
		Effects::BasicFX->SetMaterial(g_WaveMaterial);
		Effects::BasicFX->SetDiffuseMap(g_WaveMapSRV);

		pd3dImmediateContext->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xffffffff);
		landAndWavesTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(3 * g_Wave.TriangleCount(), 0, 0);

		// Restore default blend state
		pd3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
	}





	renderTargetView[0] = pRTV;
	pd3dImmediateContext->OMSetRenderTargets(1, renderTargetView, pDSV);
	g_Blur.BlurInPlace(pd3dImmediateContext, g_OffscreenSRV, g_OffscreenUAV, 4);

	pd3dImmediateContext->ClearRenderTargetView(pRTV, reinterpret_cast<const float*>(&Colors::Silver));
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//
	// DrawScreenQuad();
	//
	pd3dImmediateContext->IASetInputLayout(InputLayouts::Basic32);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// UINT stride = sizeof(Vertex::Basic32);
	// UINT offset = 0;

	XMMATRIX identity = XMMatrixIdentity();

	ID3DX11EffectTechnique* texOnlyTech = Effects::BasicFX->Light0TexTech;
	// D3DX11_TECHNIQUE_DESC techDesc;

	texOnlyTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_ScreenQuadVertexBuffer, &stride, &offset);
		pd3dImmediateContext->IASetIndexBuffer(g_ScreenQuadIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		Effects::BasicFX->SetWorld(identity);
		Effects::BasicFX->SetWorldInvTranspose(identity);
		Effects::BasicFX->SetWorldViewProj(identity);
		Effects::BasicFX->SetTexTransform(identity);
		Effects::BasicFX->SetDiffuseMap(g_Blur.GetBlurredOutput());

		texOnlyTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(6, 0, 0);
	}


	hr = swapChain->Present(0, 0);
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
	HRESULT hr = S_OK;

	// Resize off-screen texture and rebuild views:
	// Render Targer View, SRV, UAV
	SAFE_RELEASE(g_OffscreenSRV);
	SAFE_RELEASE(g_OffscreenUAV);
	SAFE_RELEASE(g_OffscreenRTV);

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = 800;
	texDesc.Height = 600;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	ID3D11Texture2D* offscreenTex = 0;
	V_RETURN(pd3dDevice->CreateTexture2D(&texDesc, 0, &offscreenTex));
	V_RETURN(pd3dDevice->CreateShaderResourceView(offscreenTex, 0, &g_OffscreenSRV));
	V_RETURN(pd3dDevice->CreateRenderTargetView(offscreenTex, 0, &g_OffscreenRTV));
	V_RETURN(pd3dDevice->CreateUnorderedAccessView(offscreenTex, 0, &g_OffscreenUAV));
	SAFE_RELEASE(offscreenTex);

	g_Blur.Init(pd3dDevice, 800, 600, DXGI_FORMAT_R8G8B8A8_UNORM);	

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
	DXUTCreateWindow(L"Blur Demo");

	DXUTCreateDevice(D3D_FEATURE_LEVEL_10_0, true, 800, 600);
	DXUTMainLoop(); 


	return DXUTGetExitCode();
}