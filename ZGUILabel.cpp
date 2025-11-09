
#include "ZGUI.h"


ZGUILabel::ZGUILabel( ZGUIDialog *pDialog )
{
	m_iType = ZGUI_CONTROL_LABEL;
    m_pParentDialog = pDialog;
	m_bShadow = FALSE;
	m_Text = "";
}

//---------------------------------------------------------------------------

ZGUILabel::~ZGUILabel()
{
	// std::string은 자동으로 소멸됨
}

//---------------------------------------------------------------------------
// 현재 컨트롤에 포커스가 있을경우 작동한다.

BOOL ZGUILabel::HandleKeyboard( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( CanHaveFocus() == FALSE )
        return FALSE;

   // switch( uMsg )
   // {
   //     case WM_KEYDOWN:
   //     {
   //         switch( wParam )
   //         {
   //             case VK_SPACE:
   //                 //m_bPressed = TRUE;
   //                 return TRUE;
   //         }
   //     }

   //     case WM_KEYUP:
   //     {
   //         switch( wParam )
   //         {
			//case VK_SPACE:
   //             if( m_bPressed == TRUE )
   //             {
   //                 //m_bPressed = FALSE;
   //                 //m_pDialog->SendEvent( EVENT_IMAGE_CLICKED, TRUE, this );
   //             }
   //             return TRUE;
   //         }
   //     }
   // }

    return FALSE;
}

//---------------------------------------------------------------------------

BOOL ZGUILabel::HandleMouse( UINT uMsg, POINT pt, WPARAM wParam, LPARAM lParam )
{
	if( CanHaveFocus() == FALSE )
        return FALSE;

  //  switch( uMsg )
  //  {
		//case WM_MOUSEMOVE:
		//{
		//	// 이 컨트롤이 속한 대화창이 포커스 상태이고 마우스 오버일 경우
		//	//if ( !m_bMouseOver && 
		//	//	 ZGUIDialog::s_pDialogFocus && ZGUIDialog::s_pDialogFocus == m_pDialog)
		//	//{
		//	//	// 수정 박찬헌 : 롤오버 이벤트 전송
		//	//	if( ContainsPoint( pt ) && !m_bSendMouseOverEvent)
		//	//	{
		//	//		m_pDialog->SendEvent( EVENT_IMAGE_OVER, TRUE, this );
		//	//		m_bSendMouseOverEvent = TRUE;
		//	//	}
		//	//}
		//	break;
		//}
  //  };

    return FALSE;
}

//---------------------------------------------------------------------------

void ZGUILabel::Render( ID3D11DeviceContext* pContext, float fElapsedTime )
{    
    if( m_bVisible == FALSE )
        return;

    m_iCurState = ZGUI_STATE_NORMAL;
    if( m_bEnabled == FALSE )
        m_iCurState = ZGUI_STATE_DISABLED;

	ZGUIElement* pElement = (m_iCurState >= 0 && m_iCurState < (int)m_ElementList.size()) ? m_ElementList[m_iCurState] : nullptr;
	if( pElement == NULL )
		return;

	pElement->FontColor.Blend( m_iCurState, fElapsedTime );

	m_pParentDialog->DrawText( (char*)m_Text.c_str(), pElement, &m_rcBoundingBox, m_bShadow );
}

//---------------------------------------------------------------------------

BOOL ZGUILabel::SetText( const std::string& text, BOOL bShadow )
{
	m_Text = text;
	m_bShadow = bShadow;

	return TRUE;
}

//---------------------------------------------------------------------------
// 각(6가지 상태) Element가 대표하는 상태의 FontColor만을 세팅한다.
// 기본적으로, 어차피 각 상태의 Element가 반영하는 Font나 Texture의 색은
// 해당 Element가 대표하는 상태에 대한 Color 값이기 때문

void ZGUILabel::SetFontColor( DirectX::XMFLOAT4 Color )
{
	ZGUIElement* pElement;

	pElement = m_ElementList[ZGUI_STATE_NORMAL];
	if( pElement )
		pElement->FontColor.States[ ZGUI_STATE_NORMAL ] = Color;

	pElement = m_ElementList[ZGUI_STATE_DISABLED];
	if( pElement )
		pElement->FontColor.States[ ZGUI_STATE_DISABLED ] = Color;

	pElement = m_ElementList[ZGUI_STATE_FOCUS];
	if( pElement )
		pElement->FontColor.States[ ZGUI_STATE_FOCUS ] = Color;

	pElement = m_ElementList[ZGUI_STATE_MOUSEOVER];
	if( pElement )
		pElement->FontColor.States[ ZGUI_STATE_MOUSEOVER ] = Color;

	pElement = m_ElementList[ZGUI_STATE_PRESSED];
	if( pElement )
		pElement->FontColor.States[ ZGUI_STATE_PRESSED ] = Color;
}
