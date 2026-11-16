struct VSOutput {
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
};

VSOutput main(uint vertex_id: SV_VertexID) {
	VSOutput output;

	float2 p;

	if (vertex_id == 0)
		p = float2(0, 0);
	else if (vertex_id == 1)
		p = float2(2, 0);
	else
		p = float2(0, 2);

	output.position = float4(p * 2.0 - 1.0, 0.0, 1.0);
	output.uv = p;

	return output;
}
