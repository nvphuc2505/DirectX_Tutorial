#include <DXUT.h>
#include <SDKmisc.h>
#include <DXUTcamera.h>
#include <fstream>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h> 
#include <assimp/scene.h> 
#include <windowsx.h>
#include "Vertex.h"
#include "Effects.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "RenderStates.h"



//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
CModelViewerCamera g_Camera;
DirectionalLight	g_DirectionalLights[3];


std::vector<ID3D11Buffer*> g_ModelVertexBuffers;
std::vector<ID3D11Buffer*> g_ModelIndexBuffers;
std::vector<UINT> g_ModelIndexCounts;
XMMATRIX	  g_ModelWorld;
Material	  g_ModelMaterial;

ID3D10ShaderResourceView* g_ModelSRV;



//--------------------------------------------------------------------------------------
// Function Helper
//--------------------------------------------------------------------------------------






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



	// --------------------------------------------------------------------------------------
	// --------------------------------------------------------------------------------------
	// --------------------------------------------------------------------------------------
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile("D:/Work/DirectX/Chapter23/Meshes/Models/nanosuit.obj", aiProcess_Triangulate);
	if (!scene || scene->mNumMeshes == 0)
	{
		MessageBox(nullptr, L"Failed to load model", L"Error", MB_OK);
		return E_FAIL;
	}

	for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
	{
		const aiMesh* mesh = scene->mMeshes[m];

		std::vector<Vertex::Basic32> vertices(mesh->mNumVertices);
		if (mesh->HasPositions())
		{
			for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			{
				aiVector3D* vp = &(mesh->mVertices[i]);
				vertices[i].Pos = XMFLOAT3(vp->x, vp->y, vp->z);
			}
		}
		if (mesh->HasNormals())
		{
			for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			{
				const aiVector3D* vn = &(mesh->mNormals[i]);
				vertices[i].Normal = XMFLOAT3(vn->x, vn->y, vn->z);
			}
		}
		if (mesh->HasTextureCoords(0))
		{
			for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			{
				const aiVector3D* vt = &(mesh->mTextureCoords[0][i]);
				vertices[i].Tex = XMFLOAT2(vt->x, vt->y);
			}
		}

		D3D11_BUFFER_DESC vbd = {};
		vbd.ByteWidth = sizeof(Vertex::Basic32) * vertices.size();
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA vInitData = {};
		vInitData.pSysMem = vertices.data();

		ID3D11Buffer* vertexBuffer;
		hr = pd3dDevice->CreateBuffer(&vbd, &vInitData, &vertexBuffer);
		if (FAILED(hr))
		{
			MessageBox(nullptr, L"Failed to create vertex buffer", L"Error", MB_OK);
			return hr;
		}
		g_ModelVertexBuffers.push_back(vertexBuffer);

		std::vector<unsigned int> indices;
		indices.reserve(mesh->mNumFaces * 3);
		if (mesh->HasFaces())
		{
			for (unsigned int j = 0; j < mesh->mNumFaces; j++)
			{
				aiFace face = mesh->mFaces[j];
				for (unsigned int k = 0; k < face.mNumIndices; k++)
				{
					indices.push_back(face.mIndices[k]);
				}
			}
		}

		UINT indexCount = indices.size();
		g_ModelIndexCounts.push_back(indexCount);

		D3D11_BUFFER_DESC ibd = {};
		ibd.ByteWidth = sizeof(unsigned int) * indexCount;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA iInitData = {};
		iInitData.pSysMem = indices.data();

		ID3D11Buffer* indexBuffer;
		hr = pd3dDevice->CreateBuffer(&ibd, &iInitData, &indexBuffer);
		if (FAILED(hr))
		{
			MessageBox(nullptr, L"Failed to create index buffer", L"Error", MB_OK);
			return hr;
		}
		g_ModelIndexBuffers.push_back(indexBuffer);
	}

	g_ModelWorld = XMMatrixTranslation(0, -5, 0) * XMMatrixRotationY(90);



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


	for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
	{
		const aiMaterial* material = scene->mMaterials[i];

		// Extract ambient color
		aiColor4D ambient;
		if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_AMBIENT, ambient))
		{
			// Use the ambient color in your shader
			// Example: Effects::BasicFX->SetAmbientColor(XMFLOAT4(ambient.r, ambient.g, ambient.b, ambient.a));

			g_ModelMaterial.Ambient = XMFLOAT4(ambient.r, ambient.g, ambient.b, ambient.a);
		}

		// Extract diffuse color
		aiColor4D diffuse;
		if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
		{
			// Example: Effects::BasicFX->SetDiffuseColor(XMFLOAT4(diffuse.r, diffuse.g, diffuse.b, diffuse.a));
			g_ModelMaterial.Diffuse = XMFLOAT4(diffuse.r, diffuse.g, diffuse.b, diffuse.a);
		}

		// Extract specular color
		aiColor4D specular;
		if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, specular))
		{
			// Example: Effects::BasicFX->SetSpecularColor(XMFLOAT4(specular.r, specular.g, specular.b, specular.a));
			g_ModelMaterial.Specular = XMFLOAT4(specular.r, specular.g, specular.b, specular.a);
		}

		g_ModelMaterial.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	
		// Extract textures
		for (unsigned int j = 0; j < AI_TEXTURE_TYPE_MAX; ++j)
		{
			aiString path;
			if (AI_SUCCESS == material->GetTexture(static_cast<aiTextureType>(j), 0, &path))
			{
				// Load the texture and set it in the shader
				// Example: Effects::BasicFX->SetTexture(j, LoadTexture(path.C_Str()));
			}
		}
	}

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
	// Effects::BasicFX->SetCubeMap(g_Sky->GetSkyCubeMap());
	XMMATRIX world = g_ModelWorld;
	XMMATRIX view = g_Camera.GetViewMatrix();
	XMMATRIX proj = g_Camera.GetProjMatrix();

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	pd3dImmediateContext->IASetInputLayout(InputLayouts::Basic32);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3DX11EffectTechnique* tech = Effects::BasicFX->Light3Tech;

	D3DX11_TECHNIQUE_DESC techDesc;
	tech->GetDesc(&techDesc);
	for (size_t i = 0; i < g_ModelVertexBuffers.size(); ++i)
	{
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_ModelVertexBuffers[i], &stride, &offset);
		pd3dImmediateContext->IASetIndexBuffer(g_ModelIndexBuffers[i], DXGI_FORMAT_R32_UINT, 0);

		ID3DX11EffectTechnique* tech = Effects::BasicFX->Light3Tech;
		D3DX11_TECHNIQUE_DESC techDesc;
		tech->GetDesc(&techDesc);
		for (UINT p = 0; p < techDesc.Passes; ++p)
		{
			XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
			XMMATRIX worldView = world * view;
			XMMATRIX worldInvTransposeView = worldInvTranspose * view;
			XMMATRIX worldViewProj = world * view * proj;

			Effects::BasicFX->SetWorld(world);
			Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::BasicFX->SetWorldViewProj(worldViewProj);
			Effects::BasicFX->SetMaterial(g_ModelMaterial);

			tech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
			pd3dImmediateContext->DrawIndexed(g_ModelIndexCounts[i], 0, 0);
		}
	}
}




//--------------------------------------------------------------------------------------
// Handle updates to the scene.
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	g_Camera.FrameMove(fElapsedTime);
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
	// g_Camera.SetButtonMasks(MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON);



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