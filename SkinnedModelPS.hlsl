// Skinned Pixel Shader with Normal Mapping
// Supports directional and point lights with normal map support

// Textures and Sampler
Texture2D diffuseMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D specularMap : register(t2);
SamplerState samplerState : register(s0);

// Light structures
struct DirectionalLight
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
    float3 direction;
    float  pad;
};

struct PointLight
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
    float3 position;
    float  range;
    float3 att;
    float  pad;
};

struct Material
{
    float4 ambient;
    float4 diffuse;
    float4 specular; // w = specular power
    float4 reflect;
};

// Constant Buffer
cbuffer ConstantBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
    matrix worldInvTranspose;
    
    Material material;
    DirectionalLight dirLight;
    PointLight pointLight;
    
    float3 eyePosW;
    float  pad;
    int    shadingMode;
    float3 pad2;
    int    enableNormalMap;
    float3 pad3;
    int    useSpecularMap;
    float3 pad4;
}

// Vertex Output (input to pixel shader)
struct VertexOut
{
    float4 posH      : SV_POSITION;
    float3 posW      : TEXCOORD0;
    float3 normalW   : TEXCOORD1;
    float2 tex       : TEXCOORD2;
    float4 color     : COLOR;
    float3 tangentW  : TEXCOORD3;
    float3 bitanW    : TEXCOORD4;
};

// Lighting calculation functions
void CalculatePhong(float3 n, float3 L, float3 V, float3 lightDiffuse, float3 lightSpecular, float3 specMapColor, out float3 diffuse, out float3 specular)
{
    // Diffuse
    float NdotL = max(dot(n, L), 0.0f);
    diffuse = NdotL * material.diffuse.rgb * lightDiffuse;
    
    // Specular (Phong: reflect vector)
    float3 R = reflect(-L, n);
    float RdotV = max(dot(R, V), 0.0f);
    float specFactor = pow(RdotV, material.specular.w);
    specular = specFactor * specMapColor.rgb * lightSpecular;
}

void CalculateBlinnPhong(float3 n, float3 L, float3 V, float3 lightDiffuse, float3 lightSpecular, float3 specMapColor, out float3 diffuse, out float3 specular)
{
    // Diffuse
    float NdotL = max(dot(n, L), 0.0f);
    diffuse = NdotL * material.diffuse.rgb * lightDiffuse;
    
    // Specular (Blinn-Phong: half vector)
    float3 H = normalize(V + L);
    float NdotH = max(dot(n, H), 0.0f);
    float specFactor = pow(NdotH, material.specular.w);
    specular = specFactor * specMapColor.rgb * lightSpecular;
}

float3 CalculateLambert(float3 n, float3 L, float3 lightDiffuse)
{
    // Diffuse only (no specular)
    float NdotL = max(dot(n, L), 0.0f);
    float3 diffuse = NdotL * material.diffuse.rgb * lightDiffuse;
    
    return diffuse;
}

// Normal mapping function
float3 CalculateNormalFromMap(float3 normalW, float3 tangentW, float3 bitangentW, float2 texCoords)
{
    // === 노멀맵 샘플링 및 변환 과정 ===
    
    // 1. 노멀맵에서 RGB 값 샘플링
    // 노멀맵 텍스처의 각 픽셀은 노멀 벡터를 RGB 형태로 저장
    // R: X 성분, G: Y 성분, B: Z 성분
    float3 normalT = normalMap.Sample(samplerState, texCoords).rgb;
    
    // 2. 텍스처 좌표계에서 노멀 벡터 좌표계로 변환
    // 텍스처는 [0, 1] 범위의 값을 가지지만, 노멀 벡터는 [-1, 1] 범위를 가져야 함
    // 이 변환은 두 단계로 이루어짐:
    //   a) normalT * 2.0f: [0, 1] -> [0, 2] 범위로 확장
    //   b) normalT * 2.0f - 1.0f: [0, 2] -> [-1, 1] 범위로 이동
    // 결과: 0 -> -1, 0.5 -> 0, 1 -> 1
    normalT = normalT * 2.0f - 1.0f;
    
    // 3. 변환된 값의 의미:
    // - normalT.x = -1: 왼쪽 방향, +1: 오른쪽 방향 (탄젠트 축)
    // - normalT.y = -1: 아래 방향, +1: 위 방향 (비탄젠트 축)  
    // - normalT.z = -1: 안쪽 방향, +1: 바깥쪽 방향 (노멀 축)
    // 이것이 바로 탄젠트 스페이스에서의 노멀 벡터임
    
    // Build TBN matrix
    float3x3 TBN = float3x3(tangentW, bitangentW, normalW);
    
    // Transform normal from tangent space to world space
    float3 bumpedNormalW = mul(normalT, TBN);
    
    return normalize(bumpedNormalW);
}

// Pixel Shader Entry Point
float4 main(VertexOut vIn) : SV_TARGET
{
    // Normalize interpolated vectors
    float3 normalW = normalize(vIn.normalW);
    float3 tangentW = normalize(vIn.tangentW);
    float3 bitangentW = normalize(vIn.bitanW); 
    
    // Sample diffuse texture
    float4 texColor = diffuseMap.Sample(samplerState, vIn.tex);
    
    // Sample specular map (only if enabled)
    float4 specMapColor = material.specular; // Use material specular as default
    if (useSpecularMap == 1)
    {
        specMapColor = specularMap.Sample(samplerState, vIn.tex);
        
        // Fallback if specular map is all black
        float specSum = specMapColor.r + specMapColor.g + specMapColor.b;
        if (specSum < 0.01)
        {
            specMapColor = material.specular; // Use material specular as fallback
        }
    }
    
    // Calculate normal (with or without normal map)
    float3 n;
    if (enableNormalMap == 1)
    {
        // Use normal map
        n = CalculateNormalFromMap(normalW, tangentW, bitangentW, vIn.tex);
    }
    else
    {
        // Use vertex normal
        n = normalW;
    }
    
    // View direction (shared by all lighting modes)
    float3 V = normalize(eyePosW - vIn.posW);
    
    // Ambient (shared by all modes except Unlit)
    float3 ambient = material.ambient.rgb * dirLight.ambient.rgb;
    float3 diffuse = float3(0, 0, 0);
    float3 specular = float3(0, 0, 0);
    
    // === Shading Mode Selection ===
    // 0: Phong
    // 1: Blinn-Phong
    // 2: Lambert
    // 3: Unlit
    
    if (shadingMode == 3)
    {
        // Unlit: Just return texture color
        return texColor;
    }
    
    // === Directional Light ===
    float3 L = -dirLight.direction;
    
    if (shadingMode == 0)
    {
        // Phong
        float3 diff, spec;
        CalculatePhong(n, L, V, dirLight.diffuse.rgb, dirLight.specular.rgb, specMapColor.rgb, diff, spec);
        diffuse += diff;
        specular += spec;
    }
    else if (shadingMode == 1)
    {
        // Blinn-Phong
        float3 diff, spec;
        CalculateBlinnPhong(n, L, V, dirLight.diffuse.rgb, dirLight.specular.rgb, specMapColor.rgb, diff, spec);
        diffuse += diff;
        specular += spec;
    }
    else if (shadingMode == 2)
    {
        // Lambert
        float3 lighting = CalculateLambert(n, L, dirLight.diffuse.rgb);
        diffuse += lighting;
    }
    
    // === Point Light ===
    float3 lightVec = pointLight.position - vIn.posW;
    float d = length(lightVec);
    
    // Only calculate if within range
    if (d < pointLight.range)
    {
        lightVec /= d; // Normalize
        
        // Point light ambient
        ambient += material.ambient.rgb * pointLight.ambient.rgb;
        
        // Attenuation
        float att = 1.0f / dot(pointLight.att, float3(1.0f, d, d * d));
        
        if (shadingMode == 0)
        {
            // Phong
            float3 diff, spec;
            CalculatePhong(n, lightVec, V, pointLight.diffuse.rgb, pointLight.specular.rgb, specMapColor.rgb, diff, spec);
            diffuse += diff * att;
            specular += spec * att;
        }
        else if (shadingMode == 1)
        {
            // Blinn-Phong
            float3 diff, spec;
            CalculateBlinnPhong(n, lightVec, V, pointLight.diffuse.rgb, pointLight.specular.rgb, specMapColor.rgb, diff, spec);
            diffuse += diff * att;
            specular += spec * att;
        }
        else if (shadingMode == 2)
        {
            // Lambert
            float3 lighting = CalculateLambert(n, lightVec, pointLight.diffuse.rgb);
            diffuse += lighting * att;
        }
    }
    
    // Final color
    float3 litColor = (ambient + diffuse) * texColor.rgb + specular;
    
    return float4(litColor, texColor.a);
}
