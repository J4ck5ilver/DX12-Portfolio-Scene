#ifndef COLOR_H
#define COLOR_H
#include<DirectXMath.h>
#include"../Assetmanager/AssetFiles/GpuResource.hpp"

using namespace DirectX;

class Color :public  GpuResource
{
public:
	Color(float r = 1, float g = 1, float b = 1, float a = 1);
	virtual ~Color() = default;
	void operator=(Color& other);

	void SetColor(float r, float g, float b, float a = 0);
	XMFLOAT4* GetColor();

private:

	std::string m_ResourceName = "Color";
	XMFLOAT4 m_Color{ 0.f, 0.f, 0.f, 0.0f };

};
#endif // COLOR_H