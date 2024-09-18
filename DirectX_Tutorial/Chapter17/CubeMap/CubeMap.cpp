#include <DXUT.h>
#include <SDKmisc.h>
#include <DXUTcamera.h>
#include <fstream>
#include "Vertex.h"
#include "Effects.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "RenderStates.h"
#include <windowsx.h>
#include "Sky.h"

using namespace DirectX;



struct AxisAlignedBox
{
	XMFLOAT3 Center;
	XMFLOAT3 Extents;	// Distance from the center to each side.
};



//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
CModelViewerCamera				g_Camera;
UINT							g_PickedTriangle;

DirectionalLight				g_DirectionalLights[3];

ID3D11Buffer* g_CarVertexBuffer;
ID3D11Buffer* g_CarIndexBuffer;
std::vector <Vertex::Basic32>	g_CarVertices;
std::vector <UINT>				g_CarIndices;
UINT							g_CarIndexCount;
AxisAlignedBox					g_CarBox;
XMFLOAT4X4						g_CarWorld;
Material						g_CarMaterial;

Material						g_PickedTriangleMaterial;



Sky* g_Sky;
ID3D11Buffer* g_SkyVertexBuffer;
ID3D11Buffer* g_SkyIndexBuffer;







//--------------------------------------------------------------------------------------
// Functional Helper
//--------------------------------------------------------------------------------------
void BuildLighting()
{
	g_DirectionalLights[0].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	g_DirectionalLights[0].Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	g_DirectionalLights[0].Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	g_DirectionalLights[0].Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	g_DirectionalLights[1].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	g_DirectionalLights[1].Diffuse = XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
	g_DirectionalLights[1].Specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	g_DirectionalLights[1].Direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

	g_DirectionalLights[2].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	g_DirectionalLights[2].Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	g_DirectionalLights[2].Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	g_DirectionalLights[2].Direction = XMFLOAT3(0.0f, -0.707f, -0.707f);
}
void BuildMeshGeometryBuffers(ID3D11Device* pd3dDevice)
{
	std::ifstream fin("Models/car.txt");
	if (!fin)
	{
		MessageBox(0, L"Models/car.txt not found.", 0, 0);
		return;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
	XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

	XMVECTOR vMin = XMLoadFloat3(&vMinf3);
	XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

	g_CarVertices.resize(vcount);
	for (UINT i = 0; i < vcount; i++)
	{
		fin >> g_CarVertices[i].Pos.x >> g_CarVertices[i].Pos.y >> g_CarVertices[i].Pos.z;
		fin >> g_CarVertices[i].Normal.x >> g_CarVertices[i].Normal.y >> g_CarVertices[i].Normal.z;

		XMVECTOR P = XMLoadFloat3(&g_CarVertices[i].Pos);

		vMin = XMVectorMin(vMin, P);
		vMax = XMVectorMax(vMax, P);
	}

	XMStoreFloat3(&g_CarBox.Center, 0.5f * (vMin + vMax));
	XMStoreFloat3(&g_CarBox.Extents, 0.5f * (vMax - vMin));

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	g_CarIndexCount = 3 * tcount;
	g_CarIndices.resize(g_CarIndexCount);
	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> g_CarIndices[i * 3 + 0] >> g_CarIndices[i * 3 + 1] >> g_CarIndices[i * 3 + 2];
	}

	fin.close();

	D3D11_BUFFER_DESC vbd = {};
	vbd.ByteWidth = sizeof(Vertex::Basic32) * vcount;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vInitData;
	vInitData.pSysMem = &g_CarVertices[0];
	HRESULT hr = pd3dDevice->CreateBuffer(&vbd, &vInitData, &g_CarVertexBuffer);

	D3D11_BUFFER_DESC ibd = {};
	ibd.ByteWidth = sizeof(UINT) * g_CarIndexCount;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iInitData;
	iInitData.pSysMem = &g_CarIndices[0];
	hr = pd3dDevice->CreateBuffer(&ibd, &iInitData, &g_CarIndexBuffer);
}

static const XMVECTOR g_UnitVectorEpsilon =
{
	1.0e-4f, 1.0e-4f, 1.0e-4f, 1.0e-4f
};
static inline BOOL XMVector3AnyTrue(FXMVECTOR V)
{
	XMVECTOR C;

	// Duplicate the fourth element from the first element.
	C = XMVectorSwizzle(V, 0, 1, 2, 0);

	return XMComparisonAnyTrue(XMVector4EqualIntR(C, XMVectorTrueInt()));
}
static inline BOOL XMVector3IsUnit(FXMVECTOR V)
{
	XMVECTOR Difference = XMVector3Length(V) - XMVectorSplatOne();

	return XMVector4Less(XMVectorAbs(Difference), g_UnitVectorEpsilon);
}
bool IntersectRayAxisAlignedBox(FXMVECTOR Origin, FXMVECTOR Direction, const AxisAlignedBox* pVolume, FLOAT* pDist)
{
	assert(pVolume);
	assert(pDist);
	assert(XMVector3IsUnit(Direction));

	static const XMVECTOR Epsilon =
	{
		1e-20f, 1e-20f, 1e-20f, 1e-20f
	};

	static const XMVECTOR FltMin =
	{
		-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX
	};

	static const XMVECTOR FltMax =
	{
		FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX
	};

	XMVECTOR Center = XMLoadFloat3(&pVolume->Center);
	XMVECTOR Extents = XMLoadFloat3(&pVolume->Extents);

	// Adjust ray origin to be relative to center of the box.
	XMVECTOR TOrigin = Center - Origin;

	// Compute the dot product againt each axis of the box.
	// Since the axii are (1,0,0), (0,1,0), (0,0,1) no computation is necessary.
	XMVECTOR AxisDotOrigin = TOrigin;
	XMVECTOR AxisDotDirection = Direction;

	// if (fabs(AxisDotDirection) <= Epsilon) the ray is nearly parallel to the slab.
	XMVECTOR IsParallel = XMVectorLessOrEqual(XMVectorAbs(AxisDotDirection), Epsilon);

	// Test against all three axii simultaneously.
	XMVECTOR InverseAxisDotDirection = XMVectorReciprocal(AxisDotDirection);
	XMVECTOR t1 = (AxisDotOrigin - Extents) * InverseAxisDotDirection;
	XMVECTOR t2 = (AxisDotOrigin + Extents) * InverseAxisDotDirection;

	// Compute the max of min(t1,t2) and the min of max(t1,t2) ensuring we don't
	// use the results from any directions parallel to the slab.
	XMVECTOR t_min = XMVectorSelect(XMVectorMin(t1, t2), FltMin, IsParallel);
	XMVECTOR t_max = XMVectorSelect(XMVectorMax(t1, t2), FltMax, IsParallel);

	// t_min.x = maximum( t_min.x, t_min.y, t_min.z );
	// t_max.x = minimum( t_max.x, t_max.y, t_max.z );
	t_min = XMVectorMax(t_min, XMVectorSplatY(t_min));  // x = max(x,y)
	t_min = XMVectorMax(t_min, XMVectorSplatZ(t_min));  // x = max(max(x,y),z)
	t_max = XMVectorMin(t_max, XMVectorSplatY(t_max));  // x = min(x,y)
	t_max = XMVectorMin(t_max, XMVectorSplatZ(t_max));  // x = min(min(x,y),z)

	// if ( t_min > t_max ) return FALSE;
	XMVECTOR NoIntersection = XMVectorGreater(XMVectorSplatX(t_min), XMVectorSplatX(t_max));

	// if ( t_max < 0.0f ) return FALSE;
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(XMVectorSplatX(t_max), XMVectorZero()));

	// if (IsParallel && (-Extents > AxisDotOrigin || Extents < AxisDotOrigin)) return FALSE;
	XMVECTOR ParallelOverlap = XMVectorInBounds(AxisDotOrigin, Extents);
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorAndCInt(IsParallel, ParallelOverlap));

	if (!XMVector3AnyTrue(NoIntersection))
	{
		// Store the x-component to *pDist
		XMStoreFloat(pDist, t_min);
		return TRUE;
	}

	return FALSE;

}
bool IntersectRayTriangle(FXMVECTOR Origin, FXMVECTOR Direction, FXMVECTOR V0, CXMVECTOR V1, CXMVECTOR V2, FLOAT* pDist)
{
	assert(pDist);
	assert(XMVector3IsUnit(Direction));

	static const XMVECTOR Epsilon =
	{
		1e-20f, 1e-20f, 1e-20f, 1e-20f
	};

	XMVECTOR Zero = XMVectorZero();

	XMVECTOR e1 = V1 - V0;
	XMVECTOR e2 = V2 - V0;

	// p = Direction ^ e2;
	XMVECTOR p = XMVector3Cross(Direction, e2);

	// det = e1 * p;
	XMVECTOR det = XMVector3Dot(e1, p);

	XMVECTOR u, v, t;

	if (XMVector3GreaterOrEqual(det, Epsilon))
	{
		// Determinate is positive (front side of the triangle).
		XMVECTOR s = Origin - V0;

		// u = s * p;
		u = XMVector3Dot(s, p);

		XMVECTOR NoIntersection = XMVectorLess(u, Zero);
		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(u, det));

		// q = s ^ e1;
		XMVECTOR q = XMVector3Cross(s, e1);

		// v = Direction * q;
		v = XMVector3Dot(Direction, q);

		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(v, Zero));
		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(u + v, det));

		// t = e2 * q;
		t = XMVector3Dot(e2, q);

		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(t, Zero));

		if (XMVector4EqualInt(NoIntersection, XMVectorTrueInt()))
			return FALSE;
	}
	else if (XMVector3LessOrEqual(det, -Epsilon))
	{
		// Determinate is negative (back side of the triangle).
		XMVECTOR s = Origin - V0;

		// u = s * p;
		u = XMVector3Dot(s, p);

		XMVECTOR NoIntersection = XMVectorGreater(u, Zero);
		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(u, det));

		// q = s ^ e1;
		XMVECTOR q = XMVector3Cross(s, e1);

		// v = Direction * q;
		v = XMVector3Dot(Direction, q);

		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(v, Zero));
		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(u + v, det));

		// t = e2 * q;
		t = XMVector3Dot(e2, q);

		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(t, Zero));

		if (XMVector4EqualInt(NoIntersection, XMVectorTrueInt()))
			return FALSE;
	}
	else
	{
		// Parallel ray.
		return FALSE;
	}

	XMVECTOR inv_det = XMVectorReciprocal(det);

	t *= inv_det;

	// u * inv_det and v * inv_det are the barycentric cooridinates of the intersection.

	// Store the x-component to *pDist
	XMStoreFloat(pDist, t);

	return TRUE;
}
void Pick(int sx, int sy)
{
	XMMATRIX P = g_Camera.GetProjMatrix();

	// Compute picking ray in view space.
	float vx = (+2.0f * sx / 800 - 1.0f) / P.r[0].m128_f32[0];
	float vy = (-2.0f * sy / 600 + 1.0f) / P.r[1].m128_f32[1];

	// Ray definition in view space.
	XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);

	// Tranform ray to local space of Mesh.
	XMMATRIX V = g_Camera.GetViewMatrix();
	XMVECTOR detV = XMMatrixDeterminant(V);
	XMMATRIX invView = XMMatrixInverse(&detV, V);

	XMMATRIX W = XMLoadFloat4x4(&g_CarWorld);
	XMVECTOR detW = XMMatrixDeterminant(W);
	XMMATRIX invWorld = XMMatrixInverse(&detW, W);

	XMMATRIX toLocal = XMMatrixMultiply(invView, invWorld);

	rayOrigin = XMVector3TransformCoord(rayOrigin, toLocal);
	rayDir = XMVector3TransformNormal(rayDir, toLocal);

	// Make the ray direction unit length for the intersection tests.
	rayDir = XMVector3Normalize(rayDir);

	// If we hit the bounding box of the Mesh, then we might have picked a Mesh triangle,
	// so do the ray/triangle tests.
	//
	// If we did not hit the bounding box, then it is impossible that we hit 
	// the Mesh, so do not waste effort doing ray/triangle tests.

	// Assume we have not picked anything yet, so init to -1.
	g_PickedTriangle = -1;
	float tmin = 0.0f;
	if (IntersectRayAxisAlignedBox(rayOrigin, rayDir, &g_CarBox, &tmin))
	{
		// Find the nearest ray/triangle intersection.
		tmin = MathHelper::Infinity;
		for (UINT i = 0; i < g_CarIndices.size() / 3; ++i)
		{
			// Indices for this triangle.
			UINT i0 = g_CarIndices[i * 3 + 0];
			UINT i1 = g_CarIndices[i * 3 + 1];
			UINT i2 = g_CarIndices[i * 3 + 2];

			// Vertices for this triangle.
			XMVECTOR v0 = XMLoadFloat3(&g_CarVertices[i0].Pos);
			XMVECTOR v1 = XMLoadFloat3(&g_CarVertices[i1].Pos);
			XMVECTOR v2 = XMLoadFloat3(&g_CarVertices[i2].Pos);

			// We have to iterate over all the triangles in order to find the nearest intersection.
			float t = 0.0f;
			if (IntersectRayTriangle(rayOrigin, rayDir, v0, v1, v2, &t))
			{
				if (t < tmin)
				{
					// This is the new nearest picked triangle.
					tmin = t;
					g_PickedTriangle = i;
				}
			}
		}
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

	BuildLighting();
	Effects::InitAll(pd3dDevice);
	InputLayouts::InitAll(pd3dDevice);
	// RenderStates::InitAll(pd3dDevice);

	g_Sky = new Sky(pd3dDevice, L"D:/Work/DirectX/Chapter17/CubeMap/Textures/grasscube1024.dds", 5000.0f);


	g_PickedTriangle = -1;


	g_PickedTriangleMaterial.Ambient = XMFLOAT4(0.0f, 0.8f, 0.4f, 1.0f);
	g_PickedTriangleMaterial.Diffuse = XMFLOAT4(0.0f, 0.8f, 0.4f, 1.0f);
	g_PickedTriangleMaterial.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);

	//
	// Car
	//
	g_CarMaterial.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	g_CarMaterial.Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	g_CarMaterial.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	g_CarMaterial.Reflect = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

	XMMATRIX CarScale = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	XMMATRIX CarOffset = XMMatrixTranslation(0.0f, 1.0f, 0.0f);
	XMStoreFloat4x4(&g_CarWorld, XMMatrixMultiply(CarScale, CarOffset));
	BuildMeshGeometryBuffers(pd3dDevice);

	//
	// Create Camera
	//
	static const XMVECTORF32 s_Eye = { 10.0f, 2.0f, -10.0f, 0.0f };
	static const XMVECTORF32 s_LookAt = { 0.0f, 1.0f, 0.0f, 0.0f };
	g_Camera.SetViewParams(s_Eye, s_LookAt);

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

	pd3dImmediateContext->IASetInputLayout(InputLayouts::Basic32);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	XMMATRIX view = g_Camera.GetViewMatrix();
	XMMATRIX proj = g_Camera.GetProjMatrix();
	XMMATRIX viewProj = view * proj;

	XMFLOAT3 eyePos;
	XMStoreFloat3(&eyePos, g_Camera.GetEyePt());
	Effects::BasicFX->SetEyePosW(eyePos);
	Effects::BasicFX->SetDirLights(g_DirectionalLights);
	Effects::BasicFX->SetCubeMap(g_Sky->GetSkyCubeMap());

	ID3DX11EffectTechnique* activeCarTech = Effects::BasicFX->Light3ReflectTech;
	ID3DX11EffectTechnique* activeTexTech = Effects::BasicFX->Light3TexTech;
	ID3DX11EffectTechnique* activeReflectTech = Effects::BasicFX->Light3TexReflectTech;

	D3DX11_TECHNIQUE_DESC techDesc;
	activeCarTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; p++)
	{
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_CarVertexBuffer, &stride, &offset);
		pd3dImmediateContext->IASetIndexBuffer(g_CarIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		XMMATRIX world = XMLoadFloat4x4(&g_CarWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world * view * proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetMaterial(g_CarMaterial);

		activeCarTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(g_CarIndexCount, 0, 0);

		// pd3dImmediateContext->RSSetState(0);

		/*
		if (g_PickedTriangle != -1)
		{
			// pd3dImmediateContext->OMSetDepthStencilState(RenderStates::LessEqualDSS, 0);

			Effects::BasicFX->SetMaterial(g_PickedTriangleMaterial);
			activeCarTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
			pd3dImmediateContext->DrawIndexed(3, 3 * g_PickedTriangle, 0);

			// restore default
			pd3dImmediateContext->OMSetDepthStencilState(0, 0);
		}
		*/
	}

	g_Sky->Draw(pd3dImmediateContext, g_Camera);

	pd3dImmediateContext->RSSetState(0);
	pd3dImmediateContext->OMSetDepthStencilState(0, 0);
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

	if ((wParam & MK_LBUTTON) != 0)
	{
		int xPos = GET_X_LPARAM(lParam);  // Lấy vị trí X của chuột
		int yPos = GET_Y_LPARAM(lParam);  // Lấy vị trí Y của chuột
		Pick(xPos, yPos);  // Gọi hàm Pick để chọn đối tượng

	}
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