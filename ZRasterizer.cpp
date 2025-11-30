#include "ZD3D11.h"
#include "ZRasterizer.h"
#include "GraphicsThrowMacros.h"

namespace Bind
{
    ZRasterizer::ZRasterizer(ZGraphics& gfx, bool isTwoSided, bool isCCW)
    {
        INFOMAN(gfx);

        D3D11_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        
        if (isTwoSided)
        {
            // 양면 렌더링 (Culling 없음)
            rasterDesc.CullMode = D3D11_CULL_NONE;
        }
        else
        {
            // 뒷면 제거
            rasterDesc.CullMode = D3D11_CULL_BACK;
        }
        
        // Winding Order 설정
        // TRUE: CCW가 앞면 (Left-handed, aiProcess_ConvertToLeftHanded)
        // FALSE: CW가 앞면 (Right-handed, D3D11 기본값)
        rasterDesc.FrontCounterClockwise = isCCW ? TRUE : FALSE;
        
        rasterDesc.DepthBias = 0;
        rasterDesc.DepthBiasClamp = 0.0f;
        rasterDesc.SlopeScaledDepthBias = 0.0f;
        rasterDesc.DepthClipEnable = TRUE;
        rasterDesc.ScissorEnable = FALSE;
        rasterDesc.MultisampleEnable = FALSE;
        rasterDesc.AntialiasedLineEnable = FALSE;

        GFX_THROW_INFO(GetDevice(gfx)->CreateRasterizerState(&rasterDesc, &pRasterizer));
    }

    ZRasterizer::ZRasterizer(ZGraphics& gfx, D3D11_FILL_MODE fillMode, bool isTwoSided, bool isCCW)
    {
        INFOMAN(gfx);

        D3D11_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.FillMode = fillMode;
        
        if (isTwoSided)
        {
            // 양면 렌더링 (Culling 없음)
            rasterDesc.CullMode = D3D11_CULL_NONE;
        }
        else
        {
            // 뒷면 제거
            rasterDesc.CullMode = D3D11_CULL_BACK;
        }
        
        // Winding Order 설정
        // TRUE: CCW가 앞면 (Left-handed, aiProcess_ConvertToLeftHanded)
        // FALSE: CW가 앞면 (Right-handed, D3D11 기본값)
        rasterDesc.FrontCounterClockwise = isCCW ? TRUE : FALSE;
        
        rasterDesc.DepthBias = 0;
        rasterDesc.DepthBiasClamp = 0.0f;
        rasterDesc.SlopeScaledDepthBias = 0.0f;
        rasterDesc.DepthClipEnable = TRUE;
        rasterDesc.ScissorEnable = FALSE;
        rasterDesc.MultisampleEnable = FALSE;
        rasterDesc.AntialiasedLineEnable = FALSE;

        GFX_THROW_INFO(GetDevice(gfx)->CreateRasterizerState(&rasterDesc, &pRasterizer));
    }

    void ZRasterizer::Bind(ZGraphics& gfx) noexcept
    {
        GetContext(gfx)->RSSetState(pRasterizer.Get());
    }
}
