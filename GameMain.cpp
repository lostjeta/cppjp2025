#include "ZD3D11.h"
// Windows에서 메모리 사용량 얻기
#include <Psapi.h>  // GetProcessMemoryInfo용
#include "Sheet.h"
#include "GameState.h"
#include "BasicRenderState.h"
#include "PlayerControlState.h"
#include <mmsystem.h> // timeGetTime()
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "ZCamera.h"
#include "ZPointLight.h"
#include "ZDirectionalLight.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "GameMain.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

GameState* g_currentState = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void ChangeState(GameState* newState, ZGraphics& gfx)
{
    if (g_currentState)
    {
        g_currentState->Exit();
        SAFE_DELETE(g_currentState)
    }
    g_currentState = newState;
    g_currentState->Enter(gfx);
}

void SetupConsole()
{
    AllocConsole();
    FILE* pConsole;
    freopen_s(&pConsole, "CONOUT$", "w", stdout);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    SetupConsole();

    // 클라이언트 영역 크기
    const int clientWidth = 1920;
    const int clientHeight = 1080;

    RECT windowRect = { 0, 0, clientWidth, clientHeight };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    std::cout << "windowRect: " << windowWidth << " " << windowHeight << std::endl;

    // 화면 중앙 위치 계산
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

	ZApp Game(_T("My D3D Test"), x, y, windowWidth, windowHeight, clientWidth, clientHeight);
    try
    {
        BOOL result = Game.Run();
        // 프로그램 종료 전 콘솔 해제
        FreeConsole();
        return result;
    }
    catch (const ChiliException& e)
    {
        MessageBoxA(nullptr, e.what(), e.GetType(), MB_OK | MB_ICONEXCLAMATION);
    }
    catch (const std::exception& e)
    {
        MessageBoxA(nullptr, e.what(), "Standard Exception", MB_OK | MB_ICONEXCLAMATION);
    }
    catch (...)
    {
        MessageBoxA(nullptr, "No details available", "Unknown Exception", MB_OK | MB_ICONEXCLAMATION);
    }

    return -1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

ZApp::ZApp(const TCHAR* pszCaption,
	DWORD XPos, DWORD YPos, DWORD Width, DWORD Height, DWORD ClientWidth, DWORD ClientHeight)
	: ZApplication(XPos, YPos, Width, Height, ClientWidth, ClientHeight)
{
    m_pGraphics = nullptr;
	_tcscpy_s(m_Caption, pszCaption);
}

ZApp::~ZApp()
{
	SAFE_DELETE(m_pGraphics);
}

DWORD ZApp::GetWidth()
{
	return m_Width;
}

DWORD ZApp::GetHeight()
{
	return m_Height;
}

void ZApp::ProcessCameraInput(float deltaTime)
{
    // ImGui 사용 중이면 카메라 제어 무시
    if (m_pGraphics->IsImguiEnabled())
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse || io.WantCaptureKeyboard)
            return;
    }

    // 카메라 방향 벡터
    DirectX::XMFLOAT3 lookDir = cam.GetLookDir();
    DirectX::XMVECTOR lookVec = DirectX::XMLoadFloat3(&lookDir);

    // 오른쪽 벡터
    DirectX::XMVECTOR upVec = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR rightVec = DirectX::XMVector3Normalize(
        DirectX::XMVector3Cross(upVec, lookVec)
    );

    // 카메라의 로컬 up 벡터 (look × right)
    DirectX::XMVECTOR localUpVec = DirectX::XMVector3Normalize(
        DirectX::XMVector3Cross(lookVec, rightVec)
    );

    float speed = CAMERA_MOVE_SPEED * deltaTime;
    DirectX::XMVECTOR posVec = DirectX::XMVectorSet(
        cam.GetXPos(), cam.GetYPos(), cam.GetZPos(), 0.0f
    );

    // WASD 입력
    if (GetAsyncKeyState('W') & 0x8000)
        posVec = DirectX::XMVectorAdd(posVec, DirectX::XMVectorScale(lookVec, speed));
    if (GetAsyncKeyState('S') & 0x8000)
        posVec = DirectX::XMVectorSubtract(posVec, DirectX::XMVectorScale(lookVec, speed));
    if (GetAsyncKeyState('A') & 0x8000)
        posVec = DirectX::XMVectorSubtract(posVec, DirectX::XMVectorScale(rightVec, speed));
    if (GetAsyncKeyState('D') & 0x8000)
        posVec = DirectX::XMVectorAdd(posVec, DirectX::XMVectorScale(rightVec, speed));
    if (GetAsyncKeyState('Q') & 0x8000)
        posVec = DirectX::XMVectorAdd(posVec, DirectX::XMVectorScale(localUpVec, speed));
    if (GetAsyncKeyState('E') & 0x8000)
        posVec = DirectX::XMVectorSubtract(posVec, DirectX::XMVectorScale(localUpVec, speed));

    DirectX::XMFLOAT3 newPos;
    DirectX::XMStoreFloat3(&newPos, posVec);
    cam.Move(newPos.x, newPos.y, newPos.z);
}

LRESULT CALLBACK ZApp::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
    {
        return true;
    }
    
    // ImGui Context가 없으면 GetIO() 호출 시 crash 발생
    // Context는 Init()에서 생성되므로, 그 전에는 기본 처리
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (!ctx)
    {
        return ZApplication::MsgProc(hWnd, uMsg, wParam, lParam);
    }
    
    const auto& imio = ImGui::GetIO();

    switch (uMsg)
    {
    case WM_CREATE:
        timeBeginPeriod(1); // 시간 정밀도를 1ms로 설정
        break;
        
    case WM_DESTROY:
        timeEndPeriod(1); // 해제
        break;

    case WM_CLOSE:
        PostQuitMessage(0); // 메시지 루프를 종료하도록 요청
        break;

    // clear keystate when window loses focus to prevent input getting "stuck"
    case WM_KILLFOCUS:
        kbd.ClearState();
        break;

    /*********** KEYBOARD MESSAGES ***********/
    case WM_KEYDOWN:
    // syskey commands need to be handled to track ALT key (VK_MENU) and F10
    case WM_SYSKEYDOWN: // ALT, F10 추적을 위해 추가
        // stifle this keyboard message if imgui wants to capture
        if (imio.WantCaptureKeyboard)
        {
            break;
        }
        if (!(lParam & 0x40000000) || kbd.AutorepeatIsEnabled()) // filter autorepeat
        {
            kbd.OnKeyPressed(static_cast<unsigned char>(wParam));
        }

        //
        if (g_currentState)
        {
            g_currentState->OnKeyDown(wParam);
        }
        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        // stifle this keyboard message if imgui wants to capture
        if (imio.WantCaptureKeyboard)
        {
            break;
        }
        kbd.OnKeyReleased(static_cast<unsigned char>(wParam));

        //
        if (g_currentState)
        {
            if (wParam == VK_SPACE && m_pGraphics)
            {
                if (m_pGraphics->IsImguiEnabled())
                {
                    m_pGraphics->DisableImgui();
                }
                else
                {
                    m_pGraphics->EnableImgui();
                }
            }
            g_currentState->OnKeyUp(wParam);
        }
        break;

    case WM_CHAR:
        // stifle this keyboard message if imgui wants to capture
        if (imio.WantCaptureKeyboard)
        {
            break;
        }
        kbd.OnChar(static_cast<unsigned char>(wParam));
        break;
    /*********** END KEYBOARD MESSAGES ***********/


    /************* MOUSE MESSAGES ****************/
    case WM_LBUTTONDOWN:
    {
        SetForegroundWindow(hWnd);
        // stifle this mouse message if imgui wants to capture
        if (imio.WantCaptureMouse)
        {
            break;
        }
        const POINTS pt = MAKEPOINTS(lParam);
        mouse.OnLeftPressed(pt.x, pt.y);

        //
        if (g_currentState)
        {
            g_currentState->OnMouseDown(LOWORD(lParam), HIWORD(lParam), 0);
        }
        break;
    }

    case WM_RBUTTONDOWN:
    {
        // stifle this mouse message if imgui wants to capture
        if (imio.WantCaptureMouse)
        {
            break;
        }
        const POINTS pt = MAKEPOINTS(lParam);
        mouse.OnRightPressed(pt.x, pt.y);

        //
        m_rightMouseDown = true;
        m_lastMouseX = LOWORD(lParam);
        m_lastMouseY = HIWORD(lParam);
        SetCapture(hWnd);
        if (g_currentState)
        {
            g_currentState->OnMouseDown(LOWORD(lParam), HIWORD(lParam), 1);
        }
        break;
    }

    case WM_MBUTTONDOWN:
        if (g_currentState)
        {
            int button = (uMsg == WM_LBUTTONDOWN) ? 0 : (uMsg == WM_RBUTTONDOWN) ? 1 : 2;
            g_currentState->OnMouseDown(LOWORD(lParam), HIWORD(lParam), button);
        }
        break;

    case WM_LBUTTONUP:
    {
        // stifle this mouse message if imgui wants to capture
        if (imio.WantCaptureMouse)
        {
            break;
        }
        const POINTS pt = MAKEPOINTS(lParam);
        mouse.OnLeftReleased(pt.x, pt.y);
        // release mouse if outside of window
        if (pt.x < 0 || pt.x >= m_ClientWidth || pt.y < 0 || pt.y >= m_ClientHeight)
        {
            ReleaseCapture();
            mouse.OnMouseLeave();
        }

        //
        if (g_currentState)
        {
            g_currentState->OnMouseUp(LOWORD(lParam), HIWORD(lParam), 0);
        }
        break;
    }

    case WM_RBUTTONUP:
    {
        // stifle this mouse message if imgui wants to capture
        if (imio.WantCaptureMouse)
        {
            break;
        }
        const POINTS pt = MAKEPOINTS(lParam);
        mouse.OnRightReleased(pt.x, pt.y);
        // release mouse if outside of window
        if (pt.x < 0 || pt.x >= m_ClientWidth || pt.y < 0 || pt.y >= m_ClientHeight)
        {
            ReleaseCapture();
            mouse.OnMouseLeave();
        }

        //
        m_rightMouseDown = false;
        ReleaseCapture();
        if (g_currentState)
        {
            g_currentState->OnMouseUp(LOWORD(lParam), HIWORD(lParam), 1);
        }
        break;
    }
    case WM_MBUTTONUP:
        if (g_currentState)
        {
            int button = (uMsg == WM_LBUTTONUP) ? 0 : (uMsg == WM_RBUTTONUP) ? 1 : 2;
            g_currentState->OnMouseUp(LOWORD(lParam), HIWORD(lParam), button);
        }
        break;

    case WM_MOUSEWHEEL:
    {
        // stifle this mouse message if imgui wants to capture
        if (imio.WantCaptureMouse)
        {
            break;
        }
        const POINTS pt = MAKEPOINTS(lParam);
        const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        mouse.OnWheelDelta(pt.x, pt.y, delta);
        break;
    }

    case WM_MOUSEMOVE:
        // stifle this mouse message if imgui wants to capture
        if (imio.WantCaptureMouse)
        {
            break;
        }
        const POINTS pt = MAKEPOINTS(lParam);
        // in client region -> log move, and log enter + capture mouse (if not previously in window)
        if (pt.x >= 0 && pt.x < m_ClientWidth && pt.y >= 0 && pt.y < m_ClientHeight)
        {
            mouse.OnMouseMove(pt.x, pt.y);
            if (!mouse.IsInWindow())
            {
                SetCapture(hWnd);
                mouse.OnMouseEnter();
            }
        }
        // not in client -> log move / maintain capture if button down
        else
        {
            if (wParam & (MK_LBUTTON | MK_RBUTTON))
            {
                mouse.OnMouseMove(pt.x, pt.y);
            }
            // button up -> release capture / log event for leaving
            else
            {
                ReleaseCapture();
                mouse.OnMouseLeave();
            }
        }


        //if (m_rightMouseDown)
        if (wParam & MK_RBUTTON)
        {
            // ImGui가 마우스를 사용 중이면 카메라 회전 무시
            bool ignoreCamera = false;
            if (m_pGraphics && m_pGraphics->IsImguiEnabled())
            {
                ImGuiIO& io = ImGui::GetIO();
                if (io.WantCaptureMouse)
                {
                    ignoreCamera = true;
                }
            }

            if (!ignoreCamera)
            {
                int currentX = LOWORD(lParam);
                int currentY = HIWORD(lParam);
                int deltaX = currentX - m_lastMouseX;
                int deltaY = currentY - m_lastMouseY;

                float yawDelta = deltaX * CAMERA_MOUSE_SENSITIVITY;
                float pitchDelta = deltaY * CAMERA_MOUSE_SENSITIVITY;

                cam.Rotate(
                    cam.GetXRotation() + pitchDelta,
                    cam.GetYRotation() + yawDelta,
                    cam.GetZRotation()
                );

                m_lastMouseX = currentX;
                m_lastMouseY = currentY;
            }
        }

        if (g_currentState)
        {
            g_currentState->OnMouseMove(LOWORD(lParam), HIWORD(lParam));
        }
        break;        

        /************** END MOUSE MESSAGES **************/
    }

	return ZApplication::MsgProc(hWnd, uMsg, wParam, lParam);
}

BOOL ZApp::Shutdown()
{
    // ImGui shutdown 순서 중요!
    ImGui_ImplDX11_Shutdown();    // 1. DX11 백엔드 먼저
    ImGui_ImplWin32_Shutdown();   // 2. Win32 백엔드
    SAFE_DELETE(m_pGraphics);     // 3. 그래픽스 삭제
    ImGui::DestroyContext();      // 4. ImGui Context 파괴 (가장 마지막!)
	return TRUE;
}

BOOL ZApp::Init()
{
    // ImGui Context 생성 (가장 먼저!)
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    
    // 한글 폰트 설정 (24pt)
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\malgun.ttf", 24.0f, NULL, io.Fonts->GetGlyphRangesKorean());
    
    ImGui_ImplWin32_Init(GetHWnd());

	m_pGraphics = new ZGraphics(GetHWnd(), 
        (double)m_ClientHeight / (double)m_ClientWidth
        , m_ClientWidth, m_ClientHeight);

    dirLight = std::make_unique<ZDirectionalLight>(*m_pGraphics);
    pointLight = std::make_unique<ZPointLight>(*m_pGraphics);

    //ChangeState(new BasicRenderState(*m_pGraphics), *m_pGraphics);
    ChangeState(new PlayerControlState(*m_pGraphics), *m_pGraphics);

    // FOV 기반 Projection matrix (올바른 방법)
    float fovAngleY = DirectX::XM_PIDIV4;  // 45도
    float aspectRatio = (float)m_ClientWidth / (float)m_ClientHeight;
    float nearZ = 0.5f;
    float farZ = 500.0f;
    m_pGraphics->SetProjection(DirectX::XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ));
    //m_pGraphics->SetCamera(DirectX::XMMatrixTranslation(0.0f, 0.0f, 20.0f));
    m_pGraphics->SetCamera(cam.GetMatrix());
    m_lastTime = timeGetTime();

	ShowMouse(TRUE);

	return TRUE;
}

BOOL ZApp::Frame()
{
    // 윈도우 내에서 현재 마우스 위치
    //RECT rect;
    //GetWindowRect(GetHWnd(), &rect);
    //POINT pt;
    //GetCursorPos(&pt);
    //pt.x -= rect.left;
    //pt.y -= rect.top;
    //std::cout << pt.x << " " << pt.y << std::endl;


    // timeGetTime() 함수는 시스템이 시작된 후 경과된 시간을 밀리초(ms) 단위로 반환합니다.
    DWORD currentTime = timeGetTime();
    // 경과 시간을 초 단위로 변환합니다. (1000ms = 1s)
    double dElapsed = currentTime / 1000.0; // sec로 변환
    // 프레임 경과 시간
    float dt = (currentTime - m_lastTime) / 1000.0f;
    float dtSave = dt;
    dt = dt * speedFactor;
    m_lastTime = currentTime;

    // sin 함수를 사용하여 시간의 흐름에 따라 0.0 ~ 1.0 사이를 부드럽게 왕복하는 값을 계산합니다.
    // sin(dValue)의 결과는 -1.0 ~ 1.0 이므로, 이를 0.0 ~ 1.0 범위로 정규화합니다.
    const float c = (float)sin(dElapsed) / 2.0f + 0.5f;

    // 계산된 'c' 값을 사용하여 화면 배경색을 동적으로 변경합니다.
    // Red와 Green 채널이 'c' 값에 따라 변하므로, 배경색이 파란색(0,0,1)과 청록색(1,1,1) 사이를 오가게 됩니다.
    m_pGraphics->BeginFrame(0, 0, 1.0f);
    //m_pGraphics->BeginFrame(c, c, 1.0f);
    //m_pGraphics->BeginFrame(0, 0, 0);

    ProcessCameraInput(dtSave);

    cam.Update();
    m_pGraphics->SetCamera(cam.GetMatrix()); // 실시간으로 변하는 카메라 행렬을 ZGraphics에 업데이트 -> ZTransformVSConstBuffer에서 사용.
    dirLight->Bind(*m_pGraphics);
    pointLight->Bind(*m_pGraphics, cam.GetMatrix());
    m_pGraphics->SetViewport();

    if (g_currentState)
    {
        g_currentState->Update(kbd.KeyIsPressed('P') ? 0.0f : dt);
        g_currentState->Render(*m_pGraphics);
    }

    // DirectionalLight
    dirLight->Render(*m_pGraphics);
    
    // PointLight
    pointLight->Render(*m_pGraphics);

    // ImGUI
    if (m_pGraphics->IsImguiEnabled())
    {
        static bool show_demo_window = false;
        if (show_demo_window)
        {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        static char buffer[1024];

        // imgui 윈도우 생성
        ImGui::SetNextWindowSize(ImVec2(450, 165), ImGuiCond_Once);
        if (ImGui::Begin((const char*)u8"Simulation Speed"))
        {
            if (speedFactor < 0.1f)
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), (const char*)u8"거의 정지");
            else if (speedFactor < 0.5f)
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), (const char*)u8"느림");
            else if (speedFactor < 1.5f)
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), (const char*)u8"보통");
            else if (speedFactor < 3.0f)
                ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), (const char*)u8"빠름");
            else
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), (const char*)u8"매우 빠름!");

            ImGui::SliderFloat((const char*)u8"Speed Factor", &speedFactor, 0.0f, 4.0f);
            //ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), (const char*)u8"FPS");
            ImGui::Text((const char*)u8"Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::InputText((const char*)u8"Input Test", buffer, sizeof(buffer));


            PROCESS_MEMORY_COUNTERS pmc;
            GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
            size_t memoryUsage = pmc.WorkingSetSize;  // 바이트 단위

            // MB로 변환 및 색상 결정
            float memoryUsageMB = memoryUsage / 1024.0f / 1024.0f;
            ImVec4 memColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);  // 기본: 회색

            if (memoryUsageMB > 100.0f)
                memColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);      // 빨강: 위험
            else if (memoryUsageMB > 50.0f)
                memColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);      // 주황: 경고

            ImGui::TextColored(memColor, "Memory: %.2f MB", memoryUsageMB);
        }
        ImGui::End();

        cam.SpawnControlWindow();
        dirLight->SpawnControlWindow();
        pointLight->SpawnControlWindow();
    }

	// 렌더링된 후면 버퍼를 화면에 표시합니다.
	m_pGraphics->EndFrame();
	return TRUE;
}