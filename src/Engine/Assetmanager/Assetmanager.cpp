#include "Assetmanager.hpp"
#include "MotherLoader.hpp"

#include<filesystem>
#include <fstream>
#include <iostream>

Assetmanager* Assetmanager::s_Instance = nullptr;

void Assetmanager::LoadFolder(const std::string& FolderPath, bool Sort)
{

	m_LoadOrderMap.clear();

	//Adds path to the m_LoadOrderMap.
	FindFiles(FolderPath);


	if (Sort)
	{
		// Uses the .fileorder file to sort m_LoadOrderMap after the types of files in a loading order.
		SortLoadOrder();
	}

	std::unordered_map<std::string, std::vector<LoadFileInfo>>::iterator it = m_LoadOrderMap.begin();

	while (it != m_LoadOrderMap.end())
	{
		for (int i = 0; i < it->second.size(); i++)
		{
			LoadFile(it->second[i].FilePath);
		}
		it++;
	}
}

bool Assetmanager::LoadFile(const std::string& FilePath, bool Reload) noexcept
{
	bool returnValue{ false };

	AssetFile<void> LoadFile(FilePath);

	switch (LoadFile.GetType())
	{
	case AssetFile<void>::FileType::MeshType:
	{

		std::vector<Mesh*> meshVector = Motherloader::Get()->LoadMesh(FilePath);
		for (int i = 0; i < meshVector.size(); i++)
		{

			AssetFile<Mesh>* TmpFilePtr{ &m_Meshes[meshVector[i]->GetName()] };

			if (TmpFilePtr->GetType() == AssetFile<Mesh>::FileType::EmptyType)
			{
				m_Meshes[meshVector[i]->GetName()] = AssetFile<Mesh>(meshVector[i]->GetName() + "." + FE_OBJ);
			}

			if (!TmpFilePtr->OnCPU() || Reload)
			{
				returnValue = true;
				TmpFilePtr->SetDataPtr(meshVector[i]);
			}
		}
	}
	break;

	case AssetFile<void>::FileType::TextureType:
	{
		Texture* TmpTexPtr{ nullptr };
		AssetFile<Texture>* TmpFilePtr{ &m_Textures[LoadFile.GetFileName()] };

		if (TmpFilePtr->GetType() == AssetFile<Texture>::FileType::EmptyType)
		{
			m_Textures[LoadFile.GetFileName()] = AssetFile<Texture>(FilePath);
		}
		if (!TmpFilePtr->OnCPU() || Reload)
		{
			
			TmpTexPtr = Motherloader::Get()->LoadTexture(FilePath); //Delete if reload?
			if (TmpTexPtr)
			{
				returnValue = true;
				TmpFilePtr->SetDataPtr(TmpTexPtr);
			}
		}
	}
	break;

	case AssetFile<void>::FileType::MaterialType:

		break;


	case AssetFile<void>::FileType::XMLType:
	{
		Motherloader::Get()->LoadXML(FilePath);
	}
	break;

	default:

		break;
	}

	LoadFile.Shutdown();

	return returnValue;
}




void Assetmanager::SortLoadOrder() noexcept
{



	if (!m_LoadOrderMap[FILEORDEREXTENTION].empty())
	{

		std::ifstream sortFile;

		sortFile.open(m_LoadOrderMap[FILEORDEREXTENTION][0].FilePath);

		std::list<LoadFileInfo> exceptions;


		if (sortFile.is_open())
		{

			std::string line;
			while (!sortFile.eof())
			{
				std::getline(sortFile, line);
				exceptions.emplace_back(CreateFileInfo(line));
			}

			sortFile.close();
			m_LoadOrderMap.erase(FILEORDEREXTENTION);
			bool OneFile;

			while (!exceptions.empty())
			{
				if (exceptions.front().FileName != "")
				{
					OneFile = true;
				}
				else
				{
					OneFile = false;
				}

				for (int i = 0; i < m_LoadOrderMap[exceptions.front().FileExtention].size(); i++)
				{
					if (OneFile)
					{
						if (m_LoadOrderMap[exceptions.front().FileExtention][i].FileName == exceptions.front().FileName)
						{
							LoadFile(m_LoadOrderMap[exceptions.front().FileExtention][i].FilePath);
							m_LoadOrderMap[exceptions.front().FileExtention].erase(m_LoadOrderMap[exceptions.front().FileExtention].begin() + i);
							break;
						}
					}
					else
					{
						LoadFile(m_LoadOrderMap[exceptions.front().FileExtention][i].FilePath);
					}
				}


				if (!OneFile)
				{
					m_LoadOrderMap.erase(exceptions.front().FileExtention);
				}

				exceptions.pop_front();
			}
		}
		else
		{

		}


	}
}

void Assetmanager::FindFiles(const std::string& FolderPath) noexcept
{

	for (const auto& dirEntry : std::filesystem::directory_iterator(FolderPath))
	{
		if (dirEntry.is_directory())
		{
			FindFiles((FolderPath + "/" + dirEntry.path().filename().string()));
		}
		else
		{
			AddToLoadMap(dirEntry.path().string());
		}
	}
}

void Assetmanager::AddToLoadMap(const std::string& FilePath) noexcept
{
	LoadFileInfo tempInfo = CreateFileInfo(FilePath);

	if (tempInfo.FileExtention != "")
	{
		m_LoadOrderMap[tempInfo.FileExtention].emplace_back(tempInfo);
	}
}

//Takes in a file path and divide it into separate information. Name, Path, Extention.
Assetmanager::LoadFileInfo Assetmanager::CreateFileInfo(const std::string& FilePath) noexcept
{

	LoadFileInfo returnValue;

	int16_t counter = FilePath.size() - 1;

	//Simple Design (To Do: Add checks)

	while (counter >= 0 && FilePath[counter] != '.')
	{
		returnValue.FileExtention += FilePath[counter];
		counter--;
	}
	std::reverse(returnValue.FileExtention.begin(), returnValue.FileExtention.end());

	if (counter != -1)
	{
		counter--;
	}

	while (counter >= 0 && FilePath[counter] != '/' && FilePath[counter] != '\\')
	{
		returnValue.FileName += FilePath[counter];
		counter--;
	}
	std::reverse(returnValue.FileName.begin(), returnValue.FileName.end());


	returnValue.FilePath = FilePath;



	return returnValue;
}

void Assetmanager::UploadMaterial(const std::string& materialType, const Material& material) noexcept
{
	// OOps! We're not checking if there already exists a pointer in this position
	m_Materials[materialType] = material;
}

void Assetmanager::UploadPipeLineState(std::string passType, ID3D12PipelineState* pipeLineState) noexcept
{
	// OOps! We're not checking if there already exists a pointer in this position
	m_PipeLineStates[passType] = pipeLineState;
}

Assetmanager* Assetmanager::Get() noexcept
{
	if (!s_Instance)
	{
		s_Instance = new Assetmanager();
	}
	return s_Instance;
}

void Assetmanager::Shutdown() noexcept
{
	if (s_Instance != nullptr)
	{

		std::unordered_map<std::string, AssetFile<Mesh>>::iterator itMe = m_Meshes.begin();
		while (itMe != m_Meshes.end())
		{
			itMe->second.Shutdown();
			itMe++;
		}


		std::unordered_map<std::string, AssetFile<Texture>>::iterator itTe = m_Textures.begin();
		while (itTe != m_Textures.end())
		{
			itTe->second.Shutdown();
			itTe++;
		}

		std::unordered_map<std::string, ID3D12PipelineState*>::iterator itPi = m_PipeLineStates.begin();
		while (itPi != m_PipeLineStates.end())
		{
			if (itPi->second != nullptr)
			{
				itPi->second->Release();
			}
			itPi++;
		}

		Motherloader::Get()->Shutdown();

		delete s_Instance;
		s_Instance = nullptr;
	}
}

Mesh* Assetmanager::GetMesh(const std::string& FileName) noexcept
{
	return m_Meshes[FileName].GetData();
}

Texture* Assetmanager::GetTexture(const std::string& FileName) noexcept
{
	Texture* returnValue = nullptr;

	if (m_Textures[FileName].GetFileType() == AssetFile<Texture>::FileType::TextureType)
	{
		if (m_Textures[FileName].OnCPU())
		{
			returnValue = m_Textures[FileName].GetData();
		}
		else
		{
			
		}
	}

	return returnValue;
}

Material* Assetmanager::GetMaterial(const std::string& FileName) noexcept
{
	return &m_Materials[FileName];
}

ID3D12PipelineState*& Assetmanager::GetPipeLineState(const std::string& passType) noexcept
{
	return m_PipeLineStates[passType];
}

Assetmanager::Assetmanager() noexcept
{

}

Assetmanager::~Assetmanager() noexcept
{
}
