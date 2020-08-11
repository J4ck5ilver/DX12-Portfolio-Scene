///// INPUT /////
Texture2D<float4> Albedo : register(t11);
Texture2D<float4> Position : register(t12);
Texture2D<float4> Normal : register(t13);
Texture2D<float4> Specular : register(t14);
SamplerState TextureSampler : register(s0);

struct VSOut
{
	float4 triPosition : SV_Position;
	float2 uv : UV;
};

cbuffer Camera : register(b0)
{
	float4 camPosition;
}

struct PointLight
{
	float3 color;
	float strength;
	float3 position;
	float radius;
	
};
StructuredBuffer<PointLight> pointLights : register(t15);

// DirectionalLights
// No upload yet
static const float3 DL1_color = { 1, 1, 1 };
static const float DL1_strength = { 0.75 };
static const float3 DL1_direction = { 1, 0, 0 };

static const float3 DL2_color = { 1, 0.2, 0.8 };
static const float DL2_strength = { 0.15 };
static const float3 DL2_direction = { 1, 1, 1 };


cbuffer LightCount : register(b2)
{
	unsigned int numberOfPointLights;
}

#ifndef LIGHT_CULLING_VALUES
#define LIGHT_CULLING_VALUES
#define MAX_LIGHT 500
#define GRID_SIZE 32
#endif

struct TileData
{
	uint count;
	uint lightIndex[MAX_LIGHT];
};
StructuredBuffer<TileData> tileData : register(t17);

///// FUNCTION DECLARATIONS /////
float4 CalculatePointLight(PointLight light, float4 position, float4 albedo, float4 normal, float4 specularStrength);
float4 CalculateDirectionalLight(float3 DL_color, float DL_strength, float3 DL_direction, float4 albedo, float4 normal);

///// MAIN /////
float4 main(VSOut input) : SV_TARGET0
{
	float4 finalColor;
	PointLight lights;
	
	
    // Textures
	float4 normal = Normal.Sample(TextureSampler, input.uv);
	float4 albedo = Albedo.Sample(TextureSampler, input.uv);
	
	 //Ambient
	float ambientStrength = 0.3f;
	finalColor = ambientStrength * float4(0.5f, 0.9f, 1.5f, 1.f) * albedo;
	
	
	//Directional light
	finalColor += CalculateDirectionalLight(DL1_color, DL1_strength, DL1_direction, albedo, normal);
	finalColor += CalculateDirectionalLight(DL2_color, DL2_strength, DL2_direction, albedo, normal);
	
	//Point lights
	if (normal.x != 0.f && normal.y != 0.f && normal.z != 0.f)
	{
		// Textures
		float4 position = Position.Sample(TextureSampler, input.uv);
		float4 specularStrength = Specular.Sample(TextureSampler, input.uv);
		
		// TILE DATA CALCULATIONS (3 steps)
		// 1. Find screen space pixel location and 2. Figure out which tile it belongs to
		int tileLocation = int((GRID_SIZE * floor(GRID_SIZE * input.uv.y)) + floor(GRID_SIZE * input.uv.x));
        	
		//3. Loop through the tile's light array and perform the basic light calculations as usual
		if (tileLocation >= 0 && tileLocation < GRID_SIZE * GRID_SIZE)
		{
			for (int i = 0; i < tileData[tileLocation].count; i++)
			{
				uint index = tileData[tileLocation].lightIndex[i];
				finalColor += CalculatePointLight(pointLights[index], position, albedo, normal, specularStrength);
			}
		}
	
		return finalColor;
	}
	
	return albedo;
}

///// FUNCTION DEFINITIONS /////
float4 CalculatePointLight(PointLight light, float4 position, float4 albedo, float4 normal, float4 specularStrength)
{
	
	
    // Attenuation
	float dist = length(light.position - (float3) position);
	float attenuation = smoothstep(light.radius * 2.f, -1.f, dist);
    
    
    // Diffuse
	float4 lightDir = normalize(float4(light.position, 0.f) - float4(position.xyz, 0.f));
	float4 diffuse = max(dot(normal, lightDir), 0.f) * float4(light.color, 1.f) * attenuation * light.strength;
    
 
	return diffuse;
}

float4 CalculateDirectionalLight(float3 DL_color, float DL_strength, float3 DL_direction, float4 albedo, float4 normal)
{
	
	float4 diffuse = max(dot(normal.xyz, normalize(DL_direction)), 0.f) * float4(DL_color, 1.f) * DL_strength;
	return diffuse;
	
}