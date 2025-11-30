#pragma once

#include "ZRenderableBase.h"
#include "FbxManager.h"
#include <DirectXMath.h>
#include <memory>
#include <wrl/client.h>
#include <vector>

// Forward declaration
namespace Bind
{
    class ZRasterizer;
}

// Simple static model constant buffer
struct FbxStaticModelConstantBuffer
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
};

// FbxStaticModel - Simple static mesh (Position + Normal + TexCoord)
// Uses VSSimple entry point
class FbxStaticModel : public ZRenderableBase<FbxStaticModel>
{
public:
    FbxStaticModel(ZGraphics& gfx, const std::string& filePath, 
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
    void BuildSimpleBuffers(ZGraphics& gfx);
    // Simple vertex structure (Position + Normal + TexCoord)
    struct VertexSimple
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT2 texcoord;
    };

    std::unique_ptr<FbxManager> m_FbxManager;
    FbxStaticModelConstantBuffer m_CBuf;
    
    // Self-managed vertex/index buffers
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_pVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_pIndexBuffer;
    UINT m_IndexCount = 0;
    std::vector<FbxSubset> m_Subsets;
    
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
