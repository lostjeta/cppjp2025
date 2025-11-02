#include "ZGUI.h"

ZGUIResource::ZGUIResource()
{
    m_pGraphicsRef = NULL;	// ID3D11Device, ID3D11DeviceContext
    // std::vector는 자동 초기화됨
}

ZGUIResource::~ZGUIResource()
{
    Clear();
    // std::vector는 자동 소멸됨
    // m_pGraphicsRef는 참조용 포인터이므로 NULL처리
    m_pGraphicsRef = NULL;
}

//---------------------------------------------------------------------------

int ZGUIResource::GetWinWidth()
{
    return m_iWinWidht;
}

//---------------------------------------------------------------------------

int ZGUIResource::GetWinHeight()
{
    return m_iWinHeight;
}

//---------------------------------------------------------------------------

BOOL ZGUIResource::Init(ZGraphics* pGraphics, int iWinWidth, int iWinHeight)
{
    m_pGraphicsRef = pGraphics;
    if (m_pGraphicsRef == NULL)
        return FALSE;

    m_iWinWidht = iWinWidth;
    m_iWinHeight = iWinHeight;

    Clear();

    return TRUE;
}

//---------------------------------------------------------------------------

BOOL ZGUIResource::Clear()
{
    // 각 폰트 객체 삭제
    for (size_t i = 0; i < m_FontList.size(); i++)
    {
        SAFE_DELETE(m_FontList[i]);
    }
    // 각 텍스처 객체 삭제
    for (size_t i = 0; i < m_TextureList.size(); i++)
    {
        SAFE_DELETE(m_TextureList[i]);
    }

    // 벡터 클리어
    m_FontList.clear();
    m_TextureList.clear();
    m_TextureRCList.clear();

    return TRUE;
}

//---------------------------------------------------------------------------

HWND ZGUIResource::GetHWND()
{
    return m_pGraphicsRef->GetHWND();
}

//---------------------------------------------------------------------------

ID3D11Device* ZGUIResource::GetD3DDevice()
{
    if (m_pGraphicsRef == NULL)
        return NULL;

    return m_pGraphicsRef->GetDeviceCOM();
}

//---------------------------------------------------------------------------

ID3D11DeviceContext* ZGUIResource::GetDeviceContext()
{
    if (m_pGraphicsRef == NULL)
        return NULL;

    return m_pGraphicsRef->GetDeviceContext();
}

//---------------------------------------------------------------------------
// 리턴 : 추가된 Index

int ZGUIResource::AddFont(std::string faceName, int iSize, BOOL bBold, BOOL bItalic)
{
    // 기존에 동일한 폰트가 있는지 확인
    for (size_t i = 0; i < m_FontList.size(); i++)
    {
        ZFont* pRegisteredFont = m_FontList[i];

        // 모두 같은가?
        if (strcmp(pRegisteredFont->GetName(), faceName.c_str()) == 0 &&
            pRegisteredFont->GetSize() == iSize &&
            pRegisteredFont->IsBold() == bBold &&
            pRegisteredFont->IsItalic() == bItalic)
        {
            return (int)i;	// 등록된 폰트의 인덱스 리턴
        }
    }

    // 새 폰트 추가
    ZFont* pNewFont = new ZFont();
    pNewFont->Create(*m_pGraphicsRef, faceName.c_str(), (long)iSize, bBold, bItalic);
    m_FontList.push_back(pNewFont);

    return (int)(m_FontList.size() - 1);
}

//---------------------------------------------------------------------------
// 리턴 : 추가된 Index

int ZGUIResource::AddTexture(char* pFileName, DirectX::XMFLOAT4 Transparent, DXGI_FORMAT Format)
{
    // Convert char* to TCHAR* for comparison
    TCHAR tFileNameToAdd[MAX_PATH];
#ifdef UNICODE
    MultiByteToWideChar(CP_ACP, 0, pFileName, -1, tFileNameToAdd, MAX_PATH);
#else
    _tcscpy_s(tFileNameToAdd, MAX_PATH, pFileName);
#endif

    // 기존에 동일한 텍스쳐가 있는지 확인
    for (size_t i = 0; i < m_TextureList.size(); i++)
    {
        Bind::ZTexture* pRegisteredTexture = m_TextureList[i];

        // 모두 같은가?
        if (_tcscmp(pRegisteredTexture->GetFileName(), tFileNameToAdd) == 0)
        {
            return (int)i;	// 등록된 텍스쳐 인덱스 리턴
        }
    }

    // 새 텍스쳐 추가
    // Note: Transparent and Format parameters are not used in current ZTexture implementation
    Bind::ZTexture* pNewTexture = new Bind::ZTexture(*m_pGraphicsRef, tFileNameToAdd);
    m_TextureList.push_back(pNewTexture);

    return (int)(m_TextureList.size() - 1);
}

//---------------------------------------------------------------------------

int ZGUIResource::RegisterDialog(ZGUIDialog* pDialog)
{
    // 다이얼로그 추가
    m_DialogList.push_back(pDialog);
    return (int)(m_DialogList.size() - 1);
}

//---------------------------------------------------------------------------
// 삭제

int ZGUIResource::UnRegisterDialog(ZGUIDialog* pDialog)
{
    // 다이얼로그 찾기
    for (size_t i = 0; i < m_DialogList.size(); i++)
    {
        if (m_DialogList[i] == pDialog)
        {
            m_DialogList.erase(m_DialogList.begin() + i);
            return (int)i;
        }
    }

    return -1;
}

//---------------------------------------------------------------------------

ZFont* ZGUIResource::GetFont(int iIndex)
{
    if (iIndex >= 0 && iIndex < (int)m_FontList.size())
        return m_FontList[iIndex];
    return nullptr;
}

//---------------------------------------------------------------------------

Bind::ZTexture* ZGUIResource::GetTexture(int iIndex)
{
    if (iIndex >= 0 && iIndex < (int)m_TextureList.size())
        return m_TextureList[iIndex];
    return nullptr;
}

//---------------------------------------------------------------------------

ZGUIDialog* ZGUIResource::GetDialog(int iIndex)
{
    if (iIndex >= 0 && iIndex < (int)m_DialogList.size())
        return m_DialogList[iIndex];
    return nullptr;
}

//---------------------------------------------------------------------------

int ZGUIResource::GetDialogCount()
{
    return (int)m_DialogList.size();
}

//---------------------------------------------------------------------------