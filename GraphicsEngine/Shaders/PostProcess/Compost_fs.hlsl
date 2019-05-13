Texture2D texColour : register(t0);
SamplerState defaultSampler : register (s0);

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

float4 main(VS_OUTPUT input) : SV_Target
{
	float3 output = texColour.Sample(defaultSampler, input.uv).xyz;
	const float gamma = 1.0f / 2.2f;
	output = pow(abs(output), float3(gamma, gamma, gamma));
	return float4(output,1.0f);
}
