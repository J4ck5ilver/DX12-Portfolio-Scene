#include "LightsHeapBlock.hpp"
#include "../RenderEngine/Renderer.hpp"

LightsHeapBlock::~LightsHeapBlock()
{
	delete m_PointLightsResourceDesc;
	m_PointLightsResourceDesc = nullptr;
}

void LightsHeapBlock::AddPointLightResource(GpuResource* resource, size_t viewIndex, size_t lightIndex)
{
	// If buffer has not been created --> create resource and view
	if (m_PointLightsResourceDesc == nullptr)
	{
		// Create buffer
		m_PointLightsResourceDesc = new PointLightResourceBufferDesc(resource->GetResourceType());

		// Define data size
		m_PointLightsResourceDesc->setDataSize(m_PointLightsResourceDesc->MaxNumberOfElements * resource->GetDataSize());

		// Create resource
		m_PointLightsResourceDesc->CreateResource();

		//Create Copy Source // (Copy to from compute / read from second pass)
		m_LightReadResource.SetNrOfResoures(2);
		m_LightReadResource.setDataSize(m_PointLightsResourceDesc->MaxNumberOfElements * resource->GetDataSize());
		m_LightReadResource.CreateResource(D3D12_HEAP_TYPE_DEFAULT);

		// Define the view drescriptor
		GpuResource::ViewType viewType = resource->GetViewType();
		viewType.SRV.Buffer.StructureByteStride = ((m_PointLightsResourceDesc->GetDataSize() + 511) & ~511);
		viewType.SRV.Buffer.NumElements = 1;
		viewType.SRV.Buffer.FirstElement = 0;
		viewType.SRV.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		viewType.SRV.Format = DXGI_FORMAT_UNKNOWN;
		viewType.SRV.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		viewType.SRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		m_PointLightsResourceDesc->SetViewType(viewType);
		m_LightReadResource.SetViewType(viewType);

		// Create view
		CreateShaderResourceView(m_PointLightsResourceDesc, 6); //(Ful kodat jag vet)
		// Create View for Copy src
		CreateShaderResourceView(&m_LightReadResource, 5);
	}

	// Check capacity before connecting more resources
	if (m_PointLightsResourceDesc->NumberOfElements < m_PointLightsResourceDesc->MaxNumberOfElements)
	{
		// Connect the DX buffer resource to the light DX resource (input)
		resource->setResources(m_PointLightsResourceDesc->GetResources(), false);

		resource->SetMemoryOffsetIndex(lightIndex);

		m_PointLightsResourceDesc->NumberOfElements++;
	}
}

void LightsHeapBlock::CreateShaderResourceView(GpuResource* resource, size_t viewIndex)
{
	std::vector<ID3D12Resource*>* dxResources = resource->GetResources();

	if (dxResources != nullptr)
	{
		UINT sizeOfView = Renderer::Get()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	
		UINT subHeapSize = Renderer::Get()->GetNumberOfViewsPerSubHeap();
		UINT viewPosition = (viewIndex - 1) * sizeOfView;

		for (int i = 0; i < Renderer::NUMBER_OF_BACK_BUFFERS; i++)
		{
			// Descriptor Heap start (main heap)
			D3D12_CPU_DESCRIPTOR_HANDLE cdh = Renderer::Get()->GetDescriptorHeaps(DeferredSP)[i]->GetCPUDescriptorHandleForHeapStart();

			cdh.ptr += (subHeapSize * DeferredSP) * sizeOfView + viewPosition;

			Renderer::Get()->GetDevice()->CreateShaderResourceView(dxResources->at(i), &resource->GetViewType().SRV, cdh);
		}
	}
}