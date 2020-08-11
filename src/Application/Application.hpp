#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include"../Engine/Engine.hpp"
#include"LightBallSpawner.h"


//Event handling
#define RIGHT_MOVE_CAM		uint16_t(0x1)
#define LEFT_MOVE_CAM		uint16_t(0x2)
#define FORWARD_MOVE_CAM	uint16_t(0x4)
#define BACK_MOVE_CAM		uint16_t(0x8)
#define DOUBLE_TIME_CAM		uint16_t(0x10)
#define SKIP_MOVEMENT_INPUT	uint16_t(0x1F)
#define RIGHT_CLICK_CAM		uint16_t(0x20)
#define SPACE_CLICK_CAM		uint16_t(0x30)
#define CAMERA_SPEED_ADJUSTMENT float(0.01)

class Application
{
public:
	Application(const Application& org) = delete;
	void operator=(const Application& org) = delete;
	
	static Application* Get() noexcept;
	void Run();
	inline static void Shutdown() { delete s_Instance; };

private:
	static Application* s_Instance;

	Application();
	virtual ~Application();

	void InitiateLights();

	void EventHandler();
	void KeyPressed();
	void KeyReleased();
	void MouseButtonPressed();
	void MouseButtonReleased();
	void MouseScroll();

	void UpdateCamera();
	void UpdateLights();

	void CreateObjectCircle(const std::string& meshName, const std::string& materialName, const unsigned int& NumberOfObjects, const XMFLOAT3& CentrumPosition, const XMFLOAT3& Direction, const XMFLOAT3& UpDirection, const float& Raduis, const float& MaxCircleOffsetRad, const XMFLOAT2& RandomSize);

	// For Debug
	void MoveFolder();

public:
	const std::string APPLICATION_NAME = "DirectX 12";
	//const XMUINT2 WINDOW_SIZE = XMUINT2(1280, 720);
	const XMUINT2 WINDOW_SIZE = XMUINT2(1600,900);

private:
	bool m_IsRunning = true;

	Camera* m_Camera;
	uint16_t m_CameraController = 0;
	float m_CameraDoubleTime = 1.0f;

	LightBallSpawner* m_LightSpawner1;
	PrefabInstance* m_Moon = nullptr;
	float m_MoonRotation = 0.f;
	float m_MoonRotationSpeed = -0.001f;
	SDL_Event m_Event;
	bool m_IsMouseHeldPressed = false;

	Lights* m_Lights = nullptr;

};
#endif