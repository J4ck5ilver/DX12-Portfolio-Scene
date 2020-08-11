#ifndef THE_RENDERER_HPP
#define THE_RENDERER_HPP

#include "Camera/Camera.hpp"

#include<Windows.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<DirectXMath.h>
#define SDL_MAIN_HANDLED
#include<SDL/SDL.h>
#include<cstdint>
#include<string>
#include <thread>
#include <future>
#include"..\Core\Becnhmarker.hpp"

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "DXGI.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "d3dcompiler.lib")

enum RenderResourceType { PreFabs, Light, Sampler, DeferredSP, Skybox };
enum CommandType { Direct, Compute, Copy };

constexpr auto NUMBER_OF_RESOURCE_TYPES = 5;
constexpr auto MAX_LIGHT = 500;
constexpr auto GRID_SIZE = 32;
constexpr auto TOTAL_NUMBER_OF_TILES = GRID_SIZE * GRID_SIZE;

using namespace DirectX;
struct TileData
{
	unsigned int count = 0;
	unsigned int lightIndex[MAX_LIGHT];
};

class Renderer
{
public:
	Renderer(const Renderer& obj) = delete;
	Renderer operator=(const Renderer& obj) = delete;

	static Renderer* Get() noexcept;
	bool Initialize(const XMUINT2& windowSize, const std::string& winName = "DirectX 12");
	void InitThreads();
	void OnNewFrame();
	void UpdateCameraResources();
	inline void SetRenderCamera(Camera* camera) noexcept { m_RenderCamera = camera; }

	// Render
	void Render();

	//Skybox
	void CreateSkybox(GpuResource* resource);

	// Get
	ID3D12Device5* GetDevice();
	ID3D12CommandQueue* GetCommandQueue(CommandType type);
	ID3D12GraphicsCommandList4* GetCommandList(CommandType type);
	ID3D12CommandAllocator* GetCommandAllocator(CommandType type);
	const UINT8 GetCurrentBackBufferIndex();
	ID3D12RootSignature* GetRootSignature();
	ID3D12DescriptorHeap** GetDescriptorHeaps(RenderResourceType type = PreFabs);
	ID3D12DescriptorHeap* GetActiveDescriptorHeap(RenderResourceType type);
	D3D12_GPU_DESCRIPTOR_HANDLE GetActiveHeapStart(const RenderResourceType& type);
	const UINT64& GetHeapStride(const RenderResourceType& type) noexcept;
	const UINT64 GetNumberOfViewsPerSubHeap();
	inline SDL_Window* Get_STDL_Window() noexcept { return m_SDLWindow; }
	XMUINT3 GetRootConstantBufferLocations();
	ID3D12Resource1* GetActiveCameraPositionresource();
	ID3D12Resource1* GetActiveCameraMatrixresource();
	UINT GetNumberOfPointLights();

	void CopyLightResources();

	inline static void Shutdown() { delete s_Instance; }


private:
	Renderer() noexcept = default;
	virtual ~Renderer() noexcept;

	// Initialization
	void CreateDevice();
	void CreateScissorAndViewport();
	void CreateCommandInterfacesAndSwapChain();
	void CreateRenderTargets();
	void CreateDSV();
	void CreateGBuffers();
	void CreateRootSignature();
	void CreateRootSignatureCBVs();
	void CreateDescriptorHeap();
	void CreateSRVsToGBuffersRTVs();
	void CreateSampler();
	void CreateFence();
	void CreateEventHandle();
	void CreateStrides();
	// Initialization light culling
	bool InitializeLightCulling();
	void CreateTileDataUAVandSRV();
	void UploadTileDataStartValues();
	void CopyUploadedValuesToComputeResource();



	// Record List
	void RecordFirstPass(ID3D12GraphicsCommandList4*& comList, ID3D12CommandAllocator** allocator, bool& workBool, bool& threadAlive);
	void MainRecordSecondPass(ID3D12GraphicsCommandList4*& comList, ID3D12CommandAllocator** allocator);
	void ExecuteLightCulling(ID3D12GraphicsCommandList4*& comList, ID3D12CommandAllocator** allocator, bool& workBool, bool& threadAlive);

	// RenderPass
	void ClearBackBuffer();
	void CleargBuffersAndSetRTV();
	void SetResourceTransitionBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter);
	void SetRootSignatureCBVs(Camera* camera);
	void Present();
	void WaitForGPU(CommandType type);


	//Time function
	void FrameDone(bool& workBool, bool& threadAlive) noexcept;

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
	// Public constants
			// Render buffers
	static const UINT8 NUMBER_OF_BACK_BUFFERS = 2;
	static const UINT8 NUMBER_OF_DEFERRED_RTVS = 4;
	// Block size in heaps
	static const UINT8 SIZE_OF_BLOCK_PREFABS = 10;
	static const UINT8 SIZE_OF_BLOCK_COMPUTE = 1;
	static const UINT8 SIZE_OF_BLOCK_SAMPLER = 1;
	static const UINT8 SIZE_OF_BLOCK_DEFFERED_SP = 5;
	static const UINT8 SIZE_OF_BLOCK_SKYBOX = 1;

private:
	static Renderer* s_Instance;

	// Private constants
	const FLOAT CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.f };
	const UINT64 NUMBER_OF_HEAP_VIEWS{ (UINT64)1e6 }; // Total number of views in the main Descriptor Heap
	const UINT64 NUMBER_OF_HEAPRANGE_VIEWS = NUMBER_OF_HEAP_VIEWS / 5;

	// Threads
	std::thread* m_RecordList1Thread = nullptr;
	bool m_RecordFirstPass = false;
	bool m_Thread1Alive = true;
	std::thread* m_RecordList2Thread = nullptr;
	bool m_RecordCompute = false;
	bool m_Thread2Alive = true;

	std::thread* m_FrameDoneThread = nullptr;
	bool m_ThreadFrameDoneAlive = true;
	bool m_FrameRecording = false;


	// Debug variable
	int m_CompleteCounter = 0;

	// Render Camera
	Camera* m_RenderCamera;
	UINT m_NumberOfPointLights;

	// Window
	HWND m_HWND = {};
	SDL_Window* m_SDLWindow = nullptr;
	XMUINT2 m_WindowSize = { 0,0 };

	// Device
	ID3D12Device5* m_Device5;

	// Backbuffers
	UINT8 m_CurrentBackBufferIndex = 0;

	// SwapChain
	IDXGISwapChain4* m_SwapChain = nullptr;

	// Command Handlers
			// Direct
	ID3D12CommandQueue* m_CommandQueueDirect = nullptr;
	ID3D12CommandAllocator* m_CommandAllocatorDirect = nullptr;
	ID3D12GraphicsCommandList4* m_CommandListDirect = nullptr;
	// Compute
	ID3D12CommandQueue* m_CommandQueueCompute = nullptr;
	ID3D12CommandAllocator* m_CommandAllocatorCompute = nullptr;
	ID3D12GraphicsCommandList4* m_CommandListCompute = nullptr;
	// Copy
	ID3D12CommandQueue* m_CommandQueueCopy = nullptr;
	ID3D12CommandAllocator* m_CommandAllocatorCopy = nullptr;
	ID3D12GraphicsCommandList4* m_CommandListCopy = nullptr;

	// FirstPass List + Allocator
	ID3D12GraphicsCommandList4* m_ComList_FP = nullptr;
	ID3D12CommandAllocator* m_Allocator_FP[NUMBER_OF_BACK_BUFFERS]{ nullptr, nullptr }; // Two are enough but for simplicity we use NUMBER_OF_BACK_BUFFERS

	// Compute List + Allocator
	ID3D12GraphicsCommandList4* m_ComList_Compute = nullptr;
	ID3D12CommandAllocator* m_Allocator_Compute[NUMBER_OF_BACK_BUFFERS]{ nullptr, nullptr }; // Two are enough but for simplicity we use NUMBER_OF_BACK_BUFFERS

	// SecondPass List + Allocator
	ID3D12GraphicsCommandList4* m_ComList_SP = nullptr;
	ID3D12CommandAllocator* m_Allocator_SP[NUMBER_OF_BACK_BUFFERS]{ nullptr, nullptr }; // Two are enough but for simplicity we use NUMBER_OF_BACK_BUFFERS

	// Viewport and ScissorRect
	D3D12_VIEWPORT m_Viewport = {};
	D3D12_RECT m_ScissorRect = {};

	// DepthStencilHeap
	ID3D12DescriptorHeap* m_DSV_DescriptorHeap = nullptr;
	ID3D12Resource1* m_DSV_Texture = nullptr;
	D3D12_SHADER_RESOURCE_VIEW_DESC m_DSV_SRVDesc;

	// RenderTargets
	ID3D12DescriptorHeap* m_RenderTargetsHeap = nullptr;
	ID3D12Resource1* m_RenderTargets[NUMBER_OF_BACK_BUFFERS] = {};
	UINT8 m_RenderTargetDescriptorSize = 0;

	// gBuffer RenderTargets
	ID3D12DescriptorHeap* m_GBufferRTsHeap = nullptr;
	ID3D12Resource1* m_GBufferRTResources[NUMBER_OF_DEFERRED_RTVS] = {};
	UINT8 m_GBufferRTDescriptorSize = 0;

	// Rootsignature
	ID3D12RootSignature* m_RootSignature = nullptr;

	// CBV resources and their respective Root Signature locations
	ID3D12Resource1* m_CameraPositionResource[NUMBER_OF_BACK_BUFFERS] = {};
	ID3D12Resource1* m_CameraVPMatrixResource[NUMBER_OF_BACK_BUFFERS] = {};
	ID3D12Resource1* m_CameraViewMatrixResource[NUMBER_OF_BACK_BUFFERS] = {};
	ID3D12Resource1* m_CameraProjectionMatrixResource[NUMBER_OF_BACK_BUFFERS] = {};
	UINT m_CBVCameraPositionLocation = 0;
	UINT m_CBVCameraVPMatrixLocation = 0;
	UINT m_CBVCameraViewMatrixLocation = 0;
	UINT m_CBVCameraProjectionMatrixLocation = 0;

	// CB light Root Signature location
	UINT m_CBLightLocation = 0;

	// SRV resources and their respective Root Signature locations
	ID3D12Resource1* m_TileDataResource[NUMBER_OF_BACK_BUFFERS] = {};
	UINT m_SRVTileDataLocation = 0;

	//Skybox
	UINT m_SRVSkyboxLocation = 0;


	// DescriptorHeap
	ID3D12DescriptorHeap* m_DescriptorHeapsRenderer[NUMBER_OF_BACK_BUFFERS]{ nullptr, nullptr };
	ID3D12DescriptorHeap* m_DescriptorHeapsSampler[NUMBER_OF_BACK_BUFFERS]{ nullptr, nullptr };
	UINT64* m_HeapStrides{ nullptr };

	// Fence and EventHandel
	HANDLE m_EventHandle = nullptr;
	//Direct
	ID3D12Fence1* m_FenceDirect = nullptr;
	UINT64 m_FenceValueDirect = 0;
	//Compute
	ID3D12Fence1* m_FenceCompute = nullptr;
	UINT64 m_FenceValueCompute = 0;
	// Copy
	ID3D12Fence1* m_FenceCopy = nullptr;
	UINT64 m_FenceValueCopy = 0;
	// Frame
	ID3D12Fence1* m_FenceFrame = nullptr;
	UINT64 m_FenceValueFrame = 0;

	// Light Culling
	TileData data[TOTAL_NUMBER_OF_TILES];

	// UAV compute resources
	ID3D12Resource1* m_UploadResource = nullptr;
	ID3D12Resource1* m_ComputeResource[NUMBER_OF_BACK_BUFFERS]{ nullptr, nullptr };

};
#endif // !THE_RENDERER_HPP