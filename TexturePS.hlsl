//Texture2D tex : register(t0); // 텍스처 생성에 사용한 slot0과 같은 위치로 맞춘다.
Texture2D tex;

SamplerState splr; // 기본값은 0번 슬랏 텍스처의 샘플러상태

float4 main(float2 tc : TexCoord) : SV_TARGET
{
	return tex.Sample(splr, tc);
}