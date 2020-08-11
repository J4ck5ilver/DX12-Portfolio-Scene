#include "Transform.hpp"

Transform::Transform() : GpuResource(TransformBuffer_Type)
{
	XMStoreFloat4x4(&m_ModelMatrix, XMMatrixIdentity());

	UINT sizeAligned = (sizeof(m_ModelMatrix) + 511) & ~511;	// 512-byte aligned.

	D3D12_BUFFER_SRV Buffsrv = {};
	Buffsrv.NumElements = 1;
	Buffsrv.FirstElement = 0;
	Buffsrv.StructureByteStride = sizeAligned;
	Buffsrv.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer = Buffsrv;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	GpuResource::ViewType temp;

	temp.SRV = srvDesc;

	SetViewType(temp);

	WriteDataOnGpu(&m_ModelMatrix, sizeof(m_ModelMatrix), GetMemeoryOffSet());

}


Transform::Transform(const Transform& obj)
{
	m_ModelMatrix = obj.m_ModelMatrix;
	m_Translate = obj.m_Translate;
	m_Rotation = obj.m_Translate;
	m_Scale = obj.m_Translate;
	m_Angle = obj.m_Angle;

	WriteDataOnGpu((void*)&GetTransform(), sizeof(m_ModelMatrix), GetMemeoryOffSet());

}

Transform Transform::operator=(const Transform& obj)
{
	if (this != &obj)
	{
		m_ModelMatrix = obj.m_ModelMatrix;
		m_Translate = obj.m_Translate;
		m_Rotation = obj.m_Translate;
		m_Scale = obj.m_Translate;
		m_Angle = obj.m_Angle;

		WriteDataOnGpu((void*)&GetTransform(), sizeof(m_ModelMatrix), GetMemeoryOffSet());


	}
	return *this;
}

const XMFLOAT4X4& Transform::GetTransform()
{
	UpdateTransform();
	return m_ModelMatrix;
}

const XMFLOAT3& Transform::GetPositionConst() const
{
	return m_Translate;
}

XMFLOAT3* Transform::GetPosition()
{
	return &m_Translate;
}

const XMFLOAT3& Transform::GetRotationConst() const
{
	return m_Rotation;
}

XMFLOAT3* Transform::GetRotation()
{
	return &m_Rotation;
}

const XMFLOAT3& Transform::GetScale() const
{
	return m_Scale;
}

void Transform::Translate(const XMFLOAT3& translate)
{
	m_Translate.x += translate.x;
	m_Translate.y += translate.y;
	m_Translate.z += translate.z;

	WriteDataOnGpu((void*)&GetTransform(), sizeof(m_ModelMatrix), GetMemeoryOffSet());
}

void Transform::Rotate(const XMFLOAT3& rotation, float angle)
{
	m_Rotation.x += rotation.x;
	m_Rotation.y += rotation.y;
	m_Rotation.z += rotation.z;

	WriteDataOnGpu((void*)&GetTransform(), sizeof(m_ModelMatrix), GetMemeoryOffSet());
}

void Transform::SetPosition(const XMFLOAT3& translate)
{
	m_Translate.x = translate.x;
	m_Translate.y = translate.y;
	m_Translate.z = translate.z;

	WriteDataOnGpu((void*)&GetTransform(), sizeof(m_ModelMatrix), GetMemeoryOffSet());
}

void Transform::SetRotation(const XMFLOAT3& rotation, float angle)
{
	m_Rotation.x = rotation.x;
	m_Rotation.y = rotation.y;
	m_Rotation.z = rotation.z;
	m_Angle = angle;

	WriteDataOnGpu((void*)&GetTransform(), sizeof(m_ModelMatrix), GetMemeoryOffSet());


}

void Transform::SetScale(const XMFLOAT3& scale)
{
	m_Scale.x = scale.x;
	m_Scale.y = scale.y;
	m_Scale.z = scale.z;

	WriteDataOnGpu((void*)&GetTransform(), sizeof(m_ModelMatrix), GetMemeoryOffSet());

}

void Transform::SetTranslateAndRotation(const XMFLOAT3& translate, const XMFLOAT3& rotation, float angle)
{
	SetPosition(translate);
	SetRotation(rotation, angle);

	WriteDataOnGpu((void*)&GetTransform(), sizeof(m_ModelMatrix), GetMemeoryOffSet());
}

void Transform::SetTranslateRotationAndScale(const XMFLOAT3& translate, const XMFLOAT3& rotation, float angle, const XMFLOAT3& scale)
{
	SetPosition(translate);
	SetRotation(rotation, angle);
	SetScale(scale);
}

void Transform::UpdateTransform()
{
	XMVECTOR rotation = XMVector3Normalize(XMLoadFloat3(&m_Rotation));

	// Because we're transposing the matrices here...
	XMStoreFloat4x4(&m_ModelMatrix, XMMatrixTranspose(
		XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z) *
		XMMatrixRotationNormal(rotation, XMConvertToRadians(m_Angle)) *
		XMMatrixTranslation(m_Translate.x, m_Translate.y, m_Translate.z)
		));


	// ... we need to transpose the matrix again to "flip" it to its original state
	XMStoreFloat4x4(&m_ModelMatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_ModelMatrix)));
}
