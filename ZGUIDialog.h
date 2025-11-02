#pragma once

// Desc:		GUI Dialog 베이스

#include <vector>
#include <string>

//---------------------------------------------------------------------------

typedef VOID(CALLBACK* PCALLBACKZGUIEVENT) (int iEvent, int iControlID, ZGUIControl* pControl, void* pUserContext);

//-----------------------------------------------------------------------------
// All controls must be assigned to a dialog, which handles
// input and rendering for the controls.
//-----------------------------------------------------------------------------
class ZGUIDialog
{
private:
    // 다이얼로그 이름
    int m_iDialogID;
    std::string m_Name;

    int m_iX;
    int m_iY;
    int m_iWidth;
    int m_iHeight;
    int m_iDefaultControlID;

    BOOL m_bVisible;
    // Drag 관련 멤버 변수
    BOOL m_bDrag;           // 현재 드래그 중인가?
    POINT m_ptMouseLast;    // 마지막 마우스 위치 (드래그용)

    BOOL m_bDragable;		// 다이얼로그 속성 (이동 가능한 것인가?)

    // m_pControlList, m_pTextureIDList, m_pFontIDList
    // 생성하는 컨트롤의 컨트롤 포인터, 텍스쳐ID, 폰트ID를 동일한 인덱스로 취급하기위해 동기화 //
    // 다이얼로그에 등록된 컨트롤 ( ZGUIControl* )
    std::vector<ZGUIControl*> m_ControlList;
    // 리소스 참조용 포인터
    ZGUIResource* m_pResourceRef;

    // OnMouseMove를 통해 OnMouseLeave와 OnMouseEnter를 처리 ( The control which is hovered over )
    ZGUIControl* m_pControlMouseOver;
    // 컨트롤들의 이벤트를 다이얼로그에서 처리하는 함수 및 전달되는 데이타
    PCALLBACKZGUIEVENT m_pCallbackEvent;
    void* m_pCallbackEventUserContext;

    static ZGUIControl* s_pControlFocus;        // The control which has focus
    //static ZGUIControl* s_pControlPressed;      // The control currently pressed

    std::unique_ptr<DirectX::SpriteBatch> _pSpriteBatch;

private:
    // Windows message handlers
    void OnMouseMove(POINT pt);
    void OnMouseUp(POINT pt);

    // 현재 다이얼로그까지 주어진 좌표에 걸리는 컨트롤을 보유한 다이얼로그 수를 리턴한다.
    int GetDupDialogCount(POINT ptMouse);

public:
    BOOL m_bKeyboardInput;
    BOOL m_bMouseInput;
    // 다이얼로그 클릭/선택 혹은 ZGUIManager에서 SetDialogFocus 지정
    // Ex) OnMouseMove 이벤트의 경우 포커스를 가진 다이얼로그만 반응하려면 필요
    static ZGUIDialog* s_pDialogFocus; // The dialog which has focus
    // 현재 다이얼로그를 최상단 및 포커스 다이얼로그로 설정
    void SetFocus();

    // Methods called by controls
    void SendEvent(int nEvent, ZGUIControl* pControl);
    void RequestFocus(ZGUIControl* pControl);
    // Sets the callback used to notify the app of control events
    BOOL SetCallback(PCALLBACKZGUIEVENT pCallback, void* pUserContext = NULL);

public:
    ZGUIDialog(ZGraphics& gfx);
    ~ZGUIDialog();

    // Need to call this now
    BOOL Init(int iDialogID, const std::string& name, ZGUIResource* pResource);

    // Windows message handler
    BOOL MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Device state notification
    void Refresh();
    BOOL Update(float fElapsedTime);
    BOOL Render(float fElapsedTime);

    // 등록된 컨트롤 제거 (각 다이얼로그의 0번 컨트롤은 배경이므로 지울때 주의)
    void RemoveControl(int iID);
    void RemoveAllControls();

    // 컨트롤 등록
    BOOL AddLabel(int iID, std::string text, int iFont, int iAlignHorizontal, int iAlignVertical, RECT rcDst, BOOL bDefault = FALSE, BOOL bEnable = TRUE, BOOL bVisible = TRUE);
    BOOL AddImage(int iID, std::string text, int iFont, int iAlignHorizontal, int iAlignVertical, int iTexture, RECT rcDst, RECT rcSource, BOOL bDefault = FALSE, BOOL bEnable = TRUE, BOOL bVisible = TRUE);
    BOOL AddButton(int iID, std::string text, int iFont, int iAlignHorizontal, int iAlignVertical, int iTexture, RECT rcDst, RECT rcSource, BOOL bVertical, BOOL bDefault = FALSE, BOOL bEnable = TRUE, BOOL bVisible = TRUE);

    // 컨트롤 참조
    ZGUILabel* GetLabel(int iID) { return static_cast<ZGUILabel*>(GetControl(iID, ZGUI_CONTROL_LABEL)); }
    ZGUIImage* GetImage(int iID) { return static_cast<ZGUIImage*>(GetControl(iID, ZGUI_CONTROL_IMAGE)); }
    ZGUIButton* GetButton(int iID) { return static_cast<ZGUIButton*>(GetControl(iID, ZGUI_CONTROL_BUTTON)); }

    // 컨트롤 사용
    Bind::ZTexture* GetTexture(int iResourceTextureIndex);
    ZFont* GetFont(int iResourceFontIndex);
    ZGUIControl* GetControl(int iID);
    ZGUIControl* GetControl(int iID, int iControlType);
    ZGUIControl* GetControlAtPoint(POINT pt);
    BOOL IsControlEnabled(int iID);
    void EnableControl(int iID, BOOL bEnabled);
    // 컨트롤 포커스
    void SetControlFocus(ZGUIControl* pControl);
    void FocusDefaultControl();
    void ClearFocus();

    BOOL DrawSprite(ZGUIElement* pElement, RECT* prcDest);
    BOOL DrawText(std::string text, ZGUIElement* pElement, RECT* prcDest, BOOL bShadow = FALSE, int iCount = -1);

    // Attributes
    HWND GetHWND();
    const std::string& GetName() const { return m_Name; }
    BOOL GetVisible() { return m_bVisible; }
    void SetVisible(BOOL bVisible) { m_bVisible = bVisible; }
    BOOL GetDragable() { return m_bDragable; }
    void SetDragable(BOOL bDragable) { m_bDragable = bDragable; }
    void GetLocation(POINT& Pt) const { Pt.x = m_iX; Pt.y = m_iY; }
    void SetLocation(int x, int y) { m_iX = x; m_iY = y; }
    void SetSize(int width, int height) { m_iWidth = width; m_iHeight = height; }
    int GetWidth() { return m_iWidth; }
    int GetHeight() { return m_iHeight; }

    ZGUIResource* GetResoruce() { return m_pResourceRef; }
    void EnableKeyboardInput(BOOL bEnable) { m_bKeyboardInput = bEnable; }
    void EnableMouseInput(BOOL bEnable) { m_bMouseInput = bEnable; }
    BOOL IsKeyboardInputEnabled() const { return m_bKeyboardInput; }
    BOOL IsMouseInputEnabled() const { return m_bMouseInput; }
};