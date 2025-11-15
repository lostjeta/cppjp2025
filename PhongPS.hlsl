// Per Frame
cbuffer LightCBuf
{
    float3 lightPos;
    float3 ambient;
    float3 diffuseColor;
    float diffuseIntensity;
    float attConst;
    float attLin;
    float attQuad;
};

// Per Object
cbuffer ObjectCBuf
{
    float3 materialColor;
    float specularIntensity;
    float specularPower;
};

//static const float3 materialColor = { 0.7f, 0.7f, 0.9f };
//static const float3 ambient = { 0.15f, 0.15f, 0.15f };
//static const float3 diffuseColor = { 1.0f, 1.0f, 1.0f };
//static const float diffuseIntensity = 1.0f;
//static const float attConst = 1.0f;
//static const float attLin = 0.045f;
//static const float attQuad = 0.0075f;

float4 main(float3 worldPos : Position, float3 n : Normal) : SV_TARGET
{
    // pixel to light vector data
    const float3 vToL = lightPos - worldPos;
    const float distToL = length(vToL);
    const float dirToL = vToL / distToL;
    // diffuse attenuation
    const float att = 1 / (attConst + attLin * distToL + attQuad * (distToL * distToL));
    // diffuse intensity
    const float3 diffuse = diffuseColor * diffuseIntensity * att * max(0.0f, dot(dirToL, n));
    
    // reflected light vector
    const float3 w = n * dot(vToL, n);
    const float3 r = w * 2.0f - vToL;
    // calculate specular intensity based on angle between viewing vector
    const float3 specular = att * (diffuseColor * diffuseIntensity) * specularIntensity * pow(max(0.0f, dot(normalize(-r), normalize(worldPos))), specularPower);
    
    // final color
    return float4(saturate((diffuse + ambient + specular) * materialColor), 1.0f);
}