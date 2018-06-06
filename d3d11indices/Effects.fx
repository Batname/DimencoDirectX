cbuffer cbPerObject
{
	float4x4 WVP;
	float2 ViewRect;
};

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 Color : COLOR;
};

VS_OUTPUT VS(float4 inPos : POSITION, float4 inColor : COLOR)
{
    VS_OUTPUT output;

	output.Pos = mul(inPos, WVP);
    output.Color = inColor;

    return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    //return input.Color;
	return float4(1, 0, 0, 1);
}