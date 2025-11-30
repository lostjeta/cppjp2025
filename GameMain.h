#pragma once

// 게임 윈도우 생성/관리
class GameState;
void ChangeState(GameState* newState, ZGraphics& gfx);

class ZApp : public ZApplication
{
private:
	ZGraphics* m_pGraphics;
    DWORD m_lastTime = 0;
    float speedFactor = 1.0f;
    ZCamera cam;
    std::unique_ptr<class ZDirectionalLight> dirLight;
    std::unique_ptr<class ZPointLight> pointLight;

    // 카메라 제어 변수 추가
    bool m_rightMouseDown = false;
    int m_lastMouseX = 0;
    int m_lastMouseY = 0;
    const float CAMERA_MOVE_SPEED = 10.0f;
    const float CAMERA_MOUSE_SENSITIVITY = 0.005f;

    void ProcessCameraInput(float deltaTime);

public:
    Keyboard kbd;
    Mouse mouse;

public:
	virtual LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	ZApp(const TCHAR* pszCaption, DWORD XPos = 0, DWORD YPos = 0, 
        DWORD Width = 640, DWORD Height = 480,
        DWORD ClientWidth = 640, DWORD ClientHeight = 480);
	virtual ~ZApp();

	DWORD GetWidth();
	DWORD GetHeight();

	BOOL Shutdown();
	BOOL Init();
	BOOL Frame();
};