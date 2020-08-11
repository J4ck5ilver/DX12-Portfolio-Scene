#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "GpuResource.hpp"
#include <stdint.h>

class Texture : public GpuResource
{
private:
	uint32_t m_Width;
	uint32_t m_Height;

	D3D12_SHADER_RESOURCE_VIEW_DESC m_SRV_Desc;

public:
	Texture();
	~Texture();

	inline const uint32_t& GetWidth() const noexcept { return m_Width; }
	inline const uint32_t& GetHeight() const noexcept { return m_Height; }
	inline 	const 	D3D12_SHADER_RESOURCE_VIEW_DESC GetSRVDesc() const noexcept{ return m_SRV_Desc; };

	//temp (should only be accesed by motherloader)
	inline void SetWidth(const uint32_t& width) noexcept { m_Width = width; }
	inline void SetHeight(const uint32_t& height) noexcept { m_Height = height; }
	inline 	void  SetSRVDesc(const D3D12_SHADER_RESOURCE_VIEW_DESC& desc) noexcept { m_SRV_Desc = desc; }


private:


};
#endif // !TEXTURE_HPP