#pragma once
#include "ZBindable.h"

namespace Bind
{
    class ZSampler : public ZBindable
    {
    public:
        ZSampler(ZGraphics& gfx);
        void Bind(ZGraphics& gfx) noexcept override;

    protected:
        Microsoft::WRL::ComPtr<ID3D11SamplerState> pSampler;
    };
}
