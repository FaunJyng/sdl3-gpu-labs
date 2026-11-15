struct PSInput {
  float4 position : SV_Position;
  float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target0 {
  float t = saturate(input.uv.x);

  float3 red = float3(1, 0, 0);
  float3 green = float3(0, 1, 0);
  float3 blue = float3(0, 0, 1);

  float3 color =
      t < 0.5 ? lerp(red, green, t * 2.0) : lerp(green, blue, (t - 0.5) * 2.0);

  return float4(color, 1.0);
}