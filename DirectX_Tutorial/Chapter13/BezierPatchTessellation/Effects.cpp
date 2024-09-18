#include "Effects.h"

#define BASIC_SHADER L"D:/Work/DirectX/Chapter13/BasicTessellation/Shader/Basic.fx"
#define TREE_SPIRE_SHADER L"D:/Work/DirectX/Chapter11_GeometryShader/Shader/TreeSprie.FX"
#define TESSELLATION_SHADER L"D:/Work/DirectX/Chapter13/BasicTessellation/Shader/Tessellation.fx"

//
// Effect
//
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





//
// Basic Effect
//
InstancedBasicEffect::InstancedBasicEffect(ID3D11Device* device, LPCWCHAR filename) : Effect(device, filename)
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
}

InstancedBasicEffect::~InstancedBasicEffect()
{

}





//
// Tree Sprite Effect
//
TreeSpriteEffect::TreeSpriteEffect(ID3D11Device* device, LPCWCHAR filename) : Effect(device, filename)
{
	Light3Tech = mFX->GetTechniqueByName("Light3");
	Light3TexAlphaClipTech = mFX->GetTechniqueByName("Light3TexAlphaClip");
	Light3TexAlphaClipFogTech = mFX->GetTechniqueByName("Light3TexAlphaClipFog");

	ViewProj = mFX->GetVariableByName("gViewProj")->AsMatrix();
	EyePosW = mFX->GetVariableByName("gEyePosW")->AsVector();
	FogColor = mFX->GetVariableByName("gFogColor")->AsVector();
	FogStart = mFX->GetVariableByName("gFogStart")->AsScalar();
	FogRange = mFX->GetVariableByName("gFogRange")->AsScalar();
	DirLights = mFX->GetVariableByName("gDirLights");
	Mat = mFX->GetVariableByName("gMaterial");
	TreeTextureMapArray = mFX->GetVariableByName("gTreeMapArray")->AsShaderResource();
}

TreeSpriteEffect::~TreeSpriteEffect()
{
	
}




//
//
//
TessellationEffect::TessellationEffect(ID3D11Device* device, LPCWCHAR filename) : Effect(device, filename)
{
	TessTech = mFX->GetTechniqueByName("Tess");

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
}

TessellationEffect::~TessellationEffect()
{
}





//
//
//
InstancedBasicEffect* Effects::BasicFX = 0;
TreeSpriteEffect* Effects::TreeSpriteFX = 0;
TessellationEffect* Effects::TessellationFX = 0;

void Effects::InitAll(ID3D11Device* device)
{
	BasicFX = new InstancedBasicEffect(device, L"D:/Work/DirectX/Chapter13/BezierPatchTessellation/Shader/Basic.fx");
	// TreeSpriteFX = new TreeSpriteEffect(device, TREE_SPIRE_SHADER);
	TessellationFX = new TessellationEffect(device, L"D:/Work/DirectX/Chapter13/BezierPatchTessellation/Shader/Tessellation.fx");
}