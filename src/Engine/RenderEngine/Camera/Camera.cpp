#include "Camera.hpp"
#include "../../../Application/Application.hpp"
#include<iostream>
#include"..\Renderer.hpp"

void Camera::Initiate(const CameraProps& cameraProps)
{

	m_DefaultRightDir = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	m_DefaultUpDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_DefaultForwardDir = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	m_CamPosition = XMVectorSet(cameraProps.Position.x, cameraProps.Position.y, cameraProps.Position.z, 0.0f);
	m_CamRight = m_DefaultRightDir;
	m_CamForward = m_DefaultForwardDir;
	m_CamUp = m_DefaultUpDir;

	m_CameraMovementSpeed = DEFAULT_CAMERA_MOVEMENT_SPEED;
	m_CameraRotationSpeed = DEFAULT_CAMERA_ROTATION_SPEED;

	OnMouseRelase();

	m_Yaw = m_Pitch = 0.0f;

	m_DefaultProps = cameraProps;



	float fovRadians = (cameraProps.FieldOfView / 180.0f) * XM_PI;

	XMStoreFloat4x4(&m_ProjectionMatrix, XMMatrixPerspectiveFovLH(fovRadians, (float)cameraProps.WindowSize.x / (float)cameraProps.WindowSize.y, cameraProps.ClippingPlanes.x, cameraProps.ClippingPlanes.y));
	UpdateViewMatrix();
}

const XMMATRIX Camera::GetProjectionMatrix() const
{
	return XMLoadFloat4x4(&m_ProjectionMatrix);;
}

const XMMATRIX Camera::GetViewMatrix()
{
	return XMLoadFloat4x4(&m_ViewMatrix);
}

const XMMATRIX Camera::GetViewProjectionMatrix()
{
	return XMLoadFloat4x4(&m_ViewMatrix) * XMLoadFloat4x4(&m_ProjectionMatrix);
}

void Camera::Translate(const XMFLOAT3& translate)
{
	m_CamPosition += XMVectorSet(translate.x, translate.y, translate.z, 0.0f);
	UpdateViewMatrix();
}

void Camera::Translate(float x, float y, float z)
{
	m_CamPosition += XMVectorSet(x, y, z, 0.0f);
	UpdateViewMatrix();
}


void Camera::SetPosition(const XMFLOAT3& position)
{
	m_CamPosition = XMVectorSet(position.x, position.y, position.z, 0.0f);
	UpdateViewMatrix();
}

void Camera::SetPosition(float x, float y, float z)
{
	m_CamPosition = XMVectorSet(x, y, z, 0.0f);
	UpdateViewMatrix();
}


void Camera::ResetSettings() noexcept
{
	Initiate(m_DefaultProps);
}

void Camera::UpdateRotationMouseInput() noexcept
{
	XMINT2 mouse;
	SDL_GetMouseState(&mouse.x, &mouse.y);

	if (m_LookStartPoint.x == -1.0f && m_LookStartPoint.y == -1.0f)
	{
		m_LookLastPoint.x = m_LookStartPoint.x = mouse.x;
		m_LookLastPoint.y = m_LookStartPoint.y = mouse.y;


		SDL_ShowCursor(false);

		m_CanTurnOffMouse = true;
	}



	XMFLOAT2 tempXMFloat(mouse.x, mouse.y);

	XMVECTOR tempFloatOne = XMLoadFloat2(&tempXMFloat);
	XMVECTOR tempFloatTwo = XMLoadFloat2(&m_LookLastPoint);

	XMVECTOR tempResult = tempFloatOne - tempFloatTwo;

	//XMFLOAT2 rotationDelta;
	XMStoreFloat2(&tempXMFloat, (tempResult * m_CameraRotationSpeed));	// scale for control sensitivity

	// update our orientation based on the command

	if ((tempXMFloat.x != 0.0f) || (tempXMFloat.y != 0.0f))
	{
		//
//			std::cout << "rotationDelta: " << tempXMFloat.x << ", " << tempXMFloat.y << std::endl;

		m_Pitch += tempXMFloat.y;		// mouse y increases down, but pitch increases up
		m_Yaw += tempXMFloat.x;			// yaw defined as CCW around y-axis

		if (m_Pitch > PI_THIRD)
		{
			m_Pitch = PI_THIRD;
		}
		else if (m_Pitch < -PI_THIRD)
		{
			m_Pitch = -PI_THIRD;
		}



		m_LookLastPoint = XMFLOAT2(mouse.x, mouse.y);	// save for next time through


		UpdateViewMatrix();
	}
}

void Camera::OnMouseRelase() noexcept
{

	if (m_CanTurnOffMouse)
	{
		m_LookStartPoint.x = -1.0f;
		m_LookStartPoint.y = -1.0f;

		SDL_ShowCursor(true);

		m_CanTurnOffMouse = false;
	}
}

void Camera::MoveRight(const float& DeltaTime) noexcept
{
	m_CamPosition += XMVector3Normalize(m_CamRight) * DeltaTime * m_CameraMovementSpeed;
	UpdateViewMatrix();
}

void Camera::MoveForward(const float& DeltaTime) noexcept
{

	m_CamPosition += XMVector3Normalize(m_CamTarget) * DeltaTime * m_CameraMovementSpeed;
	UpdateViewMatrix();
}

void Camera::OnUpdate()
{
	//// If key pressed
	//UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{

	m_CamRotationMatrix = XMMatrixRotationRollPitchYaw(m_Pitch, m_Yaw, 0.0f);
	m_CamTarget = XMVector3TransformCoord(m_DefaultForwardDir, m_CamRotationMatrix);
	m_CamTarget = XMVector3Normalize(m_CamTarget);

	m_CamRight = XMVector3TransformCoord(m_DefaultRightDir, m_CamRotationMatrix);
	m_CamForward = XMVector3TransformCoord(m_DefaultForwardDir, m_CamRotationMatrix);
	m_CamUp = XMVector3Cross(m_CamForward, m_CamRight);

	XMStoreFloat4x4(&m_ViewMatrix, XMMatrixLookAtLH(m_CamPosition, (m_CamPosition + m_CamTarget), m_CamUp));

	//Not a clean fix, can only use one camera for the movement
	//Renderer::Get()->UpdateCameraResources();

}