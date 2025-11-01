#pragma once
#include "ZBindable.h"

namespace Bind
{
    class ZIndexBuffer : public ZBindable
    {
    protected:
        UINT count;
        Microsoft::WRL::ComPtr<ID3D11Buffer> pIndexBuffer;

    public:
        ZIndexBuffer(ZGraphics& gfx, const std::vector<unsigned short>& indices);
        void Bind(ZGraphics& gfx) noexcept override;
        UINT GetCount() const noexcept;
    };
}