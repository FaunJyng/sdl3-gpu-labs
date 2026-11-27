cbuffer PushConstants : register(b0, space1)
{
	float4 vertices[3];	 // use only x and y
	float phase;		 // [0, 6)
	float3 pad;
};

static const float3 PALETTE[6] = {
	float3(1, 0, 0),  // red
	float3(1, 0, 1),  // magenta
	float3(1, 1, 0),  // yellow
	float3(0, 1, 0),  // green
	float3(0, 1, 1),  // cyan
	float3(0, 0, 1),  // blue
};

struct VSOutput {
	float4 position : SV_Position;
	linear float3 color : TEXCOORD0;
};

VSOutput main(uint id: SV_VertexID)
{
	uint index = (uint)floor(phase);
	float t = smoothstep(0.0f, 1.0f, frac(phase));

	float3 c0 = PALETTE[(id + index) % 6];
	float3 c1 = PALETTE[(id + index + 1) % 6];

	VSOutput o;
	o.position = float4(vertices[id].xy, 0.0f, 1.0f);
	o.color = lerp(c0, c1, t);
	return o;
}
