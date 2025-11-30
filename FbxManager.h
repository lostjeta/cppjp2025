#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <wrl/client.h>

// Forward declarations
struct aiScene;
struct aiNode;
struct aiMesh;
class ZGraphics;

// Simple vertex structure with TBN (Tangent, Bitangent, Normal)
struct VertexTBN
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT3 tangent;
    DirectX::XMFLOAT3 binormal;
    DirectX::XMFLOAT2 tex;
    DirectX::XMFLOAT4 color;
};

// Mesh subset for material-based rendering
struct FbxSubset
{
    uint32_t startIndex = 0;
    uint32_t indexCount = 0;
    uint32_t materialIndex = 0;
};

// Skeleton node
struct FbxSkeletonNode
{
    std::string name;
    int parent = -1;
    std::vector<int> children;
    bool isBone = false;
};

// Simplified FbxManager for D3D11
class FbxManager
{
public:
    FbxManager();
    ~FbxManager();

    // Load FBX/OBJ file using Assimp
    bool Load(ZGraphics& gfx, const std::string& filePath, const std::string& defaultTexturePath = "");
    bool Load(ZGraphics& gfx, const std::string& filePath, const std::vector<std::string>& defaultTexturePaths);
    bool Load(ZGraphics& gfx, const std::string& filePath, const std::vector<std::string>& defaultTexturePaths, const std::vector<std::string>& defaultNormalMapPaths);
    bool Load(ZGraphics& gfx, const std::string& filePath, const std::vector<std::string>& defaultTexturePaths, const std::vector<std::string>& defaultNormalMapPaths, const std::vector<std::string>& defaultSpecularMapPaths);
    void Release();

    // Queries
    bool HasMesh() const;
    ID3D11Buffer* GetVertexBuffer() const;
    ID3D11Buffer* GetIndexBuffer() const;
    int GetIndexCount() const;
    UINT GetVertexStride() const;
    UINT GetVertexOffset() const;

    const std::vector<FbxSubset>& GetSubsets() const;
    const std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>& GetMaterialSRVs() const;
    const std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>& GetNormalMapSRVs() const;
    const std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>& GetSpecularMapSRVs() const;
    
    // Access to scene data
    const struct aiScene* GetScene() const;

    // Skeleton
    const std::vector<FbxSkeletonNode>& GetSkeleton() const;
    int GetSkeletonRoot() const;
    bool HasSkeleton() const;
    bool HasSkinning() const;
    int GetBoneCount() const;

    // Animation (basic support)
    bool HasAnimations() const;
    int GetAnimationCount() const;
    const std::vector<std::string>& GetAnimationNames() const;
    
    // Animation control
    int GetCurrentAnimationIndex() const;
    void SetCurrentAnimation(int idx);
    void SetAnimationPlaying(bool playing);
    bool IsAnimationPlaying() const;
    double GetAnimationTimeSeconds() const;
    void SetAnimationTimeSeconds(double t);
    double GetAnimationDuration(int idx) const;  // Alias for GetClipDurationSec
    double GetClipDurationSec(int idx) const;
    
    // Load external animation files
    bool LoadExternalAnimation(const std::string& animFilePath, const std::string& animName = "");
    bool LoadExternalAnimations(const std::vector<std::string>& animFilePaths);
    
    // Update animation (call per frame)
    void UpdateAnimation(float deltaTime);
    
    // Upload bone palette to GPU (call before rendering)
    void UploadBonePaletteToGPU(ZGraphics& gfx);
    
    // Get bone constant buffer for binding
    ID3D11Buffer* GetBoneConstantBuffer() const;
    
    // Check if this is a Mixamo model (based on bone names)
    bool IsMixamoModel() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_;

    // Loading helpers
    bool LoadMaterials(ZGraphics& gfx, const aiScene* scene, const std::string& baseDir, const std::string& defaultTexturePath);
    bool LoadMaterials(ZGraphics& gfx, const aiScene* scene, const std::string& baseDir, const std::vector<std::string>& defaultTexturePaths);
    bool LoadMaterials(ZGraphics& gfx, const aiScene* scene, const std::string& baseDir, const std::vector<std::string>& defaultTexturePaths, const std::vector<std::string>& defaultNormalMapPaths);
    bool LoadMaterials(ZGraphics& gfx, const aiScene* scene, const std::string& baseDir, const std::vector<std::string>& defaultTexturePaths, const std::vector<std::string>& defaultNormalMapPaths, const std::vector<std::string>& defaultSpecularMapPaths);
    bool BuildMeshBuffers(ZGraphics& gfx, const aiScene* scene);
    void BuildSkeleton(const aiNode* node, int parentIndex);
    void CollectBones(const aiScene* scene);
    void InitAnimationMetadata(const aiScene* scene);

    std::string ExtractDirectory(const std::string& path);
};
