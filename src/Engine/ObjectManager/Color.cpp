#include "Color.hpp"

Color::Color(float r, float g, float b, float a) : GpuResource(ColorBuffer_Type)
{
	SetColor(r, g, b, a);
	



	UINT sizeAligned = (sizeof(m_Color) + 511) & ~511;	// 512-byte aligned.



	D3D12_BUFFER_SRV Buffsrv = {};
	Buffsrv.NumElements = 1;
	Buffsrv.FirstElement = 0;
	Buffsrv.StructureByteStride = sizeAligned;
	Buffsrv.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer = Buffsrv;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;



	GpuResource::ViewType temp;

	temp.SRV = srvDesc;

	SetViewType(temp);

	
}

void Color::SetColor(float r, float g, float b, float a)
{
	m_Color.x = r;
	m_Color.y = g;
	m_Color.z = b;
	m_Color.w = a;

	WriteDataOnGpu(&m_Color, sizeof(m_Color), GetMemeoryOffSet());
}

XMFLOAT4 * Color::GetColor()
{
	return &m_Color;
}

void Color::operator=(Color& other)
{
	SetColor(
		other.m_Color.x,
		other.m_Color.y,
		other.m_Color.z,
		other.m_Color.w);
}
