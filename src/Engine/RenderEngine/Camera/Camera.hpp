#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "../../ObjectManager/Transform.hpp"

#include<DirectXMath.h>

#define DEFAULT_CAMERA_MOVEMENT_SPEED float(0.05f)
#define DEFAULT_CAMERA_ROTATION_SPEED float(0.005f)

#define PI_THIRD (const float)(XM_PI / 3.0f) // MIN/MAX Camera Up/Down Angle

struct CameraProps
{
	XMUINT2 WindowSize = XMUINT2(0, 0);
	XMFLOAT3 Position = XMFLOAT3(0.f, 0.f, 0.f);
	XMFLOAT4 Rotation = XMFLOAT4(0.f, 0.f, 0.f, 0.f);
	float FieldOfView = 0.0f;
	XMFLOAT2 ClippingPlanes = XMFLOAT2(0.f, 0.f);

	CameraProps(const XMUINT2 windowSize, const XMFLOAT3& position = XMFLOAT3(0.0f, 0.0f, 0.0f),
		const XMFLOAT4& rotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.f),
		float fov = 60.0f,
		const XMFLOAT2& nearFar = XMFLOAT2(0.03f, 3000.0f))
		: WindowSize(windowSize), Position(position), Rotation(rotation), FieldOfView(fov), ClippingPlanes(nearFar) { }


	CameraProps() = default;
	~CameraProps() = default;
	CameraProps(const CameraProps&) = default;
	CameraProps& operator =(const CameraProps&) = default;

};

class Camera
{
public:
	Camera() = default;
	virtual ~Camera() = default;

	Camera(const Camera& obj) = delete;
	Camera& operator=(const Camera& obj) = delete;

	void Initiate(const CameraProps& cameraProps = CameraProps(XMUINT2(0, 0)));
	inline bool* ActiveStatus() { return &m_Active; }
	inline bool IsActive() const { return m_Active; }

	inline const XMFLOAT3& GetPositionConst() { XMStoreFloat3(&m_CamPosStoreFloat, m_CamPosition); return m_CamPosStoreFloat; }
	inline XMFLOAT3& GetPosition() { XMStoreFloat3(&m_CamPosStoreFloat, m_CamPosition); return m_CamPosStoreFloat; }


	const XMMATRIX GetProjectionMatrix() const;
	const XMMATRIX GetViewMatrix();
	const XMMATRIX GetViewProjectionMatrix();

	inline const float& GetCameraSpeed() noexcept { return m_CameraMovementSpeed; }

	void Translate(const XMFLOAT3& translate);
	void Translate(float x, float y, float z);


	void SetPosition(const XMFLOAT3& position);
	void SetPosition(float x, float y, float z);

	void ResetSettings() noexcept;

	inline void SetCameraSpeed(const float& NewCamSpeed) noexcept { m_CameraMovementSpeed = NewCamSpeed; }
	inline void AddCameraSpeed(const float& AdjCamSpeed) noexcept { if ((m_CameraMovementSpeed + AdjCamSpeed) >= 0) { m_CameraMovementSpeed += AdjCamSpeed; } }

	void UpdateRotationMouseInput() noexcept;
	void OnMouseRelase() noexcept;

	void MoveRight(const float& DeltaTime) noexcept;
	void MoveForward(const float& DeltaTime) noexcept;


	void OnUpdate();
	void UpdateViewMatrix();

private:
	float m_CameraMovementSpeed{ DEFAULT_CAMERA_MOVEMENT_SPEED };
	float m_CameraRotationSpeed{ DEFAULT_CAMERA_ROTATION_SPEED };
	bool m_Active{ true };

	XMFLOAT4X4 m_ProjectionMatrix = {};
	XMFLOAT4X4 m_ViewMatrix = {};

	CameraProps m_DefaultProps;

	float m_Pitch;
	float m_Yaw;
	

	bool m_CanTurnOffMouse = false;


	XMFLOAT2 m_LookLastPoint ; //Pixel Coords
	XMFLOAT2 m_LookStartPoint{ -1.0f,-1.0f }; //Pixel Coords

	XMMATRIX m_CamRotationMatrix;
	XMMATRIX m_CamViewMatrix;

	XMVECTOR m_DefaultUpDir;
	XMVECTOR m_DefaultRightDir;
	XMVECTOR m_DefaultForwardDir;

	XMVECTOR m_CamTarget;

	XMVECTOR m_CamPosition;

	XMFLOAT3 m_CamPosStoreFloat;

	XMVECTOR m_CamUp;
	XMVECTOR m_CamRight;
	XMVECTOR m_CamForward;

};
#endif