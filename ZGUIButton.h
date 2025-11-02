/*---------------------------------------------------------------------------

Desc:		버튼 컨트롤

** 버튼 이미지 구성 **

Horizontal
[ Normal | Pressed | Over | Disable ]

Vertical
[ Normal  ]
[ Pressed ]
[ Over    ]
[ Disable ]

---------------------------------------------------------------------------
*/


class ZGUIButton : public ZGUILabel
{
protected:
    BOOL m_bPressed;

public:
    ZGUIButton(ZGUIDialog* pDialog = NULL);
    ~ZGUIButton();

    virtual void OnHotkey();
    virtual BOOL HandleKeyboard(UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual BOOL HandleMouse(UINT uMsg, POINT pt, WPARAM wParam, LPARAM lParam);
    virtual void Render(ID3D11DeviceContext* pContext, float fElapsedTime);
    virtual	void SetTextureColor(DirectX::XMFLOAT4 Color);

    virtual BOOL CanHaveFocus() { return (m_bVisible && m_bEnabled); }
    virtual BOOL GetPressed() { return m_bPressed; }
    virtual void SetPressed(BOOL bPressed) { m_bPressed = bPressed; }
};