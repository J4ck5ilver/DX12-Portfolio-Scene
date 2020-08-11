#ifndef MOTHERLOADER_HPP
#define MOTHERLOADER_HPP

#include "AssetFiles/Mesh.hpp"
#include "AssetFiles/Texture.hpp"
#include "AssetFiles/Material.hpp"

#include<string>


#include <Assimp/scene.h>
#include <Assimp/Importer.hpp>
#include <Assimp/postprocess.h>
#include <Assimp/mesh.h>
#include <Assimp/vector3.h>


class Motherloader
{
private:

	struct saveVector
	{
		float position[4]{ 0,0,0,1 }; //pos
		float normal[4]{ 0,0,0,0 };
		float uv[4]{ 0,0,0,0 };
		float TBN[12]{0,0,0,0,0,0,0,0,0,0,0,0 };
	};


	static Motherloader* s_Instance;

	static UINT64 m_FenceValue;
	static ID3D12Fence1* m_Fence;
	//Public Funcs
public:
	
	static Motherloader* Get() noexcept;
	static void Shutdown() noexcept;

	//void LoadFromFolder(const std::string& FolderPath) noexcept; //Not Implemented
	//bool LoadFile(const std::string& FilePath) noexcept; // Should throw


	//For GpuResource
	bool LoadResourceToGPU(GpuResource* asset) noexcept;
	void RemoveResourceFromGPU(GpuResource* asset) noexcept;
	
	//For Assetmanager
	//Mesh* LoadMesh(const std::string& FilePath);
	std::vector<Mesh*> LoadMesh(const std::string& FilePath);
	Texture* LoadTexture(const std::string& FilePath);


	void LoadXML(const std::string& FilePath);


	//Private Funcs
private:
	//void DeleteTexture();


	Motherloader() noexcept;
	~Motherloader() noexcept;

	static void InitFence() noexcept;

	Motherloader(const Motherloader&) = delete;
	void operator=(const Motherloader&) = delete;

	void LoadMaterials(const std::string& FilePath);
	void LoadPipeLineStates(const std::string& FilePath);

	std::string CreateFileName(const std::string& FilePath) noexcept;
	
	bool FromCubeMapFolder(const std::string& FilePath) const noexcept;

	void GetBiTan(saveVector* SaveVec, int Index);

};
#endif // !MOTHERLOADER_HPP