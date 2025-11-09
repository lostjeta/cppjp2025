#pragma once

//Desc:		GUI Control 베이스 (하나의 컨트롤은 하나의 텍스쳐 파일내에 존재)

#include <vector>


//---------------------------------------------------------------------------
// Defines and macros 
//---------------------------------------------------------------------------
#define EVENT_IMAGE_CLICKED					0x0100
#define EVENT_IMAGE_OVER					0x0101

#define EVENT_BUTTON_CLICKED				0x0200
#define EVENT_BUTTON_OVER					0x0201



//---------------------------------------------------------------------------
// Enums for pre-defined control types
//---------------------------------------------------------------------------
enum ZGUI_CONTROL_TYPE 
{
    ZGUI_CONTROL_LABEL = 0,
	ZGUI_CONTROL_IMAGE,					// 추가 Pressed
    ZGUI_CONTROL_BUTTON,				// 추가 Pressed
    //ZGUI_CONTROL_CHECKBOX,
    //ZGUI_CONTROL_RADIOBUTTON,
    //ZGUI_CONTROL_COMBOBOX,
    //ZGUI_CONTROL_SLIDER,
    //ZGUI_CONTROL_EDITBOX,
    //ZGUI_CONTROL_IMEEDITBOX,
    //ZGUI_CONTROL_LISTBOX,
    //ZGUI_CONTROL_SCROLLBAR,
    //ZGUI_CONTROL_PROGRESSBAR,
    //ZGUI_CONTROL_MEMO, //SCROLLEDIT,
    
    MAX_CONTROL_TYPES
};

//---------------------------------------------------------------------------
// 하나의 Element는 하나의 State을 갖는다.
// 즉, 모든 컨트롤은 MAX_CONTROL_STATES 개의 Element를 갖는다.

enum ZGUI_CONTROL_STATE
{
    ZGUI_STATE_NORMAL = 0,
    ZGUI_STATE_DISABLED,
    ZGUI_STATE_HIDDEN,
    ZGUI_STATE_FOCUS,
    ZGUI_STATE_MOUSEOVER,
    ZGUI_STATE_PRESSED,

	MAX_CONTROL_STATES
};

//---------------------------------------------------------------------------
// 시간에 따라 변화하는 Color 값을 처리하기위해
// (버튼의 부드러운 반전에 쓰임)
// Init()으로 기본색들의 초기화를 한 후 필요에 따라 각각의 색을 States에 
// 직접 접근하여 지정한다.

struct sGUIBlendColor
{
    DirectX::XMFLOAT4  States[ MAX_CONTROL_STATES ]; // Modulate colors for all possible control states
    DirectX::XMFLOAT4 Current;

    void Init( DirectX::XMFLOAT4 defaultColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 
			   DirectX::XMFLOAT4 disabledColor = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 0.78f), 
			   DirectX::XMFLOAT4 hiddenColor = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) 
			 );
    void Blend( UINT iState, float fElapsedTime, float fRate = 0.7f );
};

//---------------------------------------------------------------------------
// Contains all the display tweakables for a sub-control
// 텍스쳐 및 폰트 Color 보관 단위
//---------------------------------------------------------------------------
class ZGUIElement
{
public:
    void SetTexture( UINT iTexture, RECT* prcTexture, DirectX::XMFLOAT4 defaultTextureColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) );
    void SetFont( UINT iFont, DirectX::XMFLOAT4 defaultFontColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), DWORD dwTextFormat = DT_CENTER | DT_VCENTER );
    void Refresh();
    
    UINT iTexture;          // Index of the texture for this Element 
    UINT iFont;             // Index of the font for this Element
    DWORD dwTextFormat;     // The format argument to DrawText 
    RECT rcTexture;         // Bounding rect of this element on the composite texture
    
    sGUIBlendColor TextureColor;
    sGUIBlendColor FontColor;
};

//---------------------------------------------------------------------------
// Base class for controls
//---------------------------------------------------------------------------
class ZGUIDialog;
class ZGUIControl
{
protected:
    int  m_iID;                 // ID number
	int  m_iCurState;			// Current Control State (Render() 함수에서 설정한다)
    int	 m_iType;				// Control type, set once in constructor  
    UINT m_nHotkey;				// Virtual key code for this control's hotkey
    RECT m_rcBoundingBox;		// Rectangle defining the active region of the control
    virtual void UpdateRects();

    BOOL m_bEnabled;			// Enabled/disabled flag
    BOOL m_bVisible;			// Shown/hidden flag
	// 내부 작동용
    BOOL m_bMouseOver;			// Mouse pointer is above control
    BOOL m_bHasFocus;			// Control has input focus
    BOOL m_bDefault;			// Is the default control

    // Size, scale, and positioning members
    int m_iX, m_iY;
    int m_iWidth, m_iHeight;

	// ZGUIElement* , 하나의 컨트롤이 각 상태에 따라 여러개의 텍스쳐와 폰트로 이루어질 수 있음
	// (기본 MAX_CONTROL_STATES 개의 List 값이 있다.)
	std::vector<ZGUIElement*> m_ElementList;		// All display elements

public:
    // These members are set by the container - 다이얼로그에서 세팅
    ZGUIDialog* m_pParentDialog;// Parent container

    ZGUIControl( ZGUIDialog *pDialog = NULL );
    virtual ~ZGUIControl();

    virtual void Refresh();
	virtual HRESULT OnInit() { return S_OK; }
	virtual void Update( float fElapsedTime ) {};
    virtual void Render( ID3D11DeviceContext* pContext, float fElapsedTime ) {};

    // Windows message handler
    virtual BOOL MsgProc( UINT uMsg, WPARAM wParam, LPARAM lParam ) { return FALSE; }
    virtual BOOL HandleKeyboard( UINT uMsg, WPARAM wParam, LPARAM lParam ) { return FALSE; }
    virtual BOOL HandleMouse( UINT uMsg, POINT pt, WPARAM wParam, LPARAM lParam ) { return FALSE; }
    virtual void OnHotkey()		{}

    virtual BOOL ContainsPoint( POINT pt )		{ return PtInRect( &m_rcBoundingBox, pt ); }
	//virtual BOOL ContainsPoint( POINT pt )		{ return D3D::IsInRect( &m_rcBoundingBox, pt ); }

	virtual void SetEnabled( BOOL bEnabled )	{ m_bEnabled = bEnabled; }
    virtual BOOL GetEnabled()					{ return m_bEnabled; }
    virtual void SetVisible( BOOL bVisible )	{ m_bVisible = bVisible; }
    virtual BOOL GetVisible()					{ return m_bVisible; }
    virtual void OnMouseEnter()					{ m_bMouseOver = TRUE; }
    virtual void OnMouseLeave()					{ m_bMouseOver = FALSE; }
    virtual void OnFocusIn()					{ m_bHasFocus = TRUE; }
    virtual void OnFocusOut()					{ m_bHasFocus = FALSE; }
    virtual BOOL CanHaveFocus()					{ return FALSE; }
	virtual void SetDefault( BOOL bDefault )	{ m_bDefault = bDefault; }
	virtual BOOL GetDefault()					{ return m_bDefault; }

    int GetType() const							{ return m_iType; }
    int  GetID() const							{ return m_iID; }
    void SetID( int ID )						{ m_iID = ID; }
    void SetLocation( int x, int y )			{ m_iX = x; m_iY = y; UpdateRects(); }
    void SetSize( int width, int height )		{ m_iWidth = width; m_iHeight = height; UpdateRects(); }
    void SetHotkey( UINT nHotkey )				{ m_nHotkey = nHotkey; }
    UINT GetHotkey()							{ return m_nHotkey; }

    ZGUIElement* GetElement( int iControlState ){ return (iControlState >= 0 && iControlState < (int)m_ElementList.size()) ? m_ElementList[iControlState] : nullptr; }
	// 컨트롤의 6가지 상태만큼만 세팅하므로 iControlState의 입력값은 MAX_CONTROL_STATES 만큼 제한된다.
    BOOL SetElement( int iControlState, ZGUIElement* pElement);
};
