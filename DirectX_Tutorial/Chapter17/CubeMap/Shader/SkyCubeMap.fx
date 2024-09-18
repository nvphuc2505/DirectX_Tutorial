
cbuffer cPerFrame
{
    float4x4 gWorldViewProj;
};

TextureCube gCubeMap;
SamplerState samTriLinearSam
{
    Filter = MIN_MAG_MIP_LINEAR;

    AddressU = Wrap;
    AddressV = Wrap;
};


struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj).xyww;
    vout.PosL = vin.PosL;
    
    return vout;
}

float4 PS(VertexOut vout) : SV_Target
{
    return gCubeMap.Sample(samTriLinearSam, vout.PosL);
}



RasterizerState NoCull
{
    CullMode = None;
};

DepthStencilState LessEqualDSS
{
    DepthFunc = LESS_EQUAL;
};

technique11 SkyTech
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS()));
        
        SetRasterizerState(NoCull);
        SetDepthStencilState(LessEqualDSS, 0);
    }
}