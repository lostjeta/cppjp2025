#include "GameGlobal.h"
#include <mmsystem.h> // timeGetTime()
#pragma comment(lib, "winmm.lib")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	ZApp Game(_T("My D3D Test"), 0, 0, 800, 600);
	return Game.Run();
}

ZApp::ZApp(const TCHAR* pszCaption,
	DWORD XPos, DWORD YPos, DWORD Width, DWORD Height)
	: ZApplication(XPos, YPos, Width, Height)
{
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

LRESULT CALLBACK ZApp::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return ZApplication::MsgProc(hWnd, uMsg, wParam, lParam);
}

BOOL ZApp::Shutdown()
{
	return TRUE;
}

BOOL ZApp::Init()
{
	m_pGraphics = new ZGraphics(GetHWnd());

	ShowMouse(TRUE);

	return TRUE;
}

BOOL ZApp::Frame()
{
	// timeGetTime() 함수는 시스템이 시작된 후 경과된 시간을 밀리초(ms) 단위로 반환합니다.
	DWORD timeValue = timeGetTime();
	// 경과 시간을 초 단위로 변환합니다. (1000ms = 1s)
	double dValue = timeValue / 1000.0;
	// sin 함수를 사용하여 시간의 흐름에 따라 0.0 ~ 1.0 사이를 부드럽게 왕복하는 값을 계산합니다.
	// sin(dValue)의 결과는 -1.0 ~ 1.0 이므로, 이를 0.0 ~ 1.0 범위로 정규화합니다.
	const float c = (float)sin(dValue) / 2.0f + 0.5f;
	// 계산된 'c' 값을 사용하여 화면 배경색을 동적으로 변경합니다.
	// Red와 Green 채널이 'c' 값에 따라 변하므로, 배경색이 파란색(0,0,1)과 청록색(1,1,1) 사이를 오가게 됩니다.
	m_pGraphics->ClearBuffer(c, c, 1.0f);

	m_pGraphics->DrawTriangle();

	// 렌더링된 후면 버퍼를 화면에 표시합니다.
	m_pGraphics->EndFrame();
	return TRUE;
}