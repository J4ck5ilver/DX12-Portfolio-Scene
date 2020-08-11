#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include<DirectXMath.h>
#include"../Assetmanager/AssetFiles/GpuResource.hpp"

using namespace DirectX;
class Transform : public GpuResource
{
public:
	Transform();
	virtual ~Transform() = default;

	Transform(const Transform& obj);
	Transform operator=(const Transform& obj);

	// Get

	const XMFLOAT4X4& GetTransform();
	const XMFLOAT3& GetPositionConst() const;
	XMFLOAT3* GetPosition();
	const XMFLOAT3& GetRotationConst() const;
	XMFLOAT3* GetRotation();
	const XMFLOAT3& GetScale() const;


	// Set
	void Translate(const XMFLOAT3& translate);
	void Rotate(const XMFLOAT3& rotation, float angle);

	void SetPosition(const XMFLOAT3& position);
	void SetRotation(const XMFLOAT3& rotation, float angle);
	void SetScale(const XMFLOAT3& scale);
	void SetTranslateAndRotation(const XMFLOAT3& translate, const XMFLOAT3& rotation, float angle);
	void SetTranslateRotationAndScale(const XMFLOAT3& translate, const XMFLOAT3& rotation, float angle, const XMFLOAT3& scale);

private:
	void UpdateTransform();

private:
	XMFLOAT4X4 m_ModelMatrix = {};

	XMFLOAT3 m_Translate = XMFLOAT3(0.f, 0.f, 0.f);
	XMFLOAT3 m_Rotation = XMFLOAT3(0.f, 0.f, 0.f);		// Pitch, Yaw, Roll
	XMFLOAT3 m_Scale = XMFLOAT3(1.f, 1.f, 1.f);

	float m_Angle = 0.f;
};
#endif