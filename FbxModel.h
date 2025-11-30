#pragma once

#include "ZRenderableBase.h"

// Skinned model constant buffer (supports skeletal animation)
struct FbxModelConstantBuffer
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
    int shadingMode;           // 0:Phong, 1:Blinn-Phong, 2:Lambert, 3:Unlit
    DirectX::XMFLOAT3 pad2;
    int enableNormalMap;
    DirectX::XMFLOAT3 pad3;
    int useSpecularMap;
    DirectX::XMFLOAT3 pad4;
};

// FbxModel - Skeletal animated mesh with bone skinning
class FbxModel : public ZRenderableBase<FbxModel>
{
public:
    // Constructor with default textures array (one per material)
    FbxModel(ZGraphics& gfx, const std::string& filePath, 
            DirectX::XMFLOAT3 baseMaterialColor = { 1.0f, 1.0f, 1.0f },
            DirectX::XMFLOAT4 baseReflectionColor = { 0.0f, 0.0f, 0.0f, 0.5f },
            const std::vector<std::string>& defaultTexturePaths = {},
            const std::vector<std::string>& defaultNormalMapPaths = {},
            const std::vector<std::string>& defaultSpecularMapPaths = {},
            const std::vector<std::string>& externalAnimPaths = {});

    void Render(ZGraphics& gfx) const noxnd;
    void Update(float deltaTime) noexcept override;
    DirectX::XMMATRIX GetTransformXM() const noexcept override;
    
    // Transform controls
    void SetPosition(DirectX::XMFLOAT3 pos) { position_ = pos; }
    void SetRotation(float pitch, float yaw, float roll) { pitch_ = pitch; yaw_ = yaw; roll_ = roll; }
    void SetScale(float scale) { scale_ = scale; }
    
    // Animation controls
    bool LoadExternalAnimation(const std::string& animFilePath, const std::string& animName = "");
    bool LoadExternalAnimations(const std::vector<std::string>& animFilePaths);
    
    // ImGui controls
    void ShowControlWindow();
    
    // Wireframe mode control
    void SetWireframe(bool enabled) { wireframe_ = enabled; }
    bool IsWireframe() const { return wireframe_; }
    
    // Normal map control
    void SetNormalMapEnabled(bool enabled) { cbuf_.enableNormalMap = enabled ? 1 : 0; }
    bool IsNormalMapEnabled() const { return cbuf_.enableNormalMap == 1; }
    
    // Specular map control
    void SetSpecularMapEnabled(bool enabled) { cbuf_.useSpecularMap = enabled ? 1 : 0; }
    bool IsSpecularMapEnabled() const { return cbuf_.useSpecularMap == 1; }

private:
    std::unique_ptr<class FbxManager> fbxManager_;
    FbxModelConstantBuffer cbuf_;
    
    // Rasterizer states for solid and wireframe modes
    std::unique_ptr<Bind::ZRasterizer> rasterizerSolid_;
    std::unique_ptr<Bind::ZRasterizer> rasterizerWireframe_;
    
    // Transform
    float roll_ = 0.0f;
    float pitch_ = 0.0f;
    float yaw_ = 0.0f;
    float scale_ = 1.0f;
    DirectX::XMFLOAT3 position_ = { 0.0f, 0.0f, -10.0f };
    
    // Rendering mode
    bool wireframe_ = false;
};
