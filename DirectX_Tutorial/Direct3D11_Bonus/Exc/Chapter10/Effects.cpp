#include "Effects.h"

#pragma region Effect
Effect::Effect(ID3D11Device* device, LPCWCHAR filename)
	: mFX(0)
{
	/**/
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	dwShaderFlags |= D3DCOMPILE_DEBUG;
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;

#endif


	// Disable optimizations to further improve shader debugging
	WCHAR str[MAX_PATH];
	HRESULT hr = (DXUTFindDXSDKMediaFileCch(str, MAX_PATH, filename));

	hr = (D3DX11CompileEffectFromFile(str, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, dwShaderFlags, 0, device, &mFX, nullptr));

}

Effect::~Effect()
{
	SAFE_RELEASE(mFX);
}
#pragma endregion

#pragma region BasicEffect
BasicEffect::BasicEffect(ID3D11Device* device, LPCWCHAR filename)
	: Effect(device, filename)
{
	if (mFX != nullptr)
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
}

BasicEffect::~BasicEffect()
{
	SAFE_RELEASE(mFX);
}
#pragma endregion

#pragma region Effects

BasicEffect* Effects::BasicFX = 0;

void Effects::InitAll(ID3D11Device* device)
{
	BasicFX = new BasicEffect(device, L"D:/Work/Direct3D11/Exc/Chapter10/FX/Basic.fx");
}

void Effects::DestroyAll()
{
	delete BasicFX;
	BasicFX = 0;
}
#pragma endregion