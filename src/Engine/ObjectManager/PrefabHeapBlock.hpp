#ifndef PREFABHEAPBLOCK_H
#define PREFABHEAPBLOCK_H

#include"PrefabInstance.hpp"
#include"../Assetmanager/AssetFiles/GpuResource.hpp"

#include <unordered_map>
#include <string>
#include <cstdint>
#include <d3d12.h>

// This class is for keeping track of the resources that is unique for each instance.
// It's only for mapping the memory while upploading the data to the gpu is done elsewhere.

class PrefabHeapBlock
{
	struct ResourceBufferDesc : public GpuResource
	{
		inline ResourceBufferDesc(ResourceType resourceType) : GpuResource(resourceType, true) {};

		unsigned int maxNrOfInstances = 0; //Based on the size increment
		unsigned int nrOfInstances = 0;

		unsigned int dataSizeOfOneBufferEntry = 0;
	};

	struct ResourceTypeHandler
	{
		std::vector<ResourceBufferDesc*> resourceBuffers;

		virtual~ResourceTypeHandler()
		{
			for (int i = 0; i < resourceBuffers.size(); i++)
			{
				delete resourceBuffers.at(i);
			}
		}
	};

public:
	PrefabHeapBlock();
	virtual ~PrefabHeapBlock();


	void AddInstance(PrefabInstance* instance);
	void RemoveInstance(unsigned int instanceArrayIndex, unsigned int instanceBufferIndex);

	void CreateShaderResourceView(GpuResource* resource, unsigned int prefabIndex, unsigned int viewIndex);

private:
	void AddInstanceResource(GpuResource* resource, unsigned int prefabIndex);

	unsigned int GetBufferIndexToWriteTo();
	unsigned int GetBufferIndexToWriteTo(unsigned int instanceIndex);
	unsigned int ConvertInstanceToBufferIndex(unsigned int index);

private:
	const unsigned int NR_OF_VIEW_PER_PREFAB = 10;

	std::vector<PrefabInstance*> m_Instances;

	std::unordered_map<ResourceType, ResourceTypeHandler> m_ResourceMap;


	std::vector<unsigned int> m_BufferSizeIncrement{ 100,1000,5000 };
	unsigned int m_NrOfInstances = 0;

	unsigned int m_MaxNrOfInstances = 0;
};




#endif // !PREFABHEAPBLOCK_H