#include "DXUT.h"
#include "SDKmisc.h"
#include "GeometryGenerator.h"

#pragma warning( disable : 4100 )

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

struct CBChangesEveryFrame
{
    XMFLOAT4X4 mWorldViewProj;
};



//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
ID3D11VertexShader* g_pVertexShader = nullptr;
ID3D11PixelShader* g_pPixelShader = nullptr;
ID3D11InputLayout* g_pVertexLayout = nullptr;
ID3D11Buffer* g_pVertexBuffer = nullptr;
ID3D11Buffer* g_pIndexBuffer = nullptr;
ID3D11Buffer* g_pCBChangesEveryFrame = nullptr;
ID3D11ShaderResourceView* g_pTextureRV = nullptr;
ID3D11SamplerState* g_pSamplerLinear = nullptr;

UINT g_TotalIndexCount;
UINT g_GridIndexCount;
UINT g_BoxIndexCount;
UINT g_SphereIndexCount;
UINT g_CylinderIndexCount;

int mBoxVertexOffset;
int mGridVertexOffset;
int mSphereVertexOffset;
int mCylinderVertexOffset;

UINT mBoxIndexOffset;
UINT mGridIndexOffset;
UINT mSphereIndexOffset;
UINT mCylinderIndexOffset;

XMFLOAT4X4                    g_BoxWorld;
XMFLOAT4X4                    g_GridWorld;
XMFLOAT4X4                    g_CenterSphere;
XMFLOAT4X4                    g_CylWorld[10];
XMFLOAT4X4                    g_SphereWorld[10];

XMMATRIX                    g_World;
XMMATRIX                    g_View;
XMMATRIX                    g_Projection;
XMFLOAT4                    g_vMeshColor(0.7f, 0.7f, 0.7f, 1.0f);






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
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
    SAFE_RELEASE(g_pVertexBuffer);
    SAFE_RELEASE(g_pIndexBuffer);
    SAFE_RELEASE(g_pVertexLayout);
    SAFE_RELEASE(g_pTextureRV);
    SAFE_RELEASE(g_pVertexShader);
    SAFE_RELEASE(g_pPixelShader);
    SAFE_RELEASE(g_pCBChangesEveryFrame);
    SAFE_RELEASE(g_pSamplerLinear);
}



//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
    HRESULT hr = S_OK;

    auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;

    // Disable optimizations to further improve shader debugging
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    // Compile the vertex shader
    ID3D10Blob* pVSBlob = nullptr;
    V_RETURN(DXUTCompileFromFile(L"D:/Work/Direct3D11/Exc/Chapter06/Shape/ShapeDemo.fx", nullptr, "VS", "vs_5_0", dwShaderFlags, 0, &pVSBlob));

    // Create the vertex shader
    hr = pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create vertex buffer", L"Error", MB_OK);
        SAFE_RELEASE(pVSBlob);
        return hr;
    }

    // Define Input Layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    // Create Input Layout
    hr = pd3dDevice->CreateInputLayout(layout, ARRAYSIZE(layout), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pVertexLayout);
    if (FAILED(hr))
        return hr;

    // Set Input Layout
    pd3dImmediateContext->IASetInputLayout(g_pVertexLayout);

    
    // Compile Pixel Shader
    ID3D10Blob* pPSBlob = nullptr;
    V_RETURN(DXUTCompileFromFile(L"D:/Work/Direct3D11/Exc/Chapter06/Shape/ShapeDemo.fx", nullptr, "PS", "ps_5_0", dwShaderFlags, 0, &pPSBlob));

    // Create pixel shader
    hr = pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
    if (FAILED(hr))
    {
        SAFE_RELEASE(pPSBlob);
        return hr;
    }

    // Build Geometry Buffers
    GeometryGenerator::MeshData box;
    GeometryGenerator::MeshData grid;
    GeometryGenerator::MeshData sphere;
    GeometryGenerator::MeshData cylinder;

    GeometryGenerator geoGen;
    geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);
    geoGen.CreateGrid(20.0f, 30.0f, 60, 40, grid);
    geoGen.CreateSphere(0.5f, 20, 20, sphere);
    geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20, cylinder);

    mBoxVertexOffset = 0;
    mGridVertexOffset = box.Vertices.size();
    mSphereVertexOffset = mGridVertexOffset + grid.Vertices.size();
    mCylinderVertexOffset = mSphereVertexOffset + sphere.Vertices.size();

    g_BoxIndexCount = box.Indices.size();
    g_GridIndexCount = grid.Indices.size();
    g_SphereIndexCount = sphere.Indices.size();
    g_CylinderIndexCount = cylinder.Indices.size();

    mBoxIndexOffset = 0;
    mGridIndexOffset = g_BoxIndexCount;
    mSphereIndexOffset = mGridIndexOffset + g_GridIndexCount;
    mCylinderIndexOffset = mSphereIndexOffset + g_SphereIndexCount;


    UINT totalVertexCount = box.Vertices.size()
        + grid.Vertices.size()
        + sphere.Vertices.size()
        + cylinder.Vertices.size();

    g_TotalIndexCount = g_BoxIndexCount
        + g_GridIndexCount
        + g_SphereIndexCount
        + g_CylinderIndexCount;

    XMFLOAT4 black(0.0f, 0.0f, 0.0f, 1.0f);
    std::vector <SimpleVertex> vertices(totalVertexCount);

    UINT k = 0;
    for (size_t i = 0; i < box.Vertices.size(); i++, k++)
    {
        vertices[k].Pos = box.Vertices[i].Position;
        vertices[k].Color = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
    }

    for (size_t i = 0; i < grid.Vertices.size(); i++, k++)
    {
        vertices[k].Pos = grid.Vertices[i].Position;
        vertices[k].Color = XMFLOAT4(0.549f, 0.549f, 0.549f, 1.0f);
    }

    for (size_t i = 0; i < sphere.Vertices.size(); i++, k++)
    {
        vertices[k].Pos = sphere.Vertices[i].Position;
        vertices[k].Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f);;
    }

    for (size_t i = 0; i < cylinder.Vertices.size(); i++, k++)
    {
        vertices[k].Pos = cylinder.Vertices[i].Position;
        vertices[k].Color = XMFLOAT4(0.0, 0.0, 0.0, 0.0);
    }

    // Create Vertex Buffer
    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(SimpleVertex) * totalVertexCount;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    V_RETURN(pd3dDevice->CreateBuffer(&vbd, &vinitData, &g_pVertexBuffer));

    // Set Vertex Buffer
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    
    // Index
    std::vector <UINT> indices;
    indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());
    indices.insert(indices.end(), grid.Indices.begin(), grid.Indices.end());
    indices.insert(indices.end(), sphere.Indices.begin(), sphere.Indices.end());
    indices.insert(indices.end(), cylinder.Indices.begin(), cylinder.Indices.end());


    // Create Index Buffer
    D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(UINT) * g_TotalIndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &indices[0];
    V_RETURN(pd3dDevice->CreateBuffer(&ibd, &iinitData, &g_pIndexBuffer));
    
    // Set Index Buffer
    pd3dImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // Set Primitive Topology
    pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);




    // Create the constant buffers
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = sizeof(CBChangesEveryFrame);
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.MiscFlags = 0;
    pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBChangesEveryFrame);

    XMMATRIX I = XMMatrixIdentity();
    //g_World = I;
    XMStoreFloat4x4(&g_GridWorld, I);

    XMMATRIX boxScale = XMMatrixScaling(2.0f, 1.0f, 2.0f);
    XMMATRIX boxOffset = XMMatrixTranslation(0.0f, 0.5f, 0.0f);
    XMStoreFloat4x4(&g_BoxWorld, XMMatrixMultiply(boxScale, boxOffset));

    XMMATRIX centerSphereScale = XMMatrixScaling(2.0f, 2.0f, 2.0f);
    XMMATRIX centerSphereOffset = XMMatrixTranslation(0.0f, 2.0f, 0.0f);
    XMStoreFloat4x4(&g_CenterSphere, XMMatrixMultiply(centerSphereScale, centerSphereOffset));

    for (int i = 0; i < 5; ++i)
    {
        XMStoreFloat4x4(&g_CylWorld[i * 2 + 0], XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f));
        XMStoreFloat4x4(&g_CylWorld[i * 2 + 1], XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f));

        XMStoreFloat4x4(&g_SphereWorld[i * 2 + 0], XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f));
        XMStoreFloat4x4(&g_SphereWorld[i * 2 + 1], XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f));
    }

    static const XMVECTORF32 s_Eye = { 0.0f, 10.0f, 20.0f, 0.0f };
    static const XMVECTORF32 s_At = { 0.0f, 0.0f, 0.0f, 0.f };
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
    // Rotate cube around the origin
    XMMATRIX rotation = XMMatrixRotationY(45.0f * XMConvertToRadians((float)fTime));
    XMStoreFloat4x4(&g_BoxWorld, rotation);
    // g_World = XMMatrixIdentity();
}



//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
    double fTime, float fElapsedTime, void* pUserContext)
{
    // Clear the back buffer
    auto pRTV = DXUTGetD3D11RenderTargetView();
    pd3dImmediateContext->ClearRenderTargetView(pRTV, Colors::MidnightBlue);

    // Clear the depth stencil
    auto pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Set shaders and constant buffers
    pd3dImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
    pd3dImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBChangesEveryFrame);
    pd3dImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
    pd3dImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBChangesEveryFrame);

    // Update and draw the grid
    {
        g_World = XMLoadFloat4x4(&g_GridWorld);
        XMMATRIX mWorldViewProjection = g_World * g_View * g_Projection;

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = pd3dImmediateContext->Map(g_pCBChangesEveryFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(hr))
        {
            MessageBox(nullptr, L"Failed to map constant buffer", L"Error", MB_OK);
            return;
        }
        auto pCB = reinterpret_cast<CBChangesEveryFrame*>(mappedResource.pData);
        XMStoreFloat4x4(&pCB->mWorldViewProj, XMMatrixTranspose(mWorldViewProjection));
        pd3dImmediateContext->Unmap(g_pCBChangesEveryFrame, 0);

        pd3dImmediateContext->DrawIndexed(g_GridIndexCount, mGridIndexOffset, mGridVertexOffset);
    }

    // Update and draw the box
    {
        g_World = XMLoadFloat4x4(&g_BoxWorld);
        XMMATRIX mWorldViewProjection = g_World * g_View * g_Projection;

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = pd3dImmediateContext->Map(g_pCBChangesEveryFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(hr))
        {
            MessageBox(nullptr, L"Failed to map constant buffer", L"Error", MB_OK);
            return;
        }
        auto pCB = reinterpret_cast<CBChangesEveryFrame*>(mappedResource.pData);
        XMStoreFloat4x4(&pCB->mWorldViewProj, XMMatrixTranspose(mWorldViewProjection));
        pd3dImmediateContext->Unmap(g_pCBChangesEveryFrame, 0);

        pd3dImmediateContext->DrawIndexed(g_BoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);
    }

    // Update and draw the center sphere
    {
        g_World = XMLoadFloat4x4(&g_CenterSphere);
        XMMATRIX mWorldViewProjection = g_World * g_View * g_Projection;

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = pd3dImmediateContext->Map(g_pCBChangesEveryFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(hr))
        {
            MessageBox(nullptr, L"Failed to map constant buffer", L"Error", MB_OK);
            return;
        }
        auto pCB = reinterpret_cast<CBChangesEveryFrame*>(mappedResource.pData);
        XMStoreFloat4x4(&pCB->mWorldViewProj, XMMatrixTranspose(mWorldViewProjection));
        pd3dImmediateContext->Unmap(g_pCBChangesEveryFrame, 0);

        pd3dImmediateContext->DrawIndexed(g_SphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
    }

    // Draw the cylinders
    for (int i = 0; i < 10; ++i)
    {
        g_World = XMLoadFloat4x4(&g_CylWorld[i]);
        XMMATRIX mWorldViewProjection = g_World * g_View * g_Projection;

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = pd3dImmediateContext->Map(g_pCBChangesEveryFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(hr))
        {
            MessageBox(nullptr, L"Failed to map constant buffer", L"Error", MB_OK);
            return;
        }
        auto pCB = reinterpret_cast<CBChangesEveryFrame*>(mappedResource.pData);
        XMStoreFloat4x4(&pCB->mWorldViewProj, XMMatrixTranspose(mWorldViewProjection));
        pd3dImmediateContext->Unmap(g_pCBChangesEveryFrame, 0);

        pd3dImmediateContext->DrawIndexed(g_CylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
    }

    // Draw the spheres
    for (int i = 0; i < 10; ++i)
    {
        g_World = XMLoadFloat4x4(&g_SphereWorld[i]);
        XMMATRIX mWorldViewProjection = g_World * g_View * g_Projection;

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = pd3dImmediateContext->Map(g_pCBChangesEveryFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(hr))
        {
            MessageBox(nullptr, L"Failed to map constant buffer", L"Error", MB_OK);
            return;
        }
        auto pCB = reinterpret_cast<CBChangesEveryFrame*>(mappedResource.pData);
        XMStoreFloat4x4(&pCB->mWorldViewProj, XMMatrixTranspose(mWorldViewProjection));
        pd3dImmediateContext->Unmap(g_pCBChangesEveryFrame, 0);

        pd3dImmediateContext->DrawIndexed(g_SphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
    }

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
    DXUTCreateWindow(L"ShapeDemo");

    // Only require 10-level hardware or later
    DXUTCreateDevice(D3D_FEATURE_LEVEL_10_0, true, 800, 600);
    DXUTMainLoop(); // Enter into the DXUT render loop

    // Perform any application-level cleanup here

    return DXUTGetExitCode();
}