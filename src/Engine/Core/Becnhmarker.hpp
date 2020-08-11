#ifndef BENCHMARKER_H
#define BENCKMARKER_H

#include<chrono>
#include<ctime>
#include<iostream>

enum ClockType
{
	GPUCLOCK,
	CPUCLOCK
};


class Benchmarker
{

private:

	static Benchmarker* m_Instance;

	static int		m_CPU_FrameCount;
	static int		m_GPU_FrameCount;
	static float	m_EndTime;
	static float	m_TestDuration;
	static float	m_StartDelay;
	static double	m_CPU_Time;
	static double	m_GPU_Time;
	static bool		m_HasPrinted;

	static double	m_Elapsed_Time;

	static std::chrono::steady_clock::time_point m_CPU_TimePoint;
	static std::chrono::steady_clock::time_point m_GPU_TimePoint;
	static std::chrono::steady_clock::time_point m_Initiate_TimePoint;


	Benchmarker() noexcept = default;
	~Benchmarker() noexcept;


	Benchmarker(const Benchmarker&) = delete;
	void operator =(const Benchmarker&) = delete;

	
public:
	static Benchmarker* Get();

	static void Initiate(const float& Duration,const float& StartDelay = 0.0f);
	static void StartClock(const ClockType& Type);
	static void EndClock(const ClockType& Type, bool AddCount = false);
	static void Run();
	static void Shutdown();

};

#endif