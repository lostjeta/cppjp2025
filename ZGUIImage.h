
// Desc:		텍스트 표시 컨트롤

class ZGUIImage : public ZGUILabel
{
protected:
    BOOL m_bPressed;

public:
    ZGUIImage( ZGUIDialog *pDialog = NULL );
	~ZGUIImage();

    virtual void OnHotkey();
	virtual BOOL HandleKeyboard( UINT uMsg, WPARAM wParam, LPARAM lParam );
    virtual BOOL HandleMouse( UINT uMsg, POINT pt, WPARAM wParam, LPARAM lParam );
    virtual void Render( ID3D11DeviceContext* pContext, float fElapsedTime );
	virtual	void SetTextureColor( DirectX::XMFLOAT4 Color);

    virtual BOOL CanHaveFocus() { return (m_bVisible && m_bEnabled); }
	virtual BOOL GetPressed() { return m_bPressed; }
	virtual void SetPressed( BOOL bPressed ) { m_bPressed = bPressed; }
};
