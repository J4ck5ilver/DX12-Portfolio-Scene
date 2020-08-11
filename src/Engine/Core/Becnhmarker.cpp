#include "Becnhmarker.hpp"


Benchmarker* Benchmarker::m_Instance = nullptr;

int		Benchmarker::m_CPU_FrameCount = 0;
int		Benchmarker::m_GPU_FrameCount = 0;
float	Benchmarker::m_EndTime = 0.0f;
float	Benchmarker::m_TestDuration = 0.0f;
float	Benchmarker::m_StartDelay = 0.0f;
double	Benchmarker::m_CPU_Time = 0.0;
double	Benchmarker::m_GPU_Time = 0.0;
bool	Benchmarker::m_HasPrinted = false;

double	Benchmarker::m_Elapsed_Time = 0.0;



std::chrono::steady_clock::time_point Benchmarker::m_CPU_TimePoint;
std::chrono::steady_clock::time_point Benchmarker::m_GPU_TimePoint;
std::chrono::steady_clock::time_point Benchmarker::m_Initiate_TimePoint;


Benchmarker::~Benchmarker() noexcept
{

}

Benchmarker* Benchmarker::Get()
{

	if (!m_Instance)
	{
		m_Instance = new Benchmarker();
	}

	return m_Instance;
}

//In seconds
void Benchmarker::Initiate(const float& Duration, const float& StartDelay)
{
	m_StartDelay = StartDelay * 1e9;
	m_Initiate_TimePoint = std::chrono::high_resolution_clock::now();

	m_TestDuration = Duration;
	double TempDuration = Duration * 1e9;

	auto startPoint = std::chrono::duration_cast<std::chrono::nanoseconds>(m_Initiate_TimePoint.time_since_epoch());

	m_EndTime = Duration * 1e9 + m_StartDelay;

	int hh = 0;
}

void Benchmarker::StartClock(const ClockType& Type)
{
	auto startPoint = std::chrono::duration_cast<std::chrono::nanoseconds>(m_Initiate_TimePoint.time_since_epoch());


	if ((m_Elapsed_Time >= m_StartDelay && m_EndTime >= m_Elapsed_Time))
	{
		switch (Type)
		{
		case ClockType::CPUCLOCK:


			m_CPU_TimePoint = std::chrono::high_resolution_clock::now();

			break;

		case ClockType::GPUCLOCK:

			m_GPU_TimePoint = std::chrono::high_resolution_clock::now();
			//std::cout << "Starting GPU Clock" << std::endl;

			break;

		default:

			break;
		}

	}
}

void Benchmarker::EndClock(const ClockType& Type, bool AddCount)
{
	auto startPoint = std::chrono::duration_cast<std::chrono::nanoseconds>(m_Initiate_TimePoint.time_since_epoch());


	if ((m_Elapsed_Time >= m_StartDelay && m_EndTime >= m_Elapsed_Time))
	{
		switch (Type)
		{
		case ClockType::CPUCLOCK:

			if (m_CPU_FrameCount != 0)
			{
				m_CPU_Time += std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - m_CPU_TimePoint)).count();
			}
			if (AddCount)
			{
				m_CPU_FrameCount++;
			}


			break;

		case ClockType::GPUCLOCK:

			if (m_CPU_FrameCount != 0)
			{
				m_GPU_Time += std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - m_GPU_TimePoint)).count();
			}
			if (AddCount)
			{
				m_GPU_FrameCount++;
			}


			break;

		default:

			break;
		}
	}

}

void Benchmarker::Run()
{

	m_Elapsed_Time = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - m_Initiate_TimePoint)).count();


	EndClock(ClockType::CPUCLOCK, true);
	StartClock(ClockType::CPUCLOCK);


	auto testPoint = std::chrono::duration_cast<std::chrono::nanoseconds>(m_Initiate_TimePoint.time_since_epoch());


	if (m_Elapsed_Time  >= m_EndTime && m_HasPrinted == false)
	{
		std::cout << "GPU Time: " << m_GPU_Time /(1e6* m_GPU_FrameCount) << " ms" << std::endl;
		std::cout << "CPU Time: " << m_CPU_Time / (1e6 * m_CPU_FrameCount) << " ms" <<std::endl;
		std::cout << "AVG FPS: " << m_CPU_FrameCount / m_TestDuration  << std::endl;

		m_HasPrinted = true;
	}



}

void Benchmarker::Shutdown()
{
	if (m_Instance)
	{
		delete m_Instance;
		m_Instance = nullptr;
	}
}
