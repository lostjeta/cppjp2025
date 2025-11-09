
//-----------------------------------------------------------------------------

#include "ZGUI.h"

//-----------------------------------------------------------------------------

// D3DCOLOR_ARGB macro for DirectX 9 compatibility
#ifndef D3DCOLOR_ARGB
#define D3DCOLOR_ARGB(a,r,g,b) \
    ((DWORD)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#endif

//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------

ZGUIManager::ZGUIManager( int iWinWidth, int iWinHeight )
{
	m_iWinWidth = iWinWidth;
	m_iWinHeight = iWinHeight;

	// 리소스 생성
	m_pResource = new ZGUIResource();

	// Init file 생성
	m_pDefaultRes = new ZInitFile();
	m_pFontRes = new ZInitFile();
	m_pTextureRes = new ZInitFile();
	m_pDialogRes = new ZInitFile();
	m_pControlRes = new ZInitFile();

	m_bInit = FALSE;
}

//-----------------------------------------------------------------------------

ZGUIManager::~ZGUIManager()
{
	Clear();

	SAFE_DELETE( m_pControlRes );
	SAFE_DELETE( m_pDialogRes );
	SAFE_DELETE( m_pTextureRes );
	SAFE_DELETE( m_pFontRes );
	SAFE_DELETE( m_pDefaultRes );

	SAFE_DELETE( m_pResource );
}

//-----------------------------------------------------------------------------

int ZGUIManager::GetWinWidth()
{
	return m_iWinWidth;
}

//-----------------------------------------------------------------------------

int ZGUIManager::GetWinHeight()
{
	return m_iWinHeight;
}

//-----------------------------------------------------------------------------

BOOL ZGUIManager::IsInit()
{
	return m_bInit;
}

//-----------------------------------------------------------------------------
// Need to call this now

BOOL ZGUIManager::Init( ZGraphics* pGraphics )
{
	int iIndex = 0;
	std::vector<std::string> zTempList;

	m_pGraphicsRef = pGraphics;
	m_pResource->Init( m_pGraphicsRef, GetWinWidth(), GetWinHeight() );
	
	// 리소스 파일 로딩
	m_pDefaultRes->LoadIFT( "./Data/DefaultRes.ift" );
	m_pFontRes->LoadIFT( m_pDefaultRes->GetValue( "InitFile", "FontRes" ) );
	m_pTextureRes->LoadIFT( m_pDefaultRes->GetValue( "InitFile", "TextureRes" ) );
	m_pDialogRes->LoadIFT( m_pDefaultRes->GetValue( "InitFile", "DialogRes" ) );
	m_pControlRes->LoadIFT( m_pDefaultRes->GetValue( "InitFile", "ControlRes" ) );

	// 폰트 리소스 초기화
	m_pFontRes->GetTitleList( zTempList );
	for (const auto& sectionName : zTempList)
	{
		// 섹션 이름이 숫자인지 확인 ("0", "1", "2" 등)
		try
		{
			int index = std::stoi(sectionName);
			
			// 숫자 섹션이면 폰트 추가
			m_pResource->AddFont( m_pFontRes->GetValue( index, "FontName" ),
								  std::stoi(m_pFontRes->GetValue( index, "Size" )),
								  std::stoi(m_pFontRes->GetValue( index, "Bold" )),
								  std::stoi(m_pFontRes->GetValue( index, "Italic" )) );
		}
		catch (const std::exception&)
		{
			// 숫자가 아닌 섹션은 무시
			continue;
		}
	}

	// 텍스쳐 리소스 초기화
	zTempList.clear();
	m_pTextureRes->GetTitleList( zTempList );
	for (const auto& sectionName : zTempList)
	{
		// 섹션 이름이 숫자인지 확인
		try
		{
			int index = std::stoi(sectionName);
			
			// Convert ARGB (0-255) to normalized float values (0.0-1.0) for XMFLOAT4
			float a = std::stoi(m_pTextureRes->GetValue( index, "TransparentA" )) / 255.0f;
			float r = std::stoi(m_pTextureRes->GetValue( index, "TransparentR" )) / 255.0f;
			float g = std::stoi(m_pTextureRes->GetValue( index, "TransparentG" )) / 255.0f;
			float b = std::stoi(m_pTextureRes->GetValue( index, "TransparentB" )) / 255.0f;
			DirectX::XMFLOAT4 transparentColor(r, g, b, a);
			
			// Note: Using DXGI_FORMAT_UNKNOWN as default
			m_pResource->AddTexture( (char*)m_pTextureRes->GetValue( index, "FileName" ).c_str(),
									 transparentColor,
									 DXGI_FORMAT_UNKNOWN );
		}
		catch (const std::exception&)
		{
			// 숫자가 아닌 섹션은 무시
			continue;
		}
	}

	// 다이얼로그 및 컨트롤 초기화
	std::vector<std::string> zControlList;
	zTempList.clear();
	m_pDialogRes->GetTitleList( zTempList );
	for (const auto& dialogSectionName : zTempList)
	{
		// 섹션 이름이 숫자인지 확인
		int iDialogIndex;
		try
		{
			iDialogIndex = std::stoi(dialogSectionName);
		}
		catch (const std::exception&)
		{
			// 숫자가 아닌 섹션은 무시
			continue;
		}

		// 다이얼로그 생성
		ZGUIDialog* pDialog = new ZGUIDialog(*pGraphics);

		pDialog->Init( iDialogIndex, m_pDialogRes->GetValue( iDialogIndex, "DialogName" ), m_pResource );
		pDialog->SetLocation( std::stoi(m_pDialogRes->GetValue( iDialogIndex, "X" )),
							  std::stoi(m_pDialogRes->GetValue( iDialogIndex, "Y" )) );
		pDialog->SetSize( std::stoi(m_pDialogRes->GetValue( iDialogIndex, "Width" )),
						  std::stoi(m_pDialogRes->GetValue( iDialogIndex, "Height" )) );
		pDialog->EnableKeyboardInput( std::stoi(m_pDialogRes->GetValue( iDialogIndex, "Keyboard" )) );
		pDialog->EnableMouseInput( std::stoi(m_pDialogRes->GetValue( iDialogIndex, "Mouse" )) );
		pDialog->SetDragable( std::stoi(m_pDialogRes->GetValue( iDialogIndex, "Dragable" )) );
		pDialog->SetVisible( std::stoi(m_pDialogRes->GetValue( iDialogIndex, "Visible" )) );

		// 다이얼로그에 컨트롤 추가
		zControlList.clear();
		m_pControlRes->GetTitleList( zControlList );
		for (const auto& controlSectionName : zControlList)
		{
			// 컨트롤 섹션 이름이 숫자인지 확인
			int iControlIndex;
			try
			{
				iControlIndex = std::stoi(controlSectionName);
			}
			catch (const std::exception&)
			{
				// 숫자가 아닌 섹션은 무시
				continue;
			}
			
			if( m_pDialogRes->GetValue( iDialogIndex, "DialogName" ) == 
				m_pControlRes->GetValue( iControlIndex, "DialogName" ) )
			{
				// 추가할 컨트롤 종류
				switch( std::stoi(m_pControlRes->GetValue( iControlIndex, "Type" )) )
				{
				case 0: // LABEL
					{
						RECT rcDst;

						rcDst.left = std::stoi(m_pControlRes->GetValue( iControlIndex, "dstLeft" ));
						rcDst.top = std::stoi(m_pControlRes->GetValue( iControlIndex, "dstTop" ));
						rcDst.right = std::stoi(m_pControlRes->GetValue( iControlIndex, "dstRight" ));
						rcDst.bottom = std::stoi(m_pControlRes->GetValue( iControlIndex, "dstBottom" ));

						if( FALSE == 
						pDialog->AddLabel( iControlIndex,
										   m_pControlRes->GetValue( iControlIndex, "Text" ).c_str(),
										   std::stoi(m_pControlRes->GetValue( iControlIndex, "FontID" )),
										   std::stoi(m_pControlRes->GetValue( iControlIndex, "AlignHorizontal" )),
										   std::stoi(m_pControlRes->GetValue( iControlIndex, "AlignVertical" )),
										   rcDst,
										   std::stoi(m_pControlRes->GetValue( iControlIndex, "Default" )),
										   std::stoi(m_pControlRes->GetValue( iControlIndex, "Enable" )),
										   std::stoi(m_pControlRes->GetValue( iControlIndex, "Visible" ))
										   )
										   )
						{
							return FALSE;
						}
						break;
					}

				case 1:	// IMAGE
					{
						RECT rcDst, rcSrc;

						rcDst.left = std::stoi(m_pControlRes->GetValue( iControlIndex, "dstLeft" ));
						rcDst.top = std::stoi(m_pControlRes->GetValue( iControlIndex, "dstTop" ));
						rcDst.right = std::stoi(m_pControlRes->GetValue( iControlIndex, "dstRight" ));
						rcDst.bottom = std::stoi(m_pControlRes->GetValue( iControlIndex, "dstBottom" ));

						rcSrc.left = std::stoi(m_pControlRes->GetValue( iControlIndex, "srcLeft" ));
						rcSrc.top = std::stoi(m_pControlRes->GetValue( iControlIndex, "srcTop" ));
						rcSrc.right = std::stoi(m_pControlRes->GetValue( iControlIndex, "srcRight" ));
						rcSrc.bottom = std::stoi(m_pControlRes->GetValue( iControlIndex, "srcBottom" ));

						if( FALSE ==
						pDialog->AddImage( iControlIndex,
										   m_pControlRes->GetValue( iControlIndex, "Text" ).c_str(),
										   std::stoi(m_pControlRes->GetValue( iControlIndex, "FontID" )),
										   std::stoi(m_pControlRes->GetValue( iControlIndex, "AlignHorizontal" )),
										   std::stoi(m_pControlRes->GetValue( iControlIndex, "AlignVertical" )),
										   std::stoi(m_pControlRes->GetValue( iControlIndex, "TextureID" )),
										   rcDst,
										   rcSrc,
										   std::stoi(m_pControlRes->GetValue( iControlIndex, "Default" )),
										   std::stoi(m_pControlRes->GetValue( iControlIndex, "Enable" )),
										   std::stoi(m_pControlRes->GetValue( iControlIndex, "Visible" ))
										   )
										   )
						{
							return FALSE;
						}
						break;
					}

				case 2: // Button
					{
						RECT rcDst, rcSrc;

						rcDst.left = std::stoi(m_pControlRes->GetValue( iControlIndex, "dstLeft" ));
						rcDst.top = std::stoi(m_pControlRes->GetValue( iControlIndex, "dstTop" ));
						rcDst.right = std::stoi(m_pControlRes->GetValue( iControlIndex, "dstRight" ));
						rcDst.bottom = std::stoi(m_pControlRes->GetValue( iControlIndex, "dstBottom" ));

						rcSrc.left = std::stoi(m_pControlRes->GetValue( iControlIndex, "srcLeft" ));
						rcSrc.top = std::stoi(m_pControlRes->GetValue( iControlIndex, "srcTop" ));
						rcSrc.right = std::stoi(m_pControlRes->GetValue( iControlIndex, "srcRight" ));
						rcSrc.bottom = std::stoi(m_pControlRes->GetValue( iControlIndex, "srcBottom" ));

						if( FALSE == 
						pDialog->AddButton( iControlIndex,
											m_pControlRes->GetValue( iControlIndex, "Text" ).c_str(),
											std::stoi(m_pControlRes->GetValue( iControlIndex, "FontID" )),
											std::stoi(m_pControlRes->GetValue( iControlIndex, "AlignHorizontal" )),
											std::stoi(m_pControlRes->GetValue( iControlIndex, "AlignVertical" )),
											std::stoi(m_pControlRes->GetValue( iControlIndex, "TextureID" )),
											rcDst,
											rcSrc,
											std::stoi(m_pControlRes->GetValue( iControlIndex, "Vertical" )),
											std::stoi(m_pControlRes->GetValue( iControlIndex, "Default" )),
											std::stoi(m_pControlRes->GetValue( iControlIndex, "Enable" )),
											std::stoi(m_pControlRes->GetValue( iControlIndex, "Visible" ))
											)
											)
						{
							return FALSE;
						}
						break;
					}

				default:;
				}
			}
		} // end of control (for)

		// 기본 컨트롤 지정
		pDialog->FocusDefaultControl();
	} // end of dialog (while)

	// 최상단 다이얼로그 포커스 부여
	int iDialogCount = m_pResource->GetDialogCount();

    if (iDialogCount > 0)
    {
	    m_pResource->GetDialog( iDialogCount-1 )->SetFocus();
	    // 초기화 완료
	    m_bInit = TRUE;
    }

	return TRUE;
}

//-----------------------------------------------------------------------------

BOOL ZGUIManager::Clear()
{
	m_pControlRes->Clear();
	m_pDialogRes->Clear();
	m_pTextureRes->Clear();
	m_pFontRes->Clear();
	m_pDefaultRes->Clear();
	m_pResource->Clear();
	m_pGraphicsRef = NULL;

	m_bInit = FALSE;
	return TRUE;
}

//-----------------------------------------------------------------------------

BOOL ZGUIManager::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( m_bInit == FALSE )
	{
		std::cout << "[ZGUIManager] Not initialized, ignoring message" << std::endl;
		return FALSE;
	}

	int iCount = m_pResource->GetDialogCount();
	ZGUIDialog* pDialog = NULL;

	// 메시지 처리 순서는 뒤쪽(최상단 다이얼로그)부터 처리하여 마우스 클릭할때
	// 상단 다이얼로그를 쉽게 구별 하는것이다.
	for( int i = (iCount-1); i >= 0; i-- )
	{
		pDialog = m_pResource->GetDialog( i );
		if( pDialog == NULL )
			return FALSE;
		if( pDialog->GetVisible() == TRUE )
			pDialog->MsgProc( hWnd, uMsg, wParam, lParam );
		
	}
	return TRUE;
}

//-----------------------------------------------------------------------------

BOOL ZGUIManager::Update( float fElapsedTime )
{
	if( m_bInit == FALSE )
		return FALSE;

	int iCount = m_pResource->GetDialogCount();
	ZGUIDialog* pDialog = NULL;

	for( int i = 0; i < iCount; i++ )
	{
		pDialog = m_pResource->GetDialog( i );
		if( pDialog == NULL )
			return FALSE;
		if( pDialog->GetVisible() == TRUE )
			pDialog->Update( fElapsedTime );	
	}

	return TRUE;
}

//-----------------------------------------------------------------------------

BOOL ZGUIManager::Render( float fElapsedTime )
{
	if( m_bInit == FALSE )
		return FALSE;

	int iCount = m_pResource->GetDialogCount();
	ZGUIDialog* pDialog = NULL;

	for( int i = 0; i < iCount; i++ )
	{
		pDialog = m_pResource->GetDialog( i );
		if( pDialog == NULL )
			return FALSE;
		if( pDialog->GetVisible() == TRUE )
			pDialog->Render( fElapsedTime );
	}

	return TRUE;
}

//-----------------------------------------------------------------------------

BOOL ZGUIManager::ShowDialog( const std::string& dialogName, BOOL bShow )
{
	int iCount = m_pResource->GetDialogCount();
	ZGUIDialog* pDialog = NULL;

	for( int i = 0; i < iCount; i++ )
	{
		pDialog = m_pResource->GetDialog( i );
		if( pDialog->GetName() == dialogName )
		{
			pDialog->SetVisible( TRUE );
			return TRUE;
		}
	}

	return FALSE;
}

//-----------------------------------------------------------------------------

BOOL ZGUIManager::ShowDialog( ZGUIDialog* pDialog, BOOL bShow )
{
	if( pDialog == NULL )
		return FALSE;

	pDialog->SetVisible( bShow );

	return TRUE;
}

//-----------------------------------------------------------------------------

ZGUIDialog* ZGUIManager::GetDialog( const std::string& dialogName )
{
	int iCount = m_pResource->GetDialogCount();
	ZGUIDialog* pDialog = NULL;

	for( int i = 0; i < iCount; i++ )
	{
		pDialog = m_pResource->GetDialog( i );
		if( pDialog->GetName() == dialogName )
		{
			return pDialog;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------

BOOL ZGUIManager::SetDialogFocus( const std::string& dialogName )
{
	int iCount = m_pResource->GetDialogCount();
	ZGUIDialog* pDialog = NULL;

	for( int i = 0; i < iCount; i++ )
	{
		pDialog = m_pResource->GetDialog( i );
		if( pDialog->GetName() == dialogName )
		{
			pDialog->SetFocus();
			return TRUE;
		}
	}

	return FALSE;
}

//-----------------------------------------------------------------------------

BOOL ZGUIManager::SetDialogFocus( ZGUIDialog* pDialog )
{
	if( pDialog == NULL )
		return FALSE;

	pDialog->SetFocus();

	return TRUE;
}

//-----------------------------------------------------------------------------
