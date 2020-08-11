#ifndef CAMERAHANDLER_HPP
#define CAMERAHANDLER_HPP

#include "Camera.hpp"
#include <vector>

class CameraHandler
{
public:
	CameraHandler() = default;
	virtual ~CameraHandler();

	CameraHandler(const CameraHandler& obj) = delete;
	CameraHandler operator=(const CameraHandler& obj) = delete;

	// Add Camera
	Camera* CreateCamera(const CameraProps& cameraProps = CameraProps(XMUINT2(0, 0)));



	// Get Camera
	Camera* GetCamera();	

private:
	std::vector<Camera*> m_Cameras;

};
#endif // ! CAMERAHANDLER_HPP