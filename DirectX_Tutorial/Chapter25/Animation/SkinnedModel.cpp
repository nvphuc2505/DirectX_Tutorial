#include "SkinnedModel.h"

void SkinnedModel::ExtractBoneOffsets(const aiScene* scene, std::vector<XMFLOAT4X4>& boneOffsets)
{
	std::map<std::string, int> boneNameToIndexMap;

	for (unsigned int i = 0; i < scene->mNumMeshes; i++)
	{
		const aiMesh* mesh = scene->mMeshes[i];
		for (unsigned int j = 0; j < mesh->mNumBones; j++)
		{
			aiBone* bone = mesh->mBones[j];
			std::string boneName = bone->mName.C_Str();

			// Check if the bone is already mapped
			if (boneNameToIndexMap.find(boneName) == boneNameToIndexMap.end())
			{
				boneNameToIndexMap[boneName] = boneNameToIndexMap.size();
			}

			int boneIndex = boneNameToIndexMap[boneName];
			aiMatrix4x4 offsetMatrix = bone->mOffsetMatrix;

			XMFLOAT4X4 boneOffset;
			XMMATRIX xmOffsetMatrix = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&offsetMatrix));
			XMStoreFloat4x4(&boneOffset, XMMatrixTranspose(xmOffsetMatrix));

			if (boneIndex >= boneOffsets.size())
			{
				boneOffsets.resize(boneIndex + 1);
			}
			boneOffsets[boneIndex] = boneOffset;
		}
	}
}


void SkinnedModel::ExtractAnimations(const aiScene* scene, std::map<std::string, AnimationClip>& animations)
{
	for (unsigned int i = 0; i < scene->mNumAnimations; i++)
	{
		const aiAnimation* anim = scene->mAnimations[i];
		AnimationClip clip;

		// Process the bone animations for each channel
		for (unsigned int j = 0; j < anim->mNumChannels; j++)
		{
			const aiNodeAnim* channel = anim->mChannels[j];

			BoneAnimation boneAnim;
			boneAnim.Keyframes.resize(channel->mNumPositionKeys);

			// Fill in keyframes for position, rotation, and scale
			for (unsigned int k = 0; k < channel->mNumPositionKeys; k++)
			{
				Keyframe keyframe;
				keyframe.TimePos = (float)channel->mPositionKeys[k].mTime;
				keyframe.Translation = XMFLOAT3(
					channel->mPositionKeys[k].mValue.x,
					channel->mPositionKeys[k].mValue.y,
					channel->mPositionKeys[k].mValue.z);

				keyframe.Scale = XMFLOAT3(
					channel->mScalingKeys[k].mValue.x,
					channel->mScalingKeys[k].mValue.y,
					channel->mScalingKeys[k].mValue.z);

				keyframe.RotationQuat = XMFLOAT4(
					channel->mRotationKeys[k].mValue.x,
					channel->mRotationKeys[k].mValue.y,
					channel->mRotationKeys[k].mValue.z,
					channel->mRotationKeys[k].mValue.w);

				boneAnim.Keyframes[k] = keyframe;
			}

			clip.BoneAnimations.push_back(boneAnim);
		}

		animations[anim->mName.C_Str()] = clip;
	}
}


void SkinnedModel::InitializeSkinnedData(const aiScene* scene)
{
	// 1. Extract Bone Hierarchy
	std::vector<int> boneHierarchy;
	ExtractBoneHierarchy(scene->mRootNode, -1, boneHierarchy);

	// 2. Extract Bone Offsets
	std::vector<XMFLOAT4X4> boneOffsets;
	ExtractBoneOffsets(scene, boneOffsets);

	// 3. Check if the model has animations before attempting to extract them
	std::map<std::string, AnimationClip> animations;
	if (scene->mNumAnimations > 0)
	{
		ExtractAnimations(scene, animations);
	}
	else
	{
		// No animations found, leave animations map empty
		OutputDebugStringA("No animations found in model.\n");
	}

	// Set skinned data even if animations are empty
	SkinnedData.Set(boneHierarchy, boneOffsets, animations);
}


void SkinnedModel::ExtractBoneHierarchy(aiNode* node, int parentIndex, std::vector<int>& boneHierarchy)
{
	// Find the index of this bone in the bone hierarchy
	int nodeIndex = boneHierarchy.size();
	boneHierarchy.push_back(parentIndex); // Parent of this bone is the index passed as parentIndex

	// Recursively process child bones
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		ExtractBoneHierarchy(node->mChildren[i], nodeIndex, boneHierarchy);
	}
}


SkinnedModel::SkinnedModel(ID3D11Device* pd3dDevice, const std::string& modelFilename, const aiScene* scene)
{
	// Extract bone hierarchy and bone offsets only
	std::vector<int> boneHierarchy;
	std::vector<XMFLOAT4X4> boneOffsets;

	// Extract bone hierarchy
	ExtractBoneHierarchy(scene->mRootNode, -1, boneHierarchy);

	// Extract bone offsets
	ExtractBoneOffsets(scene, boneOffsets);

	// Set skinned data with empty animations
	std::map<std::string, AnimationClip> emptyAnimations;
	SkinnedData.Set(boneHierarchy, boneOffsets, emptyAnimations);

	for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
	{
		const aiMesh* mesh = scene->mMeshes[m];
		Vertices.resize(mesh->mNumVertices);

		if (mesh->HasPositions())
		{
			for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
			{
				aiVector3D* vp = &(mesh->mVertices[i]);
				Vertices[i].Pos = XMFLOAT3(vp->x, vp->y, vp->z);
			}
		}

		if (mesh->HasNormals())
		{
			for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
			{
				const aiVector3D* vn = &(mesh->mNormals[i]);
				Vertices[i].Normal = XMFLOAT3(vn->x, vn->y, vn->z);
			}
		}

		if (mesh->HasTextureCoords(0))
		{
			for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
			{
				const aiVector3D* vt = &(mesh->mTextureCoords[0][i]);
				Vertices[i].Tex = XMFLOAT2(vt->x, vt->y);
			}
		}

		if (mesh->HasTangentsAndBitangents())
		{
			for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
			{
				aiVector3D* tangent = &(mesh->mTangents[i]);
				aiVector3D* bitangent = &(mesh->mBitangents[i]);
				aiVector3D* normal = &(mesh->mNormals[i]);

				XMVECTOR xmTangent = DirectX::XMVectorSet(tangent->x, tangent->y, tangent->z, 0.0f);
				XMVECTOR xmBitangent = DirectX::XMVectorSet(bitangent->x, bitangent->y, bitangent->z, 0.0f);
				XMVECTOR xmNormal = DirectX::XMVectorSet(normal->x, normal->y, normal->z, 0.0f);

				DirectX::XMVECTOR crossProduct = DirectX::XMVector3Cross(xmNormal, xmTangent);
				DirectX::XMVECTOR dotProduct = DirectX::XMVector3Dot(crossProduct, xmBitangent);
				float handedness = (DirectX::XMVectorGetX(dotProduct) < 0.0f) ? -1.0f : 1.0f;

				Vertices[i].TangentU = XMFLOAT4(tangent->x, tangent->y, tangent->z, handedness);
			}
		}

		std::vector<unsigned int> indices;
		indices.reserve(mesh->mNumFaces * 3);
		if (mesh->HasFaces())
		{
			for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
			{
				aiFace face = (mesh->mFaces[i]);
				for (unsigned int j = 0; j < face.mNumIndices; ++j)
				{
					indices.push_back(face.mIndices[j]);
				}
			}
		}
		UINT indexCount = indices.size();
		IndexCount.push_back(indexCount);

		D3D11_BUFFER_DESC vbd = {};
		vbd.ByteWidth = sizeof(Vertex::PosNormalTexTanSkinned) * Vertices.size();
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA vInitData = {};
		vInitData.pSysMem = Vertices.data();
		ID3D11Buffer* vertexBuffer;
		HRESULT hr = pd3dDevice->CreateBuffer(&vbd, &vInitData, &vertexBuffer);
		VertexBuffers.push_back(vertexBuffer);

		D3D11_BUFFER_DESC ibd = {};
		ibd.ByteWidth = sizeof(unsigned int) * indexCount;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA iInitData = {};
		iInitData.pSysMem = indices.data();
		ID3D11Buffer* indexBuffer;
		hr = pd3dDevice->CreateBuffer(&ibd, &iInitData, &indexBuffer);
		IndexBuffers.push_back(indexBuffer);
	}
}


SkinnedModel::~SkinnedModel()
{
}

void SkinnedModelInstance::Update(float dt)
{
	if (!Model->SkinnedData.HasAnimations())  // Add this check to avoid updating if no animations
		return;

	TimePos += dt;
	Model->SkinnedData.GetFinalTransforms(ClipName, TimePos, FinalTransforms);

	// Loop animation
	if (TimePos > Model->SkinnedData.GetClipEndTime(ClipName))
		TimePos = 0.0f;
}

