#include "FbxStaticModel.h"
#include "ZBindableBase.h"
#include "ZGraphics.h"
#include "imgui/imgui.h"
#include <iostream>
#include <assimp/scene.h>

using namespace Bind;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

FbxStaticModel::FbxStaticModel(ZGraphics& gfx, const std::string& filePath, XMFLOAT3 baseMaterialColor, const std::string& defaultTexturePath)
{
    // Create FbxManager and load model
    m_FbxManager = std::make_unique<FbxManager>();
    
    if (!m_FbxManager->Load(gfx, filePath, defaultTexturePath))
    {
        throw std::runtime_error("Failed to load FBX model: " + filePath);
    }

    // Build simple vertex buffer from Assimp scene
    BuildSimpleBuffers(gfx);

    if (!IsStaticInitialized())
    {
        // Setup static bindables
        if (m_FbxManager->HasMesh())
        {
            std::cout << "[FbxStaticModel] Mesh loaded successfully" << std::endl;
        }

        // StaticVS shader (VSSimple entry point)
        auto pvs = std::make_unique<ZVertexShader>(gfx, L"./x64/Debug/StaticVS.cso");
        auto pvsbc = pvs->GetBytecode();
        AddStaticBind(std::move(pvs));

        // StaticPS shader
        AddStaticBind(std::make_unique<ZPixelShader>(gfx, L"./x64/Debug/StaticPS.cso"));

        // Input Layout (Simple vertex format: Position + Normal + TexCoord)
        using Dvtx::VertexLayout;
        Dvtx::VertexLayout layout;
        layout.Append(VertexLayout::Position3D);      // POSITION
        layout.Append(VertexLayout::Normal);          // NORMAL
        layout.Append(VertexLayout::Texture2D);       // TEXCOORD

        std::cout << "[FbxStaticModel] Input Layout (Simple):" << std::endl;
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

        std::cout << "[FbxStaticModel] Static bindables initialized" << std::endl;
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
}

void FbxStaticModel::Render(ZGraphics& gfx) const noxnd
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
    cbd.ByteWidth = sizeof(FbxStaticModelConstantBuffer);

    D3D11_SUBRESOURCE_DATA csd{};
    csd.pSysMem = &cbufCopy;

    ComPtr<ID3D11Buffer> pConstantBuffer;
    gfx.GetDeviceCOM()->CreateBuffer(&cbd, &csd, &pConstantBuffer);
    gfx.GetDeviceContext()->VSSetConstantBuffers(0u, 1u, pConstantBuffer.GetAddressOf());
    gfx.GetDeviceContext()->PSSetConstantBuffers(0u, 1u, pConstantBuffer.GetAddressOf());

    // Set vertex and index buffers (use our simple buffers)
    UINT stride = sizeof(VertexSimple);
    UINT offset = 0;

    gfx.GetDeviceContext()->IASetVertexBuffers(0u, 1u, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
    gfx.GetDeviceContext()->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0u);

    // Render subsets (use our own subsets)
    const auto& srvs = m_FbxManager->GetMaterialSRVs();

    for (const auto& subset : m_Subsets)
    {
        if (subset.materialIndex >= 0 && subset.materialIndex < (int)srvs.size())
        {
            ID3D11ShaderResourceView* srv = srvs[subset.materialIndex].Get();
            gfx.GetDeviceContext()->PSSetShaderResources(0u, 1u, &srv);
        }

        gfx.GetDeviceContext()->DrawIndexed(subset.indexCount, subset.startIndex, 0);
    }
}

void FbxStaticModel::Update(float deltaTime) noexcept
{
    // Static model - no animation
}

XMMATRIX FbxStaticModel::GetTransformXM() const noexcept
{
    return XMMatrixScaling(m_Scale, m_Scale, m_Scale) *
           XMMatrixRotationRollPitchYaw(m_Pitch, m_Yaw, m_Roll) *
           XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
}

void FbxStaticModel::ShowControlWindow()
{
    if (ImGui::Begin("FbxStaticModel Controls"))
    {
        ImGui::Text("Static Model (No Animation)");
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
        ImGui::Text("Mesh Info");
        ImGui::Text("Index Count: %u", m_IndexCount);
        ImGui::Text("Subsets: %zu", m_Subsets.size());
    }
    ImGui::End();
}

void FbxStaticModel::BuildSimpleBuffers(ZGraphics& gfx)
{
    const aiScene* scene = m_FbxManager->GetScene();
    if (!scene || scene->mNumMeshes == 0)
    {
        throw std::runtime_error("No mesh data in scene");
    }

    std::vector<VertexSimple> vertices;
    std::vector<uint32_t> indices;
    m_Subsets.clear();

    std::cout << "[FbxStaticModel] Building simple vertex buffer..." << std::endl;

    // Process all meshes
    for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi)
    {
        const aiMesh* mesh = scene->mMeshes[mi];
        FbxSubset subset{};
        subset.startIndex = static_cast<uint32_t>(indices.size());
        subset.materialIndex = mesh->mMaterialIndex;

        uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());

        // Load vertices
        for (unsigned int vi = 0; vi < mesh->mNumVertices; ++vi)
        {
            VertexSimple v{};

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

            // TexCoord
            if (mesh->HasTextureCoords(0))
            {
                v.texcoord = XMFLOAT2{
                    mesh->mTextureCoords[0][vi].x,
                    mesh->mTextureCoords[0][vi].y
                };
            }
            else
            {
                v.texcoord = XMFLOAT2{ 0.0f, 0.0f };
            }

            vertices.push_back(v);
        }

        // Load indices
        for (unsigned int fi = 0; fi < mesh->mNumFaces; ++fi)
        {
            const aiFace& face = mesh->mFaces[fi];
            for (unsigned int ii = 0; ii < face.mNumIndices; ++ii)
            {
                indices.push_back(vertexOffset + face.mIndices[ii]);
            }
        }

        subset.indexCount = static_cast<uint32_t>(indices.size()) - subset.startIndex;
        m_Subsets.push_back(subset);
    }

    m_IndexCount = static_cast<UINT>(indices.size());

    std::cout << "  Total vertices: " << vertices.size() << std::endl;
    std::cout << "  Total indices: " << indices.size() << std::endl;
    std::cout << "  Subsets: " << m_Subsets.size() << std::endl;

    // Create vertex buffer
    D3D11_BUFFER_DESC vbd{};
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(VertexSimple));

    D3D11_SUBRESOURCE_DATA vbData{};
    vbData.pSysMem = vertices.data();

    ComPtr<ID3D11Device> device;
    device = gfx.GetDeviceCOM();
    HRESULT hr = device->CreateBuffer(&vbd, &vbData, &m_pVertexBuffer);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create vertex buffer");
    }

    // Create index buffer
    D3D11_BUFFER_DESC ibd{};
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = static_cast<UINT>(indices.size() * sizeof(uint32_t));

    D3D11_SUBRESOURCE_DATA ibData{};
    ibData.pSysMem = indices.data();

    hr = device->CreateBuffer(&ibd, &ibData, &m_pIndexBuffer);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create index buffer");
    }

    std::cout << "[FbxStaticModel] Simple buffers created successfully" << std::endl;
}
