static const float4 TriPositions[6] =
{
    // Tri 1
    -1.0f, -1.0f, 0.0f, 1.0f,
    -1.0f,  1.0f, 0.0f, 1.0f,
     1.0f, -1.0f, 0.0f, 1.0f,
    
    // Tri 2
    -1.0f,  1.0f, 0.0f, 1.0f,
     1.0f,  1.0f, 0.0f, 1.0f,
     1.0f, -1.0f, 0.0f, 1.0f
};
    
static const float2 UVs[6] =
{
    // Tri 1
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 1.0f,
    
    // Tri 2
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f
};

///// OUTPUT /////
struct VSOut
{
    float4 triPosition : SV_Position;
    float2 uv : UV;
};

///// MAIN /////
VSOut main(uint index : SV_VertexID)
{
    VSOut output = (VSOut) 0;
    
    output.triPosition = TriPositions[index];
    output.uv = UVs[index];
    
    return output;
}