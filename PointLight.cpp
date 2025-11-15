#include "ZD3D11.h"
#include "PointLight.h"
#include "imgui/imgui.h"
#include "ZMath.h"
#include "ZCamera.h"

PointLight::PointLight(ZGraphics& gfx, float radius)
    :
    mesh(gfx, radius),
    cbuf(gfx)
{
    Reset();
}

void PointLight::SpawnControlWindow() noexcept
{
    if (ImGui::Begin("Light"))
    {
        ImGui::Text("Position");
        ImGui::SliderFloat("X", &cbData.pos.x, -60.0f, 60.0f, "%.1f");
        ImGui::SliderFloat("Y", &cbData.pos.y, -60.0f, 60.0f, "%.1f");
        ImGui::SliderFloat("Z", &cbData.pos.z, -60.0f, 60.0f, "%.1f");

        ImGui::Text("Intensity/Color");
        ImGui::SliderFloat("Intensity", &cbData.diffuseIntensity, 0.01f, 2.0f, "%.2f", 0);
        ImGui::ColorEdit3("Diffuse Color", &cbData.diffuseColor.x);
        ImGui::ColorEdit3("Ambient", &cbData.ambient.x);
        //ImGui::ColorEdit3("Material", &cbData.materialColor.x);

        ImGui::Text("Falloff");
        ImGui::SliderFloat("Constant", &cbData.attConst, 0.05f, 10.0f, "%.2f", 1);
        ImGui::SliderFloat("Linear", &cbData.attLin, 0.0001f, 4.0f, "%.4f", 1);
        ImGui::SliderFloat("Quadratic", &cbData.attQuad, 0.0000001f, 10.0f, "%.7f", 1);

        if (ImGui::Button("Reset"))
        {
            Reset();
        }
    }
    ImGui::End();
}

void PointLight::Reset() noexcept
{
    cbData = {
        { 0.0f,0.0f,0.0f }, // light pos
        { 0.05f,0.05f,0.05f }, // ambient
        { 1.0f,1.0f,1.0f }, // diffuse
        1.0f,
        1.0f,
        0.045f,
        0.0075f,
    };
}

void PointLight::Render(ZGraphics& gfx) const noxnd
{
    mesh.SetPos(cbData.pos);
    mesh.Render(gfx);
}
/**
 * @brief Point Light 클래스의 주석을 나타내는 함수
 *
 * @return Point Light의 주석을 나타내는 문자열
 */
std::string PointLight::ToString() const
{
    std::stringstream ss;
    ss << "Point Light: pos=(" << cbData.pos.x << ", " << cbData.pos.y << ", " << cbData.pos.z << "), ambient=(" << cbData.ambient.x << ", " << cbData.ambient.y << ", " << cbData.ambient.z << "), diffuseColor=(" << cbData.diffuseColor.x << ", " << cbData.diffuseColor.y << ", " << cbData.diffuseColor.z << "), intensity=" << cbData.diffuseIntensity << ", attConst=" << cbData.attConst << ", attLin=" << cbData.attLin << ", attQuad=" << cbData.attQuad;
    return ss.str();
}

void PointLight::Bind(ZGraphics& gfx, DirectX::FXMMATRIX view) const noexcept
{
    auto dataCopy = cbData;
    const auto pos = DirectX::XMLoadFloat3(&cbData.pos);
    DirectX::XMStoreFloat3(&dataCopy.pos, DirectX::XMVector3Transform(pos, view));
    cbuf.Update(gfx, dataCopy);
    cbuf.Bind(gfx);
}