#pragma once
#include "SolidSphere.h"

class ZDirectionalLight
{
private:
    struct DirectionalLightCBuf
    {
        alignas(16) DirectX::XMFLOAT3 position;      // 시각화용 위치
        alignas(16) DirectX::XMFLOAT3 ambient;
        alignas(16) DirectX::XMFLOAT3 diffuseColor;
        float diffuseIntensity;
        alignas(16) DirectX::XMFLOAT3 direction;
        float pad;
    };

private:
    DirectionalLightCBuf cbData;
    mutable SolidSphere mesh;

public:
    ZDirectionalLight(ZGraphics& gfx, float radius = 0.5f);
    void SpawnControlWindow() noexcept;
    void Reset() noexcept;
    void Render(ZGraphics& gfx) const noxnd;
    void Bind(ZGraphics& gfx) const noexcept;
    std::string ToString(DirectionalLightCBuf lightInfo) const;
};
