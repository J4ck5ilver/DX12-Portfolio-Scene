#include "Material.hpp"

Material::Material(std::string name, Texture* diffuse, Texture* normalMap, Texture* specularMap, ID3D12PipelineState* pipeLineState)
{
	Name = name;
	Diffuse = diffuse;
	NormalMap = normalMap;
	SpecularMap = specularMap;
	PipeLineState = pipeLineState;
}

Material::~Material()
{
}

Material::Material(const Material& obj)
{
	Name = obj.Name;
	Diffuse = obj.Diffuse;
	NormalMap = obj.NormalMap;
	SpecularMap = obj.SpecularMap;
	PipeLineState = obj.PipeLineState;
}

Material& Material::operator=(const Material& obj)
{
	if (this != &obj)
	{
		Name = obj.Name;
		Diffuse = obj.Diffuse;
		NormalMap = obj.NormalMap;
		SpecularMap = obj.SpecularMap;
		PipeLineState = obj.PipeLineState;
	}
	return *this;
}
