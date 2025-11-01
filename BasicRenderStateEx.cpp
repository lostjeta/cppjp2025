#include "ZD3D11.h"
#include "SpriteBatch.h"
#include "ZTexture.h"

#include "BasicRenderState.h"
#include "GraphicsThrowMacros.h"
#include <d3dcompiler.h>
#include <DirectXMath.h> // dx math

namespace wrl = Microsoft::WRL;
namespace dx = DirectX;

void BasicRenderState::DrawTriangle(ZGraphics& gfx)
{
    INFOMAN(gfx);

    struct Vertex
    {
        struct
        {
            float x;
            float y;
        } pos;
    };

    // NDC 화면 가운데가 (0,0), X(-1,1), Y(-1,1)
    // 왼손좌표계 앞면 : CW (Clock Wise)
    Vertex vertices[] =
    {
        { 0.0f, 0.5f },
        { 0.5f, -0.5f },
        { -0.5f, -0.5f },
    };

    // Create VertexBuffer
    wrl::ComPtr<ID3D11Buffer> pVertexBuffer;
    D3D11_BUFFER_DESC bd = {};
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.CPUAccessFlags = 0u;
    bd.MiscFlags = 0u;
    bd.ByteWidth = sizeof(vertices);
    bd.StructureByteStride = sizeof(Vertex);

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = vertices;
    GFX_THROW_INFO(GetDevice(gfx)->CreateBuffer(&bd, &sd, &pVertexBuffer));

    // Bind vertex buffer to pipeline
    const UINT stride = sizeof(Vertex);
    const UINT offset = 0u;
    GetContext(gfx)->IASetVertexBuffers(0u, 1u, pVertexBuffer.GetAddressOf(), &stride, &offset);

    // Create pixel shader
    wrl::ComPtr<ID3D11PixelShader> pPixelShader;
    wrl::ComPtr<ID3DBlob> pBlob;
    GFX_THROW_INFO(D3DReadFileToBlob(L"./x64/Debug/PixelShader.cso", &pBlob));
    GFX_THROW_INFO(GetDevice(gfx)->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pPixelShader));

    // bind pixel shader
    GetContext(gfx)->PSSetShader(pPixelShader.Get(), nullptr, 0u);

    // Create vertex shader
    wrl::ComPtr<ID3D11VertexShader> pVertexShader;
    GFX_THROW_INFO(D3DReadFileToBlob(L"./x64/Debug/VertexShader.cso", &pBlob));
    GFX_THROW_INFO(GetDevice(gfx)->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pVertexShader));

    // Bind vertex shader
    GetContext(gfx)->VSSetShader(pVertexShader.Get(), nullptr, 0u);

    // VertexShader의 바이트코드(pBlob)가 필요하기 때문에 PixelShader를 먼저 생성한다.
    wrl::ComPtr<ID3D11InputLayout> pInputLayout;
    const D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        { "Position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    GFX_THROW_INFO(GetDevice(gfx)->CreateInputLayout(
        ied, (UINT)std::size(ied),
        pBlob->GetBufferPointer(),
        pBlob->GetBufferSize(),
        &pInputLayout
    ));
    // Bind vertex layout
    GetContext(gfx)->IASetInputLayout(pInputLayout.Get());

    // Bind render target
    // GetTarget(gfx)는 ID3D11RenderTargetView* 포인터를 반환하므로
    // OMSetRenderTargets에 전달하기 위해 주소 연산자(&)를 사용합니다.
    ID3D11RenderTargetView* pTarget = GetTarget(gfx);
    GetContext(gfx)->OMSetRenderTargets(1u, &pTarget, nullptr);

    // Set primitive topology to triangle list (groups of 3 vertices)
    GetContext(gfx)->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Configure viewport
    D3D11_VIEWPORT vp;
    vp.Width = (float)GetClientWidth(gfx);
    vp.Height = (float)GetClientHeight(gfx);
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    GetContext(gfx)->RSSetViewports(1u, &vp);

    GFX_THROW_INFO_ONLY(GetContext(gfx)->Draw((UINT)std::size(vertices), 0u));
}

void BasicRenderState::DrawIndexedTriangle(ZGraphics& gfx)
{
    INFOMAN(gfx);

    struct Vertex
    {
        struct
        {
            float x;
            float y;
        } pos;
        struct
        {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t a;
        } color;
    };

    // NDC 화면 가운데가 (0,0), X(-1,1), Y(-1,1)
    // 왼손좌표계 앞면 : CW (Clock Wise)
    Vertex vertices[] =
    {
        { 0.0f, 0.5f, 255, 0, 0, 0 },
        { 0.5f, -0.5f, 0, 255, 0, 0 },
        { -0.5f, -0.5f, 0, 0, 255, 0 },
        { -0.3f, 0.3f, 0, 255, 0, 0 },
        { 0.3f, 0.3f, 0, 0, 255, 0 },
        { 0.0f, -0.8f, 255, 0, 0, 0 },
    };

    // Create VertexBuffer
    wrl::ComPtr<ID3D11Buffer> pVertexBuffer;
    D3D11_BUFFER_DESC bd = {};
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.CPUAccessFlags = 0u;
    bd.MiscFlags = 0u;
    bd.ByteWidth = sizeof(vertices);
    bd.StructureByteStride = sizeof(Vertex);

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = vertices;
    GFX_THROW_INFO(GetDevice(gfx)->CreateBuffer(&bd, &sd, &pVertexBuffer));

    // Bind vertex buffer to pipeline
    const UINT stride = sizeof(Vertex);
    const UINT offset = 0u;
    GetContext(gfx)->IASetVertexBuffers(0u, 1u, pVertexBuffer.GetAddressOf(), &stride, &offset);

    // Create index buffer
    const uint16_t indices[] =
    {
        0, 1, 2,
        0, 2, 3,
        0, 4, 1,
        2, 1, 5,
    };

    // Create IndexBuffer
    wrl::ComPtr<ID3D11Buffer> pIndexBuffer;
    D3D11_BUFFER_DESC ibd = {};
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.CPUAccessFlags = 0u;
    ibd.MiscFlags = 0u;
    ibd.ByteWidth = sizeof(indices);
    ibd.StructureByteStride = sizeof(uint16_t);

    D3D11_SUBRESOURCE_DATA isd = {};
    isd.pSysMem = indices;
    GFX_THROW_INFO(GetDevice(gfx)->CreateBuffer(&ibd, &isd, &pIndexBuffer));

    // bind index buffer
    GetContext(gfx)->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0u);


    // Create pixel shader
    wrl::ComPtr<ID3D11PixelShader> pPixelShader;
    wrl::ComPtr<ID3DBlob> pBlob;
    GFX_THROW_INFO(D3DReadFileToBlob(L"./x64/Debug/PixelShaderIndexed.cso", &pBlob));
    GFX_THROW_INFO(GetDevice(gfx)->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pPixelShader));

    // bind pixel shader
    GetContext(gfx)->PSSetShader(pPixelShader.Get(), nullptr, 0u);

    // Create vertex shader
    wrl::ComPtr<ID3D11VertexShader> pVertexShader;
    GFX_THROW_INFO(D3DReadFileToBlob(L"./x64/Debug/VertexShaderIndexed.cso", &pBlob));
    GFX_THROW_INFO(GetDevice(gfx)->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pVertexShader));

    // Bind vertex shader
    GetContext(gfx)->VSSetShader(pVertexShader.Get(), nullptr, 0u);

    // VertexShader의 바이트코드(pBlob)가 필요하기 때문에 PixelShader를 먼저 생성한다.
    wrl::ComPtr<ID3D11InputLayout> pInputLayout;
    const D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        { "Position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "Color", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 8u, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    GFX_THROW_INFO(GetDevice(gfx)->CreateInputLayout(
        ied, (UINT)std::size(ied),
        pBlob->GetBufferPointer(),
        pBlob->GetBufferSize(),
        &pInputLayout
    ));
    // Bind vertex layout
    GetContext(gfx)->IASetInputLayout(pInputLayout.Get());

    // Bind render target
    auto pTarget = GetTarget(gfx);
    GetContext(gfx)->OMSetRenderTargets(1u, &pTarget, nullptr);

    // Set primitive topology to triangle list (groups of 3 vertices)
    GetContext(gfx)->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Configure viewport
    D3D11_VIEWPORT vp;
    vp.Width = (float)GetClientWidth(gfx);
    vp.Height = (float)GetClientHeight(gfx);
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    GetContext(gfx)->RSSetViewports(1u, &vp);

    GFX_THROW_INFO_ONLY(GetContext(gfx)->DrawIndexed((UINT)std::size(indices), 0u, 0u));
}

void BasicRenderState::DrawConstTriangle(ZGraphics& gfx, double angle)
{
    INFOMAN(gfx);

    struct Vertex
    {
        struct
        {
            float x;
            float y;
        } pos;
        struct
        {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t a;
        } color;
    };

    // NDC 화면 가운데가 (0,0), X(-1,1), Y(-1,1)
    // 왼손좌표계 앞면 : CW (Clock Wise)
    Vertex vertices[] =
    {
        { 0.0f, 0.5f, 255, 0, 0, 0 },
        { 0.5f, -0.5f, 0, 255, 0, 0 },
        { -0.5f, -0.5f, 0, 0, 255, 0 },
        { -0.3f, 0.3f, 0, 255, 0, 0 },
        { 0.3f, 0.3f, 0, 0, 255, 0 },
        { 0.0f, -0.8f, 255, 0, 0, 0 },
    };

    // Create VertexBuffer
    wrl::ComPtr<ID3D11Buffer> pVertexBuffer;
    D3D11_BUFFER_DESC bd = {};
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.CPUAccessFlags = 0u;
    bd.MiscFlags = 0u;
    bd.ByteWidth = sizeof(vertices);
    bd.StructureByteStride = sizeof(Vertex);

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = vertices;
    GFX_THROW_INFO(GetDevice(gfx)->CreateBuffer(&bd, &sd, &pVertexBuffer));

    // Bind vertex buffer to pipeline
    const UINT stride = sizeof(Vertex);
    const UINT offset = 0u;
    GetContext(gfx)->IASetVertexBuffers(0u, 1u, pVertexBuffer.GetAddressOf(), &stride, &offset);


    // Create index buffer
    const uint16_t indices[] =
    {
        0, 1, 2,
        0, 2, 3,
        0, 4, 1,
        2, 1, 5,
    };

    // Create IndexBuffer
    wrl::ComPtr<ID3D11Buffer> pIndexBuffer;
    D3D11_BUFFER_DESC ibd = {};
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.CPUAccessFlags = 0u;
    ibd.MiscFlags = 0u;
    ibd.ByteWidth = sizeof(indices);
    ibd.StructureByteStride = sizeof(uint16_t);

    D3D11_SUBRESOURCE_DATA isd = {};
    isd.pSysMem = indices;
    GFX_THROW_INFO(GetDevice(gfx)->CreateBuffer(&ibd, &isd, &pIndexBuffer));

    // bind index buffer
    GetContext(gfx)->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0u);


    // Create constant buffer for transformation matrix
    struct ConstantBuffer
    {
        struct
        {
            float element[4][4];
        } transformation;
    };
    double winRatio = GetWinRatio(gfx);
    const ConstantBuffer cb =
    {
        {
            float(winRatio * std::cos(angle)), float(std::sin(angle)), 0.0f, 0.0f,
            float(winRatio * -std::sin(angle)), float(std::cos(angle)), 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        }
    };
    wrl::ComPtr<ID3D11Buffer> pConstantBuffer;
    D3D11_BUFFER_DESC cbd;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbd.MiscFlags = 0u;
    cbd.ByteWidth = sizeof(cb);
    cbd.StructureByteStride = 0u;

    D3D11_SUBRESOURCE_DATA csd = {};
    csd.pSysMem = &cb;
    GFX_THROW_INFO(GetDevice(gfx)->CreateBuffer(&cbd, &csd, &pConstantBuffer));

    // Bind constant buffer to vertex shader
    GetContext(gfx)->VSSetConstantBuffers(0u, 1u, pConstantBuffer.GetAddressOf());


    // Create pixel shader
    wrl::ComPtr<ID3D11PixelShader> pPixelShader;
    wrl::ComPtr<ID3DBlob> pBlob;
    GFX_THROW_INFO(D3DReadFileToBlob(L"./x64/Debug/PixelShaderIndexed.cso", &pBlob));
    GFX_THROW_INFO(GetDevice(gfx)->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pPixelShader));

    // bind pixel shader
    GetContext(gfx)->PSSetShader(pPixelShader.Get(), nullptr, 0u);


    // Create vertex shader
    wrl::ComPtr<ID3D11VertexShader> pVertexShader;
    GFX_THROW_INFO(D3DReadFileToBlob(L"./x64/Debug/VertexShaderConst.cso", &pBlob));
    GFX_THROW_INFO(GetDevice(gfx)->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pVertexShader));

    // Bind vertex shader
    GetContext(gfx)->VSSetShader(pVertexShader.Get(), nullptr, 0u);

    // VertexShader의 바이트코드(pBlob)가 필요하기 때문에 PixelShader를 먼저 생성한다.
    wrl::ComPtr<ID3D11InputLayout> pInputLayout;
    const D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        { "Position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "Color", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 8u, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    GFX_THROW_INFO(GetDevice(gfx)->CreateInputLayout(
        ied, (UINT)std::size(ied),
        pBlob->GetBufferPointer(),
        pBlob->GetBufferSize(),
        &pInputLayout
    ));
    // Bind vertex layout
    GetContext(gfx)->IASetInputLayout(pInputLayout.Get());

    // Bind render target
    auto pTarget = GetTarget(gfx);
    GetContext(gfx)->OMSetRenderTargets(1u, &pTarget, nullptr);

    // Set primitive topology to triangle list (groups of 3 vertices)
    GetContext(gfx)->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Configure viewport
    D3D11_VIEWPORT vp;
    vp.Width = (float)GetClientWidth(gfx);
    vp.Height = (float)GetClientHeight(gfx);
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    GetContext(gfx)->RSSetViewports(1u, &vp);

    GFX_THROW_INFO_ONLY(GetContext(gfx)->DrawIndexed((UINT)std::size(indices), 0u, 0u));
}

void BasicRenderState::DrawDepthCube(ZGraphics& gfx, float angle, float x, float y)
{
    INFOMAN(gfx);

    struct Vertex
    {
        struct
        {
            float x;
            float y;
            float z;
        } pos;
    };

    // NDC 화면 가운데가 (0,0), X(-1,1), Y(-1,1)
    // 왼손좌표계 앞면 : CW (Clock Wise)
    Vertex vertices[] =
    {
        { -1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f, -1.0f },
        { -1.0f, 1.0f, -1.0f },
        { 1.0f, 1.0f, -1.0f  },
        { -1.0f, -1.0f, 1.0f },
        { 1.0f, -1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
    };

    // Create VertexBuffer
    wrl::ComPtr<ID3D11Buffer> pVertexBuffer;
    D3D11_BUFFER_DESC bd = {};
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.CPUAccessFlags = 0u;
    bd.MiscFlags = 0u;
    bd.ByteWidth = sizeof(vertices);
    bd.StructureByteStride = sizeof(Vertex);

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = vertices;
    GFX_THROW_INFO(GetDevice(gfx)->CreateBuffer(&bd, &sd, &pVertexBuffer));

    // Bind vertex buffer to pipeline
    const UINT stride = sizeof(Vertex);
    const UINT offset = 0u;
    GetContext(gfx)->IASetVertexBuffers(0u, 1u, pVertexBuffer.GetAddressOf(), &stride, &offset);


    // Create index buffer
    const uint16_t indices[] =
    {
        0, 2, 1,  2, 3, 1,
        1, 3, 5,  3, 7, 5,
        2, 6, 3,  3, 6, 7,
        4, 5, 7,  4, 7, 6,
        0, 4, 2,  2, 4, 6,
        0, 1, 4,  1, 5, 4,
    };

    // Create IndexBuffer
    wrl::ComPtr<ID3D11Buffer> pIndexBuffer;
    D3D11_BUFFER_DESC ibd = {};
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.CPUAccessFlags = 0u;
    ibd.MiscFlags = 0u;
    ibd.ByteWidth = sizeof(indices);
    ibd.StructureByteStride = sizeof(uint16_t);

    D3D11_SUBRESOURCE_DATA isd = {};
    isd.pSysMem = indices;
    GFX_THROW_INFO(GetDevice(gfx)->CreateBuffer(&ibd, &isd, &pIndexBuffer));

    // bind index buffer
    GetContext(gfx)->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0u);


    // Create constant buffer for transformation matrix
    struct ConstantBuffer
    {
        dx::XMMATRIX transform; // apply simd
    };
    float winRatio = float(GetWinRatio(gfx));
    const ConstantBuffer cb =
    {
        {
            // NDC(-1 ~ 1) : RST 변환
            dx::XMMatrixTranspose(
                dx::XMMatrixRotationZ(angle) *
                dx::XMMatrixRotationY(angle) *
                // dx::XMMatrixScaling(winRatio, 1.0f, 1.0f) * // no need ratio adjustment
                dx::XMMatrixTranslation(x, 0.0f, y + 4.0f) *
                dx::XMMatrixPerspectiveLH(1.0f, winRatio, 0.5f, 10.0f)
            )
        }
    };
    wrl::ComPtr<ID3D11Buffer> pConstantBuffer;
    D3D11_BUFFER_DESC cbd;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbd.MiscFlags = 0u;
    cbd.ByteWidth = sizeof(cb);
    cbd.StructureByteStride = 0u;

    // GPU에 상수 버퍼를 전달하기 전에 Transpose 했다. (CPU : 벡터*매트릭스, GPU : 매트릭스*벡터)
    D3D11_SUBRESOURCE_DATA csd = {};
    csd.pSysMem = &cb;
    GFX_THROW_INFO(GetDevice(gfx)->CreateBuffer(&cbd, &csd, &pConstantBuffer));

    // Bind constant buffer to vertex shader
    GetContext(gfx)->VSSetConstantBuffers(0u, 1u, pConstantBuffer.GetAddressOf());


    // 각 삼각형의 꼭지점 컬러는 다르다. 개별 값을 가져오기 위해 constant buffer 추가
    struct ConstantBuffer2
    {
        struct
        {
            float r;
            float g;
            float b;
            float a;
        } face_colors[6];
    };
    const ConstantBuffer2 cb2 =
    {
        {
            {1.0f, 0.0f, 1.0f},
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f},
            {1.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 1.0f},
        }
    };
    wrl::ComPtr<ID3D11Buffer> pConstantBuffer2;
    D3D11_BUFFER_DESC cbd2;
    cbd2.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd2.Usage = D3D11_USAGE_DEFAULT;
    cbd2.CPUAccessFlags = 0u;
    cbd2.MiscFlags = 0u;
    cbd2.ByteWidth = sizeof(cb2);
    cbd2.StructureByteStride = 0u;

    D3D11_SUBRESOURCE_DATA csd2 = {};
    csd2.pSysMem = &cb2;
    GFX_THROW_INFO(GetDevice(gfx)->CreateBuffer(&cbd2, &csd2, &pConstantBuffer2));

    // bind constant buffer to pixel shader
    GetContext(gfx)->PSSetConstantBuffers(0u, 1u, pConstantBuffer2.GetAddressOf());


    // Create pixel shader
    wrl::ComPtr<ID3D11PixelShader> pPixelShader;
    wrl::ComPtr<ID3DBlob> pBlob;
    GFX_THROW_INFO(D3DReadFileToBlob(L"./x64/Debug/PixelShaderCubeDepth.cso", &pBlob));
    GFX_THROW_INFO(GetDevice(gfx)->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pPixelShader));

    // bind pixel shader
    GetContext(gfx)->PSSetShader(pPixelShader.Get(), nullptr, 0u);


    // Create vertex shader
    wrl::ComPtr<ID3D11VertexShader> pVertexShader;
    GFX_THROW_INFO(D3DReadFileToBlob(L"./x64/Debug/VertexShaderCubeDepth.cso", &pBlob));
    GFX_THROW_INFO(GetDevice(gfx)->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pVertexShader));

    // Bind vertex shader
    GetContext(gfx)->VSSetShader(pVertexShader.Get(), nullptr, 0u);

    // VertexShader의 바이트코드(pBlob)가 필요하기 때문에 PixelShader를 먼저 생성한다.
    wrl::ComPtr<ID3D11InputLayout> pInputLayout;
    const D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        //{ "Position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    GFX_THROW_INFO(GetDevice(gfx)->CreateInputLayout(
        ied, (UINT)std::size(ied),
        pBlob->GetBufferPointer(),
        pBlob->GetBufferSize(),
        &pInputLayout
    ));
    // Bind vertex layout
    GetContext(gfx)->IASetInputLayout(pInputLayout.Get());

    // Bind render target
    //pContext->OMSetRenderTargets(1u, pTarget.GetAddressOf(), nullptr); // ZGraphics 생성자에서 DepthStensil 초기화 했다. 여기서 주석

    // Set primitive topology to triangle list (groups of 3 vertices)
    GetContext(gfx)->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Configure viewport
    D3D11_VIEWPORT vp;
    vp.Width = (float)GetClientWidth(gfx);
    vp.Height = (float)GetClientHeight(gfx);
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    GetContext(gfx)->RSSetViewports(1u, &vp);

    GFX_THROW_INFO_ONLY(GetContext(gfx)->DrawIndexed((UINT)std::size(indices), 0u, 0u));
}

void BasicRenderState::DrawTexture(ZGraphics& gfx)
{
    // 알파 블렌드 상태 활성화 (투명 텍스처 렌더링을 위해)
    const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GetContext(gfx)->OMSetBlendState(GetBlendState(gfx), blendFactor, 0xffffffff);

    pSpriteBatch->Begin();
    pTexture->Blit(pSpriteBatch.get(), { 0, 0 });
    pSpriteBatch->End();

    // 알파 블렌드 비활성화 (3D 객체 렌더링을 위해)
    GetContext(gfx)->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
}
