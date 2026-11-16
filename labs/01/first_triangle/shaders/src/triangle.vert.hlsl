struct VSOutput {
	float4 position : SV_Position;
};

VSOutput main(uint vertex_id: SV_VertexID) {
	float2 position[3] = { float2(0.0, 0.6), float2(0.6, -0.6), float2(-0.6, -0.6) };

	VSOutput output;
	output.position = float4(position[vertex_id], 0.0, 1.0);

	return output;
}
