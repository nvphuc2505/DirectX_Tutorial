
cbuffer cbChangesEveryFrame : register(b0)
{
    matrix WorldViewProj;
    //matrix World;
};

//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float3 Pos : POSITION;
    float4 Color : COLOR;
};

struct PS_INPUT
{
    float4 PosH : SV_Position;
    float4 Color : COLOR;
};



//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    
    output.PosH = mul(float4(input.Pos, 1.0f), WorldViewProj);
    output.Color = input.Color;
    
    return output;
}



//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
    return input.Color;
}