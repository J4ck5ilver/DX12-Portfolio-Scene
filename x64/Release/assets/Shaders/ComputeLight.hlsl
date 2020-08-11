/*

1. Find out the position on the screen that corresponds to the tile in location "dispatchThreadID"
2. Specify its width and height
3. Loop through all the lights
4. Calculate light position and radius into screen space
5. Check if light intersects with the tile in location "dispatchThreadID"
6. If yes, check the number of lights the tile is carrying
7. If count < max amount of lights --> insert light in tile
8. If count == max amount of lights --> see if the current light is bigger than the smallest light in the array
9. If there is a smaller light in the container --> replace it by the current light

How to find a tile's number depending on their dispatch x and y values:
Tile = x + (y * GridWidth)  (count goes from top-left to bottom-right)
... or use SV_GroupIndex

*/

///// INPUT /////
struct PointLight
{
    float3 color;
    float strength;
    float3 position;
    float radius;
};
StructuredBuffer<PointLight> pointLights : register(t15);

cbuffer Camera : register(b0)
{
    float4 camPosition;
}

cbuffer Camera : register(b1)
{
    row_major float4x4 viewProjection;
}

cbuffer LightCount : register(b2)
{
    unsigned int numberOfPointLights;
}

///// OUTPUT /////
#ifndef LIGHT_CULLING_VALUES
#define LIGHT_CULLING_VALUES
#define MAX_LIGHT 500
#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900
//#define SCREEN_WIDTH 1280
//#define SCREEN_HEIGHT 720
#define GRID_SIZE 32
#define TILE_WIDTH SCREEN_WIDTH / GRID_SIZE
#define TILE_HEIGHT SCREEN_HEIGHT / GRID_SIZE
#endif

struct TileData
{
    uint count;                  // How many lights there are in the array
    uint lightIndex[MAX_LIGHT];  // Container of the corresponding light indeces (their position in the big light array)
};
RWStructuredBuffer<TileData> tileData : register(u0); //Size = 1024

///// FUNCTION DECLARATIONS /////
float4 ConvertLightToNDC(PointLight light); // Light position and radius
bool CheckCollision(uint3 groupThreadID, float4 light);

///// MAIN /////
[numthreads(GRID_SIZE, GRID_SIZE, 1)] //Size = 1024
void main(uint groupIndex : SV_GroupIndex, uint3 groupThreadID : SV_GroupThreadID)
{
    // Tile number    
    uint tileNumber = groupIndex;
    tileData[tileNumber].count = 0;      

    // Iterate through every light
    float4 light;
    for (int i = 0; i < numberOfPointLights; i++)
    {
        // Convert position and radius into screen space
        light = ConvertLightToNDC(pointLights[i]);
        
        // Check if light intersects with a tile
        if (CheckCollision(groupThreadID, light))
        {
            // If tile count hasn't reached its maximum capacity --> add light
            if (tileData[tileNumber].count < MAX_LIGHT)
            {
                tileData[tileNumber].lightIndex[tileData[tileNumber].count++] = i;
            }
        }
    }
}

///// FUNCTION DEFINITIONS /////
float4 ConvertLightToNDC(PointLight light)
{  
    float4 NDC = mul(float4(light.position, 1.f), viewProjection);
    NDC = NDC / NDC.z;
    NDC.y *= -1;

    //// Converting from NDC to pixel
    int x = (int) round((NDC.x + 1.0f) * SCREEN_WIDTH / 2.0f);
    int y = (int) round((NDC.y + 1.0f) * SCREEN_HEIGHT / 2.0f);
    
    // Calculate radius and repeat process
    float4 NDC2 = mul(float4(light.position.x, light.position.y + light.radius, light.position.z, 1.f), viewProjection);
    NDC2 = NDC2 / NDC2.z;
    NDC2.y *= -1;

    int lightSphereSurfaceX = (int) round((NDC2.x + 1.0f) * SCREEN_WIDTH / 2.0f);
    int lightSphereSurfaceY = (int) round((NDC2.y + 1.0f) * SCREEN_HEIGHT / 2.0f);

    float radius = length(float2(x, y) - float2(lightSphereSurfaceX, lightSphereSurfaceY));
    radius * 4;
    
    return float4(x, y, 0.f, radius);
}

bool CheckCollision(uint3 groupThreadID, float4 light)
{
    // Define the tile's sides in pixels on the screen
    float minX = groupThreadID.x * TILE_WIDTH;
    float maxX = minX + TILE_WIDTH;
    float minY = groupThreadID.y * TILE_HEIGHT;
    float maxY = minY + TILE_HEIGHT;

    // Get center point circle first 
    float2 center = float2(light.x, light.y);
		
	// Calculate AABB info (center, half-extents)
    float2 aabb_half_extents = float2(float(TILE_WIDTH / 2.f), float(TILE_HEIGHT / 2.f));
    
    float2 aabb_center = float2(minX + aabb_half_extents.x, minY + aabb_half_extents.y);

	// Get difference vector between both centers
    float2 difference = center - aabb_center;
    float2 clamped = clamp(difference, -aabb_half_extents, aabb_half_extents); // A clamp operation clamps a value to a value within a given range

	// Add clamped value to AABB_center and we get the value of box closest to circle
    float2 closest = aabb_center + clamped;

	// Retrieve vector between center circle and closest point AABB and check if length <= radius
    difference = abs(closest - center);

    return length(difference) < light.w * 2;
}