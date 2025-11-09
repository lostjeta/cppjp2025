#pragma once

// Desc:		텍스트 표시 컨트롤

#include <string>

class ZGUILabel : public ZGUIControl
{
protected:
	std::string m_Text;
	BOOL m_bShadow;

public:
    ZGUILabel( ZGUIDialog *pDialog = NULL );
	~ZGUILabel();

	virtual BOOL HandleKeyboard( UINT uMsg, WPARAM wParam, LPARAM lParam );
    virtual BOOL HandleMouse( UINT uMsg, POINT pt, WPARAM wParam, LPARAM lParam );
	virtual void Render( ID3D11DeviceContext* pContext, float fElapsedTime );

    std::string GetText() { return m_Text; }
    BOOL SetText( const std::string& text, BOOL bShadow = TRUE );
	virtual	void SetFontColor( DirectX::XMFLOAT4 Color);
};
