#ifndef LIGHTS_HPP
#define LIGHTS_HPP

#include "LightsHeapBlock.hpp"

#include <DirectXMath.h>
#include <vector>

using namespace DirectX;
struct PointLightAttributes	// Must be written to GPU after every change of data, meaning all set functions
{
	XMFLOAT4 ColorAndStrength = { 1.f, 1.f, 1.f, 1.f };
	XMFLOAT3 Position = { 1.f, 1.f, 1.f };
	float Radius = { 10.f };
};

struct PointLight : GpuResource
{
	PointLightAttributes LightAttributes;

	PointLight(XMFLOAT4 colorAndStrength = { 1.f, 1.f, 1.f, 1.f }, XMFLOAT3 position = { 0.f, 0.f, 0.f }, float radius = 10.f) 
		: GpuResource(PointLight_Type, true)
	{
		LightAttributes.ColorAndStrength = colorAndStrength;
		LightAttributes.Position = position;
		LightAttributes.Radius = radius;
	};
	
	PointLight(XMFLOAT3 color = { 1.f, 1.f, 1.f }, float strength = 1.f, XMFLOAT3 position = { 0.f, 0.f, 0.f }, float radius = 10.f) 
		: GpuResource(PointLight_Type, true) 
	{
		LightAttributes.ColorAndStrength = XMFLOAT4(color.x, color.y, color.z, strength);
		LightAttributes.Position = position;
		LightAttributes.Radius = radius;
	};
	PointLight(const PointLight& obj)
	{
		LightAttributes.ColorAndStrength = obj.LightAttributes.ColorAndStrength;
		LightAttributes.Radius = obj.LightAttributes.Radius;
	}
	PointLight operator=(const PointLight& obj)
	{
		if (this != &obj)
		{
			LightAttributes.ColorAndStrength = obj.LightAttributes.ColorAndStrength;
			LightAttributes.Radius = obj.LightAttributes.Radius;
		}
		return *this;
	}
};

struct DirectionalLight : GpuResource
{
	XMFLOAT4 ColorAndStrength = { 1.f, 1.f, 1.f, 1.f };
	XMFLOAT3 Direction = { 0.f, 0.f, 0.f };

	DirectionalLight(XMFLOAT4 colorAndStrength = { 1.f, 1.f, 1.f, 1.f }, XMFLOAT3 direction = { 0.f, 0.f, 0.f }) : ColorAndStrength(colorAndStrength), Direction(direction) {};
	DirectionalLight(XMFLOAT3 color = { 1.f, 1.f, 1.f }, float strength = 1.f, XMFLOAT3 direction = { 0.f, 0.f, 0.f }) : ColorAndStrength(XMFLOAT4(color.x, color.y, color.z, strength)), Direction(direction) {};
	DirectionalLight(const DirectionalLight& obj)
	{
		ColorAndStrength = obj.ColorAndStrength;
		Direction = obj.Direction;
	}
	DirectionalLight operator=(const DirectionalLight& obj)
	{
		if (this != &obj)
		{
			ColorAndStrength = obj.ColorAndStrength;
			Direction = obj.Direction;
		}
		return *this;
	}
};

struct DirectionalLightAttributes	// Must be written to GPU after every change of data, meaning all set functions
{
	XMFLOAT4 ColorAndStrength = { 1.f, 1.f, 1.f, 1.f };
	XMFLOAT3 Direction = { 0.f, 0.f, 0.f };
	float padding{ 0.f };
};

class Lights
{
public:
	Lights();
	virtual ~Lights();

	Lights(const Lights& obj) = delete;
	Lights operator=(const Lights& obj) = delete;

	void SetData(PointLight* pointLight);
	void SetData(DirectionalLight* directionalLight);

	// Get Heapblock
	inline LightsHeapBlock* GetLightHeapBlock() { return m_LightsHeapBlock; }



	/////   POINT LIGHTS   /////
	PointLight* CreatePointLight(const PointLight& pointLight);
	
	void SetPointLightColor(const XMFLOAT3& color, const size_t index);
	void SetPointLightStrength(const float strength, const size_t index);
	void SetPointLightPosition(const XMFLOAT3& position, const size_t index);
	void SetPointLightRadius(const float radius, const size_t index);
	void SetPointLightAttributes(const XMFLOAT4& colorAndStrength, const XMFLOAT3 position, const float radius, const size_t index);
	void SetPointLightAttributes(const XMFLOAT3& color, const float strength, const XMFLOAT3 position, const float radius, const size_t index);

	size_t GetNumberOfPointLights();
	PointLight* GetPointLightAt(const size_t index);
	const std::vector<PointLight*>* GetAllPointLights();	



	/////   DIRECTIONAL LIGHTS   /////	Is not currently being used
	DirectionalLight* CreateDirectionalLight(const DirectionalLight& directionalLight);
	
	void SetDirectionalLightColor(const XMFLOAT3& color, const size_t index);
	void SetDirectionalLightStrength(const float strength, const size_t index);
	void SetDirectionalLightDirection(const XMFLOAT3& direction, const size_t index);
	void SetDirectionalLightAttributes(const XMFLOAT4& colorAndStrength, const XMFLOAT3& direction, const size_t index);
	void SetDirectionalLightAttributes(const XMFLOAT3& color, const float strength, const XMFLOAT3& direction, const size_t index);
	
	size_t GetNumberOfDirectionalLights();
	DirectionalLight* GetDirectionalLightAt(size_t index);
	const std::vector<DirectionalLight*>* GetAllDirectionalLights();

private:
	std::vector<PointLight*> m_PointLights;
	std::vector<DirectionalLight*> m_DirectionalLights;

	LightsHeapBlock* m_LightsHeapBlock = nullptr;

	PointLightAttributes m_PointLightAttributes;
};
#endif