#include "ZD3D11.h"
#include "ZTextureSRV.h"

namespace Bind
{
    ZTextureSRV::ZTextureSRV(ZGraphics& gfx, 
                             Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv,
                             UINT slot)
        :
        pTextureSRV(srv),
        m_Slot(slot)
    {
        // SRV는 외부에서 생성되어 전달됨
    }
    
    void ZTextureSRV::Bind(ZGraphics& gfx) noexcept
    {
        // Pixel Shader에 텍스처 바인딩
        GetContext(gfx)->PSSetShaderResources(m_Slot, 1u, pTextureSRV.GetAddressOf());
    }
}
