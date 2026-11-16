struct VSOutput {
	float4 position : SV_Position;
	linear float3 color : COLOR0;
};

VSOutput main(uint vertex_id: SV_VertexID) {
	float2 position[3] = { float2(0.0, 0.6), float2(0.6, -0.6), float2(-0.6, -0.6) };
	float3 colors[3] = { float3(1.0, 0.0, 0.0), float3(0.0, 1.0, 0.0), float3(0.0, 0.0, 1.0) };

	VSOutput output;
	output.position = float4(position[vertex_id], 0.0, 1.0);
	output.color = colors[vertex_id];

	return output;
}
