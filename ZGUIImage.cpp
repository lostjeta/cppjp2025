
#include "ZGUI.h"


ZGUIImage::ZGUIImage( ZGUIDialog* pDialog )
{
    m_iType = ZGUI_CONTROL_IMAGE;
    m_pParentDialog = pDialog;

    m_bPressed = FALSE;
	m_nHotkey = 0;
}

//---------------------------------------------------------------------------

ZGUIImage::~ZGUIImage()
{

}

//---------------------------------------------------------------------------

void ZGUIImage::OnHotkey() 
{ 
	if( m_pParentDialog->IsKeyboardInputEnabled() ) 
		m_pParentDialog->RequestFocus( this ); 
	m_pParentDialog->SendEvent( EVENT_IMAGE_CLICKED, this ); 
}

//---------------------------------------------------------------------------
// 현재 컨트롤에 포커스가 있을경우 작동한다.

BOOL ZGUIImage::HandleKeyboard( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	//if( CanHaveFocus() == FALSE )
 //       return FALSE;

 //   switch( uMsg )
 //   {
 //       case WM_KEYDOWN:
 //       {
 //           switch( wParam )
 //           {
 //               case VK_SPACE:
 //                   //m_bPressed = TRUE;
 //                   return TRUE;
 //           }
 //       }

 //       case WM_KEYUP:
 //       {
 //           switch( wParam )
 //           {
	//		case VK_SPACE:
 //               if( m_bPressed == TRUE )
 //               {
 //                   //m_bPressed = FALSE;
 //                   //m_pDialog->SendEvent( EVENT_IMAGE_CLICKED, TRUE, this );
 //               }
 //               return TRUE;
 //           }
 //       }
 //   }

    return FALSE;
}

//---------------------------------------------------------------------------

BOOL ZGUIImage::HandleMouse( UINT uMsg, POINT pt, WPARAM wParam, LPARAM lParam )
{
	if( CanHaveFocus() == FALSE )
        return FALSE;

    switch( uMsg )
    {
        case WM_LBUTTONDOWN:
			if( ContainsPoint( pt ) )
			{
				// Pressed while inside the control
				m_bPressed = TRUE;
				SetCapture( m_pParentDialog->GetHWND() );

				if( !m_bHasFocus )
					m_pParentDialog->RequestFocus( this );

				return TRUE;
			}
			break;

        case WM_LBUTTONUP:	
        {
            if( m_bPressed )
            {
                m_bPressed = FALSE;
                ReleaseCapture();

                if( ContainsPoint( pt ) )
				{
                    m_pParentDialog->SendEvent( EVENT_IMAGE_CLICKED, this );
				}
                return TRUE;
            }
            break;
        }

		case WM_MOUSEMOVE:
		{
			// 이 컨트롤이 속한 대화창이 포커스 상태이고 마우스 오버일 경우
			// 다이얼로그 메시지 처리부에서 OnMouseMove로 m_bMouseOver 처리
			if ( !m_bMouseOver && 
				ZGUIDialog::s_pDialogFocus && 
				ZGUIDialog::s_pDialogFocus == m_pParentDialog &&
				ContainsPoint( pt ) )
			{
				m_pParentDialog->SendEvent( EVENT_IMAGE_OVER, this );
			}
			break;
		}
    };

    return FALSE;
}

//---------------------------------------------------------------------------

void ZGUIImage::Render( ID3D11DeviceContext* pContext, float fElapsedTime )
{
    int nOffsetX = 0;
    int nOffsetY = 0;
    m_iCurState = ZGUI_STATE_NORMAL;

    if( m_bVisible == FALSE )
    {
        m_iCurState = ZGUI_STATE_HIDDEN;
		return;
    }
    else if( m_bEnabled == FALSE )
    {
        m_iCurState = ZGUI_STATE_DISABLED;
    }
    else if( m_bPressed )
    {
        m_iCurState = ZGUI_STATE_PRESSED;

        nOffsetX = 0;//1;
        nOffsetY = 0;//1;
    }
    else if( m_bMouseOver )
    {
        m_iCurState = ZGUI_STATE_MOUSEOVER;

        nOffsetX = 0;//-1;
        nOffsetY = 0;//-1;
    }
    else if( m_bHasFocus )
    {
        m_iCurState = ZGUI_STATE_FOCUS;
    }


    ZGUIElement* pElement = (m_iCurState >= 0 && m_iCurState < (int)m_ElementList.size()) ? m_ElementList[m_iCurState] : nullptr;
	if( pElement == NULL ) return;
	float fBlendRate =  ( m_iCurState == ZGUI_STATE_PRESSED ) ? 0.0f : 0.8f;

	// 이미지 움직임 효과
    RECT rcWindow = m_rcBoundingBox;
    OffsetRect( &rcWindow, nOffsetX, nOffsetY );
 
    // Blend current color
    pElement->TextureColor.Blend( m_iCurState, fElapsedTime, fBlendRate );
    pElement->FontColor.Blend( m_iCurState, fElapsedTime, fBlendRate );

	// Draw
    if (m_Text == "초보자")
    {
        std::cout << m_Text.c_str() << std::endl;
    }
    else
    {
        m_pParentDialog->DrawSprite( pElement, &rcWindow );
        m_pParentDialog->DrawText( m_Text, pElement, &rcWindow );
    }
}

//---------------------------------------------------------------------------

void ZGUIImage::SetTextureColor( DirectX::XMFLOAT4 Color)
{
	ZGUIElement* pElement;

	pElement = m_ElementList[ZGUI_STATE_NORMAL];
	if( pElement )
		pElement->TextureColor.States[ ZGUI_STATE_NORMAL ] = Color;

	pElement = m_ElementList[ZGUI_STATE_DISABLED];
	if( pElement )
		pElement->TextureColor.States[ ZGUI_STATE_DISABLED ] = Color;

	pElement = m_ElementList[ZGUI_STATE_FOCUS];
	if( pElement )
		pElement->TextureColor.States[ ZGUI_STATE_FOCUS ] = Color;

	pElement = m_ElementList[ZGUI_STATE_MOUSEOVER];
	if( pElement )
		pElement->TextureColor.States[ ZGUI_STATE_MOUSEOVER ] = Color;

	pElement = m_ElementList[ZGUI_STATE_PRESSED];
	if( pElement )
		pElement->TextureColor.States[ ZGUI_STATE_PRESSED ] = Color;
}
