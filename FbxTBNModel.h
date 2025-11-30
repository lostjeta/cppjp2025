#pragma once

#include "ZRenderableBase.h"
#include "FbxManager.h"
#include <DirectXMath.h>
#include <memory>

// Forward declaration
namespace Bind
{
    class ZRasterizer;
}

// TBN model constant buffer (supports normal mapping)
struct FbxTBNModelConstantBuffer
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
        DirectX::XMFLOAT3 att;
        float pad;
    } pointLight;
    
    DirectX::XMFLOAT3 eyePosW;
    float pad;
    int shadingMode;
    DirectX::XMFLOAT3 pad2;
    int enableNormalMap;
    DirectX::XMFLOAT3 pad3;
    int useSpecularMap;
    DirectX::XMFLOAT3 pad4;
};

// FbxTBNModel - Static mesh with TBN (Tangent, Bitangent, Normal)
// Supports normal mapping
// Uses main entry point
class FbxTBNModel : public ZRenderableBase<FbxTBNModel>
{
public:
    FbxTBNModel(ZGraphics& gfx, const std::string& filePath, 
                DirectX::XMFLOAT3 baseMaterialColor = { 1.0f, 1.0f, 1.0f },
                const std::string& defaultTexturePath = "");

    void Render(ZGraphics& gfx) const noxnd;
    void Update(float deltaTime) noexcept override;
    DirectX::XMMATRIX GetTransformXM() const noexcept override;
    
    // Transform controls
    void SetPosition(DirectX::XMFLOAT3 pos) { m_Position = pos; }
    void SetRotation(float pitch, float yaw, float roll) { m_Pitch = pitch; m_Yaw = yaw; m_Roll = roll; }
    void SetScale(float scale) { m_Scale = scale; }
    
    // ImGui control window
    void ShowControlWindow();
    
    // Wireframe mode control
    void SetWireframe(bool enabled) { m_Wireframe = enabled; }
    bool IsWireframe() const { return m_Wireframe; }

private:
    std::unique_ptr<FbxManager> m_FbxManager;
    FbxTBNModelConstantBuffer m_CBuf;
    
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
