#include "ZGUI.h"


//---------------------------------------------------------------------------
// sGUIBlendColor

void sGUIBlendColor::Init(DirectX::XMFLOAT4 defaultColor, DirectX::XMFLOAT4 disabledColor, DirectX::XMFLOAT4 hiddenColor)
{
    for (int i = 0; i < MAX_CONTROL_STATES; i++)
    {
        States[i] = defaultColor;
    }

    States[ZGUI_STATE_DISABLED] = disabledColor;
    States[ZGUI_STATE_HIDDEN] = hiddenColor;

    // 컨트롤이 시작될때 기본색이 투명하게 지정되므로 hiddenColor에서
    // 서서히 defaultColor로 변화한다.
    Current = hiddenColor;
}

//---------------------------------------------------------------------------

void sGUIBlendColor::Blend(UINT iState, float fElapsedTime, float fRate)
{
    DirectX::XMVECTOR destColor = DirectX::XMLoadFloat4(&States[iState]);
    DirectX::XMVECTOR currentVec = DirectX::XMLoadFloat4(&Current);
    DirectX::XMVECTOR result = DirectX::XMVectorLerp(currentVec, destColor, 1.0f - powf(fRate, 30 * fElapsedTime));
    DirectX::XMStoreFloat4(&Current, result);
}

//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// ZGUIElement

void ZGUIElement::SetTexture(UINT iTexture, RECT* prcTexture, DirectX::XMFLOAT4 defaultTextureColor)
{
    this->iTexture = iTexture;

    if (prcTexture)
        rcTexture = *prcTexture;
    else
        SetRectEmpty(&rcTexture);

    TextureColor.Init(defaultTextureColor);
}

//---------------------------------------------------------------------------

void ZGUIElement::SetFont(UINT iFont, DirectX::XMFLOAT4 defaultFontColor, DWORD dwTextFormat)
{
    this->iFont = iFont;
    this->dwTextFormat = dwTextFormat;

    FontColor.Init(defaultFontColor);
}

//---------------------------------------------------------------------------

void ZGUIElement::Refresh()
{
    TextureColor.Current = TextureColor.States[ZGUI_STATE_HIDDEN];
    FontColor.Current = FontColor.States[ZGUI_STATE_HIDDEN];
}

//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// ZGUIControl class

ZGUIControl::ZGUIControl(ZGUIDialog* pDialog)
{
    m_iID = 0;
    m_iCurState = 0;
    m_iType = ZGUI_CONTROL_BUTTON;
    m_nHotkey = 0;
    ZeroMemory(&m_rcBoundingBox, sizeof(m_rcBoundingBox));

    m_bEnabled = TRUE;
    m_bVisible = TRUE;
    m_bMouseOver = FALSE;
    m_bHasFocus = TRUE;
    m_bDefault = FALSE;

    // Size, scale, and positioning members
    m_iX = 0;
    m_iY = 0;
    m_iWidth = 0;
    m_iHeight = 0;

    // These members are set by the container
    m_pParentDialog = NULL;

    // ZGUIElement*
    m_ElementList.clear();
    // MAX_CONTROL_STATES 만큼 nullptr 노드 생성
    for (int i = 0; i < MAX_CONTROL_STATES; i++)
        m_ElementList.push_back(nullptr);
}

//---------------------------------------------------------------------------

ZGUIControl::~ZGUIControl()
{
    for (size_t i = 0; i < m_ElementList.size(); i++)
    {
        ZGUIElement* pElement = m_ElementList[i];
        SAFE_DELETE(pElement);
    }
    m_ElementList.clear();
}

//---------------------------------------------------------------------------
// 컨트롤 생성시 m_pElementList에는 6개의 엘리먼트가 Add되어져있다.
// 그러므로 직접 대입하면 된다.

BOOL ZGUIControl::SetElement(int iControlState, ZGUIElement* pElement)
{
    // 입력값 체크
    if (pElement == NULL || iControlState >= MAX_CONTROL_STATES || iControlState < 0)
        return FALSE;

    // 엘리먼트는 항상 MAX_CONTROL_STATES 개다.
    if (m_ElementList.size() != MAX_CONTROL_STATES)
        return FALSE;

    // 기존 엘리먼트 삭제
    SAFE_DELETE(m_ElementList[iControlState]);

    // 새 엘리먼트 생성 및 할당
    ZGUIElement* pNewElement = new ZGUIElement();
    *pNewElement = *pElement;
    m_ElementList[iControlState] = pNewElement;

    return TRUE;
}

//---------------------------------------------------------------------------

void ZGUIControl::Refresh()
{
    m_bMouseOver = FALSE;
    m_bHasFocus = FALSE;

    for (size_t i = 0; i < m_ElementList.size(); i++)
    {
        ZGUIElement* pElement = m_ElementList[i];
        if (pElement)
            pElement->Refresh();
    }
}

//---------------------------------------------------------------------------

void ZGUIControl::UpdateRects()
{
    SetRect(&m_rcBoundingBox, m_iX, m_iY, m_iX + m_iWidth, m_iY + m_iHeight);
}

//---------------------------------------------------------------------------