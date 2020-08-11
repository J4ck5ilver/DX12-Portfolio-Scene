#include "Application.hpp"
#include "../Engine/Core/Time.hpp"
#include "..\Engine\Assetmanager\Assetmanager.hpp"

//For Debug
#include<filesystem>
#include <iostream>
#include <string.h>

#include <AeonProfiler.h>
#include<random>

Application* Application::s_Instance = nullptr;

Application::Application()
{
	//MoveFolder();



	// Initiates Engine and Renderer
	Engine::Get()->Initialize(WINDOW_SIZE, APPLICATION_NAME);

	// Assetmanager Test
	Assetmanager::Get()->LoadFolder("assets", true);

	Renderer::Get()->InitThreads();

	// Camera
	m_Camera = Engine::Get()->GetCameraHandler()->CreateCamera(CameraProps(WINDOW_SIZE, XMFLOAT3(0.f, 100.f, -1200.f)));

	//Set camera for Renderer
	Renderer::Get()->SetRenderCamera(Engine::Get()->GetCameraHandler()->GetCamera());


	Prefab* cube = Engine::Get()->CreatePrefab("cube", "Sphere");


	float yOffset = -70;
	m_LightSpawner1 = new LightBallSpawner(cube, 1000, yOffset, yOffset + yOffset*-2, 0.4f, false, 1000);
	m_LightSpawner1->UpdateAllSpheres();


	CreateObjectCircle("Asteroid1", "Asteroid", (unsigned int) 5000, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), 700, 80.0f, XMFLOAT2(0.2,0.5));



	Prefab* moon = Engine::Get()->CreatePrefab("Moon", "Moon");
	m_Moon = moon->CreateInstance();
	m_Moon->GetTransform()->SetScale(DirectX::XMFLOAT3(300, 300, 300));
	m_Moon->GetTransform()->SetPosition(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));

	Engine::Get()->SetSkybox("Galaxy");


	Benchmarker::Get()->Initiate(10.0f, 10.0f);

	//Print controls
	std::cout << "Controls: \nMove with[W], [A], [S], [D]. \nHold[Right Mouse Button] to look around. \nReset view and position with[R].  \nPress[ESC] to shut down." << std::endl;


}

Application::~Application()
{


	delete m_LightSpawner1;

	Benchmarker::Get()->Shutdown();
	Assetmanager::Get()->Shutdown();
	Engine::Get()->Shutdown();
	Time::Shutdown();
}

void Application::InitiateLights()
{
	Prefab* prefab = Engine::Get()->CreatePrefab("cube", "Sphere");

	m_Lights = Engine::Get()->GetLights();
	for (int i = 0; i < 1; i++)
	{

		PrefabInstance* cube = prefab->CreateInstance();
		cube->GetTransform()->SetScale(DirectX::XMFLOAT3(1, 1, 1));

		float y = -0 + 80 * i;
		float z = y;

		cube->GetTransform()->SetPosition(XMFLOAT3(0.f, y, z));
		cube->GetColor()->SetColor(1, 1, 0.f);

		m_Lights->CreatePointLight(PointLight(XMFLOAT4(0.f, 1.f, 1.f, 0.f), XMFLOAT3(0.f, 0.f, 0.f), 0.f));
		m_Lights->SetPointLightPosition(XMFLOAT3(0.f, y, z), i);
		m_Lights->SetPointLightColor(XMFLOAT3(1, 1, 0.f), i);
		m_Lights->SetPointLightStrength(1, i);
		m_Lights->SetPointLightRadius(20, i);
	}
}

Application* Application::Get() noexcept
{
	if (!s_Instance)
	{
		s_Instance = new Application();
	}
	return s_Instance;
}

void Application::Run()
{
	while (m_IsRunning)
	{
		//Benchmarker::Get()->Run();


		// Event
		EventHandler();

		// Update
		Time::Get()->OnUpdate();
		UpdateCamera();

		//Scene Logic
		m_LightSpawner1->UpdateAllSpheres();
		m_MoonRotation += Time::Get()->DeltaTime() * m_MoonRotationSpeed;
		m_Moon->GetTransform()->SetRotation(XMFLOAT3(0, 1, 0), m_MoonRotation);

		// Render
		Engine::Get()->Render();
	}
}

void Application::EventHandler()
{
	while (SDL_PollEvent(&m_Event))
	{
		switch (m_Event.type)
		{
			// Quit (X on window)
		case SDL_QUIT:
			printf("QUIT\n");
			m_IsRunning = false;
			break;

			// Key pressed
		case SDL_KEYDOWN:
			KeyPressed();
			break;

			// Key released
		case SDL_KEYUP:
			KeyReleased();
			break;

			// Mouse button pressed
		case SDL_MOUSEBUTTONDOWN:
			MouseButtonPressed();
			break;

			// Mouse button released
		case SDL_MOUSEBUTTONUP:
			MouseButtonReleased();
			break;

		case SDL_MOUSEWHEEL:
			MouseScroll();
			break;

		default:
			break;
		}
	}
}

void Application::KeyPressed()
{
	switch (m_Event.key.keysym.sym)
	{
	case SDLK_ESCAPE:
		m_IsRunning = false;
		break;

	case SDLK_d:
		m_CameraController |= RIGHT_MOVE_CAM;
		break;

	case SDLK_a:
		m_CameraController |= LEFT_MOVE_CAM;
		break;

	case SDLK_w:
		m_CameraController |= FORWARD_MOVE_CAM;
		break;

	case SDLK_s:
		m_CameraController |= BACK_MOVE_CAM;
		break;

	case SDLK_LSHIFT:
		m_CameraController |= DOUBLE_TIME_CAM;
		break;

	case SDLK_r:
		m_Camera->ResetSettings();
		break;

	case SDLK_SPACE:
		m_CameraController |= SPACE_CLICK_CAM;
		break;

	default:
		break;
	}
}

void Application::KeyReleased()
{
	switch (m_Event.key.keysym.sym)
	{
	case SDLK_ESCAPE:
		break;

	case SDLK_d:
		m_CameraController &= ~RIGHT_MOVE_CAM;
		break;

	case SDLK_a:
		m_CameraController &= ~LEFT_MOVE_CAM;
		break;

	case SDLK_w:
		m_CameraController &= ~FORWARD_MOVE_CAM;
		break;

	case SDLK_s:
		m_CameraController &= ~BACK_MOVE_CAM;
		break;

	case SDLK_LSHIFT:
		m_CameraController &= ~DOUBLE_TIME_CAM;
		break;
	case SDLK_SPACE:
		m_CameraController &= ~SPACE_CLICK_CAM | ~RIGHT_CLICK_CAM;
		m_Camera->OnMouseRelase();
		break;

	default:
		break;
	}
}

void Application::MouseButtonPressed()
{
	switch (m_Event.button.button)
	{
	case SDL_BUTTON_RIGHT:
		m_CameraController |= RIGHT_CLICK_CAM;
		break;

	case SDL_BUTTON_MIDDLE:
		break;

	case SDL_BUTTON_LEFT:
		break;

	default:
		break;
	}

	uint16_t test = m_CameraController & RIGHT_CLICK_CAM;
}

void Application::MouseButtonReleased()
{
	switch (m_Event.button.button)
	{
	case SDL_BUTTON_RIGHT:
		m_CameraController &= ~RIGHT_CLICK_CAM | ~SPACE_CLICK_CAM;
		m_Camera->OnMouseRelase();
		break;

	case SDL_BUTTON_MIDDLE:
		break;

	case SDL_BUTTON_LEFT:
		break;

	default:
		break;
	}
}

void Application::MouseScroll()
{
	if (m_Event.wheel.y > 0)
	{
		m_Camera->AddCameraSpeed(CAMERA_SPEED_ADJUSTMENT);
	}
	else if (m_Event.wheel.y < 0)
	{
		m_Camera->AddCameraSpeed(-CAMERA_SPEED_ADJUSTMENT);
	}
}

void Application::UpdateCamera()
{
	if (m_CameraController > 0)
	{
		// Only Shift input
		if (m_CameraController != DOUBLE_TIME_CAM)
		{
			// Shift Double Time
			if (m_CameraController & DOUBLE_TIME_CAM)
			{
				m_CameraDoubleTime = 2.0f;
			}
			else
			{
				m_CameraDoubleTime = 1.0f;
			}

			// If we have only mouse input
			if ((m_CameraController & SKIP_MOVEMENT_INPUT) != 0)
			{
				// Movement
				if (m_CameraController & RIGHT_MOVE_CAM)
				{
					m_Camera->MoveRight(Time::Get()->DeltaTime() * m_CameraDoubleTime);
				}
				if (m_CameraController & LEFT_MOVE_CAM)
				{
					m_Camera->MoveRight(-Time::Get()->DeltaTime() * m_CameraDoubleTime);
				}
				if (m_CameraController & FORWARD_MOVE_CAM)
				{
					m_Camera->MoveForward(Time::Get()->DeltaTime() * m_CameraDoubleTime);
				}
				if (m_CameraController & BACK_MOVE_CAM)
				{
					m_Camera->MoveForward(-Time::Get()->DeltaTime() * m_CameraDoubleTime);
				}
			}

			// Mouse Input
			if (m_CameraController & RIGHT_CLICK_CAM || m_CameraController & SPACE_CLICK_CAM)
			{
				m_Camera->UpdateRotationMouseInput();
			}
		}
	}
}

void Application::CreateObjectCircle(const std::string& meshName, const std::string& materialName, const unsigned int& NumberOfObjects, const XMFLOAT3& CentrumPosition, const XMFLOAT3& Direction, const XMFLOAT3& UpDirection, const float& Raduis, const float& MaxCircleOffsetRad, const XMFLOAT2& RandomSize)
{
	Prefab* preFab = Engine::Get()->CreatePrefab(meshName, materialName);

	PrefabInstance* tempInstance = { nullptr };

	srand(NumberOfObjects);

	for (unsigned int i = 0; i < NumberOfObjects; i++)
	{
		tempInstance = preFab->CreateInstance();



		//Ring Distribution

		float degreeOffset = XMConvertToRadians(360.0f / NumberOfObjects);

		XMVECTOR normalizedDirectionVector = XMVectorSet(Direction.x, Direction.y, Direction.z, 0.0f);
		normalizedDirectionVector = XMVector3Normalize(normalizedDirectionVector);

		XMVECTOR upDirectionVector = XMVectorSet(UpDirection.x, UpDirection.y, UpDirection.z, 0.0f);
		upDirectionVector = XMVector3Normalize(upDirectionVector);

		XMMATRIX rotationMatrix = XMMatrixRotationAxis(upDirectionVector, degreeOffset * i);

		XMVECTOR calcVector = XMVector4Transform(normalizedDirectionVector, rotationMatrix);
		XMVECTOR  distanceVector = calcVector * Raduis;




		//Random Distribution

		calcVector = XMVector3Normalize(calcVector);

		XMVECTOR crossVector = XMVector3Cross(upDirectionVector, calcVector);
		crossVector = XMVector3Normalize(crossVector);

		float randomNumber = rand();

		float tempCalcFloat = randomNumber / MaxCircleOffsetRad;
		int floatToInt = static_cast<int>(tempCalcFloat);
		float randomDistance = MaxCircleOffsetRad * (tempCalcFloat - floatToInt);


		XMMATRIX crossRotationMatrix = XMMatrixRotationAxis(crossVector, XMConvertToRadians(randomNumber));

		XMVECTOR axisRandomVector = XMVector4Transform(calcVector, crossRotationMatrix);

		axisRandomVector *= randomDistance;


		//Store 

		XMFLOAT3 tempFloat3_1;
		XMFLOAT3 tempFloat3_2;

		XMStoreFloat3(&tempFloat3_1, axisRandomVector);
		XMStoreFloat3(&tempFloat3_2, distanceVector);


		float x = tempFloat3_1.x + tempFloat3_2.x + CentrumPosition.x;
		float y = tempFloat3_1.y + tempFloat3_2.y + CentrumPosition.y;
		float z = tempFloat3_1.z + tempFloat3_2.z + CentrumPosition.z;

		XMFLOAT3 tempFloat3_3(x, y, z);;


		tempInstance->GetTransform()->SetPosition(tempFloat3_3);

		//Random rotation

		XMFLOAT3 randomRotationContainer;
		randomRotationContainer.x =  rand();
		randomRotationContainer.y =  rand();
		randomRotationContainer.z =  rand();

		XMVECTOR randomRotationVector = XMVectorSet(randomRotationContainer.x, randomRotationContainer.y, randomRotationContainer.z, 0.0f);



		randomRotationVector = XMVector3Normalize(randomRotationVector);

		XMStoreFloat3(&randomRotationContainer, randomRotationVector);

		tempInstance->GetTransform()->SetRotation(randomRotationContainer, randomNumber);

		//Random Size

		float tempSizeVector[3];

		for (int i = 0; i < 3; i++)
		{
			int tempRandomNumber = rand();

			int tempCalcInt = tempRandomNumber / RandomSize.y;
			float tempCalcSize = tempRandomNumber - (tempCalcInt * RandomSize.y);

			if (tempCalcSize < RandomSize.x)
			{
				tempCalcSize = RandomSize.x;
			}

			tempSizeVector[i] = tempCalcSize;
		}
		tempInstance->GetTransform()->SetScale(XMFLOAT3(tempSizeVector[0], tempSizeVector[1], tempSizeVector[2]));
	}
}

void Application::MoveFolder()
{
	std::filesystem::path path = std::filesystem::current_path();
	const std::filesystem::path src = path.u8string() + "//assets";
	const std::filesystem::path dst = path.u8string() + "//x64/Debug//assets";

	try
	{
		std::filesystem::copy(src, dst, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
	}

	catch (std::string hh)
	{
		std::cout << hh << std::endl;
	}
}