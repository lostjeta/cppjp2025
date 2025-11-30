#include "ZD3D11.h"
#include "Cube.h"
#include "ZBindableBase.h"
#include "LightBox.h"
#include "imgui/imgui.h"
#include "GraphicsThrowMacros.h"


void LightBox::SyncMaterial(ZGraphics& gfx) noexcept(!IS_DEBUG)
{
    auto pConstPS = QueryBindable<MaterialCbuf>(); // 특정 상수 버퍼 획득
    assert(pConstPS != nullptr);
    pConstPS->Update(gfx, materialConstants); // 수정된 최신 정보로 갱신
}

LightBox::LightBox(ZGraphics& gfx,
    std::mt19937& rng,
    std::uniform_real_distribution<float>& adist,
    std::uniform_real_distribution<float>& ddist,   // delta
    std::uniform_real_distribution<float>& odist,   // 
    std::uniform_real_distribution<float>& rdist,   // radius
    std::uniform_real_distribution<float>& bdist,
    DirectX::XMFLOAT3 materialColor)
    :
    ZInteractableTransform(gfx, rng, adist, ddist, odist, rdist)
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

    materialConstants.color = materialColor;
    AddBind(std::make_unique<Bind::PSConstBuffer<PSMaterialConstant>>(gfx, materialConstants, 1u /*slot1*/));

    // model deformation transform (per instance, not stored as bind)
    DirectX::XMStoreFloat3x3(
        &mt,
        DirectX::XMMatrixScaling(1.0f, 1.0f, bdist(rng))
    );

    AddBind(std::make_unique<Bind::ZTransformVSConstBuffer>(gfx, *this));
}

DirectX::XMMATRIX LightBox::GetTransformXM() const noexcept
{
    namespace dx = DirectX;
    return dx::XMLoadFloat3x3(&mt) * ZInteractableTransform::GetTransformXM();
}

bool LightBox::SpawnControlWindow(int id, ZGraphics& gfx) noexcept
{
    using namespace std::string_literals;

    bool dirty = false;
    bool open = true;
    if (ImGui::Begin(("Box "s + std::to_string(id)).c_str(), &open))
    {
        ImGui::Text("Material Properties");
        const auto cd = ImGui::ColorEdit3("Material Color", &materialConstants.color.x);
        const auto sid = ImGui::SliderFloat("Specular Intensity", &materialConstants.specularIntensity, 0.05f, 4.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        const auto spd = ImGui::SliderFloat("Specular Power", &materialConstants.specularPower, 1.0f, 200.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        dirty = cd || sid || spd;

        ImGui::Text("Position");
        ImGui::SliderFloat("R", &r, 0.0f, 80.0f, "%.1f");
        ImGui::SliderAngle("Theta", &theta, -180.0f, 180.0f);
        ImGui::SliderAngle("Phi", &phi, -180.0f, 180.0f);
        ImGui::Text("Orientation");
        ImGui::SliderAngle("Roll", &roll, -180.0f, 180.0f);
        ImGui::SliderAngle("Pitch", &pitch, -180.0f, 180.0f);
        ImGui::SliderAngle("Yaw", &yaw, -180.0f, 180.0f);
    }
    ImGui::End();

    if (dirty)
    {
        SyncMaterial(gfx);
    }

    return open;
}