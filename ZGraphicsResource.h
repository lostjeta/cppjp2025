#pragma once

class ZGraphics;
class DxgiInfoManager;

class ZGraphicsResource
{
protected: // ZGraphics friend 접근 제약에 사용
	static ID3D11DeviceContext* GetContext(ZGraphics& gfx) noexcept;
	static ID3D11Device* GetDevice(ZGraphics& gfx) noexcept;
    static ID3D11RenderTargetView* GetTarget(ZGraphics& gfx) noexcept;
    static ID3D11BlendState* GetBlendState(ZGraphics& gfx) noexcept;
	static DxgiInfoManager& GetInfoManager(ZGraphics& gfx);
    static DWORD GetClientWidth(ZGraphics& gfx);
    static DWORD GetClientHeight(ZGraphics& gfx);
    static double GetWinRatio(ZGraphics& gfx);
};