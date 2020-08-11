#include "Renderer.hpp"
#include "../Assetmanager/Assetmanager.hpp"
#include "../ObjectManager/ObjectManager.h"
#include "../Core/Time.hpp"
#include <string>
#include <iostream>
#include "dxgidebug.h"
#include <wchar.h>
Renderer* Renderer::s_Instance = nullptr;

Renderer::~Renderer() noexcept
{
	m_Thread1Alive = false;
	m_Thread2Alive = false;
	m_ThreadFrameDoneAlive = false;

	WaitForGPU(Direct);
	WaitForGPU(Compute);

	
	SafeRelease(&m_Device5);
	SafeRelease(&m_SwapChain);

	SafeRelease(&m_CommandQueueDirect);
	SafeRelease(&m_CommandAllocatorDirect);
	SafeRelease(&m_CommandListDirect);

	SafeRelease(&m_DSV_DescriptorHeap);
	SafeRelease(&m_DSV_Texture);

	SafeRelease(&m_CommandQueueCompute);
	SafeRelease(&m_CommandAllocatorCompute);
	SafeRelease(&m_CommandListCompute);

	SafeRelease(&m_CommandQueueCopy);
	SafeRelease(&m_CommandAllocatorCopy);
	SafeRelease(&m_CommandListCopy);

	SafeRelease(&m_ComList_FP);
	SafeRelease(&m_ComList_Compute);
	SafeRelease(&m_ComList_SP);

	SafeRelease(&m_RenderTargetsHeap);
	SafeRelease(&m_GBufferRTsHeap);
	SafeRelease(&m_RootSignature);

	SafeRelease(&m_FenceFrame);
	SafeRelease(&m_FenceDirect);
	SafeRelease(&m_FenceCompute);
	SafeRelease(&m_FenceCopy);

	SafeRelease(&m_UploadResource);

	if (m_RecordList1Thread)
	{
		delete m_RecordList1Thread;
		m_RecordList1Thread = nullptr;
	}
	if (m_RecordList2Thread)
	{
		delete m_RecordList2Thread;
		m_RecordList2Thread = nullptr;
	}
	if (m_FrameDoneThread)
	{
		delete m_FrameDoneThread;
		m_FrameDoneThread = nullptr;
	}
	for (UINT8 i = 0; i < NUMBER_OF_BACK_BUFFERS; i++)
	{
	
		SafeRelease(&m_Allocator_FP[i]);
		SafeRelease(&m_Allocator_Compute[i]);
		SafeRelease(&m_Allocator_SP[i]);
		SafeRelease(&m_RenderTargets[i]);
		SafeRelease(&m_CameraPositionResource[i]);
		SafeRelease(&m_CameraVPMatrixResource[i]);
		SafeRelease(&m_CameraViewMatrixResource[i]);
		SafeRelease(&m_CameraProjectionMatrixResource[i]);
		SafeRelease(&m_TileDataResource[i]);
		SafeRelease(&m_DescriptorHeapsRenderer[i]);
		SafeRelease(&m_DescriptorHeapsSampler[i]);
		SafeRelease(&m_ComputeResource[i]);
	}
	for (UINT8 i = 0; i < NUMBER_OF_DEFERRED_RTVS; i++)
	{
		SafeRelease(&m_GBufferRTResources[i]);
	}
	SDL_DestroyWindow(m_SDLWindow);

	delete[] m_HeapStrides;
	m_HeapStrides = nullptr;




}

Renderer* Renderer::Get() noexcept
{
	if (!s_Instance)
	{
		s_Instance = new Renderer();
	}
	return s_Instance;
}

bool Renderer::Initialize(const XMUINT2& windowSize, const std::string& winName)
{
	bool returnValue = true;

	// Create Window
	m_SDLWindow = SDL_CreateWindow(winName.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowSize.x, windowSize.y, 0);
	m_HWND = GetActiveWindow();
	m_WindowSize.x = windowSize.x;
	m_WindowSize.y = windowSize.y;


	// Create Renderer
	CreateDevice();
	CreateScissorAndViewport();
	CreateCommandInterfacesAndSwapChain();
	CreateRenderTargets();
	CreateDSV();
	CreateGBuffers();
	CreateRootSignature();
	CreateRootSignatureCBVs();
	CreateDescriptorHeap();
	CreateSRVsToGBuffersRTVs();
	CreateSampler();
	CreateFence();
	CreateEventHandle();
	CreateStrides();
	InitializeLightCulling();

	return returnValue;
}

void Renderer::InitThreads()
{
	if (m_RecordList1Thread == nullptr)
	{
		m_RecordList1Thread = new std::thread(&Renderer::RecordFirstPass, this, std::ref(m_ComList_FP), m_Allocator_FP, std::ref(m_RecordFirstPass), std::ref(m_Thread1Alive));
		m_RecordList1Thread->detach();
	}
	if (m_RecordList2Thread == nullptr)
	{
		m_RecordList2Thread = new std::thread(&Renderer::ExecuteLightCulling, this, std::ref(m_ComList_Compute), m_Allocator_Compute, std::ref(m_RecordCompute), std::ref(m_Thread2Alive));
		m_RecordList2Thread->detach();
	}
	if (m_FrameDoneThread == nullptr)
	{
		m_FrameDoneThread = new std::thread(&Renderer::FrameDone, this, std::ref(m_FrameRecording), std::ref(m_ThreadFrameDoneAlive));


		m_FrameDoneThread->detach();
	}
}

void Renderer::OnNewFrame()
{
	CopyLightResources();

	m_RecordFirstPass = true;
	m_RecordCompute = true;

	UpdateCameraResources();
}

void Renderer::Render()
{
	m_NumberOfPointLights = ObjectManager::Get()->GetLights()->GetNumberOfPointLights();

	std::string windowTitle = "Asteroids: " + std::to_string(ObjectManager::Get()->GetPrefab(1)->GetNrOfInstances()) +
		" Lightcubes: " + std::to_string(m_NumberOfPointLights) +
		" FPS: " + std::to_string(Time::Get()->FPS());
	SDL_SetWindowTitle(m_SDLWindow, windowTitle.c_str());


	MainRecordSecondPass(m_ComList_SP, m_Allocator_SP);

	while (m_RecordFirstPass || m_RecordCompute)
	{
		m_FenceDirect->GetCompletedValue() < m_FenceValueDirect;
	}

	// Execute the command list on a Command Queue
	ID3D12CommandList* listsToExecute[] = { m_ComList_Compute };
	m_CommandQueueCompute->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);
	Benchmarker::Get()->StartClock(GPUCLOCK);

	m_FenceValueCompute++;
	m_CommandQueueCompute->Signal(m_FenceCompute, m_FenceValueCompute);


	// Execute the command list on a Direct Queue
	ID3D12CommandList* listsToExecute2[] = { m_ComList_FP };
	m_CommandQueueDirect->ExecuteCommandLists(ARRAYSIZE(listsToExecute2), listsToExecute2);



	m_FenceValueDirect++;
	m_CommandQueueDirect->Signal(m_FenceDirect, m_FenceValueDirect);


	// Sync resources before executing secondpass
	while (m_FenceCompute->GetCompletedValue() < m_FenceValueCompute ||
		m_FenceDirect->GetCompletedValue() < m_FenceValueDirect)
	{
	}
	Benchmarker::Get()->EndClock(GPUCLOCK);

	ID3D12CommandList* listsToExecute3[] = { m_ComList_SP };
	m_CommandQueueDirect->ExecuteCommandLists(ARRAYSIZE(listsToExecute3), listsToExecute3);
	Benchmarker::Get()->StartClock(GPUCLOCK);


	// Signal and increment the fence value
	// Last Frame (Due to double buffering)
	while ((m_FenceFrame->GetCompletedValue() < m_FenceValueFrame));
	m_FenceValueFrame++;
	m_CommandQueueDirect->Signal(m_FenceFrame, m_FenceValueFrame);
	m_FrameRecording = true;

	// Present the frame.
	DXGI_PRESENT_PARAMETERS pp = {};
	m_SwapChain->Present1(0, 0, &pp);
	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
}

void Renderer::RecordFirstPass(ID3D12GraphicsCommandList4*& comList, ID3D12CommandAllocator** allocator, bool& workBool, bool& threadAlive)
{
	while (threadAlive)
	{
		if (workBool)
		{
			// Reset CommandList
			allocator[m_CurrentBackBufferIndex]->Reset();
			comList->Reset(allocator[m_CurrentBackBufferIndex], nullptr);

			//CleargBuffersAndSetRTV();
			D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_GBufferRTsHeap->GetCPUDescriptorHandleForHeapStart();
			for (size_t i = 0; i < NUMBER_OF_DEFERRED_RTVS; i++)
			{
				comList->ClearRenderTargetView(cdh, CLEAR_COLOR, 0, nullptr);
				cdh.ptr += m_RenderTargetDescriptorSize;
			}

			// Clear DSV
			D3D12_CPU_DESCRIPTOR_HANDLE cdhDSV = m_DSV_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			comList->ClearDepthStencilView(cdhDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

			//RTV
			cdh = m_GBufferRTsHeap->GetCPUDescriptorHandleForHeapStart();
			comList->OMSetRenderTargets(NUMBER_OF_DEFERRED_RTVS, &cdh, true, &cdhDSV);

			// Set Root Signature
			comList->SetGraphicsRootSignature(m_RootSignature);

			// Set Descriptor Heaps (CBV_SRV_UAV & Sampler)
			ID3D12DescriptorHeap* descriptorHeaps[] = { m_DescriptorHeapsRenderer[m_CurrentBackBufferIndex], m_DescriptorHeapsSampler[m_CurrentBackBufferIndex] };
			comList->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);

			D3D12_GPU_DESCRIPTOR_HANDLE samplerHeapAdress = m_DescriptorHeapsSampler[m_CurrentBackBufferIndex]->GetGPUDescriptorHandleForHeapStart();
			comList->SetGraphicsRootDescriptorTable(Sampler, samplerHeapAdress);

			comList->SetGraphicsRootConstantBufferView(m_CBVCameraPositionLocation, m_CameraPositionResource[m_CurrentBackBufferIndex]->GetGPUVirtualAddress());
			comList->SetGraphicsRootConstantBufferView(m_CBVCameraVPMatrixLocation, m_CameraVPMatrixResource[m_CurrentBackBufferIndex]->GetGPUVirtualAddress());

			// Set Viewport and ScissorRect
			comList->RSSetViewports(1, &m_Viewport);
			comList->RSSetScissorRects(1, &m_ScissorRect);



			// Set PipeLineState
			comList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			comList->SetPipelineState(Assetmanager::Get()->GetPipeLineState("1"));

			// Descriptor Heap start
			UINT64 stride = m_Device5->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			D3D12_GPU_DESCRIPTOR_HANDLE descHeapAdress = m_DescriptorHeapsRenderer[m_CurrentBackBufferIndex]->GetGPUDescriptorHandleForHeapStart();
			descHeapAdress.ptr += NUMBER_OF_HEAPRANGE_VIEWS * PreFabs * stride;

			// Render all meshes
			size_t nrOfElements = ObjectManager::Get()->GetAllPrefabs()->size();
			for (size_t i = 0; i < nrOfElements; i++)
			{
				// Map up Descriptor Range and Descriptor Heap
				comList->SetGraphicsRootDescriptorTable(PreFabs, descHeapAdress);

				// Draw
				// Get number of Prefab vertecies + instances
				comList->DrawInstanced(ObjectManager::Get()->GetPrefab(i)->GetMeshVertexCount(), ObjectManager::Get()->GetPrefab(i)->GetNrOfInstances(), 0, 0);

				// Descriptor heap offset
				descHeapAdress.ptr += m_HeapStrides[PreFabs];
			}

			//Render Skybox

			comList->SetPipelineState(Assetmanager::Get()->GetPipeLineState("3"));

			comList->SetGraphicsRootConstantBufferView(m_CBVCameraViewMatrixLocation, m_CameraViewMatrixResource[m_CurrentBackBufferIndex]->GetGPUVirtualAddress());
			comList->SetGraphicsRootConstantBufferView(m_CBVCameraProjectionMatrixLocation, m_CameraProjectionMatrixResource[m_CurrentBackBufferIndex]->GetGPUVirtualAddress());


			descHeapAdress = m_DescriptorHeapsRenderer[m_CurrentBackBufferIndex]->GetGPUDescriptorHandleForHeapStart();
			descHeapAdress.ptr += NUMBER_OF_HEAPRANGE_VIEWS * Skybox * stride;

			comList->SetGraphicsRootDescriptorTable(m_SRVSkyboxLocation, descHeapAdress);

			comList->DrawInstanced(36, 1, 0, 0);


			comList->Close();
			m_RecordFirstPass = false;
		}
	}
}

void Renderer::MainRecordSecondPass(ID3D12GraphicsCommandList4*& comList, ID3D12CommandAllocator** allocator)
{
	allocator[m_CurrentBackBufferIndex]->Reset();
	comList->Reset(allocator[m_CurrentBackBufferIndex], nullptr);

	// Indicate that the back buffer will be used as render target.
	SetResourceTransitionBarrier(comList,
		m_RenderTargets[m_CurrentBackBufferIndex],
		D3D12_RESOURCE_STATE_PRESENT,		// State before
		D3D12_RESOURCE_STATE_RENDER_TARGET	// State after
	);

	for (size_t i = 0; i < NUMBER_OF_DEFERRED_RTVS; i++)
	{
		// Indicate that the gbuffer will be used as render target.
		SetResourceTransitionBarrier(comList,
			m_GBufferRTResources[i],
			D3D12_RESOURCE_STATE_RENDER_TARGET,	// State before
			D3D12_RESOURCE_STATE_GENERIC_READ // State after 
		);
	}

	// Set Root Signature 
	comList->SetGraphicsRootSignature(m_RootSignature);

	// Set Descriptor Heaps
	// Get Pointers
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_RenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
	cdh.ptr += m_RenderTargetDescriptorSize * m_CurrentBackBufferIndex;

	comList->OMSetRenderTargets(1, &cdh, false, nullptr);
	comList->ClearRenderTargetView(cdh, CLEAR_COLOR, 0, nullptr);

	// Set Descriptor Heaps (CBV_SRV_UAV & Sampler)
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_DescriptorHeapsRenderer[m_CurrentBackBufferIndex], m_DescriptorHeapsSampler[m_CurrentBackBufferIndex] };
	comList->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);

	D3D12_GPU_DESCRIPTOR_HANDLE samplerHeapAdress = m_DescriptorHeapsSampler[m_CurrentBackBufferIndex]->GetGPUDescriptorHandleForHeapStart();
	comList->SetGraphicsRootDescriptorTable(Sampler, samplerHeapAdress);

	// Set Root Constant Buffers
	comList->SetGraphicsRootConstantBufferView(m_CBVCameraPositionLocation, m_CameraPositionResource[m_CurrentBackBufferIndex]->GetGPUVirtualAddress());
	comList->SetGraphicsRootConstantBufferView(m_CBVCameraVPMatrixLocation, m_CameraVPMatrixResource[m_CurrentBackBufferIndex]->GetGPUVirtualAddress());
	comList->SetGraphicsRoot32BitConstant(m_CBLightLocation, m_NumberOfPointLights, 0);

	// Set Viewport and ScissorRect
	comList->RSSetViewports(1, &m_Viewport);
	comList->RSSetScissorRects(1, &m_ScissorRect);

	// Set PipeLineState
	comList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	comList->SetPipelineState(Assetmanager::Get()->GetPipeLineState("2"));

	// Descriptor heap start
	D3D12_GPU_DESCRIPTOR_HANDLE descHeapAdress = m_DescriptorHeapsRenderer[m_CurrentBackBufferIndex]->GetGPUDescriptorHandleForHeapStart();
	UINT64 stride = m_Device5->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHeapAdress.ptr += UINT(NUMBER_OF_HEAPRANGE_VIEWS * DeferredSP * stride);

	// Map up Descriptor Range and Descriptor Heap
	comList->SetGraphicsRootDescriptorTable(DeferredSP, descHeapAdress);

	// Descriptor heap start for Compute
	comList->SetGraphicsRootShaderResourceView(m_SRVTileDataLocation, m_TileDataResource[m_CurrentBackBufferIndex]->GetGPUVirtualAddress());

	// Render all gBuffers
	comList->DrawInstanced(6, 1, 0, 0);

	SetResourceTransitionBarrier(comList,
		m_RenderTargets[m_CurrentBackBufferIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET,	//state before
		D3D12_RESOURCE_STATE_PRESENT		//state after
	);

	for (size_t i = 0; i < NUMBER_OF_DEFERRED_RTVS; i++)
	{
		// Indicate that the gbuffer will be used as render target.
		SetResourceTransitionBarrier(comList,
			m_GBufferRTResources[i],
			D3D12_RESOURCE_STATE_GENERIC_READ,	 // State before
			D3D12_RESOURCE_STATE_RENDER_TARGET	 // State after 
		);
	}

	comList->Close();
}

void Renderer::ExecuteLightCulling(ID3D12GraphicsCommandList4*& comList, ID3D12CommandAllocator** allocator, bool& workBool, bool& threadAlive)
{
	// Get compute PipeLineState
	auto pipeLineState = Assetmanager::Get()->GetPipeLineState("compute");

	while (threadAlive)
	{
		if (workBool)
		{
			// Get Command Allocator and List + reset them
			allocator[m_CurrentBackBufferIndex]->Reset();
			comList->Reset(allocator[m_CurrentBackBufferIndex], pipeLineState);

			// Set Root Signature 
			comList->SetComputeRootSignature(m_RootSignature);

			// Set Descriptor Heaps (CBV_SRV_UAV)
			ID3D12DescriptorHeap* descriptorHeaps[] = { m_DescriptorHeapsRenderer[m_CurrentBackBufferIndex] };
			comList->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);

			// Set Root Constant Buffers
			comList->SetComputeRootConstantBufferView(m_CBVCameraPositionLocation, m_CameraPositionResource[m_CurrentBackBufferIndex]->GetGPUVirtualAddress());
			comList->SetComputeRootConstantBufferView(m_CBVCameraVPMatrixLocation, m_CameraVPMatrixResource[m_CurrentBackBufferIndex]->GetGPUVirtualAddress());
			comList->SetComputeRoot32BitConstant(m_CBLightLocation, m_NumberOfPointLights, 0);

			// Descriptor heap start for SecondPass (lights)
			D3D12_GPU_DESCRIPTOR_HANDLE descHeapAdress = m_DescriptorHeapsRenderer[m_CurrentBackBufferIndex]->GetGPUDescriptorHandleForHeapStart();
			UINT64 IncrementSize = m_Device5->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			descHeapAdress.ptr += UINT(NUMBER_OF_HEAPRANGE_VIEWS * IncrementSize * DeferredSP);

			// Map up Descriptor Range and Descriptor Heap for SecondPass
			comList->SetComputeRootDescriptorTable(DeferredSP, descHeapAdress);

			// Descriptor heap start for Compute
			descHeapAdress = m_DescriptorHeapsRenderer[m_CurrentBackBufferIndex]->GetGPUDescriptorHandleForHeapStart();
			descHeapAdress.ptr += UINT(NUMBER_OF_HEAPRANGE_VIEWS * IncrementSize * Light);

			// Map up Descriptor Range and Descriptor Heap for Compute
			comList->SetComputeRootDescriptorTable(Light, descHeapAdress);

			// Spawn one thread group
			comList->Dispatch(1, 1, 1);

			// Close the list to prepare it for execution
			comList->Close();
			m_RecordCompute = false;
		}
	}
}

ID3D12Device5* Renderer::GetDevice()
{
	return m_Device5;
}

ID3D12CommandQueue* Renderer::GetCommandQueue(CommandType type)
{
	if (type == CommandType::Direct)
	{
		return m_CommandQueueDirect;
	}
	else if (type == CommandType::Compute)
	{
		return m_CommandQueueCompute;
	}
	return m_CommandQueueCopy;
}

ID3D12GraphicsCommandList4* Renderer::GetCommandList(CommandType type)
{
	ID3D12GraphicsCommandList4* returnValue = nullptr;
	if (type == CommandType::Direct)
	{
		returnValue = m_CommandListDirect;
	}
	else if (type == CommandType::Compute)
	{
		returnValue = m_CommandListCompute;
	}
	else if (type == CommandType::Copy)
	{
		returnValue = m_CommandListCopy;
	}
	return returnValue;
}

ID3D12CommandAllocator* Renderer::GetCommandAllocator(CommandType type)
{
	ID3D12CommandAllocator* returnValue = nullptr;
	if (type == CommandType::Direct)
	{
		returnValue = m_CommandAllocatorDirect;
	}
	else if (type == CommandType::Compute)
	{
		returnValue = m_CommandAllocatorCompute;
	}
	else if (type == CommandType::Copy)
	{
		returnValue = m_CommandAllocatorCopy;
	}
	return returnValue;
}

const UINT8 Renderer::GetCurrentBackBufferIndex()
{
	return m_CurrentBackBufferIndex;
}

ID3D12RootSignature* Renderer::GetRootSignature()
{
	return m_RootSignature;
}

ID3D12DescriptorHeap** Renderer::GetDescriptorHeaps(RenderResourceType type)
{
	if (type == RenderResourceType::Sampler)
	{
		return m_DescriptorHeapsSampler;
	}

	return m_DescriptorHeapsRenderer;
}

ID3D12DescriptorHeap* Renderer::GetActiveDescriptorHeap(RenderResourceType type)
{
	if (type == RenderResourceType::Sampler)
	{
		return m_DescriptorHeapsSampler[m_CurrentBackBufferIndex];
	}

	return m_DescriptorHeapsRenderer[m_CurrentBackBufferIndex];
}

D3D12_GPU_DESCRIPTOR_HANDLE Renderer::GetActiveHeapStart(const RenderResourceType& type)
{
	//  Descriptor heap start and offset
	D3D12_GPU_DESCRIPTOR_HANDLE returnValue = m_DescriptorHeapsRenderer[m_CurrentBackBufferIndex]->GetGPUDescriptorHandleForHeapStart();

	switch (type)
	{
	case RenderResourceType::Light:
	{
		returnValue.ptr += NUMBER_OF_HEAPRANGE_VIEWS;
	}
	break;

	case RenderResourceType::Sampler:
	{
		returnValue = m_DescriptorHeapsSampler[m_CurrentBackBufferIndex]->GetGPUDescriptorHandleForHeapStart();
	}
	break;

	case RenderResourceType::DeferredSP:
	{
		returnValue.ptr += NUMBER_OF_HEAPRANGE_VIEWS * 2;
	}
	break;

	default:
	{
	}
	break;
	}

	return returnValue;
}

const UINT64& Renderer::GetHeapStride(const RenderResourceType& type) noexcept
{
	return m_HeapStrides[type];
}

const UINT64 Renderer::GetNumberOfViewsPerSubHeap()
{
	return NUMBER_OF_HEAPRANGE_VIEWS;
}

XMUINT3 Renderer::GetRootConstantBufferLocations()
{
	// x = Camera Position, y = Camera Matrix, z = Number of lights
	return XMUINT3(m_CBVCameraPositionLocation, m_CBVCameraVPMatrixLocation, m_CBLightLocation);
}

ID3D12Resource1* Renderer::GetActiveCameraPositionresource()
{
	return m_CameraPositionResource[m_CurrentBackBufferIndex];
}

ID3D12Resource1* Renderer::GetActiveCameraMatrixresource()
{
	return m_CameraVPMatrixResource[m_CurrentBackBufferIndex];
}

UINT Renderer::GetNumberOfPointLights()
{
	return m_NumberOfPointLights;
}

void Renderer::CopyLightResources()
{
	m_CommandAllocatorCopy->Reset();
	m_CommandListCopy->Reset(m_CommandAllocatorCopy, nullptr);

	m_CommandAllocatorDirect->Reset();
	m_CommandListDirect->Reset(m_CommandAllocatorDirect, nullptr);

	Lights* Lights = Engine::Get()->GetLights();

	ID3D12Resource* CopyDest = Lights->GetLightHeapBlock()->GetCopyLightresource().GetResources()->at(m_CurrentBackBufferIndex);
	ID3D12Resource* CopySrc = Lights->GetLightHeapBlock()->GetPointLightresource()->GetResources()->at(m_CurrentBackBufferIndex);

	D3D12_RESOURCE_BARRIER barrierDesc1 = {};
	barrierDesc1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc1.Transition.pResource = CopyDest;
	barrierDesc1.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc1.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
	barrierDesc1.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	m_CommandListDirect->ResourceBarrier(1, &barrierDesc1);
	m_CommandListDirect->Close();

	ID3D12CommandList* executeLists1[] = { m_CommandListDirect };
	m_CommandQueueDirect->ExecuteCommandLists(1, executeLists1);
	WaitForGPU(Direct);


	//Copy Upload -> Defualt
	m_CommandListCopy->CopyResource(CopyDest, CopySrc);

	m_CommandListCopy->Close();
	ID3D12CommandList* executeLists2[] = { m_CommandListCopy };
	m_CommandQueueCopy->ExecuteCommandLists(1, executeLists2);

	m_CommandAllocatorDirect->Reset();
	m_CommandListDirect->Reset(m_CommandAllocatorDirect, nullptr);


	D3D12_RESOURCE_BARRIER barrierDesc2 = {};
	barrierDesc2.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc2.Transition.pResource = CopyDest;
	barrierDesc2.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc2.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrierDesc2.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;

	m_CommandListDirect->ResourceBarrier(1, &barrierDesc2);
	m_CommandListDirect->Close();
	WaitForGPU(Copy);

	ID3D12CommandList* executeLists3[] = { m_CommandListDirect };
	m_CommandQueueDirect->ExecuteCommandLists(1, executeLists3);
	WaitForGPU(Direct);
}

void Renderer::CreateDevice()
{
#ifdef _DEBUG
	// The Debug layer has to be enabled before the device is created, otherwise the device will be removed.
	ID3D12Debug* debugController = nullptr;

	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
	SafeRelease(&debugController);
#endif // _DEBUG


	IDXGIFactory7* factory = nullptr;
	IDXGIAdapter1* adapter = nullptr;
	CreateDXGIFactory(IID_PPV_ARGS(&factory));

	for (UINT i = 0;; i++)
	{
		adapter = nullptr;
		if (DXGI_ERROR_NOT_FOUND == factory->EnumAdapters1(i, &adapter))
		{
			break;
		}

		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device5), nullptr)))
		{
			break;
		}
		SafeRelease(&adapter);
	}

	if (adapter)
	{
		HRESULT hr = S_OK;
		//Does not work with device 6
		if (SUCCEEDED(hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_Device5))))
		{

		}
	}
	else
	{
		factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
		D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device5));
	}
	SafeRelease(&adapter);
	SafeRelease(&factory);
}

void Renderer::CreateScissorAndViewport()
{
	// Viewport
	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;
	m_Viewport.Width = (float)m_WindowSize.x;
	m_Viewport.Height = (float)m_WindowSize.y;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	// Scissor Rect
	m_ScissorRect.left = (long)0;
	m_ScissorRect.right = (long)m_WindowSize.x;
	m_ScissorRect.top = (long)0;
	m_ScissorRect.bottom = (long)m_WindowSize.y;
}

void Renderer::CreateCommandInterfacesAndSwapChain()
{
	// Command queues, allocators and lists

	// Direct
	D3D12_COMMAND_QUEUE_DESC cqdDirect = {};
	cqdDirect.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	m_Device5->CreateCommandQueue(&cqdDirect, IID_PPV_ARGS(&m_CommandQueueDirect));
	m_CommandQueueDirect->SetName(L"Command Queue Direct");

	m_Device5->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocatorDirect));
	m_Device5->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_CommandAllocatorDirect,
		nullptr,
		IID_PPV_ARGS(&m_CommandListDirect));

	m_CommandListDirect->Close();

	// Compute
	D3D12_COMMAND_QUEUE_DESC cqdCompute = {};
	cqdCompute.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	m_Device5->CreateCommandQueue(&cqdCompute, IID_PPV_ARGS(&m_CommandQueueCompute));

	m_Device5->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&m_CommandAllocatorCompute));
	m_Device5->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		m_CommandAllocatorCompute, nullptr,
		IID_PPV_ARGS(&m_CommandListCompute));

	m_CommandListCompute->Close();

	// Copy
	D3D12_COMMAND_QUEUE_DESC cqdCopy = {};
	cqdCopy.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	m_Device5->CreateCommandQueue(&cqdCopy, IID_PPV_ARGS(&m_CommandQueueCopy));

	m_Device5->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_CommandAllocatorCopy));
	m_Device5->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_COPY,
		m_CommandAllocatorCopy,
		nullptr, IID_PPV_ARGS(&m_CommandListCopy));

	m_CommandListCopy->Close();



	// FirstPass 
	for (int i = 0; i < NUMBER_OF_BACK_BUFFERS; i++) //Two are enough but for simplicity we use NUMBER_OF_BACK_BUFFERS
	{
		std::string tempString = "FirstPass Command Allocator: " + std::to_string(i);
		std::wstring tempWstring(tempString.begin(), tempString.end());

		m_Device5->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_Allocator_FP[i]));

		m_Allocator_FP[i]->SetName(tempWstring.c_str());
	}
	m_Device5->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_Allocator_FP[0],
		nullptr,
		IID_PPV_ARGS(&m_ComList_FP));
	m_ComList_FP->SetName(L"FirstPass Command List");

	m_ComList_FP->Close();

	// Compute 
	for (int i = 0; i < NUMBER_OF_BACK_BUFFERS; i++) //Two are enough but for simplicity we use NUMBER_OF_BACK_BUFFERS
	{
		std::string tempString = "Compute Command Allocator: " + std::to_string(i);
		std::wstring tempWstring(tempString.begin(), tempString.end());

		m_Device5->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&m_Allocator_Compute[i]));

		m_Allocator_Compute[i]->SetName(tempWstring.c_str());
	}
	m_Device5->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		m_Allocator_Compute[0],
		nullptr,
		IID_PPV_ARGS(&m_ComList_Compute));
	m_ComList_Compute->SetName(L"Compute Command List");

	m_ComList_Compute->Close();

	// SecondPass
	for (int i = 0; i < NUMBER_OF_BACK_BUFFERS; i++) //Two are enough but for simplicity we use NUMBER_OF_BACK_BUFFERS
	{
		std::string tempString = "SecondPass Command Allocator: " + std::to_string(i);
		std::wstring tempWstring(tempString.begin(), tempString.end());

		m_Device5->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_Allocator_SP[i]));

		m_Allocator_SP[i]->SetName(tempWstring.c_str());
	}

	m_Device5->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_Allocator_SP[0],
		nullptr,
		IID_PPV_ARGS(&m_ComList_SP));

	m_ComList_SP->SetName(L"SecondPass Command List");

	m_ComList_SP->Close();



	// SwapChain
	IDXGIFactory7* factory = nullptr;
	CreateDXGIFactory(IID_PPV_ARGS(&factory));

	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width = 0;
	scDesc.Height = 0;
	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.Stereo = false;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = NUMBER_OF_BACK_BUFFERS;
	scDesc.Scaling = DXGI_SCALING_STRETCH;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.Flags = 0;
	scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	IDXGISwapChain1* swapChain1 = nullptr;
	if (SUCCEEDED(factory->CreateSwapChainForHwnd(
		m_CommandQueueDirect,
		m_HWND,
		&scDesc,
		nullptr,
		nullptr,
		&swapChain1
	)))
	{
		if (SUCCEEDED(swapChain1->QueryInterface(IID_PPV_ARGS(&m_SwapChain))))
		{
			m_SwapChain->Release();
		}
	}
	SafeRelease(&factory);
}

void Renderer::CreateRenderTargets()
{
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.NumDescriptors = NUMBER_OF_BACK_BUFFERS;
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	if (SUCCEEDED(m_Device5->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&m_RenderTargetsHeap))))
	{
		m_RenderTargetDescriptorSize = m_Device5->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_RenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();

		for (UINT i = 0; i < NUMBER_OF_BACK_BUFFERS; i++)
		{
			if (SUCCEEDED(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTargets[i]))))
			{
				m_Device5->CreateRenderTargetView(m_RenderTargets[i], nullptr, cdh);
				cdh.ptr += m_RenderTargetDescriptorSize;

				//Debug name for the buffer
				std::string debugName = "Back Buffer: " + std::to_string(i);
				std::wstring wString(debugName.begin(), debugName.end());
				m_RenderTargets[i]->SetName(wString.c_str());
			}
			else
			{
				printf("Swap chain creation failed!");
			}
		}
	}
	else
	{
		printf("Render Targets Heap creation failed!");
	}
}

void Renderer::CreateDSV()
{
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.NumDescriptors = 1;
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	if (SUCCEEDED(m_Device5->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&m_DSV_DescriptorHeap))))
	{
		m_DSV_DescriptorHeap->SetName(L"Depth Stencil Descriptor Heap");

		D3D12_HEAP_PROPERTIES heapProp;
		ZeroMemory(&heapProp, sizeof(heapProp));
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.CreationNodeMask = 1;
		heapProp.VisibleNodeMask = 1;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		D3D12_RESOURCE_DESC resourceDesc;
		ZeroMemory(&resourceDesc, sizeof(resourceDesc));
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Alignment = 0;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.Width = m_WindowSize.x;
		resourceDesc.Height = m_WindowSize.y;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE clearVal;
		clearVal.Format = resourceDesc.Format;
		clearVal.DepthStencil.Depth = 1.0f;
		clearVal.DepthStencil.Stencil = 0.0f;
		if (SUCCEEDED(m_Device5->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearVal,
			IID_PPV_ARGS(&m_DSV_Texture))))
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Texture2D.MipSlice = 0;
			desc.Format = resourceDesc.Format;
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			desc.Flags = D3D12_DSV_FLAG_NONE;

			D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_DSV_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();

			m_Device5->CreateDepthStencilView(m_DSV_Texture, &desc, cdh);

			ZeroMemory(&m_DSV_SRVDesc, sizeof(m_DSV_SRVDesc));
			m_DSV_SRVDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
			m_DSV_SRVDesc.Texture2D.MostDetailedMip = 0;
			m_DSV_SRVDesc.Format = resourceDesc.Format;
			m_DSV_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			m_DSV_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		}
	}
}

void Renderer::CreateGBuffers()
{
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.NumDescriptors = NUMBER_OF_DEFERRED_RTVS;
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	if (SUCCEEDED(m_Device5->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&m_GBufferRTsHeap))))
	{
		m_GBufferRTDescriptorSize = m_Device5->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_GBufferRTsHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_RESOURCE_DESC resourceDesc;
		ZeroMemory(&resourceDesc, sizeof(resourceDesc));
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Alignment = 0;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.MipLevels = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.Width = m_Viewport.Width;
		resourceDesc.Height = m_Viewport.Height;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		resourceDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.CreationNodeMask = 1;
		heapProperties.VisibleNodeMask = 1;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		ZeroMemory(&RTVDesc, sizeof(RTVDesc));
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.Texture2D.PlaneSlice = 0;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		RTVDesc.Format = resourceDesc.Format;

		D3D12_CLEAR_VALUE clearValue;
		clearValue.Format = resourceDesc.Format;
		clearValue.Color[0] = CLEAR_COLOR[0];
		clearValue.Color[1] = CLEAR_COLOR[1];
		clearValue.Color[2] = CLEAR_COLOR[2];
		clearValue.Color[3] = CLEAR_COLOR[3];

		for (UINT i = 0; i < NUMBER_OF_DEFERRED_RTVS; i++)
		{
			if (SUCCEEDED(m_Device5->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&m_GBufferRTResources[i]))))
			{
				m_Device5->CreateRenderTargetView(m_GBufferRTResources[i], &RTVDesc, cdh);
				cdh.ptr += m_GBufferRTDescriptorSize;
			}
			else
			{
				printf("GBufferResources creation failed in CreateGBuffers!");
				exit(0);
			}
		}
	}
	else
	{
		printf("GBuffer Render Targets Heap creation failed in CreateGBuffers!");
	}
}

void Renderer::CreateRootSignature()
{
	// For prefabs
#pragma region FirstPass_CBV_SRV_UAV

	D3D12_DESCRIPTOR_RANGE  firstPassDtRanges[1];

	// 10 SRV - Mesh, Transform[3], Color[3], DiffuseTex, NormalMap, SpecularMap
	firstPassDtRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	firstPassDtRanges[0].NumDescriptors = 10;
	firstPassDtRanges[0].BaseShaderRegister = 0; // t0 - t9
	firstPassDtRanges[0].RegisterSpace = 0;
	firstPassDtRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_DESCRIPTOR_TABLE firstPassDt;
	firstPassDt.NumDescriptorRanges = ARRAYSIZE(firstPassDtRanges);
	firstPassDt.pDescriptorRanges = firstPassDtRanges;

#pragma endregion FirstPass_CBV_SRV_UAV

	// For light culling in compute shader
#pragma region Compute_UAV

	D3D12_DESCRIPTOR_RANGE  computeDtRanges[1];

	// UAV - 1 UAV for all the culled lights
	computeDtRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	computeDtRanges[0].NumDescriptors = 1;
	computeDtRanges[0].BaseShaderRegister = 0; // u0
	computeDtRanges[0].RegisterSpace = 0;
	computeDtRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_DESCRIPTOR_TABLE computeDt;
	computeDt.NumDescriptorRanges = ARRAYSIZE(computeDtRanges);
	computeDt.pDescriptorRanges = computeDtRanges;

#pragma endregion Compute_UAV

#pragma region Sampler

	D3D12_DESCRIPTOR_RANGE  samplerDtRanges[1];

	samplerDtRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	samplerDtRanges[0].NumDescriptors = 1;
	samplerDtRanges[0].BaseShaderRegister = 0; // s0
	samplerDtRanges[0].RegisterSpace = 0;
	samplerDtRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_DESCRIPTOR_TABLE samplerDt;
	samplerDt.NumDescriptorRanges = ARRAYSIZE(samplerDtRanges);
	samplerDt.pDescriptorRanges = samplerDtRanges;

#pragma endregion Sampler

	// For combination of gBuffers and lights
#pragma region SecondPass_CBV_SRV_UAV

	D3D12_DESCRIPTOR_RANGE secondPassDtRanges[1];

	// 5 SRV - GBuffer: Position, Albedo, Normal, Specular
	//		 -PointLights
	secondPassDtRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	secondPassDtRanges[0].NumDescriptors = 5;
	secondPassDtRanges[0].BaseShaderRegister = 11; // t11 - t15
	secondPassDtRanges[0].RegisterSpace = 0;
	secondPassDtRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_DESCRIPTOR_TABLE secondPassDt;
	secondPassDt.NumDescriptorRanges = ARRAYSIZE(secondPassDtRanges);
	secondPassDt.pDescriptorRanges = secondPassDtRanges;

#pragma endregion SecondPass_CBV_SRV_UAV


#pragma region SkyboxPass_CBV_SRV_UAV

	D3D12_DESCRIPTOR_RANGE skyboxDtRanges[1];

	skyboxDtRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	skyboxDtRanges[0].NumDescriptors = 1;
	skyboxDtRanges[0].BaseShaderRegister = 16; // t16
	skyboxDtRanges[0].RegisterSpace = 0;
	skyboxDtRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_DESCRIPTOR_TABLE skyboxPassDt;
	skyboxPassDt.NumDescriptorRanges = ARRAYSIZE(skyboxDtRanges);
	skyboxPassDt.pDescriptorRanges = skyboxDtRanges;

#pragma endregion SkyboxPass_CBV_SRV_UAV

	D3D12_ROOT_PARAMETER  rootParam[11];
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable = firstPassDt;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable = computeDt;
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[2].DescriptorTable = samplerDt;
	rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	rootParam[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[3].DescriptorTable = secondPassDt;
	rootParam[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// SRV in RS -Skybox texture
	m_SRVSkyboxLocation = 4;
	rootParam[m_SRVSkyboxLocation].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[m_SRVSkyboxLocation].DescriptorTable = skyboxPassDt;
	rootParam[m_SRVSkyboxLocation].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// CBV in RS - Camera Position
	m_CBVCameraPositionLocation = 5;
	rootParam[m_CBVCameraPositionLocation].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[m_CBVCameraPositionLocation].Descriptor.ShaderRegister = 0;	// b0
	rootParam[m_CBVCameraPositionLocation].Descriptor.RegisterSpace = 0;
	rootParam[m_CBVCameraPositionLocation].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// CBV in RS - Camera ViewProjectionMatrix
	m_CBVCameraVPMatrixLocation = 6;
	rootParam[m_CBVCameraVPMatrixLocation].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[m_CBVCameraVPMatrixLocation].Descriptor.ShaderRegister = 1;	// b1
	rootParam[m_CBVCameraVPMatrixLocation].Descriptor.RegisterSpace = 0;
	rootParam[m_CBVCameraVPMatrixLocation].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// CB in RS - Light Count
	m_CBLightLocation = 7;
	rootParam[m_CBLightLocation].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParam[m_CBLightLocation].Constants.Num32BitValues = 1;
	rootParam[m_CBLightLocation].Constants.ShaderRegister = 2;	// b2
	rootParam[m_CBLightLocation].Constants.RegisterSpace = 0;
	rootParam[m_CBLightLocation].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// SRV in RS - TileData in SRV form
	m_SRVTileDataLocation = 8;
	rootParam[m_SRVTileDataLocation].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParam[m_SRVTileDataLocation].Descriptor.ShaderRegister = 17; // t17
	rootParam[m_SRVTileDataLocation].Descriptor.RegisterSpace = 0;
	rootParam[m_SRVTileDataLocation].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//CB in RSCamera view matrix
	m_CBVCameraViewMatrixLocation = 9;
	rootParam[m_CBVCameraViewMatrixLocation].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[m_CBVCameraViewMatrixLocation].Descriptor.ShaderRegister = 3;	// b3
	rootParam[m_CBVCameraViewMatrixLocation].Descriptor.RegisterSpace = 0;
	rootParam[m_CBVCameraViewMatrixLocation].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//CB in RSCamera view matrix
	m_CBVCameraProjectionMatrixLocation = 10;
	rootParam[m_CBVCameraProjectionMatrixLocation].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[m_CBVCameraProjectionMatrixLocation].Descriptor.ShaderRegister = 4;	// b4
	rootParam[m_CBVCameraProjectionMatrixLocation].Descriptor.RegisterSpace = 0;
	rootParam[m_CBVCameraProjectionMatrixLocation].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsDesc.NumParameters = ARRAYSIZE(rootParam);
	rsDesc.pParameters = rootParam;
	rsDesc.NumStaticSamplers = 0;
	rsDesc.pStaticSamplers = nullptr;


	ID3DBlob* sBlob;
	D3D12SerializeRootSignature(
		&rsDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&sBlob,
		nullptr);


	m_Device5->CreateRootSignature(
		0,
		sBlob->GetBufferPointer(),
		sBlob->GetBufferSize(),
		IID_PPV_ARGS(&m_RootSignature));
}

void Renderer::CreateRootSignatureCBVs()
{
	UINT sizeOfCameraPosition = (sizeof(XMFLOAT4) + 255) & ~255;	// 256-byte aligned CB.
	UINT sizeOfCameraMatrix = (sizeof(XMMATRIX) + 255) & ~255;	// 256-byte aligned CB.

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.CreationNodeMask = 1; //used when multi-gpu
	heapProperties.VisibleNodeMask = 1; //used when multi-gpu
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// Camera position resource descriptor
	D3D12_RESOURCE_DESC camPosResourceDesc = {};
	camPosResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	camPosResourceDesc.Width = sizeOfCameraPosition;
	camPosResourceDesc.Height = 1;
	camPosResourceDesc.DepthOrArraySize = 1;
	camPosResourceDesc.MipLevels = 1;
	camPosResourceDesc.SampleDesc.Count = 1;
	camPosResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// Camera matrix resource descriptor
	D3D12_RESOURCE_DESC camMatResourceDesc = {};
	camMatResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	camMatResourceDesc.Width = sizeOfCameraMatrix;
	camMatResourceDesc.Height = 1;
	camMatResourceDesc.DepthOrArraySize = 1;
	camMatResourceDesc.MipLevels = 1;
	camMatResourceDesc.SampleDesc.Count = 1;
	camMatResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// Create a resource heap for each resource
	for (int i = 0; i < NUMBER_OF_BACK_BUFFERS; i++)
	{
		HRESULT hr = m_Device5->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&camPosResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_CameraPositionResource[i])
		);

		hr = m_Device5->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&camMatResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_CameraVPMatrixResource[i])
		);



		//Heap corruption when closing
		hr = m_Device5->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&camMatResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_CameraViewMatrixResource[i])
		);

		hr = m_Device5->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&camMatResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_CameraProjectionMatrixResource[i])
		);
	}
}

void Renderer::CreateDescriptorHeap()
{
	if (m_Device5 != nullptr)
	{
		for (int i = 0; i < NUMBER_OF_BACK_BUFFERS; i++)
		{
			D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc = {};
			DescriptorHeapDesc.NumDescriptors = NUMBER_OF_HEAP_VIEWS;
			DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			m_Device5->CreateDescriptorHeap(&DescriptorHeapDesc, IID_PPV_ARGS(&m_DescriptorHeapsRenderer[i]));

			std::string tempString = "DescriptorHeap: " + std::to_string(i);
			std::wstring tempWstring(tempString.begin(), tempString.end());
			m_DescriptorHeapsRenderer[i]->SetName(tempWstring.c_str());


			D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDescriptorDesc = {};
			samplerHeapDescriptorDesc.NumDescriptors = 1;
			samplerHeapDescriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			samplerHeapDescriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			m_Device5->CreateDescriptorHeap(&samplerHeapDescriptorDesc, IID_PPV_ARGS(&m_DescriptorHeapsSampler[i]));

			tempString = "SamplerHeap: " + std::to_string(i);
			tempWstring = std::wstring(tempString.begin(), tempString.end());
			m_DescriptorHeapsRenderer[i]->SetName(tempWstring.c_str());
		}
	}
	else
	{
		printf("Device is null!");
	}
}

void Renderer::CreateSRVsToGBuffersRTVs()
{
	// Make SRVs that points to the gBuffers in m_GBufferRTResources
	D3D12_TEX2D_SRV tex2DSRV = {};
	tex2DSRV.ResourceMinLODClamp = 0;
	tex2DSRV.MostDetailedMip = 0;
	tex2DSRV.PlaneSlice = 0;
	tex2DSRV.MipLevels = 1;

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	ZeroMemory(&SRVDesc, sizeof(SRVDesc));
	SRVDesc.Texture2D = tex2DSRV;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	for (size_t i = 0; i < NUMBER_OF_BACK_BUFFERS; i++)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_DescriptorHeapsRenderer[i]->GetCPUDescriptorHandleForHeapStart();
		UINT64 stride = m_Device5->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		cdh.ptr += NUMBER_OF_HEAPRANGE_VIEWS * DeferredSP * stride;

		for (size_t j = 0; j < NUMBER_OF_DEFERRED_RTVS; j++)
		{
			m_Device5->CreateShaderResourceView(m_GBufferRTResources[j], &SRVDesc, cdh);
			cdh.ptr += stride;
		}
	}
}

void Renderer::CreateSampler()
{
	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MinLOD = 0.f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MipLODBias = 0.f;
	samplerDesc.MaxAnisotropy = 1;

	for (int i = 0; i < NUMBER_OF_BACK_BUFFERS; i++)
	{
		m_Device5->CreateSampler(&samplerDesc, m_DescriptorHeapsSampler[i]->GetCPUDescriptorHandleForHeapStart());
	}
}

void Renderer::CreateFence()
{
	m_Device5->CreateFence(m_FenceValueDirect, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_FenceDirect));

	m_Device5->CreateFence(m_FenceValueCompute, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_FenceCompute));

	m_Device5->CreateFence(m_FenceValueFrame, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_FenceFrame));

	m_Device5->CreateFence(m_FenceValueCopy, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_FenceCopy));
}

void Renderer::CreateEventHandle()
{
	m_EventHandle = CreateEvent(0, false, false, 0);
}

void Renderer::CreateStrides()
{
	if (m_HeapStrides != nullptr)
	{
		delete[] m_HeapStrides;
		m_HeapStrides = nullptr;
	}

	m_HeapStrides = new UINT64[NUMBER_OF_RESOURCE_TYPES];

	UINT64 IncrementSize = m_Device5->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_HeapStrides[PreFabs] = IncrementSize * SIZE_OF_BLOCK_PREFABS;
	m_HeapStrides[Light] = IncrementSize * SIZE_OF_BLOCK_COMPUTE;
	m_HeapStrides[Sampler] = IncrementSize * SIZE_OF_BLOCK_SAMPLER;
	m_HeapStrides[DeferredSP] = IncrementSize * SIZE_OF_BLOCK_DEFFERED_SP;
	m_HeapStrides[Skybox] = IncrementSize * SIZE_OF_BLOCK_SKYBOX;
}

bool Renderer::InitializeLightCulling()
{
	// Initiate start values
	for (size_t i = 0; i < TOTAL_NUMBER_OF_TILES; i++)
	{
		for (size_t j = 0; j < MAX_LIGHT; j++)
		{
			data[i].lightIndex[j] = 8;
		}
	}

	// Create UAV
	CreateTileDataUAVandSRV();

	// Upload start values in the UAV resource using the upload resource (do this once)
	UploadTileDataStartValues();

	// Copy uploaded values to the default resource (also once)
	CopyUploadedValuesToComputeResource();

	return true;
}

void Renderer::CreateTileDataUAVandSRV()
{
	UINT64 sizeInBytes = UINT64(sizeof(TileData) * TOTAL_NUMBER_OF_TILES);

	D3D12_RESOURCE_DESC resourceDesc;
	ZeroMemory(&resourceDesc, sizeof(resourceDesc));
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	// UAV
	D3D12_BUFFER_UAV BufferUAV;
	BufferUAV.CounterOffsetInBytes = 0;
	BufferUAV.FirstElement = 0;
	BufferUAV.NumElements = TOTAL_NUMBER_OF_TILES;
	BufferUAV.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	BufferUAV.StructureByteStride = sizeof(TileData);

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
	ZeroMemory(&UAVDesc, sizeof(UAVDesc));
	UAVDesc.Buffer = BufferUAV;
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	UAVDesc.Format = resourceDesc.Format;

	// SRV
	D3D12_BUFFER_SRV BufferSRV = {};
	BufferSRV.FirstElement = 0;
	BufferSRV.NumElements = 1;
	BufferSRV.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	BufferSRV.StructureByteStride = sizeof(TileData);

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	ZeroMemory(&SRVDesc, sizeof(SRVDesc));
	SRVDesc.Buffer = BufferSRV;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	// UAV RESOURCE 1
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	if (SUCCEEDED(m_Device5->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_UploadResource)
	)))
	{
	}

	UINT IncrementSize = m_Device5->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (size_t i = 0; i < Renderer::NUMBER_OF_BACK_BUFFERS; i++)
	{
		// UAV RESOURCE 2
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		if (SUCCEEDED(m_Device5->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&m_ComputeResource[i])
		)))
		{
		}


		D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_DescriptorHeapsRenderer[i]->GetCPUDescriptorHandleForHeapStart();
		cdh.ptr += UINT(NUMBER_OF_HEAPRANGE_VIEWS * IncrementSize * Light);

		m_Device5->CreateUnorderedAccessView(m_ComputeResource[i], nullptr, &UAVDesc, cdh);
		m_TileDataResource[i] = m_ComputeResource[i];
	}
}

void Renderer::UploadTileDataStartValues()
{
	// Init upload
	void* mappedMem = nullptr;
	D3D12_RANGE readRange = { 0, 0 }; // We do not intend to read this resource on the CPU
	if (SUCCEEDED(m_UploadResource->Map(0, &readRange, &mappedMem)))
	{
		memcpy(mappedMem, &data, sizeof(data));

		D3D12_RANGE writeRange = { 0, sizeof(data) };
		m_UploadResource->Unmap(0, &writeRange);
	}
}

void Renderer::CopyUploadedValuesToComputeResource()
{
	// Get Command Allocator and List + reset them
	m_CommandAllocatorCompute->Reset();
	m_CommandListCompute->Reset(m_CommandAllocatorCompute, nullptr);

	for (size_t i = 0; i < NUMBER_OF_BACK_BUFFERS; i++)
	{
		// Copy from upload heap to default heap
		SetResourceTransitionBarrier(m_CommandListCompute, m_ComputeResource[i], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
		m_CommandListCompute->CopyResource(m_ComputeResource[i], m_UploadResource);
		SetResourceTransitionBarrier(m_CommandListCompute, m_ComputeResource[i], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	// Close the list to prepare it for execution.
	m_CommandListCompute->Close();

	// Execute the command list.
	ID3D12CommandList* listsToExecute[] = { m_CommandListCompute };
	m_CommandQueueCompute->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

	m_FenceValueCompute++;
	m_CommandQueueCompute->Signal(m_FenceCompute, m_FenceValueCompute);
	while ((m_FenceCompute->GetCompletedValue() < m_FenceValueCompute));
}

void Renderer::CreateSkybox(GpuResource* resource)
{
	std::vector<ID3D12Resource*>* dxResources = resource->GetResources();

	if (dxResources != nullptr)
	{
		UINT sizeOfView = Renderer::Get()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		for (int i = 0; i < Renderer::NUMBER_OF_BACK_BUFFERS; i++)
		{

			D3D12_CPU_DESCRIPTOR_HANDLE cdh = Renderer::Get()->GetDescriptorHeaps(Skybox)[i]->GetCPUDescriptorHandleForHeapStart();

			cdh.ptr += sizeOfView * NUMBER_OF_HEAPRANGE_VIEWS * Skybox;

			int resourceIndex = resource->ConvertIndexToResourceindex(i);

			Renderer::Get()->GetDevice()->CreateShaderResourceView(dxResources->at(resourceIndex), &resource->GetViewType().SRV, cdh);

		}
	}


}

void Renderer::ClearBackBuffer()
{
	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

	// Indicate that the back buffer will be used as render target.
	SetResourceTransitionBarrier(m_CommandListDirect,
		m_RenderTargets[m_CurrentBackBufferIndex],
		D3D12_RESOURCE_STATE_PRESENT,		// State before
		D3D12_RESOURCE_STATE_RENDER_TARGET	// State after
	);

	//Get Pointers
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_RenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
	cdh.ptr += m_RenderTargetDescriptorSize * m_CurrentBackBufferIndex;

	m_CommandListDirect->OMSetRenderTargets(1, &cdh, false, nullptr);
	m_CommandListDirect->ClearRenderTargetView(cdh, CLEAR_COLOR, 0, nullptr);
}

void Renderer::CleargBuffersAndSetRTV()
{
	// Reset CommandList
	m_CommandAllocatorDirect->Reset();
	m_CommandListDirect->Reset(m_CommandAllocatorDirect, nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_GBufferRTsHeap->GetCPUDescriptorHandleForHeapStart();
	for (size_t i = 0; i < NUMBER_OF_DEFERRED_RTVS; i++)
	{
		m_CommandListDirect->ClearRenderTargetView(cdh, CLEAR_COLOR, 0, nullptr);
		cdh.ptr += m_RenderTargetDescriptorSize;
	}

	// Clear DSV
	D3D12_CPU_DESCRIPTOR_HANDLE cdhDSV = m_DSV_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_CommandListDirect->ClearDepthStencilView(cdhDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//RTV
	cdh = m_GBufferRTsHeap->GetCPUDescriptorHandleForHeapStart();
	m_CommandListDirect->OMSetRenderTargets(NUMBER_OF_DEFERRED_RTVS, &cdh, true, &cdhDSV);
}

void Renderer::SetResourceTransitionBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter)
{
	D3D12_RESOURCE_BARRIER barrierDesc = {};

	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = resource;
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = StateBefore;
	barrierDesc.Transition.StateAfter = StateAfter;

	commandList->ResourceBarrier(1, &barrierDesc);
}

void Renderer::SetRootSignatureCBVs(Camera* camera)
{
	void* mappedMem = nullptr;
	D3D12_RANGE readRange = { 0, 0 };
	if (SUCCEEDED(m_CameraPositionResource[m_CurrentBackBufferIndex]->Map(0, &readRange, &mappedMem)))
	{
		mappedMem = static_cast<char*>(mappedMem);

		memcpy(mappedMem, &camera->GetPosition(), sizeof(camera->GetPosition()));

		D3D12_RANGE writeRange = { sizeof(camera->GetPosition()) };
		m_CameraPositionResource[m_CurrentBackBufferIndex]->Unmap(0, &writeRange);
	}

	mappedMem = nullptr;
	readRange = { 0, 0 };
	if (SUCCEEDED(m_CameraVPMatrixResource[m_CurrentBackBufferIndex]->Map(0, &readRange, &mappedMem)))
	{
		mappedMem = static_cast<char*>(mappedMem);

		memcpy(mappedMem, &camera->GetViewProjectionMatrix(), sizeof(camera->GetViewProjectionMatrix()));

		D3D12_RANGE writeRange = { sizeof(camera->GetViewProjectionMatrix()) };
		m_CameraVPMatrixResource[m_CurrentBackBufferIndex]->Unmap(0, &writeRange);
	}

	m_CommandListDirect->SetGraphicsRootConstantBufferView(m_CBVCameraPositionLocation, m_CameraPositionResource[m_CurrentBackBufferIndex]->GetGPUVirtualAddress());
	m_CommandListDirect->SetGraphicsRootConstantBufferView(m_CBVCameraVPMatrixLocation, m_CameraVPMatrixResource[m_CurrentBackBufferIndex]->GetGPUVirtualAddress());
}

void Renderer::Present()
{
	// Indicate that the back buffer will now be used to present
	SetResourceTransitionBarrier(m_CommandListDirect,
		m_RenderTargets[m_CurrentBackBufferIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET,	//state before
		D3D12_RESOURCE_STATE_PRESENT		//state after
	);

	// Close the list to prepare it for execution
	m_CommandListDirect->Close();

	// Execute the command list
	ID3D12CommandList* listsToExecute[] = { m_CommandListDirect };
	m_CommandQueueDirect->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

	// Present the frame
	DXGI_PRESENT_PARAMETERS pp = {};
	m_SwapChain->Present1(0, 0, &pp);

	WaitForGPU(Direct);
}

void Renderer::WaitForGPU(CommandType type)
{
	if (type == Direct)
	{
		// Signal and increment the fence value
		m_FenceValueDirect++;
		const UINT64 fence = m_FenceValueDirect;
		m_CommandQueueDirect->Signal(m_FenceDirect, fence);

		// Wait until command queue is done
		if (m_FenceDirect->GetCompletedValue() < fence)
		{
			m_FenceDirect->SetEventOnCompletion(fence, m_EventHandle);
			WaitForSingleObject(m_EventHandle, INFINITE);
		}
	}
	else if (type == Compute)
	{
		// Signal and increment the fence value
		m_FenceValueCompute++;
		const UINT64 fence = m_FenceValueCompute;
		m_CommandQueueCompute->Signal(m_FenceCompute, fence);

		// Wait until command queue is done
		if (m_FenceCompute->GetCompletedValue() < fence)
		{
			m_FenceCompute->SetEventOnCompletion(fence, m_EventHandle);
			WaitForSingleObject(m_EventHandle, INFINITE);
		}
	}
	else if (type == Copy)
	{
		// Signal and increment the fence value
		m_FenceValueCopy++;
		const UINT64 fence = m_FenceValueCopy;
		m_CommandQueueCopy->Signal(m_FenceCopy, fence);

		// Wait until command queue is done
		if (m_FenceCopy->GetCompletedValue() < fence)
		{
			m_FenceCopy->SetEventOnCompletion(fence, m_EventHandle);
			WaitForSingleObject(m_EventHandle, INFINITE);
		}
	}
}

void Renderer::FrameDone(bool& workBool, bool& threadAlive) noexcept
{
	while (threadAlive)
	{
		if (workBool)
		{
			while ((m_FenceFrame->GetCompletedValue() < m_FenceValueFrame));

			Benchmarker::EndClock(GPUCLOCK, true);
			workBool = false;

		}
	}
}

void Renderer::UpdateCameraResources()
{
	if (m_RenderCamera)
	{
		void* mappedMem = nullptr;
		D3D12_RANGE readRange = { 0, 0 };
		XMFLOAT4 camPos = XMFLOAT4(m_RenderCamera->GetPosition().x, m_RenderCamera->GetPosition().y, m_RenderCamera->GetPosition().z, 1.f);
		if (SUCCEEDED(m_CameraPositionResource[m_CurrentBackBufferIndex]->Map(0, &readRange, &mappedMem)))
		{
			mappedMem = static_cast<char*>(mappedMem);

			memcpy(mappedMem, &camPos, sizeof(camPos));

			D3D12_RANGE writeRange = { sizeof(camPos) };
			m_CameraPositionResource[m_CurrentBackBufferIndex]->Unmap(0, &writeRange);
		}

		mappedMem = nullptr;
		readRange = { 0, 0 };
		if (SUCCEEDED(m_CameraVPMatrixResource[m_CurrentBackBufferIndex]->Map(0, &readRange, &mappedMem)))
		{
			mappedMem = static_cast<char*>(mappedMem);

			memcpy(mappedMem, &m_RenderCamera->GetViewProjectionMatrix(), sizeof(m_RenderCamera->GetViewProjectionMatrix()));

			D3D12_RANGE writeRange = { sizeof(m_RenderCamera->GetViewProjectionMatrix()) };
			m_CameraVPMatrixResource[m_CurrentBackBufferIndex]->Unmap(0, &writeRange);
		}

		mappedMem = nullptr;
		readRange = { 0, 0 };
		if (SUCCEEDED(m_CameraViewMatrixResource[m_CurrentBackBufferIndex]->Map(0, &readRange, &mappedMem)))
		{
			mappedMem = static_cast<char*>(mappedMem);

			memcpy(mappedMem, &m_RenderCamera->GetViewMatrix(), sizeof(m_RenderCamera->GetViewMatrix()));

			D3D12_RANGE writeRange = { sizeof(m_RenderCamera->GetViewMatrix()) };
			m_CameraViewMatrixResource[m_CurrentBackBufferIndex]->Unmap(0, &writeRange);
		}

		mappedMem = nullptr;
		readRange = { 0, 0 };
		if (SUCCEEDED(m_CameraProjectionMatrixResource[m_CurrentBackBufferIndex]->Map(0, &readRange, &mappedMem)))
		{
			mappedMem = static_cast<char*>(mappedMem);

			memcpy(mappedMem, &m_RenderCamera->GetProjectionMatrix(), sizeof(m_RenderCamera->GetProjectionMatrix()));

			D3D12_RANGE writeRange = { sizeof(m_RenderCamera->GetProjectionMatrix()) };
			m_CameraProjectionMatrixResource[m_CurrentBackBufferIndex]->Unmap(0, &writeRange);
		}
	}
}