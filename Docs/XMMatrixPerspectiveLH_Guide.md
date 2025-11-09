# DirectX::XMMatrixPerspectiveLH 함수 가이드

## 개요

`XMMatrixPerspectiveLH`는 DirectX Math 라이브러리에서 제공하는 함수로, **좌표계(Left-Handed) 기준의 원근 투영 행렬(Perspective Projection Matrix)**을 생성합니다.

## 함수 시그니처

```cpp
DirectX::XMMATRIX XMMatrixPerspectiveLH(
    float ViewWidth,    // 뷰 볼륨의 너비
    float ViewHeight,   // 뷰 볼륨의 높이
    float NearZ,        // Near 클리핑 평면까지의 거리
    float FarZ          // Far 클리핑 평면까지의 거리
) noexcept;
```

---

## 매개변수 설명

### 1. `ViewWidth` (float)
- **의미**: Near 클리핑 평면에서의 뷰 볼륨 너비
- **단위**: 월드 공간 단위
- **특징**: 화면의 가로 크기와 관련

### 2. `ViewHeight` (float)
- **의미**: Near 클리핑 평면에서의 뷰 볼륨 높이
- **단위**: 월드 공간 단위
- **특징**: 화면의 세로 크기와 관련

### 3. `NearZ` (float)
- **의미**: 카메라로부터 Near 클리핑 평면까지의 거리
- **제약**: 반드시 **양수**여야 하며, FarZ보다 작아야 함
- **일반적인 값**: 0.1f ~ 1.0f
- **중요**: 너무 작으면 깊이 정밀도 문제 발생

### 4. `FarZ` (float)
- **의미**: 카메라로부터 Far 클리핑 평면까지의 거리
- **제약**: NearZ보다 커야 함
- **일반적인 값**: 100.0f ~ 10000.0f
- **중요**: NearZ와의 비율이 깊이 버퍼 정밀도에 영향

---

## 반환값

- **타입**: `DirectX::XMMATRIX`
- **내용**: 4x4 원근 투영 변환 행렬

---

## 좌표계 (Left-Handed vs Right-Handed)

### Left-Handed (LH) 좌표계
- **Z축 방향**: 화면 안쪽으로 증가 (양의 방향이 카메라 전방)
- **DirectX의 기본 좌표계**
- **사용 함수**: `XMMatrixPerspectiveLH`

### Right-Handed (RH) 좌표계
- **Z축 방향**: 화면 바깥쪽으로 증가 (음의 방향이 카메라 전방)
- **OpenGL의 기본 좌표계**
- **사용 함수**: `XMMatrixPerspectiveRH`

---

## 원근 투영 행렬이란?

원근 투영은 3D 공간의 물체를 2D 화면에 표현할 때 **원근감**을 적용하는 방식입니다.

### 특징
- **멀리 있는 물체는 작게** 보임
- **가까이 있는 물체는 크게** 보임
- 평행선이 소실점에서 만나는 것처럼 보임
- **실제 인간의 시각과 유사**

---

## 사용처

### 1. 3D 게임 및 애플리케이션
- 3D 캐릭터 렌더링
- 3D 환경 렌더링
- 일인칭/삼인칭 카메라 시스템

### 2. CAD 및 3D 모델링 도구
- 3D 모델 뷰어
- 건축 시각화
- 제품 디자인

### 3. 시뮬레이션
- 비행 시뮬레이터
- 레이싱 게임
- VR/AR 애플리케이션

---

## 사용 예제

### 예제 1: 기본 투영 행렬 생성

```cpp
#include <DirectXMath.h>
using namespace DirectX;

// 화면 크기
const float screenWidth = 1920.0f;
const float screenHeight = 1080.0f;

// 뷰 볼륨 크기 (Near 평면에서)
float viewWidth = 10.0f;   // 월드 단위
float viewHeight = 10.0f;  // 월드 단위

// 클리핑 평면 거리
float nearZ = 0.1f;   // Near 평면: 카메라로부터 0.1 유닛
float farZ = 1000.0f; // Far 평면: 카메라로부터 1000 유닛

// 투영 행렬 생성
XMMATRIX projectionMatrix = XMMatrixPerspectiveLH(
    viewWidth,
    viewHeight,
    nearZ,
    farZ
);
```

### 예제 2: FOV 기반 투영 행렬 (더 일반적인 방법)

```cpp
#include <DirectXMath.h>
using namespace DirectX;

// 화면 정보
const float screenWidth = 1920.0f;
const float screenHeight = 1080.0f;
const float aspectRatio = screenWidth / screenHeight; // 16:9 = 1.777...

// 카메라 설정
float fovAngleY = XM_PIDIV4; // 45도 (라디안)
float nearZ = 0.1f;
float farZ = 1000.0f;

// FOV 기반 투영 행렬 생성 (더 권장되는 방법)
XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(
    fovAngleY,    // 수직 시야각
    aspectRatio,  // 화면 종횡비
    nearZ,        // Near 클리핑 평면
    farZ          // Far 클리핑 평면
);
```

### 예제 3: 실제 렌더링 파이프라인에서의 사용

```cpp
#include <DirectXMath.h>
using namespace DirectX;

class Camera
{
private:
    XMMATRIX m_ViewMatrix;       // 뷰 행렬
    XMMATRIX m_ProjectionMatrix; // 투영 행렬
    
    float m_NearZ;
    float m_FarZ;
    float m_AspectRatio;
    float m_FovY;

public:
    Camera()
        : m_NearZ(0.1f)
        , m_FarZ(1000.0f)
        , m_AspectRatio(16.0f / 9.0f)
        , m_FovY(XM_PIDIV4)
    {
        UpdateProjectionMatrix();
    }

    // 투영 행렬 업데이트
    void UpdateProjectionMatrix()
    {
        // FOV 기반 투영 행렬 생성
        m_ProjectionMatrix = XMMatrixPerspectiveFovLH(
            m_FovY,
            m_AspectRatio,
            m_NearZ,
            m_FarZ
        );
    }

    // 화면 크기 변경 시 호출
    void OnResize(float width, float height)
    {
        m_AspectRatio = width / height;
        UpdateProjectionMatrix();
    }

    // 투영 행렬 가져오기
    XMMATRIX GetProjectionMatrix() const
    {
        return m_ProjectionMatrix;
    }

    // VP 행렬 (View * Projection) 계산
    XMMATRIX GetViewProjectionMatrix() const
    {
        return m_ViewMatrix * m_ProjectionMatrix;
    }
};
```

### 예제 4: 상수 버퍼에 전달

```cpp
#include <DirectXMath.h>
using namespace DirectX;

// 셰이더로 전달할 상수 버퍼 구조체
struct TransformConstants
{
    XMMATRIX World;      // 월드 변환 행렬
    XMMATRIX View;       // 뷰 변환 행렬
    XMMATRIX Projection; // 투영 변환 행렬
};

void UpdateConstantBuffer(ID3D11DeviceContext* pContext)
{
    TransformConstants constants;

    // 1. 월드 행렬 설정 (물체의 위치, 회전, 크기)
    constants.World = XMMatrixIdentity();

    // 2. 뷰 행렬 설정 (카메라 위치와 방향)
    XMVECTOR eyePos = XMVectorSet(0.0f, 5.0f, -10.0f, 1.0f);  // 카메라 위치
    XMVECTOR focusPos = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);  // 바라보는 지점
    XMVECTOR upDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);     // 위쪽 방향
    constants.View = XMMatrixLookAtLH(eyePos, focusPos, upDir);

    // 3. 투영 행렬 설정
    float fovY = XM_PIDIV4;      // 45도
    float aspectRatio = 16.0f / 9.0f;
    float nearZ = 0.1f;
    float farZ = 1000.0f;
    constants.Projection = XMMatrixPerspectiveFovLH(
        fovY,
        aspectRatio,
        nearZ,
        farZ
    );

    // 4. 행렬 전치 (HLSL은 열 우선 행렬을 사용)
    constants.World = XMMatrixTranspose(constants.World);
    constants.View = XMMatrixTranspose(constants.View);
    constants.Projection = XMMatrixTranspose(constants.Projection);

    // 5. GPU로 전송
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    pContext->Map(g_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, &constants, sizeof(TransformConstants));
    pContext->Unmap(g_pConstantBuffer, 0);
}
```

### 예제 5: 버텍스 셰이더에서 사용

```hlsl
// VertexShader.hlsl

cbuffer TransformBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
};

struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;  // 화면 좌표
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    // 월드 변환
    float4 worldPos = mul(float4(input.Position, 1.0f), World);
    
    // 뷰 변환
    float4 viewPos = mul(worldPos, View);
    
    // 투영 변환 (원근 투영 적용)
    output.Position = mul(viewPos, Projection);
    
    // 또는 한 번에:
    // output.Position = mul(mul(mul(float4(input.Position, 1.0f), World), View), Projection);
    
    // 노말 변환 (월드 공간으로)
    output.Normal = mul(input.Normal, (float3x3)World);
    
    output.TexCoord = input.TexCoord;
    
    return output;
}
```

---

## XMMatrixPerspectiveLH vs XMMatrixPerspectiveFovLH

### XMMatrixPerspectiveLH
```cpp
XMMATRIX XMMatrixPerspectiveLH(
    float ViewWidth,  // 뷰 볼륨 너비
    float ViewHeight, // 뷰 볼륨 높이
    float NearZ,
    float FarZ
);
```
- **장점**: 직접적인 뷰 볼륨 크기 제어
- **단점**: 화면 비율 변경 시 계산 필요
- **사용 케이스**: 특수한 투영이 필요한 경우

### XMMatrixPerspectiveFovLH (더 일반적)
```cpp
XMMATRIX XMMatrixPerspectiveFovLH(
    float FovAngleY,   // 수직 시야각 (라디안)
    float AspectRatio, // 화면 종횡비 (width/height)
    float NearZ,
    float FarZ
);
```
- **장점**: FOV와 종횡비로 직관적 제어
- **단점**: 없음 (일반적으로 권장)
- **사용 케이스**: 대부분의 3D 애플리케이션

---

## 주의사항

### 1. NearZ와 FarZ 비율
```cpp
// ❌ 나쁜 예: 비율이 너무 큼 (깊이 정밀도 문제)
float nearZ = 0.001f;
float farZ = 100000.0f;
// 비율 = 100,000,000 : 1

// ✅ 좋은 예: 적절한 비율
float nearZ = 0.1f;
float farZ = 1000.0f;
// 비율 = 10,000 : 1
```

### 2. Near 평면은 0이 될 수 없음
```cpp
// ❌ 잘못된 사용
float nearZ = 0.0f;  // 나눗셈 오류 발생

// ✅ 올바른 사용
float nearZ = 0.1f;  // 양수 값 사용
```

### 3. 화면 비율 변경 대응
```cpp
// 창 크기 변경 시 투영 행렬 재계산 필요
void OnWindowResize(int width, int height)
{
    float aspectRatio = (float)width / (float)height;
    
    g_ProjectionMatrix = XMMatrixPerspectiveFovLH(
        XM_PIDIV4,
        aspectRatio,  // 업데이트된 비율
        0.1f,
        1000.0f
    );
}
```

---

## 관련 함수

### 투영 행렬 생성 함수들
- `XMMatrixPerspectiveFovLH` - FOV 기반 (LH) ⭐ 가장 많이 사용
- `XMMatrixPerspectiveFovRH` - FOV 기반 (RH)
- `XMMatrixPerspectiveLH` - 크기 기반 (LH)
- `XMMatrixPerspectiveRH` - 크기 기반 (RH)
- `XMMatrixOrthographicLH` - 직교 투영 (LH)
- `XMMatrixOrthographicRH` - 직교 투영 (RH)

### 뷰 행렬 생성 함수들
- `XMMatrixLookAtLH` - 카메라 뷰 행렬 (LH)
- `XMMatrixLookAtRH` - 카메라 뷰 행렬 (RH)

---

## 참고 자료

- [Microsoft DirectXMath Documentation](https://docs.microsoft.com/en-us/windows/win32/dxmath/ovw-xnamath-reference-functions-matrix)
- [DirectX 11 Tutorial](https://docs.microsoft.com/en-us/windows/win32/direct3d11/dx-graphics-overviews)
- [3D Graphics Fundamentals](https://learnopengl.com/Getting-started/Coordinate-Systems)
