///// INPUT /////
TextureCube<float4> Diffuse : register(t16);
SamplerState TextureSampler : register(s0);

struct VSOut
{
    float4 position : SV_Position;
	float3 texCoords : TEXCOORDS;
};

/// OUTPUT /////
struct FSOut
{
	
    float4 albedo : SV_TARGET0;
};

///// MAIN /////
FSOut main(VSOut input) : SV_TARGET
{
    FSOut output = (FSOut) 0;
    
	output.albedo = Diffuse.Sample(TextureSampler, input.texCoords);
	//output.albedo = float4(0.f, 1.f, 0.f, 1.f);
    
    return output;
}