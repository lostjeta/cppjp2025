---
marp: true
theme: default
paginate: true
header: 'ImGui FPS & SpeedFactor 제어'
footer: 'DirectX 11 + ImGui'
style: |
  section {
    font-size: 26px;
  }
  code {
    font-size: 18px;
  }
  pre {
    font-size: 16px;
  }
---

# ImGui를 사용한 FPS 및 SpeedFactor 제어

**실시간 FPS 표시 및 시뮬레이션 속도 조절 GUI 구현**

---

## 목차

1. ZGraphics 수정 - 프레임 관리 개선
2. GameMain 수정 - ImGui UI 추가
3. ImGui 토글 기능 추가
4. SpeedFactor 제어 구현
5. 한글 폰트 지원 추가
6. 확장 아이디어

---

## 구현 목표

### 기능
- 실시간 FPS 표시
- 시뮬레이션 속도 조절 (0.0 ~ 4.0x)
- 스페이스 키로 UI 토글
- 한글 폰트 지원

### 장점
- 디버깅 효율성 향상
- 실시간 파라미터 조정
- 성능 모니터링

---

## ZGraphics 수정 (1/4)

### 목적
ImGui 초기화/해제를 `BeginFrame()`과 `EndFrame()`으로 캡슐화

### ZGraphics.h 수정

```cpp
class ZGraphics
{
public:
    // 새로운 프레임 관리 함수
    void BeginFrame(float red, float green, float blue) noexcept;
    void EndFrame();

protected:
    // ClearBuffer을 protected로 이동
    void ClearBuffer(float red, float green, float blue) noexcept;
};
```

---

## ZGraphics 수정 (2/4)

### BeginFrame 구현

```cpp
void ZGraphics::BeginFrame(float red, float green, float blue) noexcept
{
    // 화면 클리어
    ClearBuffer(red, green, blue);
    
    // ImGui 프레임 시작
    if (m_bImguiEnabled)
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }
}
```

**순서**: 화면 클리어 → ImGui 프레임 시작

---

## ZGraphics 수정 (3/4)

### EndFrame 구현

```cpp
void ZGraphics::EndFrame()
{
    // ImGui 렌더링
    if (m_bImguiEnabled)
    {
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
    
    // 백버퍼를 프론트버퍼로 전환
    m_pSwapChain->Present(1u, 0u);
}
```

**순서**: ImGui 렌더링 → 화면 표시

---

## ZGraphics 수정 (4/4)

### ImGui 제어 함수 추가

```cpp
class ZGraphics
{
public:
    void EnableImgui() { m_bImguiEnabled = true; }
    void DisableImgui() { m_bImguiEnabled = false; }
    bool IsImguiEnabled() const { return m_bImguiEnabled; }

private:
    bool m_bImguiEnabled = true;  // 기본값: 활성화
};
```

---

## GameMain.h 수정

### SpeedFactor 변수 추가

```cpp
class ZApp : public ZApplication
{
public:
    // ... 기존 멤버들 ...

private:
    void DrawImGuiUI();          // ImGui UI 그리기 함수
    float m_speedFactor = 1.0f;  // 시뮬레이션 속도 배율
};
```

**의미**:
- `m_speedFactor = 1.0f`: 정상 속도
- `m_speedFactor = 0.5f`: 절반 속도 (슬로우 모션)
- `m_speedFactor = 2.0f`: 2배 속도 (빠른 재생)

---

## GameMain.cpp - Frame() 수정 (1/2)

### BeginFrame/EndFrame 사용

```cpp
BOOL ZApp::Frame()
{
    // 델타 타임 계산
    static ULONGLONG prevTime = GetTickCount64();
    ULONGLONG currentTime = GetTickCount64();
    float dt = (currentTime - prevTime) / 1000.0f;
    prevTime = currentTime;
    
    // SpeedFactor 적용
    dt = dt * m_speedFactor;
    
    // BeginFrame: 화면 클리어 + ImGui 시작
    m_pGraphics->BeginFrame(0.0f, 0.0f, 0.0f);
```

---

## GameMain.cpp - Frame() 수정 (2/2)

```cpp
    // 게임 로직 업데이트
    Update(dt);
    
    // 렌더링
    Render();
    
    // ImGui UI 그리기
    if (m_pGraphics->IsImguiEnabled())
    {
        DrawImGuiUI();
    }
    
    // EndFrame: ImGui 렌더링 + 화면 표시
    m_pGraphics->EndFrame();
    
    return TRUE;
}
```

---

## DrawImGuiUI() 구현 (1/3)

### UI 윈도우 생성

```cpp
void ZApp::DrawImGuiUI()
{
    static char buffer[1024] = "";
    
    if (ImGui::Begin("Simulation Speed 한글!"))
    {
        // FPS 표시
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                    1000.0f / ImGui::GetIO().Framerate, 
                    ImGui::GetIO().Framerate);
        
        ImGui::Separator();
```

---

## DrawImGuiUI() 구현 (2/3)

### SpeedFactor 슬라이더 및 상태 표시

```cpp
        // SpeedFactor 슬라이더 (0.0 ~ 4.0)
        ImGui::SliderFloat("Speed Factor", &m_speedFactor, 0.0f, 4.0f);
        
        // 속도에 따른 색상 피드백
        if (m_speedFactor < 0.1f)
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "거의 정지");
        else if (m_speedFactor < 0.5f)
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "느림");
        else if (m_speedFactor < 1.5f)
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "보통");
        else
            ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "빠름");
```

---

## DrawImGuiUI() 구현 (3/3)

### 입력 테스트 및 윈도우 종료

```cpp
        ImGui::Separator();
        
        // 텍스트 입력 테스트
        ImGui::InputText("Input Test", buffer, sizeof(buffer));
    }
    ImGui::End();
    
    // Demo 윈도우 (선택사항)
    static bool show_demo = true;
    if (show_demo)
    {
        ImGui::ShowDemoWindow(&show_demo);
    }
}
```

---

## ImGui 토글 기능

### 스페이스 키로 UI 켜기/끄기

```cpp
LRESULT CALLBACK ZApp::MsgProc(HWND hWnd, UINT uMsg, 
                               WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    switch (uMsg)
    {
    case WM_KEYUP:
        if (wParam == VK_SPACE && m_pGraphics)
        {
            if (m_pGraphics->IsImguiEnabled())
                m_pGraphics->DisableImgui();
            else
                m_pGraphics->EnableImgui();
        }
        break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
```

---

## SpeedFactor 작동 원리 (1/2)

### 수학적 계산

```
정상 속도 (speedFactor = 1.0):
dt = 0.016s (약 60 FPS)

느린 속도 (speedFactor = 0.5):
dt = 0.016s × 0.5 = 0.008s
→ 시뮬레이션이 실제 시간의 절반 속도

빠른 속도 (speedFactor = 2.0):
dt = 0.016s × 2.0 = 0.032s
→ 시뮬레이션이 실제 시간의 2배 속도

정지 (speedFactor = 0.0):
dt = 0.016s × 0.0 = 0.0s
→ 일시정지 효과
```

---

## SpeedFactor 작동 원리 (2/2)

### 사용 예제

```cpp
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

**모든 시간 기반 계산이 speedFactor의 영향을 받음**

---

## UI 위젯 설명

### SliderFloat

```cpp
ImGui::SliderFloat("Speed Factor", &m_speedFactor, 0.0f, 4.0f);
```

- **첫 번째 인자**: 레이블 텍스트
- **두 번째 인자**: 제어할 변수의 포인터
- **세 번째/네 번째**: 최소값, 최대값

### TextColored - 조건부 색상 피드백

#### 예제 1: SpeedFactor에 따른 색상 변경
```cpp
if (m_speedFactor < 0.1f)
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "거의 정지");
else if (m_speedFactor < 0.5f)
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "느림");
else if (m_speedFactor < 1.5f)
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "보통");
else if (m_speedFactor < 3.0f)
    ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "빠름");
else
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "매우 빠름!");
```

#### 예제 2: FPS에 따른 색상 변경
```cpp
float fps = ImGui::GetIO().Framerate;
ImVec4 fpsColor;
if (fps >= 60.0f)
    fpsColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);  // 녹색: 좋음
else if (fps >= 30.0f)
    fpsColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);  // 노랑: 보통
else
    fpsColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);  // 빨강: 나쁨

ImGui::TextColored(fpsColor, "FPS: %.1f", fps);
```

#### 예제 3: 메모리 사용량 경고
```cpp
// Windows에서 메모리 사용량 얻기
#include <Psapi.h>  // GetProcessMemoryInfo용

PROCESS_MEMORY_COUNTERS pmc;
GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
size_t memoryUsage = pmc.WorkingSetSize;  // 바이트 단위

// MB로 변환 및 색상 결정
float memoryUsageMB = memoryUsage / 1024.0f / 1024.0f;
ImVec4 memColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);  // 기본: 회색

if (memoryUsageMB > 500.0f)
    memColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);      // 빨강: 위험
else if (memoryUsageMB > 300.0f)
    memColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);      // 주황: 경고

ImGui::TextColored(memColor, "Memory: %.2f MB", memoryUsageMB);
```

---

## 한글 폰트 지원 (1/3)

### 목적
ImGui에서 한글을 올바르게 표시하기 위해 한글 폰트 로드

### ImguiManager.cpp 생성자에서 폰트 추가

```cpp
ImGuiManager::ImGuiManager(HWND hWnd)
{
    // ... DirectX 초기화 ...
    
    // ImGui 초기화
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // 한글 폰트 추가
    io.Fonts->AddFontFromFileTTF(
        "C:\\Windows\\Fonts\\malgun.ttf",    // 맑은 고딕
        24.0f,                                // 폰트 크기
        NULL,
        io.Fonts->GetGlyphRangesKorean()     // 한글 문자 범위
    );
```

---

## 한글 폰트 지원 (2/3)

### 여러 폰트 추가 예제

```cpp
ImGuiIO& io = ImGui::GetIO();

// 맑은 고딕 - 24pt
ImFont* font1 = io.Fonts->AddFontFromFileTTF(
    "C:\\Windows\\Fonts\\malgun.ttf", 24.0f, NULL, 
    io.Fonts->GetGlyphRangesKorean()
);

// 맑은 고딕 볼드 - 18pt
ImFont* font2 = io.Fonts->AddFontFromFileTTF(
    "C:\\Windows\\Fonts\\malgunbd.ttf", 18.0f, NULL, 
    io.Fonts->GetGlyphRangesKorean()
);

// 나눔고딕 - 20pt
ImFont* font3 = io.Fonts->AddFontFromFileTTF(
    "C:\\Windows\\Fonts\\NanumGothic.ttf", 20.0f, NULL, 
    io.Fonts->GetGlyphRangesKorean()
);
```

---

## 한글 폰트 지원 (3/3)

### Windows 기본 한글 폰트

| 폰트 파일 | 이름 | 경로 |
|-----------|------|------|
| `malgun.ttf` | 맑은 고딕 | `C:\Windows\Fonts\malgun.ttf` |
| `malgunbd.ttf` | 맑은 고딕 볼드 | `C:\Windows\Fonts\malgunbd.ttf` |
| `gulim.ttc` | 굴림 | `C:\Windows\Fonts\gulim.ttc` |
| `batang.ttc` | 바탕 | `C:\Windows\Fonts\batang.ttc` |
| `NanumGothic.ttf` | 나눔고딕 | `C:\Windows\Fonts\NanumGothic.ttf` |


---

## 주의사항 (1/3)

### 폰트 파일 경로

```cpp
// 폰트 파일이 없으면 크래시 발생
io.Fonts->AddFontFromFileTTF("없는파일.ttf", 24.0f, NULL, 
                              io.Fonts->GetGlyphRangesKorean());

// 폰트 로드 실패 체크
ImFont* font = io.Fonts->AddFontFromFileTTF(
    "C:\\Windows\\Fonts\\malgun.ttf", 24.0f, NULL, 
    io.Fonts->GetGlyphRangesKorean()
);
if (font == NULL)
{
    io.Fonts->AddFontDefault();  // 기본 폰트 사용
}
```

---

## 주의사항 (2/3)

### SpeedFactor 범위 검증

```cpp
// 너무 큰 값은 시뮬레이션 불안정 초래
if (m_speedFactor > 10.0f)
    m_speedFactor = 10.0f;

// 음수 값 방지
if (m_speedFactor < 0.0f)
    m_speedFactor = 0.0f;
```

**권장 범위**: 0.0 ~ 4.0
- 너무 큰 값은 물리 시뮬레이션 오류 발생 가능
- 음수는 시간 역행으로 예상치 못한 동작

---

## 주의사항 (3/3)

### ImGui 상태 확인

```cpp
// ImGui 비활성화 시 UI 그리기 스킵
if (m_pGraphics->IsImguiEnabled())
{
    DrawImGuiUI();
}
```

**이유**:
- 불필요한 ImGui 호출 방지
- 성능 최적화
- 릴리즈 빌드에서 UI 숨김 가능

---

## 확장 아이디어 (1/3)

### 일시정지 및 속도 프리셋 버튼

```cpp
// 일시정지 버튼
static bool isPaused = false;
if (ImGui::Button(isPaused ? "Resume" : "Pause"))
{
    isPaused = !isPaused;
    m_speedFactor = isPaused ? 0.0f : 1.0f;
}

// 속도 프리셋 버튼
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

---

## 확장 아이디어 (2/3)

### 프레임 타임 그래프

```cpp
// 최근 90프레임의 프레임 타임을 그래프로 표시
static float values[90] = {};
static int values_offset = 0;

values[values_offset] = 1000.0f / ImGui::GetIO().Framerate;
values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);

ImGui::PlotLines("Frame Times (ms)", 
                 values, 
                 IM_ARRAYSIZE(values), 
                 values_offset, 
                 NULL, 
                 0.0f,      // 최소값
                 33.0f,     // 최대값 (30 FPS)
                 ImVec2(0, 80));
```

---

## 실용적인 활용 사례
```
게임 개발
- 슬로우 모션 전투 장면 테스트
- 빠른 재생으로 긴 애니메이션 확인
- 물리 시뮬레이션 디버깅

시뮬레이션
- 시간 가속으로 장기 시뮬레이션 관찰
- 느린 재생으로 세밀한 동작 분석
- 일시정지하여 특정 순간 관찰

성능 최적화
- 다양한 속도에서 FPS 안정성 확인
- 프레임 드롭 발생 지점 파악
- 메모리 사용량 모니터링
```
---

## 참고 자료

### 공식 문서 및 예제

- [ImGui GitHub](https://github.com/ocornut/imgui)
  전체 소스 코드 및 문서

- [ImGui DEMO](https://pthom.github.io/imgui_manual_online/manual/imgui_manual.html)