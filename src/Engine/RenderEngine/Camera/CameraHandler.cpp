#include "CameraHandler.hpp"

CameraHandler::~CameraHandler()
{
	int numberOfElements = m_Cameras.size();
	for (size_t i = 0; i < numberOfElements; i++)
	{
		delete m_Cameras[i];
		m_Cameras[i] = nullptr;
	}
}

Camera* CameraHandler::CreateCamera(const CameraProps& cameraProps)
{
	m_Cameras.emplace_back(new Camera());
	m_Cameras.back()->Initiate(cameraProps);

	return m_Cameras.back();
}

Camera* CameraHandler::GetCamera()
{
	return m_Cameras.back();
}