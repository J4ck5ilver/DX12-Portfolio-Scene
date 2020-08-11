#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "ObjectManager/ObjectManager.h"
#include "RenderEngine/Renderer.hpp"
#include "RenderEngine/Camera/CameraHandler.hpp"

class Engine
{
public:
	static Engine* Get()noexcept;
	bool Initialize(const XMUINT2& windowSize, const std::string& winName = "DirectX 12");
	inline static void Shutdown() { delete s_Instance; };
	void Render(); 

	//Object Manager
	Prefab* CreatePrefab(std::string meshName, std::string materialName = "");

	void SetSkybox(std::string textureName);

	CameraHandler* GetCameraHandler();

	// Light related functions
	PointLight* CreateLight(const PointLight& pointlight);
	DirectionalLight* CreateLight(const DirectionalLight& directionalLight);
	Lights* GetLights();

private:
	Engine() noexcept = default;
	virtual ~Engine() noexcept;

private:
	static Engine* s_Instance;

	CameraHandler m_CameraHandler;

};
#endif // ENGINE_H