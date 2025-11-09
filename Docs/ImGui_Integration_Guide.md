# ImGui 통합 가이드

이 문서는 기존 DirectX 11 프로젝트에 ImGui를 통합하는 과정을 단계별로 설명합니다.

## 목차
1. [필수 파일 추가](#1-필수-파일-추가)
2. [GameMain.cpp 수정](#2-gamemaincpp-수정)
3. [ZGraphics.cpp 수정](#3-zgraphicscpp-수정)
4. [프레임 렌더링 통합](#4-프레임-렌더링-통합)

---

## 1. 필수 파일 추가

### 1.1 ImGui 최소 파일 추가
프로젝트에 ImGui 라이브러리의 최소 필수 파일들을 추가합니다.

### 1.2 ImguiManager 클래스 추가
- `ImguiManager.h`
- `ImguiManager.cpp`

---

## 2. GameMain.cpp 수정

### 2.1 헤더 파일 추가
```cpp
#include "imgui/imgui_impl_win32.h"
```

### 2.2 전방 선언 추가
```cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
```

### 2.3 메시지 프로시저 수정
`LRESULT CALLBACK ZApp::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)` 함수의 **첫 부분**에 다음 코드를 추가합니다:

```cpp
if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
{
    return true;
}
```

**목적**: ImGui가 윈도우 메시지를 처리할 수 있도록 합니다.

### 2.4 초기화 함수 수정
`BOOL ZApp::Init()` 함수의 **첫 줄**에 다음 코드를 추가합니다:

```cpp
ImGui_ImplWin32_Init(GetHWnd());
```

**목적**: ImGui의 Win32 플랫폼 백엔드를 초기화합니다.

### 2.5 종료 함수 수정
`BOOL ZApp::Shutdown()` 함수에서 `m_pGraphics` 삭제 **후**에 다음 코드를 추가합니다:

```cpp
ImGui_ImplWin32_Shutdown();
```

**목적**: ImGui의 Win32 플랫폼 백엔드를 정리합니다.

---

## 3. ZGraphics.cpp 수정

### 3.1 헤더 파일 추가
```cpp
#include "imgui/imgui_impl_dx11.h"
```

### 3.2 생성자 수정
`ZGraphics()` 생성자의 **마지막 부분**에 다음 코드를 추가합니다:

```cpp
ImGui_ImplDX11_Init(pDevice.Get(), pContext.Get());
```

**목적**: ImGui의 DirectX 11 렌더링 백엔드를 초기화합니다.

### 3.3 소멸자 수정
`~ZGraphics()` 소멸자에 다음 코드를 추가합니다:

```cpp
ImGui_ImplDX11_Shutdown();
```

**목적**: ImGui의 DirectX 11 렌더링 백엔드를 정리합니다.

---

## 4. 프레임 렌더링 통합

### 4.1 프레임 함수 수정
`BOOL ZApp::Frame()` 함수에 다음 ImGui 렌더링 코드를 추가합니다:

```cpp
// ImGUI
ImGui_ImplDX11_NewFrame();
ImGui_ImplWin32_NewFrame();
ImGui::NewFrame();

static bool show_demo_window = true;
if (show_demo_window)
{
    ImGui::ShowDemoWindow(&show_demo_window);
}
ImGui::Render();
ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
```

### 4.2 코드 설명

#### 프레임 시작
```cpp
ImGui_ImplDX11_NewFrame();
ImGui_ImplWin32_NewFrame();
ImGui::NewFrame();
```
- 새로운 ImGui 프레임을 시작합니다.
- 순서가 중요합니다: DX11 → Win32 → ImGui

#### UI 요소 생성
```cpp
static bool show_demo_window = true;
if (show_demo_window)
{
    ImGui::ShowDemoWindow(&show_demo_window);
}
```
- ImGui의 데모 윈도우를 표시합니다.
- 이 부분을 커스텀 UI 코드로 대체할 수 있습니다.

#### 렌더링
```cpp
ImGui::Render();
ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
```
- ImGui UI를 렌더링합니다.
- `ImGui::Render()`로 렌더링 데이터를 준비하고
- `ImGui_ImplDX11_RenderDrawData()`로 실제 그리기를 수행합니다.

---

## 통합 순서 요약

1. **파일 추가**: ImGui 라이브러리 파일 및 ImguiManager 추가
2. **Win32 통합**: GameMain.cpp에서 Win32 플랫폼 백엔드 설정
3. **DirectX 11 통합**: ZGraphics.cpp에서 DX11 렌더링 백엔드 설정
4. **렌더링 루프**: ZApp::Frame()에서 ImGui 렌더링 코드 추가

---

## 주의사항

### 초기화 순서
1. Win32 백엔드 초기화 (`ImGui_ImplWin32_Init`)
2. DirectX 11 백엔드 초기화 (`ImGui_ImplDX11_Init`)

### 종료 순서
1. DirectX 11 백엔드 종료 (`ImGui_ImplDX11_Shutdown`)
2. Win32 백엔드 종료 (`ImGui_ImplWin32_Shutdown`)

### 프레임 렌더링 순서
1. `ImGui_ImplDX11_NewFrame()`
2. `ImGui_ImplWin32_NewFrame()`
3. `ImGui::NewFrame()`
4. UI 코드 작성
5. `ImGui::Render()`
6. `ImGui_ImplDX11_RenderDrawData()`

---

## 다음 단계

ImGui가 성공적으로 통합되었다면:
- `ImGui::ShowDemoWindow()`를 제거하고 커스텀 UI를 작성할 수 있습니다.
- ImguiManager 클래스를 확장하여 UI 로직을 캡슐화할 수 있습니다.
- 다양한 ImGui 위젯을 사용하여 디버그 UI, 게임 설정 등을 구현할 수 있습니다.

---

## 참고 자료

- [ImGui GitHub](https://github.com/ocornut/imgui)
- [ImGui Wiki](https://github.com/ocornut/imgui/wiki)
- [ImGui Demo](https://github.com/ocornut/imgui/blob/master/imgui_demo.cpp)
