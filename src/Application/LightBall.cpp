#include "LightBall.hpp"
#include "../Engine/Core/Time.hpp"
#include <math.h>

LightBall::LightBall(PrefabInstance* instance, float zPos, float timeOffset, float rotationLenght, float windingRadius, ColorID startColor, bool flipDirection, bool spawnLight)
{
	m_instance = instance;
	m_YPos = zPos;
	m_CurrentTime = timeOffset;
	m_TimeOffset = timeOffset;
	m_RotationLenght = rotationLenght;
	m_WindingRadius = windingRadius;
	m_CurrentColorID = startColor;
	m_FlipDirection = flipDirection;

	m_instance->GetTransform()->SetRotation(XMFLOAT3(1, 1, 1), 45);
	//m_instance->GetTransform()->SetScale(XMFLOAT3(10, 10, 10));

	if (spawnLight)
	{
		m_HasLight = true;

		m_Lights = Engine::Get()->GetLights();
		m_lightID = m_Lights->GetNumberOfPointLights();

		m_Lights->CreatePointLight(PointLight(XMFLOAT4(0.f, 1.f, 1.f, 0.f), XMFLOAT3(0.f, 0.f, 0.f), 0.f));
		m_Lights->SetPointLightStrength(1, m_lightID);
		m_Lights->SetPointLightRadius(25, m_lightID);
	}


}

LightBall::~LightBall()
{
	m_instance->Destroy();

}

void LightBall::Rotate(float dt)
{
	float speed = 0.05;
	m_CurrentTime += dt* speed;


	float x = m_WindingRadius * cos(m_CurrentTime);
	float z = m_WindingRadius * sin(m_CurrentTime);
	
	if (m_FlipDirection)
	{
		m_instance->GetTransform()->SetPosition(XMFLOAT3(z, x, m_YPos));
		if (m_HasLight)
		{
			m_Lights->SetPointLightPosition(XMFLOAT3(z, x, m_YPos), m_lightID);
		}
		
	}
	else
	{
		m_instance->GetTransform()->SetPosition(XMFLOAT3(x, m_YPos , z));
		if (m_HasLight)
		{
			m_Lights->SetPointLightPosition(XMFLOAT3(x , m_YPos , z), m_lightID);

		}
	}


	for (int i = 0; i < 3; i++)
	{
		 m_RGB[i] += dt *0.1 * (i + 1) + m_YPos * dt* 0.001;               //Pulsing
		//m_RGB[i] += dt * 0.1 * (i + 1) + m_TimeOffset * dt * 0.001;        //Epilepsy warning

		if (m_RGB[i] > 1)
		{
			m_RGB[i] = 0;
		}
	}

	if (m_HasLight)
	{
		m_Lights->SetPointLightColor(XMFLOAT3(m_RGB[0], m_RGB[1], m_RGB[2]), m_lightID);
	}

	m_instance->GetColor()->SetColor(m_RGB[0], m_RGB[1], m_RGB[2]);
}


