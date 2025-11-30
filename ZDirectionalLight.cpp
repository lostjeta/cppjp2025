#include "ZD3D11.h"
#include "ZDirectionalLight.h"
#include "imgui/imgui.h"
#include <sstream>

ZDirectionalLight::ZDirectionalLight(ZGraphics& gfx, float radius)
    :
    mesh(gfx, radius)
{
    Reset();
}

void ZDirectionalLight::SpawnControlWindow() noexcept
{
    if (ImGui::Begin("Directional Light"))
    {
        ImGui::Text("Position (Visualization)");
        ImGui::SliderFloat("Pos X", &cbData.position.x, -60.0f, 60.0f, "%.1f");
        ImGui::SliderFloat("Pos Y", &cbData.position.y, -60.0f, 60.0f, "%.1f");
        ImGui::SliderFloat("Pos Z", &cbData.position.z, -60.0f, 60.0f, "%.1f");

        ImGui::Text("Direction");
        ImGui::SliderFloat("Dir X", &cbData.direction.x, -1.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Dir Y", &cbData.direction.y, -1.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Dir Z", &cbData.direction.z, -1.0f, 1.0f, "%.2f");

        ImGui::Text("Intensity/Color");
        ImGui::SliderFloat("Intensity", &cbData.diffuseIntensity, 0.0f, 2.0f, "%.2f");
        ImGui::ColorEdit3("Diffuse Color", &cbData.diffuseColor.x);
        ImGui::ColorEdit3("Ambient", &cbData.ambient.x);

        if (ImGui::Button("Reset"))
        {
            Reset();
        }
    }
    ImGui::End();
}

void ZDirectionalLight::Reset() noexcept
{
    cbData = {
        { 0.0f, 0.0f, -10.0f },     // position (시각화용, 위쪽)
        { 0.2f, 0.2f, 0.2f },      // ambient (약한 ambient)
        { 1.0f, 1.0f, 1.0f },      // diffuse color (흰색)
        0.9f,                       // diffuse intensity (약함)
        { 0.0f, -1.0f, 1.0f },     // direction (위에서 아래로, 약간 앞쪽)
        0.0f                        // pad
    };
}

void ZDirectionalLight::Render(ZGraphics& gfx) const noxnd
{
    mesh.SetPos(cbData.position);
    mesh.Render(gfx);
}

std::string ZDirectionalLight::ToString(DirectionalLightCBuf lightInfo) const
{
    std::stringstream ss;
    ss << "Directional Light: direction=(" << lightInfo.direction.x << ", " 
       << lightInfo.direction.y << ", " << lightInfo.direction.z 
       << "), ambient=(" << lightInfo.ambient.x << ", " << lightInfo.ambient.y << ", " 
       << lightInfo.ambient.z << "), diffuseColor=(" << lightInfo.diffuseColor.x << ", " 
       << lightInfo.diffuseColor.y << ", " << lightInfo.diffuseColor.z 
       << "), intensity=" << lightInfo.diffuseIntensity;
    return ss.str();
}

void ZDirectionalLight::Bind(ZGraphics& gfx) const noexcept
{
    // ZGraphics에 DirectionalLight 정보 전달
    ZGraphics::DirectionalLightData lightData;
    lightData.ambient = DirectX::XMFLOAT4{ cbData.ambient.x, cbData.ambient.y, cbData.ambient.z, 1.0f };
    lightData.diffuse = DirectX::XMFLOAT4{ 
        cbData.diffuseColor.x * cbData.diffuseIntensity, 
        cbData.diffuseColor.y * cbData.diffuseIntensity, 
        cbData.diffuseColor.z * cbData.diffuseIntensity, 
        1.0f 
    };
    lightData.specular = DirectX::XMFLOAT4{ 0.2f, 0.2f, 0.2f, 1.0f };
    lightData.direction = cbData.direction;
    
    gfx.SetDirectionalLight(lightData);
}
