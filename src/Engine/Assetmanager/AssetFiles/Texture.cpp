#include "Texture.hpp"
#include<DirectX12/WICTextureLoader.h>

Texture::Texture() : GpuResource(Default_Type,false),
	m_Height(0), 
	m_Width(0),
	m_SRV_Desc()
{
}

Texture::~Texture()
{
}
