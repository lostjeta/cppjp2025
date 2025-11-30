#include "FbxTBNModel.h"
#include "ZBindableBase.h"
#include "ZGraphics.h"
#include "imgui/imgui.h"
#include <iostream>

using namespace Bind;
using namespace DirectX;

FbxTBNModel::FbxTBNModel(ZGraphics& gfx, const std::string& filePath, XMFLOAT3 baseMaterialColor, const std::string& defaultTexturePath)
{
    // Create FbxManager and load model
    m_FbxManager = std::make_unique<FbxManager>();
    
    if (!m_FbxManager->Load(gfx, filePath, defaultTexturePath))
    {
        throw std::runtime_error("Failed to load FBX model: " + filePath);
    }

    if (!IsStaticInitialized())
    {
        // Setup static bindables
        if (m_FbxManager->HasMesh())
        {
            std::cout << "[FbxTBNModel] Mesh loaded successfully" << std::endl;
        }

        // TBNVS shader (main entry point)
        auto pvs = std::make_unique<ZVertexShader>(gfx, L"./x64/Debug/TBNVS.cso");
        auto pvsbc = pvs->GetBytecode();
        AddStaticBind(std::move(pvs));

        // TBNPS shader
        AddStaticBind(std::make_unique<ZPixelShader>(gfx, L"./x64/Debug/TBNPS.cso"));

        // Input Layout (TBN vertex format)
        using Dvtx::VertexLayout;
        Dvtx::VertexLayout layout;
        layout.Append(VertexLayout::Position3D);      // POSITION
        layout.Append(VertexLayout::Normal);          // NORMAL
        layout.Append(VertexLayout::Tangent);         // TANGENT
        layout.Append(VertexLayout::Bitangent);       // BINORMAL
        layout.Append(VertexLayout::Texture2D);       // TEXCOORD
        layout.Append(VertexLayout::Float4Color);     // COLOR

        std::cout << "[FbxTBNModel] Input Layout (TBN):" << std::endl;
        auto d3dLayout = layout.GetD3DLayout();
        for (size_t i = 0; i < d3dLayout.size(); ++i)
        {
            std::cout << "  [" << i << "] " << d3dLayout[i].SemanticName 
                      << " offset=" << d3dLayout[i].AlignedByteOffset << std::endl;
        }
        std::cout << "  Total stride: " << layout.Size() << " bytes" << std::endl;

        AddStaticBind(std::make_unique<ZInputLayout>(gfx, d3dLayout, pvsbc));
        AddStaticBind(std::make_unique<ZTopology>(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
        AddStaticBind(std::make_unique<ZSampler>(gfx));

        std::cout << "[FbxTBNModel] Static bindables initialized" << std::endl;
    }
    
    // Create dynamic rasterizer states (not static, so we can switch between them)
    m_pRasterizerSolid = std::make_unique<Bind::ZRasterizer>(gfx, D3D11_FILL_SOLID, true, false);
    m_pRasterizerWireframe = std::make_unique<Bind::ZRasterizer>(gfx, D3D11_FILL_WIREFRAME, true, false);

    // Initialize material
    m_CBuf.material.ambient = XMFLOAT4{ baseMaterialColor.x * 0.2f, baseMaterialColor.y * 0.2f, baseMaterialColor.z * 0.2f, 1.0f };
    m_CBuf.material.diffuse = XMFLOAT4{ baseMaterialColor.x, baseMaterialColor.y, baseMaterialColor.z, 1.0f };
    m_CBuf.material.specular = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 32.0f };
    m_CBuf.material.reflect = XMFLOAT4{ 0.0f, 0.0f, 0.0f, 0.5f };

    // Initialize lights
    m_CBuf.dirLight.ambient = XMFLOAT4{ 0.2f, 0.2f, 0.2f, 1.0f };
    m_CBuf.dirLight.diffuse = XMFLOAT4{ 0.3f, 0.3f, 0.3f, 1.0f };
    m_CBuf.dirLight.specular = XMFLOAT4{ 0.2f, 0.2f, 0.2f, 1.0f };
    m_CBuf.dirLight.direction = XMFLOAT3{ 0.0f, -1.0f, 1.0f };

    m_CBuf.pointLight.ambient = XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f };
    m_CBuf.pointLight.diffuse = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
    m_CBuf.pointLight.specular = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
    m_CBuf.pointLight.position = XMFLOAT3{ 0.0f, 0.0f, 0.0f };
    m_CBuf.pointLight.range = 100.0f;
    m_CBuf.pointLight.att = XMFLOAT3{ 1.0f, 0.045f, 0.0075f };

    m_CBuf.shadingMode = 0;  // Phong
    m_CBuf.enableNormalMap = 0;  // Disabled by default
    m_CBuf.useSpecularMap = 0;   // Disabled by default
}

void FbxTBNModel::Render(ZGraphics& gfx) const noxnd
{
    if (!m_FbxManager || !m_FbxManager->HasMesh())
        return;

    // Bind all static bindables (Shaders, Input Layout, Topology, etc.)
    BindAll(gfx);
    
    // Bind appropriate rasterizer state based on wireframe mode
    if (m_Wireframe && m_pRasterizerWireframe)
    {
        m_pRasterizerWireframe->Bind(gfx);
    }
    else if (m_pRasterizerSolid)
    {
        m_pRasterizerSolid->Bind(gfx);
    }

    // Update constant buffer
    auto cbufCopy = m_CBuf;
    
    // Set matrices
    const auto modelTransform = GetTransformXM();
    XMStoreFloat4x4(&cbufCopy.world, XMMatrixTranspose(modelTransform));
    
    XMMATRIX worldInvTranspose = XMMatrixInverse(nullptr, modelTransform);
    worldInvTranspose = XMMatrixTranspose(worldInvTranspose);
    XMStoreFloat4x4(&cbufCopy.worldInvTranspose, XMMatrixTranspose(worldInvTranspose));
    
    const auto view = gfx.GetCamera();
    XMStoreFloat4x4(&cbufCopy.view, XMMatrixTranspose(view));
    
    const auto proj = gfx.GetProjection();
    XMStoreFloat4x4(&cbufCopy.proj, XMMatrixTranspose(proj));
    
    // Camera position
    XMMATRIX invView = XMMatrixInverse(nullptr, view);
    XMFLOAT4X4 invViewF;
    XMStoreFloat4x4(&invViewF, invView);
    cbufCopy.eyePosW = XMFLOAT3{ invViewF._41, invViewF._42, invViewF._43 };

    // Update light data from ZGraphics
    const auto& dirLight = gfx.GetDirectionalLight();
    cbufCopy.dirLight.ambient = dirLight.ambient;
    cbufCopy.dirLight.diffuse = dirLight.diffuse;
    cbufCopy.dirLight.specular = dirLight.specular;
    cbufCopy.dirLight.direction = dirLight.direction;
    
    const auto& pointLight = gfx.GetPointLight();
    cbufCopy.pointLight.ambient = pointLight.ambient;
    cbufCopy.pointLight.diffuse = pointLight.diffuse;
    cbufCopy.pointLight.specular = pointLight.specular;
    cbufCopy.pointLight.position = pointLight.position;
    cbufCopy.pointLight.range = pointLight.range;
    cbufCopy.pointLight.att = pointLight.att;

    // Create and bind constant buffer
    D3D11_BUFFER_DESC cbd{};
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbd.ByteWidth = sizeof(FbxTBNModelConstantBuffer);

    D3D11_SUBRESOURCE_DATA csd{};
    csd.pSysMem = &cbufCopy;

    Microsoft::WRL::ComPtr<ID3D11Buffer> pConstantBuffer;
    gfx.GetDeviceCOM()->CreateBuffer(&cbd, &csd, &pConstantBuffer);
    gfx.GetDeviceContext()->VSSetConstantBuffers(0u, 1u, pConstantBuffer.GetAddressOf());
    gfx.GetDeviceContext()->PSSetConstantBuffers(0u, 1u, pConstantBuffer.GetAddressOf());

    // Set vertex and index buffers
    ID3D11Buffer* vb = m_FbxManager->GetVertexBuffer();
    ID3D11Buffer* ib = m_FbxManager->GetIndexBuffer();
    UINT stride = m_FbxManager->GetVertexStride();
    UINT offset = m_FbxManager->GetVertexOffset();

    gfx.GetDeviceContext()->IASetVertexBuffers(0u, 1u, &vb, &stride, &offset);
    gfx.GetDeviceContext()->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0u);

    // Render subsets
    const auto& subsets = m_FbxManager->GetSubsets();
    const auto& srvs = m_FbxManager->GetMaterialSRVs();

    for (const auto& subset : subsets)
    {
        if (subset.materialIndex >= 0 && subset.materialIndex < (int)srvs.size())
        {
            ID3D11ShaderResourceView* srv = srvs[subset.materialIndex].Get();
            gfx.GetDeviceContext()->PSSetShaderResources(0u, 1u, &srv);
        }

        gfx.GetDeviceContext()->DrawIndexed(subset.indexCount, subset.startIndex, 0);
    }
}

void FbxTBNModel::Update(float deltaTime) noexcept
{
    // Static model - no animation
}

XMMATRIX FbxTBNModel::GetTransformXM() const noexcept
{
    return XMMatrixScaling(m_Scale, m_Scale, m_Scale) *
           XMMatrixRotationRollPitchYaw(m_Pitch, m_Yaw, m_Roll) *
           XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
}

void FbxTBNModel::ShowControlWindow()
{
    if (ImGui::Begin("FbxTBNModel Controls"))
    {
        ImGui::Text("TBN Model (Normal Mapping Support)");
        ImGui::Separator();
        
        // Rendering mode
        ImGui::Text("Rendering Mode");
        ImGui::Checkbox("Wireframe", &m_Wireframe);
        ImGui::Separator();
        
        // Transform controls
        ImGui::Text("Transform");
        ImGui::SliderFloat3("Position", &m_Position.x, -20.0f, 20.0f);
        ImGui::SliderFloat("Scale", &m_Scale, 0.001f, 5.0f);
        
        float pitchDeg = XMConvertToDegrees(m_Pitch);
        float yawDeg = XMConvertToDegrees(m_Yaw);
        float rollDeg = XMConvertToDegrees(m_Roll);
        
        if (ImGui::SliderFloat("Pitch", &pitchDeg, -180.0f, 180.0f))
            m_Pitch = XMConvertToRadians(pitchDeg);
        if (ImGui::SliderFloat("Yaw", &yawDeg, -180.0f, 180.0f))
            m_Yaw = XMConvertToRadians(yawDeg);
        if (ImGui::SliderFloat("Roll", &rollDeg, -180.0f, 180.0f))
            m_Roll = XMConvertToRadians(rollDeg);
        
        if (ImGui::Button("Reset Transform"))
        {
            m_Position = { 0.0f, 0.0f, -10.0f };
            m_Scale = 1.0f;
            m_Pitch = m_Yaw = m_Roll = 0.0f;
        }
        
        ImGui::Separator();
        
        // Mesh info
        if (m_FbxManager && m_FbxManager->HasMesh())
        {
            ImGui::Text("Mesh Info");
            ImGui::Text("Vertex Count: %d", m_FbxManager->GetIndexCount());
            ImGui::Text("Subsets: %zu", m_FbxManager->GetSubsets().size());
        }
    }
    ImGui::End();
}
