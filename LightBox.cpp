#include "ZD3D11.h"
#include "Cube.h"
#include "LightBox.h"
#include "ZBindableBase.h"
#include "GraphicsThrowMacros.h"


LightBox::LightBox(ZGraphics& gfx,
    std::mt19937& rng,
    std::uniform_real_distribution<float>& adist,
    std::uniform_real_distribution<float>& ddist,   // delta
    std::uniform_real_distribution<float>& odist,   // 
    std::uniform_real_distribution<float>& rdist,   // radius
    std::uniform_real_distribution<float>& bdist,   // radius
    DirectX::XMFLOAT3 materialColor)
    :
    r(rdist(rng)),
    droll(ddist(rng)),
    dpitch(ddist(rng)),
    dyaw(ddist(rng)),
    dphi(odist(rng)),
    dtheta(odist(rng)),
    dchi(odist(rng)),
    chi(adist(rng)),
    theta(adist(rng)),
    phi(adist(rng))
{
    if (!IsStaticInitialized())
    {
        struct Vertex
        {
            DirectX::XMFLOAT3 pos;
            DirectX::XMFLOAT3 n;
        };
        auto model = Cube::MakeIndependent<Vertex>();
        model.SetNormalsIndependentFlat();

        AddStaticBind(std::make_unique<Bind::ZVertexBuffer>(gfx, model.vertices));

        auto pvs = std::make_unique<Bind::ZVertexShader>(gfx, L"./x64/Debug/PhongVS.cso");
        auto pvsbc = pvs->GetBytecode();
        AddStaticBind(std::move(pvs));

        AddStaticBind(std::make_unique<Bind::ZPixelShader>(gfx, L"./x64/Debug/PhongPS.cso"));

        AddStaticIndexBuffer(std::make_unique<Bind::ZIndexBuffer>(gfx, model.indices));

        const std::vector<D3D11_INPUT_ELEMENT_DESC> ied =
        {
            { "Position",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 },
            { "Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        AddStaticBind(std::make_unique<Bind::ZInputLayout>(gfx, ied, pvsbc));

        AddStaticBind(std::make_unique<Bind::ZTopology>(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
    }
    else
    {
        SetIndexFromStatic();
    }

    AddBind(std::make_unique<Bind::ZTransformVSConstBuffer>(gfx, *this));

    struct PSMaterialConstant
    {
        alignas(16) DirectX::XMFLOAT3 color;
        float specularIntensity = 0.6;
        float specularPower = 30.0;
        float padding[2];
    } colorConst;
    colorConst.color = materialColor;
    AddBind(std::make_unique<Bind::PSConstBuffer<PSMaterialConstant>>(gfx, colorConst, 1u));
}

void LightBox::Update(float dt) noexcept
{
    roll += droll * dt;
    pitch += dpitch * dt;
    yaw += dyaw * dt;
    theta += dtheta * dt;
    phi += dphi * dt;
    chi += dchi * dt;
}

DirectX::XMMATRIX LightBox::GetTransformXM() const noexcept
{
    return DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll) *
        DirectX::XMMatrixTranslation(r, 0.0f, 0.0f) *
        DirectX::XMMatrixRotationRollPitchYaw(theta, phi, chi);
    //* DirectX::XMMatrixTranslation(0.0f, 0.0f, 20.0f); // 이제 카메라가 있기 때문에 직접 거리를 둘 필요 없다.
}
