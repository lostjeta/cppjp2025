#include <memory>
#include <iostream>
#include <DirectXMath.h>
#include "imgui/imgui.h"

#include "FbxManager.h"
#include "ZRasterizer.h"
#include "ZGraphics.h"
#include "ZBindableBase.h"
#include "FbxModel.h"


using namespace Bind;
using namespace DirectX;

FbxModel::FbxModel(
    ZGraphics& gfx, 
    const std::string& filePath, 
    XMFLOAT3 baseMaterialColor, 
    XMFLOAT4 baseReflectionColor, 
    const std::vector<std::string>& defaultTexturePaths, 
    const std::vector<std::string>& defaultNormalMapPaths, 
    const std::vector<std::string>& defaultSpecularMapPaths, 
    const std::vector<std::string>& externalAnimPaths)
{
    // Create FbxManager and load model with multiple textures and normal maps
    fbxManager_ = std::make_unique<FbxManager>();
    
    // Choose appropriate Load function based on whether normal maps and specular maps are provided
    bool loadSuccess = false;
    if (!defaultSpecularMapPaths.empty())
    {
        std::cout << "[FbxModel] Loading with normal maps and specular maps..." << std::endl;
        loadSuccess = fbxManager_->Load(gfx, filePath, defaultTexturePaths, defaultNormalMapPaths, defaultSpecularMapPaths);
    }
    else if (!defaultNormalMapPaths.empty())
    {
        std::cout << "[FbxModel] Loading with normal maps..." << std::endl;
        loadSuccess = fbxManager_->Load(gfx, filePath, defaultTexturePaths, defaultNormalMapPaths);
    }
    else if (!defaultTexturePaths.empty())
    {
        std::cout << "[FbxModel] Loading with multiple textures..." << std::endl;
        loadSuccess = fbxManager_->Load(gfx, filePath, defaultTexturePaths);
    }
    else
    {
        std::cout << "[FbxModel] Loading with default texture..." << std::endl;
        loadSuccess = fbxManager_->Load(gfx, filePath);
    }
    
    if (!loadSuccess)
    {
        throw std::runtime_error("Failed to load FBX model: " + filePath);
    }
    
    // Load external animations if provided
    if (externalAnimPaths.size() > 0)
    {
        std::cout << "[FbxModel] Loading " << externalAnimPaths.size() << " external animations..." << std::endl;
        bool loadResult = fbxManager_->LoadExternalAnimations(externalAnimPaths);
        
        if (loadResult) {
            std::cout << "[FbxModel] All external animations loaded successfully!" << std::endl;
            
            // 애니메이션 정보 출력
            const auto& animNames = fbxManager_->GetAnimationNames();
            std::cout << "[FbxModel] Available animations (" << animNames.size() << "):" << std::endl;
            for (size_t i = 0; i < animNames.size(); ++i) {
                double duration = fbxManager_->GetClipDurationSec(i);
                std::cout << "  [" << i << "] " << animNames[i] << " (duration: " << duration << "s)" << std::endl;
            }
        } else {
            std::cout << "[FbxModel] Failed to load some external animations!" << std::endl;
        }
    }

    if (!IsStaticInitialized())
    {
        // Setup static bindables
        if (fbxManager_->HasMesh())
        {
            std::cout << "[FbxModel] Mesh loaded successfully" << std::endl;
            std::cout << "  Vertices: " << fbxManager_->GetIndexCount() / 3 << " triangles" << std::endl;
            std::cout << "  Subsets: " << fbxManager_->GetSubsets().size() << std::endl;
        }

        // SkinnedVS shader
        auto pvs = std::make_unique<ZVertexShader>(gfx, L"./x64/Debug/SkinnedModelVS.cso");
        auto pvsbc = pvs->GetBytecode();
        AddStaticBind(std::move(pvs));

        // SkinnedPS shader
        std::cout << "[FbxModel] Using SkinnedPS_NormalMap shader" << std::endl;
        AddStaticBind(std::make_unique<ZPixelShader>(gfx, L"./x64/Debug/SkinnedModelPS.cso"));

        // Input Layout (Skinned vertex format matching VertexInSkinned in SkinnedModelVS.hlsl)
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

        std::cout << "[FbxModel] Input Layout (Skinned):" << std::endl;
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

        std::cout << "[FbxModel] Static bindables initialized" << std::endl;
    }
    
    // Create dynamic rasterizer states (not static, so we can switch between them)
    rasterizerSolid_ = std::make_unique<Bind::ZRasterizer>(gfx, D3D11_FILL_SOLID, true, false);
    rasterizerWireframe_ = std::make_unique<Bind::ZRasterizer>(gfx, D3D11_FILL_WIREFRAME, true, false);

    // Initialize material
    cbuf_.material.ambient = XMFLOAT4{ baseMaterialColor.x * 0.2f, baseMaterialColor.y * 0.2f, baseMaterialColor.z * 0.2f, 1.0f };
    cbuf_.material.diffuse = XMFLOAT4{ baseMaterialColor.x, baseMaterialColor.y, baseMaterialColor.z, 1.0f };
    cbuf_.material.specular = XMFLOAT4{ baseMaterialColor.x, baseMaterialColor.y, baseMaterialColor.z, 32.0f };
    cbuf_.material.reflect = XMFLOAT4{ baseReflectionColor.x, baseReflectionColor.y, baseReflectionColor.z, baseReflectionColor.w };

    // Initialize lights
    cbuf_.dirLight.ambient = XMFLOAT4{ 0.2f, 0.2f, 0.2f, 1.0f };
    cbuf_.dirLight.diffuse = XMFLOAT4{ 0.3f, 0.3f, 0.3f, 1.0f };
    cbuf_.dirLight.specular = XMFLOAT4{ 0.2f, 0.2f, 0.2f, 1.0f };
    cbuf_.dirLight.direction = XMFLOAT3{ 0.0f, 1.0f, 1.0f };

    cbuf_.pointLight.ambient = XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f };
    cbuf_.pointLight.diffuse = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
    cbuf_.pointLight.specular = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
    cbuf_.pointLight.position = XMFLOAT3{ 0.0f, 0.0f, 0.0f };
    cbuf_.pointLight.range = 100.0f;
    cbuf_.pointLight.att = XMFLOAT3{ 1.0f, 0.045f, 0.0075f };

    cbuf_.shadingMode = 0;
    cbuf_.enableNormalMap = !defaultNormalMapPaths.empty() ? 1 : 0;
    cbuf_.useSpecularMap = !defaultSpecularMapPaths.empty() ? 1 : 0;

    // Initialize animation
    if (fbxManager_->HasAnimations() && fbxManager_->GetAnimationCount() > 0)
    {
        std::cout << "[FbxModel] Found " << fbxManager_->GetAnimationCount() << " animations" << std::endl;
        fbxManager_->SetCurrentAnimation(0);
        fbxManager_->SetAnimationPlaying(true);
    }
}

void FbxModel::Render(ZGraphics& gfx) const noxnd
{
    if (!fbxManager_ || !fbxManager_->HasMesh())
        return;

    // Bind all static bindables (Shaders, Input Layout, Topology, etc.)
    BindAll(gfx);
    
    // Bind appropriate rasterizer state based on wireframe mode
    if (wireframe_ && rasterizerWireframe_)
    {
        rasterizerWireframe_->Bind(gfx);
    }
    else if (rasterizerSolid_)
    {
        rasterizerSolid_->Bind(gfx);
    }

    // Update constant buffer
    auto cbufCopy = cbuf_;
    
    const auto modelTransform = GetTransformXM();
    XMStoreFloat4x4(&cbufCopy.world, XMMatrixTranspose(modelTransform));
    
    XMMATRIX worldInvTranspose = XMMatrixInverse(nullptr, modelTransform);
    worldInvTranspose = XMMatrixTranspose(worldInvTranspose);
    XMStoreFloat4x4(&cbufCopy.worldInvTranspose, XMMatrixTranspose(worldInvTranspose));
    
    const auto view = gfx.GetCamera();
    XMStoreFloat4x4(&cbufCopy.view, XMMatrixTranspose(view));
    
    const auto proj = gfx.GetProjection();
    XMStoreFloat4x4(&cbufCopy.proj, XMMatrixTranspose(proj));
    
    // XMMatrixInverse의 첫 번째 파라미터(pDeterminant)는 행렬식(determinant)을 저장할 XMVECTOR 포인터
    // - nullptr: 행렬식을 별도로 반환하지 않음 (일반적인 경우)
    // - XMVECTOR*: 행렬식 값을 받아올 포인터 (디버깅이나 특수한 경우에 사용)
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
    cbd.ByteWidth = sizeof(FbxModelConstantBuffer);

    D3D11_SUBRESOURCE_DATA csd{};
    csd.pSysMem = &cbufCopy;

    Microsoft::WRL::ComPtr<ID3D11Buffer> pConstantBuffer;
    gfx.GetDeviceCOM()->CreateBuffer(&cbd, &csd, &pConstantBuffer);
    gfx.GetDeviceContext()->VSSetConstantBuffers(0u, 1u, pConstantBuffer.GetAddressOf());
    gfx.GetDeviceContext()->PSSetConstantBuffers(0u, 1u, pConstantBuffer.GetAddressOf());

    // Upload bone palette to GPU
    if (fbxManager_->HasSkeleton() && fbxManager_->HasAnimations())
    {
        const_cast<FbxModel*>(this)->fbxManager_->UploadBonePaletteToGPU(gfx);
        
        ID3D11Buffer* boneCB = fbxManager_->GetBoneConstantBuffer();
        if (boneCB)
        {
            gfx.GetDeviceContext()->VSSetConstantBuffers(1, 1, &boneCB);
        }
    }

    // Set vertex and index buffers
    ID3D11Buffer* vb = fbxManager_->GetVertexBuffer();
    ID3D11Buffer* ib = fbxManager_->GetIndexBuffer();
    UINT stride = fbxManager_->GetVertexStride();
    UINT offset = fbxManager_->GetVertexOffset();

    gfx.GetDeviceContext()->IASetVertexBuffers(0u, 1u, &vb, &stride, &offset);
    gfx.GetDeviceContext()->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0u);

    // Render subsets
    // Get rendering data from FbxManager
    // Subsets: Groups of triangles that share the same material properties
    // Each subset represents a portion of the mesh with a specific material/texture
    const auto& subsets = fbxManager_->GetSubsets();
    const auto& srvs = fbxManager_->GetMaterialSRVs();
    const auto& normalMapSRVs = fbxManager_->GetNormalMapSRVs();
    const auto& specularMapSRVs = fbxManager_->GetSpecularMapSRVs();

    for (const auto& subset : subsets)
    {
        // Bind diffuse texture to slot 0
        if (subset.materialIndex >= 0 && subset.materialIndex < (int)srvs.size())
        {
            ID3D11ShaderResourceView* srv = srvs[subset.materialIndex].Get();
            gfx.GetDeviceContext()->PSSetShaderResources(0u, 1u, &srv);
        }

        // Bind normal map to slot 1 if available
        if (cbuf_.enableNormalMap == 1 && subset.materialIndex >= 0 && subset.materialIndex < (int)normalMapSRVs.size())
        {
            ID3D11ShaderResourceView* normalSRV = normalMapSRVs[subset.materialIndex].Get();
            if (normalSRV)
            {
                gfx.GetDeviceContext()->PSSetShaderResources(1u, 1u, &normalSRV);
            }
        }

        // Bind specular map to slot 2 if available
        if (subset.materialIndex >= 0 && subset.materialIndex < (int)specularMapSRVs.size())
        {
            ID3D11ShaderResourceView* specularSRV = specularMapSRVs[subset.materialIndex].Get();
            if (specularSRV)
            {
                gfx.GetDeviceContext()->PSSetShaderResources(2u, 1u, &specularSRV);
            }
        }

        gfx.GetDeviceContext()->DrawIndexed(subset.indexCount, subset.startIndex, 0);
    }
}

void FbxModel::Update(float deltaTime) noexcept
{
    if (fbxManager_ && fbxManager_->HasAnimations())
    {
        fbxManager_->UpdateAnimation(deltaTime);
    }
}

XMMATRIX FbxModel::GetTransformXM() const noexcept
{
    return XMMatrixScaling(scale_, scale_, scale_) *
           XMMatrixRotationRollPitchYaw(pitch_, yaw_, roll_) *
           XMMatrixTranslation(position_.x, position_.y, position_.z);
}

void FbxModel::ShowControlWindow()
{
    if (ImGui::Begin("Skinned Model Animation Control"))
    {
        // Rendering mode
        ImGui::Text("Rendering Mode");
        ImGui::Checkbox("Wireframe", &wireframe_);
        ImGui::Separator();
        
        if (fbxManager_ && fbxManager_->HasAnimations())
        {
            ImGui::Text("Animation Control");
            ImGui::Separator();
            
            const auto& animNames = fbxManager_->GetAnimationNames();
            int currentAnim = fbxManager_->GetCurrentAnimationIndex();
            
            ImGui::Text("Animations: %d", (int)animNames.size());
            
            if (ImGui::BeginCombo("Animation", currentAnim >= 0 ? animNames[currentAnim].c_str() : "None"))
            {
                for (int i = 0; i < (int)animNames.size(); ++i)
                {
                    bool isSelected = (currentAnim == i);
                    std::string label = animNames[i] + "##" + std::to_string(i);
                    if (ImGui::Selectable(label.c_str(), isSelected))
                    {
                        fbxManager_->SetCurrentAnimation(i);
                        fbxManager_->SetAnimationTimeSeconds(0.0);
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            
            bool isPlaying = fbxManager_->IsAnimationPlaying();
            if (ImGui::Button(isPlaying ? "Pause" : "Play"))
            {
                fbxManager_->SetAnimationPlaying(!isPlaying);
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Reset"))
            {
                fbxManager_->SetAnimationTimeSeconds(0.0);
            }
            
            if (currentAnim >= 0)
            {
                ImGui::Separator();
                ImGui::Text("Current: %s", animNames[currentAnim].c_str());
                
                double currentTime = fbxManager_->GetAnimationTimeSeconds();
                double duration = fbxManager_->GetAnimationDuration(currentAnim);
                
                ImGui::Text("Time: %.2f / %.2f sec", currentTime, duration);
                
                float timeFloat = static_cast<float>(currentTime);
                float durationFloat = static_cast<float>(duration);
                if (ImGui::SliderFloat("Timeline", &timeFloat, 0.0f, durationFloat, "%.2f"))
                {
                    fbxManager_->SetAnimationTimeSeconds(static_cast<double>(timeFloat));
                }
                
                float progress = duration > 0.0 ? static_cast<float>(currentTime / duration) : 0.0f;
                ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
            }
            
            ImGui::Separator();
            ImGui::Text("Skeleton Info");
            if (fbxManager_->HasSkinning())
            {
                ImGui::Text("Bones: %d", fbxManager_->GetBoneCount());
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
        if (ImGui::Combo("Mode", &cbuf_.shadingMode, shadingModes, 4))
        {
            // Shading mode changed
        }
        
        // Normal map control
        bool normalMapEnabled = IsNormalMapEnabled();
        if (ImGui::Checkbox("Enable Normal Map", &normalMapEnabled))
        {
            SetNormalMapEnabled(normalMapEnabled);
        }
        
        // Specular map control
        bool specularMapEnabled = IsSpecularMapEnabled();
        if (ImGui::Checkbox("Enable Specular Map", &specularMapEnabled))
        {
            SetSpecularMapEnabled(specularMapEnabled);
        }
        
        ImGui::Separator();
        ImGui::Text("Material Properties");
        
        // Specular Color Presets
        ImGui::Text("Specular Color Presets:");
        if (ImGui::Button("White"))
        {
            cbuf_.material.specular.x = 1.0f;
            cbuf_.material.specular.y = 1.0f;
            cbuf_.material.specular.z = 1.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Gold"))
        {
            cbuf_.material.specular.x = 1.0f;
            cbuf_.material.specular.y = 0.8f;
            cbuf_.material.specular.z = 0.6f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cool Metal"))
        {
            cbuf_.material.specular.x = 0.9f;
            cbuf_.material.specular.y = 0.9f;
            cbuf_.material.specular.z = 1.0f;
        }
        
        // Specular Color (RGB)
        ImGui::ColorEdit3("Specular Color", &cbuf_.material.specular.x);
        
        // Specular Power (stored in material.specular.w)
        ImGui::SliderFloat("Specular Power", &cbuf_.material.specular.w, 1.0f, 200.0f, "%.1f");
        ImGui::Text("(5: Plastic, 30: Metal/Glass, 100: Mirror/Chrome)");
        
        ImGui::Separator();
        ImGui::Text("Transform");
        
        ImGui::DragFloat3("Position", &position_.x, 0.1f);
        ImGui::DragFloat("Scale", &scale_, 0.001f, 0.001f, 10.0f);
        ImGui::DragFloat("Pitch", &pitch_, 0.01f);
        ImGui::DragFloat("Yaw", &yaw_, 0.01f);
        ImGui::DragFloat("Roll", &roll_, 0.01f);
    }
    ImGui::End();
}

// Load external animation
bool FbxModel::LoadExternalAnimation(const std::string& animFilePath, const std::string& animName)
{
    if (!fbxManager_)
        return false;
    
    return fbxManager_->LoadExternalAnimation(animFilePath, animName);
}

// Load multiple external animations
bool FbxModel::LoadExternalAnimations(const std::vector<std::string>& animFilePaths)
{
    if (!fbxManager_)
        return false;
    
    return fbxManager_->LoadExternalAnimations(animFilePaths);
}