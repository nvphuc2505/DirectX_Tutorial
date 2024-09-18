#include "MyEffects.h"

#pragma region Effect
Effect::Effect(ID3D11Device* device, const std::wstring& filename)
	: mFX(0)
{
	/**/
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	dwShaderFlags |= D3DCOMPILE_DEBUG;
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;

#endif

#if D3D_COMPILER_VERSION >= 46
	// Disable optimizations to further improve shader debugging
	WCHAR str[MAX_PATH];
	HRESULT hr = (DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"D:/Work/Direct3D11/Exc/Chapter08/FX/Basic.fx"));

	hr = (D3DX11CompileEffectFromFile(str, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, dwShaderFlags, 0, device, &mFX, nullptr));
#else
	ID3DBlob* pEffectBuffer = nullptr;
	V_RETURN(DXUTCompileFromFile(L"D:/Work/Direct3D11/Exc/Chapter08/FX/Basic.fx", nullptr, "none", "fx_5_0", dwShaderFlags, 0, &pEffectBuffer));
	hr = D3DX11CreateEffectFromMemory(pEffectBuffer->GetBufferPointer(), pEffectBuffer->GetBufferSize(), 0, device, &mFX);
	SAFE_RELEASE(pEffectBuffer);
	if (FAILED(hr))
		return hr;
#endif
}

Effect::~Effect()
{
	SAFE_RELEASE(mFX);
}
#pragma endregion

#pragma region BasicEffect
BasicEffect::BasicEffect(ID3D11Device* device, const std::wstring& filename)
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

		WorldViewProj = mFX->GetVariableByName("gWorldViewProj")->AsMatrix();
		World = mFX->GetVariableByName("gWorld")->AsMatrix();
		WorldInvTranspose = mFX->GetVariableByName("gWorldInvTranspose")->AsMatrix();
		TexTransform = mFX->GetVariableByName("gTexTransform")->AsMatrix();
		EyePosW = mFX->GetVariableByName("gEyePosW")->AsVector();
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
	BasicFX = new BasicEffect(device, L"D:/Work/Direct3D11/Exc/Chapter08/FX/Basic.fx");
}

void Effects::DestroyAll()
{
	delete BasicFX;
	BasicFX = 0;
}
#pragma endregion