#include "ZGUI.h"
#include "Sheet.h"
#include "SampleBox.h"
#include "BasicRenderState.h"
#include "GraphicsThrowMacros.h"
#include <d3dcompiler.h>
#include <DirectXMath.h> // dx math

namespace wrl = Microsoft::WRL;
namespace dx = DirectX;

extern void ChangeState(GameState* newState, ZGraphics& gfx);

BasicRenderState::BasicRenderState(ZGraphics& gfx)
    : 
    _elapsedTime(0.0), 
    _deltaTime(0.0),
    _curX(0),
    _curY(0),
    _pGraphicsRef(&gfx)
{
}

BasicRenderState::~BasicRenderState()
{
}

void BasicRenderState::Enter(ZGraphics& gfx)
{
    std::cout << "Enter BasicRenderState" << std::endl;

    m_pGUIManager = new ZGUIManager(gfx.GetClientWidth(), gfx.GetClientHeight());

    if (FALSE == m_pGUIManager->Init(&gfx))
        return;


    ZInitFile iniFile;

    std::string path = "./Data/Config.ift";
    if (iniFile.LoadIFT(path)) {
        std::cout << "파일 로드 성공!" << std::endl;
        std::cout << iniFile.GetValue("TEST","Key1");
    }
    else {
        iniFile.SetValue("TEST", "Key1", "Value1");
    }

    m_Font.Create(gfx, "./Data/Font/NanumGothic_Full_16.spritefont",24); // 폰트 생성

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> adist(0.0f, 3.1415f * 2.0f);
    std::uniform_real_distribution<float> ddist(0.0f, 3.1415f * 2.0f);
    std::uniform_real_distribution<float> odist(0.0f, 3.1415f * 0.3f);
    std::uniform_real_distribution<float> rdist(6.0f, 20.0f);
    for (auto i = 0; i < 80; i++)
    {
        boxes.push_back(std::make_unique<SampleBox>(
            gfx, rng, adist,
            ddist, odist, rdist
        ));

        sheets.push_back(std::make_unique<Sheet>(
            gfx, rng, adist,
            ddist, odist, rdist
        ));
    }

    pSpriteBatch = std::make_unique<dx::SpriteBatch>(GetContext(gfx));
    pTexture = std::make_unique<Bind::ZTexture>(gfx, L"dxlogo_256.bmp");
}

void BasicRenderState::Exit()
{
    std::cout << "Exit BasicRenderState" << std::endl;

    m_Font.Free();

    m_pGUIManager->Clear();
    SAFE_DELETE(m_pGUIManager);
}

void BasicRenderState::Update(float deltaTime)
{
    _elapsedTime += deltaTime;
    _deltaTime = deltaTime;

    for (auto& b : boxes) b->Update(deltaTime);
    for (auto& b : sheets) b->Update(deltaTime);

    m_pGUIManager->Update(deltaTime);
}

void BasicRenderState::Render(ZGraphics& gfx)
{
    //DrawTriangle(gfx);
    //DrawIndexedTriangle(gfx);
    //DrawConstTriangle(gfx, _elapsedTime);

    //DrawDepthCube(gfx,
    //    float(-_elapsedTime),
    //    0.0f,
    //    0.0f
    //);
    //DrawDepthCube(gfx, 
    //    float(_elapsedTime), 
    //    (((float)_curX / ((float)GetClientWidth(gfx) / 2.0f)) - 1.0f) * 2,
    //    ((-(float)_curY / ((float)GetClientHeight(gfx) / 2.0f)) + 1.0f) * 2
    //); // using face color

   //  DrawTexture(gfx);
    /*
    for (auto& b : boxes)
    {
        b->Render(gfx);
    }

    for (auto& b : sheets)
    {
        b->Render(gfx);
    }
    */

    const float blendFactor[4] = {0.0f,0.0f ,0.0f ,0.0f };
    GetContext(gfx)->OMSetBlendState(GetBlendState(gfx), blendFactor, 0xffffffff);

    m_pGUIManager->Render(_deltaTime);

    pSpriteBatch->Begin();
    if (m_Font.GetSpriteFont() != nullptr) {
        m_Font.FastPrint(100, 80, u8"안녕", pSpriteBatch.get()); // 한글폰트까지 출력가능하게 수정
        m_Font.FastPrint(100, 100, "Hello World", pSpriteBatch.get());
        m_Font.FastPrint(100, 120, "Welcome!", pSpriteBatch.get());
        m_Font.FastPrint(100, 140, "Font Test", pSpriteBatch.get());

        // Color test (Red)
        DirectX::XMFLOAT4 red(1.0f, 0.0f, 0.0f, 1.0f);
        m_Font.PrintLine(100, 160, 300, red, "Red Text", pSpriteBatch.get());

        // Green text
        DirectX::XMFLOAT4 green(0.0f, 1.0f, 0.0f, 1.0f);
        m_Font.PrintLine(100, 180, 300, green, "Green Text", pSpriteBatch.get());

        // Blue text
        DirectX::XMFLOAT4 blue(0.0f, 0.0f, 1.0f, 1.0f);
        m_Font.PrintLine(100, 200, 300, blue, "Blue Text", pSpriteBatch.get());

        // Centered text (Top)
        DirectX::XMFLOAT4 white(1.0f, 1.0f, 1.0f, 1.0f);
        m_Font.PrintEx(0, 0, 800, 100, white, DT_CENTER | DT_VCENTER, "Game Title", pSpriteBatch.get());

        // Centered text (Bottom)
        DirectX::XMFLOAT4 yellow(1.0f, 1.0f, 0.0f, 1.0f);
        m_Font.PrintEx(0, 500, 800, 100, yellow, DT_CENTER | DT_BOTTOM, "Press SPACE to start", pSpriteBatch.get());
    }
   

    pSpriteBatch->End();

    GetContext(gfx)->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
}

void BasicRenderState::OnKeyDown(WPARAM wParam)
{
    if (wParam == VK_RETURN)
    {
        std::cout << "Goto PlayState" << std::endl;
        //ChangeState(new PlayState());
    }
}

void BasicRenderState::OnKeyUp(WPARAM wParam)
{

}

void BasicRenderState::OnMouseDown(int x, int y, int button)
{
    std::cout << "mouse down" << std::endl;
    if (m_pGUIManager && m_pGUIManager->IsInit())
    {
        UINT uMsg = (button == 0) ? WM_LBUTTONDOWN : (button == 1) ? WM_RBUTTONDOWN : WM_MBUTTONDOWN;
        LPARAM IParam = MAKELPARAM(x, y);
        WPARAM wParam = (button == 0) ? MK_LBUTTON : (button == 1) ? MK_RBUTTON : MK_MBUTTON;

        // ZGUIManager에 메시지 전달
        HWND hWnd = _pGraphicsRef->GetHWND();
        m_pGUIManager->MsgProc(hWnd, uMsg, wParam, IParam);
    }
}

void BasicRenderState::OnMouseUp(int x, int y, int button)
{
    std::cout << "mouse up" << std::endl;
    if (m_pGUIManager && m_pGUIManager->IsInit())
    {
        UINT uMsg = (button == 0) ? WM_LBUTTONUP : (button == 1) ? WM_RBUTTONUP : WM_MBUTTONUP;
        LPARAM IParam = MAKELPARAM(x, y);
        WPARAM wParam = (button == 0) ? MK_LBUTTON : (button == 1) ? MK_RBUTTON : MK_MBUTTON;

        // ZGUIManager에 메시지 전달
        HWND hWnd = _pGraphicsRef->GetHWND();
        m_pGUIManager->MsgProc(hWnd, uMsg, wParam, IParam);
    }
}

void BasicRenderState::OnMouseMove(int x, int y)
{
    _curX = x;
    _curY = y;
    
    if (m_pGUIManager && m_pGUIManager->IsInit()) {
        LPARAM IParam = MAKELPARAM(x, y);
        WPARAM wParam = 0;

        HWND hWnd = _pGraphicsRef->GetHWND();
        m_pGUIManager->MsgProc(hWnd, WM_MOUSEMOVE, wParam, IParam);
    }
}
