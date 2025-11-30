#pragma once
#include "ZMath.h"
#include "ZRenderableBase.h"

template<class T>
class ZInteractableTransform : public ZRenderableBase<T>
{
public:
    ZInteractableTransform(ZGraphics& gfx, std::mt19937& rng,
        std::uniform_real_distribution<float>& adist,
        std::uniform_real_distribution<float>& ddist,
        std::uniform_real_distribution<float>& odist,
        std::uniform_real_distribution<float>& rdist)
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
    {}
    void Update(float dt) noexcept override
    {
        roll = wrap_angle(roll + droll * dt);
        pitch = wrap_angle(pitch + dpitch * dt);
        yaw = wrap_angle(yaw + dyaw * dt);
        theta = wrap_angle(theta + dtheta * dt);
        phi = wrap_angle(phi + dphi * dt);
        chi = wrap_angle(chi + dchi * dt);
    }
    DirectX::XMMATRIX GetTransformXM() const noexcept
    {
        return DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll) *    // 자전
            DirectX::XMMatrixTranslation(r, 0.0f, 0.0f) *                   // 공전 거리
            DirectX::XMMatrixRotationRollPitchYaw(theta, phi, chi);         // 공전
        //* DirectX::XMMatrixTranslation(0.0f, 0.0f, 20.0f); // 이제 카메라가 있기 때문에 직접 거리를 둘 필요 없다.
    }
protected:
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
};