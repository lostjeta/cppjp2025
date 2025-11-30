// slot0: 프레임마다 변경될 수 있는 라이트 정보
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

// slot1: 오브젝트마다 다른 재질 정보
// Per Object
cbuffer ObjectCBuf
{
    // GPU에서는 float3를 16바이트 정렬로 처리한다. 그러나 마지막 float3 다음에 float 있으므로 패딩하지 않고 다음을 사용한다.
    // CPU 사이드와의 불일치를 막기위해 padding 추가하여 맞춘다.
    float3 materialColors[6];
    float padding;
    float specularIntensity;
    float specularPower;
};


float4 main(float3 worldPos : Position, float3 n : Normal, uint tid : SV_PrimitiveID) : SV_Target
{
	// fragment to light vector data
    const float3 vToL = lightPos - worldPos;
    const float distToL = length(vToL);
    const float3 dirToL = vToL / distToL;
	// attenuation
    const float att = 1.0f / (attConst + attLin * distToL + attQuad * (distToL * distToL));
	// diffuse intensity
    const float3 diffuse = diffuseColor * diffuseIntensity * att * max(0.0f, dot(dirToL, n));
	// reflected light vector
    const float3 w = n * dot(vToL, n);
    const float3 r = w * 2.0f - vToL;
    // 뷰 공간에서는 카메라가 원점(0,0,0)에 위치하므로
    // 픽셀 → 카메라 방향 = 0 - worldPos = -worldPos
    const float3 viewDir = normalize(-worldPos);
	// calculate specular intensity based on angle between viewing vector and reflection vector, narrow with power function
    //const float3 specular = att * (diffuseColor * diffuseIntensity) * specularIntensity
    //                    * pow(max(0.0f, dot(normalize(-r), normalize(worldPos))), specularPower);
    const float3 specular = att * (diffuseColor * diffuseIntensity) * specularIntensity
                        * pow(max(0.0f, dot(normalize(r), viewDir)), specularPower);
	// final color
    return float4(saturate((diffuse + ambient + specular) * materialColors[(tid / 2) % 6]), 1.0f);
}