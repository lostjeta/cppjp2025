#pragma once
#include "ZRenderableBase.h"

class SolidSphere : public ZRenderableBase<SolidSphere>
{
public:
    SolidSphere(ZGraphics& gfx, float radius);
    void Update(float dt) noexcept override;
    void SetPos(DirectX::XMFLOAT3 pos) noexcept;
    DirectX::XMMATRIX GetTransformXM() const noexcept override;
private:
    DirectX::XMFLOAT3 pos = { 1.0f,1.0f,1.0f };
};