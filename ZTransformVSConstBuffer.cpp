#include "ZD3D11.h"
#include "ZTransformVSConstBuffer.h"

namespace Bind
{
    ZTransformVSConstBuffer::ZTransformVSConstBuffer(ZGraphics& gfx, const ZRenderable& parent, UINT slot)
        :
        parent(parent)
    {
        if (!pVcbuf) // 한번만 생성한다.
        {
            pVcbuf = std::make_unique<Bind::VSConstBuffer<Transforms>>(gfx, slot);
        }
    }

    void ZTransformVSConstBuffer::Bind(ZGraphics& gfx) noexcept
    {
        const auto modelView = parent.GetTransformXM() * gfx.GetCamera();
        const Transforms tf =
        {
            // 전치행렬 사용 이유 : D3D(행벡터*행렬) --> Shader(행렬*열벡터) 전치행렬 관계다.
            DirectX::XMMatrixTranspose(modelView),
            DirectX::XMMatrixTranspose(
                modelView *
                gfx.GetProjection()
            )
        };
        pVcbuf->Update(gfx, tf);
        pVcbuf->Bind(gfx);
    }

    //std::unique_ptr<Bind::VSConstBuffer<DirectX::XMMATRIX>> ZTransformVSConstBuffer::pVcbuf;
    std::unique_ptr<Bind::VSConstBuffer<ZTransformVSConstBuffer::Transforms>> ZTransformVSConstBuffer::pVcbuf;
}