#pragma once



#include <d3d11.h>
#include <wrl.h> // Microsoft::WRL::ComPtr
#include <tchar.h>
#include "SpriteBatch.h" // From DirectX Tool Kit
#include "SimpleMath.h" // From DirectX Tool Kit
#include "ZBindable.h"

class ZGraphics;

namespace Bind
{
    class ZTexture : public ZBindable
    {
    protected:
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pTextureSRV; // Texture Shader Resource View
    
        unsigned int m_Width, m_Height; // Dimensions of texture image.
        TCHAR m_pFileName[MAX_PATH];

    public:
        ZTexture(ZGraphics& gfx, const TCHAR* Filename);
        ~ZTexture();

        const TCHAR* GetFileName();

        ID3D11ShaderResourceView* GetTextureSRV(); // Return texture SRV

        // Load a texture from file
        BOOL Load(ZGraphics& gfx, const TCHAR* Filename);

        // Create a texture using specific dimensions and format
        BOOL Create(ZGraphics& gfx, unsigned int Width, unsigned int Height, DXGI_FORMAT Format);

        void Bind(ZGraphics& gfx) noexcept;

        // Free texture object
        BOOL Free();

        BOOL IsLoaded(); // Return TRUE if texture is loaded
        long GetWidth(); // Return width of texture
        long GetHeight(); // Return height of texture
        DXGI_FORMAT GetFormat(); // Return texture storage format

        // Draw a 2-D portion of texture to device
        BOOL Blit(DirectX::SpriteBatch* spriteBatch,
                  const DirectX::SimpleMath::Vector2& position,
                  const RECT* srcrect = nullptr,
                  DirectX::SimpleMath::Vector4 color = DirectX::SimpleMath::Vector4(1.f, 1.f, 1.f, 1.f),
                  float rotation = 0.f,
                  const DirectX::SimpleMath::Vector2& origin = DirectX::SimpleMath::Vector2(0.f, 0.f),
                  float scale = 1.f);
    };
}
