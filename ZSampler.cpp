#include "ZD3D11.h"
#include "ZSampler.h"
#include "GraphicsThrowMacros.h"


namespace Bind
{
    ZSampler::ZSampler(ZGraphics& gfx)
    {
        INFOMAN(gfx);

        D3D11_SAMPLER_DESC samplerDesc = {};
        //samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        //samplerDesc.BorderColor[0] = 0.0f; // R
        //samplerDesc.BorderColor[1] = 1.0f; // G
        //samplerDesc.BorderColor[2] = 0.0f; // B
        //samplerDesc.BorderColor[3] = 0.0f; // A
        //samplerDesc.MaxAnisotropy
        //samplerDesc.MaxLOD

        GFX_THROW_INFO(GetDevice(gfx)->CreateSamplerState(&samplerDesc, &pSampler));
    }

    void ZSampler::Bind(ZGraphics& gfx) noexcept
    {
        GetContext(gfx)->PSSetSamplers(0, 1, pSampler.GetAddressOf());
    }
}

