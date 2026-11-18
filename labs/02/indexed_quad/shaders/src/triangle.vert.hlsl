struct VSInput {
	float2 position : TEXCOORD0;
	linear float3 color : TEXCOORD1;
};

struct VSOutput {
	float4 position : SV_Position;
	float3 color : TEXCOORD0;
};

VSOutput main(VSInput input) {
	VSOutput output;

	output.position = float4(input.position, 0.0f, 1.0f);
	output.color = input.color;

	return output;
}
