#include "Sky.h"



Sky::Sky(ID3D11Device* pd3dDevice, LPCWCHAR cubemapFilename, float skySphereRadius)
{
	HRESULT hr = (DXUTCreateShaderResourceViewFromFile(pd3dDevice, cubemapFilename, &m_SkyMapSRV));

	GeometryGenerator::MeshData sphere;
	GeometryGenerator geoGen;
	geoGen.CreateSphere(skySphereRadius, 30, 30, sphere);



	std::vector<XMFLOAT3> vertices(sphere.Vertices.size());
	for (UINT i = 0; i < sphere.Vertices.size(); ++i)
	{
		vertices[i] = sphere.Vertices[i].Position;
	}

	D3D11_BUFFER_DESC vbd = {};
	vbd.ByteWidth = sizeof(XMFLOAT3) * vertices.size();
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA vInitData;
	vInitData.pSysMem = &vertices[0];
	hr = pd3dDevice->CreateBuffer(&vbd, &vInitData, &m_SkyVertexBuffer);



	std::vector <USHORT> indices16;
	indices16.assign(sphere.Indices.begin(), sphere.Indices.end());
	m_SkyIndexCount = sphere.Indices.size();

	D3D11_BUFFER_DESC ibd = {};
	ibd.ByteWidth = sizeof(USHORT) * m_SkyIndexCount;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iInitData = {};
	iInitData.pSysMem = &indices16[0];
	hr = pd3dDevice->CreateBuffer(&ibd, &iInitData, &m_SkyIndexBuffer);
}

Sky::~Sky()
{
	SAFE_RELEASE(m_SkyVertexBuffer);
	SAFE_RELEASE(m_SkyIndexBuffer);
	SAFE_RELEASE(m_SkyMapSRV);

}

ID3D11ShaderResourceView* Sky::GetSkyCubeMap()
{
	return m_SkyMapSRV;
}

void Sky::Draw(ID3D11DeviceContext* pd3dImmediateContext, CModelViewerCamera camera)
{
	XMFLOAT3 eyePos;
	XMStoreFloat3(&eyePos, camera.GetEyePt());

	XMMATRIX T = XMMatrixTranslation(eyePos.x, eyePos.y, eyePos.z);
	XMMATRIX V = camera.GetViewMatrix();
	XMMATRIX P = camera.GetProjMatrix();
	XMMATRIX WorldViewProj = T * V * P;

	Effects::SkyFX->SetWorldViewProj(WorldViewProj);
	Effects::SkyFX->SetCubeMap(m_SkyMapSRV);

	UINT stride = sizeof(XMFLOAT3);
	UINT offset = 0;
	pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_SkyVertexBuffer, &stride, &offset);
	pd3dImmediateContext->IASetIndexBuffer(m_SkyIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	pd3dImmediateContext->IASetInputLayout(InputLayouts::Pos);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3DX11_TECHNIQUE_DESC techDesc;
	Effects::SkyFX->SkyTech->GetDesc(&techDesc);

	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		ID3DX11EffectPass* pass = Effects::SkyFX->SkyTech->GetPassByIndex(p);

		pass->Apply(0, pd3dImmediateContext);

		pd3dImmediateContext->DrawIndexed(m_SkyIndexCount, 0, 0);
	}
}
