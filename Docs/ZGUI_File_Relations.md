# ZGUI 파일 관계 분석

## 1. 파일 목록 및 역할

### 코어
- **ZGUI.h**: 통합 헤더 (모든 ZGUI 컴포넌트 포함)

### 관리자
- **ZGUIManager** (.h/.cpp): GUI 시스템 최상위 관리자
- **ZGUIResource** (.h/.cpp): 리소스 중앙 관리 (폰트, 텍스처, 다이얼로그)

### 컨테이너
- **ZGUIDialog** (.h/.cpp): 컨트롤 컨테이너, 다이얼로그

### 컨트롤
- **ZGUIControl** (.h/.cpp): 컨트롤 베이스 클래스
- **ZGUILabel** (.h/.cpp): 텍스트 레이블
- **ZGUIImage** (.h/.cpp): 이미지
- **ZGUIButton** (.h/.cpp): 버튼

### 리소스
- **ZFont** (.h/.cpp): 폰트 렌더링
- **ZInitFile** (.h/.cpp): UTF-8 INI 파일 파서

---

## 2. 의존성 그래프

```
Application
    │
    ▼
ZGUIManager
    ├─> ZInitFile (5개 파일)
    ├─> ZGUIResource
    │     ├─> ZFont
    │     ├─> ZTexture
    │     └─> ZGUIDialog (여러 개)
    │           ├─> SpriteBatch
    │           └─> ZGUIControl (여러 개)
    │                 ├─> ZGUILabel
    │                 ├─> ZGUIImage
    │                 └─> ZGUIButton
    │
    └─> ZGraphics
```

---

## 3. 포함 관계

### ZGUI.h
```cpp
#include "ZGUIManager.h"
#include "ZGUIDialog.h"
#include "ZGUIControl.h"
#include "ZGUILabel.h"
#include "ZGUIImage.h"
#include "ZGUIButton.h"
#include "ZGUIResource.h"
#include "ZFont.h"
#include "ZInitFile.h"
```

### 각 .cpp 파일
- **ZGUIManager.cpp**: ZGUI.h, ZGraphics.h
- **ZGUIDialog.cpp**: ZGUI.h, SpriteBatch.h
- **ZGUIControl.cpp/Label/Image/Button.cpp**: ZGUI.h
- **ZFont.cpp**: ZGUI.h, SpriteFont.h, SpriteBatch.h
- **ZInitFile.cpp**: fstream, sstream

---

## 4. 데이터 흐름

### 초기화
```
BasicRenderState::Enter
  → ZGUIManager::Init
    → DefaultRes.ift 로드
    → FontRes.ift → AddFont
    → TextureRes.ift → AddTexture
    → DialogRes.ift → ZGUIDialog 생성
    → ControlRes.ift → 컨트롤 추가
```

### 렌더링
```
BasicRenderState::Render
  → ZGUIManager::Render
    → ZGUIDialog::Render (각 다이얼로그)
      → SpriteBatch::Begin
      → ZGUIControl::Render (각 컨트롤)
        → DrawSprite (SpriteBatch::Draw)
        → DrawText (ZFont::PrintEx)
      → SpriteBatch::End
```

### 입력 처리
```
WndProc(WM_LBUTTONDOWN)
  → BasicRenderState::OnMouseDown
    → ZGUIManager::MsgProc
      → ZGUIDialog::MsgProc (각 다이얼로그)
        → ZGUIControl::HandleMouse
```

---

## 5. 주요 클래스 관계

### ZGUIManager
- **소유**: ZGUIResource, ZInitFile (5개)
- **관리**: ZGUIDialog (여러 개)
- **역할**: 시스템 초기화, 업데이트, 렌더링, 메시지 분배

### ZGUIResource
- **소유**: ZFont (여러 개), ZTexture (여러 개), ZGUIDialog (여러 개)
- **역할**: 리소스 인덱스 관리, 검색

### ZGUIDialog
- **소유**: ZGUIControl (여러 개), SpriteBatch
- **참조**: ZGUIResource
- **역할**: 컨트롤 관리, 입력 처리, 렌더링, 드래그

### ZGUIControl (베이스)
- **참조**: ZGUIDialog (부모)
- **파생**: ZGUILabel, ZGUIImage, ZGUIButton
- **역할**: 공통 속성 및 메서드

### ZFont
- **소유**: DirectX::SpriteFont, DirectX::SpriteBatch
- **역할**: UTF-8 텍스트 렌더링, 크기 조절

### ZInitFile
- **역할**: UTF-8 INI 파일 파싱, 캐시 관리

---

## 6. 확장 가이드

### 새 컨트롤 추가
1. ZGUIControl 상속
2. OnInit, Render, HandleMouse 구현
3. ZGUIDialog에 AddXXX 메서드 추가
4. ControlRes.ift에 Type 정의

### 새 리소스 추가
1. ZGUIResource에 리스트 추가
2. AddXXX, GetXXX 메서드 추가
3. XXXRes.ift 파일 생성
4. ZGUIManager::Init에서 로드

---

**작성일**: 2025-11-01  
**버전**: 1.0
