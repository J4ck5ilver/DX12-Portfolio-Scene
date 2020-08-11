#include "Lights.hpp"

Lights::Lights()
{
	m_LightsHeapBlock = new LightsHeapBlock();
}

Lights::~Lights()
{
	size_t nrOfElements = m_PointLights.size();
	for (size_t i = 0; i < nrOfElements; i++)
	{
		delete m_PointLights[i];
		m_PointLights[i] = nullptr;
	}

	nrOfElements = m_DirectionalLights.size();
	for (size_t i = 0; i < nrOfElements; i++)
	{
		delete m_DirectionalLights[i];
		m_DirectionalLights[i] = nullptr;
	}

	delete m_LightsHeapBlock;
	m_LightsHeapBlock = nullptr;
}

void Lights::SetData(PointLight* pointLight)
{
	pointLight->WriteDataOnGpu(&pointLight->LightAttributes, sizeof(pointLight->LightAttributes), pointLight->GetMemeoryOffSet());
}

void Lights::SetData(DirectionalLight* directionalLight)
{
	DirectionalLightAttributes light;
	light.ColorAndStrength = directionalLight->ColorAndStrength;
	light.Direction = directionalLight->Direction;

	directionalLight->WriteDataOnGpu(&light, sizeof(light), directionalLight->GetMemeoryOffSet());
}



/////   POINT LIGHTS   /////
PointLight* Lights::CreatePointLight(const PointLight& pointLight)
{
	// Create and store point light
	m_PointLights.emplace_back(new PointLight(pointLight.LightAttributes.ColorAndStrength, pointLight.LightAttributes.Position, pointLight.LightAttributes.Radius));

	// Set data on GPU
	SetData(m_PointLights.back());

	// Add point light resource
	m_LightsHeapBlock->AddPointLightResource(m_PointLights.back(), (size_t)5, m_PointLights.size() - 1);

	// Return point light pointer
	return m_PointLights.back();
}

void Lights::SetPointLightColor(const XMFLOAT3& color, const size_t index)
{
	m_PointLights[index]->LightAttributes.ColorAndStrength = XMFLOAT4(color.x, color.y, color.z, m_PointLights[index]->LightAttributes.ColorAndStrength.w);
	SetData(m_PointLights[index]);
}

void Lights::SetPointLightStrength(const float strength, const size_t index)
{
	m_PointLights[index]->LightAttributes.ColorAndStrength.w = strength;
	SetData(m_PointLights[index]);
}

void Lights::SetPointLightPosition(const XMFLOAT3& position, const size_t index)
{
	m_PointLights[index]->LightAttributes.Position = position;
	SetData(m_PointLights[index]);
}

void Lights::SetPointLightRadius(const float radius, const size_t index)
{
	m_PointLights[index]->LightAttributes.Radius = radius;
	SetData(m_PointLights[index]);
}

void Lights::SetPointLightAttributes(const XMFLOAT4& colorAndStrength, const XMFLOAT3 position, const float radius, const size_t index)
{
	delete m_PointLights[index];
	m_PointLights[index] = new PointLight(colorAndStrength, position, radius);
	SetData(m_PointLights[index]);
}

void Lights::SetPointLightAttributes(const XMFLOAT3& color, const float strength, const XMFLOAT3 position, const float radius, const size_t index)
{
	m_PointLights[index];
	m_PointLights[index] = new PointLight(color, strength, position, radius);
	SetData(m_PointLights[index]);
}

size_t Lights::GetNumberOfPointLights()
{
	return m_PointLights.size();
}

PointLight* Lights::GetPointLightAt(const size_t index)
{
	return m_PointLights[index];
}

const std::vector<PointLight*>* Lights::GetAllPointLights()
{
	return &m_PointLights;
}



/////   DIRECTIONAL LIGHTS   /////
DirectionalLight* Lights::CreateDirectionalLight(const DirectionalLight& directionalLight)
{
	m_DirectionalLights.emplace_back(new DirectionalLight(directionalLight.ColorAndStrength, directionalLight.Direction));
	return m_DirectionalLights.back();
}

void Lights::SetDirectionalLightColor(const XMFLOAT3& color, const size_t index)
{
	m_DirectionalLights[index]->ColorAndStrength = XMFLOAT4(color.x, color.y, color.z, m_DirectionalLights[index]->ColorAndStrength.w);
}

void Lights::SetDirectionalLightStrength(const float strength, const size_t index)
{
	m_DirectionalLights[index]->ColorAndStrength.w = strength;
}

void Lights::SetDirectionalLightDirection(const XMFLOAT3& direction, const size_t index)
{
	m_DirectionalLights[index]->Direction = direction;
}

void Lights::SetDirectionalLightAttributes(const XMFLOAT4& colorAndStrength, const XMFLOAT3& direction, const size_t index)
{
	delete m_DirectionalLights[index];
	m_DirectionalLights[index] = new DirectionalLight(colorAndStrength, direction);
}

void Lights::SetDirectionalLightAttributes(const XMFLOAT3& color, const float strength, const XMFLOAT3& direction, const size_t index)
{
	delete m_DirectionalLights[index];
	m_DirectionalLights[index] = new DirectionalLight(color, strength, direction);
}

size_t Lights::GetNumberOfDirectionalLights()
{
	return m_DirectionalLights.size();
}

DirectionalLight* Lights::GetDirectionalLightAt(size_t index)
{
	return m_DirectionalLights[index];
}

const std::vector<DirectionalLight*>* Lights::GetAllDirectionalLights()
{
	return &m_DirectionalLights;
}