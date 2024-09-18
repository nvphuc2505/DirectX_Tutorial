#include "Effects.h"



Effect::Effect(ID3D11Device* device, LPCWCHAR filename)
{
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#ifdef _DEBUG
	dwShaderFlags |= D3DCOMPILE_DEBUG;
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG

	WCHAR str[MAX_PATH];
	HRESULT hr = DXUTFindDXSDKMediaFileCch(str, MAX_PATH, filename);
	hr = D3DX11CompileEffectFromFile(str, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, dwShaderFlags, 0, device, &mFX, nullptr);
}


Effect::~Effect()
{
	SAFE_RELEASE(mFX);
}





BasicEffect::BasicEffect(ID3D11Device* device, LPCWCHAR filename) : Effect(device, filename)
{
	Light1Tech = mFX->GetTechniqueByName("Light1");
	Light2Tech = mFX->GetTechniqueByName("Light2");
	Light3Tech = mFX->GetTechniqueByName("Light3");

	Light0TexTech = mFX->GetTechniqueByName("Light0Tex");
	Light1TexTech = mFX->GetTechniqueByName("Light1Tex");
	Light2TexTech = mFX->GetTechniqueByName("Light2Tex");
	Light3TexTech = mFX->GetTechniqueByName("Light3Tex");

	Light0TexAlphaClipTech = mFX->GetTechniqueByName("Light0TexAlphaClip");
	Light1TexAlphaClipTech = mFX->GetTechniqueByName("Light1TexAlphaClip");
	Light2TexAlphaClipTech = mFX->GetTechniqueByName("Light2TexAlphaClip");
	Light3TexAlphaClipTech = mFX->GetTechniqueByName("Light3TexAlphaClip");

	Light1FogTech = mFX->GetTechniqueByName("Light1Fog");
	Light2FogTech = mFX->GetTechniqueByName("Light2Fog");
	Light3FogTech = mFX->GetTechniqueByName("Light3Fog");

	Light0TexFogTech = mFX->GetTechniqueByName("Light0TexFog");
	Light1TexFogTech = mFX->GetTechniqueByName("Light1TexFog");
	Light2TexFogTech = mFX->GetTechniqueByName("Light2TexFog");
	Light3TexFogTech = mFX->GetTechniqueByName("Light3TexFog");

	Light0TexAlphaClipFogTech = mFX->GetTechniqueByName("Light0TexAlphaClipFog");
	Light1TexAlphaClipFogTech = mFX->GetTechniqueByName("Light1TexAlphaClipFog");
	Light2TexAlphaClipFogTech = mFX->GetTechniqueByName("Light2TexAlphaClipFog");
	Light3TexAlphaClipFogTech = mFX->GetTechniqueByName("Light3TexAlphaClipFog");

	Light1ReflectTech = mFX->GetTechniqueByName("Light1Reflect");
	Light2ReflectTech = mFX->GetTechniqueByName("Light2Reflect");
	Light3ReflectTech = mFX->GetTechniqueByName("Light3Reflect");

	Light0TexReflectTech = mFX->GetTechniqueByName("Light0TexReflect");
	Light1TexReflectTech = mFX->GetTechniqueByName("Light1TexReflect");
	Light2TexReflectTech = mFX->GetTechniqueByName("Light2TexReflect");
	Light3TexReflectTech = mFX->GetTechniqueByName("Light3TexReflect");

	Light0TexAlphaClipReflectTech = mFX->GetTechniqueByName("Light0TexAlphaClipReflect");
	Light1TexAlphaClipReflectTech = mFX->GetTechniqueByName("Light1TexAlphaClipReflect");
	Light2TexAlphaClipReflectTech = mFX->GetTechniqueByName("Light2TexAlphaClipReflect");
	Light3TexAlphaClipReflectTech = mFX->GetTechniqueByName("Light3TexAlphaClipReflect");

	Light1FogReflectTech = mFX->GetTechniqueByName("Light1FogReflect");
	Light2FogReflectTech = mFX->GetTechniqueByName("Light2FogReflect");
	Light3FogReflectTech = mFX->GetTechniqueByName("Light3FogReflect");

	Light0TexFogReflectTech = mFX->GetTechniqueByName("Light0TexFogReflect");
	Light1TexFogReflectTech = mFX->GetTechniqueByName("Light1TexFogReflect");
	Light2TexFogReflectTech = mFX->GetTechniqueByName("Light2TexFogReflect");
	Light3TexFogReflectTech = mFX->GetTechniqueByName("Light3TexFogReflect");

	Light0TexAlphaClipFogReflectTech = mFX->GetTechniqueByName("Light0TexAlphaClipFogReflect");
	Light1TexAlphaClipFogReflectTech = mFX->GetTechniqueByName("Light1TexAlphaClipFogReflect");
	Light2TexAlphaClipFogReflectTech = mFX->GetTechniqueByName("Light2TexAlphaClipFogReflect");
	Light3TexAlphaClipFogReflectTech = mFX->GetTechniqueByName("Light3TexAlphaClipFogReflect");

	WorldViewProj = mFX->GetVariableByName("gWorldViewProj")->AsMatrix();
	World = mFX->GetVariableByName("gWorld")->AsMatrix();
	WorldInvTranspose = mFX->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	TexTransform = mFX->GetVariableByName("gTexTransform")->AsMatrix();
	EyePosW = mFX->GetVariableByName("gEyePosW")->AsVector();
	FogColor = mFX->GetVariableByName("gFogColor")->AsVector();
	FogStart = mFX->GetVariableByName("gFogStart")->AsScalar();
	FogRange = mFX->GetVariableByName("gFogRange")->AsScalar();
	DirLights = mFX->GetVariableByName("gDirLights");
	Mat = mFX->GetVariableByName("gMaterial");
	DiffuseMap = mFX->GetVariableByName("gDiffuseMap")->AsShaderResource();
	CubeMap = mFX->GetVariableByName("gCubeMap")->AsShaderResource();
}

BasicEffect::~BasicEffect()
{
	
}





NormalMapEffect::NormalMapEffect(ID3D11Device* device, LPCWCHAR filename)
	: Effect(device, filename)
{
	Light1Tech = mFX->GetTechniqueByName("Light1");
	Light2Tech = mFX->GetTechniqueByName("Light2");
	Light3Tech = mFX->GetTechniqueByName("Light3");

	Light0TexTech = mFX->GetTechniqueByName("Light0Tex");
	Light1TexTech = mFX->GetTechniqueByName("Light1Tex");
	Light2TexTech = mFX->GetTechniqueByName("Light2Tex");
	Light3TexTech = mFX->GetTechniqueByName("Light3Tex");

	Light0TexAlphaClipTech = mFX->GetTechniqueByName("Light0TexAlphaClip");
	Light1TexAlphaClipTech = mFX->GetTechniqueByName("Light1TexAlphaClip");
	Light2TexAlphaClipTech = mFX->GetTechniqueByName("Light2TexAlphaClip");
	Light3TexAlphaClipTech = mFX->GetTechniqueByName("Light3TexAlphaClip");

	Light1FogTech = mFX->GetTechniqueByName("Light1Fog");
	Light2FogTech = mFX->GetTechniqueByName("Light2Fog");
	Light3FogTech = mFX->GetTechniqueByName("Light3Fog");

	Light0TexFogTech = mFX->GetTechniqueByName("Light0TexFog");
	Light1TexFogTech = mFX->GetTechniqueByName("Light1TexFog");
	Light2TexFogTech = mFX->GetTechniqueByName("Light2TexFog");
	Light3TexFogTech = mFX->GetTechniqueByName("Light3TexFog");

	Light0TexAlphaClipFogTech = mFX->GetTechniqueByName("Light0TexAlphaClipFog");
	Light1TexAlphaClipFogTech = mFX->GetTechniqueByName("Light1TexAlphaClipFog");
	Light2TexAlphaClipFogTech = mFX->GetTechniqueByName("Light2TexAlphaClipFog");
	Light3TexAlphaClipFogTech = mFX->GetTechniqueByName("Light3TexAlphaClipFog");

	Light1ReflectTech = mFX->GetTechniqueByName("Light1Reflect");
	Light2ReflectTech = mFX->GetTechniqueByName("Light2Reflect");
	Light3ReflectTech = mFX->GetTechniqueByName("Light3Reflect");

	Light0TexReflectTech = mFX->GetTechniqueByName("Light0TexReflect");
	Light1TexReflectTech = mFX->GetTechniqueByName("Light1TexReflect");
	Light2TexReflectTech = mFX->GetTechniqueByName("Light2TexReflect");
	Light3TexReflectTech = mFX->GetTechniqueByName("Light3TexReflect");

	Light0TexAlphaClipReflectTech = mFX->GetTechniqueByName("Light0TexAlphaClipReflect");
	Light1TexAlphaClipReflectTech = mFX->GetTechniqueByName("Light1TexAlphaClipReflect");
	Light2TexAlphaClipReflectTech = mFX->GetTechniqueByName("Light2TexAlphaClipReflect");
	Light3TexAlphaClipReflectTech = mFX->GetTechniqueByName("Light3TexAlphaClipReflect");

	Light1FogReflectTech = mFX->GetTechniqueByName("Light1FogReflect");
	Light2FogReflectTech = mFX->GetTechniqueByName("Light2FogReflect");
	Light3FogReflectTech = mFX->GetTechniqueByName("Light3FogReflect");

	Light0TexFogReflectTech = mFX->GetTechniqueByName("Light0TexFogReflect");
	Light1TexFogReflectTech = mFX->GetTechniqueByName("Light1TexFogReflect");
	Light2TexFogReflectTech = mFX->GetTechniqueByName("Light2TexFogReflect");
	Light3TexFogReflectTech = mFX->GetTechniqueByName("Light3TexFogReflect");

	Light0TexAlphaClipFogReflectTech = mFX->GetTechniqueByName("Light0TexAlphaClipFogReflect");
	Light1TexAlphaClipFogReflectTech = mFX->GetTechniqueByName("Light1TexAlphaClipFogReflect");
	Light2TexAlphaClipFogReflectTech = mFX->GetTechniqueByName("Light2TexAlphaClipFogReflect");
	Light3TexAlphaClipFogReflectTech = mFX->GetTechniqueByName("Light3TexAlphaClipFogReflect");

	Light1SkinnedTech = mFX->GetTechniqueByName("Light1Skinned");
	Light2SkinnedTech = mFX->GetTechniqueByName("Light2Skinned");
	Light3SkinnedTech = mFX->GetTechniqueByName("Light3Skinned");

	Light0TexSkinnedTech = mFX->GetTechniqueByName("Light0TexSkinned");
	Light1TexSkinnedTech = mFX->GetTechniqueByName("Light1TexSkinned");
	Light2TexSkinnedTech = mFX->GetTechniqueByName("Light2TexSkinned");
	Light3TexSkinnedTech = mFX->GetTechniqueByName("Light3TexSkinned");

	Light0TexAlphaClipSkinnedTech = mFX->GetTechniqueByName("Light0TexAlphaClipSkinned");
	Light1TexAlphaClipSkinnedTech = mFX->GetTechniqueByName("Light1TexAlphaClipSkinned");
	Light2TexAlphaClipSkinnedTech = mFX->GetTechniqueByName("Light2TexAlphaClipSkinned");
	Light3TexAlphaClipSkinnedTech = mFX->GetTechniqueByName("Light3TexAlphaClipSkinned");

	Light1FogSkinnedTech = mFX->GetTechniqueByName("Light1FogSkinned");
	Light2FogSkinnedTech = mFX->GetTechniqueByName("Light2FogSkinned");
	Light3FogSkinnedTech = mFX->GetTechniqueByName("Light3FogSkinned");

	Light0TexFogSkinnedTech = mFX->GetTechniqueByName("Light0TexFogSkinned");
	Light1TexFogSkinnedTech = mFX->GetTechniqueByName("Light1TexFogSkinned");
	Light2TexFogSkinnedTech = mFX->GetTechniqueByName("Light2TexFogSkinned");
	Light3TexFogSkinnedTech = mFX->GetTechniqueByName("Light3TexFogSkinned");

	Light0TexAlphaClipFogSkinnedTech = mFX->GetTechniqueByName("Light0TexAlphaClipFogSkinned");
	Light1TexAlphaClipFogSkinnedTech = mFX->GetTechniqueByName("Light1TexAlphaClipFogSkinned");
	Light2TexAlphaClipFogSkinnedTech = mFX->GetTechniqueByName("Light2TexAlphaClipFogSkinned");
	Light3TexAlphaClipFogSkinnedTech = mFX->GetTechniqueByName("Light3TexAlphaClipFogSkinned");

	Light1ReflectSkinnedTech = mFX->GetTechniqueByName("Light1ReflectSkinned");
	Light2ReflectSkinnedTech = mFX->GetTechniqueByName("Light2ReflectSkinned");
	Light3ReflectSkinnedTech = mFX->GetTechniqueByName("Light3ReflectSkinned");

	Light0TexReflectSkinnedTech = mFX->GetTechniqueByName("Light0TexReflectSkinned");
	Light1TexReflectSkinnedTech = mFX->GetTechniqueByName("Light1TexReflectSkinned");
	Light2TexReflectSkinnedTech = mFX->GetTechniqueByName("Light2TexReflectSkinned");
	Light3TexReflectSkinnedTech = mFX->GetTechniqueByName("Light3TexReflectSkinned");

	Light0TexAlphaClipReflectSkinnedTech = mFX->GetTechniqueByName("Light0TexAlphaClipReflectSkinned");
	Light1TexAlphaClipReflectSkinnedTech = mFX->GetTechniqueByName("Light1TexAlphaClipReflectSkinned");
	Light2TexAlphaClipReflectSkinnedTech = mFX->GetTechniqueByName("Light2TexAlphaClipReflectSkinned");
	Light3TexAlphaClipReflectSkinnedTech = mFX->GetTechniqueByName("Light3TexAlphaClipReflectSkinned");

	Light1FogReflectSkinnedTech = mFX->GetTechniqueByName("Light1FogReflectSkinned");
	Light2FogReflectSkinnedTech = mFX->GetTechniqueByName("Light2FogReflectSkinned");
	Light3FogReflectSkinnedTech = mFX->GetTechniqueByName("Light3FogReflectSkinned");

	Light0TexFogReflectSkinnedTech = mFX->GetTechniqueByName("Light0TexFogReflectSkinned");
	Light1TexFogReflectSkinnedTech = mFX->GetTechniqueByName("Light1TexFogReflectSkinned");
	Light2TexFogReflectSkinnedTech = mFX->GetTechniqueByName("Light2TexFogReflectSkinned");
	Light3TexFogReflectSkinnedTech = mFX->GetTechniqueByName("Light3TexFogReflectSkinned");

	Light0TexAlphaClipFogReflectSkinnedTech = mFX->GetTechniqueByName("Light0TexAlphaClipFogReflectSkinned");
	Light1TexAlphaClipFogReflectSkinnedTech = mFX->GetTechniqueByName("Light1TexAlphaClipFogReflectSkinned");
	Light2TexAlphaClipFogReflectSkinnedTech = mFX->GetTechniqueByName("Light2TexAlphaClipFogReflectSkinned");
	Light3TexAlphaClipFogReflectSkinnedTech = mFX->GetTechniqueByName("Light3TexAlphaClipFogReflectSkinned");

	WorldViewProj = mFX->GetVariableByName("gWorldViewProj")->AsMatrix();
	WorldViewProjTex = mFX->GetVariableByName("gWorldViewProjTex")->AsMatrix();
	World = mFX->GetVariableByName("gWorld")->AsMatrix();
	WorldInvTranspose = mFX->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	BoneTransforms = mFX->GetVariableByName("gBoneTransforms")->AsMatrix();
	ShadowTransform = mFX->GetVariableByName("gShadowTransform")->AsMatrix();
	TexTransform = mFX->GetVariableByName("gTexTransform")->AsMatrix();
	EyePosW = mFX->GetVariableByName("gEyePosW")->AsVector();
	FogColor = mFX->GetVariableByName("gFogColor")->AsVector();
	FogStart = mFX->GetVariableByName("gFogStart")->AsScalar();
	FogRange = mFX->GetVariableByName("gFogRange")->AsScalar();
	DirLights = mFX->GetVariableByName("gDirLights");
	Mat = mFX->GetVariableByName("gMaterial");
	DiffuseMap = mFX->GetVariableByName("gDiffuseMap")->AsShaderResource();
	CubeMap = mFX->GetVariableByName("gCubeMap")->AsShaderResource();
	NormalMap = mFX->GetVariableByName("gNormalMap")->AsShaderResource();
	ShadowMap = mFX->GetVariableByName("gShadowMap")->AsShaderResource();
	SsaoMap = mFX->GetVariableByName("gSsaoMap")->AsShaderResource();
}

NormalMapEffect::~NormalMapEffect()
{
}





SkyEffect::SkyEffect(ID3D11Device* pd3dDevice, LPCWCHAR filename) : Effect(pd3dDevice, filename)
{
	SkyTech       = mFX->GetTechniqueByName("SkyTech");
	WorldViewProj = mFX->GetVariableByName("gWorldViewProj")->AsMatrix();
	CubeMap       = mFX->GetVariableByName("gCubeMap")->AsShaderResource();
}

SkyEffect::~SkyEffect()
{
}


BasicEffect* Effects::BasicFX = 0;
NormalMapEffect* Effects::NormalMapFX = 0;
SkyEffect* Effects::SkyFX = 0;

void Effects::InitAll(ID3D11Device* device)
{
	BasicFX = new BasicEffect(device, L"D:/Work/DirectX/Chapter25/Animation/Shader/Basic.fx");
	NormalMapFX = new NormalMapEffect(device, L"D:/Work/DirectX/Chapter25/Animation/Shader/NormalMap.fx");
	SkyFX = new SkyEffect(device, L"D:/Work/DirectX/Chapter17/CubeMap/Shader/SkyCubeMap.fx");
}

void Effects::DestroyAll()
{
	
}