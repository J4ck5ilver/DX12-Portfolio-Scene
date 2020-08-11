#include "PrefabHeapBlock.hpp"
#include "../RenderEngine/Renderer.hpp"

PrefabHeapBlock::PrefabHeapBlock()
{
	int sum = 0;

	for (int i = 0; i < m_BufferSizeIncrement.size(); i++)
	{
		sum += m_BufferSizeIncrement.at(i);
	}
	
	m_Instances.resize(sum);
	m_MaxNrOfInstances = sum;
}

PrefabHeapBlock::~PrefabHeapBlock()
{

}

void PrefabHeapBlock::AddInstance(PrefabInstance* instance)
{
	if (m_NrOfInstances < m_MaxNrOfInstances)
	{
		//Color
		AddInstanceResource(instance->GetColor(), instance->GetHeapIndex());
		//Transform
		AddInstanceResource(instance->GetTransform(), instance->GetHeapIndex());


		//Tell the instance which index it has in the arrays.
		instance->SetIndexInInstanceBuffer(ConvertInstanceToBufferIndex(m_NrOfInstances));
		instance->SetInstanceBufferIndex(GetBufferIndexToWriteTo(m_NrOfInstances));
		m_Instances[m_NrOfInstances] = instance;

		m_NrOfInstances++;

		
	}
	else
	{
		//Error
	}

}

//Removes the instance and all its resources from the mapping.
void PrefabHeapBlock::RemoveInstance(unsigned int instanceArrayIndex, unsigned int instanceBufferIndex)
{

	//If there is a hole in the buffers, move the last element to the hole.
	unsigned int lastIndex = *&m_ResourceMap.begin()->second.resourceBuffers.at(instanceBufferIndex)->nrOfInstances - 1;
	unsigned int currentIndex = instanceArrayIndex;
	//Convert the index of the last element from local to global
	for (int i = 0; i < instanceBufferIndex; i++)
	{
		lastIndex += m_BufferSizeIncrement.at(i);
		currentIndex += m_BufferSizeIncrement.at(i);
	}


	if (currentIndex != lastIndex)
	{


		m_Instances[currentIndex] = m_Instances[lastIndex];
		m_Instances[currentIndex]->SetIndexInInstanceBuffer(instanceArrayIndex);

		//Move the data on gpu

		//Color
		Color* color = m_Instances[currentIndex]->GetColor();

		color->WriteDataOnGpu(
			&*color->GetColor(),
			color->GetDataSize(),
			color->GetDataSize() * m_Instances[currentIndex]->GetIndexInInstanceBuffer());

		//Transform
		Transform* transform = m_Instances[currentIndex]->GetTransform();

		//auto t = transform->GetTransform();

		transform->WriteDataOnGpu(
		(void*)&transform->GetTransform(),
			transform->GetDataSize(),
			transform->GetDataSize() * m_Instances[currentIndex]->GetIndexInInstanceBuffer());

	}
	m_Instances.at(lastIndex) = nullptr;


	//Delete all the resorces for that instance
	for (auto& resourceIt : m_ResourceMap)
	{
		resourceIt.second.resourceBuffers.at(instanceBufferIndex)->nrOfInstances--;
	}

	m_NrOfInstances--;
}

//Adds an gpuResorce to its instance array resource. Creates the array resorce if it does not already exist.
void PrefabHeapBlock::AddInstanceResource(GpuResource* resource, unsigned int prefabIndex)
{
	unsigned int bufferToWriteTo = GetBufferIndexToWriteTo(m_NrOfInstances);
	ResourceBufferDesc* resourceBufferDesc = nullptr;


	// Check if this type of resorce has already been registered.
	if (m_ResourceMap.find(resource->GetResourceType()) == m_ResourceMap.end())
	{
		m_ResourceMap.emplace(resource->GetResourceType(), ResourceTypeHandler());
	}

	// Check if the buffer has been created or not
	if (m_ResourceMap.at(resource->GetResourceType()).resourceBuffers.size() <= bufferToWriteTo)
	{
		m_ResourceMap.at(resource->GetResourceType()).resourceBuffers.emplace_back(new ResourceBufferDesc(resource->GetResourceType()));

		resourceBufferDesc = m_ResourceMap.at(resource->GetResourceType()).resourceBuffers.back();

		// The ResourceBufferDesc needs to be filled with the right information.
		unsigned int sizeIncrementIndex = (unsigned int)m_ResourceMap.at(resource->GetResourceType()).resourceBuffers.size() - 1;
		resourceBufferDesc->maxNrOfInstances = m_BufferSizeIncrement[sizeIncrementIndex];
		resourceBufferDesc->setDataSize(resourceBufferDesc->maxNrOfInstances * resource->GetDataSize());
		resourceBufferDesc->dataSizeOfOneBufferEntry = resource->GetDataSize();

		resourceBufferDesc->CreateResource();

		// Fix the view drescriptor
		GpuResource::ViewType tempType = resource->GetViewType();
		tempType.SRV.Buffer.StructureByteStride = ((resourceBufferDesc->GetDataSize() + 511) & ~511);
		tempType.SRV.Buffer.NumElements = 1;
		resourceBufferDesc->SetViewType(tempType);

		// CreateView
		CreateShaderResourceView(resourceBufferDesc, prefabIndex, resource->GetResourceType() + bufferToWriteTo);
	}

	ResourceTypeHandler* resourceTypeHandler = &m_ResourceMap.at(resource->GetResourceType());
	resourceBufferDesc = resourceTypeHandler->resourceBuffers[bufferToWriteTo];

	resourceBufferDesc->nrOfInstances++;



	//As all the instances resources are collected to a buffer, they share the resource.
	resource->setResources(resourceBufferDesc->GetResources(), false);
}




//Creates a view and maps a resource to it.
void PrefabHeapBlock::CreateShaderResourceView(GpuResource* resource, unsigned int prefabIndex, unsigned int viewIndex)
{
	std::vector<ID3D12Resource*>* dxResources = resource->GetResources();

	if (dxResources != nullptr)
	{
		UINT sizeOfView = Renderer::Get()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		UINT offset = Renderer::Get()->GetHeapStride(PreFabs) * prefabIndex + sizeOfView * viewIndex;

		for (int i = 0; i < Renderer::NUMBER_OF_BACK_BUFFERS; i++)
		{

			D3D12_CPU_DESCRIPTOR_HANDLE cdh = Renderer::Get()->GetDescriptorHeaps(PreFabs)[i]->GetCPUDescriptorHandleForHeapStart();

			cdh.ptr += offset;

			int resourceIndex = resource->ConvertIndexToResourceindex(i);

			Renderer::Get()->GetDevice()->CreateShaderResourceView(dxResources->at(resourceIndex), &resource->GetViewType().SRV, cdh);

		}
	}


}


//Gets the current buffer index that should be written to.
unsigned int PrefabHeapBlock::GetBufferIndexToWriteTo()
{
	//The ResourceTypeHandlers are mapped 1 to 1 so just take the first one
	ResourceTypeHandler* resourceTypeHandler = &m_ResourceMap.begin()->second;


	for (int i = 0; i < resourceTypeHandler->resourceBuffers.size(); i++)
	{
		if (resourceTypeHandler->resourceBuffers.at(i)->nrOfInstances < resourceTypeHandler->resourceBuffers.at(i)->maxNrOfInstances)
		{
			return i;
		}
	}


	return 0;
}

//Gets the buffer index based on an instance index.
unsigned int PrefabHeapBlock::GetBufferIndexToWriteTo(unsigned int instanceIndex)
{
	int numberToAdd = 0;

	for (int i = 0; i < m_BufferSizeIncrement.size(); i++)
	{
		if (instanceIndex < m_BufferSizeIncrement[i] + numberToAdd)
		{
			return i;
		}
		numberToAdd += m_BufferSizeIncrement[i];
	}

	return 0;
}

//As the instances are divided into different buffers with increasing sizes, the index needs to be converted from "global index" to "local index".
unsigned int PrefabHeapBlock::ConvertInstanceToBufferIndex(unsigned int index)
{
	int numberToRemove = 0;

	for (int i = 0; i < m_BufferSizeIncrement.size(); i++)
	{
		if (index < m_BufferSizeIncrement[i] + numberToRemove)
		{
			return index - numberToRemove;
		}
		numberToRemove += m_BufferSizeIncrement[i];
	}
	return 0;
}
