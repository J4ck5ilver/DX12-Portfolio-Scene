#ifndef ASSETMANAGER_HPP
#define ASSETMANAGER_HPP
#include<unordered_map>
#include"AssetFile.hpp"

#include"AssetFiles/Mesh.hpp"
#include"AssetFiles/Texture.hpp"
#include"AssetFiles/Material.hpp"

#include<vector>


class Assetmanager
{
private:

#define FILEORDEREXTENTION "fileorder"


	struct LoadFileInfo
	{
		std::string FileName{ "" };
		std::string FilePath{ "" };
		std::string FileExtention{ "" };
	};



	friend class Motherloader;

	static Assetmanager* s_Instance;



	std::unordered_map<std::string, std::vector<LoadFileInfo>> m_LoadOrderMap;

	Assetmanager() noexcept;
	~Assetmanager() noexcept;

	Assetmanager(const Assetmanager&) = delete;
	void operator=(const Assetmanager&) = delete;


	std::unordered_map<std::string, AssetFile<Mesh>> m_Meshes;
	std::unordered_map<std::string, AssetFile<Texture>> m_Textures;
	std::unordered_map<std::string, Material> m_Materials;
	std::unordered_map<std::string, ID3D12PipelineState*> m_PipeLineStates;



public:

	static Assetmanager* Get() noexcept;
	void Shutdown() noexcept; // should  trow

	Mesh* GetMesh(const std::string& FileName) noexcept;
	Texture* GetTexture(const std::string& FileName) noexcept;
	Material* GetMaterial(const std::string& FileName) noexcept;
	ID3D12PipelineState*& GetPipeLineState(const std::string& passType) noexcept;


	void LoadFolder(const std::string& FolderPath, bool Sort); //should throw
	bool LoadFile(const std::string& FilePath, bool Reload = false) noexcept;

	void UploadMaterial(const std::string& materialType, const Material& material) noexcept;
	void UploadPipeLineState(std::string passType, ID3D12PipelineState* pipeLineState) noexcept;


private:

	bool AddMesh(const std::string& File, Mesh* texPtr) noexcept;
	bool AddTexture(const std::string& FilePath, Texture* texPtr) noexcept;

	void SortLoadOrder() noexcept;

	void FindFiles(const std::string& FolderPath) noexcept;


	void AddToLoadMap(const std::string& FilePath) noexcept;

	LoadFileInfo CreateFileInfo(const std::string& FilePath) noexcept;

	

};

#endif // !ASSETMANAGER_HPP

