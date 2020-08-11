///// INPUT /////
struct Vertex
{
	float4 position;
	float4 normal;
	float4 uv;
	float4x3 TBN;
	
};
StructuredBuffer<Vertex> vertex : register(t0);

StructuredBuffer<float4x4> transform0 : register(t4); //Size = 100 
StructuredBuffer<float4x4> transform1 : register(t5); //Size = 1000 
StructuredBuffer<float4x4> transform2 : register(t6); //Size = 5000

StructuredBuffer<float4> color0 : register(t7); //Size = 100 
StructuredBuffer<float4> color1 : register(t8); //Size = 1000
StructuredBuffer<float4> color2 : register(t9); //Size = 5000

static const uint sizeForBuffers[3] = { 100, 1000, 5000 };
static const uint nrOfBuffers = 3;

cbuffer Camera : register(b1)
{
	row_major float4x4 viewProjection;
}

static const float4 TriPositions[6] =
{    
    // Tri 1
	-0.5f, -0.5f, 0.f, 1.f,
    0.f, 0.5f, 0.f, 1.f,
    0.5f, -0.5f, 0.f, 1.f,
    
    // Tri 2
    -1.f, 1.f, 0.f, 1.f,
    1.f, 1.f, 0.f, 1.f,
    1.f, -1.f, 0.f, 1.f
};
    
static const float2 UVs[6] =
{
    // Tri 1
	0.f, 1.f,
    0.f, 0.f,
    1.f, 1.f,
    
    // Tri 2
    0.f, 0.f,
    1.f, 0.f,
    1.f, 1.f
};

static const float4 VertexColors[6] =
{
    // Tri 1
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
    
    //Tri 2
	0.0f, 0.0f, 1.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 1.0f 
};

///// OUTPUT /////
struct VSOut
{
	float4 position : SV_Position;
	float4 worldPosition : POSITION;
	float4 normal : NORMAL;
	float2 uv : UV;
	float4 color : COLOR;
	float3x3 tbn : TBN;
};

uint GetBufferIndex(uint instanceID)
{
	uint numberToAdd = 0;
	
	for (uint i = 0; i < nrOfBuffers; i++)
	{
		if (instanceID < (sizeForBuffers[i] + numberToAdd))
		{
			return i;
		}
		numberToAdd += sizeForBuffers[i];
	}
	return 0;
}

uint GetIndexInBuffer(uint instanceID)
{
	uint numberToRemove = 0;

	for (uint i = 0; i < nrOfBuffers; i++)
	{
		if (instanceID < (sizeForBuffers[i] + numberToRemove))
		{
			return instanceID - numberToRemove;

		}
		numberToRemove += sizeForBuffers[i];
	}
	return 0;
}

float4x4 GetTransform(uint bufferIndex, uint indexInBuffer)
{
	switch (bufferIndex)
	{
		case 0:
			return transform0[indexInBuffer];
		case 1:
			return transform1[indexInBuffer];
		case 2:
			return transform2[indexInBuffer];
	}
	float4x4 defaultValue = float4x4(1.f, 0.f, 0.f, 0.f, 
                                0.f, 1.f, 0.f, 0.f,
                                0.f, 0.f, 1.f, 0.f,
                                0.f, 0.f, 0.f, 1.f);
	return defaultValue;
}

float4 GetColor(uint bufferIndex, uint indexInBuffer)
{
	switch (bufferIndex)
	{
		case 0:
			return color0[indexInBuffer];
		case 1:
			return color1[indexInBuffer];
		case 2:
			return color2[indexInBuffer];
	}
	float4 defaultValue;
	return defaultValue;
}

///// MAIN /////
VSOut main(uint vertexIndex : SV_VertexID, uint instanceIndex : SV_InstanceID)
{
	VSOut output = (VSOut) 0;

	uint bufferIndex = GetBufferIndex(instanceIndex);
	uint indexInBuffer = GetIndexInBuffer(instanceIndex);
	float4x4 transform = GetTransform(bufferIndex, indexInBuffer);
	float4 color = GetColor(bufferIndex, indexInBuffer);
	
	transform = transpose(transform);
	matrix MVP = mul(transform, viewProjection);
	
	float3x4 tempTBN= transpose(vertex[vertexIndex].TBN);
	
	float4 T = normalize(mul(tempTBN[0], transform));
	float4 B = normalize(mul(tempTBN[1], transform));
	float4 N = normalize(mul(tempTBN[2], transform));
	
	output.tbn._11_12_13 = T.xyz;
	output.tbn._21_22_23 = B.xyz;
	output.tbn._31_32_33 = N.xyz;
	
	output.position = mul(vertex[vertexIndex].position, MVP);
	output.worldPosition = mul(vertex[vertexIndex].position, transform);
	output.normal = mul(normalize(vertex[vertexIndex].normal), transform);
	output.uv = vertex[vertexIndex].uv;
	output.color = color;

	return output;
}
