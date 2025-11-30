// TBN Vertex Shader (독립 버전)
// Supports normal mapping with Tangent, Bitangent, Normal

// Constant Buffer
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

// Vertex Input (TBN format)
struct VertexIn
{
    float3 posL     : POSITION;
    float3 normalL  : NORMAL;
    float3 tangentL : TANGENT;
    float3 bitanL   : BINORMAL;
    float2 tex      : TEXCOORD;
    float4 color    : COLOR;
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
VertexOut main(VertexIn vIn)
{
    VertexOut vOut;
    
    // Transform position to world space
    float4 posL = float4(vIn.posL, 1.0f);
    float4 posW = mul(posL, world);
    float4 posV = mul(posW, view);
    vOut.posH = mul(posV, proj);
    vOut.posW = posW.xyz;

    // Transform normal, tangent, bitangent to world space
    float3 nW = normalize(mul(vIn.normalL, (float3x3) worldInvTranspose));
    float3 tW = mul(vIn.tangentL, (float3x3) world);
    float3 bW = mul(vIn.bitanL, (float3x3) world);

    vOut.normalW = nW;
    vOut.tangentW = tW;
    vOut.bitanW = bW;

    vOut.tex = vIn.tex;
    vOut.color = vIn.color;
    
    return vOut;
}
