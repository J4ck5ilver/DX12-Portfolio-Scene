#include "Time.hpp"
#include "../../../libaries/includes/SDL/SDL_timer.h"

Time* Time::s_Instance = nullptr;
float Time::m_DeltaTime = 0.f;
int Time::m_PerformanceCounterStart = 0;
int Time::m_PerformanceCounterLast = 0;
int Time::m_FPS = 0;
int Time::m_FrameCount = 0;
int Time::m_FPSCounter = 0;
float Time::m_FPSStartTimer = float(std::clock()) * 0.001f;
int Time::m_FPSUpdateRate = 1;
float Time::m_LastFrame = 0;

Time::~Time() noexcept
{
}

Time* Time::Get() noexcept
{
	if (s_Instance == nullptr)
	{
		s_Instance = new Time();
	}
	return s_Instance;
}

float Time::DeltaTime()
{
	return m_DeltaTime;
}

int Time::FPS()
{
	return m_FPS;
}

int Time::GetFrameCount()
{
	return m_FrameCount;
}

float Time::CurrentTime()
{
	return float(std::clock()) * 0.001f;
}



void Time::OnUpdate()
{
	// DeltaTime
	float currentFrame = float(std::clock());
	m_DeltaTime = currentFrame - m_LastFrame;
	m_LastFrame = currentFrame;

	if (m_DeltaTime < 0)
	{
		m_DeltaTime = 0;
	}

	// FPS
	if (CurrentTime() - m_FPSStartTimer >= m_FPSUpdateRate )
	{
		m_FPS = m_FPSCounter;
		m_FPSStartTimer = CurrentTime();
		m_FPSCounter = 0;
	}
	m_FPSCounter++;
	m_FrameCount++;
}