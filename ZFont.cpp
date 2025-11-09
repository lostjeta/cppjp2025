
//---------------------------------------------------------------------------

#include "ZGUI.h"
#include "ZFont.h"
#include <cstring>  // for strlen

//---------------------------------------------------------------------------
// ZFont
//---------------------------------------------------------------------------

// UTF-8 char* to UTF-16 wstring 변환 헬퍼 함수
static std::wstring Utf8ToWstring(const char* utf8Str)
{
    if (utf8Str == nullptr || *utf8Str == '\0')
        return std::wstring();

    // UTF-8 to UTF-16 변환
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);
    if (wideSize <= 0)
        return std::wstring();

    std::wstring wideStr(wideSize - 1, 0);  // -1은 null terminator 제외
    MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, &wideStr[0], wideSize);

    return wideStr;
}

//---------------------------------------------------------------------------

ZFont::ZFont()
{
    m_pSpriteFont = nullptr;
    m_pSpriteBatch = nullptr;
    m_Name = "";
    m_Size = 16;
}

//---------------------------------------------------------------------------

ZFont::~ZFont()
{
    // std::string은 자동으로 소멸됨
    Free();
}

//---------------------------------------------------------------------------

const char* ZFont::GetName() const
{
    return m_Name.c_str();
}

//---------------------------------------------------------------------------

int ZFont::GetSize() const
{
    return m_iSize;
}

//---------------------------------------------------------------------------

BOOL ZFont::IsBold() const
{
    return m_bBold;
}

//---------------------------------------------------------------------------

BOOL ZFont::IsItalic() const
{
    return m_bItalic;
}

//---------------------------------------------------------------------------

DirectX::SpriteFont* ZFont::GetSpriteFont()
{
    return m_pSpriteFont;
}

//---------------------------------------------------------------------------

DirectX::SpriteBatch* ZFont::GetSpriteBatch()
{
    return m_pSpriteBatch;
}

//---------------------------------------------------------------------------

BOOL ZFont::Create(ZGraphics& gfx, const char* SpriteFontFile, long Size, BOOL Bold, BOOL Italic)
{
    if (SpriteFontFile == NULL)
        return FALSE;

    ID3D11Device* pDevice = gfx.GetDeviceCOM();
    ID3D11DeviceContext* pContext = gfx.GetDeviceContext();

    if (pDevice == NULL || pContext == NULL)
        return FALSE;

    // 다른 폰트와 비교용 데이타 보관
    m_Name = SpriteFontFile;
    m_iSize = (int)Size;
    m_bBold = Bold;
    m_bItalic = Italic;
    m_Size = Size;

    try
    {
        // DirectXTK SpriteFont 로드 (.spritefont 파일)
        // MakeSpriteFont.exe로 생성한 파일 사용
        // 예: MakeSpriteFont.exe "Arial" Arial_16.spritefont /FontSize:16

        // char* to wstring 변환
        std::wstring wSpriteFontFile(SpriteFontFile, SpriteFontFile + strlen(SpriteFontFile));
        
        // DirectXTK SpriteFont와 SpriteBatch 생성
        m_pSpriteFont = new DirectX::SpriteFont(pDevice, wSpriteFontFile.c_str());
        m_pSpriteBatch = new DirectX::SpriteBatch(pContext);

        return TRUE;
    }
    catch (...)
    {
        return FALSE;
    }
}

//---------------------------------------------------------------------------

BOOL ZFont::Free()
{
    // DirectXTK 객체 삭제
    if (m_pSpriteFont)
    {
        delete m_pSpriteFont;
        m_pSpriteFont = nullptr;
    }
    if (m_pSpriteBatch)
    {
        delete m_pSpriteBatch;
        m_pSpriteBatch = nullptr;
    }
    return TRUE;
}

//---------------------------------------------------------------------------

BOOL ZFont::FastPrint(long XPos, long YPos, const char* text, DirectX::SpriteBatch* externalBatch)
{
    if (m_pSpriteFont == nullptr)
        return FALSE;
    
    // 사용할 SpriteBatch 결정
    DirectX::SpriteBatch* batch = externalBatch ? externalBatch : m_pSpriteBatch;
    if (batch == nullptr)
        return FALSE;

    // UTF-8 to UTF-16 변환
    std::wstring wText = Utf8ToWstring(text);

    // DirectXTK SpriteFont 사용 (Begin/End는 외부에서 관리)
    DirectX::XMFLOAT2 pos((float)XPos, (float)YPos);
    
    // 요청된 크기와 .spritefont 파일의 기본 크기를 비교하여 스케일 계산
    float defaultSize = m_pSpriteFont->GetLineSpacing();
    float scale = (m_Size > 0 && defaultSize > 0) ? (float)m_Size / defaultSize : 1.0f;
    
    m_pSpriteFont->DrawString(batch, wText.c_str(), pos, DirectX::Colors::White, 0.0f, DirectX::XMFLOAT2(0, 0), scale);

    return TRUE;
}

//---------------------------------------------------------------------------

BOOL ZFont::PrintLine(long XPos, long YPos, long Width, DirectX::XMFLOAT4 Color, const char* text, DirectX::SpriteBatch* externalBatch)
{
    if (m_pSpriteFont == nullptr)
        return FALSE;
    
    // 사용할 SpriteBatch 결정
    DirectX::SpriteBatch* batch = externalBatch ? externalBatch : m_pSpriteBatch;
    if (batch == nullptr)
        return FALSE;

    // UTF-8 to UTF-16 변환
    std::wstring wText = Utf8ToWstring(text);

    // DirectXTK SpriteFont 사용 (Begin/End는 외부에서 관리)
    DirectX::XMVECTOR colorVec = DirectX::XMLoadFloat4(&Color);
    DirectX::XMFLOAT2 pos((float)XPos, (float)YPos);
    
    // 요청된 크기와 .spritefont 파일의 기본 크기를 비교하여 스케일 계산
    float defaultSize = m_pSpriteFont->GetLineSpacing();
    float scale = (m_Size > 0 && defaultSize > 0) ? (float)m_Size / defaultSize : 1.0f;
    
    m_pSpriteFont->DrawString(batch, wText.c_str(), pos, colorVec, 0.0f, DirectX::XMFLOAT2(0, 0), scale);

    return TRUE;
}

//---------------------------------------------------------------------------

BOOL ZFont::PrintEx(long XPos, long YPos, long Width, long Height, DirectX::XMFLOAT4 Color, DWORD Format, const char* text, DirectX::SpriteBatch* externalBatch)
{
    if (m_pSpriteFont == nullptr)
        return FALSE;
    
    // 사용할 SpriteBatch 결정
    DirectX::SpriteBatch* batch = externalBatch ? externalBatch : m_pSpriteBatch;
    if (batch == nullptr)
        return FALSE;

    // UTF-8 to UTF-16 변환
    std::wstring wText = Utf8ToWstring(text);

    // DirectXTK SpriteFont 사용 (Begin/End는 외부에서 관리)
    DirectX::XMVECTOR colorVec = DirectX::XMLoadFloat4(&Color);
    DirectX::XMFLOAT2 pos((float)XPos, (float)YPos);
    DirectX::XMFLOAT2 origin(0, 0);
    
    // 요청된 크기와 .spritefont 파일의 기본 크기를 비교하여 스케일 계산
    // SpriteFont의 LineSpacing을 기본 크기로 사용
    float defaultSize = m_pSpriteFont->GetLineSpacing();
    float scale = (m_Size > 0 && defaultSize > 0) ? (float)m_Size / defaultSize : 1.0f;
    
    m_pSpriteFont->DrawString(batch, wText.c_str(), pos, colorVec, 0.0f, origin, scale);

    // Format 플래그는 DirectXTK에서 직접 처리 필요 (DT_CENTER, DT_RIGHT 등)

    return TRUE;
}

//---------------------------------------------------------------------------

