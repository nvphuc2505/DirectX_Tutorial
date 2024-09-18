#pragma once

#include <DXUT.h>
#include <SDKmisc.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include "Vertex.h"
#include "SkinData.h"
#include <map>

class SkinnedModel
{
public:
	SkinnedModel(ID3D11Device* pd3dDevice, const std::string& modelFilename, const aiScene* scene);
	~SkinnedModel();

	
	std::vector<ID3D11Buffer*> VertexBuffers;
	std::vector<ID3D11Buffer*> IndexBuffers;

	std::vector<Vertex::PosNormalTexTanSkinned> Vertices;

	std::vector<unsigned int> IndexCount;
	
	ID3D10ShaderResourceView* g_ModelSRV;
	SkinnedData SkinnedData;

	void InitializeSkinnedData(const aiScene* scene);
	void ExtractAnimations(const aiScene* scene, std::map<std::string, AnimationClip>& animations);
	void ExtractBoneOffsets(const aiScene* scene, std::vector<XMFLOAT4X4>& boneOffsets);
	void ExtractBoneHierarchy(aiNode* node, int parentIndex, std::vector<int>& boneHierarchy);
};



struct SkinnedModelInstance
{
public:
	SkinnedModel* Model;

	float TimePos;
	std::string ClipName;
	XMFLOAT4X4 World;
	std::vector<XMFLOAT4X4> FinalTransforms;

	void Update(float dt);
};
