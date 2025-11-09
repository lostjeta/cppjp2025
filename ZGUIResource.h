
#pragma once

//Desc:		Texture & Font 정보 관리

#include <vector>

class ZGUIDialog;
class ZGUIResource
{
private:
	int m_iWinWidht;
	int m_iWinHeight;

	ZGraphics* m_pGraphicsRef;	// ID3D11Device, ID3D11DeviceContext
	std::vector<ZFont*> m_FontList;		// Font list
	std::vector<Bind::ZTexture*> m_TextureList;	// Texture list
	std::vector<RECT> m_TextureRCList;	// 리소스로 읽어들일 영역 위치
	RECT rcSourceTexture;	// 해당 텍스쳐에서 읽어들일 부분
	// 다이얼로그 생성시 등록
	//( 다이얼로그 포인터들은 참조용이므로 각각 지울 필요없이 m_DialogList를 제거한다.)
	// ZGUIManager의 SetDialogFocus를 통해
	std::vector<ZGUIDialog*> m_DialogList;	// Dialog list

protected:
public:
	ZGUIResource();
	~ZGUIResource();
	int GetWinWidth();
	int GetWinHeight();

	BOOL Init( ZGraphics* pGraphics, int iWinWidht, int iWinHeight );
	BOOL Clear();
	HWND GetHWND();
	ID3D11Device* GetD3DDevice();
	ID3D11DeviceContext* GetDeviceContext();
	// Note: D3D11 doesn't have state blocks, use manual state management


    int AddFont( std::string faceName = "굴림", int iSize = 12, BOOL bBold = FALSE, BOOL bItalic = FALSE );
    int AddTexture( char* pFileName, DirectX::XMFLOAT4 Transparent = DirectX::XMFLOAT4(0,0,0,0), DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN );
	int RegisterDialog( ZGUIDialog* pDialog );		// 뒤에 추가
	int UnRegisterDialog( ZGUIDialog* pDialog );	// 삭제

    ZFont* GetFont( int iIndex );
    Bind::ZTexture* GetTexture( int iIndex );
	ZGUIDialog* GetDialog( int iIndex );
	int GetDialogCount();
};
