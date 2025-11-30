#include "FbxManager.h"
#include "ZGraphics.h"
#include "ZVertex.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <wincodec.h>
#include <chrono>
#include <algorithm>

#define USE_DIRECTXTK
// DirectXTK headers (conditional)
#ifdef USE_DIRECTXTK
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#endif
#include <filesystem>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// Helper function to clean bone names for matching
std::string CleanBoneName(const std::string& animBoneName) {
    std::string cleaned = animBoneName;
    
    // Replace "mixamorig1:" with "mixamorig:"
    if (cleaned.find("mixamorig1:") == 0) {
        cleaned = "mixamorig:" + cleaned.substr(11); // Skip "mixamorig1:"
    }
    
    // Remove "$AssimpFbx$_Rotation" suffix
    size_t pos = cleaned.find("$AssimpFbx$_Rotation");
    if (pos != std::string::npos) {
        cleaned = cleaned.substr(0, pos);
    }
    
    // Remove "$AssimpFbx$_Translation" suffix
    pos = cleaned.find("$AssimpFbx$_Translation");
    if (pos != std::string::npos) {
        cleaned = cleaned.substr(0, pos);
    }
    
    // Remove "$AssimpFbx$_PreRotation" suffix
    pos = cleaned.find("$AssimpFbx$_PreRotation");
    if (pos != std::string::npos) {
        cleaned = cleaned.substr(0, pos);
    }
    
    return cleaned;
}

// Skinned vertex structure matching VertexInSkinned in shader
struct VertexSkinned
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT3 tangent;
    XMFLOAT3 bitangent;
    XMFLOAT2 texCoord;
    XMFLOAT4 color;
    UINT boneIndices[4];  // uint4 in shader
    XMFLOAT4 boneWeights; // float4 in shader
};

// Implementation struct
struct FbxManager::Impl
{
    // Assimp
    std::unique_ptr<Assimp::Importer> importer;
    const aiScene* scene = nullptr;

    // Mesh buffers
    ID3D11Buffer* pVertexBuffer = nullptr;
    ID3D11Buffer* pIndexBuffer = nullptr;
    int indexCount = 0;
    UINT vertexStride = sizeof(VertexSkinned);  // Skinned vertex with bone data
    UINT vertexOffset = 0;

    // Subsets and materials
    std::vector<FbxSubset> subsets;
    std::vector<ComPtr<ID3D11ShaderResourceView>> materialSRVs;
    std::vector<ComPtr<ID3D11ShaderResourceView>> normalMapSRVs;
    std::vector<ComPtr<ID3D11ShaderResourceView>> specularMapSRVs;
    std::unordered_map<std::string, ComPtr<ID3D11ShaderResourceView>> textureCache;
    ComPtr<ID3D11ShaderResourceView> fallbackBaseTexture;

    // Skeleton
    std::vector<FbxSkeletonNode> skeleton;
    int skeletonRoot = -1;
    std::unordered_map<std::string, int> nodeIndexOfName;

    // Animation
    bool hasAnimations = false;
    std::vector<std::string> animationNames;
    std::vector<double> clipDurationSec;
    std::vector<double> clipTicksPerSec;
    int currentClip = -1;
    double clipTimeSec = 0.0;
    bool playing = false;
    
    // Bone data for skeletal animation
    std::vector<std::string> boneNames;
    std::vector<XMFLOAT4X4> boneOffsets;
    std::unordered_map<std::string, int> boneIndexOfName;
    XMFLOAT4X4 globalInverse;
    bool hasSkinning = false;
    
    // Animation channel cache
    std::vector<const aiNodeAnim*> channelOfNode;
    
    // External animations
    struct ExternalAnimation
    {
        std::unique_ptr<Assimp::Importer> importer;
        const aiScene* scene = nullptr;
        std::string name;
        double duration = 0.0;
        double ticksPerSecond = 25.0;
    };
    std::vector<ExternalAnimation> externalAnimations;
    int baseAnimationCount = 0;  // Number of animations from main file
    int lastLoggedClip = -1;     // Track last logged animation for debug output
    
    // Bone constant buffer for GPU skinning
    ID3D11Buffer* pBoneCB = nullptr;
    static constexpr int kMaxBones = 1023;  // Match shader MAX_BONES
    
    // Current bone palette (computed each frame)
    std::vector<XMMATRIX> currentBonePalette;
};

FbxManager::FbxManager()
    : m_(std::make_unique<Impl>())
{
}

FbxManager::~FbxManager()
{
    Release();
}

void FbxManager::Release()
{
    if (m_->pVertexBuffer)
    {
        m_->pVertexBuffer->Release();
        m_->pVertexBuffer = nullptr;
    }

    if (m_->pIndexBuffer)
    {
        m_->pIndexBuffer->Release();
        m_->pIndexBuffer = nullptr;
    }

    m_->materialSRVs.clear();
    m_->normalMapSRVs.clear();
    m_->specularMapSRVs.clear();
    m_->textureCache.clear();
    m_->fallbackBaseTexture.Reset();
    m_->subsets.clear();
    m_->skeleton.clear();
    m_->animationNames.clear();
    m_->boneNames.clear();
    m_->boneOffsets.clear();
    m_->boneIndexOfName.clear();
    m_->channelOfNode.clear();
    
    if (m_->pBoneCB)
    {
        m_->pBoneCB->Release();
        m_->pBoneCB = nullptr;
    }
    
    m_->scene = nullptr;
    m_->importer.reset();
}

bool FbxManager::Load(ZGraphics& gfx, const std::string& filePath, const std::string& defaultTexturePath)
{
    auto loadStartTime = std::chrono::high_resolution_clock::now();
    std::cout << "=== FbxManager::Load ===" << std::endl;
    std::cout << "Loading: " << filePath << std::endl;
    if (!defaultTexturePath.empty())
    {
        std::cout << "Default texture: " << defaultTexturePath << std::endl;
    }

    // Create Assimp importer
    m_->importer = std::make_unique<Assimp::Importer>();

    // Load with Assimp
    // Note: GenNormals and CalcTangentSpace are SLOW on large models
    // Only use them if the model doesn't already have them
    const unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ConvertToLeftHanded |
        aiProcess_GenSmoothNormals |  // Only generate if missing
        aiProcess_CalcTangentSpace |   // Calculate tangent space (slow but needed)
        aiProcess_LimitBoneWeights;

    std::cout << "Reading file with Assimp..." << std::flush;
    auto assimpStart = std::chrono::high_resolution_clock::now();
    m_->scene = m_->importer->ReadFile(filePath, flags);
    auto assimpEnd = std::chrono::high_resolution_clock::now();
    auto assimpDuration = std::chrono::duration_cast<std::chrono::milliseconds>(assimpEnd - assimpStart);
    std::cout << " Done (" << assimpDuration.count() << "ms)" << std::endl;

    if (!m_->scene || !m_->scene->mRootNode || m_->scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
    {
        std::cerr << "Assimp Error: " << m_->importer->GetErrorString() << std::endl;
        return false;
    }

    std::cout << "Mesh count: " << m_->scene->mNumMeshes << std::endl;
    std::cout << "Material count: " << m_->scene->mNumMaterials << std::endl;

    // Save global inverse of root (for animation)
    if (m_->scene->mRootNode)
    {
        aiMatrix4x4 rootInv = m_->scene->mRootNode->mTransformation;
        rootInv.Inverse();
        m_->globalInverse._11 = rootInv.a1; m_->globalInverse._12 = rootInv.a2; m_->globalInverse._13 = rootInv.a3; m_->globalInverse._14 = rootInv.a4;
        m_->globalInverse._21 = rootInv.b1; m_->globalInverse._22 = rootInv.b2; m_->globalInverse._23 = rootInv.b3; m_->globalInverse._24 = rootInv.b4;
        m_->globalInverse._31 = rootInv.c1; m_->globalInverse._32 = rootInv.c2; m_->globalInverse._33 = rootInv.c3; m_->globalInverse._34 = rootInv.c4;
        m_->globalInverse._41 = rootInv.d1; m_->globalInverse._42 = rootInv.d2; m_->globalInverse._43 = rootInv.d3; m_->globalInverse._44 = rootInv.d4;
    }

    // Extract base directory
    std::string baseDir = ExtractDirectory(filePath);

    // Build skeleton
    std::cout << "Building skeleton..." << std::flush;
    auto skelStart = std::chrono::high_resolution_clock::now();
    if (m_->scene->mRootNode)
    {
        BuildSkeleton(m_->scene->mRootNode, -1);
        m_->skeletonRoot = 0;
        CollectBones(m_->scene);
    }
    auto skelEnd = std::chrono::high_resolution_clock::now();
    auto skelDuration = std::chrono::duration_cast<std::chrono::milliseconds>(skelEnd - skelStart);
    std::cout << " Done (" << skelDuration.count() << "ms)" << std::endl;
    std::cout << "Skeleton nodes: " << m_->skeleton.size() << std::endl;

    // Load materials and textures
    if (!LoadMaterials(gfx, m_->scene, baseDir, defaultTexturePath))
    {
        std::cerr << "Failed to load materials" << std::endl;
        return false;
    }

    // Build mesh buffers
    if (!BuildMeshBuffers(gfx, m_->scene))
    {
        std::cerr << "Failed to build mesh buffers" << std::endl;
        return false;
    }

    // Initialize animation metadata
    InitAnimationMetadata(m_->scene);

    auto loadEndTime = std::chrono::high_resolution_clock::now();
    auto loadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(loadEndTime - loadStartTime);
    std::cout << "=== FbxManager::Load Complete (Total time: " << loadDuration.count() << "ms) ===" << std::endl;
    return true;
}

// Load with multiple default textures (one per material)
bool FbxManager::Load(ZGraphics& gfx, const std::string& filePath, const std::vector<std::string>& defaultTexturePaths)
{
    auto loadStartTime = std::chrono::high_resolution_clock::now();
    std::cout << "=== FbxManager::Load ===" << std::endl;
    std::cout << "Loading: " << filePath << std::endl;
    if (!defaultTexturePaths.empty())
    {
        std::cout << "Default textures: " << defaultTexturePaths.size() << " textures" << std::endl;
    }

    // Create Assimp importer
    m_->importer = std::make_unique<Assimp::Importer>();

    // Load with Assimp
    const unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ConvertToLeftHanded |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_LimitBoneWeights;

    std::cout << "Reading file with Assimp..." << std::flush;
    auto assimpStart = std::chrono::high_resolution_clock::now();
    m_->scene = m_->importer->ReadFile(filePath, flags);
    auto assimpEnd = std::chrono::high_resolution_clock::now();
    auto assimpDuration = std::chrono::duration_cast<std::chrono::milliseconds>(assimpEnd - assimpStart);
    std::cout << " Done (" << assimpDuration.count() << "ms)" << std::endl;

    if (!m_->scene || !m_->scene->mRootNode || m_->scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
    {
        std::cerr << "Assimp Error: " << m_->importer->GetErrorString() << std::endl;
        return false;
    }

    std::cout << "Mesh count: " << m_->scene->mNumMeshes << std::endl;
    std::cout << "Material count: " << m_->scene->mNumMaterials << std::endl;

    // Save global inverse of root (for animation)
    if (m_->scene->mRootNode)
    {
        aiMatrix4x4 rootInv = m_->scene->mRootNode->mTransformation;
        rootInv.Inverse();
        m_->globalInverse._11 = rootInv.a1; m_->globalInverse._12 = rootInv.a2; m_->globalInverse._13 = rootInv.a3; m_->globalInverse._14 = rootInv.a4;
        m_->globalInverse._21 = rootInv.b1; m_->globalInverse._22 = rootInv.b2; m_->globalInverse._23 = rootInv.b3; m_->globalInverse._24 = rootInv.b4;
        m_->globalInverse._31 = rootInv.c1; m_->globalInverse._32 = rootInv.c2; m_->globalInverse._33 = rootInv.c3; m_->globalInverse._34 = rootInv.c4;
        m_->globalInverse._41 = rootInv.d1; m_->globalInverse._42 = rootInv.d2; m_->globalInverse._43 = rootInv.d3; m_->globalInverse._44 = rootInv.d4;
    }

    // Extract base directory
    std::string baseDir = ExtractDirectory(filePath);

    // Build skeleton
    std::cout << "Building skeleton..." << std::flush;
    auto skelStart = std::chrono::high_resolution_clock::now();
    if (m_->scene->mRootNode)
    {
        BuildSkeleton(m_->scene->mRootNode, -1);
        m_->skeletonRoot = 0;
        CollectBones(m_->scene);
    }
    auto skelEnd = std::chrono::high_resolution_clock::now();
    auto skelDuration = std::chrono::duration_cast<std::chrono::milliseconds>(skelEnd - skelStart);
    std::cout << " Done (" << skelDuration.count() << "ms)" << std::endl;
    std::cout << "Skeleton nodes: " << m_->skeleton.size() << std::endl;

    // Load materials and textures with multiple defaults
    if (!LoadMaterials(gfx, m_->scene, baseDir, defaultTexturePaths))
    {
        std::cerr << "Failed to load materials" << std::endl;
        return false;
    }

    // Build mesh buffers
    if (!BuildMeshBuffers(gfx, m_->scene))
    {
        std::cerr << "Failed to build mesh buffers" << std::endl;
        return false;
    }

    // Initialize animation metadata
    InitAnimationMetadata(m_->scene);

    auto loadEndTime = std::chrono::high_resolution_clock::now();
    auto loadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(loadEndTime - loadStartTime);
    std::cout << "=== FbxManager::Load Complete (Total time: " << loadDuration.count() << "ms) ===" << std::endl;
    return true;
}

// Load with multiple default textures and normal maps (one per material)
bool FbxManager::Load(ZGraphics& gfx, const std::string& filePath, const std::vector<std::string>& defaultTexturePaths, const std::vector<std::string>& defaultNormalMapPaths)
{
    auto loadStartTime = std::chrono::high_resolution_clock::now();
    std::cout << "=== FbxManager::Load ===" << std::endl;
    std::cout << "Loading: " << filePath << std::endl;
    if (!defaultTexturePaths.empty())
    {
        std::cout << "Default textures: " << defaultTexturePaths.size() << " textures" << std::endl;
    }
    if (!defaultNormalMapPaths.empty())
    {
        std::cout << "Default normal maps: " << defaultNormalMapPaths.size() << " normal maps" << std::endl;
    }

    // Create Assimp importer
    m_->importer = std::make_unique<Assimp::Importer>();

    // Load with Assimp
    const unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ConvertToLeftHanded |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_LimitBoneWeights;

    std::cout << "Reading file with Assimp..." << std::flush;
    auto assimpStart = std::chrono::high_resolution_clock::now();
    m_->scene = m_->importer->ReadFile(filePath, flags);
    auto assimpEnd = std::chrono::high_resolution_clock::now();
    auto assimpDuration = std::chrono::duration_cast<std::chrono::milliseconds>(assimpEnd - assimpStart);
    std::cout << " Done (" << assimpDuration.count() << "ms)" << std::endl;

    if (!m_->scene || !m_->scene->mRootNode || m_->scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
    {
        std::cerr << "Assimp Error: " << m_->importer->GetErrorString() << std::endl;
        return false;
    }

    std::cout << "Mesh count: " << m_->scene->mNumMeshes << std::endl;
    std::cout << "Material count: " << m_->scene->mNumMaterials << std::endl;

    // Save global inverse of root (for animation)
    if (m_->scene->mRootNode)
    {
        XMMATRIX globalTransform = XMMATRIX(
            m_->scene->mRootNode->mTransformation.a1, m_->scene->mRootNode->mTransformation.b1, m_->scene->mRootNode->mTransformation.c1, m_->scene->mRootNode->mTransformation.d1,
            m_->scene->mRootNode->mTransformation.a2, m_->scene->mRootNode->mTransformation.b2, m_->scene->mRootNode->mTransformation.c2, m_->scene->mRootNode->mTransformation.d2,
            m_->scene->mRootNode->mTransformation.a3, m_->scene->mRootNode->mTransformation.b3, m_->scene->mRootNode->mTransformation.c3, m_->scene->mRootNode->mTransformation.d3,
            m_->scene->mRootNode->mTransformation.a4, m_->scene->mRootNode->mTransformation.b4, m_->scene->mRootNode->mTransformation.c4, m_->scene->mRootNode->mTransformation.d4
        );
        XMStoreFloat4x4(&m_->globalInverse, XMMatrixInverse(nullptr, globalTransform));
    }

    // Extract base directory for texture loading
    std::string baseDir = ExtractDirectory(filePath);

    // Build skeleton
    std::cout << "Building skeleton..." << std::flush;
    auto skelStart = std::chrono::high_resolution_clock::now();
    BuildSkeleton(m_->scene->mRootNode, -1);
    CollectBones(m_->scene);
    auto skelEnd = std::chrono::high_resolution_clock::now();
    auto skelDuration = std::chrono::duration_cast<std::chrono::milliseconds>(skelEnd - skelStart);
    std::cout << " Done (" << skelDuration.count() << "ms)" << std::endl;
    std::cout << "Skeleton nodes: " << m_->skeleton.size() << std::endl;

    // Load materials and textures with multiple defaults and normal maps
    if (!LoadMaterials(gfx, m_->scene, baseDir, defaultTexturePaths, defaultNormalMapPaths))
    {
        std::cerr << "Failed to load materials" << std::endl;
        return false;
    }

    // Build mesh buffers
    if (!BuildMeshBuffers(gfx, m_->scene))
    {
        std::cerr << "Failed to build mesh buffers" << std::endl;
        return false;
    }

    // Initialize animation metadata
    InitAnimationMetadata(m_->scene);

    auto loadEndTime = std::chrono::high_resolution_clock::now();
    auto loadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(loadEndTime - loadStartTime);
    std::cout << "=== FbxManager::Load Complete (Total time: " << loadDuration.count() << "ms) ===" << std::endl;
    return true;
}

// Load with multiple default textures, normal maps, and specular maps (one per material)
bool FbxManager::Load(ZGraphics& gfx, const std::string& filePath, const std::vector<std::string>& defaultTexturePaths, const std::vector<std::string>& defaultNormalMapPaths, const std::vector<std::string>& defaultSpecularMapPaths)
{
    auto loadStartTime = std::chrono::high_resolution_clock::now();
    std::cout << "=== FbxManager::Load ===" << std::endl;
    std::cout << "Loading: " << filePath << std::endl;
    if (!defaultTexturePaths.empty())
    {
        std::cout << "Default textures: " << defaultTexturePaths.size() << " textures" << std::endl;
    }
    if (!defaultNormalMapPaths.empty())
    {
        std::cout << "Default normal maps: " << defaultNormalMapPaths.size() << " normal maps" << std::endl;
    }
    if (!defaultSpecularMapPaths.empty())
    {
        std::cout << "Default specular maps: " << defaultSpecularMapPaths.size() << " specular maps" << std::endl;
    }

    // Create Assimp importer
    m_->importer = std::make_unique<Assimp::Importer>();

    // Load with Assimp
    const unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ConvertToLeftHanded |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_LimitBoneWeights;

    std::cout << "Reading file with Assimp..." << std::flush;
    auto assimpStart = std::chrono::high_resolution_clock::now();
    m_->scene = m_->importer->ReadFile(filePath, flags);
    auto assimpEnd = std::chrono::high_resolution_clock::now();
    auto assimpDuration = std::chrono::duration_cast<std::chrono::milliseconds>(assimpEnd - assimpStart);
    std::cout << " Done (" << assimpDuration.count() << "ms)" << std::endl;

    if (!m_->scene || m_->scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_->scene->mRootNode)
    {
        std::cerr << "Error loading FBX file: " << m_->importer->GetErrorString() << std::endl;
        return false;
    }

    // Save global inverse of root (for animation) - CRITICAL!
    if (m_->scene->mRootNode)
    {
        XMMATRIX globalTransform = XMMATRIX(
            m_->scene->mRootNode->mTransformation.a1, m_->scene->mRootNode->mTransformation.b1, m_->scene->mRootNode->mTransformation.c1, m_->scene->mRootNode->mTransformation.d1,
            m_->scene->mRootNode->mTransformation.a2, m_->scene->mRootNode->mTransformation.b2, m_->scene->mRootNode->mTransformation.c2, m_->scene->mRootNode->mTransformation.d2,
            m_->scene->mRootNode->mTransformation.a3, m_->scene->mRootNode->mTransformation.b3, m_->scene->mRootNode->mTransformation.c3, m_->scene->mRootNode->mTransformation.d3,
            m_->scene->mRootNode->mTransformation.a4, m_->scene->mRootNode->mTransformation.b4, m_->scene->mRootNode->mTransformation.c4, m_->scene->mRootNode->mTransformation.d4
        );
        XMStoreFloat4x4(&m_->globalInverse, XMMatrixInverse(nullptr, globalTransform));
    }

    // Extract base directory from file path
    std::string baseDir = filePath.substr(0, filePath.find_last_of("/\\") + 1);

    // Build skeleton
    std::cout << "Building skeleton..." << std::flush;
    auto skelStart = std::chrono::high_resolution_clock::now();
    BuildSkeleton(m_->scene->mRootNode, -1);
    CollectBones(m_->scene);
    auto skelEnd = std::chrono::high_resolution_clock::now();
    auto skelDuration = std::chrono::duration_cast<std::chrono::milliseconds>(skelEnd - skelStart);
    std::cout << " Done (" << skelDuration.count() << "ms)" << std::endl;
    std::cout << "Skeleton nodes: " << m_->skeleton.size() << std::endl;

    // Load materials and textures with multiple defaults, normal maps, and specular maps
    if (!LoadMaterials(gfx, m_->scene, baseDir, defaultTexturePaths, defaultNormalMapPaths, defaultSpecularMapPaths))
    {
        std::cerr << "Failed to load materials" << std::endl;
        return false;
    }

    // Build mesh buffers
    if (!BuildMeshBuffers(gfx, m_->scene))
    {
        std::cerr << "Failed to build mesh buffers" << std::endl;
        return false;
    }

    // Initialize animation metadata
    InitAnimationMetadata(m_->scene);

    auto loadEndTime = std::chrono::high_resolution_clock::now();
    auto loadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(loadEndTime - loadStartTime);
    std::cout << "=== FbxManager::Load Complete (Total time: " << loadDuration.count() << "ms) ===" << std::endl;
    return true;
}

bool FbxManager::HasMesh() const
{
    return m_->pVertexBuffer != nullptr && m_->pIndexBuffer != nullptr;
}

ID3D11Buffer* FbxManager::GetVertexBuffer() const
{
    return m_->pVertexBuffer;
}

ID3D11Buffer* FbxManager::GetIndexBuffer() const
{
    return m_->pIndexBuffer;
}

int FbxManager::GetIndexCount() const
{
    return m_->indexCount;
}

UINT FbxManager::GetVertexStride() const
{
    return m_->vertexStride;
}

UINT FbxManager::GetVertexOffset() const
{
    return m_->vertexOffset;
}

const std::vector<FbxSubset>& FbxManager::GetSubsets() const
{
    return m_->subsets;
}

const std::vector<ComPtr<ID3D11ShaderResourceView>>& FbxManager::GetMaterialSRVs() const
{
    return m_->materialSRVs;
}

const std::vector<ComPtr<ID3D11ShaderResourceView>>& FbxManager::GetNormalMapSRVs() const
{
    return m_->normalMapSRVs;
}

const std::vector<ComPtr<ID3D11ShaderResourceView>>& FbxManager::GetSpecularMapSRVs() const
{
    return m_->specularMapSRVs;
}

const aiScene* FbxManager::GetScene() const
{
    return m_->scene;
}

const std::vector<FbxSkeletonNode>& FbxManager::GetSkeleton() const
{
    return m_->skeleton;
}

int FbxManager::GetSkeletonRoot() const
{
    return m_->skeletonRoot;
}

bool FbxManager::HasSkeleton() const
{
    return !m_->skeleton.empty();
}

bool FbxManager::HasAnimations() const
{
    return m_->hasAnimations;
}

int FbxManager::GetAnimationCount() const
{
    return static_cast<int>(m_->animationNames.size());
}

const std::vector<std::string>& FbxManager::GetAnimationNames() const
{
    return m_->animationNames;
}

// Helper: Extract directory from file path
std::string FbxManager::ExtractDirectory(const std::string& path)
{
    size_t slash = path.find_last_of("/\\");
    return (slash == std::string::npos) ? "" : path.substr(0, slash + 1);
}

// Helper: Build skeleton hierarchy
void FbxManager::BuildSkeleton(const aiNode* node, int parentIndex)
{
    if (!node) return;

    int currentIndex = static_cast<int>(m_->skeleton.size());

    FbxSkeletonNode skelNode;
    skelNode.name = node->mName.C_Str();
    skelNode.parent = parentIndex;
    skelNode.isBone = false;

    m_->skeleton.push_back(skelNode);
    m_->nodeIndexOfName[skelNode.name] = currentIndex;

    // Process children
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        int childIndex = static_cast<int>(m_->skeleton.size());
        m_->skeleton[currentIndex].children.push_back(childIndex);
        BuildSkeleton(node->mChildren[i], currentIndex);
    }
}

// Helper: Collect bones from meshes
void FbxManager::CollectBones(const aiScene* scene)
{
    for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi)
    {
        const aiMesh* mesh = scene->mMeshes[mi];
        
        for (unsigned int bi = 0; bi < mesh->mNumBones; ++bi)
        {
            const aiBone* bone = mesh->mBones[bi];
            std::string boneName = bone->mName.C_Str();

            // Mark skeleton node as bone
            auto it = m_->nodeIndexOfName.find(boneName);
            if (it != m_->nodeIndexOfName.end())
            {
                m_->skeleton[it->second].isBone = true;
            }
            
            // Collect bone offset matrix (only once per bone)
            auto boneIt = m_->boneIndexOfName.find(boneName);
            if (boneIt == m_->boneIndexOfName.end())
            {
                int boneIdx = static_cast<int>(m_->boneNames.size());
                m_->boneNames.push_back(boneName);
                m_->boneIndexOfName[boneName] = boneIdx;
                
                // Store bone offset matrix
                const aiMatrix4x4& offset = bone->mOffsetMatrix;
                XMFLOAT4X4 offsetMat;
                offsetMat._11 = offset.a1; offsetMat._12 = offset.a2; offsetMat._13 = offset.a3; offsetMat._14 = offset.a4;
                offsetMat._21 = offset.b1; offsetMat._22 = offset.b2; offsetMat._23 = offset.b3; offsetMat._24 = offset.b4;
                offsetMat._31 = offset.c1; offsetMat._32 = offset.c2; offsetMat._33 = offset.c3; offsetMat._34 = offset.c4;
                offsetMat._41 = offset.d1; offsetMat._42 = offset.d2; offsetMat._43 = offset.d3; offsetMat._44 = offset.d4;
                m_->boneOffsets.push_back(offsetMat);
            }
        }
    }
    
    m_->hasSkinning = !m_->boneNames.empty();
    if (m_->hasSkinning)
    {
        std::cout << "Bones collected: " << m_->boneNames.size() << std::endl;
    }
}

// Helper: Initialize animation metadata
void FbxManager::InitAnimationMetadata(const aiScene* scene)
{
    if (scene->mNumAnimations == 0) return;

    m_->hasAnimations = true;
    m_->animationNames.reserve(scene->mNumAnimations);
    m_->clipDurationSec.reserve(scene->mNumAnimations);
    m_->clipTicksPerSec.reserve(scene->mNumAnimations);

    for (unsigned int i = 0; i < scene->mNumAnimations; ++i)
    {
        const aiAnimation* anim = scene->mAnimations[i];
        std::string name = (anim->mName.length > 0) ? anim->mName.C_Str() : ("Animation_" + std::to_string(i));
        m_->animationNames.push_back(name);

        // Store duration and ticks per second
        double tps = (anim->mTicksPerSecond > 0.0) ? anim->mTicksPerSecond : 25.0;
        double durationSec = anim->mDuration / tps;
        m_->clipTicksPerSec.push_back(tps);
        m_->clipDurationSec.push_back(durationSec);

        std::cout << "    " << name << " (duration: " << durationSec << "s, tps: " << tps << ")" << std::endl;
    }

    std::cout << "Animations: " << m_->animationNames.size() << std::endl;
    
    // Store base animation count
    m_->baseAnimationCount = static_cast<int>(m_->animationNames.size());
    std::cout << "[FbxManager] Base animation count set to: " << m_->baseAnimationCount << std::endl;
}

// Helper: Load materials and textures
bool FbxManager::LoadMaterials(ZGraphics& gfx, const aiScene* scene, const std::string& baseDir, const std::string& defaultTexturePath)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    std::cout << "=== LoadMaterials ===" << std::endl;
    std::cout << "Loading " << scene->mNumMaterials << " materials..." << std::endl;
    if (!defaultTexturePath.empty())
    {
        std::cout << "Default texture path: " << defaultTexturePath << std::endl;
    }

    // Create white fallback texture
    if (!m_->fallbackBaseTexture)
    {
        UINT white = 0xFFFFFFFF;
        D3D11_TEXTURE2D_DESC td{};
        td.Width = 1;
        td.Height = 1;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_IMMUTABLE;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA sd{};
        sd.pSysMem = &white;
        sd.SysMemPitch = sizeof(UINT);

        ComPtr<ID3D11Texture2D> tex;
        HRESULT hr = gfx.GetDeviceCOM()->CreateTexture2D(&td, &sd, &tex);
        if (SUCCEEDED(hr))
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
            srvd.Format = td.Format;
            srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvd.Texture2D.MipLevels = 1;
            srvd.Texture2D.MostDetailedMip = 0;

            gfx.GetDeviceCOM()->CreateShaderResourceView(tex.Get(), &srvd, &m_->fallbackBaseTexture);
        }
    }

    // Resize material SRVs
    m_->materialSRVs.resize(scene->mNumMaterials);

    // Load textures for each material
    for (unsigned int m = 0; m < scene->mNumMaterials; ++m)
    {
        aiMaterial* mat = scene->mMaterials[m];
        aiString texPath;

        std::cout << "Material [" << m << "]: ";

        if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
        {
            std::string texPathStr = texPath.C_Str();
            std::cout << "Original texture path: " << texPathStr << std::endl;

            if (!texPathStr.empty())
            {
                // Extract filename only
                std::string filename = texPathStr;
                size_t lastSlash = filename.find_last_of("/\\");
                if (lastSlash != std::string::npos)
                {
                    filename = filename.substr(lastSlash + 1);
                }
                
                std::cout << "Extracted filename: " << filename << std::endl;
                
                // Check cache first (use original path as key)
                auto cacheIt = m_->textureCache.find(texPathStr);
                if (cacheIt != m_->textureCache.end())
                {
                    m_->materialSRVs[m] = cacheIt->second;
                    std::cout << "  -> From cache" << std::endl;
                    continue;
                }

                // Try multiple possible paths
                std::vector<std::string> possiblePaths = {
                    baseDir + texPathStr,           // Original path
                    baseDir + filename,             // Direct in baseDir
                    baseDir + "../" + filename      // Parent directory
                };
                
                #ifdef USE_DIRECTXTK
                ComPtr<ID3D11Resource> res;
                ID3D11ShaderResourceView* srv = nullptr;
                HRESULT hr = E_FAIL;
                std::string successPath;
                
                // Try each possible path
                for (const auto& tryPath : possiblePaths)
                {
                    std::wstring wPath(tryPath.begin(), tryPath.end());
                    std::cout << "  -> Trying: " << tryPath << std::flush;
                    
                    hr = DirectX::CreateWICTextureFromFile(
                        gfx.GetDeviceCOM(),
                        wPath.c_str(),
                        res.GetAddressOf(),
                        &srv);
                    
                    if (SUCCEEDED(hr))
                    {
                        successPath = tryPath;
                        std::cout << " SUCCESS" << std::endl;
                        break;
                    }
                    else
                    {
                        std::cout << " FAILED (0x" << std::hex << hr << std::dec << ")" << std::endl;
                    }
                }
                
                if (SUCCEEDED(hr))
                {
                    m_->materialSRVs[m].Attach(srv);
                    m_->textureCache[texPathStr] = m_->materialSRVs[m];
                    std::cout << "  -> Loaded from: " << successPath << std::endl;
                }
                else
                {
                    std::cout << "  -> All paths failed" << std::endl;
                    
                    // Try default texture path if provided
                    if (!defaultTexturePath.empty())
                    {
                        std::wstring wDefaultPath(defaultTexturePath.begin(), defaultTexturePath.end());
                        std::cout << "  -> Trying default texture: " << defaultTexturePath << std::flush;
                        
                        hr = DirectX::CreateWICTextureFromFile(
                            gfx.GetDeviceCOM(),
                            wDefaultPath.c_str(),
                            res.GetAddressOf(),
                            &srv);
                        
                        if (SUCCEEDED(hr))
                        {
                            m_->materialSRVs[m].Attach(srv);
                            m_->textureCache[texPathStr] = m_->materialSRVs[m];
                            std::cout << " SUCCESS" << std::endl;
                        }
                        else
                        {
                            std::cout << " FAILED (0x" << std::hex << hr << std::dec << ")" << std::endl;
                        }
                    }
                }
                #else
                std::cout << "  -> USE_DIRECTXTK not defined, using white texture" << std::endl;
                #endif
            }
        }
        else
        {
            std::cout << "No texture" << std::endl;
        }

        // Fallback to white
        if (!m_->materialSRVs[m])
        {
            m_->materialSRVs[m] = m_->fallbackBaseTexture;
            std::cout << "  -> Using fallback base texture" << std::endl;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "=== LoadMaterials Complete (took " << duration.count() << "ms) ===" << std::endl;
    return true;
}

// Helper: Load materials with multiple default textures (one per material)
bool FbxManager::LoadMaterials(ZGraphics& gfx, const aiScene* scene, const std::string& baseDir, const std::vector<std::string>& defaultTexturePaths)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    std::cout << "=== LoadMaterials (Multiple Defaults) ===" << std::endl;
    std::cout << "Loading " << scene->mNumMaterials << " materials..." << std::endl;
    std::cout << "Default textures provided: " << defaultTexturePaths.size() << std::endl;
    
    // DEBUG: Print detailed texture mapping info
    for (size_t i = 0; i < defaultTexturePaths.size(); ++i)
    {
        std::cout << "  Texture[" << i << "]: " << defaultTexturePaths[i] << std::endl;
    }

    // Create white fallback texture
    if (!m_->fallbackBaseTexture)
    {
        UINT white = 0xFFFFFFFF;
        D3D11_TEXTURE2D_DESC td{};
        td.Width = 1;
        td.Height = 1;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_IMMUTABLE;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA sd{};
        sd.pSysMem = &white;
        sd.SysMemPitch = sizeof(UINT);

        ComPtr<ID3D11Texture2D> tex;
        HRESULT hr = gfx.GetDeviceCOM()->CreateTexture2D(&td, &sd, &tex);
        if (SUCCEEDED(hr))
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
            srvd.Format = td.Format;
            srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvd.Texture2D.MipLevels = 1;
            srvd.Texture2D.MostDetailedMip = 0;

            gfx.GetDeviceCOM()->CreateShaderResourceView(tex.Get(), &srvd, &m_->fallbackBaseTexture);
        }
    }

    // Resize material SRVs
    m_->materialSRVs.resize(scene->mNumMaterials);

    // Load textures for each material
    for (unsigned int m = 0; m < scene->mNumMaterials; ++m)
    {
        aiMaterial* mat = scene->mMaterials[m];
        aiString texPath;

        std::cout << "Material [" << m << "]: ";

        // Get default texture for this material index
        std::string defaultTexForMaterial;
        if (m < defaultTexturePaths.size() && !defaultTexturePaths[m].empty())
        {
            defaultTexForMaterial = defaultTexturePaths[m];
            std::cout << "(Default: " << defaultTexForMaterial << ") ";
        }

        if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
        {
            std::string texPathStr = texPath.C_Str();
            std::cout << "Original texture path: " << texPathStr << std::endl;

            if (!texPathStr.empty())
            {
                // Extract filename only
                std::string filename = texPathStr;
                size_t lastSlash = filename.find_last_of("/\\");
                if (lastSlash != std::string::npos)
                {
                    filename = filename.substr(lastSlash + 1);
                }
                
                std::cout << "Extracted filename: " << filename << std::endl;
                
                // Check cache first
                auto cacheIt = m_->textureCache.find(texPathStr);
                if (cacheIt != m_->textureCache.end())
                {
                    m_->materialSRVs[m] = cacheIt->second;
                    std::cout << "  -> From cache" << std::endl;
                    continue;
                }

                // Try multiple possible paths
                std::vector<std::string> possiblePaths = {
                    baseDir + texPathStr,
                    baseDir + filename,
                    baseDir + "../" + filename
                };
                
                #ifdef USE_DIRECTXTK
                ComPtr<ID3D11Resource> res;
                ID3D11ShaderResourceView* srv = nullptr;
                HRESULT hr = E_FAIL;
                std::string successPath;
                
                // Try each possible path
                for (const auto& tryPath : possiblePaths)
                {
                    std::wstring wPath(tryPath.begin(), tryPath.end());
                    std::cout << "  -> Trying: " << tryPath << std::flush;
                    
                    hr = DirectX::CreateWICTextureFromFile(
                        gfx.GetDeviceCOM(),
                        wPath.c_str(),
                        res.GetAddressOf(),
                        &srv);
                    
                    if (SUCCEEDED(hr))
                    {
                        successPath = tryPath;
                        std::cout << " SUCCESS" << std::endl;
                        break;
                    }
                    else
                    {
                        std::cout << " FAILED" << std::endl;
                    }
                }
                
                if (SUCCEEDED(hr))
                {
                    m_->materialSRVs[m].Attach(srv);
                    m_->textureCache[texPathStr] = m_->materialSRVs[m];
                }
                else if (!defaultTexForMaterial.empty())
                {
                    // Try material-specific default texture
                    std::cout << "  -> Trying material default: " << defaultTexForMaterial << std::flush;
                    std::wstring wDefaultPath(defaultTexForMaterial.begin(), defaultTexForMaterial.end());
                    
                    hr = DirectX::CreateWICTextureFromFile(
                        gfx.GetDeviceCOM(),
                        wDefaultPath.c_str(),
                        res.GetAddressOf(),
                        &srv);
                    
                    if (SUCCEEDED(hr))
                    {
                        m_->materialSRVs[m].Attach(srv);
                        std::cout << " SUCCESS" << std::endl;
                    }
                    else
                    {
                        std::cout << " FAILED" << std::endl;
                    }
                }
                #endif
            }
        }
        else if (!defaultTexForMaterial.empty())
        {
            // No embedded texture, use default for this material
            std::cout << "No embedded texture, using default: " << defaultTexForMaterial << std::flush;
            
            #ifdef USE_DIRECTXTK
            ComPtr<ID3D11Resource> res;
            ID3D11ShaderResourceView* srv = nullptr;
            std::wstring wDefaultPath(defaultTexForMaterial.begin(), defaultTexForMaterial.end());
            
            HRESULT hr = DirectX::CreateWICTextureFromFile(
                gfx.GetDeviceCOM(),
                wDefaultPath.c_str(),
                res.GetAddressOf(),
                &srv);
            
            if (SUCCEEDED(hr))
            {
                m_->materialSRVs[m].Attach(srv);
                std::cout << " SUCCESS" << std::endl;
            }
            else
            {
                std::cout << " FAILED" << std::endl;
            }
            #endif
        }
        else
        {
            std::cout << "No texture" << std::endl;
        }

        // Fallback to white
        if (!m_->materialSRVs[m])
        {
            m_->materialSRVs[m] = m_->fallbackBaseTexture;
            std::cout << "  -> Using fallback base texture" << std::endl;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "=== LoadMaterials Complete (took " << duration.count() << "ms) ===" << std::endl;
    return true;
}

// Helper: Load materials with multiple default textures and normal maps (one per material)
bool FbxManager::LoadMaterials(ZGraphics& gfx, const aiScene* scene, const std::string& baseDir, const std::vector<std::string>& defaultTexturePaths, const std::vector<std::string>& defaultNormalMapPaths)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    std::cout << "=== LoadMaterials (Multiple Defaults with Normal Maps) ===" << std::endl;
    std::cout << "Loading " << scene->mNumMaterials << " materials..." << std::endl;
    std::cout << "Default textures provided: " << defaultTexturePaths.size() << std::endl;
    std::cout << "Default normal maps provided: " << defaultNormalMapPaths.size() << std::endl;
    
    // DEBUG: Print detailed texture mapping info
    for (size_t i = 0; i < defaultTexturePaths.size(); ++i)
    {
        std::cout << "  Texture[" << i << "]: " << defaultTexturePaths[i] << std::endl;
    }
    for (size_t i = 0; i < defaultNormalMapPaths.size(); ++i)
    {
        std::cout << "  NormalMap[" << i << "]: " << defaultNormalMapPaths[i] << std::endl;
    }

    if (!scene || !scene->mNumMaterials)
    {
        std::cout << "No materials to load" << std::endl;
        return true;
    }

    // Clear existing materials
    m_->materialSRVs.clear();
    m_->normalMapSRVs.clear();

    m_->materialSRVs.reserve(scene->mNumMaterials);
    m_->normalMapSRVs.reserve(scene->mNumMaterials);

    for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
    {
        aiMaterial* material = scene->mMaterials[i];
        std::cout << "Processing material " << i << "..." << std::endl;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureSRV = nullptr;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalMapSRV = nullptr;

        // Try to load diffuse texture from the material
        aiString texturePath;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
        {
            std::string fullPath = baseDir + texturePath.C_Str();
            std::cout << "  Found material diffuse texture: " << fullPath << std::endl;
            
            try
            {
                std::wstring wFullPath(fullPath.begin(), fullPath.end());
                Microsoft::WRL::ComPtr<ID3D11Resource> res;
                ID3D11ShaderResourceView* srv = nullptr;
                
                HRESULT hr = DirectX::CreateWICTextureFromFile(
                    gfx.GetDeviceCOM(),
                    wFullPath.c_str(),
                    res.GetAddressOf(),
                    &srv);
                    
                if (SUCCEEDED(hr))
                {
                    textureSRV.Attach(srv);
                    std::cout << "  Loaded material diffuse texture successfully" << std::endl;
                }
                else
                {
                    std::cout << "  Failed to load material diffuse texture (0x" << std::hex << hr << std::dec << ")" << std::endl;
                }
            }
            catch (const std::exception& e)
            {
                std::cout << "  Failed to load material diffuse texture: " << e.what() << std::endl;
            }
        }

        // Try to load normal map from the material
        aiString normalPath;
        if (material->GetTexture(aiTextureType_NORMALS, 0, &normalPath) == AI_SUCCESS)
        {
            std::string fullNormalPath = baseDir + normalPath.C_Str();
            std::cout << "  Found material normal map: " << fullNormalPath << std::endl;
            
            try
            {
                std::wstring wFullNormalPath(fullNormalPath.begin(), fullNormalPath.end());
                Microsoft::WRL::ComPtr<ID3D11Resource> res;
                ID3D11ShaderResourceView* srv = nullptr;
                
                HRESULT hr = DirectX::CreateWICTextureFromFile(
                    gfx.GetDeviceCOM(),
                    wFullNormalPath.c_str(),
                    res.GetAddressOf(),
                    &srv);
                    
                if (SUCCEEDED(hr))
                {
                    normalMapSRV.Attach(srv);
                    std::cout << "  Loaded material normal map successfully" << std::endl;
                }
                else
                {
                    std::cout << "  Failed to load material normal map (0x" << std::hex << hr << std::dec << ")" << std::endl;
                }
            }
            catch (const std::exception& e)
            {
                std::cout << "  Failed to load material normal map: " << e.what() << std::endl;
            }
        }

        // If no material texture found, use default texture if available
        if (!textureSRV && i < defaultTexturePaths.size() && !defaultTexturePaths[i].empty())
        {
            std::cout << "  Using default texture: " << defaultTexturePaths[i] << std::endl;
            try
            {
                std::wstring wDefaultTexturePath(defaultTexturePaths[i].begin(), defaultTexturePaths[i].end());
                Microsoft::WRL::ComPtr<ID3D11Resource> res;
                ID3D11ShaderResourceView* srv = nullptr;
                
                HRESULT hr = DirectX::CreateWICTextureFromFile(
                    gfx.GetDeviceCOM(),
                    wDefaultTexturePath.c_str(),
                    res.GetAddressOf(),
                    &srv);
                    
                if (SUCCEEDED(hr))
                {
                    textureSRV.Attach(srv);
                    std::cout << "  Loaded default texture successfully" << std::endl;
                }
                else
                {
                    std::cout << "  Failed to load default texture (0x" << std::hex << hr << std::dec << ")" << std::endl;
                }
            }
            catch (const std::exception& e)
            {
                std::cout << "  Failed to load default texture: " << e.what() << std::endl;
            }
        }

        // If no material normal map found, use default normal map if available
        if (!normalMapSRV && i < defaultNormalMapPaths.size() && !defaultNormalMapPaths[i].empty())
        {
            std::cout << "  Using default normal map: " << defaultNormalMapPaths[i] << std::endl;
            try
            {
                std::wstring wDefaultNormalPath(defaultNormalMapPaths[i].begin(), defaultNormalMapPaths[i].end());
                Microsoft::WRL::ComPtr<ID3D11Resource> res;
                ID3D11ShaderResourceView* srv = nullptr;
                
                HRESULT hr = DirectX::CreateWICTextureFromFile(
                    gfx.GetDeviceCOM(),
                    wDefaultNormalPath.c_str(),
                    res.GetAddressOf(),
                    &srv);
                    
                if (SUCCEEDED(hr))
                {
                    normalMapSRV.Attach(srv);
                    std::cout << "  Loaded default normal map successfully" << std::endl;
                }
                else
                {
                    std::cout << "  Failed to load default normal map (0x" << std::hex << hr << std::dec << ")" << std::endl;
                }
            }
            catch (const std::exception& e)
            {
                std::cout << "  Failed to load default normal map: " << e.what() << std::endl;
            }
        }

        // If still no texture, try to load a default white texture
        if (!textureSRV)
        {
            std::cout << "  Using fallback white texture" << std::endl;
            try
            {
                std::wstring wWhitePath(L"Data/Images/white.png");
                Microsoft::WRL::ComPtr<ID3D11Resource> res;
                ID3D11ShaderResourceView* srv = nullptr;
                
                HRESULT hr = DirectX::CreateWICTextureFromFile(
                    gfx.GetDeviceCOM(),
                    wWhitePath.c_str(),
                    res.GetAddressOf(),
                    &srv);
                    
                if (SUCCEEDED(hr))
                {
                    textureSRV.Attach(srv);
                }
                else
                {
                    std::cout << "  Failed to load fallback white texture (0x" << std::hex << hr << std::dec << ")" << std::endl;
                }
            }
            catch (...)
            {
                std::cout << "  Failed to load fallback white texture" << std::endl;
            }
        }

        m_->materialSRVs.push_back(textureSRV);
        m_->normalMapSRVs.push_back(normalMapSRV);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "=== LoadMaterials Complete (took " << duration.count() << "ms) ===" << std::endl;
    return true;
}

// Helper: Load materials with multiple default textures and normal maps (one per material)
bool FbxManager::LoadMaterials(ZGraphics& gfx, const aiScene* scene, const std::string& baseDir, const std::vector<std::string>& defaultTexturePaths, const std::vector<std::string>& defaultNormalMapPaths, const std::vector<std::string>& defaultSpecularMapPaths)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    std::cout << "=== LoadMaterials (Multiple Defaults with Normal Maps) ===" << std::endl;
    std::cout << "Loading " << scene->mNumMaterials << " materials..." << std::endl;
    std::cout << "Default textures provided: " << defaultTexturePaths.size() << std::endl;
    std::cout << "Default normal maps provided: " << defaultNormalMapPaths.size() << std::endl;
    std::cout << "Default specular maps provided: " << defaultSpecularMapPaths.size() << std::endl;
    
    // DEBUG: Print detailed texture mapping info
    for (size_t i = 0; i < defaultTexturePaths.size(); ++i)
    {
        std::cout << "  Texture[" << i << "]: " << defaultTexturePaths[i] << std::endl;
    }
    for (size_t i = 0; i < defaultNormalMapPaths.size(); ++i)
    {
        std::cout << "  NormalMap[" << i << "]: " << defaultNormalMapPaths[i] << std::endl;
    }
    for (size_t i = 0; i < defaultSpecularMapPaths.size(); ++i)
    {
        std::cout << "  SpecularMap[" << i << "]: " << defaultSpecularMapPaths[i] << std::endl;
    }

    // Create white fallback texture (general purpose for any missing texture)
    ComPtr<ID3D11ShaderResourceView> fallbackBaseTexture;
    {
        UINT white = 0xFFFFFFFF;
        D3D11_TEXTURE2D_DESC td{};
        td.Width = 1;
        td.Height = 1;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_IMMUTABLE;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA sd{};
        sd.pSysMem = &white;
        sd.SysMemPitch = sizeof(UINT);

        ComPtr<ID3D11Texture2D> tex;
        HRESULT hr = gfx.GetDeviceCOM()->CreateTexture2D(&td, &sd, &tex);
        if (SUCCEEDED(hr))
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
            srvd.Format = td.Format;
            srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvd.Texture2D.MipLevels = 1;
            srvd.Texture2D.MostDetailedMip = 0;

            gfx.GetDeviceCOM()->CreateShaderResourceView(tex.Get(), &srvd, &fallbackBaseTexture);
        }
    }

    // Create fallback base normal map (flat normal: RGB = 128, 128, 255)
    ComPtr<ID3D11ShaderResourceView> fallbackBaseNormalMap;
    {
        UINT neutralNormal = 0xFF8080FF;  // B8G8R8A8 format: (255, 128, 128, 255) = normal pointing up
        // 탄젠트 스페이스 노멀맵 설명 (B8G8R8A8 포맷 기준):
        // - B(255) = 노멀(N) 성분: 255 → 1.0 (최대값, 위 방향을 가리킴)
        // - G(128) = 바이탄젠트(V) 성분: 128 → 0.0 (중간값, 탄젠트 스페이스에서 0)
        // - R(128) = 탄젠트(U) 성분: 128 → 0.0 (중간값, 탄젠트 스페이스에서 0)
        // - A(255) = 알파: 1.0 (불투명)
        // 
        // 탄젠트 스페이스 좌표계: RGB [0,255] → XYZ [-1,1] 매핑
        // - 0 → -1.0, 128 → 0.0, 255 → 1.0
        // - 따라서 (128,128,255)는 (0,0,1) = 표면 법선 방향(위)을 의미
        // - 이는 평평한 표면의 기본 노멀 벡터로, 노멀맵이 없을 때의 안전한 기본값
        D3D11_TEXTURE2D_DESC td{};
        td.Width = 1;
        td.Height = 1;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_IMMUTABLE;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA sd{};
        sd.pSysMem = &neutralNormal;
        sd.SysMemPitch = sizeof(UINT);

        ComPtr<ID3D11Texture2D> tex;
        HRESULT hr = gfx.GetDeviceCOM()->CreateTexture2D(&td, &sd, &tex);
        if (SUCCEEDED(hr))
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
            srvd.Format = td.Format;
            srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvd.Texture2D.MipLevels = 1;
            srvd.Texture2D.MostDetailedMip = 0;

            gfx.GetDeviceCOM()->CreateShaderResourceView(tex.Get(), &srvd, &fallbackBaseNormalMap);
        }
    }

    // Create fallback base specular map (white for full specular response)
    ComPtr<ID3D11ShaderResourceView> fallbackBaseSpecularMap;
    {
        UINT white = 0xFFFFFFFF;  // 완전한 반사광 응답
        D3D11_TEXTURE2D_DESC td{};
        td.Width = 1;
        td.Height = 1;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_IMMUTABLE;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA sd{};
        sd.pSysMem = &white;
        sd.SysMemPitch = sizeof(UINT);

        ComPtr<ID3D11Texture2D> tex;
        HRESULT hr = gfx.GetDeviceCOM()->CreateTexture2D(&td, &sd, &tex);
        if (SUCCEEDED(hr))
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
            srvd.Format = td.Format;
            srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvd.Texture2D.MipLevels = 1;
            srvd.Texture2D.MostDetailedMip = 0;

            gfx.GetDeviceCOM()->CreateShaderResourceView(tex.Get(), &srvd, &fallbackBaseSpecularMap);
        }
    }

    // Resize material SRVs and normal map SRVs
    m_->materialSRVs.resize(scene->mNumMaterials);
    m_->normalMapSRVs.resize(scene->mNumMaterials);
    m_->specularMapSRVs.resize(scene->mNumMaterials);

    // Load textures for each material
    for (unsigned int m = 0; m < scene->mNumMaterials; ++m)
    {
        std::cout << "Material " << m << ":" << std::endl;

        // Load diffuse texture
        if (m < defaultTexturePaths.size() && !defaultTexturePaths[m].empty())
        {
            std::string texPath = defaultTexturePaths[m];
            std::string fullTexPath;
            
            // Check if texPath is already a full path (starts with "./" or contains drive letter)
            if (texPath.find("./") == 0 || texPath.find(":") != std::string::npos)
            {
                fullTexPath = texPath;  // Already a full path
            }
            else
            {
                fullTexPath = baseDir + texPath;  // Relative path, add baseDir
            }

            std::cout << "  Loading diffuse texture: " << fullTexPath << std::endl;

            // Check cache first
            auto cacheIt = m_->textureCache.find(fullTexPath);
            if (cacheIt != m_->textureCache.end())
            {
                m_->materialSRVs[m] = cacheIt->second;
                std::cout << "    -> Found in cache" << std::endl;
            }
            else
            {
                // Load texture
                std::wstring widePath(fullTexPath.begin(), fullTexPath.end());
                Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
                HRESULT hr = DirectX::CreateWICTextureFromFile(gfx.GetDeviceCOM(), gfx.GetDeviceContext(), widePath.c_str(), nullptr, &srv);

                if (SUCCEEDED(hr))
                {
                    m_->materialSRVs[m] = srv;
                    m_->textureCache[fullTexPath] = m_->materialSRVs[m];
                    std::cout << "    -> Loaded successfully" << std::endl;
                }
                else
                {
                    std::cout << "    -> Failed to load (HRESULT: 0x" << std::hex << hr << ")" << std::endl;
                }
            }
        }
        else
        {
            std::cout << "  No default diffuse texture provided" << std::endl;
        }

        // Load normal map
        if (m < defaultNormalMapPaths.size() && !defaultNormalMapPaths[m].empty())
        {
            std::string normalPath = defaultNormalMapPaths[m];
            std::string fullNormalPath;
            
            // Check if normalPath is already a full path (starts with "./" or contains drive letter)
            if (normalPath.find("./") == 0 || normalPath.find(":") != std::string::npos)
            {
                fullNormalPath = normalPath;  // Already a full path
            }
            else
            {
                fullNormalPath = baseDir + normalPath;  // Relative path, add baseDir
            }

            std::cout << "  Loading normal map: " << fullNormalPath << std::endl;

            // Check cache first
            auto cacheIt = m_->textureCache.find(fullNormalPath);
            if (cacheIt != m_->textureCache.end())
            {
                m_->normalMapSRVs[m] = cacheIt->second;
                std::cout << "    -> Found in cache" << std::endl;
            }
            else
            {
                // Load normal map
                std::wstring widePath(fullNormalPath.begin(), fullNormalPath.end());
                Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
                HRESULT hr = DirectX::CreateWICTextureFromFile(gfx.GetDeviceCOM(), gfx.GetDeviceContext(), widePath.c_str(), nullptr, &srv);

                if (SUCCEEDED(hr))
                {
                    m_->normalMapSRVs[m] = srv;
                    m_->textureCache[fullNormalPath] = m_->normalMapSRVs[m];
                    std::cout << "    -> Loaded successfully" << std::endl;
                }
                else
                {
                    std::cout << "    -> Failed to load normal map (HRESULT: 0x" << std::hex << hr << ")" << std::endl;
                }
            }
        }
        else
        {
            std::cout << "  No default normal map provided" << std::endl;
        }

        // Load specular map
        if (m < defaultSpecularMapPaths.size() && !defaultSpecularMapPaths[m].empty())
        {
            std::string specularPath = defaultSpecularMapPaths[m];
            std::string fullSpecularPath;
            
            // Check if specularPath is already a full path (starts with "./" or contains drive letter)
            if (specularPath.find("./") == 0 || specularPath.find(":") != std::string::npos)
            {
                fullSpecularPath = specularPath;  // Already a full path
            }
            else
            {
                fullSpecularPath = baseDir + specularPath;  // Relative path, add baseDir
            }

            std::cout << "  Loading specular map: " << fullSpecularPath << std::endl;

            // Check cache first
            auto cacheIt = m_->textureCache.find(fullSpecularPath);
            if (cacheIt != m_->textureCache.end())
            {
                m_->specularMapSRVs[m] = cacheIt->second;
                std::cout << "    -> Loaded from cache" << std::endl;
            }
            else
            {
                // Load from file
                std::wstring wideSpecularPath(fullSpecularPath.begin(), fullSpecularPath.end());
                Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
                HRESULT hr = DirectX::CreateWICTextureFromFile(gfx.GetDeviceCOM(), gfx.GetDeviceContext(), wideSpecularPath.c_str(), nullptr, &srv);

                if (SUCCEEDED(hr))
                {
                    m_->specularMapSRVs[m] = srv;
                    m_->textureCache[fullSpecularPath] = m_->specularMapSRVs[m];
                    std::cout << "    -> Loaded successfully" << std::endl;
                }
                else
                {
                    std::cout << "    -> Failed to load (HRESULT: 0x" << std::hex << hr << ")" << std::endl;
                }
            }
        }
        else
        {
            std::cout << "  No default specular map provided" << std::endl;
        }

        // Fallback to white texture for diffuse
        if (!m_->materialSRVs[m])
        {
            m_->materialSRVs[m] = fallbackBaseTexture;
            std::cout << "  -> Using fallback base texture for diffuse" << std::endl;
        }

        // Fallback to base normal map
        if (!m_->normalMapSRVs[m])
        {
            m_->normalMapSRVs[m] = fallbackBaseNormalMap;
            std::cout << "  -> Using fallback base normal map" << std::endl;
        }

        // Fallback to base specular map
        if (!m_->specularMapSRVs[m])
        {
            m_->specularMapSRVs[m] = fallbackBaseSpecularMap;
            std::cout << "  -> Using fallback base specular map" << std::endl;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "=== LoadMaterials Complete (took " << duration.count() << "ms) ===" << std::endl;
    return true;
}

// Helper: Build vertex and index buffers
bool FbxManager::BuildMeshBuffers(ZGraphics& gfx, const aiScene* scene)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    std::cout << "=== BuildMeshBuffers ===" << std::endl;
    std::cout << "Processing " << scene->mNumMeshes << " meshes..." << std::endl;
    std::vector<VertexSkinned> vertices;
    std::vector<uint32_t> indices;

    m_->subsets.clear();

    // Process all meshes
    for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi)
    {
        const aiMesh* mesh = scene->mMeshes[mi];

        FbxSubset subset;
        subset.startIndex = static_cast<uint32_t>(indices.size());
        subset.materialIndex = mesh->mMaterialIndex;

        uint32_t baseVertex = static_cast<uint32_t>(vertices.size());

        // Build bone weight map for this mesh
        std::vector<std::vector<std::pair<int, float>>> vertexBoneWeights(mesh->mNumVertices);
        
        for (unsigned int bi = 0; bi < mesh->mNumBones; ++bi)
        {
            const aiBone* bone = mesh->mBones[bi];
            std::string boneName = bone->mName.C_Str();
            
            auto it = m_->boneIndexOfName.find(boneName);
            if (it == m_->boneIndexOfName.end()) continue;
            
            int boneIdx = it->second;
            
            for (unsigned int wi = 0; wi < bone->mNumWeights; ++wi)
            {
                unsigned int vertexId = bone->mWeights[wi].mVertexId;
                float weight = bone->mWeights[wi].mWeight;
                
                if (vertexId < mesh->mNumVertices)
                {
                    vertexBoneWeights[vertexId].push_back({boneIdx, weight});
                }
            }
        }
        
        // Load vertices
        for (unsigned int vi = 0; vi < mesh->mNumVertices; ++vi)
        {
            VertexSkinned v{};

            // Position
            v.position = XMFLOAT3{
                mesh->mVertices[vi].x,
                mesh->mVertices[vi].y,
                mesh->mVertices[vi].z
            };

            // Normal
            if (mesh->HasNormals())
            {
                v.normal = XMFLOAT3{
                    mesh->mNormals[vi].x,
                    mesh->mNormals[vi].y,
                    mesh->mNormals[vi].z
                };
            }
            else
            {
                v.normal = XMFLOAT3{ 0.0f, 1.0f, 0.0f };
            }

            // Tangent and Bitangent
            if (mesh->HasTangentsAndBitangents())
            {
                v.tangent = XMFLOAT3{
                    mesh->mTangents[vi].x,
                    mesh->mTangents[vi].y,
                    mesh->mTangents[vi].z
                };

                v.bitangent = XMFLOAT3{
                    mesh->mBitangents[vi].x,
                    mesh->mBitangents[vi].y,
                    mesh->mBitangents[vi].z
                };
            }
            else
            {
                v.tangent = XMFLOAT3{ 1.0f, 0.0f, 0.0f };
                v.bitangent = XMFLOAT3{ 0.0f, 0.0f, 1.0f };
            }

            // TexCoord
            if (mesh->HasTextureCoords(0))
            {
                v.texCoord = XMFLOAT2{
                    mesh->mTextureCoords[0][vi].x,
                    mesh->mTextureCoords[0][vi].y
                };
            }
            else
            {
                v.texCoord = XMFLOAT2{ 0.0f, 0.0f };
            }

            // Color (default white)
            v.color = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
            
            // Bone indices and weights
            v.boneIndices[0] = v.boneIndices[1] = v.boneIndices[2] = v.boneIndices[3] = 0;
            v.boneWeights = XMFLOAT4{ 0.0f, 0.0f, 0.0f, 0.0f };
            
            auto& boneWeights = vertexBoneWeights[vi];
            if (!boneWeights.empty())
            {
                // Sort by weight descending
                std::sort(boneWeights.begin(), boneWeights.end(),
                    [](const auto& a, const auto& b) { return a.second > b.second; });
                
                // Take top 4 weights
                float totalWeight = 0.0f;
                for (size_t i = 0; i < (std::min)(boneWeights.size(), size_t(4)); ++i)
                {
                    v.boneIndices[i] = boneWeights[i].first;
                    reinterpret_cast<float*>(&v.boneWeights)[i] = boneWeights[i].second;
                    totalWeight += boneWeights[i].second;
                }
                
                // Normalize weights
                if (totalWeight > 0.0f)
                {
                    v.boneWeights.x /= totalWeight;
                    v.boneWeights.y /= totalWeight;
                    v.boneWeights.z /= totalWeight;
                    v.boneWeights.w /= totalWeight;
                }
            }
            else
            {
                // No bone weights - use identity (bone 0, weight 1.0)
                v.boneIndices[0] = 0;
                v.boneWeights.x = 1.0f;
            }

            vertices.push_back(v);
        }

        // Load indices
        for (unsigned int fi = 0; fi < mesh->mNumFaces; ++fi)
        {
            const aiFace& face = mesh->mFaces[fi];
            for (unsigned int ii = 0; ii < face.mNumIndices; ++ii)
            {
                indices.push_back(baseVertex + face.mIndices[ii]);
            }
        }

        subset.indexCount = static_cast<uint32_t>(indices.size()) - subset.startIndex;
        m_->subsets.push_back(subset);

        std::cout << "Subset [" << mi << "]: vertices=" << mesh->mNumVertices
                  << ", indices=" << subset.indexCount
                  << ", material=" << subset.materialIndex << std::endl;
    }

    // Debug: Print vertex structure size
    std::cout << "sizeof(VertexSkinned) = " << sizeof(VertexSkinned) << " bytes" << std::endl;

    // Create vertex buffer
    D3D11_BUFFER_DESC vbd{};
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(VertexSkinned));

    D3D11_SUBRESOURCE_DATA vbData{};
    vbData.pSysMem = vertices.data();

    HRESULT hr = gfx.GetDeviceCOM()->CreateBuffer(&vbd, &vbData, &m_->pVertexBuffer);
    if (FAILED(hr))
    {
        std::cerr << "Failed to create vertex buffer" << std::endl;
        return false;
    }

    // Create index buffer
    D3D11_BUFFER_DESC ibd{};
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = static_cast<UINT>(indices.size() * sizeof(uint32_t));

    D3D11_SUBRESOURCE_DATA ibData{};
    ibData.pSysMem = indices.data();

    hr = gfx.GetDeviceCOM()->CreateBuffer(&ibd, &ibData, &m_->pIndexBuffer);
    if (FAILED(hr))
    {
        std::cerr << "Failed to create index buffer" << std::endl;
        return false;
    }

    m_->indexCount = static_cast<int>(indices.size());

    // Calculate bounding box
    if (!vertices.empty())
    {
        XMFLOAT3 minBounds = vertices[0].position;
        XMFLOAT3 maxBounds = vertices[0].position;
        
        for (const auto& v : vertices)
        {
            minBounds.x = (std::min)(minBounds.x, v.position.x);
            minBounds.y = (std::min)(minBounds.y, v.position.y);
            minBounds.z = (std::min)(minBounds.z, v.position.z);
            maxBounds.x = (std::max)(maxBounds.x, v.position.x);
            maxBounds.y = (std::max)(maxBounds.y, v.position.y);
            maxBounds.z = (std::max)(maxBounds.z, v.position.z);
        }
        
        XMFLOAT3 size = {
            maxBounds.x - minBounds.x,
            maxBounds.y - minBounds.y,
            maxBounds.z - minBounds.z
        };
        
        std::cout << "Bounding Box:" << std::endl;
        std::cout << "  Min: (" << minBounds.x << ", " << minBounds.y << ", " << minBounds.z << ")" << std::endl;
        std::cout << "  Max: (" << maxBounds.x << ", " << maxBounds.y << ", " << maxBounds.z << ")" << std::endl;
        std::cout << "  Size: (" << size.x << ", " << size.y << ", " << size.z << ")" << std::endl;
    }
    
    std::cout << "Total vertices: " << vertices.size() << std::endl;
    std::cout << "Total indices: " << indices.size() << std::endl;
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "=== BuildMeshBuffers Complete (took " << duration.count() << "ms) ===" << std::endl;

    return true;
}

// Animation control methods
int FbxManager::GetCurrentAnimationIndex() const
{
    return m_->currentClip;
}

void FbxManager::SetCurrentAnimation(int idx)
{
    if (!m_->hasAnimations) return;
    if (idx < 0 || idx >= static_cast<int>(m_->animationNames.size())) return;
    
    m_->currentClip = idx;
    m_->clipTimeSec = 0.0;
    
    std::cout << "[FbxManager] Animation set to: " << m_->animationNames[idx] << std::endl;
}

void FbxManager::SetAnimationPlaying(bool playing)
{
    m_->playing = playing;
    std::cout << "[FbxManager] Animation " << (playing ? "playing" : "paused") << std::endl;
}

bool FbxManager::IsAnimationPlaying() const
{
    return m_->playing;
}

double FbxManager::GetAnimationTimeSeconds() const
{
    return m_->clipTimeSec;
}

void FbxManager::SetAnimationTimeSeconds(double t)
{
    if (!m_->hasAnimations || m_->currentClip < 0)
    {
        m_->clipTimeSec = 0.0;
        return;
    }
    
    double dur = m_->clipDurationSec[m_->currentClip];
    if (dur <= 0.0)
    {
        m_->clipTimeSec = 0.0;
        return;
    }
    
    // Loop animation
    while (t < 0.0) t += dur;
    while (t >= dur) t -= dur;
    
    m_->clipTimeSec = t;
}

double FbxManager::GetClipDurationSec(int idx) const
{
    if (idx >= 0 && idx < static_cast<int>(m_->clipDurationSec.size()))
        return m_->clipDurationSec[idx];
    return 0.0;
}

double FbxManager::GetAnimationDuration(int idx) const
{
    return GetClipDurationSec(idx);
}

bool FbxManager::HasSkinning() const
{
    return m_->hasSkinning;
}

int FbxManager::GetBoneCount() const
{
    return static_cast<int>(m_->boneOffsets.size());
}

// Helper: Create bone constant buffer
void CreateBoneConstantBuffer(ZGraphics& gfx, ID3D11Buffer** ppBuffer, int maxBones)
{
    if (!ppBuffer || *ppBuffer) return;
    
    // Bone CB structure: array of matrices + bone count (matching shader)
    struct BoneCB
    {
        XMFLOAT4X4 boneMatrices[1023];  // MAX_BONES from shader
        unsigned int boneCount;
        float padding[3];
    };
    
    D3D11_BUFFER_DESC cbd{};
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbd.ByteWidth = sizeof(BoneCB);
    
    HRESULT hr = gfx.GetDeviceCOM()->CreateBuffer(&cbd, nullptr, ppBuffer);
    if (SUCCEEDED(hr))
    {
        std::cout << "[FbxManager] Bone constant buffer created (size: " << sizeof(BoneCB) << " bytes)" << std::endl;
    }
    else
    {
        std::cerr << "[FbxManager] Failed to create bone constant buffer!" << std::endl;
    }
}

// Helper: Upload bone palette to GPU
void UploadBonePalette(ZGraphics& gfx, ID3D11Buffer* pBoneCB, const std::vector<XMMATRIX>& palette)
{
    if (!pBoneCB) return;
    
    struct BoneCB
    {
        XMFLOAT4X4 boneMatrices[1023];
        unsigned int boneCount;
        float padding[3];
    };
    
    BoneCB cb{};
    
    // Initialize all matrices to identity
    XMMATRIX identity = XMMatrixIdentity();
    for (int i = 0; i < 1023; ++i)
    {
        XMStoreFloat4x4(&cb.boneMatrices[i], XMMatrixTranspose(identity));
    }
    
    // Copy bone palette (transposed for HLSL)
    size_t numBones = (std::min)(palette.size(), size_t(1023));
    for (size_t i = 0; i < numBones; ++i)
    {
        XMMATRIX transposed = XMMatrixTranspose(palette[i]);
        XMStoreFloat4x4(&cb.boneMatrices[i], transposed);
    }
    
    cb.boneCount = static_cast<unsigned int>(numBones);
    
    // Map and upload
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = gfx.GetDeviceContext()->Map(pBoneCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        memcpy(mapped.pData, &cb, sizeof(BoneCB));
        gfx.GetDeviceContext()->Unmap(pBoneCB, 0);
    }
}

// Helper: Interpolate position keyframes
static aiVector3D InterpolatePosition(double time, const aiNodeAnim* channel)
{
    if (channel->mNumPositionKeys == 1)
        return channel->mPositionKeys[0].mValue;
    
    // Find keyframe indices
    unsigned int frame = 0;
    for (unsigned int i = 0; i < channel->mNumPositionKeys - 1; ++i)
    {
        if (time < channel->mPositionKeys[i + 1].mTime)
        {
            frame = i;
            break;
        }
    }
    
    unsigned int nextFrame = (frame + 1) % channel->mNumPositionKeys;
    const aiVectorKey& key = channel->mPositionKeys[frame];
    const aiVectorKey& nextKey = channel->mPositionKeys[nextFrame];
    
    double deltaTime = nextKey.mTime - key.mTime;
    double factor = (time - key.mTime) / deltaTime;
    
    aiVector3D result;
    result.x = static_cast<float>(key.mValue.x + factor * (nextKey.mValue.x - key.mValue.x));
    result.y = static_cast<float>(key.mValue.y + factor * (nextKey.mValue.y - key.mValue.y));
    result.z = static_cast<float>(key.mValue.z + factor * (nextKey.mValue.z - key.mValue.z));
    return result;
}

// Helper: Interpolate rotation keyframes
static aiQuaternion InterpolateRotation(double time, const aiNodeAnim* channel)
{
    if (channel->mNumRotationKeys == 1)
        return channel->mRotationKeys[0].mValue;
    
    unsigned int frame = 0;
    for (unsigned int i = 0; i < channel->mNumRotationKeys - 1; ++i)
    {
        if (time < channel->mRotationKeys[i + 1].mTime)
        {
            frame = i;
            break;
        }
    }
    
    unsigned int nextFrame = (frame + 1) % channel->mNumRotationKeys;
    const aiQuatKey& key = channel->mRotationKeys[frame];
    const aiQuatKey& nextKey = channel->mRotationKeys[nextFrame];
    
    double deltaTime = nextKey.mTime - key.mTime;
    float factor = static_cast<float>((time - key.mTime) / deltaTime);
    
    aiQuaternion result;
    aiQuaternion::Interpolate(result, key.mValue, nextKey.mValue, factor);
    result.Normalize();
    return result;
}

// Helper: Interpolate scaling keyframes
static aiVector3D InterpolateScaling(double time, const aiNodeAnim* channel)
{
    if (channel->mNumScalingKeys == 1)
        return channel->mScalingKeys[0].mValue;
    
    unsigned int frame = 0;
    for (unsigned int i = 0; i < channel->mNumScalingKeys - 1; ++i)
    {
        if (time < channel->mScalingKeys[i + 1].mTime)
        {
            frame = i;
            break;
        }
    }
    
    unsigned int nextFrame = (frame + 1) % channel->mNumScalingKeys;
    const aiVectorKey& key = channel->mScalingKeys[frame];
    const aiVectorKey& nextKey = channel->mScalingKeys[nextFrame];
    
    double deltaTime = nextKey.mTime - key.mTime;
    double factor = (time - key.mTime) / deltaTime;
    
    aiVector3D result;
    result.x = static_cast<float>(key.mValue.x + factor * (nextKey.mValue.x - key.mValue.x));
    result.y = static_cast<float>(key.mValue.y + factor * (nextKey.mValue.y - key.mValue.y));
    result.z = static_cast<float>(key.mValue.z + factor * (nextKey.mValue.z - key.mValue.z));
    return result;
}

// Helper: Evaluate global matrices for all nodes
void EvaluateGlobalMatrices(const aiScene* scene, const aiNode* node, int nodeIdx,
                            const std::vector<const aiNodeAnim*>& channelOfNode,
                            std::vector<XMFLOAT4X4>& outGlobal,
                            const std::unordered_map<std::string, int>& nodeIndexOfName,
                            double animTime, XMMATRIX parentGlobal)
{
    // Get local transform
    aiMatrix4x4 localTransform = node->mTransformation;
    
    // Apply animation if channel exists
    if (nodeIdx >= 0 && nodeIdx < static_cast<int>(channelOfNode.size()) && channelOfNode[nodeIdx])
    {
        const aiNodeAnim* channel = channelOfNode[nodeIdx];
        
        // Interpolate position, rotation, scaling
        aiVector3D pos = InterpolatePosition(animTime, channel);
        aiQuaternion rot = InterpolateRotation(animTime, channel);
        aiVector3D scale = InterpolateScaling(animTime, channel);
        
        // Build local matrix from TRS
        aiMatrix4x4 matScale, matRot, matTrans;
        aiMatrix4x4::Scaling(scale, matScale);
        matRot = aiMatrix4x4(rot.GetMatrix());
        aiMatrix4x4::Translation(pos, matTrans);
        
        localTransform = matTrans * matRot * matScale;
    }
    
    // Convert to XMMATRIX
    XMFLOAT4X4 localF;
    localF._11 = localTransform.a1; localF._12 = localTransform.a2; localF._13 = localTransform.a3; localF._14 = localTransform.a4;
    localF._21 = localTransform.b1; localF._22 = localTransform.b2; localF._23 = localTransform.b3; localF._24 = localTransform.b4;
    localF._31 = localTransform.c1; localF._32 = localTransform.c2; localF._33 = localTransform.c3; localF._34 = localTransform.c4;
    localF._41 = localTransform.d1; localF._42 = localTransform.d2; localF._43 = localTransform.d3; localF._44 = localTransform.d4;
    
    XMMATRIX local = XMLoadFloat4x4(&localF);
    XMMATRIX global = XMMatrixMultiply(parentGlobal, local);
    
    // Store global matrix
    if (nodeIdx >= 0 && nodeIdx < static_cast<int>(outGlobal.size()))
    {
        XMStoreFloat4x4(&outGlobal[nodeIdx], global);
    }
    
    // Process children
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        auto it = nodeIndexOfName.find(node->mChildren[i]->mName.C_Str());
        int childIdx = (it != nodeIndexOfName.end()) ? it->second : -1;
        EvaluateGlobalMatrices(scene, node->mChildren[i], childIdx, channelOfNode, 
                              outGlobal, nodeIndexOfName, animTime, global);
    }
}

void FbxManager::UpdateAnimation(float deltaTime)
{
    // Update animation time if playing
    if (m_->playing && m_->hasAnimations && m_->currentClip >= 0)
    {
        SetAnimationTimeSeconds(m_->clipTimeSec + static_cast<double>(deltaTime));
        
        // Log animation time periodically (every 1 second)
        static double lastLogTime = 0.0;
        if (m_->clipTimeSec - lastLogTime >= 1.0)
        {
            std::cout << "[FbxManager] Animation time: " << m_->clipTimeSec 
                      << "s / " << m_->clipDurationSec[m_->currentClip] << "s" << std::endl;
            lastLogTime = m_->clipTimeSec;
        }
    }
    
    // If no skinning, nothing to update
    if (!m_->hasSkinning || !m_->hasAnimations || m_->currentClip < 0)
        return;
    
    // 1. Get current animation (from base or external)
    const aiAnimation* anim = nullptr;
    const aiScene* animScene = nullptr;
    
    if (m_->currentClip < m_->baseAnimationCount)
    {
        // Animation from base model
        anim = m_->scene->mAnimations[m_->currentClip];
        animScene = m_->scene;
    }
    else
    {
        // External animation
        int extIndex = m_->currentClip - m_->baseAnimationCount;
        
        if (extIndex >= 0 && extIndex < static_cast<int>(m_->externalAnimations.size()))
        {
            const auto& extAnim = m_->externalAnimations[extIndex];
            if (extAnim.scene && extAnim.scene->mNumAnimations > 0)
            {
                anim = extAnim.scene->mAnimations[0];
                animScene = extAnim.scene;
            }
        }
        else
        {
            std::cout << "[FbxManager] Invalid external animation index!" << std::endl;
        }
    }
    
    if (!anim || !animScene)
        return;
    
    // 2. Build channel map for current animation
    m_->channelOfNode.assign(m_->skeleton.size(), nullptr);
    
    // Log animation change only once when animation switches
    bool shouldLog = (m_->lastLoggedClip != m_->currentClip);
    
    if (shouldLog)
    {
        if (m_->currentClip < m_->baseAnimationCount)
        {
            std::cout << "[FbxManager] Using base animation: " << anim->mName.C_Str() << std::endl;
        }
        else
        {
            int extIndex = m_->currentClip - m_->baseAnimationCount;
            std::cout << "[FbxManager] External animation selected - currentClip: " << m_->currentClip 
                      << ", baseAnimationCount: " << m_->baseAnimationCount 
                      << ", extIndex: " << extIndex << std::endl;
            
            if (extIndex >= 0 && extIndex < static_cast<int>(m_->externalAnimations.size()))
            {
                const auto& extAnim = m_->externalAnimations[extIndex];
                std::cout << "[FbxManager] Using external animation: " << extAnim.name << std::endl;
            }
        }
        
        std::cout << "[FbxManager] Building channel map for animation: " << anim->mName.C_Str() << std::endl;
        std::cout << "  Animation channels: " << anim->mNumChannels << std::endl;
        std::cout << "  Skeleton bones: " << m_->skeleton.size() << std::endl;
        
        // Print base model bone names for comparison
        std::cout << "  Base model bone names (first 10):" << std::endl;
        int boneCount = 0;
        for (const auto& pair : m_->nodeIndexOfName)
        {
            if (boneCount < 10)
            {
                std::cout << "    " << pair.first << " -> bone " << pair.second << std::endl;
                boneCount++;
            }
            else
            {
                std::cout << "    ... (showing first 10 of " << m_->nodeIndexOfName.size() << " bones)" << std::endl;
                break;
            }
        }
    }
    
    // 본 매핑: 애니메이션 채널을 스켈레톤 본에 연결
    // 애니메이션 채널: 각 본의 애니메이션 데이터(위치, 회전, 크기 키프레임)를 담고 있는 구조체
    // 예: mixamorig:Hips 본의 시간에 따른 위치/회전/크기 변화 정보
    // 이 채널들을 기본 모델의 실제 본들에 매핑하여 애니메이션을 적용
    int matchedChannels = 0;
    for (unsigned int i = 0; i < anim->mNumChannels; ++i)
    {
        // 현재 애니메이션 채널 가져오기 (본 이름과 애니메이션 데이터 포함)
        // aiNodeAnim: 특정 본의 애니메이션 키프레임들을 저장하는 Assimp 구조체
        const aiNodeAnim* channel = anim->mChannels[i];
        
        // 애니메이션 채널의 본 이름으로 기본 모델의 본 인덱스 찾기
        // m_->nodeIndexOfName: 본 이름 -> 본 인덱스 맵
        auto it = m_->nodeIndexOfName.find(channel->mNodeName.C_Str());
        
        if (it != m_->nodeIndexOfName.end())
        {
            // 기본 모델에 해당 본이 존재하는 경우
            int nodeIdx = it->second;
            
            // 본 인덱스가 유효한 범위인지 확인
            if (nodeIdx >= 0 && nodeIdx < static_cast<int>(m_->channelOfNode.size()))
            {
                // 채널 포인터를 본 매핑 배열에 저장
                // m_->channelOfNode[nodeIdx]: 해당 본에 적용할 애니메이션 채널
                m_->channelOfNode[nodeIdx] = channel;
                matchedChannels++;
                
                // 디버그 로그: 성공적으로 매핑된 본 정보 출력
                if (shouldLog)
                {
                    std::cout << "  Matched: " << channel->mNodeName.C_Str() << " -> bone " << nodeIdx << std::endl;
                }
            }
        }
        else
        {
            // 기본 모델에 해당 본이 없는 경우 (본 이름 불일치)
            // 이 경우 애니메이션이 적용되지 않음
            if (shouldLog)
            {
                std::cout << "  Unmatched: " << channel->mNodeName.C_Str() << " (no corresponding bone)" << std::endl;
            }
        }
    }
    
    if (shouldLog)
    {
        std::cout << "  Total matched channels: " << matchedChannels << "/" << anim->mNumChannels << std::endl;
        m_->lastLoggedClip = m_->currentClip;
    }
    
    // 3. Evaluate global matrices for all nodes
    std::vector<XMFLOAT4X4> globalMatrices(m_->skeleton.size());
    double animTime = m_->clipTimeSec * m_->clipTicksPerSec[m_->currentClip];
    
    EvaluateGlobalMatrices(animScene, animScene->mRootNode, m_->skeletonRoot,
                          m_->channelOfNode, globalMatrices, m_->nodeIndexOfName,
                          animTime, XMMatrixIdentity());
    
    // 3. Build bone palette matrices
    m_->currentBonePalette.resize(m_->boneNames.size());
    XMMATRIX globalInverse = XMLoadFloat4x4(&m_->globalInverse);
    
    // Debug: Compare globalInverse matrix between models to detect coordinate system differences
    if (shouldLog)
    {
        std::cout << "[FbxManager] GlobalInverse matrix analysis:" << std::endl;
        
        // Extract rotation from globalInverse
        XMVECTOR giScale, giRot, giTrans;
        XMMatrixDecompose(&giScale, &giRot, &giTrans, globalInverse);
        
        XMFLOAT4 giRotFloat;
        XMStoreFloat4(&giRotFloat, giRot);
        
        // Convert to Euler angles
        float sinr_cosp = 2 * (giRotFloat.w * giRotFloat.x + giRotFloat.y * giRotFloat.z);
        float cosr_cosp = 1 - 2 * (giRotFloat.x * giRotFloat.x + giRotFloat.y * giRotFloat.y);
        float giPitch = std::atan2(sinr_cosp, cosr_cosp);
        
        float sinp = 2 * (giRotFloat.w * giRotFloat.y - giRotFloat.z * giRotFloat.x);
        float giYaw;
        if (std::abs(sinp) >= 1)
            giYaw = std::copysign(XM_PI / 2, sinp);
        else
            giYaw = std::asin(sinp);
        
        float siny_cosp = 2 * (giRotFloat.w * giRotFloat.z + giRotFloat.x * giRotFloat.y);
        float cosy_cosp = 1 - 2 * (giRotFloat.y * giRotFloat.y + giRotFloat.z * giRotFloat.z);
        float giRoll = std::atan2(siny_cosp, cosy_cosp);
        
        std::cout << "  GlobalInverse rotation - Y: " << (giYaw * 180.0f / XM_PI) << "°, X: " 
                  << (giPitch * 180.0f / XM_PI) << "°, Z: " << (giRoll * 180.0f / XM_PI) << "°" << std::endl;
        
        // Check if this is likely a Mixamo model based on bone names
        bool isMixamo = false;
        for (const auto& pair : m_->nodeIndexOfName)
        {
            if (pair.first.find("mixamorig:") != std::string::npos)
            {
                isMixamo = true;
                break;
            }
        }
        
        if (isMixamo)
        {
            std::cout << "  Model type: Mixamo (check for 180° Y-rotation in GlobalInverse)" << std::endl;
            if (std::abs(giYaw * 180.0f / XM_PI) > 90.0f)
            {
                std::cout << "  WARNING: Mixamo model has large Y-rotation in GlobalInverse!" << std::endl;
            }
        }
        else
        {
            std::cout << "  Model type: Non-Mixamo (Alice or similar)" << std::endl;
        }
    }
    
    // Debug: Log matrix calculations for key bones to detect rotation issues
    std::vector<std::string> debugBones = {"mixamorig:Hips", "mixamorig:Spine", "mixamorig:LeftArm"};
    
    for (size_t i = 0; i < m_->boneNames.size(); ++i)
    {
        auto it = m_->nodeIndexOfName.find(m_->boneNames[i]);
        if (it != m_->nodeIndexOfName.end())
        {
            int nodeIdx = it->second;
            XMMATRIX global = XMLoadFloat4x4(&globalMatrices[nodeIdx]);
            XMMATRIX offset = XMLoadFloat4x4(&m_->boneOffsets[i]);
            
            // Debug logging for key bones
            if (shouldLog)
            {
                for (const std::string& debugBone : debugBones)
                {
                    if (m_->boneNames[i] == debugBone)
                    {
                        // Extract rotation from global matrix
                        XMVECTOR scale, rot, trans;
                        XMMatrixDecompose(&scale, &rot, &trans, global);
                        
                        // Convert quaternion to Euler angles
                        XMFLOAT4 rotQuat;
                        XMStoreFloat4(&rotQuat, rot);
                        float pitch, yaw, roll;
                        
                        // Simple quaternion to Euler conversion (YXZ order)
                        float sinr_cosp = 2 * (rotQuat.w * rotQuat.x + rotQuat.y * rotQuat.z);
                        float cosr_cosp = 1 - 2 * (rotQuat.x * rotQuat.x + rotQuat.y * rotQuat.y);
                        pitch = std::atan2(sinr_cosp, cosr_cosp);
                        
                        float sinp = 2 * (rotQuat.w * rotQuat.y - rotQuat.z * rotQuat.x);
                        if (std::abs(sinp) >= 1)
                            yaw = std::copysign(XM_PI / 2, sinp); // use 90 degrees if out of range
                        else
                            yaw = std::asin(sinp);
                        
                        float siny_cosp = 2 * (rotQuat.w * rotQuat.z + rotQuat.x * rotQuat.y);
                        float cosy_cosp = 1 - 2 * (rotQuat.y * rotQuat.y + rotQuat.z * rotQuat.z);
                        roll = std::atan2(siny_cosp, cosy_cosp);
                        
                        std::cout << "  DEBUG [" << m_->boneNames[i] << "]" << std::endl;
                        std::cout << "    Global Y-rotation (yaw): " << (yaw * 180.0f / XM_PI) << "°" << std::endl;
                        std::cout << "    Global X-rotation (pitch): " << (pitch * 180.0f / XM_PI) << "°" << std::endl;
                        std::cout << "    Global Z-rotation (roll): " << (roll * 180.0f / XM_PI) << "°" << std::endl;
                        break;
                    }
                }
            }
            
            // Final bone matrix: GlobalInverse * Global * Offset
            m_->currentBonePalette[i] = XMMatrixMultiply(XMMatrixMultiply(globalInverse, global), offset);
        }
        else
        {
            m_->currentBonePalette[i] = XMMatrixIdentity();
        }
    }
    
    // Log first few updates
    static int updateCount = 0;
    if (updateCount < 3)
    {
        std::cout << "[FbxManager] Bone palette computed: " << m_->currentBonePalette.size() << " bones" << std::endl;
        updateCount++;
    }
}

// Upload bone palette to GPU (called from FbxModel before rendering)
void FbxManager::UploadBonePaletteToGPU(ZGraphics& gfx)
{
    if (!m_->hasSkinning || m_->currentBonePalette.empty())
        return;
    
    // Create bone constant buffer if needed
    if (!m_->pBoneCB)
    {
        CreateBoneConstantBuffer(gfx, &m_->pBoneCB, Impl::kMaxBones);
    }
    
    // Upload current bone palette
    if (m_->pBoneCB)
    {
        UploadBonePalette(gfx, m_->pBoneCB, m_->currentBonePalette);
    }
}

ID3D11Buffer* FbxManager::GetBoneConstantBuffer() const
{
    return m_->pBoneCB;
}

bool FbxManager::IsMixamoModel() const
{
    // Check if any bone name contains "mixamorig:" prefix
    for (const auto& pair : m_->nodeIndexOfName)
    {
        if (pair.first.find("mixamorig:") != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

bool FbxManager::LoadExternalAnimation(const std::string& animFilePath, const std::string& animName)
{
    if (!m_->hasSkinning)
    {
        std::cerr << "[FbxManager] Cannot load external animation: No skeleton in base model" << std::endl;
        return false;
    }
    
    // Create new importer for this animation
    Impl::ExternalAnimation extAnim;
    extAnim.importer = std::make_unique<Assimp::Importer>();
    
    // Load animation file with same flags as base model for coordinate system consistency
    extAnim.scene = extAnim.importer->ReadFile(animFilePath,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ConvertToLeftHanded |  // CRITICAL: Must match base model loading
        aiProcess_LimitBoneWeights);
    
    if (!extAnim.scene || !extAnim.scene->mAnimations || extAnim.scene->mNumAnimations == 0)
    {
        std::cerr << "[FbxManager] Failed to load animation from: " << animFilePath << std::endl;
        return false;
    }
    
    // Use first animation from the file
    const aiAnimation* anim = extAnim.scene->mAnimations[0];
    
    // Set animation name
    if (!animName.empty())
    {
        extAnim.name = animName;
    }
    else if (anim->mName.length > 0)
    {
        extAnim.name = anim->mName.C_Str();
    }
    else
    {
        extAnim.name = "External_" + std::to_string(m_->externalAnimations.size());
    }
    
    // Store duration and ticks per second
    extAnim.ticksPerSecond = (anim->mTicksPerSecond > 0.0) ? anim->mTicksPerSecond : 25.0;
    extAnim.duration = anim->mDuration / extAnim.ticksPerSecond;
    
    std::cout << "  Loaded: " << extAnim.name << " (duration: " << extAnim.duration << "s)" << std::endl;
    
    // Add to animation lists
    m_->animationNames.push_back(extAnim.name);
    m_->clipDurationSec.push_back(extAnim.duration);
    m_->clipTicksPerSec.push_back(extAnim.ticksPerSecond);
    
    // Store external animation
    m_->externalAnimations.push_back(std::move(extAnim));
    
    m_->hasAnimations = true;
    
    return true;
}

// Load multiple external animation files
bool FbxManager::LoadExternalAnimations(const std::vector<std::string>& animFilePaths)
{
    bool allSuccess = true;
    
    for (const auto& path : animFilePaths)
    {
        std::string filename = std::filesystem::path(path).filename().replace_extension().string();

        if (!LoadExternalAnimation(path, filename))
        {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}
