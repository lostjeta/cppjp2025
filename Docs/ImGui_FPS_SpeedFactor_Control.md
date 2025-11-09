# ImGui를 사용한 FPS 및 SpeedFactor 제어 GUI 구현

이 문서는 ImGui를 사용하여 실시간으로 FPS를 표시하고 시뮬레이션 속도를 조절할 수 있는 GUI를 구현하는 과정을 설명합니다.

## 목차
1. [ZGraphics 수정 - 프레임 관리 개선](#1-zgraphics-수정---프레임-관리-개선)
2. [GameMain 수정 - ImGui UI 추가](#2-gamemain-수정---imgui-ui-추가)
3. [ImGui 토글 기능 추가](#3-imgui-토글-기능-추가)
4. [SpeedFactor 제어 구현](#4-speedfactor-제어-구현)
5. [한글 폰트 지원 추가](#5-한글-폰트-지원-추가)

---

## 1. ZGraphics 수정 - 프레임 관리 개선

### 1.1 목적
ImGui의 초기화와 해제를 `BeginFrame()`과 `EndFrame()` 내부로 캡슐화하여 코드를 깔끔하게 관리합니다.

### 1.2 ZGraphics.h 수정

#### ClearFrame을 protected로 이동
```cpp
class ZGraphics
{
public:
    // ... 기존 public 멤버들 ...
    
    // BeginFrame 추가
    void BeginFrame(float red, float green, float blue) noexcept;
    void EndFrame();

protected:
    // ClearFrame을 protected로 이동
    void ClearFrame(float red, float green, float blue) noexcept;
    
private:
    // ... 기존 private 멤버들 ...
};
```

**변경 이유**: 
- `ClearFrame`을 직접 호출하지 않고 `BeginFrame`을 통해 호출
- 프레임 시작/종료 로직을 명확하게 분리

### 1.3 ZGraphics.cpp 수정

#### BeginFrame 구현
```cpp
void ZGraphics::BeginFrame(float red, float green, float blue) noexcept
{
    // 화면 클리어
    ClearFrame(red, green, blue);
    
    // ImGui 프레임 시작
    if (m_bImguiEnabled)
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }
}
```

#### EndFrame 수정
```cpp
void ZGraphics::EndFrame()
{
    // ImGui 렌더링
    if (m_bImguiEnabled)
    {
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
    
    // 백버퍼를 프론트버퍼로 전환 (화면에 표시)
    m_pSwapChain->Present(1u, 0u);
}
```

### 1.4 ImGui 활성화/비활성화 관리

#### ZGraphics.h에 추가
```cpp
class ZGraphics
{
public:
    // ImGui 제어 함수
    void EnableImgui() { m_bImguiEnabled = true; }
    void DisableImgui() { m_bImguiEnabled = false; }
    bool IsImguiEnabled() const { return m_bImguiEnabled; }

private:
    bool m_bImguiEnabled = true;  // ImGui 활성화 플래그
};
```

---

## 2. GameMain 수정 - ImGui UI 추가

### 2.1 GameMain.h 수정

#### speed_factor 변수 추가
```cpp
class ZApp : public ZApplication
{
public:
    // ... 기존 public 멤버들 ...

private:
    // ... 기존 private 멤버들 ...
    
    // 시뮬레이션 속도 제어
    float m_speedFactor = 1.0f;
};
```

**설명**: 
- `m_speedFactor`: 시뮬레이션 속도 배율 (1.0 = 정상 속도)
- 0.0 ~ 4.0 범위로 조절 가능

### 2.2 GameMain.cpp - Frame() 함수 수정

#### BeginFrame/EndFrame 사용
```cpp
BOOL ZApp::Frame()
{
    // 델타 타임 계산
    static float elapsedTime = 0.0f;
    static ULONGLONG prevTime = GetTickCount64();
    ULONGLONG currentTime = GetTickCount64();
    float dt = (currentTime - prevTime) / 1000.0f;
    prevTime = currentTime;
    
    // ✨ SpeedFactor 적용
    dt = dt * m_speedFactor;
    
    elapsedTime += dt;

    // ✨ BeginFrame으로 화면 클리어 및 ImGui 프레임 시작
    m_pGraphics->BeginFrame(0.0f, 0.0f, 0.0f);
    
    // 게임 로직 업데이트
    Update(dt);
    
    // 렌더링
    Render();
    
    // ✨ ImGui UI 그리기
    if (m_pGraphics->IsImguiEnabled())
    {
        DrawImGuiUI();
    }
    
    // ✨ EndFrame으로 ImGui 렌더링 및 화면 표시
    m_pGraphics->EndFrame();
    
    return TRUE;
}
```

#### ImGui UI 그리기 함수 구현
```cpp
void ZApp::DrawImGuiUI()
{
    static char buffer[1024] = "";  // 입력 테스트용 버퍼
    
    // ImGui 윈도우 생성
    if (ImGui::Begin("Simulation Speed 한글!"))
    {
        // FPS 표시
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                    1000.0f / ImGui::GetIO().Framerate, 
                    ImGui::GetIO().Framerate);
        
        ImGui::Separator();  // 구분선
        
        // SpeedFactor 슬라이더
        ImGui::SliderFloat("Speed Factor", &m_speedFactor, 0.0f, 4.0f);
        
        // 현재 속도 텍스트 표시
        if (m_speedFactor < 0.1f)
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "거의 정지");
        else if (m_speedFactor < 0.5f)
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "느림");
        else if (m_speedFactor < 1.5f)
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "보통");
        else
            ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "빠름");
        
        ImGui::Separator();
        
        // 텍스트 입력 테스트
        ImGui::InputText("Input Test", buffer, sizeof(buffer));
        
        // 추가 정보
        ImGui::Text("Delta Time: %.4f seconds", dt * m_speedFactor);
    }
    ImGui::End();
    
    // ✨ Demo 윈도우 (선택사항)
    static bool show_demo_window = true;
    if (show_demo_window)
    {
        ImGui::ShowDemoWindow(&show_demo_window);
    }
}
```

### 2.3 GameMain.h에 DrawImGuiUI 선언 추가
```cpp
class ZApp : public ZApplication
{
private:
    void DrawImGuiUI();  // ImGui UI 그리기 함수
    float m_speedFactor = 1.0f;
};
```

---

## 3. ImGui 토글 기능 추가

### 3.1 목적
스페이스 키를 눌러 ImGui UI를 켜고 끌 수 있는 기능을 추가합니다.

### 3.2 GameMain.cpp - MsgProc 수정

```cpp
LRESULT CALLBACK ZApp::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // ImGui 메시지 처리 (최우선)
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
    {
        return true;
    }

    switch (uMsg)
    {
    case WM_KEYUP:
        {
            // ✨ 스페이스 키로 ImGui 토글
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
        }
        break;
        
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
```

**동작**:
- 스페이스 키 누름 → ImGui 활성화 상태 확인
- 활성화 상태면 → 비활성화
- 비활성화 상태면 → 활성화

---

## 4. SpeedFactor 제어 구현

### 4.1 작동 원리

```
정상 속도 (speedFactor = 1.0):
dt = 0.016s (약 60 FPS)

느린 속도 (speedFactor = 0.5):
dt = 0.016s * 0.5 = 0.008s
→ 시뮬레이션이 실제 시간의 절반 속도로 진행

빠른 속도 (speedFactor = 2.0):
dt = 0.016s * 2.0 = 0.032s
→ 시뮬레이션이 실제 시간의 2배 속도로 진행

정지 (speedFactor = 0.0):
dt = 0.016s * 0.0 = 0.0s
→ 시뮬레이션이 정지 (일시정지 효과)
```

### 4.2 사용 예제

```cpp
// 물리 업데이트에서 dt 사용
void ZApp::Update(float dt)
{
    // dt는 이미 speedFactor가 적용된 값
    
    // 위치 업데이트
    position += velocity * dt;
    
    // 회전 업데이트
    angle += angularVelocity * dt;
    
    // 애니메이션 업데이트
    animationTime += dt;
}
```

### 4.3 UI 설명

#### SliderFloat 위젯
```cpp
ImGui::SliderFloat("Speed Factor", &m_speedFactor, 0.0f, 4.0f);
```
- **첫 번째 인자**: 표시할 레이블
- **두 번째 인자**: 제어할 변수의 포인터
- **세 번째, 네 번째 인자**: 최소값, 최대값

#### 조건부 색상 텍스트
```cpp
if (m_speedFactor < 0.1f)
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "거의 정지");
```
- `ImVec4(R, G, B, A)`: 색상 값 (0.0 ~ 1.0)
- 속도에 따라 다른 색상으로 상태 표시

---

## 5. 한글 폰트 지원 추가

### 5.1 목적
ImGui에서 한글을 제대로 표시하기 위해 한글 폰트를 추가합니다.

### 5.2 ZGraphics.cpp - 생성자에서 폰트 로드

```cpp
ZGraphics::ZGraphics(HWND hWnd)
{
    // ... DirectX 초기화 코드 ...
    
    // ImGui 초기화
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // ✨ 한글 폰트 추가
    io.Fonts->AddFontFromFileTTF(
        "C:\\Windows\\Fonts\\malgun.ttf",  // 맑은 고딕 폰트
        24.0f,                              // 폰트 크기
        NULL,                               // 폰트 설정 (기본값)
        io.Fonts->GetGlyphRangesKorean()   // 한글 문자 범위
    );
    
    // ImGui 스타일 설정 (선택사항)
    ImGui::StyleColorsDark();
    
    // DirectX 11 백엔드 초기화
    ImGui_ImplDX11_Init(pDevice.Get(), pContext.Get());
}
```

### 5.3 다양한 폰트 사용 예제

#### 여러 폰트 추가
```cpp
ImGuiIO& io = ImGui::GetIO();

// 기본 폰트 (영문)
io.Fonts->AddFontDefault();

// 맑은 고딕 - 24pt
ImFont* font1 = io.Fonts->AddFontFromFileTTF(
    "C:\\Windows\\Fonts\\malgun.ttf", 
    24.0f, 
    NULL, 
    io.Fonts->GetGlyphRangesKorean()
);

// 맑은 고딕 볼드 - 18pt
ImFont* font2 = io.Fonts->AddFontFromFileTTF(
    "C:\\Windows\\Fonts\\malgunbd.ttf", 
    18.0f, 
    NULL, 
    io.Fonts->GetGlyphRangesKorean()
);

// 나눔고딕 - 20pt
ImFont* font3 = io.Fonts->AddFontFromFileTTF(
    "C:\\Windows\\Fonts\\NanumGothic.ttf", 
    20.0f, 
    NULL, 
    io.Fonts->GetGlyphRangesKorean()
);
```

#### 특정 UI에서 다른 폰트 사용
```cpp
void ZApp::DrawImGuiUI()
{
    ImGuiIO& io = ImGui::GetIO();
    
    // 큰 폰트로 제목 표시
    ImGui::PushFont(io.Fonts->Fonts[1]);  // 두 번째 폰트 사용
    ImGui::Text("시뮬레이션 제어 패널");
    ImGui::PopFont();
    
    // 일반 폰트로 나머지 UI
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
}
```

### 5.4 사용 가능한 Windows 한글 폰트

| 폰트 파일 | 이름 | 경로 |
|-----------|------|------|
| `malgun.ttf` | 맑은 고딕 | `C:\Windows\Fonts\malgun.ttf` |
| `malgunbd.ttf` | 맑은 고딕 볼드 | `C:\Windows\Fonts\malgunbd.ttf` |
| `gulim.ttc` | 굴림 | `C:\Windows\Fonts\gulim.ttc` |
| `batang.ttc` | 바탕 | `C:\Windows\Fonts\batang.ttc` |
| `NanumGothic.ttf` | 나눔고딕 | `C:\Windows\Fonts\NanumGothic.ttf` |

---

## 전체 구현 순서 요약

### 1단계: ZGraphics 수정
```
✅ ClearFrame을 protected로 이동
✅ BeginFrame() 추가 - ImGui 프레임 시작 포함
✅ EndFrame() 수정 - ImGui 렌더링 포함
✅ EnableImgui(), DisableImgui(), IsImguiEnabled() 추가
```

### 2단계: GameMain.h 수정
```
✅ m_speedFactor 변수 추가 (float, 기본값 1.0f)
✅ DrawImGuiUI() 함수 선언
```

### 3단계: GameMain.cpp 수정
```
✅ Frame() - BeginFrame/EndFrame 사용
✅ Frame() - dt에 speedFactor 곱하기
✅ Frame() - DrawImGuiUI() 호출
✅ DrawImGuiUI() - UI 구현
✅ MsgProc() - 스페이스 키로 ImGui 토글
```

### 4단계: 한글 폰트 추가
```
✅ ZGraphics 생성자에서 한글 폰트 로드
```

---

## 실행 결과

### ImGui 활성화 상태
- **FPS 표시**: 실시간 프레임 레이트 및 프레임 시간
- **Speed Factor 슬라이더**: 0.0 ~ 4.0 범위
- **상태 텍스트**: 현재 속도에 따른 색상 변경
- **입력 테스트**: 텍스트 입력 가능
- **Demo 윈도우**: ImGui 기능 테스트

### 키보드 제어
- **Space**: ImGui UI 토글 (켜기/끄기)

---

## 주의사항

### 1. 폰트 파일 경로
```cpp
// ❌ 폰트 파일이 없으면 크래시 발생
io.Fonts->AddFontFromFileTTF("없는파일.ttf", 24.0f, NULL, io.Fonts->GetGlyphRangesKorean());

// ✅ 폰트 로드 실패 체크
ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\malgun.ttf", 24.0f, NULL, io.Fonts->GetGlyphRangesKorean());
if (font == NULL)
{
    // 기본 폰트 사용
    io.Fonts->AddFontDefault();
}
```

### 2. SpeedFactor 범위
```cpp
// 너무 큰 값은 시뮬레이션 불안정 초래 가능
if (m_speedFactor > 10.0f)
    m_speedFactor = 10.0f;

// 음수 값 방지
if (m_speedFactor < 0.0f)
    m_speedFactor = 0.0f;
```

### 3. ImGui 상태 확인
```cpp
// ImGui가 비활성화 상태일 때는 UI 그리기 스킵
if (m_pGraphics->IsImguiEnabled())
{
    DrawImGuiUI();
}
```

---

## 확장 아이디어

### 1. 추가 제어 기능
```cpp
// 일시정지 버튼
static bool isPaused = false;
if (ImGui::Button(isPaused ? "Resume" : "Pause"))
{
    isPaused = !isPaused;
    m_speedFactor = isPaused ? 0.0f : 1.0f;
}

// 프리셋 버튼들
if (ImGui::Button("0.25x")) m_speedFactor = 0.25f;
ImGui::SameLine();
if (ImGui::Button("0.5x")) m_speedFactor = 0.5f;
ImGui::SameLine();
if (ImGui::Button("1x")) m_speedFactor = 1.0f;
ImGui::SameLine();
if (ImGui::Button("2x")) m_speedFactor = 2.0f;
ImGui::SameLine();
if (ImGui::Button("4x")) m_speedFactor = 4.0f;
```

### 2. 성능 모니터링
```cpp
// 프레임 타임 그래프
static float values[90] = {};
static int values_offset = 0;
values[values_offset] = 1000.0f / ImGui::GetIO().Framerate;
values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);

ImGui::PlotLines("Frame Times (ms)", values, IM_ARRAYSIZE(values), values_offset, NULL, 0.0f, 33.0f, ImVec2(0, 80));
```

### 3. 디버그 정보
```cpp
ImGui::Text("Screen: %dx%d", screenWidth, screenHeight);
ImGui::Text("Objects: %d", objectCount);
ImGui::Text("Draw Calls: %d", drawCallCount);
ImGui::Text("Memory: %.2f MB", memoryUsage / 1024.0f / 1024.0f);
```

---

## 참고 자료

- [ImGui GitHub](https://github.com/ocornut/imgui)
- [ImGui Demo Code](https://github.com/ocornut/imgui/blob/master/imgui_demo.cpp)
- [ImGui Font Loading](https://github.com/ocornut/imgui/blob/master/docs/FONTS.md)
