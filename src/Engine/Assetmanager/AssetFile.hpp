#ifndef ASSETFILE_HPP
#define ASSETFILE_HPP

#include<string>


#define FE_JPG "jpg"
#define FE_DDS "dds"
#define FE_PNG "png"
#define FE_OBJ "obj"
#define FE_MAT "mat"
#define FE_XML "xml"

template<typename T>
class AssetFile
{
public:
	enum class FileType
	{
		EmptyType = 0,
		MeshType = 1,
		TextureType = 2,
		MaterialType = 3,
		XMLType = 4
	};



private:

	//may not be needed
	//bool m_On_GPU;
	//bool m_On_CPU;

	T** m_Data;

	FileType m_Type;

	std::string m_FileName;
	std::string m_FilePath;
	std::string m_FileExtension;

	friend class Assetmanager;
public:


	AssetFile(const std::string& FilePath = "") noexcept;
	~AssetFile() noexcept;

	AssetFile(const AssetFile& other) noexcept;
	AssetFile& operator=(const AssetFile& other) noexcept;

	const bool& OnCPU() noexcept;

	//May not be needed (if needed, inheritance to other classes?)
	//const bool& OnGPU() noexcept;

	inline T* GetData() noexcept { return *m_Data; }

	inline const FileType& GetType() const noexcept { return m_Type; }

	inline const std::string& GetFileName() noexcept { return m_FileName; }
	inline const std::string& GetFilePath() noexcept { return m_FilePath; }
	inline const std::string& GetFileExtension() noexcept { return m_FileExtension; }

private:

	void Shutdown();
	void RemoveFromCPU();
	void SetDataPtr(T* dataPtr);
	const void SetInfoFromPath() noexcept; // Should throw warning
	FileType GetFileType();
	//const void SetFileName() noexcept; // Should throw
	

};




template<typename T>
AssetFile<T>::AssetFile(const std::string& FilePath) noexcept :
	m_FileExtension(""),
	m_FilePath(FilePath),
	m_Data(nullptr)
{
	SetInfoFromPath();


	if (m_Type != FileType::EmptyType)
	{
		m_Data = new T* {nullptr};
		//*m_Data = nullptr;
	}

}


template<typename T>
AssetFile<T>::~AssetFile() noexcept
{

	//Use shutdown at the end



	//if (m_Type == FileType::EmptyType)
	//{
	//	delete m_Data;
	//	m_Data = nullptr;
	//}

	

	//if (m_Data)
	//{
	//	//To Do
	//	//other assetfiles of same type will delete this. (made as copy)

	//	delete m_Data;
	//	m_Data = nullptr;
	//}

}

template<typename T>
AssetFile<T>::AssetFile(const AssetFile& other) noexcept
{

	m_Data = other.m_Data;
	m_Type = other.m_Type;
	m_FilePath = other.m_FilePath;
	m_FileName = other.m_FileName;
	m_FileExtension = other.m_FileExtension;

}

template<typename T>
AssetFile<T>& AssetFile<T>::operator=(const AssetFile& other)  noexcept
{
	if (this != &other)
	{

		m_Data = other.m_Data;
		m_Type = other.m_Type;
		m_FilePath = other.m_FilePath;
		m_FileName = other.m_FileName;
		m_FileExtension = other.m_FileExtension;


	}
	return *this;
}

template<typename T>
const bool& AssetFile<T>::OnCPU() noexcept
{
	bool returnValue = true;

	if (*m_Data == nullptr)
	{
		returnValue = false;
	}

	return returnValue;


//	return *m_Data;

}


template<typename T>
typename AssetFile<T>::FileType AssetFile<T>::GetFileType()
{
	AssetFile<T>::FileType returnValue(AssetFile<T>::FileType::EmptyType);

	if (m_FileExtension == FE_JPG || m_FileExtension == FE_PNG || m_FileExtension == FE_DDS)
	{
		returnValue = AssetFile<T>::FileType::TextureType;
	}
	else if (m_FileExtension == FE_OBJ)
	{
		returnValue = AssetFile<T>::FileType::MeshType;
	}
	else if (m_FileExtension == FE_MAT)
	{
		returnValue = AssetFile<T>::FileType::MaterialType;
	}
	else if (m_FileExtension == FE_XML)
	{
		returnValue = AssetFile<T>::FileType::XMLType;
	}

	return returnValue;
}

template<typename T>
inline void AssetFile<T>::Shutdown()
{
	if (m_Data)
	{
		if (*m_Data)
		{
			delete* m_Data;
			*m_Data = nullptr;
		}

		delete m_Data;
		m_Data = nullptr;
	}
}

template<typename T>
void AssetFile<T>::RemoveFromCPU()
{
	if (*m_Data)
	{
		delete* m_Data;
		*m_Data = nullptr;
	}
	else
	{

		//RemoveIfNotNeeded

		//Debug if getting here
		__debugbreak();
	}
}

template<typename T>
void AssetFile<T>::SetDataPtr(T* dataPtr)
{
	if (*m_Data)
	{
		RemoveFromCPU();
	}
	*m_Data = dataPtr;
}

template<typename T>
const void AssetFile<T>::SetInfoFromPath() noexcept
{

	int16_t counter = m_FilePath.size() - 1;

	//Simple Design (To Do: Add checks)

	while (counter >= 0 && m_FilePath[counter] != '.')
	{
		m_FileExtension += m_FilePath[counter];
		counter--;
	}
	std::reverse(m_FileExtension.begin(), m_FileExtension.end());

	if (counter != 0)
	{
		counter--;
	}

	while (counter >= 0 && m_FilePath[counter] != '/' && m_FilePath[counter] != '\\' )
	{
		m_FileName += m_FilePath[counter];
		counter--;
	}
	std::reverse(m_FileName.begin(), m_FileName.end());


	m_Type = GetFileType();




	////fast fix for meshes
	//if (m_FileName == "")
	//{
	//	if (m_FilePath.size() != 0)
	//	{
	//		if (m_FilePath[0] != '.')
	//		{
	//			m_FileName = m_FilePath;
	//			m_Type = AssetFile<T>::FileType::MeshType;
	//		}
	//	}
	//}




}


#endif // !ASSETFILE_HPP