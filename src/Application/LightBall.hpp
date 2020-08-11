
#ifndef LIGHTBALL_H
#define LIGHTBALL_H

#include"../Engine/Engine.hpp"

enum ColorID
{
	Red = 0,
	Green = 1,
	Blue = 2
};
class LightBall
{



public:
	LightBall(PrefabInstance* instance, float zPos = 0.0f, float timeOffset = 0.0f, float rotationLenght = 0.0f, float windingRadius = 0.0f, ColorID startColor = Red, 
		bool flipDirection = false, bool spawnLight = false);
	virtual ~LightBall();

	void Rotate(float dt);
private:


private:

	PrefabInstance* m_instance = nullptr;

	float m_YPos = 0.0f;
	float m_RotationLenght = 0.0f;
	float m_WindingRadius = 0.0f;
	float m_CurrentTime = 0.0f;
	float m_TimeOffset = 0.0f;
	float m_RGB[3]{ 0,0,0 };
	int m_CurrentColorID = 0;

	float m_ColorDelay = 0.5f;
	float m_CurrentCollorTime = 0.0f;

	bool m_FlipDirection = false;

	bool m_HasLight = false;
	Lights* m_Lights = nullptr;
	unsigned int m_lightID = 0;

};



#endif // !LIGHTBALL_H