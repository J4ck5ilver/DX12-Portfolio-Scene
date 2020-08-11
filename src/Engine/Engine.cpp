#include "Engine.hpp"
#include "Assetmanager/Assetmanager.hpp"
Engine* Engine::s_Instance = nullptr;

Engine* Engine::Get() noexcept
{
	if (!s_Instance)
	{
		s_Instance = new Engine();
	}
	return s_Instance;
}

bool Engine::Initialize(const XMUINT2& windowSize, const std::string& winName)
{
	bool returnValue = true;

	Renderer::Get()->Initialize(windowSize, winName);

	return returnValue;
}

void Engine::Render()
{
	Renderer::Get()->OnNewFrame();
	Renderer::Get()->Render();
	ObjectManager::Get()->OnNewFrame();
}

Engine::~Engine() noexcept
{
	ObjectManager::Get()->Shutdown();
	Renderer::Get()->Shutdown();
}

Prefab* Engine::CreatePrefab(std::string meshName, std::string materialName)
{
	return ObjectManager::Get()->CreatePrefab(meshName, materialName);
}

void Engine::SetSkybox(std::string textureName)
{
	Renderer::Get()->CreateSkybox(Assetmanager::Get()->GetTexture(textureName));
}

CameraHandler* Engine::GetCameraHandler()
{
	return &m_CameraHandler;
}

PointLight* Engine::CreateLight(const PointLight& pointlight)
{
	return ObjectManager::Get()->CreateLight(pointlight);
}

DirectionalLight* Engine::CreateLight(const DirectionalLight& directionalLight)
{
	return ObjectManager::Get()->CreateLight(directionalLight);
}

Lights* Engine::GetLights()
{
	return ObjectManager::Get()->GetLights();
}