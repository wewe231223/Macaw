struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
};

PS_INPUT mainVS(VS_INPUT Input)
{
    PS_INPUT Output;

    Output.Position = float4(Input.Position, 1.0f);
    Output.Normal = Input.Normal;

    return Output;
}

float4 mainPS(PS_INPUT Input) : SV_TARGET
{
    float3 Normal = normalize(Input.Normal);
    float3 LightDirection = normalize(float3(0.5f, 1.0f, -0.5f));

    float NDotL = saturate(dot(Normal, LightDirection));
    float3 BaseColor = float3(0.7f, 0.7f, 0.75f);

    return float4(BaseColor * (0.2f + NDotL * 0.8f), 1.0f);
}