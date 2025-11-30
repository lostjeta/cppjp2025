#pragma once

#include "ZRenderableBase.h"
#include "FbxManager.h"
#include "ZRasterizer.h"
#include <DirectXMath.h>
#include <memory>
#include <vector>

// Skinned model constant buffer with normal map support
struct FbxSkinnedModelNormalMapConstantBuffer
{
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 proj;
    DirectX::XMFLOAT4X4 worldInvTranspose;
    
    struct Material
    {
        DirectX::XMFLOAT4 ambient;
        DirectX::XMFLOAT4 diffuse;
        DirectX::XMFLOAT4 specular;  // w = specular power
        DirectX::XMFLOAT4 reflect;   // w = roughness
    } material;
    
    struct DirectionalLight
    {
        DirectX::XMFLOAT4 ambient;
        DirectX::XMFLOAT4 diffuse;
        DirectX::XMFLOAT4 specular;
        DirectX::XMFLOAT3 direction;
        float pad;
    } dirLight;
    
    struct PointLight
    {
        DirectX::XMFLOAT4 ambient;
        DirectX::XMFLOAT4 diffuse;
        DirectX::XMFLOAT4 specular;
        DirectX::XMFLOAT3 position;
        float range;
        DirectX::XMFLOAT3 att;  // (constant, linear, quadratic)
        float pad;
    } pointLight;
    
    DirectX::XMFLOAT3 eyePosW;
    float pad;
    int shadingMode;           // 0:Phong, 1:Blinn-Phong, 2:Lambert
    DirectX::XMFLOAT3 pad2;
    int enableNormalMap;       // 0: disabled, 1: enabled
    DirectX::XMFLOAT3 pad3;
    int useSpecularMap;
    DirectX::XMFLOAT3 pad4;
};

// FbxSkinnedModel_NormalMap - Skeletal animated mesh with bone skinning and normal mapping
// Uses VSSkinned entry point and SkinnedPS_NormalMap pixel shader
class FbxSkinnedModel_NormalMap : public ZRenderableBase<FbxSkinnedModel_NormalMap>
{
public:
    // Constructor with default textures and normal map paths
    FbxSkinnedModel_NormalMap(ZGraphics& gfx, const std::string& filePath, 
                              DirectX::XMFLOAT3 materialColor = { 1.0f, 1.0f, 1.0f },
                              const std::vector<std::string>& defaultTexturePaths = {},
                              const std::vector<std::string>& normalMapPaths = {},
                              const std::vector<std::string>& externalAnimPaths = {});

    void Render(ZGraphics& gfx) const noxnd;
    void Update(float deltaTime) noexcept override;
    DirectX::XMMATRIX GetTransformXM() const noexcept override;
    
    // Transform controls
    void SetPosition(DirectX::XMFLOAT3 pos) { m_Position = pos; }
    void SetRotation(float pitch, float yaw, float roll) { m_Pitch = pitch; m_Yaw = yaw; m_Roll = roll; }
    void SetScale(float scale) { m_Scale = scale; }
    
    // Animation controls
    bool LoadExternalAnimation(const std::string& animFilePath, const std::string& animName = "");
    bool LoadExternalAnimations(const std::vector<std::string>& animFilePaths);
    
    // Normal map controls
    void SetNormalMapEnabled(bool enabled) { m_CBuf.enableNormalMap = enabled ? 1 : 0; }
    bool IsNormalMapEnabled() const { return m_CBuf.enableNormalMap == 1; }
    
    // ImGui controls
    void ShowControlWindow();
    
    // Wireframe mode control
    void SetWireframe(bool enabled) { m_Wireframe = enabled; }
    bool IsWireframe() const { return m_Wireframe; }

private:
    // Load normal map textures
    void LoadNormalMaps(ZGraphics& gfx, const std::vector<std::string>& normalMapPaths);
    
    std::unique_ptr<FbxManager> m_FbxManager;
    FbxSkinnedModelNormalMapConstantBuffer m_CBuf;
    
    // Normal map SRVs (one per material)
    std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_NormalMapSRVs;
    
    // Rasterizer states for solid and wireframe modes
    std::unique_ptr<Bind::ZRasterizer> m_pRasterizerSolid;
    std::unique_ptr<Bind::ZRasterizer> m_pRasterizerWireframe;
    
    // Transform
    float m_Roll = 0.0f;
    float m_Pitch = 0.0f;
    float m_Yaw = 0.0f;
    float m_Scale = 1.0f;
    DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, -10.0f };
    
    // Rendering mode
    bool m_Wireframe = false;
};
