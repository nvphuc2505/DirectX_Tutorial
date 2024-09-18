#include <DXUT.h>
#include <SDKmisc.h>
#include <DXUTcamera.h>
#include <fstream>
#include <windowsx.h>
#include "Vertex.h"
#include "Effects.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "RenderStates.h"
#include "Sky.h"



//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------

CModelViewerCamera g_Camera;
CModelViewerCamera g_CubeMapCamera[6];

static const int CubeMapSize = 256;

ID3D11DepthStencilView*		g_DynamicCubeMapDSV;
ID3D11RenderTargetView*		g_DynamicCubeMapRTV[6];
ID3D11ShaderResourceView*	g_DynamicCubeMapSRV;
D3D11_VIEWPORT				g_CubeMapViewport;

UINT				g_LightCount;
DirectionalLight	g_DirectionalLights[3];
Material			mGridMat;
Material			mBoxMat;
Material			mCylinderMat;
Material			mSphereMat;
Material			mSkullMat;
Material			mCenterSphereMat;

Sky*		  g_Sky;

UINT		  g_SkullIndexCount;
ID3D11Buffer* g_SkullVertexBuffer;
ID3D11Buffer* g_SkullIndexBuffer;
XMFLOAT4X4    g_SkullWorld;

ID3D11Buffer* mShapesVB;
ID3D11Buffer* mShapesIB;


XMFLOAT4X4 mSphereWorld[10];
XMFLOAT4X4 mCylWorld[10];
XMFLOAT4X4 mBoxWorld;
XMFLOAT4X4 mGridWorld;
XMFLOAT4X4 mCenterSphereWorld;

int mBoxVertexOffset;
int mGridVertexOffset;
int mSphereVertexOffset;
int mCylinderVertexOffset;

UINT mBoxIndexOffset;
UINT mGridIndexOffset;
UINT mSphereIndexOffset;
UINT mCylinderIndexOffset;

UINT mBoxIndexCount;
UINT mGridIndexCount;
UINT mSphereIndexCount;
UINT mCylinderIndexCount;

ID3D11ShaderResourceView* mFloorTexSRV;
ID3D11ShaderResourceView* mStoneTexSRV;
ID3D11ShaderResourceView* mBrickTexSRV;

ID3D11ShaderResourceView* mStoneNormalTexSRV;
ID3D11ShaderResourceView* mBrickNormalTexSRV;
ID3D11ShaderResourceView* mFloorNormalTexSRV;






//--------------------------------------------------------------------------------------
// Function Helper
//--------------------------------------------------------------------------------------
void BuildDynamicCubeMapViews(ID3D11Device* pd3dDevice)
{
	//
	// Building the Cube Map
	//
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = CubeMapSize;
	texDesc.Height = CubeMapSize;
	texDesc.MipLevels = 0;
	texDesc.ArraySize = 6;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE;

	ID3D11Texture2D* cubeTex = 0;
	HRESULT hr = pd3dDevice->CreateTexture2D(&texDesc, 0, &cubeTex);


	
	//
	// Building Render Target View
	//
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.ArraySize = 1;
	rtvDesc.Texture2DArray.MipSlice = 0;
	for (int i = 0; i < 6; ++i)
	{
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		hr = pd3dDevice->CreateRenderTargetView(cubeTex, &rtvDesc, &g_DynamicCubeMapRTV[i]);
	}



	//
	// Building Shader Resource View
	//
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = -1;
	hr = pd3dDevice->CreateShaderResourceView(cubeTex, &srvDesc, &g_DynamicCubeMapSRV);



	//
	// Building Depth Stencil View
	//
	D3D11_TEXTURE2D_DESC depthTexDesc;
	depthTexDesc.Width = CubeMapSize;
	depthTexDesc.Height = CubeMapSize;
	depthTexDesc.MipLevels = 1;
	depthTexDesc.ArraySize = 1;
	depthTexDesc.SampleDesc.Count = 1;
	depthTexDesc.SampleDesc.Quality = 0;
	depthTexDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthTexDesc.Usage = D3D11_USAGE_DEFAULT;
	depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthTexDesc.CPUAccessFlags = 0;
	depthTexDesc.MiscFlags = 0;
	ID3D11Texture2D* depthTex = 0;
	hr = pd3dDevice->CreateTexture2D(&depthTexDesc, 0, &depthTex);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = depthTexDesc.Format;
	dsvDesc.Flags = 0;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	hr = pd3dDevice->CreateDepthStencilView(depthTex, &dsvDesc, &g_DynamicCubeMapDSV);



	//
	// Viewport for drawing into cubemap.
	// 
	g_CubeMapViewport.TopLeftX = 0.0f;
	g_CubeMapViewport.TopLeftY = 0.0f;
	g_CubeMapViewport.Width    = (float)CubeMapSize;
	g_CubeMapViewport.Height   = (float)CubeMapSize;
	g_CubeMapViewport.MinDepth = 0.0f;
	g_CubeMapViewport.MaxDepth = 1.0f;
}
void BuildCubeFaceCamera(float x, float y, float z)
{
	XMFLOAT3 center(x, y, z);
	XMFLOAT3 worldUp(0.0f, 1.0f, 0.0f);

	// Look along each coordinate axis.
	XMFLOAT3 targets[6] =
	{
		XMFLOAT3(x + 1.0f, y, z), // +X
		XMFLOAT3(x - 1.0f, y, z), // -X
		XMFLOAT3(x, y + 1.0f, z), // +Y
		XMFLOAT3(x, y - 1.0f, z), // -Y
		XMFLOAT3(x, y, z + 1.0f), // +Z
		XMFLOAT3(x, y, z - 1.0f)  // -Z
	};

	// Use world up vector (0,1,0) for all directions except +Y/-Y.  In these cases, we
	// are looking down +Y or -Y, so we need a different "up" vector.
	XMFLOAT3 ups[6] =
	{
		XMFLOAT3(0.0f, 1.0f, 0.0f),  // +X
		XMFLOAT3(0.0f, 1.0f, 0.0f),  // -X
		XMFLOAT3(0.0f, 0.0f, -1.0f), // +Y
		XMFLOAT3(0.0f, 0.0f, +1.0f), // -Y
		XMFLOAT3(0.0f, 1.0f, 0.0f),	 // +Z
		XMFLOAT3(0.0f, 1.0f, 0.0f)	 // -Z
	};

	for (int i = 0; i < 6; ++i)
	{
		XMVECTOR eyePosition = XMLoadFloat3(&center);
		XMVECTOR target = XMLoadFloat3(&targets[i]);
		XMVECTOR up = XMLoadFloat3(&ups[i]);

		g_CubeMapCamera[i].SetViewParams(eyePosition, target);
		g_CubeMapCamera[i].SetProjParams(0.5f * XM_PI, 1.0f, 0.1f, 1000.0f);
		g_CubeMapCamera[i].SetModelCenter(center);
	}
}
void BuildSkullGeometryBuffers(ID3D11Device* pd3dDevice)
{
	std::ifstream fin("D:/Work/Direct3D11/Exc/Chapter10/Models/skull.txt");
	if (!fin)
	{
		MessageBox(0, L"skull.txt not found!", 0, 0);
		return;
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
	HRESULT hr = (pd3dDevice->CreateBuffer(&skullVBD, &skullVInitData, &g_SkullVertexBuffer));

	D3D11_BUFFER_DESC skullIBD = {};
	skullIBD.ByteWidth = sizeof(UINT) * g_SkullIndexCount;
	skullIBD.Usage = D3D11_USAGE_IMMUTABLE;
	skullIBD.BindFlags = D3D11_BIND_INDEX_BUFFER;
	skullIBD.CPUAccessFlags = 0;
	skullIBD.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA skullIInitData;
	skullIInitData.pSysMem = &skullIndices[0];
	hr = (pd3dDevice->CreateBuffer(&skullIBD, &skullIInitData, &g_SkullIndexBuffer));
}
void BuildShapeGeometryBuffers(ID3D11Device* md3dDevice)
{
	GeometryGenerator::MeshData box;
	GeometryGenerator::MeshData grid;
	GeometryGenerator::MeshData sphere;
	GeometryGenerator::MeshData cylinder;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);
	geoGen.CreateGrid(20.0f, 30.0f, 60, 40, grid);
	geoGen.CreateSphere(0.5f, 20, 20, sphere);
	geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20, cylinder);

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	mBoxVertexOffset = 0;
	mGridVertexOffset = box.Vertices.size();
	mSphereVertexOffset = mGridVertexOffset + grid.Vertices.size();
	mCylinderVertexOffset = mSphereVertexOffset + sphere.Vertices.size();

	// Cache the index count of each object.
	mBoxIndexCount = box.Indices.size();
	mGridIndexCount = grid.Indices.size();
	mSphereIndexCount = sphere.Indices.size();
	mCylinderIndexCount = cylinder.Indices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	mBoxIndexOffset = 0;
	mGridIndexOffset = mBoxIndexCount;
	mSphereIndexOffset = mGridIndexOffset + mGridIndexCount;
	mCylinderIndexOffset = mSphereIndexOffset + mSphereIndexCount;

	UINT totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();

	UINT totalIndexCount =
		mBoxIndexCount +
		mGridIndexCount +
		mSphereIndexCount +
		mCylinderIndexCount;

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	std::vector<Vertex::PosNormalTexTan> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].Tex = box.Vertices[i].TexC;
		vertices[k].TangentU = box.Vertices[i].TangentU;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].Tex = grid.Vertices[i].TexC;
		vertices[k].TangentU = grid.Vertices[i].TangentU;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].Tex = sphere.Vertices[i].TexC;
		vertices[k].TangentU = sphere.Vertices[i].TangentU;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].Tex = cylinder.Vertices[i].TexC;
		vertices[k].TangentU = cylinder.Vertices[i].TangentU;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::PosNormalTexTan) * totalVertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HRESULT hr = (md3dDevice->CreateBuffer(&vbd, &vinitData, &mShapesVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	std::vector<UINT> indices;
	indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());
	indices.insert(indices.end(), grid.Indices.begin(), grid.Indices.end());
	indices.insert(indices.end(), sphere.Indices.begin(), sphere.Indices.end());
	indices.insert(indices.end(), cylinder.Indices.begin(), cylinder.Indices.end());

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * totalIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	hr = (md3dDevice->CreateBuffer(&ibd, &iinitData, &mShapesIB));
}
void DrawScene(ID3D11DeviceContext* pd3dImmediateContext, CModelViewerCamera camera, bool drawCenterSphere)
{
	pd3dImmediateContext->IASetInputLayout(InputLayouts::Basic32);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	XMMATRIX view = g_Camera.GetViewMatrix();
	XMMATRIX proj = g_Camera.GetProjMatrix();
	XMMATRIX viewProj = view * proj;

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	XMFLOAT3 eyePos;
	XMStoreFloat3(&eyePos, g_Camera.GetEyePt());
	Effects::BasicFX->SetEyePosW(eyePos);
	Effects::BasicFX->SetDirLights(g_DirectionalLights);
	Effects::BasicFX->SetCubeMap(g_Sky->GetSkyCubeMap());


	ID3DX11EffectTechnique* activeTexTech = Effects::BasicFX->Light3TexTech;
	ID3DX11EffectTechnique* activeSkullTech = Effects::BasicFX->Light3ReflectTech;
	ID3DX11EffectTechnique* activeReflectTech = Effects::BasicFX->Light3ReflectTech;
	
	XMMATRIX world;
	XMMATRIX worldInvTranspose;
	XMMATRIX worldViewProj;

	//
	// Draw the skull.
	//
	D3DX11_TECHNIQUE_DESC techDesc;
	activeSkullTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_SkullVertexBuffer, &stride, &offset);
		pd3dImmediateContext->IASetIndexBuffer(g_SkullIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		world = XMLoadFloat4x4(&g_SkullWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
		Effects::BasicFX->SetMaterial(mSkullMat);
		Effects::BasicFX->SetCubeMap(g_DynamicCubeMapSRV);

		activeSkullTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(g_SkullIndexCount, 0, 0);
	}

	pd3dImmediateContext->IASetVertexBuffers(0, 1, &mShapesVB, &stride, &offset);
	pd3dImmediateContext->IASetIndexBuffer(mShapesIB, DXGI_FORMAT_R32_UINT, 0);

	activeTexTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		// Draw the grid.
		world = XMLoadFloat4x4(&mGridWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMMatrixScaling(6.0f, 8.0f, 1.0f));
		Effects::BasicFX->SetMaterial(mGridMat);
		Effects::BasicFX->SetDiffuseMap(mFloorTexSRV);

		activeTexTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);

		// Draw the box.
		world = XMLoadFloat4x4(&mBoxWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
		Effects::BasicFX->SetMaterial(mBoxMat);
		Effects::BasicFX->SetDiffuseMap(mStoneTexSRV);

		activeTexTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

		// Draw the cylinders.
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&mCylWorld[i]);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;

			Effects::BasicFX->SetWorld(world);
			Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::BasicFX->SetWorldViewProj(worldViewProj);
			Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
			Effects::BasicFX->SetMaterial(mCylinderMat);
			Effects::BasicFX->SetDiffuseMap(mBrickTexSRV);

			activeTexTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
			pd3dImmediateContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
		}

		// Draw the spheres.
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&mSphereWorld[i]);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;

			Effects::BasicFX->SetWorld(world);
			Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::BasicFX->SetWorldViewProj(worldViewProj);
			Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
			Effects::BasicFX->SetMaterial(mSphereMat);
			Effects::BasicFX->SetDiffuseMap(mStoneTexSRV);

			activeTexTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
			pd3dImmediateContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
		}
	}



	//
	// Draw the center sphere with the dynamic cube map.
	//
	if(drawCenterSphere)
	{
		activeReflectTech->GetDesc(&techDesc);
		for (UINT p = 0; p < techDesc.Passes; ++p)
		{
			// Draw the center sphere.

			world = XMLoadFloat4x4(&mCenterSphereWorld);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;

			Effects::BasicFX->SetWorld(world);
			Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::BasicFX->SetWorldViewProj(worldViewProj);
			Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
			Effects::BasicFX->SetMaterial(mCenterSphereMat);
			Effects::BasicFX->SetDiffuseMap(mStoneTexSRV);
			Effects::BasicFX->SetCubeMap(g_DynamicCubeMapSRV);

			activeReflectTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
			pd3dImmediateContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
		}

	}


	g_Sky->Draw(pd3dImmediateContext, g_Camera);
	pd3dImmediateContext->RSSetState(0);
	pd3dImmediateContext->OMSetDepthStencilState(0, 0);
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

	g_Sky = new Sky(pd3dDevice, L"Textures/sunsetcube1024.dds", 5000.0f);

	hr = (DXUTCreateShaderResourceViewFromFile(pd3dDevice, L"Textures/floor.dds", &mFloorTexSRV));
	hr = (DXUTCreateShaderResourceViewFromFile(pd3dDevice, L"Textures/stone.dds", &mStoneTexSRV));
	hr = (DXUTCreateShaderResourceViewFromFile(pd3dDevice, L"Textures/bricks.dds", &mBrickTexSRV));
	hr = (DXUTCreateShaderResourceViewFromFile(pd3dDevice, L"Textures/bricks_nmap.dds", &mBrickNormalTexSRV));
	hr = (DXUTCreateShaderResourceViewFromFile(pd3dDevice, L"Textures/stones_nmap.dds", &mStoneNormalTexSRV));
	hr = (DXUTCreateShaderResourceViewFromFile(pd3dDevice, L"Textures/floor_nmap.dds", &mFloorNormalTexSRV));

	Effects::InitAll(pd3dDevice);
	InputLayouts::InitAll(pd3dDevice);

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

	BuildCubeFaceCamera(0.0f, 2.0f, 0.0f);
	BuildDynamicCubeMapViews(pd3dDevice);
	BuildSkullGeometryBuffers(pd3dDevice);
	BuildShapeGeometryBuffers(pd3dDevice);

	mGridMat.Ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mGridMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mGridMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
	mGridMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	mCylinderMat.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mCylinderMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mCylinderMat.Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 32.0f);
	mCylinderMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	mSphereMat.Ambient = XMFLOAT4(0.6f, 0.8f, 1.0f, 1.0f);
	mSphereMat.Diffuse = XMFLOAT4(0.6f, 0.8f, 1.0f, 1.0f);
	mSphereMat.Specular = XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f);
	mSphereMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	mBoxMat.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mBoxMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mBoxMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	mBoxMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	mSkullMat.Ambient = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	mSkullMat.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mSkullMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	mSkullMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	mCenterSphereMat.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mCenterSphereMat.Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mCenterSphereMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	mCenterSphereMat.Reflect = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);




	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mGridWorld, I);

	XMMATRIX boxScale = XMMatrixScaling(3.0f, 1.0f, 3.0f);
	XMMATRIX boxOffset = XMMatrixTranslation(0.0f, 0.5f, 0.0f);
	XMStoreFloat4x4(&mBoxWorld, XMMatrixMultiply(boxScale, boxOffset));

	XMMATRIX centerSphereScale = XMMatrixScaling(2.0f, 2.0f, 2.0f);
	XMMATRIX centerSphereOffset = XMMatrixTranslation(0.0f, 2.0f, 0.0f);
	XMStoreFloat4x4(&mCenterSphereWorld, XMMatrixMultiply(centerSphereScale, centerSphereOffset));

	for (int i = 0; i < 5; ++i)
	{
		XMStoreFloat4x4(&mCylWorld[i * 2 + 0], XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f));
		XMStoreFloat4x4(&mCylWorld[i * 2 + 1], XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f));

		XMStoreFloat4x4(&mSphereWorld[i * 2 + 0], XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f));
		XMStoreFloat4x4(&mSphereWorld[i * 2 + 1], XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f));
	}



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
	pd3dImmediateContext->ClearRenderTargetView(pRTV, reinterpret_cast<const float*>(&Colors::Silver));

	pd3dImmediateContext->IASetInputLayout(InputLayouts::PosNormalTexTan);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	XMMATRIX view = g_Camera.GetViewMatrix();
	XMMATRIX proj = g_Camera.GetProjMatrix();
	XMMATRIX viewProj = view * proj;

	XMFLOAT3 eyePos;
	XMStoreFloat3(&eyePos, g_Camera.GetEyePt());
	
	Effects::BasicFX->SetDirLights(g_DirectionalLights);
	Effects::BasicFX->SetEyePosW(eyePos);
	Effects::BasicFX->SetCubeMap(g_Sky->GetSkyCubeMap());

	Effects::NormalMapFX->SetEyePosW(eyePos);
	Effects::NormalMapFX->SetDirLights(g_DirectionalLights);
	Effects::NormalMapFX->SetCubeMap(g_Sky->GetSkyCubeMap());

	XMMATRIX world;
	XMMATRIX worldInvTranspose;
	XMMATRIX worldViewProj;

	

	ID3DX11EffectTechnique* activeTech = Effects::NormalMapFX->Light3TexTech;
	ID3DX11EffectTechnique* activeSphereTech = Effects::BasicFX->Light3ReflectTech;
	ID3DX11EffectTechnique* activeSkullTech = Effects::BasicFX->Light3ReflectTech;



	//
	// Draw the grid, cylinders, and box without any cubemap reflection.
	// 
	UINT stride = sizeof(Vertex::PosNormalTexTan);
	UINT offset = 0;

	pd3dImmediateContext->IASetInputLayout(InputLayouts::PosNormalTexTan);
	pd3dImmediateContext->IASetVertexBuffers(0, 1, &mShapesVB, &stride, &offset);
	pd3dImmediateContext->IASetIndexBuffer(mShapesIB, DXGI_FORMAT_R32_UINT, 0);

	D3DX11_TECHNIQUE_DESC techDesc;
	activeTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		// Draw the grid.
		world = XMLoadFloat4x4(&mGridWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		Effects::NormalMapFX->SetWorld(world);
		Effects::NormalMapFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::NormalMapFX->SetWorldViewProj(worldViewProj);
		Effects::NormalMapFX->SetTexTransform(XMMatrixScaling(8.0f, 10.0f, 1.0f));
		Effects::NormalMapFX->SetMaterial(mGridMat);
		Effects::NormalMapFX->SetDiffuseMap(mFloorTexSRV);
		Effects::NormalMapFX->SetNormalMap(mFloorNormalTexSRV);

		activeTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);

		// Draw the box.
		world = XMLoadFloat4x4(&mBoxWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

			Effects::NormalMapFX->SetWorld(world);
			Effects::NormalMapFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::NormalMapFX->SetWorldViewProj(worldViewProj);
			Effects::NormalMapFX->SetTexTransform(XMMatrixScaling(2.0f, 1.0f, 1.0f));
			Effects::NormalMapFX->SetMaterial(mBoxMat);
			Effects::NormalMapFX->SetDiffuseMap(mBrickTexSRV);
			Effects::NormalMapFX->SetNormalMap(mBrickNormalTexSRV);
			

		activeTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

		// Draw the cylinders.
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&mCylWorld[i]);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;

			Effects::NormalMapFX->SetWorld(world);
			Effects::NormalMapFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::NormalMapFX->SetWorldViewProj(worldViewProj);
			Effects::NormalMapFX->SetTexTransform(XMMatrixScaling(1.0f, 2.0f, 1.0f));
			Effects::NormalMapFX->SetMaterial(mCylinderMat);
			Effects::NormalMapFX->SetDiffuseMap(mStoneTexSRV);
			Effects::NormalMapFX->SetNormalMap(mStoneNormalTexSRV);

			activeTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
			pd3dImmediateContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
		}
	}

	g_Sky->Draw(pd3dImmediateContext, g_Camera);
}




//--------------------------------------------------------------------------------------
// Handle updates to the scene.
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	g_Camera.FrameMove(fElapsedTime);
	
	XMMATRIX skullScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
	XMMATRIX skullOffset = XMMatrixTranslation(3.0f, 2.0f, 0.0f);
	XMMATRIX skullLocalRotate = XMMatrixRotationY(2.0f * fTime);
	XMMATRIX skullGlobalRotate = XMMatrixRotationY(3.0f * fTime);
	XMStoreFloat4x4(&g_SkullWorld, skullScale * skullLocalRotate * skullOffset * skullGlobalRotate);
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