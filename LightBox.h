#pragma once
#include "ZInteractableTransform.h"

class LightBox : public ZInteractableTransform<LightBox>
{
private:
    struct PSMaterialConstant
    {
        DirectX::XMFLOAT3 color;
        float specularIntensity = 0.6f;
        float specularPower = 5.0; // 5:플라스틱,무광택, 30:금속/유리, 100:거울/크롬
        float padding[3];
    } materialConstants;
    using MaterialCbuf = Bind::PSConstBuffer<PSMaterialConstant>;

    // model transform
    DirectX::XMFLOAT3X3 mt;

private:
    void SyncMaterial(ZGraphics& gfx) noexcept(!IS_DEBUG);
public:
    LightBox(ZGraphics& gfx, std::mt19937& rng,
        std::uniform_real_distribution<float>& adist,
        std::uniform_real_distribution<float>& ddist,
        std::uniform_real_distribution<float>& odist,
        std::uniform_real_distribution<float>& rdist,
        std::uniform_real_distribution<float>& bdist,
        DirectX::XMFLOAT3 materialColor);
    DirectX::XMMATRIX GetTransformXM() const noexcept override;
    bool SpawnControlWindow(int id, ZGraphics& gfx) noexcept;
};
