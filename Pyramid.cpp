#include "ZD3D11.h"
#include "Pyramid.h"
#include "ZBindableBase.h"
#include "GraphicsThrowMacros.h"
#include "Cone.h"
#include <array>

using namespace Bind;

Pyramid::Pyramid(ZGraphics& gfx, std::mt19937& rng,
    std::uniform_real_distribution<float>& adist,
    std::uniform_real_distribution<float>& ddist,
    std::uniform_real_distribution<float>& odist,
    std::uniform_real_distribution<float>& rdist,
    std::uniform_int_distribution<int>& tdist)
    :
    ZInteractableTransform(gfx, rng, adist, ddist, odist, rdist)
{
    namespace dx = DirectX;

    if (!IsStaticInitialized())
    {
        auto pvs = std::make_unique<ZVertexShader>(gfx, L"./x64/Debug/BlendedPhongVS.cso");
        auto pvsbc = pvs->GetBytecode();
        AddStaticBind(std::move(pvs));

        AddStaticBind(std::make_unique<ZPixelShader>(gfx, L"./x64/Debug/BlendedPhongPS.cso"));

        const std::vector<D3D11_INPUT_ELEMENT_DESC> ied =
        {
            { "Position",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 },
            { "Normal",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0 },
            { "Color",0,DXGI_FORMAT_R8G8B8A8_UNORM,0,24,D3D11_INPUT_PER_VERTEX_DATA,0 },
        };
        AddStaticBind(std::make_unique<ZInputLayout>(gfx, ied, pvsbc));

        AddStaticBind(std::make_unique<ZTopology>(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));

        struct PSMaterialConstant
        {
            float specularIntensity = 0.6f;
            float specularPower = 30.0f;
            float padding[2];
        } colorConst;
        AddStaticBind(std::make_unique<PSConstBuffer<PSMaterialConstant>>(gfx, colorConst, 1u /*slot1*/));
    }
    //else
    //{
    //    SetIndexFromStatic();
    //}

    struct Vertex
    {
        dx::XMFLOAT3 pos;
        dx::XMFLOAT3 n;
        std::array<char, 4> color;
        char padding;
    };
    const auto tesselation = tdist(rng);
    auto model = Cone::MakeTesselatedIndependentFaces<Vertex>(tesselation);
    // set vertex colors for mesh (tip red blending to white base)
    for (auto& v : model.vertices)
    {
        v.color = { (char)10,(char)255,(char)255 };
    }
    for (int i = 0; i < tesselation; i++)
    {
        model.vertices[i * 3].color = { (char)255,(char)10,(char)10 };
    }
    // squash mesh a bit in the z direction
    model.Transform(dx::XMMatrixScaling(1.0f, 1.0f, 0.7f));
    // add normals
    model.SetNormalsIndependentFlat();

    AddBind(std::make_unique<ZVertexBuffer>(gfx, model.vertices));
    AddIndexBuffer(std::make_unique<ZIndexBuffer>(gfx, model.indices));

    AddBind(std::make_unique<ZTransformVSConstBuffer>(gfx, *this));
}