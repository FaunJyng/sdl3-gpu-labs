cbuffer Triangle : register(b0, space1) { float4 vertices[3]; };

float4 main(uint id: SV_VertexID) : SV_Position { return float4(vertices[id].xy, 0.0f, 1.0f); }
