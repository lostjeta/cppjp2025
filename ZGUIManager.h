#pragma once

//Desc:		리소스 등록 및 다이얼로그 생성

class ZGUIManager
{
private:
    BOOL m_bInit;				// 초기화 되었는가?
    ZGraphics* m_pGraphicsRef;
    ZGUIResource* m_pResource;

    ZInitFile* m_pDefaultRes;	// 기본 리소스
    ZInitFile* m_pFontRes;		// 폰트 리소스
    ZInitFile* m_pTextureRes;	// 텍스쳐 리소스
    ZInitFile* m_pDialogRes;	// 다이얼로그 리소스
    ZInitFile* m_pControlRes;	// 컨트롤 리소스

    int m_iWinWidth;
    int m_iWinHeight;

public:
    ZGUIManager(int iWinWidth, int iWinHeight);
    virtual	~ZGUIManager(void);
    int GetWinWidth();
    int GetWinHeight();

    // Need to call this now
    virtual BOOL IsInit();
    virtual BOOL Init(ZGraphics* pGraphics);
    virtual BOOL Clear();
    virtual	BOOL MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual BOOL Update(float fElapsedTime);
    virtual BOOL Render(float fElapsedTime);

    BOOL ShowDialog(const std::string& dialogName, BOOL bShow = TRUE);
    BOOL ShowDialog(ZGUIDialog* pDialog, BOOL bShow = TRUE);
    ZGUIDialog* GetDialog(const std::string& dialogName);
    BOOL SetDialogFocus(const std::string& dialogName);
    BOOL SetDialogFocus(ZGUIDialog* pDialog);
};