#ifndef OBJECTMANAGER_H
#define OBJECTMANAGER_H

#include "Prefab.h"
#include "Lights.hpp"

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include "../../Application/LightBall.hpp"
class ObjectManager
{
	struct ResourceUpdateTicket
	{
		ResourceUpdateTicket(GpuResource* resource, unsigned int index) : resource(resource), index(index) {}
		GpuResource* resource;
		unsigned int index;
	};

public:
	static ObjectManager* Get() noexcept;
	Prefab* CreatePrefab(std::string meshName, std::string materialName);
	inline static void Shutdown() { delete s_Instance; };

	Prefab* GetPrefab(unsigned int prefabId);
	std::unordered_map<unsigned int, Prefab*>* GetAllPrefabs();

	// Lights
	PointLight* CreateLight(const PointLight& pointlight);
	DirectionalLight* CreateLight(const DirectionalLight& directionalLight);
	Lights* GetLights();

	void AddResourceToUpdateQueue(GpuResource* resource,unsigned int index);
	void OnNewFrame();

private:
	ObjectManager() noexcept;
	virtual ~ObjectManager() noexcept;

	static void RunThread(std::queue<ResourceUpdateTicket>& queue, bool& work, bool& shutDown);

private:

	static ObjectManager* s_Instance;

	std::unordered_map<unsigned int, Prefab*>m_Prefabs;
	static unsigned int s_PrefabIdCounter;

	std::vector<std::thread*> m_Threads;
	std::mutex m_Mutex;

	int m_NrOfThreads = 2;
	int m_QueToPlaceIn = 0;

	Lights m_Lights;

	std::queue<ResourceUpdateTicket>* m_ResourcesToUpdate;
	bool* m_ThreadWorkBools;
	bool m_ShutdownThreads = false;

};
#endif // !OBJECTMANAGER_H