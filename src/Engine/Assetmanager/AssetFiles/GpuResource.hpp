#ifndef GPURESOURCE_HPP
#define GPURESOURCE_HPP

#include <d3d12.h>
#include <string>
#include <vector>
//Test class

//Enums for heap mapping
enum  ResourceType {
	Default_Type = -1, //Only for default constructor
	Mesh_Type = 0,
	DiffuseTexture_Type = 1,
	NormalTexture_Type = 2,
	SpecularTexture_Type = 3,
	TransformBuffer_Type = 4,
	PointLight_Type = 5,
	PointLightRead_Type = 6,
	ColorBuffer_Type = 7
};



class GpuResource
{
public:

	union  ViewType
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBV;
		D3D12_SHADER_RESOURCE_VIEW_DESC SRV;
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAV;
	};


private:

	const UINT MAX_NR_OF_DX_RESOURCES = 2;

	
	std::vector<ID3D12Resource*> m_Resources;
	UINT m_NrOfResources = 1;
	UINT m_CurrentDxResourceIndex = 0;
	D3D12_SHADER_RESOURCE_VIEW_DESC m_ViewDesc;
	UINT m_DataSize = 0;
	UINT m_MemoryOffsetIndex = 0; //If the resources belongs to an instance, this value represents the position in the resource array.  
	ResourceType m_ResourceType = Default_Type;
	bool m_MoreThanOneDxResource = true;
	void* m_Data = nullptr;
	ViewType m_Type;
	bool m_DestroyDXResorceOnDelete = true;

	friend class Motherloader;


public:
	GpuResource(ResourceType resourceType = Default_Type, bool oneResoursPerBackBuffer = false, UINT offsetInMemory = 0);
	virtual ~GpuResource();

	const bool& LoadToGPU();
	void RemoveFromGPU();


	inline std::vector<ID3D12Resource*>* GetResources() { return &m_Resources; }
	inline UINT GetDataSize() { return m_DataSize; }

	inline ResourceType GetResourceType() { return m_ResourceType; };
	inline UINT GetMemeoryOffSet() { return m_MemoryOffsetIndex; };
	inline const ViewType& GetViewType() noexcept { return m_Type; }


	 void setResources(std::vector<ID3D12Resource*>* resource, bool destroyDXResourceOnDelete = true) 
	 {
		 m_Resources = *resource; 
		 m_NrOfResources = resource->size();
		 m_DestroyDXResorceOnDelete = destroyDXResourceOnDelete;
	 };
	inline void setDataSize(UINT dataSize) { m_DataSize = dataSize; };
	inline void SetResourceType(ResourceType resourceType) { m_ResourceType = resourceType; }

	inline void SetMemoryOffsetIndex(UINT memoryOffsetIndex) { m_MemoryOffsetIndex = memoryOffsetIndex; }
	inline void SetViewType(const ViewType& view) noexcept { m_Type = view; }
	inline void SetNrOfResoures(const int& nr) { m_NrOfResources = nr; m_Resources.resize(nr); }
	


	void CreateResource(D3D12_HEAP_TYPE HeapType = D3D12_HEAP_TYPE_UPLOAD);
	void WriteDataOnGpu(void* data, SIZE_T dataSize, UINT offset = 0);
	void UpdateResourceOnGpu(UINT index);

	unsigned int ConvertIndexToResourceindex(UINT backBufferIndex);
private:

	

};




#endif // !ASSET_HPP

