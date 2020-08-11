#ifndef TRIPLELINKEDLIST_HPP
#define TRIPLELINKEDLIST_HPP
#include<cstdint >

//Notes and how to use:
//Save the node to be able to remove the element from the list without searching for it in the list.
//The list will destroy all the elements if it gets deleted. Remove them first if you want to keep them after that.

//To do:
//Make the list a friend class to the node to prevent access to the nodes functions outside of the list.

template<class T>
class TripleLinkedListNode
{
public:		//Variables

private:	//Variables
	T m_Element;
	TripleLinkedListNode<T>* m_PreviousNode;
	TripleLinkedListNode<T>* m_NextNode;

	uint32_t* m_NrOfNodes = nullptr;
	TripleLinkedListNode<T>** m_FirstNode = nullptr;
	TripleLinkedListNode<T>** m_LastNode = nullptr;
	

public:		//Functions
	TripleLinkedListNode(T element, uint32_t* nrOfNodes, TripleLinkedListNode<T>** FirstNode, TripleLinkedListNode<T>** LastNode);

	~TripleLinkedListNode();

	TripleLinkedListNode<T>* GetPreviousNode();
	TripleLinkedListNode<T>* GetNextNode();
	T GetElement();

	void RelinkForwardsWith(TripleLinkedListNode<T>* node);
	void RelinkBackwardsWith(TripleLinkedListNode<T>* node);




private:	//Functions


};

template<class T>
class TripleLinkedList
{
public:		//Variables

protected:  //Variables



private:	//Variables
	uint32_t m_NrOfNodes = 0;
	TripleLinkedListNode<T>* m_FirstNode = nullptr;
	TripleLinkedListNode<T>* m_LastNode = nullptr;

	friend class TripleLinkedListNode<T>;

public:		//Functions
	TripleLinkedList();
	~TripleLinkedList();


	void ClearList(bool deleteElements = false);
	bool DeleteAt(uint32_t pos);
	bool DeleteAtFront();
	bool DeleteAtBack();

	TripleLinkedListNode<T>* InsertAt(T element, uint32_t pos);
	TripleLinkedListNode<T>* InsertAtFront(T element);
	TripleLinkedListNode<T>* InsertAtBack(T element);

	T GetAt(uint32_t pos);
	T GetFirst();
	T GetLast();

	uint32_t GetSize();


private:	//Functions


};



template<class T>
inline TripleLinkedList<T>::TripleLinkedList()
{
	m_NrOfNodes = 0;
	m_FirstNode = nullptr;
	m_LastNode = nullptr;
}


template<class T>
inline TripleLinkedList<T>::~TripleLinkedList()
{
	ClearList(true);
}



template<class T>
inline void TripleLinkedList<T>::ClearList(bool deleteElements)
{
	TripleLinkedListNode<T>* traveler = m_FirstNode;
	TripleLinkedListNode<T>* temp = nullptr;



	uint32_t nrOfIterations = m_NrOfNodes;
	for (uint32_t i = 0; i < nrOfIterations; i++)
	{
		temp = traveler->GetNextNode();
		if (deleteElements == true)
		{
			delete traveler->GetElement();
		}

		delete traveler; 

		traveler = temp;
	}

	m_FirstNode = nullptr;
	m_LastNode = nullptr;
}

template<class T>
inline bool TripleLinkedList<T>::DeleteAt(uint32_t pos)
{
	if (pos < m_NrOfNodes && pos >= 0)
	{
		if (pos == 0)
		{
			return DeleteAtFront();
		}
		else if (pos == m_NrOfNodes - 1)
		{
			return DeleteAtBack();
		}
		else
		{
			TripleLinkedListNode<T>* traveler = m_FirstNode->GetNextNode();
			for (uint32_t i = 1; i < pos; i++)
			{
				traveler = traveler->GetNextNode();
			}

			TripleLinkedListNode<T>* next = traveler->GetNextNode();
			TripleLinkedListNode<T>* Previous = traveler->GetPreviousNode();

			delete traveler;

			Previous->RelinkForwardsWith(next);
			next->RelinkBackwardsWith(Previous);

			//m_NrOfNodes--;
			return true;
		}

	}

	return false;
}

template<class T>
inline bool TripleLinkedList<T>::DeleteAtFront()
{
	if (m_FirstNode != nullptr)
	{
		//m_NrOfNodes--;
		TripleLinkedListNode<T>* temp = m_FirstNode->GetNextNode();
		delete m_FirstNode;
		m_FirstNode = temp;

		if (m_NrOfNodes == 1)
		{
			m_LastNode = m_FirstNode;
		}
		return true;
	}
	return false;
}

template<class T>
inline bool TripleLinkedList<T>::DeleteAtBack()
{
	if (m_LastNode != nullptr)
	{
		//m_NrOfNodes--;
		TripleLinkedListNode<T>* temp = m_LastNode->GetPreviousNode();
		delete m_LastNode;
		m_LastNode = temp;
		m_LastNode->RelinkForwardsWith(nullptr);

		if (m_NrOfNodes == 1)
		{
			m_FirstNode = m_LastNode;
		}
		return true;
	}


	return false;
}

template<class T>
inline TripleLinkedListNode<T>* TripleLinkedList<T>::InsertAt(T element, uint32_t pos)
{

	if (pos <= 0)
	{
		return InsertAtFront(element);
	}
	else if (pos >= m_NrOfNodes)
	{
		return InsertAtBack(element);
	}
	else
	{
		TripleLinkedListNode<T>* traveler = m_FirstNode->GetNextNode();
		for (uint32_t i = 1; i < pos; i++)
		{
			traveler = traveler->GetNextNode();
		}

		TripleLinkedListNode<T>* temp = new TripleLinkedListNode<T>(element, &m_NrOfNodes, m_FirstNode, m_LastNode);

		temp->RelinkBackwardsWith(traveler);
		temp->RelinkBackwardsWith(traveler->GetNextNode());

		traveler->GetNextNode()->RelinkBackwardsWith(temp);
		traveler->RelinkForwardsWith(temp);

		return temp;
	}
}

template<class T>
inline TripleLinkedListNode<T>* TripleLinkedList<T>::InsertAtFront(T element)
{
	m_NrOfNodes++;
	TripleLinkedListNode<T>* temp = new TripleLinkedListNode<T>(element, &m_NrOfNodes, m_FirstNode, m_LastNode);
	temp->RelinkBackwardsWith(nullptr);
	temp->RelinkForwardsWith(m_FirstNode);
	m_FirstNode = temp;

	if (m_NrOfNodes == 1)
	{
		m_LastNode = m_FirstNode;
	}

	return m_FirstNode;
}

template<class T>
inline TripleLinkedListNode<T>* TripleLinkedList<T>::InsertAtBack(T element)
{
	m_NrOfNodes++;
	TripleLinkedListNode<T>* temp = new TripleLinkedListNode<T>(element, &m_NrOfNodes, &m_FirstNode, &m_LastNode);

	temp->RelinkBackwardsWith(m_LastNode);
	temp->RelinkForwardsWith(nullptr);
	if (m_LastNode != nullptr)
	{
		m_LastNode->RelinkForwardsWith(temp);
	}
	m_LastNode = temp;

	if (m_NrOfNodes == 1)
	{
		m_FirstNode = m_LastNode;
	}

	return m_LastNode;
}

template<class T>
inline T TripleLinkedList<T>::GetAt(uint32_t pos)
{
	int test = m_NrOfNodes;

	if (pos <= 0)
	{
		return GetFirst();
	}
	else if (pos >= m_NrOfNodes)
	{
		return GetLast();
	}
	else
	{
		TripleLinkedListNode<T>* traveler = m_FirstNode->GetNextNode();
		for (uint32_t i = 1; i < pos; i++)
		{
			traveler = traveler->GetNextNode();
		}

		return traveler->GetElement();
	}
}

template<class T>
inline T TripleLinkedList<T>::GetFirst()
{
	return m_FirstNode->GetElement();
}

template<class T>
inline T TripleLinkedList<T>::GetLast()
{
	return m_LastNode->GetElement();
}

template<class T>
inline uint32_t TripleLinkedList<T>::GetSize()
{
	return m_NrOfNodes;
}








template<class T>
inline TripleLinkedListNode<T>::TripleLinkedListNode(T element, uint32_t* nrOfNodes, TripleLinkedListNode<T>** firstNode, TripleLinkedListNode<T>** lastNode)
{
	m_Element = element;
	m_NrOfNodes = nrOfNodes;
	m_NextNode = nullptr;
	m_PreviousNode = nullptr;

	m_FirstNode = firstNode;
	m_LastNode = lastNode;

}

template<class T>
inline TripleLinkedListNode<T>::~TripleLinkedListNode()
{
	*m_NrOfNodes = *m_NrOfNodes - 1;

	if (m_PreviousNode != nullptr)
	{
		m_PreviousNode->RelinkForwardsWith(m_NextNode);
	}
	else
	{
		*m_FirstNode = m_NextNode;
	}

	if (m_NextNode != nullptr)
	{
		m_NextNode->RelinkBackwardsWith(m_PreviousNode);
	}
	else
	{
		*m_LastNode = m_PreviousNode;
	}




}

template<class T>
inline TripleLinkedListNode<T>* TripleLinkedListNode<T>::GetPreviousNode()
{
	return m_PreviousNode;
}

template<class T>
inline TripleLinkedListNode<T>* TripleLinkedListNode<T>::GetNextNode()
{
	return m_NextNode;
}

template<class T>
inline T TripleLinkedListNode<T>::GetElement()
{
	return m_Element;
}

template<class T>
inline void TripleLinkedListNode<T>::RelinkForwardsWith(TripleLinkedListNode<T>* node)
{
	m_NextNode = node;
}

template<class T>
inline void TripleLinkedListNode<T>::RelinkBackwardsWith(TripleLinkedListNode<T>* node)
{
	m_PreviousNode = node;
}

#endif // !TRIPLELINKEDLIST_HPP