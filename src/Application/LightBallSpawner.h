#ifndef LIGHTBALLSPAWNER_H
#define LIGHTBALLSPAWNER_H
#include "LightBall.hpp"
#include <vector>
#include <thread>

class LightBallSpawner
{
public:

	LightBallSpawner(Prefab* prefab,int nrOfObjectsToSpawn = 0, float startZ = 0, float endZ = 0, float radiusIncrease = 0.0f, bool flipDirection = false, unsigned int nrOfLights = 0);
	virtual ~LightBallSpawner();
	
	
	void UpdateAllSpheres();

private:
	static void UpateSphereByRange(std::vector<LightBall*>& vector, int start, int end, bool& work, bool& shutDown, float& dt);


private:

	Prefab* m_Prefab;

	std::vector<LightBall*> m_LightBalls;
	std::vector<std::thread*> m_Threads;
	bool* m_ThreadWorkBools;
	bool m_ShutdownThreads = false;
	int m_NrOfThreads = 2;
	float m_StartTimeUpdate = 0;
	int m_TargetFrequence = 60; //Does not always reach this number. For example: 60 could end up as 35

	//For debuging
	float m_StartTimeOutput = 0;
	int m_UpdatesPerSec = 0;
	float m_DtForUpdate = 0.f; //The time between each update;

};

#endif // !LIGHTBALL_H
