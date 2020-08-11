#include "PrefabInstance.hpp"
#include "ObjectManager.h"
PrefabInstance::PrefabInstance(std::string& meshName, std::string& materialName, unsigned int& prefabId, unsigned int& heapIndex)
{
	m_MeshName = &meshName;
	m_MaterialName = &materialName;
	m_PrefabId = &prefabId;
	m_HeapIndex = &heapIndex;


}

PrefabInstance::~PrefabInstance()
{
}

void PrefabInstance::Destroy()
{
	delete m_ListNode;
	ObjectManager::Get()->GetPrefab(*m_PrefabId)->GetPrefabHeapBlock()->RemoveInstance(m_IndexInInstanceBuffer, m_InstanceBufferIndex);
	delete this;
}

void PrefabInstance::SetLinkNode(TripleLinkedListNode<PrefabInstance*>* node)
{
	if (m_ListNode == nullptr)
	{
		m_ListNode = node;

	}
}

Color* PrefabInstance::GetColor()
{
	return &m_Color;
}

Transform * PrefabInstance::GetTransform()
{
	return &m_Transform;
}

unsigned int PrefabInstance::GetIndexInInstanceBuffer()
{
	return m_IndexInInstanceBuffer;
}

unsigned int PrefabInstance::GetInstanceBufferIndex()
{
	return m_InstanceBufferIndex;
}

unsigned int PrefabInstance::GetHeapIndex()
{
	return *m_HeapIndex;
}

void PrefabInstance::SetIndexInInstanceBuffer(unsigned int index)
{
	m_IndexInInstanceBuffer = index;
	m_Color.SetMemoryOffsetIndex(m_IndexInInstanceBuffer);
	m_Transform.SetMemoryOffsetIndex(m_IndexInInstanceBuffer);
}

void PrefabInstance::SetInstanceBufferIndex(unsigned int index)
{
	m_InstanceBufferIndex = index;
}




