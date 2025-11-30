#pragma once
#include "ZInteractableTransform.h"

class TexturedBox : public ZInteractableTransform<TexturedBox>
{
private:
    DirectX::XMFLOAT3X3 mt;

public:
    TexturedBox(ZGraphics& gfx, std::mt19937& rng,
        std::uniform_real_distribution<float>& adist,
        std::uniform_real_distribution<float>& ddist,
        std::uniform_real_distribution<float>& odist,
        std::uniform_real_distribution<float>& rdist);
    DirectX::XMMATRIX GetTransformXM() const noexcept override;
};