#ifndef LIGHTSHEAPBLOCK_HPP
#define LIGHTSHEAPBLOCK_HPP

#include "../Assetmanager/AssetFiles/GpuResource.hpp"

class LightsHeapBlock
{
	struct PointLightResourceBufferDesc : public GpuResource	// 1 per light type
	{
		inline PointLightResourceBufferDesc(ResourceType resourceType) : GpuResource(resourceType, true) {};

		unsigned int MaxNumberOfElements = 6100;
		unsigned int NumberOfElements = 0;
	};

public:
	LightsHeapBlock() = default;
	virtual ~LightsHeapBlock();

	LightsHeapBlock(const LightsHeapBlock& obj) = delete;
	LightsHeapBlock operator=(const LightsHeapBlock& obj) = delete;

	void AddPointLightResource(GpuResource* resource, size_t viewIndex, size_t lightIndex);
	void CreateShaderResourceView(GpuResource* resource, size_t viewIndex);

	inline PointLightResourceBufferDesc* GetPointLightresource() {return m_PointLightsResourceDesc;}
	inline GpuResource& GetCopyLightresource() {return m_LightReadResource;}

private:
	PointLightResourceBufferDesc* m_PointLightsResourceDesc = nullptr;
	GpuResource m_LightReadResource;
};
#endif // !LIGHTSHEAPBLOCK_HPP