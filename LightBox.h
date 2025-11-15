#pragma once
#include "ZRenderableBase.h"

class LightBox : public ZRenderableBase<LightBox>
{
private:
    float r; // box radius
    // camera center rotation
    float roll = 0.0f;  // z
    float pitch = 0.0f; // x
    float yaw = 0.0f;   // y
    // world center rotation
    float theta;
    float phi;
    float chi;

    // speed (delta/s)
    float droll;
    float dpitch;
    float dyaw;
    float dtheta;
    float dphi;
    float dchi;

public:
    LightBox(ZGraphics& gfx, std::mt19937& rng,
        std::uniform_real_distribution<float>& adist,
        std::uniform_real_distribution<float>& ddist,
        std::uniform_real_distribution<float>& odist,
        std::uniform_real_distribution<float>& rdist,
        std::uniform_real_distribution<float>& bdist,
        DirectX::XMFLOAT3 materialColor);
    void Update(float dt) noexcept override;
    DirectX::XMMATRIX GetTransformXM() const noexcept override;
};
