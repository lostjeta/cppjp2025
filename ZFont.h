#pragma once

// DirectXTK
#include "SpriteFont.h"
#include "SpriteBatch.h"

class ZFont
{
private:
    DirectX:: SpriteFont* m_pSpriteFont;
    DirectX::SpriteBatch* m_pSpriteBatch;
    long m_Size;
    std::string m_Name;
    int m_iSize;
    BOOL m_bBold;
    BOOL m_bItalic;

public:
    ZFont();
    ~ZFont();

    const char* GetName() const;
    int GetSize() const;
    BOOL IsBold() const;
    BOOL IsItalic() const;

    DirectX::SpriteFont* GetSpriteFont();
    DirectX::SpriteBatch* GetSpriteBatch();

    BOOL Create(ZGraphics& gfx, const char* spriteFontFile, long size=16, BOOL bold = FALSE, BOOL italic = FALSE); // sprtie 폰트파일을 저장 후 생성
    BOOL Free();

    // D3D11 텍스트 렌더링 (색상은 DirecxtX::XMFLOAT4 사용) RGBA -> XYZW 매핑
    // SpriteBatch를 외부에서 전달 받아서 사용 가능 (nulltptr이면 내부 spriteBatch 사용)
    BOOL FastPrint(long xPos, long yPos, const char* text, DirectX::SpriteBatch* externalBatch = nullptr);
    BOOL FastPrint(long xPos, long yPos, const char8_t* text, DirectX::SpriteBatch* externalBatch = nullptr);
    BOOL PrintLine(long xPos, long yPos, long width, DirectX::XMFLOAT4 color, const char* text, DirectX::SpriteBatch* externalBatch = nullptr);
    BOOL PrintLine(long xPos, long yPos, long width, DirectX::XMFLOAT4 color, const char8_t* text, DirectX::SpriteBatch* externalBatch = nullptr);
    BOOL PrintEx(long xPos, long yPos, long width, long height, DirectX::XMFLOAT4 color, DWORD format, const char* text, DirectX::SpriteBatch* externalBatch = nullptr);
    BOOL PrintEx(long xPos, long yPos, long width, long height, DirectX::XMFLOAT4 color, DWORD format, const char8_t* text, DirectX::SpriteBatch* externalBatch = nullptr);

    /*
    Format[in]
    : Specifies the method of formatting the text.

    It can be any combination of the following values:
    DT_BOTTOM
    : Justifies the text to the bottom of the rectangle. This value must be combined with DT_SINGLELINE.
    DT_CALCRECT
    : Determines the width and height of the rectangle. If there are multiple lines of text, ID3DXFont::DrawText uses the width of the rectangle pointed to by the pRect parameter and extends the base of the rectangle to bound the last line of text. If there is only one line of text, ID3DXFont::DrawText modifies the right side of the rectangle so that it bounds the last character in the line. In either case, ID3DXFont::DrawText returns the height of the formatted text but does not draw the text.
    DT_CENTER
    : Centers text horizontally in the rectangle.
    DT_EXPANDTABS
    : Expands tab characters. The default number of characters per tab is eight.
    DT_LEFT
    : Aligns text to the left.
    DT_NOCLIP
    : Draws without clipping. ID3DXFont::DrawText is somewhat faster when DT_NOCLIP is used.
    DT_RIGHT
    : Aligns text to the right.
    DT_RTLREADING
    : Displays text in right-to-left reading order for bidirectional text when a Hebrew or Arabic font is selected. The default reading order for all text is left-to-right.
    DT_SINGLELINE
    : Displays text on a single line only. Carriage returns and line feeds do not break the line.
    DT_TOP
    : Top-justifies text.
    DT_VCENTER
    : Centers text vertically (single line only).
    DT_WORDBREAK
    : Breaks words. Lines are automatically broken between words if a word would extend past the edge of the rectangle specified by the pRect parameter. A carriage return/line feed sequence also breaks the line.
    */
};