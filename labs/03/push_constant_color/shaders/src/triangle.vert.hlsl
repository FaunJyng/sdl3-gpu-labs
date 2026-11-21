struct VSInput {
	float2 position : TEXCOORD0;
};

float4 main(VSInput input) : SV_Position { return float4(input.position, 0.0f, 1.0f); }
