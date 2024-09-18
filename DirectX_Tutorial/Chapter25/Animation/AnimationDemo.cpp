#include <DXUT.h>
#include <SDKmisc.h>
#include <DXUTcamera.h>
#include <fstream>
#include <unordered_map>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h> 
#include <assimp/scene.h> 
#include <windowsx.h>
#include "Vertex.h"
#include "Effects.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "RenderStates.h"
#include "SkinnedModel.h"



//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
struct BoneTransformTrack 
{
	std::vector<float> positionTimestamps = {};
	std::vector<float> rotationTimestamps = {};
	std::vector<float> scaleTimestamps = {};

	std::vector<XMFLOAT3> positions = {};
	std::vector<XMFLOAT4> rotations = {};
	std::vector<XMFLOAT3> scales = {};
};

struct Animation 
{
	float duration = 0.0f;
	float ticksPerSecond = 1.0f;
	std::unordered_map<std::string, BoneTransformTrack> boneTransforms = {};
};



CModelViewerCamera g_Camera;
DirectionalLight	g_DirectionalLights[3];

SkinnedModel* g_CharacterModel;
SkinnedModelInstance g_CharacterInstance1;
SkinnedModelInstance g_CharacterInstance2;

XMMATRIX	  g_ModelWorld;
Material	  g_ModelMaterial;






//--------------------------------------------------------------------------------------
// Function Helper
//--------------------------------------------------------------------------------------
void GetClipNameFromAssimp(SkinnedModelInstance& instance, const aiScene* scene)
{
	if (scene && scene->HasAnimations())
	{
		aiAnimation* anim = scene->mAnimations[0];
		instance.ClipName = anim->mName.C_Str();

		if (instance.ClipName.empty())
		{
			instance.ClipName = "Default_Clip";
		}
	}
	else
	{
		instance.ClipName = "No_Animation";
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

	static const XMVECTORF32 s_Eye = { 0.0f, 3.0f, -10.0f, 0.f };
	static const XMVECTORF32 s_At = { 0.0f, 1.0f, 0.0f, 0.f };
	g_Camera.SetViewParams(s_Eye, s_At);

	g_DirectionalLights[0].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	g_DirectionalLights[0].Diffuse = XMFLOAT4(0.7f, 0.7f, 0.6f, 1.0f);
	g_DirectionalLights[0].Specular = XMFLOAT4(0.8f, 0.8f, 0.7f, 1.0f);
	g_DirectionalLights[0].Direction = XMFLOAT3(0.707f, 0.0f, 0.707f);

	g_DirectionalLights[1].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	g_DirectionalLights[1].Diffuse = XMFLOAT4(0.40f, 0.40f, 0.40f, 1.0f);
	g_DirectionalLights[1].Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	g_DirectionalLights[1].Direction = XMFLOAT3(0.0f, -0.707f, 0.707f);

	g_DirectionalLights[2].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	g_DirectionalLights[2].Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	g_DirectionalLights[2].Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	g_DirectionalLights[2].Direction = XMFLOAT3(-0.57735f, -0.57735f, -0.57735f);


	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile("D:/Work/DirectX/Chapter25/Animation/Models/boy.dae", aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
	g_CharacterModel = new SkinnedModel(pd3dDevice, "D:/Work/DirectX/Chapter25/Animation/Models/boy.dae", scene);
	g_CharacterInstance1.Model = g_CharacterModel;
	g_CharacterInstance1.TimePos = 0.0f;
	GetClipNameFromAssimp(g_CharacterInstance1, scene);
	g_CharacterInstance1.FinalTransforms.resize(g_CharacterModel->SkinnedData.BoneCount());
	g_CharacterInstance2.Model = g_CharacterModel;
	g_CharacterInstance2.TimePos = 0.0f;
	GetClipNameFromAssimp(g_CharacterInstance2, scene);
	g_CharacterInstance2.FinalTransforms.resize(g_CharacterModel->SkinnedData.BoneCount());

	XMMATRIX modelScale = XMMatrixScaling(0.05f, 0.05f, -0.05f);
	XMMATRIX modelRot = XMMatrixRotationY(MathHelper::Pi);
	XMMATRIX modelOffset = XMMatrixTranslation(-2.0f, 0.0f, -7.0f);
	XMStoreFloat4x4(&g_CharacterInstance1.World, modelScale * modelRot * modelOffset);

	modelOffset = XMMatrixTranslation(2.0f, 0.0f, -7.0f);
	XMStoreFloat4x4(&g_CharacterInstance2.World, modelScale * modelRot * modelOffset);

	return S_OK;
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
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);

	XMFLOAT3 eyePos;
	XMStoreFloat3(&eyePos, g_Camera.GetEyePt());
	Effects::BasicFX->SetEyePosW(eyePos);
	Effects::BasicFX->SetDirLights(g_DirectionalLights);
	XMMATRIX world = (XMMatrixIdentity());
	XMMATRIX view = g_Camera.GetViewMatrix();
	XMMATRIX proj = g_Camera.GetProjMatrix();

	UINT stride = sizeof(Vertex::PosNormalTexTanSkinned);
	UINT offset = 0;

	//
	// Draw the animated characters.
	//
	XMMATRIX worldInvTranspose;
	XMMATRIX worldViewProj;
	XMMATRIX toTexSpace(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	pd3dImmediateContext->IASetInputLayout(InputLayouts::PosNormalTexTanSkinned);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ID3DX11EffectTechnique* activeSkinnedTech = Effects::NormalMapFX->Light3TexSkinnedTech;
	// D3DX11_TECHNIQUE_DESC techDesc;
	// activeSkinnedTech->GetDesc(&techDesc);

	for (size_t i = 0; i < g_CharacterInstance1.Model->VertexBuffers.size(); ++i)
	{
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_CharacterInstance1.Model->VertexBuffers[i], &stride, &offset);
		pd3dImmediateContext->IASetIndexBuffer(g_CharacterInstance1.Model->IndexBuffers[i], DXGI_FORMAT_R32_UINT, 0);

		ID3DX11EffectTechnique* tech = Effects::NormalMapFX->Light3TexSkinnedTech;
		D3DX11_TECHNIQUE_DESC techDesc;
		tech->GetDesc(&techDesc);
		for (UINT p = 0; p < techDesc.Passes; ++p)
		{
			XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
			XMMATRIX worldView = world * view;
			XMMATRIX worldInvTransposeView = worldInvTranspose * view;
			XMMATRIX worldViewProj = world * view * proj;

			Effects::NormalMapFX->SetWorld(world);
			Effects::NormalMapFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::NormalMapFX->SetWorldViewProj(worldViewProj);
			Effects::NormalMapFX->SetWorldViewProjTex(worldViewProj * toTexSpace);
			// Effects::NormalMapFX->SetShadowTransform(world * shadowTransform);
			Effects::NormalMapFX->SetTexTransform(XMMatrixScaling(1.0f, 1.0f, 1.0f));
			Effects::NormalMapFX->SetBoneTransforms(
				&g_CharacterInstance1.FinalTransforms[0],
				g_CharacterInstance1.FinalTransforms.size());


			tech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
			pd3dImmediateContext->DrawIndexed(g_CharacterInstance1.Model->IndexCount[i], 0, 0);
		}
	}
}




//--------------------------------------------------------------------------------------
// Handle updates to the scene.
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	g_Camera.FrameMove(fElapsedTime);

	g_CharacterInstance1.Update(fTime);
	g_CharacterInstance2.Update(fTime);
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
	g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);

	return S_OK;
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

	float fNearPlane = 0.1f;
	float fFarPlane = 1000.0f;
	float fFOV = XM_PI / 4;
	float fAspect = static_cast<float>(pBackBufferSurfaceDesc->Width) / static_cast<float>(pBackBufferSurfaceDesc->Height);

	g_Camera.SetProjParams(fFOV, fAspect, fNearPlane, fFarPlane);
	g_Camera.SetWindow(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);

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

	DXUTCreateWindow(L"Picking");

	DXUTCreateDevice(D3D_FEATURE_LEVEL_10_0, true, 800, 600);
	DXUTMainLoop();

	return DXUTGetExitCode();
}