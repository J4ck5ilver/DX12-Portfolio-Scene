#include "ObjectManager.h"
#include <iostream>

ObjectManager* ObjectManager::s_Instance = nullptr;
unsigned int ObjectManager::s_PrefabIdCounter = 0;

ObjectManager* ObjectManager::Get() noexcept
{
	if (!s_Instance)
	{
		s_Instance = new ObjectManager();
	}
	return s_Instance;
}

ObjectManager::ObjectManager() noexcept
{
	m_Prefabs.reserve(10);
	m_Threads.resize(m_NrOfThreads);
	m_ThreadWorkBools = new bool[m_NrOfThreads];
	m_ResourcesToUpdate = new std::queue<ResourceUpdateTicket>[m_NrOfThreads];

	for (int i = 0; i < m_NrOfThreads; i++)
	{
		m_ThreadWorkBools[i] = false;
		m_Threads.at(i) = new std::thread(RunThread, std::ref(m_ResourcesToUpdate[i]), std::ref(m_ThreadWorkBools[i]), std::ref(m_ShutdownThreads));
		m_Threads[i]->detach();
	}
}


ObjectManager::~ObjectManager() noexcept
{
	m_ShutdownThreads = true;

	for (int i = 0; i < m_Prefabs.size(); i++)
	{
		delete m_Prefabs[i];
	}

	for (int i = 0; i < m_NrOfThreads; i++)
	{
		delete m_Threads.at(i);
		m_Threads.at(i) = nullptr;
	}
	delete[] m_ThreadWorkBools;
	delete[] m_ResourcesToUpdate;
}

void ObjectManager::RunThread(std::queue<ResourceUpdateTicket>& queue, bool& work, bool& shutDown)
{
	while (!shutDown)
	{
		if (work == true)
		{
			while (queue.size() > 0)
			{
				queue.front().resource->UpdateResourceOnGpu(queue.front().index);
				queue.pop();
			}
			work = false;
		}
	}
}

Prefab* ObjectManager::CreatePrefab(std::string meshName, std::string materialName)
{
	Prefab* temp = new Prefab(meshName, materialName, s_PrefabIdCounter);
	m_Prefabs.emplace(s_PrefabIdCounter, temp);
	s_PrefabIdCounter++;
	return temp;
}

Prefab* ObjectManager::GetPrefab(unsigned int prefabId)
{
	return m_Prefabs.at(prefabId);
}

std::unordered_map<unsigned int, Prefab*>* ObjectManager::GetAllPrefabs()
{
	return &m_Prefabs;
}

PointLight* ObjectManager::CreateLight(const PointLight& pointlight)
{
	return m_Lights.CreatePointLight(pointlight);
}

DirectionalLight* ObjectManager::CreateLight(const DirectionalLight& directionalLight)
{
	return m_Lights.CreateDirectionalLight(directionalLight);
}

Lights* ObjectManager::GetLights()
{
	return &m_Lights;
}

void ObjectManager::AddResourceToUpdateQueue(GpuResource* resource, unsigned int index)
{
	m_Mutex.lock();

	m_ResourcesToUpdate[m_QueToPlaceIn].emplace(ResourceUpdateTicket(resource, index));
	m_QueToPlaceIn++;

	if (m_QueToPlaceIn >= m_NrOfThreads)
	{
		m_QueToPlaceIn = 0;
	}

	m_Mutex.unlock();
}

// Updates the resources that could not be writen to during the last fram.
void ObjectManager::OnNewFrame()
{
	for (int i = 0; i < m_NrOfThreads; i++)
	{
		//Set the threads to work
		m_ThreadWorkBools[i] = true;
	}

	for (int i = 0; i < m_NrOfThreads; i++)
	{
		while (m_ThreadWorkBools[i])
		{
			//Wait for the threads
		}
	}
}
