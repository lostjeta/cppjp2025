#include "FbxSkinnedModel_NormalMap.h"
#include "ZBindableBase.h"
#include "ZGraphics.h"
#include "imgui/imgui.h"
#include <iostream>
#include <WICTextureLoader.h>

using namespace Bind;
using namespace DirectX;

FbxSkinnedModel_NormalMap::FbxSkinnedModel_NormalMap(ZGraphics& gfx, const std::string& filePath, XMFLOAT3 materialColor, const std::vector<std::string>& defaultTexturePaths, const std::vector<std::string>& normalMapPaths, const std::vector<std::string>& externalAnimPaths)
{
    // Create FbxManager and load model with multiple textures
    m_FbxManager = std::make_unique<FbxManager>();
    
    if (!m_FbxManager->Load(gfx, filePath, defaultTexturePaths))
    {
        throw std::runtime_error("Failed to load FBX model: " + filePath);
    }
    
    // Load normal maps
    LoadNormalMaps(gfx, normalMapPaths);
    
    // Load external animations if provided
    if (!externalAnimPaths.empty())
    {
        std::cout << "[FbxSkinnedModel_NormalMap] Loading " << externalAnimPaths.size() << " external animations..." << std::endl;
        m_FbxManager->LoadExternalAnimations(externalAnimPaths);
    }

    if (!IsStaticInitialized())
    {
        // Setup static bindables
        if (m_FbxManager->HasMesh())
        {
            std::cout << "[FbxSkinnedModel_NormalMap] Mesh loaded successfully" << std::endl;
            std::cout << "  Vertices: " << m_FbxManager->GetIndexCount() / 3 << " triangles" << std::endl;
            std::cout << "  Subsets: " << m_FbxManager->GetSubsets().size() << std::endl;
        }

        // SkinnedVS_NormalMap shader (VSSkinned entry point with normal mapping support)
        auto pvs = std::make_unique<ZVertexShader>(gfx, L"./x64/Debug/SkinnedVS_NormalMap.cso");
        auto pvsbc = pvs->GetBytecode();
        AddStaticBind(std::move(pvs));

        // SkinnedPS_NormalMap shader (NEW: uses normal map pixel shader)
        AddStaticBind(std::make_unique<ZPixelShader>(gfx, L"./x64/Debug/SkinnedPS_NormalMap.cso"));

        // Input Layout (Skinned vertex format matching VertexInSkinned)
        using Dvtx::VertexLayout;
        Dvtx::VertexLayout layout;
        layout.Append(VertexLayout::Position3D);
        layout.Append(VertexLayout::Normal);
        layout.Append(VertexLayout::Tangent);
        layout.Append(VertexLayout::Bitangent);
        layout.Append(VertexLayout::Texture2D);
        layout.Append(VertexLayout::Float4Color);
        layout.Append(VertexLayout::UInt4);
        layout.Append(VertexLayout::Float4);

        std::cout << "[FbxSkinnedModel_NormalMap] Input Layout (Skinned with Normal Maps):" << std::endl;
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

        std::cout << "[FbxSkinnedModel_NormalMap] Static bindables initialized" << std::endl;
    }
    
    // Create dynamic rasterizer states (not static, so we can switch between them)
    m_pRasterizerSolid = std::make_unique<Bind::ZRasterizer>(gfx, D3D11_FILL_SOLID, true, false);
    m_pRasterizerWireframe = std::make_unique<Bind::ZRasterizer>(gfx, D3D11_FILL_WIREFRAME, true, false);

    // Initialize material
    m_CBuf.material.ambient = XMFLOAT4{ materialColor.x * 0.2f, materialColor.y * 0.2f, materialColor.z * 0.2f, 1.0f };
    m_CBuf.material.diffuse = XMFLOAT4{ materialColor.x, materialColor.y, materialColor.z, 1.0f };
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

    m_CBuf.shadingMode = 0;
    m_CBuf.enableNormalMap = 1;  // NEW: Enable normal mapping by default
    m_CBuf.useSpecularMap = 0;

    // Initialize animation
    if (m_FbxManager->HasAnimations() && m_FbxManager->GetAnimationCount() > 0)
    {
        std::cout << "[FbxSkinnedModel_NormalMap] Found " << m_FbxManager->GetAnimationCount() << " animations" << std::endl;
        m_FbxManager->SetCurrentAnimation(0);
        m_FbxManager->SetAnimationPlaying(true);
    }
}

void FbxSkinnedModel_NormalMap::LoadNormalMaps(ZGraphics& gfx, const std::vector<std::string>& normalMapPaths)
{
    std::cout << "[FbxSkinnedModel_NormalMap] Loading " << normalMapPaths.size() << " normal maps..." << std::endl;
    
    m_NormalMapSRVs.clear();
    m_NormalMapSRVs.resize(normalMapPaths.size());
    
    for (size_t i = 0; i < normalMapPaths.size(); ++i)
    {
        try
        {
            std::wstring widePath(normalMapPaths[i].begin(), normalMapPaths[i].end());
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
            
            HRESULT hr = DirectX::CreateWICTextureFromFile(
                gfx.GetDeviceCOM().Get(),
                gfx.GetDeviceContext().Get(),
                widePath.c_str(),
                nullptr,
                &srv
            );
            
            if (SUCCEEDED(hr))
            {
                m_NormalMapSRVs[i] = srv;
                std::cout << "  Loaded normal map: " << normalMapPaths[i] << std::endl;
            }
            else
            {
                std::cout << "  Failed to load normal map: " << normalMapPaths[i] << std::endl;
                // Create a default flat normal map (RGB = 128, 128, 255 = normal pointing up)
                // This would require additional code to generate a default texture
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "  Exception loading normal map " << normalMapPaths[i] << ": " << e.what() << std::endl;
        }
    }
}

void FbxSkinnedModel_NormalMap::Render(ZGraphics& gfx) const noxnd
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
    
    const auto modelTransform = GetTransformXM();
    XMStoreFloat4x4(&cbufCopy.world, XMMatrixTranspose(modelTransform));
    
    XMMATRIX worldInvTranspose = XMMatrixInverse(nullptr, modelTransform);
    worldInvTranspose = XMMatrixTranspose(worldInvTranspose);
    XMStoreFloat4x4(&cbufCopy.worldInvTranspose, XMMatrixTranspose(worldInvTranspose));
    
    const auto view = gfx.GetCamera();
    XMStoreFloat4x4(&cbufCopy.view, XMMatrixTranspose(view));
    
    const auto proj = gfx.GetProjection();
    XMStoreFloat4x4(&cbufCopy.proj, XMMatrixTranspose(proj));
    
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
    cbd.ByteWidth = sizeof(FbxSkinnedModelNormalMapConstantBuffer);

    D3D11_SUBRESOURCE_DATA csd{};
    csd.pSysMem = &cbufCopy;

    Microsoft::WRL::ComPtr<ID3D11Buffer> pConstantBuffer;
    gfx.GetDeviceCOM()->CreateBuffer(&cbd, &csd, &pConstantBuffer);
    gfx.GetDeviceContext()->VSSetConstantBuffers(0u, 1u, pConstantBuffer.GetAddressOf());
    gfx.GetDeviceContext()->PSSetConstantBuffers(0u, 1u, pConstantBuffer.GetAddressOf());

    // Upload bone palette to GPU
    if (m_FbxManager->HasSkeleton() && m_FbxManager->HasAnimations())
    {
        const_cast<FbxSkinnedModel_NormalMap*>(this)->m_FbxManager->UploadBonePaletteToGPU(gfx);
        
        ID3D11Buffer* boneCB = m_FbxManager->GetBoneConstantBuffer();
        if (boneCB)
        {
            gfx.GetDeviceContext()->VSSetConstantBuffers(1, 1, &boneCB);
        }
    }

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
        // Bind diffuse texture (slot 0)
        if (subset.materialIndex >= 0 && subset.materialIndex < (int)srvs.size())
        {
            ID3D11ShaderResourceView* diffuseSRV = srvs[subset.materialIndex].Get();
            gfx.GetDeviceContext()->PSSetShaderResources(0u, 1u, &diffuseSRV);
        }

        // NEW: Bind normal map texture (slot 1)
        if (m_CBuf.enableNormalMap == 1 && subset.materialIndex >= 0 && subset.materialIndex < (int)m_NormalMapSRVs.size())
        {
            ID3D11ShaderResourceView* normalSRV = m_NormalMapSRVs[subset.materialIndex].Get();
            if (normalSRV)
            {
                gfx.GetDeviceContext()->PSSetShaderResources(1u, 1u, &normalSRV);
            }
        }

        gfx.GetDeviceContext()->DrawIndexed(subset.indexCount, subset.startIndex, 0);
    }
}

void FbxSkinnedModel_NormalMap::Update(float deltaTime) noexcept
{
    if (m_FbxManager && m_FbxManager->HasAnimations())
    {
        m_FbxManager->UpdateAnimation(deltaTime);
    }
}

XMMATRIX FbxSkinnedModel_NormalMap::GetTransformXM() const noexcept
{
    return XMMatrixScaling(m_Scale, m_Scale, m_Scale) *
           XMMatrixRotationRollPitchYaw(m_Pitch, m_Yaw, m_Roll) *
           XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
}

void FbxSkinnedModel_NormalMap::ShowControlWindow()
{
    if (ImGui::Begin("Skinned Model Animation Control (Normal Map)"))
    {
        // Rendering mode
        ImGui::Text("Rendering Mode");
        ImGui::Checkbox("Wireframe", &m_Wireframe);
        ImGui::Separator();
        
        // NEW: Normal map controls
        ImGui::Text("Normal Mapping");
        bool normalMapEnabled = (m_CBuf.enableNormalMap == 1);
        if (ImGui::Checkbox("Enable Normal Map", &normalMapEnabled))
        {
            m_CBuf.enableNormalMap = normalMapEnabled ? 1 : 0;
        }
        ImGui::Text("Normal maps loaded: %d", (int)m_NormalMapSRVs.size());
        ImGui::Separator();
        
        if (m_FbxManager && m_FbxManager->HasAnimations())
        {
            ImGui::Text("Animation Control");
            ImGui::Separator();
            
            const auto& animNames = m_FbxManager->GetAnimationNames();
            int currentAnim = m_FbxManager->GetCurrentAnimationIndex();
            
            ImGui::Text("Animations: %d", (int)animNames.size());
            
            if (ImGui::BeginCombo("Animation", currentAnim >= 0 ? animNames[currentAnim].c_str() : "None"))
            {
                for (int i = 0; i < (int)animNames.size(); ++i)
                {
                    bool isSelected = (currentAnim == i);
                    if (ImGui::Selectable(animNames[i].c_str(), isSelected))
                    {
                        m_FbxManager->SetCurrentAnimation(i);
                        m_FbxManager->SetAnimationTimeSeconds(0.0);
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            
            bool isPlaying = m_FbxManager->IsAnimationPlaying();
            if (ImGui::Button(isPlaying ? "Pause" : "Play"))
            {
                m_FbxManager->SetAnimationPlaying(!isPlaying);
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Reset"))
            {
                m_FbxManager->SetAnimationTimeSeconds(0.0);
            }
            
            if (currentAnim >= 0)
            {
                ImGui::Separator();
                ImGui::Text("Current: %s", animNames[currentAnim].c_str());
                
                double currentTime = m_FbxManager->GetAnimationTimeSeconds();
                double duration = m_FbxManager->GetAnimationDuration(currentAnim);
                
                ImGui::Text("Time: %.2f / %.2f sec", currentTime, duration);
                
                float timeFloat = static_cast<float>(currentTime);
                float durationFloat = static_cast<float>(duration);
                if (ImGui::SliderFloat("Timeline", &timeFloat, 0.0f, durationFloat, "%.2f"))
                {
                    m_FbxManager->SetAnimationTimeSeconds(static_cast<double>(timeFloat));
                }
                
                float progress = duration > 0.0 ? static_cast<float>(currentTime / duration) : 0.0f;
                ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
            }
            
            ImGui::Separator();
            ImGui::Text("Skeleton Info");
            if (m_FbxManager->HasSkinning())
            {
                ImGui::Text("Bones: %d", m_FbxManager->GetBoneCount());
            }
            else
            {
                ImGui::Text("No skinning data");
            }
        }
        else
        {
            ImGui::Text("No animations loaded");
        }
        
        ImGui::Separator();
        ImGui::Text("Shading Mode");
        
        const char* shadingModes[] = { "Phong", "Blinn-Phong", "Lambert", "Unlit" };
        if (ImGui::Combo("Mode", &m_CBuf.shadingMode, shadingModes, 4))
        {
            // Shading mode changed
        }
        
        ImGui::Separator();
        ImGui::Text("Material Properties");
        
        // Specular Color Presets
        ImGui::Text("Specular Color Presets:");
        if (ImGui::Button("White"))
        {
            m_CBuf.material.specular.x = 1.0f;
            m_CBuf.material.specular.y = 1.0f;
            m_CBuf.material.specular.z = 1.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Gold"))
        {
            m_CBuf.material.specular.x = 1.0f;
            m_CBuf.material.specular.y = 0.8f;
            m_CBuf.material.specular.z = 0.6f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cool Metal"))
        {
            m_CBuf.material.specular.x = 0.9f;
            m_CBuf.material.specular.y = 0.9f;
            m_CBuf.material.specular.z = 1.0f;
        }
        
        // Specular Color (RGB)
        ImGui::ColorEdit3("Specular Color", &m_CBuf.material.specular.x);
        
        // Specular Power (stored in material.specular.w)
        ImGui::SliderFloat("Specular Power", &m_CBuf.material.specular.w, 5.0f, 200.0f, "%.1f");
        ImGui::Text("(5: Plastic, 30: Metal/Glass, 100: Mirror/Chrome)");
        
        ImGui::Separator();
        ImGui::Text("Transform");
        
        ImGui::DragFloat3("Position", &m_Position.x, 0.1f);
        ImGui::DragFloat("Scale", &m_Scale, 0.001f, 0.001f, 10.0f);
        ImGui::DragFloat("Pitch", &m_Pitch, 0.01f);
        ImGui::DragFloat("Yaw", &m_Yaw, 0.01f);
        ImGui::DragFloat("Roll", &m_Roll, 0.01f);
    }
    ImGui::End();
}

// Load external animation
bool FbxSkinnedModel_NormalMap::LoadExternalAnimation(const std::string& animFilePath, const std::string& animName)
{
    if (!m_FbxManager)
        return false;
    
    return m_FbxManager->LoadExternalAnimation(animFilePath, animName);
}

// Load multiple external animations
bool FbxSkinnedModel_NormalMap::LoadExternalAnimations(const std::vector<std::string>& animFilePaths)
{
    if (!m_FbxManager)
        return false;
    
    return m_FbxManager->LoadExternalAnimations(animFilePaths);
}
