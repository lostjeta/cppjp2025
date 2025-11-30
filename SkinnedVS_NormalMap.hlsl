// Skinned Vertex Shader with Normal Mapping Support
// Supports skeletal animation with bone palette and tangent space vectors for normal mapping

// Constant Buffer (b0)
cbuffer ConstantBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
    matrix worldInvTranspose;
    
    // Material (not used in VS, but keeps buffer layout consistent)
    float4 materialAmbient;
    float4 materialDiffuse;
    float4 materialSpecular;
    float4 materialReflect;
    
    // DirectionalLight (not used in VS)
    float4 dirLightAmbient;
    float4 dirLightDiffuse;
    float4 dirLightSpecular;
    float3 dirLightDirection;
    float  dirLightPad;
    
    // PointLight (not used in VS)
    float4 pointLightAmbient;
    float4 pointLightDiffuse;
    float4 pointLightSpecular;
    float3 pointLightPosition;
    float  pointLightRange;
    float3 pointLightAtt;
    float  pointLightPad;
    
    float3 eyePosW;
    float  pad;
    int    shadingMode;
    float3 pad2;
    int    enableNormalMap;       // Used only in pixel shader
    float3 pad3;
    int    useSpecularMap;
    float3 pad4;
}

// Bone Palette Buffer (b1)
#define MAX_BONES 1023
cbuffer BonesBuffer : register(b1)
{
    row_major matrix bonePalette[MAX_BONES];
    uint boneCount;
    float3 bonePad;
}

// Vertex Input (Skinned format with tangent space)
struct VertexInSkinned
{
    float3 posL     : POSITION;
    float3 normalL  : NORMAL;
    float3 tangentL : TANGENT;
    float3 bitanL   : BINORMAL;
    float2 tex      : TEXCOORD;
    float4 color    : COLOR;
    uint4  boneIdx  : BLENDINDICES;
    float4 boneW    : BLENDWEIGHT;
};

// Vertex Output (includes tangent space for normal mapping)
struct VertexOut
{
    float4 posH      : SV_POSITION;
    float3 posW      : TEXCOORD0;
    float3 normalW   : TEXCOORD1;
    float2 tex       : TEXCOORD2;
    float4 color     : COLOR;
    float3 tangentW  : TEXCOORD3;    // Required for normal mapping
    float3 bitanW    : TEXCOORD4;    // Required for normal mapping
};

// Vertex Shader Entry Point
VertexOut main(VertexInSkinned vIn)
{
    VertexOut vOut;
    
    // Skinning: blend up to 4 bones
    float4x4 skinTransform = (float4x4)0;
    for (int i = 0; i < 4; ++i)
    {
        uint boneIndex = vIn.boneIdx[i];
        float weight = vIn.boneW[i];
        
        if (weight > 0.0f && boneIndex < boneCount)
        {
            skinTransform += bonePalette[boneIndex] * weight;
        }
    }
    
    // Apply skinning to position
    float4 posL = mul(float4(vIn.posL, 1.0f), skinTransform);
    
    // Transform to world space
    float4 posW = mul(posL, world);
    vOut.posH = mul(posW, view);
    vOut.posH = mul(vOut.posH, proj);
    vOut.posW = posW.xyz;
    
    // Apply skinning to normal, tangent, bitangent (only rotation, no translation)
    // This ensures proper tangent space transformation for normal mapping
    float3 normalL = mul(vIn.normalL, (float3x3)skinTransform);
    float3 tangentL = mul(vIn.tangentL, (float3x3)skinTransform);
    float3 bitanL = mul(vIn.bitanL, (float3x3)skinTransform);
    
    // Transform to world space
    // === 노멀 벡터 변환 (비균일 스케일링 보정) ===
    // 1. 일반적인 월드 행렬 사용 시 문제점:
    //    - 비균일 스케일링(x, y, z가 다른 비율) 적용 시 노멀이 표면에 수직을 유지하지 못함
    //    - 예: 구를 타원으로 늘리면 노멀이 더 이상 표면에서 바깥을 향하지 않음
    
    // 2. 해결책: 역전치 행렬(Inverse Transpose Matrix) 사용
    //    - worldInvTranspose = (world^-1)^T
    //    - 이 행렬은 비균일 스케일링을 보정하여 노멀이 항상 표면에 수직을 유지하도록 함
    
    // 3. 역전치 행렬의 의미:
    //    **역행렬(Inverse)**: 원래 변환을 되돌리는 행렬
    //    - M * M^-1 = I (단위행렬)
    //    - 변환된 공간을 원래 공간으로 되돌림
    //    
    //    **전치 행렬(Transpose)**: 행과 열을 바꾼 행렬
    //    - M^T[i,j] = M[j,i]
    //    - **중요**: 일반적인 전치 행렬은 직교성을 보존하지 않음!
    //    - 직교성을 보존하는 것은 **직교 행렬(Orthogonal Matrix)**뿐
    //    - 직교 행렬의 조건: M^T = M^-1 (전치가 역행렬과 같음)
    //    
    //    **역전치 행렬**: 역행렬의 전치
    //    - (M^-1)^T = (M^T)^-1
    //    - 비균일 스케일링에서 노멀과 접선의 직교성을 보존하는 핵심 역할
    
    // 4. 왜 노멀에만 역전치 행렬이 필요한가?
    //    **위치 벡터**: 일반적인 월드 행렬로 변환 가능
    //    - 위치는 방향과 크기를 모두 가짐
    //    - 스케일링, 회전, 이동 모두 적용됨
    //    
    //    **방향 벡터(탄젠트, 비탄젠트)**: 월드 행렬의 회전/스케일링만 적용
    //    - 방향만 있으므로 이동은 제외 (float3x3 캐스팅)
    //    - 표면의 접선 방향을 따라감
    //    
    //    **노멀 벡터**: 역전치 행렬로 변환해야 함
    //    - 표면에 수직을 유지해야 하는 특별한 제약
    //    - 비균일 스케일링 시 수직성이 깨지기 때문에 보정 필요
    
    // 5. 변환 과정:
    //    a) 스키닝된 로컬 노멀 normalL을 (float3x3)worldInvTranspose로 변환
    //    b) float3x3으로 캐스팅하여 이동 성분 제거 (노멀은 방향만 있으므로)
    //    c) 정규화하여 단위 벡터로 만듦
    
    // 6. 수학적 원리:
    //    - 만약 M이 비균일 스케일링 행렬이라면, 노멀 n에 대해:
    //    - 올바른 변환: n' = normalize((M^-1)^T * n)
    //    - 이는 노멀이 변환된 표면에 항상 수직을 유지하게 함
    //    
    //    **직교성 보존의 원리**:
    //    - 원래 노멀 n은 표면의 접선 t와 직교: n · t = 0
    //    - 변환 후에도 직교성 유지: n' · t' = 0
    //    - 역전치 행렬이 이 직교성을 보존함
    
    vOut.normalW = normalize(mul(normalL, (float3x3)worldInvTranspose));
    
    // Tangent and bitangent transform with world matrix (no inverse transpose needed)
    // They form the TBN matrix with the normal for normal mapping
    vOut.tangentW = mul(tangentL, (float3x3)world);
    vOut.bitanW = mul(bitanL, (float3x3)world);
    
    vOut.tex = vIn.tex;
    vOut.color = vIn.color;
    
    return vOut;
}
