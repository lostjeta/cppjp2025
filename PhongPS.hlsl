/**
 * Phong Lighting Model Pixel Shader
 * 
 * Phong 조명 모델은 세 가지 구성 요소로 이루어집니다:
 * 1. Ambient (주변광): 모든 방향에서 균일하게 들어오는 빛
 * 2. Diffuse (난반사): 표면에서 모든 방향으로 균일하게 반사되는 빛 (Lambert)
 * 3. Specular (정반사): 특정 방향으로 강하게 반사되는 빛 (광택)
 * 
 * 최종 색상 = (Ambient + Diffuse + Specular) × MaterialColor
 */

// slot0: 프레임마다 변경될 수 있는 라이트 정보
// Per Frame
cbuffer LightCBuf
{
    // GPU에서는 float3를 16바이트 정렬로 처리한다.
    float3 lightPos;          // 라이트의 위치 (뷰 공간 좌표)
    float3 ambient;           // 주변광 색상 (RGB)
    float3 diffuseColor;      // 난반사광 색상 (RGB)
    float diffuseIntensity;   // 난반사광 강도 (0.0 ~ 2.0)
    
    // 거리에 따른 감쇠 계수 (Attenuation)
    // att = 1 / (attConst + attLin * d + attQuad * d^2)
    float attConst;           // 상수 감쇠 (일반적으로 1.0)
    float attLin;             // 선형 감쇠
    float attQuad;            // 이차 감쇠
};

// slot1: 오브젝트마다 다른 재질 정보
// Per Object
cbuffer ObjectCBuf
{
    // GPU에서는 float3를 16바이트 정렬로 처리한다.
    float3 materialColor;      // 재질의 기본 색상 (RGB)
    float specularIntensity;   // 정반사광 강도 (0.0 ~ 1.0)
    float specularPower;       // 정반사광 집중도 (높을수록 작고 밝은 하이라이트)
};

/**
 * Phong Lighting Pixel Shader Main Function
 * 
 * @param viewPos  픽셀의 위치 (뷰 공간 좌표)
 * @param n        픽셀의 법선 벡터 (정규화되어 있어야 함)
 * @return         최종 픽셀 색상 (RGBA)
 */
float4 main(float3 viewPos : Position, float3 n : Normal) : SV_TARGET
{
    // ========================================
    // 1단계: 픽셀에서 라이트로의 벡터 정보 계산
    // ========================================
    
    // 픽셀 → 라이트 벡터 (Vector to Light)
    const float3 vToL = lightPos - viewPos;
    
    // 픽셀과 라이트 사이의 거리
    const float distToL = length(vToL);
    
    // 픽셀 → 라이트 방향 (정규화된 벡터)
    const float3 dirToL = vToL / distToL;
    // ========================================
    // 2단계: 거리 감쇠 계산 (Attenuation)
    // ========================================
    
    // 거리에 따른 빛의 감쇠
    // 물리적으로 빛은 거리의 제곱에 반비례하지만,
    // 예술적 효과를 위해 조정 가능한 공식 사용
    // att = 1 / (C + L*d + Q*d²)
    const float att = 1.0f / (attConst + attLin * distToL + attQuad * (distToL * distToL));
    
    // ========================================
    // 3단계: Diffuse (난반사) 계산
    // ========================================
    
    // Lambert의 코사인 법칙:
    // 표면의 밝기는 법선과 광선 방향의 코사인에 비례
    // diffuse = lightColor × intensity × attenuation × max(0, N·L)
    //
    // dot(dirToL, n): 법선과 라이트 방향의 내적 = cos(θ)
    // - θ = 0°  (정면): cos(0) = 1.0  → 최대 밝기
    // - θ = 90° (측면): cos(90) = 0.0 → 어두움
    // - θ > 90° (뒷면): cos > 0 이면 음수 → max(0, ...)로 클램핑
    const float3 diffuse = diffuseColor * diffuseIntensity * att * max(0.0f, dot(dirToL, n));
    
    // ========================================
    // 4단계: Specular (정반사) 계산
    // ========================================
    
    // 4-1. 반사 벡터 계산
    // 입사광(vToL)이 표면에서 반사되는 방향을 계산
    // 공식: r = 2(N·L)N - L
    // 
    // 기하학적 설명:
    //   N (법선)
    //   |
    //   |  
    //   +------ w = N × (N·L)  (법선 방향 투영)
    //  /|
    // L | R (반사 벡터)
    //
    const float3 w = n * dot(vToL, n);      // 법선 방향으로의 투영
    const float3 r = w * 2.0f - vToL;       // 반사 벡터 = 2w - L
    
    // 4-2. 뷰 방향 계산
    // 뷰 공간에서는 카메라가 원점(0,0,0)에 위치하므로
    // 픽셀 → 카메라 방향 = 0 - viewPos = -viewPos
    const float3 viewDir = normalize(-viewPos);
    
    // 4-3. Specular 강도 계산
    // Phong Reflection Model:
    // specular = lightColor × intensity × attenuation × 
    //            specularIntensity × (R·V)^specularPower
    //
    // (R·V)^n:
    // - R·V: 반사 벡터와 뷰 방향의 내적 = cos(α)
    // - specularPower (n): 하이라이트 집중도
    //   * n이 클수록: 작고 밝은 하이라이트 (금속, 유리)
    //   * n이 작을수록: 크고 흐릿한 하이라이트 (플라스틱)
    //
    // 예: n = 30이면 α = 20°에서도 cos^30(20°) ≈ 0.3
    //     n = 100이면 α = 10°에서 cos^100(10°) ≈ 0.0
    const float3 specular = att * (diffuseColor * diffuseIntensity) * specularIntensity
                        * pow(max(0.0f, dot(normalize(r), viewDir)), specularPower);
    
    // ========================================
    // 5단계: 최종 색상 합성
    // ========================================
    
    // Phong 조명 모델 최종 공식:
    // FinalColor = (Ambient + Diffuse + Specular) × MaterialColor
    //
    // Ambient:  주변광 (항상 일정)
    // Diffuse:  난반사광 (각도에 따라 변화)
    // Specular: 정반사광 (뷰 각도와 반사각에 따라 변화)
    //
    // saturate(): [0, 1] 범위로 클램핑
    // - 값이 1.0을 초과하면 1.0으로
    // - 값이 0.0 미만이면 0.0으로
    return float4(saturate((diffuse + ambient + specular) * materialColor), 1.0f);
    
    // 법선을 RGB 색상으로 표시 (-1~1 → 0~1 변환)
    //return float4(n * 0.5 + 0.5, 1.0f);
}