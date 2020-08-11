///// INPUT /////
Texture2D<float4> Diffuse : register(t1);
Texture2D<float4> NormalMap : register(t2);
Texture2D<float4> SpecularMap : register(t3);
SamplerState TextureSampler : register(s0);

struct VSOut
{
    float4 position : SV_Position;
    float4 worldPosition : POSITION;
    float4 normal : NORMAL;
    float2 uv : UV;
    float4 color : COLOR;
	float3x3 tbn : TBN;
};

/// OUTPUT /////
struct FSOut
{
    float4 albedo : SV_TARGET0;
    float4 position : SV_TARGET1;
    float4 normal : SV_TARGET2;
    float4 specular : SV_TARGET3;
};

///// MAIN /////
FSOut main(VSOut input) : SV_TARGET
{
    FSOut output = (FSOut) 0;
    
    float4 diffuse = Diffuse.Sample(TextureSampler, input.uv);
    float3 normalMap = NormalMap.Sample(TextureSampler, input.uv).xyz;
    float4 specularMap = SpecularMap.Sample(TextureSampler, input.uv);


	float3 tempNormal = normalize(normalMap * 2.0f - 1.0f);
    
	tempNormal = mul(tempNormal, input.tbn);
    
    output.position = input.worldPosition;
	output.albedo = diffuse + input.color;
	output.normal = normalize(float4(tempNormal, 0.0f));

    output.specular = specularMap;
    
    return output;
}