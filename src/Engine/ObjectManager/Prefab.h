#ifndef PREFAB_H
#define PREFAB_H

#include<string>

#include"PrefabInstance.hpp"
#include"PrefabHeapBlock.hpp"
#include"TripleLinkedList.hpp"

#include"../Assetmanager/AssetFiles/Mesh.hpp"
#include"../Assetmanager/AssetFiles/Material.hpp"

struct RenderDescriptor
{
	enum CullingMethod { BackFaceCulling, FrontFaceCulling, NoCulling };

	//ShaderPiplineState
	bool m_WireFrame = false;
	CullingMethod m_CullingMethod = NoCulling;
};




class Prefab
{
public:
	Prefab(std::string meshName, std::string materialName, unsigned int prefabId) noexcept;
	virtual ~Prefab()noexcept;

	
	PrefabInstance* CreateInstance();
	PrefabHeapBlock* GetPrefabHeapBlock();

	unsigned int GetNrOfInstances();
	unsigned int GetMeshVertexCount();
	

private:

private:
	static unsigned int s_PrefabCounter;
	unsigned int m_PrefabId= 0;
	unsigned int m_HeapIndex = 0;

	std::string m_MeshName;
	std::string m_MaterialName;

	Mesh* m_Mesh;
	Material* m_Material;

	RenderDescriptor m_RenderDescriptor;

	
	TripleLinkedList<PrefabInstance*> m_InstanceList;

	PrefabHeapBlock m_PrefabHeapBlock;


};



#endif // !OBJECTTEMPLATE_H

