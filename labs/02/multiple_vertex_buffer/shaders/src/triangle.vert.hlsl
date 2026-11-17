struct VSInput {
	float2 position : TEXCOORD0;
	float3 color : TEXCOORD1;
};

struct VSOutput {
	float4 position : SV_Position;
	float3 color : COLOR0;
};

VSOutput main(VSInput input) {
	VSOutput output;

	output.position = float4(input.position, 0.0, 1.0);
	output.color = input.color;

	return output;
}
