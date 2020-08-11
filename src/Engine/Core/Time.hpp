#ifndef TIME_HPP
#define TIME_HPP

#include <ctime>

class Time
{
public:
	Time(const Time& obj) = delete;
	void operator=(const Time& obj) = delete;

	static Time* Get() noexcept;

	float DeltaTime();
	int FPS();
	int GetFrameCount();
	// Returns time in seconds
	float CurrentTime();

	void OnUpdate();
	static inline void Shutdown() { delete s_Instance; }

private:
	Time() noexcept = default;
	virtual ~Time() noexcept;

private:
	static Time* s_Instance;

	// DeltaTime
	static float m_DeltaTime;
	static int m_PerformanceCounterStart;	// Gets frame performance counter
	static int m_PerformanceCounterLast;	// Saves latest performance counter

	// FPS
	static int m_FPS;
	static int m_FrameCount;				//Counts the total number of frames
	static int m_FPSCounter;				// Updates every frame, is then used for m_FPS
	static float m_FPSStartTimer;			// Digits for timer start (in seconds)
	static int m_FPSUpdateRate;				// Rate (in seconds) at which m_FPS should be updated
	static float m_LastFrame;
};
#endif