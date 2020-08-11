#include "GpuResource.hpp"
#include "../MotherLoader.hpp"
#include"../../RenderEngine/Renderer.hpp"
#include"../../ObjectManager/ObjectManager.h"

GpuResource::GpuResource(ResourceType resourceType, bool oneResoursPerBackBuffer, UINT offsetInMemory) :
	m_ResourceType(resourceType),
	m_MoreThanOneDxResource(oneResoursPerBackBuffer),
	m_MemoryOffsetIndex(offsetInMemory)

{
	if (m_MoreThanOneDxResource)
	{
		m_NrOfResources = MAX_NR_OF_DX_RESOURCES;
	}

	for (int i = 0; i < m_NrOfResources; i++)
	{
		m_Resources.emplace_back(nullptr);
	}



}

GpuResource::~GpuResource()
{
	if (m_DestroyDXResorceOnDelete)
	{
		RemoveFromGPU();
	}
}



void GpuResource::RemoveFromGPU()
{
	for (int i = 0; i < m_Resources.size(); i++)
	{
		if (m_Resources.at(i) != nullptr)
		{

			m_Resources.at(i)->Release();
		}
	}
	
}


void GpuResource::CreateResource(D3D12_HEAP_TYPE HeapType)
{
	

	UINT cbSizeAligned = (m_DataSize + 511) & ~511;

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = HeapType;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.CreationNodeMask = 1; //used when multi-gpu
	heapProperties.VisibleNodeMask = 1; //used when multi-gpu
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = cbSizeAligned;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;


	for (int i = 0; i < m_NrOfResources; i++)
	{
		if (!SUCCEEDED(Renderer::Get()->GetDevice()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_Resources.at(i)))))
		{
			__debugbreak();
		}
	}
}

void GpuResource::WriteDataOnGpu(void* data, SIZE_T dataSize, UINT offset)
{
	if (m_Data == nullptr)
	{
		m_Data = data;
		m_DataSize = dataSize;
		m_MemoryOffsetIndex = offset;
	}

	UINT currentResource = ConvertIndexToResourceindex(Renderer::Get()->GetCurrentBackBufferIndex());

	if (m_Resources.size() > 0 && m_Resources[currentResource] != nullptr)
	{
		UpdateResourceOnGpu(currentResource);

		if (m_NrOfResources > 1)
		{
			ObjectManager::Get()->AddResourceToUpdateQueue(this, ConvertIndexToResourceindex(currentResource + 1));
		}
	}
}

void GpuResource::UpdateResourceOnGpu(UINT index)
{

	if (m_Resources.size() > index)
	{
		void* mappedMem = nullptr;

		UINT backBufferIndex = Renderer::Get()->GetCurrentBackBufferIndex();

		D3D12_RANGE readRange = { 0, 0 };
		if (SUCCEEDED(m_Resources[index]->Map(0, &readRange, &mappedMem)))
		{
			mappedMem = static_cast<char*>(mappedMem) + m_MemoryOffsetIndex * m_DataSize;

			memcpy(mappedMem, m_Data, m_DataSize);

			D3D12_RANGE writeRange = { m_MemoryOffsetIndex * m_DataSize, m_DataSize };
			m_Resources[index]->Unmap(0, &writeRange);
		}
	}
	
}

unsigned int GpuResource::ConvertIndexToResourceindex(UINT otherIndex)
{
	if (otherIndex >= m_NrOfResources)
	{
		otherIndex = 0;
	}

	return otherIndex;
}