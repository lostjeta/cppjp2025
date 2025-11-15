#pragma once
#include "ZConstBuffer.h"
#include "ZRenderable.h"
#include <DirectXMath.h>

namespace Bind
{
    class ZTransformVSConstBuffer : public ZBindable
    {
    private:
        struct Transforms
        {
            DirectX::XMMATRIX modelViewProj;
            DirectX::XMMATRIX model;
        };
    private:
        // VSConstBuffer는 매프래임마다 다시 계산하기 때문에 재활용한다.
        static std::unique_ptr<Bind::VSConstBuffer<ZTransformVSConstBuffer::Transforms>> pVcbuf;
        const ZRenderable& parent;

    public:
        ZTransformVSConstBuffer(ZGraphics& gfx, const ZRenderable& parent, UINT slog = 0);
        void Bind(ZGraphics& gfx) noexcept override;
    };
}