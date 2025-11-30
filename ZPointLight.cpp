#include "ZD3D11.h"
#include "ZPointLight.h"
#include "imgui/imgui.h"
#include "ZMath.h"
#include "ZCamera.h"

ZPointLight::ZPointLight(ZGraphics& gfx, float radius)
    :
    mesh(gfx, radius),
    cbuf(gfx)
{
    Reset();
}

void ZPointLight::SpawnControlWindow() noexcept
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

void ZPointLight::Reset() noexcept
{
    /*
     * 이 구조체는 PointLight CBuf에 업데이트 되어 사용된다.
     * 이 정보는 ImGui를 이용하여 사용자 입력을 받아 업데이트 된다.
     */
    cbData = {
        { 0.0f,0.0f,0.0f },     // pos: 라이트의 위치
        { 0.05f,0.05f,0.05f },  // ambient: 라이트의 환경광
        { 1.0f,1.0f,1.0f },     // diffuseColor: 라이트의 발산광
        1.0f,                   // diffuseIntensity: 라이트의 발산광 강도
        1.0f,                   // attConst: 라이트의 거리에 따른 감산 상수
        0.045f,                 // attLin: 라이트의 거리에 따른 감산 계수
        0.0075f,                // attQuad: 라이트의 거리에 따른 감산 제곱 계수
    };
}

void ZPointLight::Render(ZGraphics& gfx) const noxnd
{
    mesh.SetPos(cbData.pos);
    mesh.Render(gfx);
}
/**
 * @brief Point Light 클래스의 주석을 나타내는 함수
 *
 * @return Point Light의 주석을 나타내는 문자열
 */
std::string ZPointLight::ToString(PointLightCBuf lightInfo) const
{
    std::stringstream ss;
    ss << "Point Light: pos=(" << lightInfo.pos.x << ", " << lightInfo.pos.y << ", " << lightInfo.pos.z << "), ambient=(" << lightInfo.ambient.x << ", " << lightInfo.ambient.y << ", " << lightInfo.ambient.z << "), diffuseColor=(" << lightInfo.diffuseColor.x << ", " << lightInfo.diffuseColor.y << ", " << lightInfo.diffuseColor.z << "), intensity=" << lightInfo.diffuseIntensity << ", attConst=" << lightInfo.attConst << ", attLin=" << lightInfo.attLin << ", attQuad=" << lightInfo.attQuad;
    return ss.str();
}

void ZPointLight::Bind(ZGraphics& gfx, DirectX::FXMMATRIX view) const noexcept
{
    //std::cout << ToString(cbData) << std::endl;
    auto dataCopy = cbData;
    const auto pos = DirectX::XMLoadFloat3(&cbData.pos);
    DirectX::XMStoreFloat3(&dataCopy.pos, DirectX::XMVector3Transform(pos, view)); // light 위치를 상대적 카메라 위치로 맞춘다.
    //std::cout << ToString(dataCopy) << std::endl;
    cbuf.Update(gfx, dataCopy);
    
    // ZGraphics에 PointLight 정보 전달 (Alice 등 다른 객체에서 사용)
    ZGraphics::PointLightData lightData;
    lightData.ambient = DirectX::XMFLOAT4{ cbData.ambient.x, cbData.ambient.y, cbData.ambient.z, 1.0f };
    lightData.diffuse = DirectX::XMFLOAT4{ cbData.diffuseColor.x * cbData.diffuseIntensity, 
                                           cbData.diffuseColor.y * cbData.diffuseIntensity, 
                                           cbData.diffuseColor.z * cbData.diffuseIntensity, 1.0f };
    lightData.specular = DirectX::XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
    lightData.position = cbData.pos;  // World space position
    lightData.range = 100.0f;
    lightData.att = DirectX::XMFLOAT3{ cbData.attConst, cbData.attLin, cbData.attQuad };
    gfx.SetPointLight(lightData);
    cbuf.Bind(gfx);
}