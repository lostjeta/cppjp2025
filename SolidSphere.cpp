#include "ZD3D11.h"
#include "SolidSphere.h"
#include "ZBindableBase.h"
#include "GraphicsThrowMacros.h"
#include "ZVertex.h"
#include "Sphere.h"


SolidSphere::SolidSphere(ZGraphics& gfx, float radius)
{
    using namespace Bind;
    namespace dx = DirectX;

    if (!IsStaticInitialized())
    {
        struct Vertex
        {
            dx::XMFLOAT3 pos;
        };
        auto model = Sphere::Make<Vertex>();
        model.Transform(dx::XMMatrixScaling(radius, radius, radius));
        AddStaticBind(std::make_unique<ZVertexBuffer>(gfx, model.vertices));
        AddStaticIndexBuffer(std::make_unique<ZIndexBuffer>(gfx, model.indices));

        auto pvs = std::make_unique<ZVertexShader>(gfx, L"./x64/Debug/SolidVS.cso");
        auto pvsbc = pvs->GetBytecode();
        AddStaticBind(std::move(pvs));

        AddStaticBind(std::make_unique<ZPixelShader>(gfx, L"./x64/Debug/SolidPS.cso"));

        struct PSColorConstant
        {
            dx::XMFLOAT3 color = { 1.0f,1.0f,1.0f };
            float padding;
        } colorConst;
        AddStaticBind(std::make_unique<PSConstBuffer<PSColorConstant>>(gfx, colorConst));

        const std::vector<D3D11_INPUT_ELEMENT_DESC> ied =
        {
            { "Position",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 },
        };
        AddStaticBind(std::make_unique<ZInputLayout>(gfx, ied, pvsbc));

        AddStaticBind(std::make_unique<ZTopology>(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
    }
    else
    {
        SetIndexFromStatic();
    }

    AddBind(std::make_unique<ZTransformVSConstBuffer>(gfx, *this));
}

void SolidSphere::Update(float dt) noexcept {}

void SolidSphere::SetPos(DirectX::XMFLOAT3 pos) noexcept
{
    this->pos = pos;
}

DirectX::XMMATRIX SolidSphere::GetTransformXM() const noexcept
{
    return DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
}