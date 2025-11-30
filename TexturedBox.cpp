#include "ZD3D11.h"
#include "Cube.h"
#include "TexturedBox.h"
#include "ZBindableBase.h"
#include "GraphicsThrowMacros.h"
#include "Surface.h"
#include "ZTexture.h"
#include "ZSampler.h"


using namespace Bind;

TexturedBox::TexturedBox(ZGraphics& gfx,
    std::mt19937& rng,
    std::uniform_real_distribution<float>& adist,
    std::uniform_real_distribution<float>& ddist,
    std::uniform_real_distribution<float>& odist,
    std::uniform_real_distribution<float>& rdist)
    :
    ZInteractableTransform(gfx, rng, adist, ddist, odist, rdist)
{
    namespace dx = DirectX;

    if (!IsStaticInitialized())
    {
        struct Vertex
        {
            dx::XMFLOAT3 pos;
            dx::XMFLOAT3 n;
            dx::XMFLOAT2 tc;
        };
        //const auto model = Cube::MakeSkinned<Vertex>();
        auto model = Cube::MakeIndependentTextured<Vertex>();
        model.SetNormalsIndependentFlat();

        AddStaticBind(std::make_unique<ZVertexBuffer>(gfx, model.vertices));

        AddStaticBind(std::make_unique<Bind::ZTexture>(gfx, L"./Data/Images/crate.bmp"));

        AddStaticBind(std::make_unique<Bind::ZSampler>(gfx));

        auto pvs = std::make_unique<ZVertexShader>(gfx, L"./x64/Debug/TexturedPhongVS.cso");
        auto pvsbc = pvs->GetBytecode();
        AddStaticBind(std::move(pvs));

        AddStaticBind(std::make_unique<ZPixelShader>(gfx, L"./x64/Debug/TexturedPhongPS.cso"));

        AddStaticIndexBuffer(std::make_unique<ZIndexBuffer>(gfx, model.indices));

        const std::vector<D3D11_INPUT_ELEMENT_DESC> ied =
        {
            { "Position",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 },
            { "Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            { "TexCoord",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0 },
        };
        AddStaticBind(std::make_unique<ZInputLayout>(gfx, ied, pvsbc));

        AddStaticBind(std::make_unique<ZTopology>(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));

        struct PSMaterialConstant
        {
            float specularIntensity = 0.6f;
            float specularPower = 30.0f;
            float padding[2];
        } colorConst;
        AddStaticBind(std::make_unique<PSConstBuffer<PSMaterialConstant>>(gfx, colorConst, 1u));
    }
    else
    {
        SetIndexFromStatic();
    }

    // model scaling transform (per instance, not stored as bind)
    DirectX::XMStoreFloat3x3(
        &mt,
        DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f)
    );

    AddBind(std::make_unique<ZTransformVSConstBuffer>(gfx, *this));
}

DirectX::XMMATRIX TexturedBox::GetTransformXM() const noexcept
{
    namespace dx = DirectX;
    return dx::XMLoadFloat3x3(&mt) * ZInteractableTransform::GetTransformXM();
}