// Skinned Vertex Shader (독립 버전)
// Supports skeletal animation with bone palette

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
    int    enableNormalMap;
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

// Vertex Input (Skinned format)
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

// Vertex Output
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
    float3 normalL = mul(vIn.normalL, (float3x3)skinTransform);
    float3 tangentL = mul(vIn.tangentL, (float3x3)skinTransform);
    float3 bitanL = mul(vIn.bitanL, (float3x3)skinTransform);
    
    // Transform to world space
    vOut.normalW = normalize(mul(normalL, (float3x3)worldInvTranspose));
    vOut.tangentW = mul(tangentL, (float3x3)world);
    vOut.bitanW = mul(bitanL, (float3x3)world);
    
    vOut.tex = vIn.tex;
    vOut.color = vIn.color;
    
    return vOut;
}
