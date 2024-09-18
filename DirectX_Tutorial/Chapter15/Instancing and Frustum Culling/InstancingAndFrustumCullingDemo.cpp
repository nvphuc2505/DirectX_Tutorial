	#include <DXUT.h>
	#include <DXUTcamera.h>
	#include <SDKmisc.h>
	#include <fstream>
	#include <string>
	#include <sstream>
	#include <iostream>
	#include "Vertex.h"
	#include "MathHelper.h"
	#include "LightHelper.h"
	#include "Effects.h"
	#include "Windows.h"

	using namespace DirectX;

	//--------------------------------------------------------------------------------------
	// Global Variables
	//--------------------------------------------------------------------------------------
	struct AxisAlignedBox
	{
		XMFLOAT3 Center;            // Center of the box.
		XMFLOAT3 Extents;           // Distance from the center to each side.
	};

	struct OrientedBox
	{
		XMFLOAT3 Center;            // Center of the box.
		XMFLOAT3 Extents;           // Distance from the center to each side.
		XMFLOAT4 Orientation;       // Unit quaternion representing rotation (box -> world).
	};

	struct Frustum
	{
		XMFLOAT3 Origin;            // Origin of the frustum (and projection).
		XMFLOAT4 Orientation;       // Unit quaternion representing rotation.

		float RightSlope;           // Positive X slope (X/Z).
		float LeftSlope;            // Negative X slope.
		float TopSlope;             // Positive Y slope (Y/Z).
		float BottomSlope;          // Negative Y slope.
		float Near, Far;            // Z of the near plane and far plane.
	};

	struct InstancedData
	{
		XMFLOAT4X4 World;
		XMFLOAT4 Color;
	};

	UINT			g_SkullIndexCount;
	ID3D11Buffer*	g_SkullIndexBuffer;
	ID3D11Buffer*	g_SkullVetexBuffer;
	ID3D11Buffer*	g_SkullInstancedBuffer;
	XMFLOAT4X4		g_SkullWorld;
	AxisAlignedBox  g_SkullBox;

	CModelViewerCamera			g_Camera;
	Frustum						g_CameraFrustum;

	std::vector <InstancedData> g_InstancedData;

	DirectionalLight	g_DirectionLight[3];
	Material			g_SkullMaterial;

	bool bUseFrustumCulling = true;
	UINT g_VisibleObjectCount;
	std::wstring mMainWndCaption;
		
	//--------------------------------------------------------------------------------------
	// FUNCTIONAL HELPER
	//--------------------------------------------------------------------------------------
	void BuildSkullGeometryBuffers(ID3D11Device* pd3dDevice)
	{
		std::ifstream fin("D:/Work/DirectX/Chapter15/Instancing and Frustum Culling/Models/skull.txt");
		if (!fin)
		{
			MessageBox(0, L"Failed to open model file", 0, 0);
			return;
		}

		UINT vcount = 0;
		UINT icount = 0;
		std::string ignore;

		fin >> ignore >> vcount;
		fin >> ignore >> icount;
		fin >> ignore >> ignore >> ignore >> ignore;

		XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
		XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

		XMVECTOR vMin = XMLoadFloat3(&vMinf3);
		XMVECTOR vMax = XMLoadFloat3(&vMaxf3);
		std::vector<Vertex::Basic32> vertices(vcount);
		for (UINT i = 0; i < vcount; ++i)
		{
			fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
			fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

			XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);

			vMin = XMVectorMin(vMin, P);
			vMax = XMVectorMax(vMax, P);
		}

		XMStoreFloat3(&g_SkullBox.Center, 0.5f * (vMin + vMax));
		XMStoreFloat3(&g_SkullBox.Extents, 0.5f * (vMax - vMin));

	


		fin >> ignore;
		fin >> ignore;
		fin >> ignore;

		g_SkullIndexCount = 3 * icount;
		std::vector<UINT> indices(g_SkullIndexCount);
		for (UINT i = 0; i < icount; ++i)
		{
			fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
		}

		fin.close();

		D3D11_BUFFER_DESC vbd;
		vbd.ByteWidth = sizeof(Vertex::Basic32) * vcount;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vInitData;
		vInitData.pSysMem = &vertices[0];
		HRESULT hr = pd3dDevice->CreateBuffer(&vbd, &vInitData, &g_SkullVetexBuffer);

		D3D11_BUFFER_DESC ibd;
		ibd.ByteWidth = sizeof(UINT) * g_SkullIndexCount;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iInitData;
		iInitData.pSysMem = &indices[0];
		hr = pd3dDevice->CreateBuffer(&ibd, &iInitData, &g_SkullIndexBuffer);
	}

	void BuildInstancedBuffer(ID3D11Device* pd3dDevice)
	{
		const int n = 5;
		g_InstancedData.resize(n * n * n);

		float width = 200.0f;
		float height = 200.0f;
		float depth = 200.0f;

		float x = -0.5f * width;
		float y = -0.5f * height;
		float z = -0.5f * depth;
		float dx = width / (n - 1);
		float dy = height / (n - 1);
		float dz = depth / (n - 1);

		for (int k = 0; k < n; k++)
		{
			for (int i = 0; i < n; i++)
			{
				for (int j = 0; j < n; j++)
				{
					// Position instanced along a 3D grid - Translation Matrix
					g_InstancedData[k * n * n + i * n + j].World = XMFLOAT4X4
					(
						1.0f, 0.0f, 0.0f, 0.0f,
						0.0f, 1.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f, 0.0f,
						(x + j * dx), (y + i * dy), (z + k * dz), 1.0f
					);

					// Random color.
					g_InstancedData[k * n * n + i * n + j].Color.x = MathHelper::RandF(0.0f, 1.0f);
					g_InstancedData[k * n * n + i * n + j].Color.y = MathHelper::RandF(0.0f, 1.0f);
					g_InstancedData[k * n * n + i * n + j].Color.z = MathHelper::RandF(0.0f, 1.0f);
					g_InstancedData[k * n * n + i * n + j].Color.w = 1.0f;
				}
			}
		}


		D3D11_BUFFER_DESC vbd;
		vbd.ByteWidth = sizeof(InstancedData) * g_InstancedData.size();
		vbd.Usage = D3D11_USAGE_DYNAMIC;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		vbd.MiscFlags = 0;
		vbd.StructureByteStride = 0;

		HRESULT hr = pd3dDevice->CreateBuffer(&vbd, 0, &g_SkullInstancedBuffer);
	}

	void BuildLighting()
	{
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
	}

	void ComputeFrustumFromProjection(Frustum* pOut, XMMATRIX* pProjection)
	{
		// XMASSERT(pOut);
		// XMASSERT(pProjection);

		// Corners of the projection frustum in homogenous space.
		static XMVECTOR HomogenousPoints[6] =
		{
			{  1.0f,  0.0f, 1.0f, 1.0f },   // right (at far plane)
			{ -1.0f,  0.0f, 1.0f, 1.0f },   // left
			{  0.0f,  1.0f, 1.0f, 1.0f },   // top
			{  0.0f, -1.0f, 1.0f, 1.0f },   // bottom

			{ 0.0f, 0.0f, 0.0f, 1.0f },     // near
			{ 0.0f, 0.0f, 1.0f, 1.0f }      // far
		};

		XMVECTOR Determinant;
		XMMATRIX matInverse = XMMatrixInverse(&Determinant, *pProjection);

		// Compute the frustum corners in world space.
		XMVECTOR Points[6];

		for (INT i = 0; i < 6; i++)
		{
			// Transform point.
			Points[i] = XMVector4Transform(HomogenousPoints[i], matInverse);
		}

		pOut->Origin = XMFLOAT3(0.0f, 0.0f, 0.0f);
		pOut->Orientation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		// Compute the slopes.
		Points[0] = Points[0] * XMVectorReciprocal(XMVectorSplatZ(Points[0]));
		Points[1] = Points[1] * XMVectorReciprocal(XMVectorSplatZ(Points[1]));
		Points[2] = Points[2] * XMVectorReciprocal(XMVectorSplatZ(Points[2]));
		Points[3] = Points[3] * XMVectorReciprocal(XMVectorSplatZ(Points[3]));

		pOut->RightSlope = XMVectorGetX(Points[0]);
		pOut->LeftSlope = XMVectorGetX(Points[1]);
		pOut->TopSlope = XMVectorGetY(Points[2]);
		pOut->BottomSlope = XMVectorGetY(Points[3]);

		// Compute near and far.
		Points[4] = Points[4] * XMVectorReciprocal(XMVectorSplatW(Points[4]));
		Points[5] = Points[5] * XMVectorReciprocal(XMVectorSplatW(Points[5]));

		pOut->Near = XMVectorGetZ(Points[4]);
		pOut->Far = XMVectorGetZ(Points[5]);

		return;
	}

	void TransformFrustum(Frustum* pOut, const Frustum* pIn, FLOAT Scale, FXMVECTOR Rotation, FXMVECTOR Translation)
	{
		// XMASSERT(pOut);
		// XMASSERT(pIn);
		// XMASSERT(XMQuaternionIsUnit(Rotation));

		// Load the frustum.
		XMVECTOR Origin = XMLoadFloat3(&pIn->Origin);
		XMVECTOR Orientation = XMLoadFloat4(&pIn->Orientation);

		// XMASSERT(XMQuaternionIsUnit(Orientation));

		// Composite the frustum rotation and the transform rotation.
		Orientation = XMQuaternionMultiply(Orientation, Rotation);

		// Transform the origin.
		Origin = XMVector3Rotate(Origin * XMVectorReplicate(Scale), Rotation) + Translation;

		// Store the frustum.
		XMStoreFloat3(&pOut->Origin, Origin);
		XMStoreFloat4(&pOut->Orientation, Orientation);

		// Scale the near and far distances (the slopes remain the same).
		pOut->Near = pIn->Near * Scale;
		pOut->Far = pIn->Far * Scale;

		// Copy the slopes.
		pOut->RightSlope = pIn->RightSlope;
		pOut->LeftSlope = pIn->LeftSlope;
		pOut->TopSlope = pIn->TopSlope;
		pOut->BottomSlope = pIn->BottomSlope;

		return;
	}

	static const XMVECTOR g_UnitQuaternionEpsilon =
	{
		1.0e-4f, 1.0e-4f, 1.0e-4f, 1.0e-4f
	};
	static const XMVECTOR g_UnitVectorEpsilon =
	{
		1.0e-4f, 1.0e-4f, 1.0e-4f, 1.0e-4f
	};
	static const XMVECTOR g_UnitPlaneEpsilon =
	{
		1.0e-4f, 1.0e-4f, 1.0e-4f, 1.0e-4f
	};

	static inline BOOL XMQuaternionIsUnit(FXMVECTOR Q)
	{
		XMVECTOR Difference = XMVector4Length(Q) - XMVectorSplatOne();

		return XMVector4Less(XMVectorAbs(Difference), g_UnitQuaternionEpsilon);
	}

	static inline BOOL XMVector3AnyTrue(FXMVECTOR V)
	{
		XMVECTOR C;

		// Duplicate the fourth element from the first element.
		C = XMVectorSwizzle(V, 0, 1, 2, 0);

		return XMComparisonAnyTrue(XMVector4EqualIntR(C, XMVectorTrueInt()));
	}

	int IntersectOrientedBoxFrustum(const OrientedBox* pVolumeA, const Frustum* pVolumeB)
	{
		static const XMVECTORI32 SelectY =
		{
			XM_SELECT_0, XM_SELECT_1, XM_SELECT_0, XM_SELECT_0
		};
		static const XMVECTORI32 SelectZ =
		{
			XM_SELECT_0, XM_SELECT_0, XM_SELECT_1, XM_SELECT_0
		};

		XMVECTOR Zero = XMVectorZero();

		// Build the frustum planes.
		XMVECTOR Planes[6];
		Planes[0] = XMVectorSet(0.0f, 0.0f, -1.0f, pVolumeB->Near);
		Planes[1] = XMVectorSet(0.0f, 0.0f, 1.0f, -pVolumeB->Far);
		Planes[2] = XMVectorSet(1.0f, 0.0f, -pVolumeB->RightSlope, 0.0f);
		Planes[3] = XMVectorSet(-1.0f, 0.0f, pVolumeB->LeftSlope, 0.0f);
		Planes[4] = XMVectorSet(0.0f, 1.0f, -pVolumeB->TopSlope, 0.0f);
		Planes[5] = XMVectorSet(0.0f, -1.0f, pVolumeB->BottomSlope, 0.0f);

		// Load origin and orientation of the frustum.
		XMVECTOR Origin = XMLoadFloat3(&pVolumeB->Origin);
		XMVECTOR FrustumOrientation = XMLoadFloat4(&pVolumeB->Orientation);

		XMQuaternionIsUnit(FrustumOrientation);
	

		// Load the box.
		XMVECTOR Center = XMLoadFloat3(&pVolumeA->Center);
		XMVECTOR Extents = XMLoadFloat3(&pVolumeA->Extents);
		XMVECTOR BoxOrientation = XMLoadFloat4(&pVolumeA->Orientation);

		XMQuaternionIsUnit(BoxOrientation);

		// Transform the oriented box into the space of the frustum in order to 
		// minimize the number of transforms we have to do.
		Center = XMVector3InverseRotate(Center - Origin, FrustumOrientation);
		BoxOrientation = XMQuaternionMultiply(BoxOrientation, XMQuaternionConjugate(FrustumOrientation));

		// Set w of the center to one so we can dot4 with the plane.
		Center = XMVectorInsert(Center, XMVectorSplatOne(), 0, 0, 0, 0, 1);

		// Build the 3x3 rotation matrix that defines the box axes.
		XMMATRIX R = XMMatrixRotationQuaternion(BoxOrientation);

		// Check against each plane of the frustum.
		XMVECTOR Outside = XMVectorFalseInt();
		XMVECTOR InsideAll = XMVectorTrueInt();
		XMVECTOR CenterInsideAll = XMVectorTrueInt();

		for (INT i = 0; i < 6; i++)
		{
			// Compute the distance to the center of the box.
			XMVECTOR Dist = XMVector4Dot(Center, Planes[i]);

			// Project the axes of the box onto the normal of the plane.  Half the
			// length of the projection (sometime called the "radius") is equal to
			// h(u) * abs(n dot b(u))) + h(v) * abs(n dot b(v)) + h(w) * abs(n dot b(w))
			// where h(i) are extents of the box, n is the plane normal, and b(i) are the 
			// axes of the box.
			XMVECTOR Radius = XMVector3Dot(Planes[i], R.r[0]);
			Radius = XMVectorSelect(Radius, XMVector3Dot(Planes[i], R.r[1]), SelectY);
			Radius = XMVectorSelect(Radius, XMVector3Dot(Planes[i], R.r[2]), SelectZ);
			Radius = XMVector3Dot(Extents, XMVectorAbs(Radius));

			// Outside the plane?
			Outside = XMVectorOrInt(Outside, XMVectorGreater(Dist, Radius));

			// Fully inside the plane?
			InsideAll = XMVectorAndInt(InsideAll, XMVectorLessOrEqual(Dist, -Radius));

			// Check if the center is inside the plane.
			CenterInsideAll = XMVectorAndInt(CenterInsideAll, XMVectorLessOrEqual(Dist, Zero));
		}

		// If the box is outside any of the planes it is outside. 
		if (XMVector4EqualInt(Outside, XMVectorTrueInt()))
			return 0;

		// If the box is inside all planes it is fully inside.
		if (XMVector4EqualInt(InsideAll, XMVectorTrueInt()))
			return 2;

		// If the center of the box is inside all planes and the box intersects 
		// one or more planes then it must intersect.
		if (XMVector4EqualInt(CenterInsideAll, XMVectorTrueInt()))
			return 1;

		// Build the corners of the frustum.
		XMVECTOR RightTop = XMVectorSet(pVolumeB->RightSlope, pVolumeB->TopSlope, 1.0f, 0.0f);
		XMVECTOR RightBottom = XMVectorSet(pVolumeB->RightSlope, pVolumeB->BottomSlope, 1.0f, 0.0f);
		XMVECTOR LeftTop = XMVectorSet(pVolumeB->LeftSlope, pVolumeB->TopSlope, 1.0f, 0.0f);
		XMVECTOR LeftBottom = XMVectorSet(pVolumeB->LeftSlope, pVolumeB->BottomSlope, 1.0f, 0.0f);
		XMVECTOR Near = XMVectorReplicatePtr(&pVolumeB->Near);
		XMVECTOR Far = XMVectorReplicatePtr(&pVolumeB->Far);

		XMVECTOR Corners[8];
		Corners[0] = RightTop * Near;
		Corners[1] = RightBottom * Near;
		Corners[2] = LeftTop * Near;
		Corners[3] = LeftBottom * Near;
		Corners[4] = RightTop * Far;
		Corners[5] = RightBottom * Far;
		Corners[6] = LeftTop * Far;
		Corners[7] = LeftBottom * Far;

		// Test against box axes (3)
		{
			// Find the min/max values of the projection of the frustum onto each axis.
			XMVECTOR FrustumMin, FrustumMax;

			FrustumMin = XMVector3Dot(Corners[0], R.r[0]);
			FrustumMin = XMVectorSelect(FrustumMin, XMVector3Dot(Corners[0], R.r[1]), SelectY);
			FrustumMin = XMVectorSelect(FrustumMin, XMVector3Dot(Corners[0], R.r[2]), SelectZ);
			FrustumMax = FrustumMin;

			for (INT i = 1; i < 8; i++)
			{
				XMVECTOR Temp = XMVector3Dot(Corners[i], R.r[0]);
				Temp = XMVectorSelect(Temp, XMVector3Dot(Corners[i], R.r[1]), SelectY);
				Temp = XMVectorSelect(Temp, XMVector3Dot(Corners[i], R.r[2]), SelectZ);

				FrustumMin = XMVectorMin(FrustumMin, Temp);
				FrustumMax = XMVectorMax(FrustumMax, Temp);
			}

			// Project the center of the box onto the axes.
			XMVECTOR BoxDist = XMVector3Dot(Center, R.r[0]);
			BoxDist = XMVectorSelect(BoxDist, XMVector3Dot(Center, R.r[1]), SelectY);
			BoxDist = XMVectorSelect(BoxDist, XMVector3Dot(Center, R.r[2]), SelectZ);

			// The projection of the box onto the axis is just its Center and Extents.
			// if (min > box_max || max < box_min) reject;
			XMVECTOR Result = XMVectorOrInt(XMVectorGreater(FrustumMin, BoxDist + Extents),
				XMVectorLess(FrustumMax, BoxDist - Extents));

			if (XMVector3AnyTrue(Result))
				return 0;
		}

		// Test against edge/edge axes (3*6).
		XMVECTOR FrustumEdgeAxis[6];

		FrustumEdgeAxis[0] = RightTop;
		FrustumEdgeAxis[1] = RightBottom;
		FrustumEdgeAxis[2] = LeftTop;
		FrustumEdgeAxis[3] = LeftBottom;
		FrustumEdgeAxis[4] = RightTop - LeftTop;
		FrustumEdgeAxis[5] = LeftBottom - LeftTop;

		for (INT i = 0; i < 3; i++)
		{
			for (INT j = 0; j < 6; j++)
			{
				// Compute the axis we are going to test.
				XMVECTOR Axis = XMVector3Cross(R.r[i], FrustumEdgeAxis[j]);

				// Find the min/max values of the projection of the frustum onto the axis.
				XMVECTOR FrustumMin, FrustumMax;

				FrustumMin = FrustumMax = XMVector3Dot(Axis, Corners[0]);

				for (INT k = 1; k < 8; k++)
				{
					XMVECTOR Temp = XMVector3Dot(Axis, Corners[k]);
					FrustumMin = XMVectorMin(FrustumMin, Temp);
					FrustumMax = XMVectorMax(FrustumMax, Temp);
				}

				// Project the center of the box onto the axis.
				XMVECTOR Dist = XMVector3Dot(Center, Axis);

				// Project the axes of the box onto the axis to find the "radius" of the box.
				XMVECTOR Radius = XMVector3Dot(Axis, R.r[0]);
				Radius = XMVectorSelect(Radius, XMVector3Dot(Axis, R.r[1]), SelectY);
				Radius = XMVectorSelect(Radius, XMVector3Dot(Axis, R.r[2]), SelectZ);
				Radius = XMVector3Dot(Extents, XMVectorAbs(Radius));

				// if (center > max + radius || center < min - radius) reject;
				Outside = XMVectorOrInt(Outside, XMVectorGreater(Dist, FrustumMax + Radius));
				Outside = XMVectorOrInt(Outside, XMVectorLess(Dist, FrustumMin - Radius));
			}
		}

		if (XMVector4EqualInt(Outside, XMVectorTrueInt()))
			return 0;

		// If we did not find a separating plane then the box must intersect the frustum.
		return 1;
	}

	int IntersectAxisAlignedBoxFrustum(const AxisAlignedBox* pVolumeA, const Frustum* pVolumeB)
	{
		// Make the axis aligned box oriented and do an OBB vs frustum test.
		OrientedBox BoxA;

		BoxA.Center = pVolumeA->Center;
		BoxA.Extents = pVolumeA->Extents;
		BoxA.Orientation.x = 0.0f;
		BoxA.Orientation.y = 0.0f;
		BoxA.Orientation.z = 0.0f;
		BoxA.Orientation.w = 1.0f;

		return IntersectOrientedBoxFrustum(&BoxA, pVolumeB);
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
		// Lighting
		//
		BuildLighting();
		Effects::InitAll(pd3dDevice);
		InputLayouts::InitAll(pd3dDevice);


		//
		// Skull
		//
		g_SkullMaterial.Ambient = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
		g_SkullMaterial.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		g_SkullMaterial.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);

		XMMATRIX skullScale = XMMatrixScaling(5.0f, 5.0f, 5.0f);
		XMMATRIX skullOffset = XMMatrixTranslation(0.0f, 1.0f, 0.0f);
		XMStoreFloat4x4(&g_SkullWorld, XMMatrixMultiply(skullScale, skullOffset));

		BuildSkullGeometryBuffers(pd3dDevice);
		BuildInstancedBuffer(pd3dDevice);



		//
		// Create Camera
		//
		static const XMVECTORF32 s_Eye = { 0.0f, 2.0f, -15.0f, 0.0f };
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


		pd3dImmediateContext->IASetInputLayout(InputLayouts::InstancedBasic32);
		pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		UINT stride[2] = { sizeof(Vertex::Basic32), sizeof(InstancedData) };
		UINT offset[2] = { 0, 0 };

		ID3D11Buffer* vbs[2] = { g_SkullVetexBuffer, g_SkullInstancedBuffer };

		XMMATRIX view = g_Camera.GetViewMatrix();
		XMMATRIX proj = g_Camera.GetProjMatrix();
		XMMATRIX viewProj = view * proj;

		// Set per frame constants.
		XMFLOAT3 eyePos;
		XMStoreFloat3(&eyePos, g_Camera.GetEyePt());
		Effects::InstancedBasicFX->SetEyePosW(eyePos);
		Effects::InstancedBasicFX->SetDirLights(g_DirectionLight);

		ID3DX11EffectTechnique* activeTech = Effects::InstancedBasicFX->Light3Tech;

		D3DX11_TECHNIQUE_DESC techDesc;
		activeTech->GetDesc(&techDesc);

		for (UINT p = 0; p < techDesc.Passes; p++)
		{
		
			pd3dImmediateContext->IASetVertexBuffers(0, 2, vbs, stride, offset);
			pd3dImmediateContext->IASetIndexBuffer(g_SkullIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

			XMMATRIX world = XMLoadFloat4x4(&g_SkullWorld);
			XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);

			Effects::InstancedBasicFX->SetWorld(world);
			Effects::InstancedBasicFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::InstancedBasicFX->SetViewProj(viewProj);
			Effects::InstancedBasicFX->SetMaterial(g_SkullMaterial);


			activeTech->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
			pd3dImmediateContext->DrawIndexedInstanced(g_SkullIndexCount, g_VisibleObjectCount, 0, 0, 0);
		}
	}





	//--------------------------------------------------------------------------------------
	// Handle updates to the scene.
	//--------------------------------------------------------------------------------------
	void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
	{
		g_Camera.FrameMove(fElapsedTime);
		auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
		//
		// Performance Frustum Culling
		//
		if (GetAsyncKeyState('1') & 0x8000)
			bUseFrustumCulling = true;

		if (GetAsyncKeyState('2') & 0x8000)
			bUseFrustumCulling = false;

		g_VisibleObjectCount = 0;
		if (bUseFrustumCulling)
		{
			XMVECTOR detViewMatrix = XMMatrixDeterminant(g_Camera.GetViewMatrix());
			XMMATRIX invViewMatrix = XMMatrixInverse(&detViewMatrix, g_Camera.GetViewMatrix());
		
			D3D11_MAPPED_SUBRESOURCE mappedData;
			pd3dImmediateContext->Map(g_SkullInstancedBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);

			InstancedData* dataView = reinterpret_cast<InstancedData*>(mappedData.pData);

			for (UINT i = 0; i < g_InstancedData.size(); i++)
			{
				XMMATRIX World = XMLoadFloat4x4(&g_InstancedData[i].World);
				XMVECTOR detWorld = XMMatrixDeterminant(World);
				XMMATRIX invWorld = XMMatrixInverse(&detWorld, World);

				// View space to the object's local space.
				XMMATRIX toLocal = invViewMatrix * invWorld;

				// Decompose the into its individual parts
				XMVECTOR scale;
				XMVECTOR rotQuat;
				XMVECTOR translation;
				XMMatrixDecompose(&scale, &rotQuat, &translation, toLocal);

				Frustum localSpaceFrustum;
				TransformFrustum(&localSpaceFrustum, &g_CameraFrustum, XMVectorGetX(scale), rotQuat, translation);

				// Check intersection
				if (IntersectAxisAlignedBoxFrustum(&g_SkullBox, &localSpaceFrustum) != 0)
					dataView[g_VisibleObjectCount++] = g_InstancedData[i];
			}

			pd3dImmediateContext->Unmap(g_SkullInstancedBuffer, 0);
		}
		else
		{
			
			D3D11_MAPPED_SUBRESOURCE mappedData;
			pd3dImmediateContext->Map(g_SkullInstancedBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
			InstancedData* dataView = reinterpret_cast<InstancedData*>(mappedData.pData);
			for (UINT i = 0; i < g_InstancedData.size(); ++i)
			{
				dataView[g_VisibleObjectCount++] = g_InstancedData[i];
			}
			pd3dImmediateContext->Unmap(g_SkullInstancedBuffer, 0);
			
		}

		std::wostringstream outs;
		outs.precision(6);
		outs << L"Instancing and Culling Demo" <<
			L"    " << g_VisibleObjectCount <<
			L" objects visible out of " << g_InstancedData.size();
		
		HWND hWnd = DXUTGetHWND();  // Get the handle to the window
		SetWindowText(hWnd, outs.str().c_str());

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
		g_Camera.SetButtonMasks(MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON);

		// Get Frustum of Camera
		XMMATRIX projView = g_Camera.GetProjMatrix();
		ComputeFrustumFromProjection(&g_CameraFrustum, &projView);

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
	
	

		DXUTCreateWindow(L"Instancing and Frustum Culling Demo");
		// SetTextCharacterExtra(mMainWndCaption.c_str());

		DXUTCreateDevice(D3D_FEATURE_LEVEL_10_0, true, 800, 600);
		DXUTMainLoop();


		return DXUTGetExitCode();
	}