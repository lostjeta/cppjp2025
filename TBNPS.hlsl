// TBN Pixel Shader (독립 버전)
// Supports normal mapping with Tangent, Bitangent, Normal

// Textures and Sampler
Texture2D diffuseMap : register(t0);
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

// Pixel Shader Entry Point
float4 main(VertexOut vIn) : SV_TARGET
{
    // Normalize interpolated vectors
    float3 normalW = normalize(vIn.normalW);
    float3 tangentW = normalize(vIn.tangentW);
    float3 bitangentW = normalize(vIn.bitanW);
    
    // Sample diffuse texture
    float4 texColor = diffuseMap.Sample(samplerState, vIn.tex);
    
    // Optional: Sample normal map (if enableNormalMap is set)
    // For now, just use vertex normal
    float3 n = normalW;
    
    // View direction (shared by both lights)
    float3 V = normalize(eyePosW - vIn.posW);
    
    // === Directional Light ===
    float3 L = -dirLight.direction;
    float NdotL = max(dot(L, n), 0.0f);
    
    float3 ambient = material.ambient.rgb * dirLight.ambient.rgb;
    float3 diffuse = NdotL * material.diffuse.rgb * dirLight.diffuse.rgb;
    
    // Specular (Blinn-Phong)
    float3 H = normalize(V + L);
    float NdotH = pow(max(dot(H, n), 0.0f), material.specular.w);
    float3 specular = NdotH * material.specular.rgb * dirLight.specular.rgb;
    
    // === Point Light ===
    float3 lightVec = pointLight.position - vIn.posW;
    float d = length(lightVec);
    
    // Only calculate if within range
    if (d < pointLight.range)
    {
        lightVec /= d; // Normalize
        
        // Point light ambient
        ambient += material.ambient.rgb * pointLight.ambient.rgb;
        
        // Point light diffuse
        float pointNdotL = max(dot(lightVec, n), 0.0f);
        
        // Point light specular
        float3 pointH = normalize(V + lightVec);
        float pointNdotH = pow(max(dot(pointH, n), 0.0f), material.specular.w);
        
        // Attenuation
        float att = 1.0f / dot(pointLight.att, float3(1.0f, d, d * d));
        
        diffuse += pointNdotL * material.diffuse.rgb * pointLight.diffuse.rgb * att;
        specular += pointNdotH * material.specular.rgb * pointLight.specular.rgb * att;
    }
    
    // Final color
    float3 litColor = (ambient + diffuse) * texColor.rgb + specular;
    
    return float4(litColor, texColor.a);
}
