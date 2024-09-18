#include <DXUT.h>
#include <SDKmisc.h>
#include "Waves.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "GeometryGenerator.h"
#include <d3dx11effect.h>
#include <fstream>


#pragma warning( disable : 4100 )

using namespace DirectX;

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
};

struct cbPerFrame
{
    DirectionalLight            g_DirectionLight;
    PointLight                  g_PointLight;
    SpotLight                   g_SpotLight;
    XMFLOAT3					g_EyePosW;
};

struct cbPerObject
{
    XMFLOAT4X4 mWorld;
    XMFLOAT4X4 mWorldInvTranspose;
    XMFLOAT4X4 mWorldViewProj;
    XMFLOAT4X4 mMaterial;
};

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
DirectionalLight            g_DirectionLight;
PointLight                  g_PointLight;
SpotLight                   g_SpotLight;
Material                    g_LandMaterial;
Material                    g_WavesMaterial;

ID3DX11Effect*               g_FX = nullptr;
ID3DX11EffectTechnique*      g_Tech = nullptr;

ID3DX11EffectMatrixVariable* g_fxWorldViewProj;
ID3DX11EffectMatrixVariable* g_fxWorld;
ID3DX11EffectMatrixVariable* g_fxWorldInvTranspose;
ID3DX11EffectVectorVariable* g_fxEyePosW;
ID3DX11EffectVariable*       g_fxDirLight;
ID3DX11EffectVariable*       g_fxPointLight;
ID3DX11EffectVariable*       g_fxSpotLight;
ID3DX11EffectVariable*       g_fxMaterial;

ID3D11VertexShader*			g_pVertexShader = nullptr;
ID3D11PixelShader*			g_pPixelShader = nullptr;

ID3D11InputLayout*			g_pInputLayout = nullptr;

Waves						g_Waves;
ID3D11Buffer*				g_WaveVertexBuffer = nullptr;
ID3D11Buffer*				g_WaveIndexBuffer = nullptr;

UINT						g_LandIndexCount = 0;
ID3D11Buffer*				g_LandVertexBuffer = nullptr;
ID3D11Buffer*				g_LandIndexBuffer = nullptr;

ID3D11Buffer*               g_cbPerFrameBuffer;
ID3D11Buffer*               g_cbPerObjectBuffer;

XMMATRIX                    g_LandWorld;
XMMATRIX					g_WavesWorld;
XMMATRIX                    g_World;
XMMATRIX                    g_View;
XMMATRIX                    g_Projection;
XMFLOAT3					g_EyePosW;




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

    auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    dwShaderFlags |= D3DCOMPILE_DEBUG;
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif


#if D3D_COMPILER_VERSION >= 46

    // Read the D3DX effect file
    WCHAR str[MAX_PATH];
    V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"D:/Work/Direct3D11/Exc/Chapter07/HillAndWave/Light.fx"));

    V_RETURN(D3DX11CompileEffectFromFile(str, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, dwShaderFlags, 0, pd3dDevice, &g_FX, nullptr));

#endif

    

    g_Tech = g_FX->GetTechniqueByName("LightTech");
    g_fxWorldViewProj = g_FX->GetVariableByName("gWorldViewProj")->AsMatrix();
    g_fxWorld = g_FX->GetVariableByName("gWorld")->AsMatrix();
    g_fxWorldInvTranspose = g_FX->GetVariableByName("gWorldInvTranspose")->AsMatrix();
    g_fxEyePosW = g_FX->GetVariableByName("gEyePosW")->AsVector();
    g_fxDirLight = g_FX->GetVariableByName("gDirLight");
    g_fxPointLight = g_FX->GetVariableByName("gPointLight");
    g_fxSpotLight = g_FX->GetVariableByName("gSpotLight");
    g_fxMaterial = g_FX->GetVariableByName("gMaterial");

    //
    // Initializating Lighting
    //
    // ===== Directional light =====
    g_DirectionLight.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    g_DirectionLight.Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    g_DirectionLight.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    g_DirectionLight.Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

    // ===== Point light =====
    // position is changed every frame to animate in UpdateScene function.
    g_PointLight.Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
    g_PointLight.Diffuse = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
    g_PointLight.Specular = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
    g_PointLight.Att = XMFLOAT3(0.0f, 0.1f, 0.0f);
    g_PointLight.Range = 25.0f;

    // Spot light--position and direction changed every frame to animate in UpdateScene function.
    g_SpotLight.Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    g_SpotLight.Diffuse = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
    g_SpotLight.Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    g_SpotLight.Att = XMFLOAT3(1.0f, 0.0f, 0.0f);
    g_SpotLight.Spot = 96.0f;
    g_SpotLight.Range = 10000.0f;

    g_LandMaterial.Ambient = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
    g_LandMaterial.Diffuse = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
    g_LandMaterial.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

    g_WavesMaterial.Ambient = XMFLOAT4(0.137f, 0.42f, 0.556f, 1.0f);
    g_WavesMaterial.Diffuse = XMFLOAT4(0.137f, 0.42f, 0.556f, 1.0f);
    g_WavesMaterial.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 96.0f);



    //
    // Vertex Shader
    //
    ID3D10Blob* pVSBlob = nullptr;
    V_RETURN(DXUTCompileFromFile(L"D:/Work/Direct3D11/Exc/Chapter07/HillAndWave/Light.fx", nullptr, "VS", "vs_5_0", dwShaderFlags, 0, &pVSBlob));
    hr = pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
    if(FAILED(hr))
    {
        SAFE_RELEASE(pVSBlob);
        return hr;
    }



    //
    // Input Layout
    //
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    hr = pd3dDevice->CreateInputLayout(layout, ARRAYSIZE(layout), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pInputLayout);
    if (FAILED(hr))
        return hr;
    pd3dImmediateContext->IASetInputLayout(g_pInputLayout);



    //
    // Pixel Shader
    //
    ID3D10Blob* pPSBlob = nullptr;
    V_RETURN(DXUTCompileFromFile(L"D:/Work/Direct3D11/Exc/Chapter07/HillAndWave/Light.fx", nullptr, "PS", "ps_5_0", dwShaderFlags, 0, &pPSBlob));
    hr = pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
    if (FAILED(hr))
    {
        SAFE_RELEASE(pVSBlob);
        return hr;
    }



    //
    // Build Land Geometry
    //
    GeometryGenerator::MeshData grid;
    GeometryGenerator geoGen;
    geoGen.CreateGrid(300.0f, 300.0f, 200, 200, grid);
    g_LandIndexCount = grid.Indices.size();

    // ===== LAND VERTEX =====
    std::vector<Vertex> landVertices(grid.Vertices.size());
    for (size_t i = 0; i < grid.Vertices.size(); ++i)
    {
        XMFLOAT3 p = grid.Vertices[i].Position;

        p.y = 0.3f * (p.z * sinf(0.1f * p.x) + p.x * cosf(0.1f * p.z));

        landVertices[i].Pos = p;
        landVertices[i].Normal = GetHillNormal(p.x, p.z);
    }

    D3D11_BUFFER_DESC landvbd = {};
    landvbd.Usage = D3D11_USAGE_IMMUTABLE;
    landvbd.ByteWidth = sizeof(Vertex) * grid.Vertices.size();
    landvbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    landvbd.CPUAccessFlags = 0;
    landvbd.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA landVInitData = {};
    landVInitData.pSysMem = &landVertices[0];
    V_RETURN(pd3dDevice->CreateBuffer(&landvbd, &landVInitData, &g_LandVertexBuffer));

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_LandVertexBuffer, &stride, &offset);

    // ===== LAND INDEX =====
    D3D11_BUFFER_DESC landibd = { };
    landibd.Usage = D3D11_USAGE_IMMUTABLE;
    landibd.ByteWidth = sizeof(UINT) * g_LandIndexCount;
    landibd.CPUAccessFlags = 0;
    landibd.MiscFlags = 0;
    landibd.BindFlags = D3D10_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA landIInitData = {};
    landIInitData.pSysMem = &grid.Indices[0];
    V_RETURN(pd3dDevice->CreateBuffer(&landibd, &landIInitData, &g_LandIndexBuffer));
    pd3dImmediateContext->IASetIndexBuffer(g_LandIndexBuffer, DXGI_FORMAT_R32_UINT, 0);




    //
    // Build Wave Geometry
    //
    g_Waves.Init(300, 300, 1.0f, 0.03f, 3.25f, 0.4f);

    // ===== WAVE VERTEX =====
    D3D11_BUFFER_DESC wavevbd = {};
    wavevbd.ByteWidth = sizeof(Vertex) * g_Waves.VertexCount();
    wavevbd.Usage = D3D11_USAGE_DYNAMIC;
    wavevbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    wavevbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    wavevbd.MiscFlags = 0;
    V_RETURN(pd3dDevice->CreateBuffer(&wavevbd, nullptr, &g_WaveVertexBuffer));
    pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_WaveVertexBuffer, &stride, &offset);

    // ===== WAVE INDEX =====
    std::vector<UINT> waveIndicies(3 * g_Waves.TriangleCount());
    UINT m = g_Waves.RowCount();
    UINT n = g_Waves.ColumnCount();
    int k = 0;
    for (UINT i = 0; i < m - 1; ++i)
    {
        for (DWORD j = 0; j < n - 1; ++j)
        {
            waveIndicies[k] = i * n + j;
            waveIndicies[k + 1] = i * n + j + 1;
            waveIndicies[k + 2] = (i + 1) * n + j;

            waveIndicies[k + 3] = (i + 1) * n + j;
            waveIndicies[k + 4] = i * n + j + 1;
            waveIndicies[k + 5] = (i + 1) * n + j + 1;

            k += 6;
        }
    }

    D3D11_BUFFER_DESC waveibd = {};
    waveibd.ByteWidth = sizeof(UINT) * waveIndicies.size();
    waveibd.Usage = D3D11_USAGE_IMMUTABLE;
    waveibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    waveibd.CPUAccessFlags = 0;
    waveibd.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA waveIInitData = {};
    waveIInitData.pSysMem = &waveIndicies[0];
    V_RETURN(pd3dDevice->CreateBuffer(&waveibd, &waveIInitData, &g_WaveIndexBuffer));
    pd3dImmediateContext->IASetIndexBuffer(g_WaveIndexBuffer, DXGI_FORMAT_R32_UINT, 0);


    //
    // Constant Buffer
    // 
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = sizeof(cbPerFrame);
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.MiscFlags = 0;
    pd3dDevice->CreateBuffer(&bd, nullptr, &g_cbPerFrameBuffer);

    bd.ByteWidth = sizeof(cbPerObject);
    pd3dDevice->CreateBuffer(&bd, nullptr, &g_cbPerObjectBuffer);


    //
    //
    //

    g_LandWorld = XMMatrixIdentity();
    g_WavesWorld = XMMatrixTranslation(0.0f, -3.0f, 0.0f);

    static const XMVECTORF32 s_Eye = { 0.0f, 100.0f, 200.0f, 0.f };
    static const XMVECTORF32 s_At = { 0.0f, 1.0f, 0.0f, 0.f };
    static const XMVECTORF32 s_Up = { 0.0f, 1.0f, 0.0f, 0.f };
    g_View = XMMatrixLookAtLH(s_Eye, s_At, s_Up);

    return S_OK;
}



//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
    const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
    // Setup the projection parameters
    float fAspect = static_cast<float>(pBackBufferSurfaceDesc->Width) / static_cast<float>(pBackBufferSurfaceDesc->Height);
    g_Projection = XMMatrixPerspectiveFovLH(XM_PI * 0.25f, fAspect, 0.1f, 1000.0f);

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

    // g_View = XMMatrixLookAtLH(pos, target, up);

    // Every quarter second, generate a random wave.
    static float t_base = 0.0f;
    if ((fTime - t_base) >= 0.25f)
    {
        t_base += 0.25f;

        DWORD i = 5 + rand() % (g_Waves.RowCount() - 10);
        DWORD j = 5 + rand() % (g_Waves.ColumnCount() - 10);

        float r = MathHelper::RandF(1.0f, 2.0f);

        g_Waves.Disturb(i, j, r);
    }

    // Update the wave simulation.
    g_Waves.Update(fElapsedTime);

    // Update the wave vertex buffer with the new solution.
    D3D11_MAPPED_SUBRESOURCE mappedData;
    auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    pd3dImmediateContext->Map(g_WaveVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);

    Vertex* v = reinterpret_cast<Vertex*>(mappedData.pData);
    for (UINT i = 0; i < g_Waves.VertexCount(); ++i)
    {
        v[i].Pos = g_Waves[i];
        v[i].Normal = g_Waves.Normal(i);
    }

    pd3dImmediateContext->Unmap(g_WaveVertexBuffer, 0);
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


    g_fxDirLight->SetRawValue(&g_DirectionLight, 0, sizeof(g_DirectionLight));
    g_fxPointLight->SetRawValue(&g_PointLight, 0, sizeof(g_PointLight));
    g_fxSpotLight->SetRawValue(&g_SpotLight, 0, sizeof(g_SpotLight));
    g_fxEyePosW->SetRawValue(&g_EyePosW, 0, sizeof(g_EyePosW));

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    D3DX11_TECHNIQUE_DESC techDesc;
    g_Tech->GetDesc(&techDesc);
    for (UINT p = 0; p < techDesc.Passes; ++p)
    {
        //
        // Draw the hills.
        //
        
        pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_LandVertexBuffer, &stride, &offset);
        pd3dImmediateContext->IASetIndexBuffer(g_LandIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

        XMMATRIX world = g_LandWorld;
        XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
        XMMATRIX worldViewProj = world * g_View * g_Projection;

        g_fxWorld->SetMatrix(reinterpret_cast<float*>(&world));
        g_fxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
        g_fxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
        g_fxMaterial->SetRawValue(&g_LandMaterial, 0, sizeof(g_LandMaterial));

        g_Tech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
        pd3dImmediateContext->DrawIndexed(g_LandIndexCount, 0, 0);

        //
        // Draw the waves.
        //

        pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_WaveVertexBuffer, &stride, &offset);
        pd3dImmediateContext->IASetIndexBuffer(g_WaveIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        
        world = g_WavesWorld;
        worldInvTranspose = MathHelper::InverseTranspose(world);
        worldViewProj = world * g_View * g_Projection;

        g_fxWorld->SetMatrix(reinterpret_cast<float*>(&world));
        g_fxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
        g_fxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
        g_fxMaterial->SetRawValue(&g_WavesMaterial, 0, sizeof(g_WavesMaterial));

        g_Tech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
        pd3dImmediateContext->DrawIndexed(3 * g_Waves.TriangleCount(), 0, 0);
        
    }

}



//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
    SAFE_RELEASE(g_LandVertexBuffer);
    SAFE_RELEASE(g_LandVertexBuffer);
    SAFE_RELEASE(g_pInputLayout);
    SAFE_RELEASE(g_WaveVertexBuffer);
    SAFE_RELEASE(g_WaveIndexBuffer);
    SAFE_RELEASE(g_pVertexShader);
    SAFE_RELEASE(g_pPixelShader);
    SAFE_RELEASE(g_cbPerFrameBuffer);
    SAFE_RELEASE(g_cbPerObjectBuffer);
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