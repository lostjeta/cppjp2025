#include "ZD3D11.h"
#include "ZTexture.h"
#include "ZGraphics.h"
#include "WICTextureLoader.h" // From DirectX Tool Kit
#include "GraphicsThrowMacros.h"


namespace Bind
{
    ZTexture::ZTexture(ZGraphics& gfx, const TCHAR* Filename)
    {
        pTextureSRV = nullptr;
        m_Width = m_Height = 0;
        m_pFileName[0] = _T('\0'); // Initialize the string

        Load(gfx, Filename);
    }

    ZTexture::~ZTexture()
    {
        Free();
    }

    const TCHAR* ZTexture::GetFileName()
    {
        return m_pFileName;
    }

    BOOL ZTexture::Load(ZGraphics& gfx, const TCHAR* Filename)
    {
        Free();

        if (GetDevice(gfx) == NULL)
            return FALSE;
        if (Filename == NULL)
            return FALSE;

        INFOMAN(gfx);

        // Store filename
        _tcscpy_s(m_pFileName, MAX_PATH, Filename);

        Microsoft::WRL::ComPtr<ID3D11Resource> resource;
        GFX_THROW_INFO(DirectX::CreateWICTextureFromFile(
            GetDevice(gfx),
            Filename,
            resource.GetAddressOf(),
            pTextureSRV.GetAddressOf()
        ));

        // Get texture dimensions
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2D;
        hr = resource.As(&texture2D);
        if (SUCCEEDED(hr))
        {
            D3D11_TEXTURE2D_DESC desc;
            texture2D->GetDesc(&desc);
            m_Width = desc.Width;
            m_Height = desc.Height;
        }

        return TRUE;
    }

    //---------------------------------------------------------------------------
    // Load : 파일 로딩
    // Create : 텍스처 생성
    // 위처럼 분리할 경우에 Create 사용한다.

    BOOL ZTexture::Create(ZGraphics& gfx, unsigned int Width, unsigned int Height, DXGI_FORMAT Format)
    {
        Free();

        INFOMAN(gfx);

        m_Width = Width;
        m_Height = Height;

        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = Width;
        textureDesc.Height = Height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = Format;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        //// 파일 직접 로딩일 경우
        //D3D11_SUBRESOURCE_DATA sd = {};
        //sd.pSysMem = gf.GetBufferPtr();
        //sd.SysMemPitch = gf.GetWidth() * sizeof(Surface::Color);
    
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2D;
        GFX_THROW_INFO((GetDevice(gfx)->CreateTexture2D(&textureDesc, nullptr, texture2D.GetAddressOf())));

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        GFX_THROW_INFO((GetDevice(gfx)->CreateShaderResourceView(texture2D.Get(), &srvDesc, pTextureSRV.GetAddressOf())));

        return TRUE;
    }

    BOOL ZTexture::Free()
    {
        pTextureSRV.Reset();
        m_Width = m_Height = 0;
        m_pFileName[0] = _T('\0');

        return TRUE;
    }

    void ZTexture::Bind(ZGraphics& gfx) noexcept
    {
        GetContext(gfx)->PSSetShaderResources(0u, 1u, pTextureSRV.GetAddressOf());
    }

    BOOL ZTexture::IsLoaded()
    {
        return (pTextureSRV != nullptr);
    }

    ID3D11ShaderResourceView* ZTexture::GetTextureSRV()
    {
        return pTextureSRV.Get();
    }

    long ZTexture::GetWidth()
    {
        return m_Width;
    }

    long ZTexture::GetHeight()
    {
        return m_Height;
    }

    DXGI_FORMAT ZTexture::GetFormat()
    {
        if (!pTextureSRV)
            return DXGI_FORMAT_UNKNOWN;

        Microsoft::WRL::ComPtr<ID3D11Resource> resource;
        pTextureSRV->GetResource(resource.GetAddressOf());

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2D;
        if (FAILED(resource.As(&texture2D)))
            return DXGI_FORMAT_UNKNOWN;

        D3D11_TEXTURE2D_DESC desc;
        texture2D->GetDesc(&desc);

        return desc.Format;
    }

    BOOL ZTexture::Blit(DirectX::SpriteBatch* spriteBatch,
                        const DirectX::SimpleMath::Vector2& position,
                        const RECT* srcrect,
                        DirectX::SimpleMath::Vector4 color,
                        float rotation,
                        const DirectX::SimpleMath::Vector2& origin,
                        float scale)
    {
        if (pTextureSRV == nullptr || spriteBatch == nullptr)
            return FALSE;

        spriteBatch->Draw(pTextureSRV.Get(), position, srcrect, color, rotation, origin, scale);

        return TRUE;
    }
}
