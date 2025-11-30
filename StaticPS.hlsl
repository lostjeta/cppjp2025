// ===================================================
// StaticPS.hlsl - Standalone Static Pixel Shader
// Simple pixel shader with basic Blinn-Phong lighting (no normal mapping)
// ===================================================

// ----- Textures and Samplers -----
Texture2D g_DiffuseMap : register(t0);
SamplerState g_Sampler : register(s0);

// ----- Lighting Structures -----
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
    float3 att;  // (constant, linear, quadratic)
    float  pad;
};

struct Material
{
    float4 ambient;
    float4 diffuse;
    float4 specular; // w = specular power
    float4 reflect;  // w = roughness
};

// ----- Constant Buffer -----
cbuffer ConstantBuffer : register(b0)
{
    matrix g_World;
    matrix g_View;
    matrix g_Proj;
    matrix g_WorldInvTranspose;

    Material g_Material;
    DirectionalLight g_DirLight;
    PointLight g_PointLight;
    
    float3 g_EyePosW;
    float  g_Pad;
    int    g_ShadingMode;
    float3 g_Pad2;
}

// ----- Vertex Output (Input to Pixel Shader) -----
struct VertexOut
{
    float4 posH      : SV_POSITION;
    float3 posW      : TEXCOORD0;    // World position
    float3 normalW   : TEXCOORD1;    // World normal
    float2 tex       : TEXCOORD2;
    float4 color     : COLOR;
    float3 tangentW  : TEXCOORD3;    // World tangent
    float3 bitanW    : TEXCOORD4;    // World bitangent
};

// ----- Pixel Shader Entry Point -----
float4 main(VertexOut vIn) : SV_TARGET
{
    // Normalize interpolated normal
    float3 normalW = normalize(vIn.normalW);
    
    // Sample diffuse texture
    float4 texColor = g_DiffuseMap.Sample(g_Sampler, vIn.tex);
    
    // Directional light calculation
    float3 lightVec = -g_DirLight.direction;
    float diffuseFactor = max(dot(lightVec, normalW), 0.0f);
    
    // Ambient term
    float3 ambient = g_Material.ambient.rgb * g_DirLight.ambient.rgb;
    
    // Diffuse term
    float3 diffuse = diffuseFactor * g_Material.diffuse.rgb * g_DirLight.diffuse.rgb;
    
    // Specular term (Blinn-Phong)
    float3 toEye = normalize(g_EyePosW - vIn.posW);
    float3 halfVec = normalize(toEye + lightVec);
    float specFactor = pow(max(dot(halfVec, normalW), 0.0f), g_Material.specular.w);
    float3 specular = specFactor * g_Material.specular.rgb * g_DirLight.specular.rgb;
    
    // Final color = (ambient + diffuse) * texture + specular
    float3 litColor = (ambient + diffuse) * texColor.rgb + specular;
    
    return float4(litColor, texColor.a);
}
