#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include "Texture.hpp"
#include "PipelineState.hpp"

struct Material
{
	Material() = default;
	Material(std::string name, Texture* diffuse, Texture* normalMap,Texture* specularMap, ID3D12PipelineState* pipeLineState);

	~Material();

	Material(const Material& obj);
	Material& operator=(const Material& obj);

private:
	template<class Interface>
	inline void SafeRelease(Interface** ppInterfaceToRelease)
	{
		if (*ppInterfaceToRelease != NULL)
		{
			(*ppInterfaceToRelease)->Release();
			(*ppInterfaceToRelease) = NULL;
		}
	}

public:
	std::string Name{ "" };
	Texture* Diffuse = nullptr;
	Texture* NormalMap = nullptr;
	Texture* SpecularMap = nullptr;
	ID3D12PipelineState* PipeLineState = nullptr;

};
#endif // !MATERIAL_HPP