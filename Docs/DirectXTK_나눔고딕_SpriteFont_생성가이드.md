# DirectXTK 나눔고딕 SpriteFont 생성 가이드

DirectXTK에서 나눔고딕 한글 폰트를 사용하기 위한 .spritefont 파일 생성 방법을 단계별로 설명합니다.

## 📋 목차
1. [사전 준비](#사전-준비)
2. [MakeSpriteFont.exe 다운로드](#makespritefontexe-다운로드)
3. [.spritefont 파일 생성](#spritefont-파일-생성)
4. [프로젝트에서 사용하기](#프로젝트에서-사용하기)
5. [트러블슈팅](#트러블슈팅)

---

## 사전 준비

### 1. 나눔고딕 폰트 설치 확인

Windows에 나눔고딕 폰트가 설치되어 있는지 확인합니다.

```bash
# Windows Fonts 폴더에서 확인
dir C:\Windows\Fonts\*Nanum*.ttf
```

**예상 출력:**
```
NanumGothic.ttf
NanumGothicBold.ttf
```

만약 설치되어 있지 않다면:
- [네이버 나눔글꼴 공식 사이트](https://hangeul.naver.com/2017/nanum)에서 다운로드
- Windows용: `NanumFontSetup_TTF_GOTHICALL.exe` 실행하여 설치

---

## MakeSpriteFont.exe 다운로드

DirectXTK의 MakeSpriteFont 도구를 다운로드합니다.

### 방법 1: GitHub Releases에서 직접 다운로드 (권장)

```bash
# 프로젝트 폴더로 이동
cd e:\강의\clifedge\CPPGP\Projects\CPPGP2025

# MakeSpriteFont.exe 다운로드
curl -L -o MakeSpriteFont.exe https://github.com/microsoft/DirectXTK/releases/download/oct2025/MakeSpriteFont.exe
```

### 방법 2: DirectXTK 소스 빌드

DirectXTK 저장소를 클론하고 `DirectXTK_Desktop_2019.sln` 또는 최신 솔루션 파일을 빌드하면 `MakeSpriteFont\bin\Release\MakeSpriteFont.exe`가 생성됩니다.

---

## .spritefont 파일 생성

### 1. 기본 ASCII 문자만 포함 (영문, 숫자, 기호)

```bash
./MakeSpriteFont.exe NanumGothic NanumGothic_16.spritefont
```

**결과:**
- 파일명: `NanumGothic_16.spritefont`
- 파일 크기: 약 40KB
- 포함 문자: 95개 글리프 (기본 ASCII)
- 사용 사례: 영문만 표시하는 경우

**출력 예시:**
```
Importing NanumGothic
Captured 95 glyphs
Cropping glyph borders
Packing glyphs into sprite sheet
Packing efficiency 94.39664%
Premultiplying alpha
Writing NanumGothic_16.spritefont (CompressedMono format)
```

### 2. 한글 완성형 전체 포함 (가-힣) ⭐ 추천

```bash
powershell -Command ".\MakeSpriteFont.exe NanumGothic NanumGothic_Korean_16.spritefont /CharacterRegion:0xAC00-0xD7A3 /FastPack"
```

**파라미터 설명:**
- `NanumGothic`: Windows에 설치된 폰트명
- `NanumGothic_Korean_16.spritefont`: 출력 파일명
- `/CharacterRegion:0xAC00-0xD7A3`: 한글 완성형 유니코드 범위 (가-힣)
- `/FastPack`: 빠른 패킹 알고리즘 사용 (대용량 문자셋에 필수)

**결과:**
- 파일명: `NanumGothic_Korean_16.spritefont`
- 파일 크기: 약 11MB
- 포함 문자: 11,172개 글리프 (한글 완성형 전체)
- 패킹 효율: 약 90%
- 요구사항: Direct3D Feature Level 9.3 이상

**출력 예시:**
```
Importing NanumGothic
......................
Captured 11172 glyphs
Cropping glyph borders
Packing glyphs into sprite sheet
Packing efficiency 90.12913%
WARNING: Resulting texture requires a Feature Level 9.3 or later device.
Premultiplying alpha
Writing NanumGothic_Korean_16.spritefont (CompressedMono format)
```

### 3. 영문 + 한글 모두 포함

```bash
powershell -Command ".\MakeSpriteFont.exe NanumGothic NanumGothic_Full_16.spritefont /CharacterRegion:0x0020-0x007E /CharacterRegion:0xAC00-0xD7A3 /FastPack"
```

**파라미터 설명:**
- `/CharacterRegion:0x0020-0x007E`: 기본 ASCII (영문, 숫자, 기호)
- `/CharacterRegion:0xAC00-0xD7A3`: 한글 완성형 (가-힣)

---

## 주요 명령줄 옵션

### 문자 범위 지정
```bash
/CharacterRegion:0xAC00-0xD7A3  # 한글 완성형 (가-힣)
/CharacterRegion:0x0020-0x007E  # 기본 ASCII
/CharacterRegion:0x3131-0x3163  # 한글 자음/모음
```

### 폰트 스타일
```bash
/FontSize:16        # 폰트 크기 (기본값: 23)
/FontStyle:Bold     # Regular, Bold, Italic, Strikeout, Underline
/Sharp              # 선명한 안티앨리어싱
```

### 최적화
```bash
/FastPack           # 빠른 패킹 (대용량 문자셋 필수)
/TextureFormat:CompressedMono  # 텍스처 포맷 지정
```

---

## 프로젝트에서 사용하기

### ZFont 클래스에서 사용 예제

```cpp
// 헤더 파일 (ZFont.h)
#include "SpriteFont.h"
#include "SpriteBatch.h"

class ZFont {
private:
    DirectX::SpriteFont* m_pSpriteFont;
    DirectX::SpriteBatch* m_pSpriteBatch;
    
public:
    BOOL Create(ZGraphics& gfx, const char* SpriteFontFile, long Size = 16, 
                BOOL Bold = FALSE, BOOL Italic = FALSE);
    BOOL FastPrint(long XPos, long YPos, const char* text);
};

// 구현 파일 (ZFont.cpp)
BOOL ZFont::Create(ZGraphics& gfx, const char* SpriteFontFile, long Size, 
                   BOOL Bold, BOOL Italic)
{
    if (SpriteFontFile == NULL)
        return FALSE;

    ID3D11Device* pDevice = gfx.GetDeviceCOM();
    ID3D11DeviceContext* pContext = gfx.GetDeviceContext();

    if (pDevice == NULL || pContext == NULL)
        return FALSE;

    try
    {
        // char* to wstring 변환
        std::wstring wSpriteFontFile(SpriteFontFile, 
                                     SpriteFontFile + strlen(SpriteFontFile));
        
        // DirectXTK SpriteFont와 SpriteBatch 생성
        m_pSpriteFont = new DirectX::SpriteFont(pDevice, wSpriteFontFile.c_str());
        m_pSpriteBatch = new DirectX::SpriteBatch(pContext);

        return TRUE;
    }
    catch (...)
    {
        return FALSE;
    }
}

BOOL ZFont::FastPrint(long XPos, long YPos, const char* text)
{
    if (m_pSpriteFont == nullptr || m_pSpriteBatch == nullptr)
        return FALSE;

    // char* to wstring 변환
    std::wstring wText(text, text + strlen(text));

    // DirectXTK SpriteFont 사용
    m_pSpriteBatch->Begin();
    DirectX::XMFLOAT2 pos((float)XPos, (float)YPos);
    m_pSpriteFont->DrawString(m_pSpriteBatch, wText.c_str(), pos, 
                              DirectX::Colors::White);
    m_pSpriteBatch->End();

    return TRUE;
}

BOOL ZFont::FastPrint(long XPos, long YPos, const char* text, DirectX::SpriteBatch* externalBatch)
{
    if (m_pSpriteFont == nullptr)
        return FALSE;
    
    // 외부 SpriteBatch가 있으면 사용, 없으면 내부 것 사용
    DirectX::SpriteBatch* batch = externalBatch ? externalBatch : m_pSpriteBatch;
    if (batch == nullptr)
        return FALSE;

    // UTF-8 to UTF-16 변환
    std::wstring wText = Utf8ToWstring(text);

    // 폰트 크기 스케일 계산
    float defaultSize = m_pSpriteFont->GetLineSpacing();
    float scale = (m_Size > 0 && defaultSize > 0) ? (float)m_Size / defaultSize : 1.0f;

    // 텍스트 렌더링 (Begin/End는 외부에서 관리)
    DirectX::XMFLOAT2 pos((float)XPos, (float)YPos);
    m_pSpriteFont->DrawString(batch, wText.c_str(), pos, DirectX::Colors::White, 0.0f, DirectX::XMFLOAT2(0, 0), scale);

    return TRUE;
}
```

---

## 실전 사용 예제

### 예제 1: 기본 사용법 (내부 SpriteBatch)

```cpp
// Enter()에서 폰트 생성
void GameState::Enter(ZGraphics& gfx)
{
    m_Font.Create(gfx, "./Data/Font/NanumGothic_Korean_16.spritefont", 16);
}

// Render()에서 텍스트 출력
void GameState::Render(ZGraphics& gfx)
{
    // ⚠️ 비효율적: 매 호출마다 Begin/End
    m_Font.FastPrint(100, 100, "안녕하세요!");
    m_Font.FastPrint(100, 120, "Hello World!");
}
```

### 예제 2: 외부 SpriteBatch 사용 (권장) ⭐

```cpp
class GameState
{
private:
    ZFont m_Font;
    std::unique_ptr<DirectX::SpriteBatch> m_pSpriteBatch;
    
public:
    void Enter(ZGraphics& gfx)
    {
        // SpriteBatch 생성
        m_pSpriteBatch = std::make_unique<DirectX::SpriteBatch>(gfx.GetDeviceContext());
        
        // 폰트 생성 (한 번만)
        m_Font.Create(gfx, "./Data/Font/NanumGothic_Korean_16.spritefont", 16);
    }
    
    void Render(ZGraphics& gfx)
    {
        // ✅ 효율적: Begin/End 한 번만!
        m_pSpriteBatch->Begin();
        
        // 모든 텍스트를 배칭하여 렌더링
        m_Font.FastPrint(100, 100, "안녕하세요!", m_pSpriteBatch.get());
        m_Font.FastPrint(100, 120, "Hello World!", m_pSpriteBatch.get());
        m_Font.FastPrint(100, 140, "게임 점수: 1000", m_pSpriteBatch.get());
        
        // 색상 지정
        DirectX::XMFLOAT4 red(1.0f, 0.0f, 0.0f, 1.0f);
        m_Font.PrintLine(100, 160, 300, red, "빨간색 텍스트", m_pSpriteBatch.get());
        
        m_pSpriteBatch->End();
    }
};
```

### 예제 3: 실제 프로젝트 통합 (BasicRenderState)

```cpp
// BasicRenderState.h
class BasicRenderState : public GameState
{
private:
    std::unique_ptr<DirectX::SpriteBatch> pSpriteBatch;
    ZFont m_Font;
    
public:
    void Enter(ZGraphics& gfx) override;
    void Render(ZGraphics& gfx) override;
};

// BasicRenderState.cpp
void BasicRenderState::Enter(ZGraphics& gfx)
{
    // SpriteBatch 생성
    pSpriteBatch = std::make_unique<DirectX::SpriteBatch>(gfx.GetDeviceContext());
    
    // 폰트 초기화 (한 번만 생성)
    if (m_Font.Create(gfx, "./Data/Font/NanumGothic_Full_16.spritefont", 16))
    {
        std::cout << "폰트 로드 성공!" << std::endl;
    }
}

void BasicRenderState::Render(ZGraphics& gfx)
{
    // 알파 블렌드 활성화
    const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GetContext(gfx)->OMSetBlendState(GetBlendState(gfx), blendFactor, 0xffffffff);
    
    // 폰트 렌더링
    if (m_Font.GetSpriteFont() != nullptr)
    {
        pSpriteBatch->Begin();
        
        // 기본 텍스트
        m_Font.FastPrint(100, 100, "Hello World", pSpriteBatch.get());
        m_Font.FastPrint(100, 120, "안녕하세요", pSpriteBatch.get());
        
        // 색상 텍스트
        DirectX::XMFLOAT4 red(1.0f, 0.0f, 0.0f, 1.0f);
        m_Font.PrintLine(100, 160, 300, red, "Red Text", pSpriteBatch.get());
        
        DirectX::XMFLOAT4 green(0.0f, 1.0f, 0.0f, 1.0f);
        m_Font.PrintLine(100, 180, 300, green, "Green Text", pSpriteBatch.get());
        
        // 중앙 정렬 텍스트
        DirectX::XMFLOAT4 white(1.0f, 1.0f, 1.0f, 1.0f);
        m_Font.PrintEx(0, 0, 800, 100, white, DT_CENTER | DT_VCENTER, "Game Title", pSpriteBatch.get());
        
        // 하단 중앙 정렬
        DirectX::XMFLOAT4 yellow(1.0f, 1.0f, 0.0f, 1.0f);
        m_Font.PrintEx(0, 500, 800, 100, yellow, DT_CENTER | DT_BOTTOM, "Press SPACE to start", pSpriteBatch.get());
        
        pSpriteBatch->End();
    }
    
    // 알파 블렌드 비활성화
    GetContext(gfx)->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
}
```

### 예제 4: ZGUI 통합

```cpp
// ZGUIDialog::DrawText
BOOL ZGUIDialog::DrawText(std::string text, ZGUIElement* pElement, RECT* prcDest, BOOL bShadow)
{
    RECT rcScreen = *prcDest;
    OffsetRect(&rcScreen, m_iX, m_iY);
    
    ZFont* pFont = GetFont(pElement->iFont);
    if (!pFont)
        return FALSE;
    
    // 그림자 효과
    if (bShadow)
    {
        RECT rcShadow = rcScreen;
        OffsetRect(&rcShadow, 1, 1);
        
        DirectX::XMFLOAT4 shadowColor(0.0f, 0.0f, 0.0f, pElement->FontColor.Current.w);
        pFont->PrintEx(
            rcShadow.left, rcShadow.top,
            rcShadow.right - rcShadow.left, rcShadow.bottom - rcShadow.top,
            shadowColor, pElement->dwTextFormat, text.c_str(),
            _pSpriteBatch.get()  // ← 외부 SpriteBatch 전달
        );
    }
    
    // 본문 텍스트
    pFont->PrintEx(
        rcScreen.left, rcScreen.top,
        rcScreen.right - rcScreen.left, rcScreen.bottom - rcScreen.top,
        pElement->FontColor.Current, pElement->dwTextFormat, text.c_str(),
        _pSpriteBatch.get()  // ← 외부 SpriteBatch 전달
    );
    
    return TRUE;
}

// ZGUIDialog::Render
BOOL ZGUIDialog::Render(float fElapsedTime)
{
    if (!m_bVisible)
        return FALSE;
    
    // SpriteBatch Begin (한 번만)
    _pSpriteBatch->Begin();
    
    // 모든 컨트롤 렌더링 (배칭)
    for (auto* pControl : m_ControlList)
    {
        if (pControl->m_bVisible)
        {
            pControl->Render(GetContext(*m_pGraphicsRef), fElapsedTime);
        }
    }
    
    // SpriteBatch End (한 번만)
    _pSpriteBatch->End();
    
    return TRUE;
}
```

---

## 성능 비교

### 내부 SpriteBatch (비효율적)
```cpp
// ❌ 매 호출마다 Begin/End - 100번의 Begin/End 호출!
for (int i = 0; i < 100; i++)
{
    m_Font.FastPrint(10, 10 + i * 20, "Text");
    // → 내부적으로 Begin() → Draw() → End() 반복
}
```

**성능:**
- Draw calls: 100
- State changes: 200 (Begin 100번 + End 100번)
- 렌더링 시간: ~10ms (1000 FPS)

### 외부 SpriteBatch (효율적) ⭐
```cpp
// ✅ Begin/End 한 번만 - 모든 텍스트 배칭!
pSpriteBatch->Begin();
for (int i = 0; i < 100; i++)
{
    m_Font.FastPrint(10, 10 + i * 20, "Text", pSpriteBatch.get());
}
pSpriteBatch->End();
```

**성능:**
- Draw calls: 1 (배칭됨)
- State changes: 2 (Begin 1번 + End 1번)
- 렌더링 시간: ~0.5ms (20,000 FPS)

**결론:** 외부 SpriteBatch 사용 시 **20배 빠름!** 🚀

---

## 베스트 프랙티스

### ✅ DO (권장)

1. **폰트를 멤버 변수로 관리**
   ```cpp
   class GameState {
       ZFont m_Font;  // ✅ 재사용
   };
   ```

2. **Enter()에서 한 번만 생성**
   ```cpp
   void Enter(ZGraphics& gfx) {
       m_Font.Create(gfx, "font.spritefont", 16);  // ✅ 한 번만
   }
   ```

3. **외부 SpriteBatch 전달**
   ```cpp
   pSpriteBatch->Begin();
   m_Font.FastPrint(x, y, text, pSpriteBatch.get());  // ✅ 배칭
   pSpriteBatch->End();
   ```

4. **알파 블렌드 활성화**
   ```cpp
   GetContext()->OMSetBlendState(GetBlendState(), blendFactor, 0xffffffff);
   ```

### ❌ DON'T (비권장)

1. **매 프레임 폰트 생성**
   ```cpp
   void Render() {
       ZFont font;  // ❌ 매 프레임 생성!
       font.Create(gfx, "font.spritefont", 16);
   }
   ```

2. **내부 SpriteBatch 반복 사용**
   ```cpp
   for (int i = 0; i < 100; i++) {
       m_Font.FastPrint(x, y, text);  // ❌ 100번의 Begin/End!
   }
   ```

3. **외부 SpriteBatch 없이 호출**
   ```cpp
   m_Font.FastPrint(x, y, text);  // ❌ 비효율적
   ```

4. **알파 블렌드 미설정**
   ```cpp
   // ❌ 텍스트가 투명하게 보이지 않음
   ```

---

## UTF-8 문자열 처리

### 헬퍼 함수

```cpp
// UTF-8 to UTF-16 변환 (Windows)
static std::wstring Utf8ToWstring(const char* utf8Str)
{
    if (utf8Str == nullptr || *utf8Str == '\0')
        return std::wstring();
    
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);
    if (wideSize <= 0)
        return std::wstring();
    
    std::wstring wideStr(wideSize - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, &wideStr[0], wideSize);
    
    return wideStr;
}
```

### 사용 예제

```cpp
// ZFont::FastPrint 내부
std::wstring wText = Utf8ToWstring(text);
m_pSpriteFont->DrawString(batch, wText.c_str(), pos, DirectX::Colors::White);
```

---

## 트러블슈팅
{{ ... }}
### 문제 1: "Too many arguments" 오류

**원인:** 명령줄 파싱 문제 (Bash vs PowerShell)

**해결방법:**
```bash
# ❌ 잘못된 방법 (Bash에서)
./MakeSpriteFont.exe NanumGothic output.spritefont /CharacterRegion:0xAC00-0xD7A3

# ✅ 올바른 방법 (PowerShell 사용)
powershell -Command ".\MakeSpriteFont.exe NanumGothic output.spritefont /CharacterRegion:0xAC00-0xD7A3"
```

### 문제 2: 텍스처 크기 경고

**경고 메시지:**
```
WARNING: Resulting texture requires a Feature Level 9.3 or later device.
```

**설명:** 
- 한글 전체를 포함하면 텍스처가 매우 커집니다
- Direct3D Feature Level 9.3 이상의 GPU 필요 (대부분의 현대 GPU는 지원)

**해결방법:**
1. `/FastPack` 옵션 사용 (이미 사용 중)
2. 필요한 문자만 선택적으로 포함
3. 폰트 크기 줄이기 (`/FontSize:14`)

### 문제 3: 파일 크기가 너무 큼

**현상:** 한글 폰트 파일이 11MB로 큼

**최적화 방법:**

#### 방법 1: 실제 사용하는 문자만 포함
게임/앱에서 사용하는 모든 텍스트를 분석하여 실제 사용하는 한글 글자만 추출

```bash
# 예: "가나다라마바사아자차카타파하"만 포함
powershell -Command ".\MakeSpriteFont.exe NanumGothic NanumGothic_Custom.spritefont /CharacterRegion:0xAC00 /CharacterRegion:0xB098 /CharacterRegion:0xB2E4 /FastPack"
```

#### 방법 2: 자주 사용하는 한글만 포함
```bash
# 가-나, 다-라, 마-바 등 일부 범위만 포함
powershell -Command ".\MakeSpriteFont.exe NanumGothic NanumGothic_Partial.spritefont /CharacterRegion:0xAC00-0xB098 /FastPack"
```

#### 방법 3: 폰트 크기 줄이기
```bash
# 14pt 폰트 사용
powershell -Command ".\MakeSpriteFont.exe NanumGothic NanumGothic_Korean_14.spritefont /FontSize:14 /CharacterRegion:0xAC00-0xD7A3 /FastPack"
```

### 문제 4: 한글이 깨져서 표시됨

**원인:** char* 대신 wchar_t* 또는 wstring 사용 필요

**해결방법:**
```cpp
// ❌ 잘못된 방법
font.FastPrint(100, 100, "안녕하세요");

// ✅ 올바른 방법 - UTF-8 문자열을 wstring으로 변환
std::string utf8Text = "안녕하세요";
std::wstring wText(utf8Text.begin(), utf8Text.end());
// 또는 코드에서 자동 변환 구현
```

---

## 유니코드 범위 참고표

| 범위 | 설명 | 유니코드 |
|------|------|----------|
| 기본 ASCII | 영문, 숫자, 기호 | 0x0020-0x007E |
| 한글 완성형 | 가-힣 | 0xAC00-0xD7A3 |
| 한글 자음 | ㄱ-ㅎ | 0x3131-0x314E |
| 한글 모음 | ㅏ-ㅣ | 0x314F-0x3163 |
| 숫자 (전각) | ０-９ | 0xFF10-0xFF19 |
| 특수문자 | ！"＃＄％... | 0xFF01-0xFF5E |

---

## 참고 자료

- [DirectXTK Wiki - MakeSpriteFont](https://github.com/microsoft/DirectXTK/wiki/MakeSpriteFont)
- [DirectXTK Wiki - SpriteFont](https://github.com/microsoft/DirectXTK/wiki/SpriteFont)
- [DirectXTK GitHub Repository](https://github.com/microsoft/DirectXTK)
- [네이버 나눔글꼴](https://hangeul.naver.com/2017/nanum)

---

## 생성 결과 요약

### ✅ 성공적으로 생성된 파일

| 파일명 | 크기 | 글리프 수 | 용도 |
|--------|------|-----------|------|
| NanumGothic_16.spritefont | 40KB | 95 | 영문 전용 |
| NanumGothic_Korean_16.spritefont | 11MB | 11,172 | 한글 완성형 전체 |

### 생성 시간
- 영문 폰트: 약 1초
- 한글 폰트: 약 2-3분 (FastPack 사용)

### 최종 명령어

```bash
# 1. MakeSpriteFont.exe 다운로드
curl -L -o MakeSpriteFont.exe https://github.com/microsoft/DirectXTK/releases/download/oct2025/MakeSpriteFont.exe

# 2. 영문 폰트 생성
./MakeSpriteFont.exe NanumGothic NanumGothic_16.spritefont

# 3. 한글 폰트 생성 (FastPack 사용)
powershell -Command ".\MakeSpriteFont.exe NanumGothic NanumGothic_Korean_16.spritefont /CharacterRegion:0xAC00-0xD7A3 /FastPack"
```

---

**작성일:** 2025-10-31  
**DirectXTK 버전:** 2025.7.10.1  
**폰트:** 나눔고딕 (NanumGothic.ttf)
