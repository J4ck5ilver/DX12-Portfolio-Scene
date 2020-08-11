#ifndef PREFABINSTANCE_H
#define PREFABINSTANCE_H
#include"TripleLinkedList.hpp"
#include"Color.hpp"
#include"Transform.hpp"

#include<DirectXMath.h>
#include<string>

using namespace DirectX;

class PrefabInstance
{
public:

	PrefabInstance(std::string& meshName, std::string& materialName, unsigned int& prefabId, unsigned int& heapIndex);
	virtual ~PrefabInstance();
	
	void Destroy();
	void SetLinkNode(TripleLinkedListNode<PrefabInstance*>* node);
	
	Color* GetColor();
	Transform* GetTransform();
	unsigned int GetIndexInInstanceBuffer();
	unsigned int GetInstanceBufferIndex();
	unsigned int GetHeapIndex();

	void SetIndexInInstanceBuffer(unsigned int index);
	void SetInstanceBufferIndex(unsigned int index);

private:
	/*The index in the recource arrays for this instance.
	If the instane is moved in gpu memory, this needs to be updated.*/
	unsigned int m_IndexInInstanceBuffer = 0;
	unsigned int m_InstanceBufferIndex = 0;
	unsigned int* m_PrefabId;
	unsigned int* m_HeapIndex;
	TripleLinkedListNode<PrefabInstance*>* m_ListNode = nullptr;

	std::string* m_MeshName;
	std::string* m_MaterialName;

	Transform m_Transform;
	Color m_Color;



};

#endif // !PREFABINSTANCE_H
