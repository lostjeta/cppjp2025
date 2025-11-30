#pragma once
#include "SolidSphere.h"
#include "ZConstBuffer.h"

class ZPointLight
{
private:
    struct PointLightCBuf
    {
        alignas(16) DirectX::XMFLOAT3 pos;
        //alignas(16) DirectX::XMFLOAT3 materialColor;
        alignas(16) DirectX::XMFLOAT3 ambient;
        alignas(16) DirectX::XMFLOAT3 diffuseColor;
        float diffuseIntensity;
        float attConst;
        float attLin;
        float attQuad;
    };
private:
    PointLightCBuf cbData;
    mutable SolidSphere mesh;
    mutable Bind::PSConstBuffer<PointLightCBuf> cbuf;
public:
    ZPointLight(ZGraphics & gfx,float radius = 0.5f);
    void SpawnControlWindow() noexcept;
    void Reset() noexcept;
    void Render(ZGraphics& gfx) const noxnd;
    void Bind(ZGraphics& gfx,DirectX::FXMMATRIX view) const noexcept;
    std::string ToString(PointLightCBuf lightInfo) const;
};