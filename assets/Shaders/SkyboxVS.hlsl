///// INPUT /////
struct Vertex
{
	float4 position;
	float4 normal;
	float4 uv;
};
cbuffer viewProj : register(b1)
{
	row_major float4x4 viewProj;
}

cbuffer view : register(b3)
{
	row_major float4x4 view;
}

cbuffer projection : register(b4)
{
	row_major float4x4 projection;
}


static const float3 skyboxVertices[36] =
{
    // positions          
	-1.0f, 1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
     1.0f, 1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
     1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,

    -1.0f, 1.0f, -1.0f,
     1.0f, 1.0f, -1.0f,
     1.0f, 1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
     1.0f, -1.0f, 1.0f
};
    


///// OUTPUT /////
struct VSOut
{
	float4 position : SV_Position;
	float3 texCoords : TEXCOORDS;
};



///// MAIN /////
VSOut main(uint vertexIndex : SV_VertexID, uint instanceIndex : SV_InstanceID)
{
	VSOut output = (VSOut) 0;

	output.texCoords = skyboxVertices[vertexIndex];
    
	float4x4 viewTemp = view;
    
	viewTemp[0, 3] = 0;
	viewTemp[1, 3] = 0;
	viewTemp[2, 3] = 0;
  

    
	output.position = mul(float4(skyboxVertices[vertexIndex].xyz, 1.0), mul(viewTemp, projection));
	//output.position = mul(float4(skyboxVertices[vertexIndex].xyz, 1.0), viewProj);
	output.position = output.position.xyww; //So it is always drawn as furthest away.
	return output;
}
