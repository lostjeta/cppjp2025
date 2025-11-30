// ===================================================
// StaticVS.hlsl - Standalone Static Vertex Shader
// Simple static mesh vertex shader (Position + Normal + TexCoord)
// ===================================================

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

// ----- Vertex Input (Simple) -----
struct VertexInSimple
{
    float3 posL    : POSITION;
    float3 normalL : NORMAL;
    float2 tex     : TEXCOORD;
};

// ----- Vertex Output -----
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

// ----- Vertex Shader Entry Point -----
VertexOut main(VertexInSimple vIn)
{
    VertexOut vOut;
    
    // Transform position to world space
    float4 posW = mul(float4(vIn.posL, 1.0f), g_World);
    vOut.posW = posW.xyz;
    
    // Transform to homogeneous clip space
    vOut.posH = mul(posW, g_View);
    vOut.posH = mul(vOut.posH, g_Proj);

    // Transform normal to world space
    vOut.normalW = normalize(mul(vIn.normalL, (float3x3) g_WorldInvTranspose));
    
    // Default tangent and bitangent (not used for simple static models)
    vOut.tangentW = float3(1, 0, 0);
    vOut.bitanW   = float3(0, 1, 0);

    // Pass through texture coordinates
    vOut.tex = vIn.tex;
    
    // Default color (white)
    vOut.color = float4(1, 1, 1, 1);
    
    return vOut;
}
