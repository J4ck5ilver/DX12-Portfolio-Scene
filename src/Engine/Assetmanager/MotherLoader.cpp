#include "MotherLoader.hpp"
#include "Assetmanager.hpp"
#include "..\RenderEngine\Renderer.hpp"
#include "Assetmanager.hpp"

#include <DirectX12/ResourceUploadBatch.h>
#include <DirectX12/WICTextureLoader.h>
#include <PugiXML/pugixml.hpp>

//#include <Assimp/scene.h>
//#include <Assimp/Importer.hpp>
//#include <Assimp/postprocess.h>
//#include <Assimp/mesh.h>
//#include <Assimp/vector3.h>
#include <DirectXMath.h>


#include<DirectX12/DDSTextureLoader.h>

#include <iostream>

Motherloader* Motherloader::s_Instance = nullptr;
UINT64 Motherloader::m_FenceValue = 0;
ID3D12Fence1* Motherloader::m_Fence = nullptr;

Motherloader* Motherloader::Get() noexcept
{
	if (!s_Instance)
	{
		s_Instance = new Motherloader();
		InitFence();
	}
	return s_Instance;
}

void Motherloader::Shutdown() noexcept
{
	if (s_Instance != nullptr)
	{
		m_Fence->Release();
		delete s_Instance;
		s_Instance = nullptr;
	}
}





//For Assetmanager
std::vector<Mesh*> Motherloader::LoadMesh(const std::string& FilePath)
{

	std::vector<Mesh*> returnValue;

	Assimp::Importer asImporter;

	const aiScene* TempScene = asImporter.ReadFile(FilePath, 0);

	if (TempScene)
	{
		int nrOfVerticies;
		saveVector* copyVecArray = nullptr;

		//size in bytes hard coded, need to change size between different typs of meshes?
		int posByteSize = sizeof(aiVector3D);
		int normByteSize = sizeof(aiVector3D);
		int uvSize = sizeof(float) * 2;

		for (unsigned int i = 0; i < TempScene->mNumMeshes; i++)
		{

			//Default object at init?? 
			std::string InitName;
			int sizeOfName = TempScene->mMeshes[i]->mName.length;
			InitName.resize(sizeOfName);
			memcpy(&InitName[0], TempScene->mMeshes[i]->mName.C_Str(), sizeOfName);

			nrOfVerticies = TempScene->mMeshes[i]->mNumVertices;
			copyVecArray = new saveVector[nrOfVerticies];
			Mesh* tempMeshPtr = new Mesh(InitName, nrOfVerticies);


			for (int j = 0; j < nrOfVerticies; j ++)
			{

					if (TempScene->mMeshes[i]->HasPositions())
					{
						memcpy(copyVecArray[j].position, &TempScene->mMeshes[i]->mVertices[j], posByteSize);
					}

					//ToDo: Add else and calculate the normals.
					if (TempScene->mMeshes[i]->HasNormals())
					{
						memcpy(copyVecArray[j].normal, &TempScene->mMeshes[i]->mNormals[j], normByteSize);


					}

					if (TempScene->mMeshes[i]->HasTextureCoords(0))
					{
						memcpy(copyVecArray[j].uv, &TempScene->mMeshes[i]->mTextureCoords[0][j], sizeof(float) * 2);
					}

			}


			// Ugly but simple solution (All info had to be loaded for BiTan calculations)
			for (int j = 0; j < nrOfVerticies; j++)
			{
				if (TempScene->mMeshes[i]->HasTextureCoords(0))
				{
					GetBiTan(copyVecArray, j);
				}
			}


			//check for asset inside assetmanager and load material / textures?
			//Load mesh

			int sizeInBytes = sizeof(saveVector) * nrOfVerticies;
			UINT cbSizeAligned = (sizeInBytes + 511) & ~511;	// 512-byte aligned.


			//Set up temp heap for copy

			ID3D12Resource* tempRecource;

			D3D12_RESOURCE_DESC resourceDesc = {};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Width = cbSizeAligned;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;


			D3D12_HEAP_PROPERTIES heapProperties = {};
			heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.CreationNodeMask = 1;
			heapProperties.VisibleNodeMask = 1;
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;



			//Update GPU memory

			Renderer::Get()->GetDevice()->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&tempRecource));

			tempRecource->SetName(L"Copy Heap");


			//Copy data to temp heap

			void* mappedMem = nullptr;
			D3D12_RANGE readRange = { 0, 0 };
			int readRangeInBytes = nrOfVerticies * sizeof(float);

			if (SUCCEEDED(tempRecource->Map(0, &readRange, &mappedMem)))
			{
				memcpy(mappedMem, &copyVecArray[0], sizeInBytes);

				D3D12_RANGE writeRange = { 0, sizeInBytes };
				tempRecource->Unmap(0, &writeRange);
			}



			//Set up copy destination

			D3D12_RESOURCE_DESC finalResourceDesc = {};
			finalResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			finalResourceDesc.Width = cbSizeAligned;
			finalResourceDesc.Height = 1;
			finalResourceDesc.DepthOrArraySize = 1;
			finalResourceDesc.MipLevels = 1;
			finalResourceDesc.SampleDesc.Count = 1;
			finalResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;


			D3D12_HEAP_PROPERTIES finalHeapProperties = {};
			finalHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
			finalHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			finalHeapProperties.CreationNodeMask = 1;
			finalHeapProperties.VisibleNodeMask = 1;
			finalHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;



			if (SUCCEEDED(Renderer::Get()->GetDevice()->CreateCommittedResource(
				&finalHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&finalResourceDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&tempMeshPtr->GetResources()->at(0)))))
			{
				

				std::wstring tempWstring(InitName.begin(), InitName.end());

				tempMeshPtr->GetResources()->at(0)->SetName(tempWstring.c_str());


				//Executing copy

				ID3D12GraphicsCommandList4* comList = Renderer::Get()->GetCommandList(CommandType::Direct);

				comList->Reset(Renderer::Get()->GetCommandAllocator(CommandType::Direct), nullptr);

				comList->CopyResource(tempMeshPtr->GetResources()->at(0), tempRecource);


				//(Future improvment, should be a func in renderer?)
				D3D12_RESOURCE_BARRIER barrierDesc = {};

				barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrierDesc.Transition.pResource = tempMeshPtr->GetResources()->at(0);
				barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
				barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

				comList->ResourceBarrier(1, &barrierDesc);


				comList->Close();

				ID3D12CommandList* listsToExecute[] = { comList };
				Renderer::Get()->GetCommandQueue(CommandType::Direct)->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);



				//Fence wait

				m_FenceValue++;
				Renderer::Get()->GetCommandQueue(CommandType::Direct)->Signal(m_Fence, m_FenceValue);
				while (m_Fence->GetCompletedValue() < m_FenceValue);


				delete[] copyVecArray;
				copyVecArray = nullptr;

				tempRecource->Release();
				tempRecource = nullptr;



				//Create view and save to the GpuResource base class for later use

				D3D12_BUFFER_SRV Buffsrv = {};
				Buffsrv.NumElements = 1;
				Buffsrv.FirstElement = 0;
				Buffsrv.StructureByteStride = cbSizeAligned;
				Buffsrv.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = finalResourceDesc.Format;
				srvDesc.Buffer = Buffsrv;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;


				GpuResource::ViewType tempViewType;
				tempViewType.SRV = srvDesc;

				tempMeshPtr->SetViewType(tempViewType);


				returnValue.push_back(tempMeshPtr);

			}
			else
			{
				//Failed to create resource,
				__debugbreak();
			}
		}
	}

	return returnValue;
}

Texture* Motherloader::LoadTexture(const std::string& FilePath)
{

	Texture* tempTexture = new Texture();

	DirectX::ResourceUploadBatch RUB(Renderer::Get()->GetDevice());

	RUB.Begin();

	std::wstring widestr = std::wstring(FilePath.begin(), FilePath.end());

	HRESULT status;

	if (FromCubeMapFolder(FilePath))
	{

		status = DirectX::CreateDDSTextureFromFile(
			Renderer::Get()->GetDevice(),
			RUB,
			widestr.c_str(),
			&tempTexture->GetResources()->at(0));

	}
	else
	{

		status = DirectX::CreateWICTextureFromFile(
			Renderer::Get()->GetDevice(),
			RUB,
			widestr.c_str(),
			&tempTexture->GetResources()->at(0));
	}



	if (status != S_OK)
	{
		std::cout << "Error could not load texture " << FilePath << std::endl;
		__debugbreak();
	}


	if (SUCCEEDED(status))
	{
		D3D12_RESOURCE_DESC tempDesc = tempTexture->GetResources()->at(0)->GetDesc();

		GpuResource::ViewType viewDesc;
		viewDesc.SRV = D3D12_SHADER_RESOURCE_VIEW_DESC();


		viewDesc.SRV.Format = tempDesc.Format;
		if (FromCubeMapFolder(FilePath))
		{
			D3D12_TEXCUBE_SRV tempTexCubeSRV = {};

			tempTexCubeSRV.ResourceMinLODClamp = 0;
			tempTexCubeSRV.MostDetailedMip = 0;
			tempTexCubeSRV.MipLevels = 1;

			viewDesc.SRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			viewDesc.SRV.TextureCube = tempTexCubeSRV;
		}
		else
		{

			D3D12_TEX2D_SRV tempTex2DSRV = {};

			tempTex2DSRV.ResourceMinLODClamp = 0;
			tempTex2DSRV.MostDetailedMip = 0;
			tempTex2DSRV.PlaneSlice = 0;
			tempTex2DSRV.MipLevels = tempDesc.MipLevels;


			viewDesc.SRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			viewDesc.SRV.Texture2D = tempTex2DSRV;
		}

		viewDesc.SRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		tempTexture->SetViewType((const GpuResource::ViewType)viewDesc);
		tempTexture->SetWidth(tempTexture->GetResources()->at(0)->GetDesc().Width);
		tempTexture->SetHeight(tempTexture->GetResources()->at(0)->GetDesc().Height);

		auto finish = RUB.End(Renderer::Get()->GetCommandQueue(CommandType::Direct));

		finish.wait();

	}
	else
	{
		delete tempTexture;
		tempTexture = nullptr;
	}


	return tempTexture;
}

void Motherloader::LoadXML(const std::string& FilePath)
{
	std::string fileName = CreateFileName(FilePath);

	// Check XML type - Materials or PipeLineStates
	// If Materials --> call for LoadMaterial (private) which reads the XML-file and uploads to AssetManager
	if (fileName == "Materials")
	{
		LoadMaterials(FilePath);
	}
	// Else if PipeLineStates --> call for LoadPipeLineState (private) which reads the XML-file and uploads to AssetManager
	else if (fileName == "PipeLineStates")
	{
		LoadPipeLineStates(FilePath);
	}
}

bool Motherloader::FromCubeMapFolder(const std::string& FilePath) const noexcept
{
	bool returnValue{ false };

	std::size_t found = FilePath.find("cubemaps");

	if (found != std::string::npos)
	{
		returnValue = true;
	}


	return returnValue;
}

void Motherloader::GetBiTan(saveVector* SaveVec, int Index)
{

	float v1[3];
	float v2[3];

	float uv1[2];
	float uv2[2];

	switch (Index % 3)
	{
	case 0:
		
		memcpy(v1,&SaveVec[Index + 1].position,sizeof(float)*3);
		memcpy(v2,&SaveVec[Index + 2].position,sizeof(float)*3);

		memcpy(uv1, &SaveVec[Index + 1].uv, sizeof(float) * 2);
		memcpy(uv2, &SaveVec[Index + 2].uv, sizeof(float) * 2);

		break;

	case 1:
		memcpy(v1, &SaveVec[Index - 1].position, sizeof(float) * 3);
		memcpy(v2, &SaveVec[Index + 1].position, sizeof(float) * 3);

		memcpy(uv1, &SaveVec[Index - 1].uv, sizeof(float) * 2);
		memcpy(uv2, &SaveVec[Index + 1].uv, sizeof(float) * 2);

		break;

	case 2: 
		memcpy(v1, &SaveVec[Index - 1].position, sizeof(float) * 3);
		memcpy(v2, &SaveVec[Index - 2].position, sizeof(float) * 3);

		memcpy(uv1, &SaveVec[Index - 1].uv, sizeof(float) * 2);
		memcpy(uv2, &SaveVec[Index - 2].uv, sizeof(float) * 2);

		break;
	}

	//Positions
	v1[0] = v1[0] - SaveVec[Index].position[0];
	v1[1] = v1[1] - SaveVec[Index].position[1];
	v1[2] = v1[2] - SaveVec[Index].position[2];

	v2[0] = v2[0] - SaveVec[Index].position[0];
	v2[1] = v2[1] - SaveVec[Index].position[1];
	v2[2] = v2[2] - SaveVec[Index].position[2];


	XMVECTOR tempVec1 = DirectX::XMVectorSet(v1[0], v1[1], v1[2], 0.0f);
	XMVECTOR tempVec2 = DirectX::XMVectorSet(v2[0], v2[1], v2[2], 0.0f);

	XMFLOAT3 tempXMFloat1;
	XMFLOAT3 tempXMFloat2;

	XMStoreFloat3(&tempXMFloat1, tempVec1);
	XMStoreFloat3(&tempXMFloat2, tempVec2);

	//UVs
	uv1[0] = uv1[0] - SaveVec[Index].uv[0];
	uv1[1] = uv1[1] - SaveVec[Index].uv[1];
						
	uv2[0] = uv2[0] - SaveVec[Index].uv[0];
	uv2[1] = uv2[1] - SaveVec[Index].uv[1];



	float tempFloat = (1.0f / ((uv1[0] * uv2[1]) - (uv2[0] * uv1[1])));

	XMFLOAT3 TempTan;
	XMFLOAT3 TempBiTan;

	TempTan.x = tempFloat * (uv2[1] * tempXMFloat1.x - uv1[1] * tempXMFloat2.x);
	TempTan.y = tempFloat * (uv2[1] * tempXMFloat1.y - uv1[1] * tempXMFloat2.y);
	TempTan.z = tempFloat * (uv2[1] * tempXMFloat1.z - uv1[1] * tempXMFloat2.z);
	
	XMVECTOR tempNormalTan = DirectX::XMVectorSet(TempTan.x, TempTan.y, TempTan.z, 0.0f);

	tempNormalTan = XMVector3Normalize(tempNormalTan);

	XMVECTOR tempNormal = XMVectorSet(SaveVec[Index].normal[0], SaveVec[Index].normal[1], SaveVec[Index].normal[2], SaveVec[Index].normal[3]);

	XMVECTOR BiTan = XMVector3Cross(tempNormal, tempNormalTan);

	BiTan = XMVector3Normalize(BiTan);

	XMStoreFloat3(&TempTan, tempNormalTan);
	XMStoreFloat3(&TempBiTan, BiTan);


	memcpy(&SaveVec[Index].TBN[0], &TempTan, sizeof(float) * 3);
	memcpy(&SaveVec[Index].TBN[4], &TempBiTan, sizeof(float) * 3);
	memcpy(&SaveVec[Index].TBN[8], &SaveVec[Index].normal[0], sizeof(float) * 3);

}





Motherloader::Motherloader() noexcept
{

}

Motherloader::~Motherloader() noexcept
{
}


void Motherloader::InitFence() noexcept
{
	Renderer::Get()->GetDevice()->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
}

void Motherloader::LoadMaterials(const std::string& FilePath)
{
	// Create neccessary files (XML)
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(FilePath.c_str());

	// Check result - break/signal if failing
	if (result)
	{
		// Get all nodes
		pugi::xml_node materials = doc.child("materials");

		// Retrieve the amount of materials
		//size_t amount = std::distance(materials.children().begin(), materials.children().end());

		// Retrieve and save info
		Material material;
		std::string line = "";
		Uint8 counter = 0;
		bool loaded;
		for (pugi::xml_node m = materials.child("material"); m; m = m.next_sibling("material"))
		{
			loaded = true;

			///// TEXTURES /////
			// 1. Get material type (should be unique for each material)
			// 2. Get texture name
			// 3. Get texture pointer from AssetManager
			// 4. If null is retreved, signal a failure
			// 5. Repeat with the other textures

			// Type
			line = m.attribute("type").as_string();
			if (line == "")
				loaded = false;
			material.Name = line;

			// Diffuse
			line = m.attribute("diffuseTexture").as_string();
			if (line != "" && Assetmanager::Get()->GetTexture(line) != nullptr)
			{
				material.Diffuse = Assetmanager::Get()->GetTexture(line);
				material.Diffuse->SetResourceType(ResourceType::DiffuseTexture_Type);
			}
			else if (line != "" && Assetmanager::Get()->GetTexture(line) == nullptr)
			{
				loaded = false;
			}

			// Normal Map
			line = m.attribute("normalTexture").as_string();
			if (line != "" && Assetmanager::Get()->GetTexture(line) != nullptr)
			{
				material.NormalMap = Assetmanager::Get()->GetTexture(line);
				material.NormalMap->SetResourceType(ResourceType::NormalTexture_Type);
			}
			else if (line != "" && Assetmanager::Get()->GetTexture(line) == nullptr)
			{
				loaded = false;
			}
			// Specular Map
			line = m.attribute("specularTexture").as_string();
			if (line != "" && Assetmanager::Get()->GetTexture(line) != nullptr)
			{
				material.SpecularMap = Assetmanager::Get()->GetTexture(line);
				material.SpecularMap->SetResourceType(ResourceType::SpecularTexture_Type);
			}
			else if (line != "" && Assetmanager::Get()->GetTexture(line) == nullptr)
			{
				loaded = false;
			}
			///// PIPELINESTATE /////
			// 1. Get pipeLineState name
			// 2. Get pipeLineState pointer from AssetManager (should already be loaded since we now use the order-file for loading stuff)
			// 3. Save pipeLineState pointer to material

			line = m.attribute("shaderPipeLineState").as_string();
			if (Assetmanager::Get()->GetPipeLineState(line) != nullptr)
				material.PipeLineState = Assetmanager::Get()->GetPipeLineState(line);
			else
				loaded = false;


			// Check if everything was loaded correctly
			if (!loaded)
			{
				std::string output = "One or multiple sources weren't available in AssetManager for material #" + std::to_string(counter) + "\n";
				printf(output.c_str());
			}

			line = "";
			counter++;

			// Upload material to AssetManager
			Assetmanager::Get()->UploadMaterial(material.Name, material);
		}
	}
	else
	{
		printf("Materials.xml couldn't open!");
	}
}

void Motherloader::LoadPipeLineStates(const std::string& FilePath)
{
	//Create neccessary files (XML)
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(FilePath.c_str());

	// Check result - break/signal if failing
	if (result)
	{
		// Get all nodes
		pugi::xml_node pipeLineStates = doc.child("pipeLineStates");

		// Retrieve and save info
		PipeLineStateLoader PSLoader;
		PipeLineState PS;
		std::string line = "";
		int numberOfRTVs = 0;
		bool depthBuffer = false;
		for (pugi::xml_node p = pipeLineStates.child("pipeLineState"); p; p = p.next_sibling("pipeLineState"))
		{
			///// PIPELINESTATE /////
			// 1. Check shader type
			// 2. If graphics - get vs and fs name
			// 3. Create GraphicsPipeLineState from PipeLineState class
			// 4. Else if compute - get cs name
			// 5. Create ComputePipeLineState from PipeLineState class
			// 6. Once the pipeLineState has been created and uploaded to AsseManager (internal in function), reset PipeLineState (PS)

			line = p.attribute("type").as_string();
			if (line == "graphics")
			{
				PS.VertexShader = p.attribute("vertexShader").as_string();
				PS.FragmentShader = p.attribute("fragmentShader").as_string();
				PS.PassType = p.attribute("pass").as_string();
				PS.NumberOfRTVs = p.attribute("numberOfRTVs").as_int();

				// Topology type
				line = p.attribute("topologyType").as_string();
				if (line == "triangle")
					PS.Topology = TopologyType::Triangle;
				else if (line == "line")
					PS.Topology = TopologyType::Line;
				else if (line == "point")
					PS.Topology = TopologyType::Point;

				// Fill mode
				line = p.attribute("fillMode").as_string();
				if (line == "solid")
					PS.Fill = FillMode::Solid;
				else if (line == "wireFrame")
					PS.Fill = FillMode::WireFrame;

				// Cull mode
				line = p.attribute("cullMode").as_string();
				if (line == "back")
					PS.Cull = CullMode::Back;
				else if (line == "front")
					PS.Cull = CullMode::Front;
				else if (line == "none")
					PS.Cull = CullMode::None;

				// Depth buffer
				PS.DepthBuffer = p.attribute("depthBuffer").as_bool();

				// Create PipeLineStateObject
				PSLoader.CreateGraphicsPipeLineStateObject(PipeLineState(PS.VertexShader, PS.FragmentShader, PS.PassType, PS.NumberOfRTVs, PS.Topology, PS.Fill, PS.Cull, PS.DepthBuffer));

				// Reset PipeLine (information container) for possible re-use
				PS.ResetPipeLineState();
			}
			else if (line == "compute")
			{
				PS.ComputeShader = p.attribute("computeShader").as_string();
				PS.PassType = p.attribute("pass").as_string();

				// Create PipeLineStateObject
				PSLoader.CreateComputePipeLineStateObject(PipeLineState(PS.ComputeShader, PS.PassType, PS.Topology, PS.Fill, PS.Cull, PS.DepthBuffer));

				// Reset PipeLine (information container) for possible re-use
				PS.ResetPipeLineState();
			}
		}
	}
	else
	{
		printf("Materials.xml couldn't open!");
	}
}

std::string Motherloader::CreateFileName(const std::string& FilePath) noexcept
{

	std::string returnValue{ "" };

	int16_t counter = FilePath.size() - 1;

	//Simple Design (To Do: Add checks)

	while (counter >= 0 && FilePath[counter] != '.')
	{
		counter--;
	}

	if (counter != 0)
	{
		counter--;
	}

	while (counter >= 0 && FilePath[counter] != '/' && FilePath[counter] != '\\')
	{
		returnValue += FilePath[counter];
		counter--;
	}
	std::reverse(returnValue.begin(), returnValue.end());

	return returnValue;

}