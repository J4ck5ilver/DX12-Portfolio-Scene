#include "LightBallSpawner.h"
#include "../Engine/Core/Time.hpp"
#include <iostream>


LightBallSpawner::LightBallSpawner(Prefab* prefab,int nrOfObjectsToSpawn, float startY, float endY, float radiusIncrease, bool flipDirection, unsigned int nrOfLights)
{
	m_Threads.resize(m_NrOfThreads);


	m_Prefab = prefab; 

	int nrOfSpawned = 0;
	float radius = 590.0f;

	m_LightBalls.reserve(nrOfObjectsToSpawn);
	for (int i = 0; i < nrOfObjectsToSpawn; i++)
	{
		nrOfSpawned++;



		int offsetFactor = nrOfSpawned;
		float timeOffset = 0;

		//Helix Spinners
		//float rowOffset = i;
		//float rowOffset = i * 0.2f;
	

		//Spread-out Spinners
		//float rowOffset = i*i * 0.2f;
		//float rowOffset = i * i* 0.2f * 0.2f;
		float rowOffset = i * i* 0.2f * 0.2f * 0.2f;
		

		//Hypno Spinners
		//float rowOffset = i * 0.2f * 0.2f;
		//float rowOffset =  i* 0.2f * 0.2f * 0.2f;

		float placement = (float)nrOfSpawned / (float)nrOfObjectsToSpawn;
		if (placement < 0.5)
		{
			radius += radiusIncrease;
		}
		else
		{
			radius -= radiusIncrease;

		}

		/*
			if (m_LightBalls.size() % 5 == 0)
			{
				offsetFactor--;
				timeOffset = 15;
			}*/
		
		

		float y = ((endY - startY) / (float)nrOfObjectsToSpawn) * offsetFactor + startY;
		timeOffset += rowOffset;

		bool spawnLight = false;

		if (nrOfLights > 0)
		{
			if (i % (nrOfObjectsToSpawn / nrOfLights) == 0)
			{
				spawnLight = true;
			}
		}
		




		m_LightBalls.emplace_back(new LightBall(m_Prefab->CreateInstance(), y, timeOffset, 0, radius,Red, flipDirection, spawnLight));

	}


	int start = 0;
	int end = 0;
	float div = 0;

	m_ThreadWorkBools = new bool[m_NrOfThreads];
	for (int i = 0; i < m_NrOfThreads; i++)
	{

		end = start + (m_LightBalls.size() / m_NrOfThreads);

		m_ThreadWorkBools[i] = false;
		m_Threads[i] = new std::thread(UpateSphereByRange, std::ref(m_LightBalls), start, end, std::ref(m_ThreadWorkBools[i]), std::ref(m_ShutdownThreads), std::ref(m_DtForUpdate));
		m_Threads[i]->detach();

		start += end ;
	}

}

LightBallSpawner::~LightBallSpawner()
{
	m_ShutdownThreads = true;

	for (int i = 0; i < m_LightBalls.size(); i++)
	{
		delete m_LightBalls[i];
		m_LightBalls.at(i) = nullptr;
	}


	for (int i = 0; i < m_NrOfThreads; i++)
	{
		delete m_Threads.at(i);
		m_Threads.at(i) = nullptr;
	}

	delete m_ThreadWorkBools;
}

void LightBallSpawner::UpdateAllSpheres()
{

	//Locks the update rate
	float timeSinceLastUpdate = Time::Get()->CurrentTime() - m_StartTimeUpdate;
	if (timeSinceLastUpdate >= ((float)1 / (float)m_TargetFrequence))
	{
		m_DtForUpdate = timeSinceLastUpdate;

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
	
		m_StartTimeUpdate = Time::Get()->CurrentTime();
		m_UpdatesPerSec++;

	}



	//For debuging
	/*if (Time::Get()->CurrentTime() - m_StartTimeOutput >= 1)
	{
		std::cout << m_UpdatesPerSec << "  :  " << Time::Get()->FPS() << std::endl;

		m_StartTimeOutput = Time::Get()->CurrentTime();
		m_UpdatesPerSec = 0;
	}*/
}

void LightBallSpawner::UpateSphereByRange(std::vector<LightBall*>& vector, int start, int end, bool& work, bool& shutDown, float& dt)
{
	while (!shutDown)
	{
		//std::cout << "Waiting: " << std::endl;
		if (work == true)
		{
			for (int i = start; i < end; i++)
			{
				vector.at(i)->Rotate(dt);
			}
			work = false;
		}
	}
}


