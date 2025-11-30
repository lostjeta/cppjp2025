#include "ZD3D11.h"
#include "Cylinder.h"
#include "Prism.h"
#include "ZBindableBase.h"
#include "GraphicsThrowMacros.h"


Cylinder::Cylinder(ZGraphics& gfx, std::mt19937& rng,
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
        auto pvs = std::make_unique<Bind::ZVertexShader>(gfx, L"./x64/Debug/PhongVS.cso");
        auto pvsbc = pvs->GetBytecode();
        AddStaticBind(std::move(pvs));

        AddStaticBind(std::make_unique<Bind::ZPixelShader>(gfx, L"./x64/Debug/IndexedPhongPS.cso"));

        const std::vector<D3D11_INPUT_ELEMENT_DESC> ied =
        {
            { "Position",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 },
            { "Normal",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0 },
        };
        AddStaticBind(std::make_unique<Bind::ZInputLayout>(gfx, ied, pvsbc));

        AddStaticBind(std::make_unique<Bind::ZTopology>(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));

        struct PSMaterialConstant
        {
            dx::XMFLOAT3A colors[6] = {
                {1.0f,0.0f,0.0f},
                {0.0f,1.0f,0.0f},
                {0.0f,0.0f,1.0f},
                {1.0f,1.0f,0.0f},
                {1.0f,0.0f,1.0f},
                {0.0f,1.0f,1.0f}, // CPU에서는 마지막에도 padding 추가되어 alignas(16) 맞춰준다.
            };
            float specularIntensity = 0.6f;
            float specularPower = 30.0f;
        } matConst;
        AddStaticBind(std::make_unique<Bind::PSConstBuffer<PSMaterialConstant>>(gfx, matConst, 1u));
    }
    //else
    //{
    //    SetIndexFromStatic();
    //}

    struct Vertex
    {
        dx::XMFLOAT3 pos;
        dx::XMFLOAT3 n;
    };
    auto model = Prism::MakeTesselatedIndependentCapNormals<Vertex>(tdist(rng));

    AddBind(std::make_unique<Bind::ZVertexBuffer>(gfx, model.vertices));
    AddIndexBuffer(std::make_unique<Bind::ZIndexBuffer>(gfx, model.indices));

    AddBind(std::make_unique<Bind::ZTransformVSConstBuffer>(gfx, *this));
}