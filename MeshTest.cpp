#include "ZD3D11.h"
#include "MeshTest.h"
#include "ZBindableBase.h"
#include "GraphicsThrowMacros.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "ZVertex.h"

using namespace Bind;

MeshTest::MeshTest(ZGraphics& gfx, std::mt19937& rng,
    std::uniform_real_distribution<float>& adist,
    std::uniform_real_distribution<float>& ddist,
    std::uniform_real_distribution<float>& odist,
    std::uniform_real_distribution<float>& rdist,
    DirectX::XMFLOAT3 material,
    float scale)
    :
    ZInteractableTransform(gfx, rng, adist, ddist, odist, rdist)
{
    namespace dx = DirectX;

    if (!IsStaticInitialized())
    {
        using Dvtx::VertexLayout;
        Dvtx::VertexBuffer vbuf(std::move(
            VertexLayout{}
            .Append(VertexLayout::Position3D)
            .Append(VertexLayout::Normal)
        ));

        Assimp::Importer imp;
        const auto pModel = imp.ReadFile("./Data/Models/suzanne.obj",
            // 복잡한 메시도 내부 삼각형으로 개별 처리
            // https://the-asset-importer-lib-documentation.readthedocs.io/en/latest/usage/postprocessing.html
            aiProcess_Triangulate | 
            aiProcess_JoinIdenticalVertices
        );
        const auto pMesh = pModel->mMeshes[0];

        for (unsigned int i = 0; i < pMesh->mNumVertices; i++)
        {
            vbuf.EmplaceBack(
                dx::XMFLOAT3{ pMesh->mVertices[i].x * scale,pMesh->mVertices[i].y * scale,pMesh->mVertices[i].z * scale },
                *reinterpret_cast<dx::XMFLOAT3*>(&pMesh->mNormals[i])
            );
        }

        std::vector<unsigned short> indices;
        indices.reserve(pMesh->mNumFaces * 3);
        for (unsigned int i = 0; i < pMesh->mNumFaces; i++)
        {
            const auto& face = pMesh->mFaces[i];
            assert(face.mNumIndices == 3);
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }

        AddStaticBind(std::make_unique<ZVertexBuffer>(gfx, vbuf));

        AddStaticIndexBuffer(std::make_unique<ZIndexBuffer>(gfx, indices));

        auto pvs = std::make_unique<ZVertexShader>(gfx, L"./x64/Debug/PhongVS.cso");
        auto pvsbc = pvs->GetBytecode();
        AddStaticBind(std::move(pvs));

        AddStaticBind(std::make_unique<ZPixelShader>(gfx, L"./x64/Debug/PhongPS.cso"));

        AddStaticBind(std::make_unique<ZInputLayout>(gfx, vbuf.GetLayout().GetD3DLayout(), pvsbc));

        AddStaticBind(std::make_unique<ZTopology>(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
    }
    else
    {
        SetIndexFromStatic();
    }

    struct PSMaterialConstant
    {
        DirectX::XMFLOAT3 color;
        float specularIntensity = 0.6f;
        float specularPower = 30.0f;
        float padding[3];
    } pmc;
    pmc.color = material;
    AddBind(std::make_unique<PSConstBuffer<PSMaterialConstant>>(gfx, pmc, 1u));

    AddBind(std::make_unique<ZTransformVSConstBuffer>(gfx, *this));
}