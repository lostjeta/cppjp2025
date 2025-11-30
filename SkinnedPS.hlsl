// Skinned Pixel Shader (독립 버전)
// Supports directional and point lights

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

// Lighting calculation functions
void CalculatePhong(float3 n, float3 L, float3 V, float3 lightDiffuse, float3 lightSpecular, out float3 diffuse, out float3 specular)
{
    // Diffuse
    float NdotL = max(dot(n, L), 0.0f);
    diffuse = NdotL * material.diffuse.rgb * lightDiffuse;
    
    // Specular (Phong: reflect vector)
    float3 R = reflect(-L, n);
    float RdotV = max(dot(R, V), 0.0f);
    float specFactor = pow(RdotV, material.specular.w);
    specular = specFactor * material.specular.rgb * lightSpecular;
}

void CalculateBlinnPhong(float3 n, float3 L, float3 V, float3 lightDiffuse, float3 lightSpecular, out float3 diffuse, out float3 specular)
{
    // Diffuse
    float NdotL = max(dot(n, L), 0.0f);
    diffuse = NdotL * material.diffuse.rgb * lightDiffuse;
    
    // Specular (Blinn-Phong: half vector)
    float3 H = normalize(V + L);
    float NdotH = max(dot(n, H), 0.0f);
    float specFactor = pow(NdotH, material.specular.w);
    specular = specFactor * material.specular.rgb * lightSpecular;
}

float3 CalculateLambert(float3 n, float3 L, float3 lightDiffuse)
{
    // Diffuse only (no specular)
    float NdotL = max(dot(n, L), 0.0f);
    float3 diffuse = NdotL * material.diffuse.rgb * lightDiffuse;
    
    return diffuse;
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
    
    // Use vertex normal (normal mapping can be added later)
    float3 n = normalW;
    
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
        CalculatePhong(n, L, V, dirLight.diffuse.rgb, dirLight.specular.rgb, diff, spec);
        diffuse += diff;
        specular += spec;
    }
    else if (shadingMode == 1)
    {
        // Blinn-Phong
        float3 diff, spec;
        CalculateBlinnPhong(n, L, V, dirLight.diffuse.rgb, dirLight.specular.rgb, diff, spec);
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
            CalculatePhong(n, lightVec, V, pointLight.diffuse.rgb, pointLight.specular.rgb, diff, spec);
            diffuse += diff * att;
            specular += spec * att;
        }
        else if (shadingMode == 1)
        {
            // Blinn-Phong
            float3 diff, spec;
            CalculateBlinnPhong(n, lightVec, V, pointLight.diffuse.rgb, pointLight.specular.rgb, diff, spec);
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
