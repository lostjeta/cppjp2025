#pragma once
#include "ZBindable.h"
#include <d3d11.h>
#include <wrl.h>

namespace Bind
{
    /**
     * @brief 이미 생성된 ShaderResourceView를 바인딩하는 클래스
     * 
     * ZTexture와 달리 파일에서 로딩하지 않고,
     * 외부에서 생성된 SRV를 직접 받아서 바인딩합니다.
     * AnimationTest의 LoadMaterials에서 사용.
     */
    class ZTextureSRV : public ZBindable
    {
    public:
        /**
         * @param gfx Graphics 객체
         * @param srv 이미 생성된 ShaderResourceView (ComPtr로 관리됨)
         * @param slot Texture register slot (기본값: 0, t0)
         */
        ZTextureSRV(ZGraphics& gfx, 
                    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv,
                    UINT slot = 0u);
        
        void Bind(ZGraphics& gfx) noexcept override;
        
        ID3D11ShaderResourceView* GetSRV() const noexcept { return pTextureSRV.Get(); }
        
    protected:
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pTextureSRV;
        UINT m_Slot;
    };
}
