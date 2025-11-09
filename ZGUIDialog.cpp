
//-----------------------------------------------------------------------------

#include "ZGUI.h"
#include <iostream>

//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------

#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN 0x020B // (not always defined)
#endif
#ifndef WM_XBUTTONUP
#define WM_XBUTTONUP 0x020C // (not always defined)
#endif
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A // (not always defined)
#endif
#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120 // (not always defined)
#endif

ZGUIControl* ZGUIDialog::s_pControlFocus = NULL;	// The control which has focus
//ZGUIControl* ZGUIDialog::s_pControlPressed = NULL;	// The control currently pressed
ZGUIDialog* ZGUIDialog::s_pDialogFocus = NULL;		// The control currently pressed

//-----------------------------------------------------------------------------

ZGUIDialog::ZGUIDialog(ZGraphics& gfx)
{
	m_iX = 0;
    m_iY = 0;
    m_iWidth = 0;
    m_iHeight = 0;
	m_iDefaultControlID = -1;

    m_bVisible = TRUE;
    m_bDrag = FALSE;
	m_bDragable = TRUE;
	m_ptMouseLast.x = 0;
	m_ptMouseLast.y = 0;

	// m_Name은 이미 std::string으로 선언됨
	// m_ControlList는 이미 std::vector<ZGUIControl*>로 선언됨
	// 리소스 참조용 포인터
    m_pResourceRef = NULL;
	// OnMouseMove를 통해 OnMouseLeave와 OnMouseEnter를 처리 ( The control which is hovered over )
    m_pControlMouseOver = NULL;
	// 컨트롤들의 이벤트를 다이얼로그에서 처리하는 함수 및 전달되는 데이타
    m_pCallbackEvent = NULL;
    m_pCallbackEventUserContext = NULL;

	m_bKeyboardInput = FALSE;
	m_bMouseInput = TRUE;

    _pSpriteBatch = std::make_unique<DirectX::SpriteBatch>(gfx.GetDeviceContext());
}

//-----------------------------------------------------------------------------

ZGUIDialog::~ZGUIDialog()
{
	// m_pLog 제거됨 (std::cout 사용)
	// m_Name은 std::string으로 자동 소멸

	for( auto pControl : m_ControlList )
	{
		SAFE_DELETE( pControl );
	}
	m_ControlList.clear();
}

//-----------------------------------------------------------------------------

void ZGUIDialog::OnMouseUp( POINT pt )
{
    //s_pControlPressed = NULL;
    m_pControlMouseOver = NULL;
}

//-----------------------------------------------------------------------------

void ZGUIDialog::OnMouseMove( POINT pt )
{
    // Figure out which control the mouse is over now
	// WM_MOVE 메시지일 경우 NULL이 리턴된다.
    ZGUIControl* pControl = GetControlAtPoint( pt );

    // If the mouse is still over the same control, nothing needs to be done
    if( pControl == m_pControlMouseOver )
        return;

	// 서로다른 컨트롤이므로 Enter/Leave 채크
    // Handle mouse leaving the old control
    if( m_pControlMouseOver )
        m_pControlMouseOver->OnMouseLeave();

    // Handle mouse entering the new control
    m_pControlMouseOver = pControl;
	if( m_pControlMouseOver )
		m_pControlMouseOver->OnMouseEnter();
}

//-----------------------------------------------------------------------------
// 현재 다이얼로그까지 주어진 좌표에 걸리는 컨트롤을 보유한 다이얼로그 수를 리턴한다.

int ZGUIDialog::GetDupDialogCount( POINT ptGlobalMouse )
{
	RECT rcDialog;
	POINT ptLocalMouse;
	int iCount = m_pResourceRef->GetDialogCount();
	int iDupCount = 0;
	ZGUIDialog* pDialog = NULL;

	for( int i = (iCount-1); i >= 0; i-- )
	{
		pDialog = m_pResourceRef->GetDialog( i );
		if( pDialog == NULL ) return 0;
		rcDialog.left = pDialog->m_iX;
		rcDialog.top = pDialog->m_iY;
		rcDialog.right = pDialog->m_iX + pDialog->m_iWidth;
		rcDialog.bottom = pDialog->m_iY + pDialog->m_iHeight;

		// Dup 채크가 타당한 영역의 다이얼로그 인가?
		if( PtInRect( &rcDialog, ptGlobalMouse ) == TRUE )
		{
			ptLocalMouse.x = ptGlobalMouse.x - pDialog->m_iX;
			ptLocalMouse.y = ptGlobalMouse.y - pDialog->m_iY;
			if( pDialog->GetControlAtPoint( ptLocalMouse ) != NULL )
			{
				iDupCount++;
				//m_pLog->Log( "Dup : %s", pDialog->GetName()->c_str() );

				if( this == pDialog )
					break;
			}
		}
	}

	return iDupCount;
}

//-----------------------------------------------------------------------------
// 다이얼로그에 포커스를 주며 랜더링 순서를 최상단 창으로 적용

void ZGUIDialog::SetFocus()
{
	m_pResourceRef->UnRegisterDialog( this );
	// 렌더링 순서 가장 뒤로
	m_pResourceRef->RegisterDialog( this );
	// 현재 다이얼로그를 포커스 다이얼로그로 지정
	s_pDialogFocus = this;
}

//-----------------------------------------------------------------------------
// Methods called by controls

void ZGUIDialog::SendEvent( int iEvent, ZGUIControl* pControl )
{
    // If no callback has been registered there's nowhere to send the event to
    if( m_pCallbackEvent == NULL )
        return;

	// pControl이 NULL인지 여부는 내부에서 채크
    m_pCallbackEvent( iEvent, pControl->GetID(), pControl, m_pCallbackEventUserContext );
}

//-----------------------------------------------------------------------------
// Methods called by controls

void ZGUIDialog::RequestFocus( ZGUIControl* pControl )
{
	// 포커스중인 컨트롤가 요청한 컨트롤이 중복되는가?
    if( s_pControlFocus == pControl )
        return;

	// 요청한 컨트롤이 포커스를 가질 수 있는가?
    if( !pControl->CanHaveFocus() )
        return;

	// 포커스 중인 컨트롤이 있는가?
    if( s_pControlFocus )
        s_pControlFocus->OnFocusOut();

	// 요청 컨트롤로 포커스 교체
    pControl->OnFocusIn();
    s_pControlFocus = pControl;
}

//-----------------------------------------------------------------------------
// Sets the callback used to notify the app of control events

BOOL ZGUIDialog::SetCallback( PCALLBACKZGUIEVENT pCallback, void* pUserContext )
{
	if( pCallback == NULL )
		return FALSE;

    m_pCallbackEvent = pCallback; 
    m_pCallbackEventUserContext = pUserContext;

	return TRUE;
}

//-----------------------------------------------------------------------------
// Need to call this now


BOOL ZGUIDialog::Init(int iDialogID, const std::string& name, ZGUIResource* pResource)
{
    if (pResource == NULL)
        return FALSE;

    m_iDialogID = iDialogID;
    m_Name = name;
    m_pResourceRef = pResource;
    // 모든 다이얼로그는 리소스에 등록되고 참조된다.
    m_pResourceRef->RegisterDialog(this);

    // 다이얼로그 초기화 로그 출력 (std::cout 사용)
    std::cout << "[ZGUIDialog] Initialized: " << name << " (ID: " << iDialogID << ")" << std::endl;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Windows message handler

BOOL ZGUIDialog::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    BOOL bHandled = FALSE;

	// 다이얼로그가 보이나?
    if( m_bVisible == FALSE )
    {
        std::cout << "[Dialog] Not visible, ignoring message" << std::endl;
        return FALSE;
    }

    // 메시지 로그
    if (uMsg == WM_LBUTTONDOWN)
        std::cout << "[Dialog::MsgProc] WM_LBUTTONDOWN received at (" 
                  << short(LOWORD(lParam)) << ", " << short(HIWORD(lParam)) << ")" << std::endl;
    else if (uMsg == WM_LBUTTONUP)
        std::cout << "[Dialog::MsgProc] WM_LBUTTONUP received at (" 
                  << short(LOWORD(lParam)) << ", " << short(HIWORD(lParam)) << ")" << std::endl;
    else if (uMsg == WM_MOUSEMOVE)
        ; // 너무 많아서 주석

	// 클릭/더블클릭 등으로 선택한 다이얼로그를 포커스 다이얼로그로 지정(SetFocus())
	if( uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK )//|| uMsg == WM_LBUTTONUP )
	{
		// 화면 마우스 위치
		POINT ptMouse;
		ptMouse.x = short(LOWORD(lParam));
		ptMouse.y = short(HIWORD(lParam));
		POINT ptCurDialog, ptFocusDialog;
		// 현재 다이얼로그에서의 마우스 상대 좌표
		ptCurDialog.x = ptMouse.x - m_iX;
		ptCurDialog.y = ptMouse.y - m_iY;
		// 포커스 다이얼로그에서의 마우스 상대 좌표
		if( s_pDialogFocus != NULL )
		{
			POINT ptFocusXY;
			s_pDialogFocus->GetLocation( ptFocusXY );
			ptFocusDialog.x = ptMouse.x - ptFocusXY.x;
			ptFocusDialog.y = ptMouse.y - ptFocusXY.y;
		}

		// (상대적상단)다이얼로그가 클릭 지점에 속하는 컨트롤을 가지는가?
		// (MsgProc()는 상단 다이얼로그부터 처리하며 GetControlAtPoint()또한 상단 컨트롤이 먼저 리턴된다.)
		ZGUIControl* pControl = GetControlAtPoint( ptCurDialog );
		if( pControl != NULL )
		{
			// 포커스 다이얼로그가 있는가?
			if( s_pDialogFocus != NULL )
			{
				// 클릭 지점이 포커스 다이얼로그와 일치하는가?
				// 일치할 경우 : 겹쳐진 상단 다이얼로그가 포커스 다이얼로그이다. SetFocus() 무시!
				pControl = s_pDialogFocus->GetControlAtPoint( ptFocusDialog );
				std::cout << "[Dialog] Focus Point: " << ptFocusDialog.x << "," << ptFocusDialog.y << std::endl;
				if( pControl == NULL )
					// 불일치
					SetFocus();
			}
			else
				// 상대적 상단 다이얼로그에 포커스를 준다.
				SetFocus();
		}
	}

	// 포커스 컨트롤 메시지일 경우 먼저 기회를 준다.
	// 포커스 컨트롤이 현재 다이얼로그것이며 키보드입력 가능한 컨트롤이 포커스컨트롤일 때
	// (일반 컨트롤들은 MsgProc를 구현하지 않아 실패(FALSE)한다.)
    if( s_pControlFocus && 
        s_pControlFocus->m_pParentDialog == this && 
        s_pControlFocus->GetEnabled() )
    {
        // If the control MsgProc handles it, then we don't.
        if( s_pControlFocus->MsgProc( uMsg, wParam, lParam ) )
            return TRUE; // Ex) 채팅창 입력일 경우 아래쪽 메시지 처리는 불필요
    }

	// 키보드 입력 없는(ex. 채팅) 일반 컨트롤의 메시지 처리
    switch( uMsg )
    {
        case WM_SIZE:
        case WM_MOVE:
        {
			// 윈도우 밖으로 마우스가 나갔으므로 m_pControlMouseOver 멤버를 초기화 한다.
			// ((-1,-1) 좌표는 컨트롤이 없으므로(NULL) OnMouseMove 내부에서 m_pControlMouseOver를 
			// m_pControlMouseOver->OnMouseLeave() 후 NULL 처리한다.
            POINT pt = { -1, -1 };
            OnMouseMove( pt );
            break;
        }

        case WM_ACTIVATEAPP:
            // Call OnFocusIn()/OnFocusOut() of the control that currently has the focus
            // as the application is activated/deactivated.  This matches the Windows
            // behavior.
            if( s_pControlFocus && 
                s_pControlFocus->m_pParentDialog == this && 
                s_pControlFocus->GetEnabled() )
            {
				std::cout << "[Dialog] WM_ACTIVATEAPP" << std::endl;
                if( wParam == TRUE )	// activated dialog
                    s_pControlFocus->OnFocusIn();
                else					// deactivated dialog
                    s_pControlFocus->OnFocusOut();
            }
            break;

        // Keyboard messages (일반 키보드 적용 및 핫키 등등)
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
			// 포커스 컨트롤에 먼저 기회를 줘야할 이유를 불필요할것 같다. 아직은...
            // If a control is in focus, it belongs to this dialog, and it's enabled, then give
            // it the first chance at handling the message.
    //        if( s_pControlFocus && 
    //            s_pControlFocus->m_pParentDialog == this && 
    //            s_pControlFocus->GetEnabled() )
    //        {
				//m_pLog->Log( "Keyboard Messages" );
    //            if( s_pControlFocus->HandleKeyboard( uMsg, wParam, lParam ) )
    //                return TRUE;
    //        }


			// (20060527:남병철) : 핫키 처리, EDITBOX 및 IMEEDITBOX 처리시 수정 필요 
			// --> 키입력 에디트 컨트롤은 위에서 2번째 MsgProc에서 처리
            // Not yet handled, see if this matches a control's hotkey
            // Activate the hotkey if the focus doesn't belong to an edit box.
			// WM_KEYDOWN : wParam == Specifies the virtual-key code of the nonsystem key
            if( uMsg == WM_KEYDOWN && ( s_pControlFocus == NULL ) )//||
                                     // ( s_pControlFocus->GetType() != DXUT_CONTROL_EDITBOX
                                     //&& s_pControlFocus->GetType() != DXUT_CONTROL_IMEEDITBOX ) ) )
            {
				std::cout << "[Dialog] HotKey: " << (char)wParam << std::endl;
                for( size_t i=0; i < m_ControlList.size(); i++ )
                {
					ZGUIControl* pControl = m_ControlList[i];

					if( pControl == NULL )
						continue;

					if( pControl->GetHotkey() == wParam )
                    {
                        pControl->OnHotkey();
                        return TRUE;
                    }
                }
            }
            break;
        }

		// Mouse messages
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
  //      case WM_MBUTTONDOWN:
  //      case WM_MBUTTONUP:
  //      case WM_RBUTTONDOWN:
  //      case WM_RBUTTONUP:
  //      case WM_XBUTTONDOWN:
  //      case WM_XBUTTONUP:
  //      case WM_LBUTTONDBLCLK:
  //      case WM_MBUTTONDBLCLK:
  //      case WM_RBUTTONDBLCLK:
  //      case WM_XBUTTONDBLCLK:
		//case WM_MOUSEWHEEL:	
		{
			// If not accepting mouse input, return false to indicate the message should still 
			// be handled by the application (usually to move the camera).
			if( !m_bMouseInput )
				return FALSE;

			// 다이얼로그 위의 상대적 마우스 위치 찾기
			// (마우스 위치는 게임 윈도우를 기준으로 값이 전달되며 다이얼로그는 게임 윈도우에서
			//  상대적 위치(m_iX, m_iY)를 가지므로 아래는 다이얼로그 기준의 상대적 위치를 구함)
			POINT ptLocalMouse, ptGlobalMouse;
			RECT rcCurDialog;
			ptGlobalMouse.x = short(LOWORD(lParam));
			ptGlobalMouse.y = short(HIWORD(lParam));
			ptLocalMouse.x = ptGlobalMouse.x - m_iX;
			ptLocalMouse.y = ptGlobalMouse.y - m_iY;
			rcCurDialog.left = m_iX;
			rcCurDialog.top = m_iY;
			rcCurDialog.right = m_iX + m_iWidth;
			rcCurDialog.bottom = m_iY + m_iHeight;

			if( PtInRect( &rcCurDialog, ptGlobalMouse ) == TRUE )
			{
				std::cout << "[Dialog] DupCount(" << ptGlobalMouse.x << "," << ptGlobalMouse.y << ") = " << GetDupDialogCount( ptGlobalMouse ) << std::endl;
				if( GetDupDialogCount( ptGlobalMouse ) == 1 )
				{
					OnMouseMove( ptLocalMouse );	// Mouse enter/leave 채크

					ZGUIControl* pControl = GetControlAtPoint( ptLocalMouse );
					if( pControl != NULL ) 
					{
						// 배경
						if( pControl == static_cast<ZGUIControl*>(m_ControlList[0]) )
						{
							// Mouse not over any controls in this dialog, if there was a control
							// which had focus it just lost it
							// 눌려있는 컨트롤을 제거
							for( size_t i = 0; i < m_ControlList.size(); i++ )
							{
								// 이미지, 버튼 컨트롤들은 눌림 기능이 추가되어 있으므로 해제
								switch( m_ControlList[i]->GetType() )
								{
								case ZGUI_CONTROL_IMAGE:
									{
									ZGUIImage* pImage = static_cast<ZGUIImage*>(m_ControlList[i]);
									if( pImage != NULL && pImage->GetPressed() )
										pImage->HandleMouse( WM_LBUTTONUP, ptLocalMouse, NULL, NULL );
									break;
									}

								case ZGUI_CONTROL_BUTTON:
									{
									ZGUIButton* pButton = static_cast<ZGUIButton*>(m_ControlList[i]);
									if( pButton != NULL && pButton->GetPressed() )
										pButton->HandleMouse( WM_LBUTTONUP, ptLocalMouse, NULL, NULL );
									break;
									}
								}
							}
						}
						else
						{
							std::cout << "[Dialog] Handle Control" << std::endl;
							bHandled = pControl->HandleMouse( uMsg, ptLocalMouse, wParam, lParam );
							if( bHandled )
								return TRUE;
						}
					}
				}
			}

			// 다이얼로그 드래그 구현시 참고
			// Still not handled, hand this off to the dialog. Return false to indicate the
			// message should still be handled by the application (usually to move the camera).
			switch( uMsg )
			{
			case WM_MOUSEMOVE:
				//OnMouseMove( ptLocalMouse );

				if ( !s_pDialogFocus)
					break;
			
				if( m_bDragable && m_bDrag && s_pDialogFocus == this )
				{
					RECT rcClient;
					POINT mousePoint = { short(LOWORD(lParam)), short(HIWORD(lParam)) };
					ZGUIControl* pControl = GetControlAtPoint( mousePoint );
					POINT	ptOffSet;
					
					GetClientRect( m_pResourceRef->GetHWND(), &rcClient );

					std::cout << "[Dialog] [MOVE] Able(" << m_bDragable << ") Drag(" << m_bDrag << ")" << std::endl;
					std::cout << "[Dialog] Focus: " << s_pDialogFocus << " This: " << this << std::endl;

					ptOffSet.x = mousePoint.x - m_ptMouseLast.x;
					ptOffSet.y = mousePoint.y - m_ptMouseLast.y;
					std::cout << "[Dialog] OffsetXY(" << ptOffSet.x << "," << ptOffSet.y << ")" << std::endl;

					m_iX += ptOffSet.x;
					m_iY += ptOffSet.y;
					if ( m_iX < 0 )
						m_iX = 0;
					else if ( m_iX >= ( (rcClient.right - rcClient.left) - m_iWidth) )
						m_iX = (rcClient.right - rcClient.left) - m_iWidth;

					if ( m_iY < 0 )
						m_iY = 0;
					else if ( m_iY >= (rcClient.bottom - rcClient.top) - m_iHeight)
						m_iY = (rcClient.bottom - rcClient.top) - m_iHeight;

					m_ptMouseLast.x = mousePoint.x;
					m_ptMouseLast.y = mousePoint.y;
					std::cout << "[Dialog] NewLastXY(" << m_ptMouseLast.x << "," << m_ptMouseLast.y << ")" << std::endl;

					return TRUE;
				}
				break;

			case WM_LBUTTONDOWN:
				if ( !s_pDialogFocus)
					break;

				if( m_bDragable && !m_bDrag && s_pDialogFocus == this )
				{
					POINT mousePoint = { short(LOWORD(lParam)), short(HIWORD(lParam)) };

					// 다이얼로그 범위 채크
					if( mousePoint.x >= m_iX && mousePoint.x < m_iX + m_iWidth &&
						mousePoint.y >= m_iY && mousePoint.y < m_iY + m_iHeight )
					{
						m_bDrag = TRUE;
						SetCapture( GetHWND() );
						m_ptMouseLast.x = mousePoint.x;
						m_ptMouseLast.y = mousePoint.y;
						std::cout << "[Dialog] LastXY(" << m_ptMouseLast.x << "," << m_ptMouseLast.y << ")" << std::endl;

						return TRUE;
					}
				}
				break;

			case WM_LBUTTONUP:
				if ( m_bDragable && m_bDrag )
				{
					ReleaseCapture();
					m_bDrag = FALSE;

					std::cout << "[Dialog] [UP] Able(" << m_bDragable << ") Drag(" << m_bDrag << ")" << std::endl;
					std::cout << "[Dialog] Focus: " << s_pDialogFocus << " This: " << this << std::endl;

					return TRUE;
				}
				break;

			default:;
			} // End of switch( uMsg )
		break;
		} // End of mouse handled

		case WM_CAPTURECHANGED:
		{
			// The application has lost mouse capture.
			// The dialog object may not have received
			// a WM_MOUSEUP when capture changed. Reset
			// m_bDrag so that the dialog does not mistakenly
			// think the mouse button is still held down.
			if( (HWND)lParam != hWnd )
				m_bDrag = FALSE;
		}

		default:;
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
// Device state notification

void ZGUIDialog::Refresh()
{
    if( s_pControlFocus )
        s_pControlFocus->OnFocusOut();

    if( m_pControlMouseOver )
        m_pControlMouseOver->OnMouseLeave();

    s_pControlFocus = NULL;
    //s_pControlPressed = NULL;
    m_pControlMouseOver = NULL;

	for( auto pControl : m_ControlList )
    {
		if( pControl == NULL )
			continue;
        pControl->Refresh();
    }

	// 키보드(핫키등)를 사용할 수 있으면 지정된 기본 컨트롤에 포커스를 준다.
	// Ex) 리프래쉬 직후 에디트 컨트롤의 커서 깜박임 등
    if( m_bKeyboardInput )
        FocusDefaultControl();
}

//-----------------------------------------------------------------------------

BOOL ZGUIDialog::Update( float fElapsedTime )
{
	return TRUE;
}

//-----------------------------------------------------------------------------

BOOL ZGUIDialog::Render( float fElapsedTime )
{
	if( m_pResourceRef->GetD3DDevice() == NULL )
		return FALSE;

    // For invisible dialog, out now.
    if( m_bVisible == FALSE )
		return TRUE;


    ID3D11DeviceContext* pContext = m_pResourceRef->GetDeviceContext();

    // Note: D3D11 state management is different from D3D9
    // Set up blend state, rasterizer state, etc. using D3D11 state objects
    // This is a placeholder - actual implementation depends on your ZGraphics wrapper
    // You should set up:
    // - Blend State (alpha blending enabled)
    // - Rasterizer State (no culling, fill mode solid)
    // - Depth Stencil State (depth test disabled)
    // - Sampler State (linear filtering, clamp addressing)
    
    // Begin 2D rendering (implementation depends on your sprite system)
    _pSpriteBatch->Begin();

	// 다이얼로그 배경 랜더링
	//DXUTTextureNode* pTextureNode = GetTexture( 0 );
	//pd3dDevice->SetTexture( 0, pTextureNode->pTexture );
	//m_pManager->m_pSprite->Begin( D3DXSPRITE_DONOTSAVESTATE );
	//DrawSprite( &m_BGElement, &m_rcBg);

	// 각 컨트롤 렌더링
	for( auto pControl : m_ControlList )
	{
		// Focused control is drawn last
		if( pControl == s_pControlFocus )
			continue;

		pControl->Render( pContext, fElapsedTime );
	}

	if( s_pControlFocus != NULL && s_pControlFocus->m_pParentDialog == this )
		s_pControlFocus->Render( pContext, fElapsedTime );

	// End 2D rendering
	// Note: In D3D11, restore previous state manually or use state objects
    _pSpriteBatch->End();

	return TRUE;
}

//-----------------------------------------------------------------------------
// 등록된 컨트롤 제거 (각 다이얼로그의 0번 컨트롤은 배경이므로 지울때 주의)

void ZGUIDialog::RemoveControl( int iID )
{
	for( auto it = m_ControlList.begin(); it != m_ControlList.end(); ++it )
	{
		ZGUIControl* pControl = *it;
		if( pControl == NULL )
			continue;

		if( pControl->GetID() == iID )
		{
			m_ControlList.erase(it);
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// (각 다이얼로그의 0번 컨트롤은 배경이므로 지울때 주의)

void ZGUIDialog::RemoveAllControls()
{
	for( auto pControl : m_ControlList )
	{
		SAFE_DELETE( pControl );
	}
	m_ControlList.clear();
}

//-----------------------------------------------------------------------------
// 컨트롤 등록

BOOL ZGUIDialog::AddLabel( int iID, std::string text, int iFont, int iAlignHorizontal, int iAlignVertical, RECT rcDst, BOOL bDefault, BOOL bEnable, BOOL bVisible )
{
	ZGUILabel* pLabel = new ZGUILabel( this );

    if( pLabel == NULL )
        return FALSE;

	m_ControlList.push_back( pLabel );

    // Set the ID and list index
	pLabel->SetID( iID );
	pLabel->SetText( text );
	pLabel->SetLocation( rcDst.left, rcDst.top ); // 영역 채크에 사용
	pLabel->SetSize( rcDst.right - rcDst.left, rcDst.bottom - rcDst.top );
	pLabel->SetDefault( bDefault );
	pLabel->SetEnabled( bEnable );
	pLabel->SetVisible( bVisible );
	if( bDefault == TRUE )
		m_iDefaultControlID = iID;

	// 컨트롤 엘리먼트 세팅
	ZGUIElement Element;
	for( int i = 0; i < MAX_CONTROL_STATES; i++ )
	{
		Element.SetFont( iFont );
		if( iAlignHorizontal == 0 && iAlignVertical == 0 ) Element.dwTextFormat = DT_LEFT|DT_TOP;
		else if( iAlignHorizontal == 0 && iAlignVertical == 1 ) Element.dwTextFormat = DT_LEFT|DT_VCENTER;
		else if( iAlignHorizontal == 0 && iAlignVertical == 2 ) Element.dwTextFormat = DT_LEFT|DT_BOTTOM;
		else if( iAlignHorizontal == 1 && iAlignVertical == 0 ) Element.dwTextFormat = DT_CENTER|DT_TOP;
		else if( iAlignHorizontal == 1 && iAlignVertical == 1 ) Element.dwTextFormat = DT_CENTER|DT_VCENTER;
		else if( iAlignHorizontal == 1 && iAlignVertical == 2 ) Element.dwTextFormat = DT_CENTER|DT_BOTTOM;
		else if( iAlignHorizontal == 2 && iAlignVertical == 0 ) Element.dwTextFormat = DT_RIGHT|DT_TOP;
		else if( iAlignHorizontal == 2 && iAlignVertical == 1 ) Element.dwTextFormat = DT_RIGHT|DT_VCENTER;
		else if( iAlignHorizontal == 2 && iAlignVertical == 2 ) Element.dwTextFormat = DT_RIGHT|DT_BOTTOM;
		else Element.dwTextFormat = DT_CENTER|DT_VCENTER;
		// 모든 컨트롤 엘리먼트는 MAX_CONTROL_STATES 만큼 있다.
		if( FALSE == pLabel->SetElement( i, &Element ) )
			return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------

BOOL ZGUIDialog::AddImage( int iID, std::string text, int iFont, int iAlignHorizontal, int iAlignVertical, int iTexture, RECT rcDst, RECT rcSource, BOOL bDefault, BOOL bEnable, BOOL bVisible )
{
	ZGUIImage* pImage = new ZGUIImage( this );

    if( pImage == NULL )
        return FALSE;

	// 컨트롤 정보 등록
	m_ControlList.push_back( pImage );

    pImage->SetID( iID );
	pImage->SetText( text );
	pImage->SetLocation( rcDst.left, rcDst.top ); // 영역 채크에 사용
	pImage->SetSize( rcDst.right - rcDst.left, rcDst.bottom - rcDst.top );
	pImage->SetDefault( bDefault );
	pImage->SetEnabled( bEnable );
	pImage->SetVisible( bVisible );
	if( bDefault == TRUE )
		m_iDefaultControlID = iID;

	// 컨트롤 엘리먼트 세팅
	ZGUIElement Element;
	for( int i = 0; i < MAX_CONTROL_STATES; i++ )
	{
		Element.SetFont( iFont );
		if( iAlignHorizontal == 0 && iAlignVertical == 0 ) Element.dwTextFormat = DT_LEFT|DT_TOP;
		else if( iAlignHorizontal == 0 && iAlignVertical == 1 ) Element.dwTextFormat = DT_LEFT|DT_VCENTER;
		else if( iAlignHorizontal == 0 && iAlignVertical == 2 ) Element.dwTextFormat = DT_LEFT|DT_BOTTOM;
		else if( iAlignHorizontal == 1 && iAlignVertical == 0 ) Element.dwTextFormat = DT_CENTER|DT_TOP;
		else if( iAlignHorizontal == 1 && iAlignVertical == 1 ) Element.dwTextFormat = DT_CENTER|DT_VCENTER;
		else if( iAlignHorizontal == 1 && iAlignVertical == 2 ) Element.dwTextFormat = DT_CENTER|DT_BOTTOM;
		else if( iAlignHorizontal == 2 && iAlignVertical == 0 ) Element.dwTextFormat = DT_RIGHT|DT_TOP;
		else if( iAlignHorizontal == 2 && iAlignVertical == 1 ) Element.dwTextFormat = DT_RIGHT|DT_VCENTER;
		else if( iAlignHorizontal == 2 && iAlignVertical == 2 ) Element.dwTextFormat = DT_RIGHT|DT_BOTTOM;
		else Element.dwTextFormat = DT_CENTER|DT_VCENTER;
		Element.SetTexture( iTexture, &rcSource );
		// 모든 컨트롤 엘리먼트는 MAX_CONTROL_STATES 만큼 있다.
		if( FALSE == pImage->SetElement( i, &Element ) )
			return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------

BOOL ZGUIDialog::AddButton( int iID, std::string text, int iFont, int iAlignHorizontal, int iAlignVertical, int iTexture, RECT rcDst, RECT rcSource, BOOL bVertical, BOOL bDefault, BOOL bEnable, BOOL bVisible )
{
	int iWidth, iHeight;
    ZGUIButton* pButton = new ZGUIButton( this );

    if( pButton == NULL )
        return FALSE;

	// rcSource를 기준으로 분할된 부분 버튼 크기
	if( bVertical == TRUE )	// 분할 방향
	{
		iWidth = rcSource.right - rcSource.left;
		iHeight = ( rcSource.bottom - rcSource.top ) / 4;
	}
	else
	{
		iWidth = ( rcSource.right - rcSource.left) / 4;
		iHeight = rcSource.bottom - rcSource.top;
	}

	// 컨트롤 정보 등록
	m_ControlList.push_back( pButton );

    pButton->SetID( iID );
	pButton->SetText( text );
	pButton->SetLocation( rcDst.left, rcDst.top );	// 영역 채크에 사용
	pButton->SetSize( rcDst.right - rcDst.left, rcDst.bottom - rcDst.top );
	pButton->SetDefault( bDefault );
	pButton->SetEnabled( bEnable );
	pButton->SetVisible( bVisible );
	if( bDefault == TRUE )
		m_iDefaultControlID = iID;

	// 컨트롤 엘리먼트 세팅
	ZGUIElement Element;
	Element.SetFont( iFont );
	if( iAlignHorizontal == 0 && iAlignVertical == 0 ) Element.dwTextFormat = DT_LEFT|DT_TOP;
	else if( iAlignHorizontal == 0 && iAlignVertical == 1 ) Element.dwTextFormat = DT_LEFT|DT_VCENTER;
	else if( iAlignHorizontal == 0 && iAlignVertical == 2 ) Element.dwTextFormat = DT_LEFT|DT_BOTTOM;
	else if( iAlignHorizontal == 1 && iAlignVertical == 0 ) Element.dwTextFormat = DT_CENTER|DT_TOP;
	else if( iAlignHorizontal == 1 && iAlignVertical == 1 ) Element.dwTextFormat = DT_CENTER|DT_VCENTER;
	else if( iAlignHorizontal == 1 && iAlignVertical == 2 ) Element.dwTextFormat = DT_CENTER|DT_BOTTOM;
	else if( iAlignHorizontal == 2 && iAlignVertical == 0 ) Element.dwTextFormat = DT_RIGHT|DT_TOP;
	else if( iAlignHorizontal == 2 && iAlignVertical == 1 ) Element.dwTextFormat = DT_RIGHT|DT_VCENTER;
	else if( iAlignHorizontal == 2 && iAlignVertical == 2 ) Element.dwTextFormat = DT_RIGHT|DT_BOTTOM;
	else Element.dwTextFormat = DT_CENTER|DT_VCENTER;

	if( bVertical == TRUE )
	{	// 세로 버튼 배열

		// ZGUI_STATE_NORMAL
		rcSource.bottom = rcSource.top + iHeight;
		Element.SetTexture( iTexture, &rcSource );
		pButton->SetElement( ZGUI_STATE_NORMAL, &Element );
		pButton->SetElement( ZGUI_STATE_HIDDEN, &Element );
		pButton->SetElement( ZGUI_STATE_FOCUS, &Element );

		// ZGUI_STATE_DISABLED
		rcSource.top += iHeight;
		rcSource.bottom = rcSource.top + iHeight;
		Element.SetTexture( iTexture, &rcSource );
		pButton->SetElement( ZGUI_STATE_PRESSED, &Element );

		// ZGUI_STATE_MOUSEOVER
		rcSource.top += iHeight;
		rcSource.bottom = rcSource.top + iHeight;
		Element.SetTexture( iTexture, &rcSource );
		pButton->SetElement( ZGUI_STATE_MOUSEOVER, &Element );

		// ZGUI_STATE_PRESSED
		rcSource.top += iHeight;
		rcSource.bottom = rcSource.top + iHeight;
		Element.SetTexture( iTexture, &rcSource );
		pButton->SetElement( ZGUI_STATE_DISABLED, &Element );		
	}
	else
	{	// 가로 버튼 배열

		// ZGUI_STATE_NORMAL
		rcSource.right = rcSource.left + iWidth;
		Element.SetTexture( iTexture, &rcSource );
		pButton->SetElement( ZGUI_STATE_NORMAL, &Element );
		pButton->SetElement( ZGUI_STATE_HIDDEN, &Element );
		pButton->SetElement( ZGUI_STATE_FOCUS, &Element );

		// ZGUI_STATE_DISABLED
		rcSource.left += iWidth;
		rcSource.right = rcSource.left + iWidth;
		Element.SetTexture( iTexture, &rcSource );
		pButton->SetElement( ZGUI_STATE_PRESSED, &Element );

		// ZGUI_STATE_MOUSEOVER
		rcSource.left += iWidth;
		rcSource.right = rcSource.left + iWidth;
		Element.SetTexture( iTexture, &rcSource );
		pButton->SetElement( ZGUI_STATE_MOUSEOVER, &Element );

		// ZGUI_STATE_PRESSED
		rcSource.left += iWidth;
		rcSource.right = rcSource.left + iWidth;
		Element.SetTexture( iTexture, &rcSource );
		pButton->SetElement( ZGUI_STATE_DISABLED, &Element );
	}

	return TRUE;
}

//-----------------------------------------------------------------------------

Bind::ZTexture* ZGUIDialog::GetTexture( int iResourceTextureIndex )
{
	return m_pResourceRef->GetTexture( iResourceTextureIndex );
}

//-----------------------------------------------------------------------------

ZFont* ZGUIDialog::GetFont( int iResourceFontIndex )
{
	return m_pResourceRef->GetFont( iResourceFontIndex );
}

//-----------------------------------------------------------------------------

ZGUIControl* ZGUIDialog::GetControl( int iID )
{
	for( auto pControl : m_ControlList )
	{
		if( pControl == NULL )
			continue;
		if( pControl->GetID() == iID )
			return pControl;
	}

	return NULL;
}

//-----------------------------------------------------------------------------

ZGUIControl* ZGUIDialog::GetControl( int iID, int iControlType )
{
	ZGUIControl* pControl = NULL;

	pControl = GetControl( iID );
	if( pControl != NULL )
	{
		if( pControl->GetType() == iControlType )
			return pControl;
	}

	return NULL;
}

//-----------------------------------------------------------------------------

ZGUIControl* ZGUIDialog::GetControlAtPoint( POINT pt )
{
	// 0번 컨트롤은 배경이므로 항상 걸린다.
	for( int i = (int)m_ControlList.size()-1; i >= 0; i-- )
	{
		ZGUIControl* pControl = m_ControlList[i];
		if( pControl == NULL )
			continue;

        // We only return the current control if it is visible
        // and enabled.  Because GetControlAtPoint() is used to do mouse
        // hittest, it makes sense to perform this filtering.
		if( pControl->ContainsPoint( pt ) && pControl->GetEnabled() && pControl->GetVisible() )
		{
			return pControl;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------

BOOL ZGUIDialog::IsControlEnabled( int iID )
{
	ZGUIControl* pControl = GetControl( iID );

	if( pControl == NULL )
		return FALSE;

	return pControl->GetEnabled();
}

//-----------------------------------------------------------------------------

void ZGUIDialog::EnableControl( int iID, BOOL bEnabled )
{
	ZGUIControl* pControl = GetControl( iID );

	if( pControl != NULL )
		pControl->SetEnabled( bEnabled );
}

//-----------------------------------------------------------------------------
// 컨트롤 포커스

void ZGUIDialog::FocusDefaultControl()
{
	ZGUIControl* pControl = GetControl( m_iDefaultControlID );

	if( pControl != NULL )
		RequestFocus( pControl );
}

//-----------------------------------------------------------------------------

void ZGUIDialog::ClearFocus()
{
    if( s_pControlFocus )
    {
        s_pControlFocus->OnFocusOut();
        s_pControlFocus = NULL;
    }

    ReleaseCapture();
}

//-----------------------------------------------------------------------------

HWND ZGUIDialog::GetHWND()
{
	return m_pResourceRef->GetHWND();
}

//-----------------------------------------------------------------------------

BOOL ZGUIDialog::DrawSprite( ZGUIElement* pElement, RECT* prcDest )
{
	// No need to draw fully transparent layers
	if( pElement->TextureColor.Current.w == 0 )
		return TRUE;

    RECT rcTexture = pElement->rcTexture;   
    RECT rcScreen = *prcDest;
    OffsetRect( &rcScreen, m_iX, m_iY );

	Bind::ZTexture* pTexture = GetTexture( pElement->iTexture );
	if( pTexture == NULL )
		return FALSE;
    
	// 화면 위치와 크기 계산
    DirectX::SimpleMath::Vector2 position((float)rcScreen.left, (float)rcScreen.top);
    
    // 소스 영역 (rcTexture)
    RECT srcRect = rcTexture;
    
    // 목적지 크기에 맞게 스케일 계산
    float fScaleX = (float)RectWidth(rcScreen) / (float)RectWidth(rcTexture);
    float fScaleY = (float)RectHeight(rcScreen) / (float)RectHeight(rcTexture);
    
    // X와 Y 스케일이 다른 경우 평균값 사용 또는 작은 값 사용
    float scale = (fScaleX + fScaleY) / 2.0f;
    
    // 색상 (투명도 포함)
    DirectX::SimpleMath::Vector4 color(
        pElement->TextureColor.Current.x,
        pElement->TextureColor.Current.y,
        pElement->TextureColor.Current.z,
        pElement->TextureColor.Current.w
    );
    
    // Blit 호출
    //return pTexture->Blit(_pSpriteBatch.get(), position, &srcRect, color, 0.0f, DirectX::SimpleMath::Vector2(0.f, 0.f), scale);

    // X와 Y를 독립적으로 스케일하려면 ZTexture::Blit 대신 SpriteBatch를 직접 사용
    _pSpriteBatch->Draw(
        pTexture->GetTextureSRV(),
        RECT{ rcScreen.left, rcScreen.top, rcScreen.right, rcScreen.bottom },  // 목적지 크기
        &srcRect,  // 소스 영역
        DirectX::XMLoadFloat4(&pElement->TextureColor.Current)
    );
    return TRUE;
}

//-----------------------------------------------------------------------------

BOOL ZGUIDialog::DrawText( std::string text, ZGUIElement* pElement, RECT* prcDest, BOOL bShadow, int iCount )
{
    // No need to draw fully transparent layers
    if( pElement->FontColor.Current.w == 0 )
        return TRUE;

    RECT rcScreen = *prcDest;
    OffsetRect( &rcScreen, m_iX, m_iY );

    DirectX::XMMATRIX matTransform;
    matTransform = DirectX::XMMatrixIdentity();
	// Note: Set identity transform for sprite batch

	ZFont* pFont = GetFont( pElement->iFont );
	if( pFont == NULL )
		return FALSE;
    
    if( bShadow )
    {
        RECT rcShadow = rcScreen;
        OffsetRect( &rcShadow, 1, 1 );
		// Shadow color: black with alpha from FontColor
		DirectX::XMFLOAT4 shadowColor(0.0f, 0.0f, 0.0f, pElement->FontColor.Current.w);
		pFont->PrintEx(rcShadow.left, rcShadow.top, 
			rcShadow.right - rcShadow.left, rcShadow.bottom - rcShadow.top, 
			shadowColor, pElement->dwTextFormat, text.c_str(), _pSpriteBatch.get());
    }

	// Draw main text
	pFont->PrintEx(rcScreen.left, rcScreen.top, 
		rcScreen.right - rcScreen.left, rcScreen.bottom - rcScreen.top, 
		pElement->FontColor.Current, pElement->dwTextFormat, text.c_str(), _pSpriteBatch.get());

	return TRUE;
}

//-----------------------------------------------------------------------------

