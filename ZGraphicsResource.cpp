#include "ZD3D11.h"
#include "ZGraphicsResource.h"
#include <stdexcept>


ID3D11DeviceContext* ZGraphicsResource::GetContext(ZGraphics& gfx) noexcept
{
	return gfx.pContext.Get();
}

ID3D11Device* ZGraphicsResource::GetDevice(ZGraphics& gfx) noexcept
{
	return gfx.pDevice.Get();
}

ID3D11RenderTargetView* ZGraphicsResource::GetTarget(ZGraphics& gfx) noexcept
{
    return gfx.pTarget.Get();
}

ID3D11BlendState* ZGraphicsResource::GetBlendState(ZGraphics& gfx) noexcept
{
    return gfx.pBlendState.Get();
}

DxgiInfoManager& ZGraphicsResource::GetInfoManager(ZGraphics& gfx)
{
#ifndef NDEBUG
    return gfx.infoManager;
#else
    throw std::logic_error("Error! (tried to access gfx.infoManager in Release config)");
#endif
}

DWORD ZGraphicsResource::GetClientWidth(ZGraphics& gfx)
{
    return gfx.m_ClientWidth;
}

DWORD ZGraphicsResource::GetClientHeight(ZGraphics& gfx)
{
    return gfx.m_ClientHeight;
}

double ZGraphicsResource::GetWinRatio(ZGraphics& gfx)
{
    return gfx.winRatio;
}
