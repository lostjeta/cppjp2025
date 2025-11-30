#pragma once
#include "ZBindable.h"

namespace Bind
{
    /**
     * @brief Rasterizer State 바인더
     * 
     * Winding Order, Culling Mode, Fill Mode 등을 설정합니다.
     */
    class ZRasterizer : public ZBindable
    {
    public:
        /**
         * @param gfx Graphics 객체
         * @param isTwoSided true면 양면 렌더링 (Cull None)
         * @param isCCW true면 CCW가 앞면 (Left-handed 좌표계)
         */
        ZRasterizer(ZGraphics& gfx, bool isTwoSided = false, bool isCCW = true);
        
        /**
         * @param gfx Graphics 객체
         * @param fillMode D3D11_FILL_SOLID 또는 D3D11_FILL_WIREFRAME
         * @param isTwoSided true면 양면 렌더링 (Cull None)
         * @param isCCW true면 CCW가 앞면 (Left-handed 좌표계)
         */
        ZRasterizer(ZGraphics& gfx, D3D11_FILL_MODE fillMode, bool isTwoSided = false, bool isCCW = true);
        
        void Bind(ZGraphics& gfx) noexcept override;
        
    protected:
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> pRasterizer;
    };
}
