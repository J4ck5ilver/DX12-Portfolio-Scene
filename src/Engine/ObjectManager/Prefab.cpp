#include "Prefab.h"
#include"../Assetmanager/Assetmanager.hpp"
unsigned int Prefab::s_PrefabCounter = 0;

Prefab::Prefab(std::string meshName, std::string materialName, unsigned int prefabId) noexcept
{
	m_HeapIndex = s_PrefabCounter++;
	
	m_PrefabId= prefabId;
	m_MeshName = meshName;
	m_MaterialName = materialName;

	//Get the resorces
	m_Mesh =  Assetmanager::Get()->GetMesh(meshName);
	m_Material = Assetmanager::Get()->GetMaterial(materialName);

	//Create the views for the discriptor heap
	if (m_Mesh)
	{
		m_PrefabHeapBlock.CreateShaderResourceView(m_Mesh,m_HeapIndex, m_Mesh->GetResourceType());
	}
	
	//Do the same for the texures
	if (m_Material)
	{
		if (m_Material->Diffuse)
		{
			m_PrefabHeapBlock.CreateShaderResourceView(m_Material->Diffuse, m_HeapIndex, m_Material->Diffuse->GetResourceType());
		}

		if (m_Material->NormalMap)
		{
			m_PrefabHeapBlock.CreateShaderResourceView(m_Material->NormalMap, m_HeapIndex, m_Material->NormalMap->GetResourceType());
		}

		if (m_Material->SpecularMap)
		{
			m_PrefabHeapBlock.CreateShaderResourceView(m_Material->SpecularMap, m_HeapIndex, m_Material->SpecularMap->GetResourceType());
		}
	}
	
}

Prefab::~Prefab() noexcept
{
	
}

PrefabInstance * Prefab::CreateInstance()
{

	TripleLinkedListNode<PrefabInstance*>* TempNode = m_InstanceList.InsertAtBack(new PrefabInstance(m_MeshName, m_MaterialName, m_PrefabId, m_HeapIndex));
	TempNode->GetElement()->SetLinkNode(TempNode);

	m_PrefabHeapBlock.AddInstance(TempNode->GetElement());

	return TempNode->GetElement();

}

PrefabHeapBlock * Prefab::GetPrefabHeapBlock()
{
	return &m_PrefabHeapBlock;
}

unsigned int Prefab::GetNrOfInstances()
{
	return  m_InstanceList.GetSize();
}

unsigned int Prefab::GetMeshVertexCount()
{
	return m_Mesh->GetVertexCount();
}
