#include <DXUT.h>
#include <SDKmisc.h>
#include "Vertex.h"
#include "Effects.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "RenderStates.h"
#include "GeometryGenerator.h"

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------

ID3D11Buffer*	g_QuadPatchVB;

XMFLOAT4X4		g_World;
XMFLOAT3		g_WorldTranslation;

XMMATRIX		g_View;
XMMATRIX		g_Proj;

XMFLOAT3		g_EyePosW;

float mTheta;
float mPhi;
float mRadius;

POINT g_LastMousePos;





//--------------------------------------------------------------------------------------
// Function Helper
//--------------------------------------------------------------------------------------
void BuildQuadPatchBuffer(ID3D11Device* device)
{
	HRESULT hr = S_OK;

	XMFLOAT3 vertices[4] =
	{
		XMFLOAT3(-10.0f, 0.0f, +10.0f),
		XMFLOAT3(+10.0f, 0.0f, +10.0f),
		XMFLOAT3(-10.0f, 0.0f, -10.0f),
		XMFLOAT3(+10.0f, 0.0f, -10.0f)
	};

	D3D11_BUFFER_DESC vbd;
	vbd.ByteWidth = sizeof(XMFLOAT3) * 4;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vInitData;
	vInitData.pSysMem = &vertices[0];

	hr = device->CreateBuffer(&vbd, &vInitData, &g_QuadPatchVB);
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
	RenderStates::InitAll(pd3dDevice);

	BuildQuadPatchBuffer(pd3dDevice);

	return S_OK;
}





// --------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
	double fTime, float fElapsedTime, void* pUserContext)
{
	HRESULT hr = S_OK;

	auto pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	auto pRTV = DXUTGetD3D11RenderTargetView();
	pd3dImmediateContext->ClearRenderTargetView(pRTV, reinterpret_cast<const float*>(&Colors::MidnightBlue));

	XMMATRIX world = g_View * g_Proj;

	pd3dImmediateContext->IASetInputLayout(InputLayouts::Basic32);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pd3dImmediateContext->IASetInputLayout(InputLayouts::Pos);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);

	UINT stride = sizeof(Vertex::Pos);
	UINT offset = 0;

	Effects::TessellationFX->SetEyePosW(g_EyePosW);
	// Effects::TessellationFX->SetFogColor(Colors::Silver);
	// Effects::TessellationFX->SetFogStart(15.0f);
	// Effects::TessellationFX->SetFogRange(175.0f);

	D3DX11_TECHNIQUE_DESC techDesc;
	Effects::TessellationFX->TessTech->GetDesc(&techDesc);

	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_QuadPatchVB, &stride, &offset);

		// Set per object constants.
		XMMATRIX world = XMLoadFloat4x4(&g_World);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world * g_View * g_Proj;

		Effects::TessellationFX->SetWorld(world);
		Effects::TessellationFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::TessellationFX->SetWorldViewProj(worldViewProj);
		Effects::TessellationFX->SetTexTransform(XMMatrixIdentity());
		// Effects::TessellationFX->SetMaterial(0);
		Effects::TessellationFX->SetDiffuseMap(0);

		Effects::TessellationFX->TessTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);

		pd3dImmediateContext->RSSetState(RenderStates::WireframeRS);
		pd3dImmediateContext->Draw(4, 0);
	}
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

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	g_View = XMMatrixLookAtLH(pos, target, up);


	if (GetAsyncKeyState('A') & 0x8000)
		g_WorldTranslation.x -= 10.0f * fElapsedTime;

	if (GetAsyncKeyState('D') & 0x8000)
		g_WorldTranslation.x += 10.0f * fElapsedTime;

	if (GetAsyncKeyState('W') & 0x8000)
		g_WorldTranslation.y += 10.0f * fElapsedTime;

	if (GetAsyncKeyState('S') & 0x8000)
		g_WorldTranslation.y -= 10.0f * fElapsedTime;

	if (GetAsyncKeyState('Q') & 0x8000)
		g_WorldTranslation.z += 10.0f * fElapsedTime;

	if (GetAsyncKeyState('E') & 0x8000)
		g_WorldTranslation.z -= 10.0f * fElapsedTime;

	XMMATRIX offset = XMMatrixTranslation(g_WorldTranslation.x, g_WorldTranslation.y, g_WorldTranslation.z);
	XMStoreFloat4x4(&g_World, offset);
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
	DXUTCreateWindow(L"Basic Tessellation Demo");

	DXUTCreateDevice(D3D_FEATURE_LEVEL_10_0, true, 800, 600);
	DXUTMainLoop();


	return DXUTGetExitCode();
}