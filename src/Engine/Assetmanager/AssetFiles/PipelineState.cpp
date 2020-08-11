#include "PipelineState.hpp"
#include "../../RenderEngine/Renderer.hpp"
#include "../Assetmanager.hpp"

#include <d3dcompiler.h>
#include <iostream>

#pragma comment (lib, "d3dcompiler.lib")

///// PIPELINESTATE INFO /////

PipeLineState::PipeLineState(const std::string vertexShader, const std::string fragmentShader, const std::string passType, const int nrOfRTVs, TopologyType topologyType, FillMode fillMode, CullMode cullMode, bool depthBuffer)
{
	VertexShader = vertexShader;
	FragmentShader = fragmentShader;
	ComputeShader = "";
	PassType = passType;
	NumberOfRTVs = nrOfRTVs;
	Topology = topologyType;
	Fill = fillMode;
	Cull = cullMode;
	DepthBuffer = depthBuffer;
}

PipeLineState::PipeLineState(const std::string computeShader, const std::string passType, TopologyType topologyType, FillMode fillMode, CullMode cullMode, bool depthBuffer)
{
	VertexShader = "";
	FragmentShader = "";
	ComputeShader = computeShader;
	PassType = passType;
	NumberOfRTVs = 0;
	Topology = topologyType;
	Fill = fillMode;
	Cull = cullMode;
	DepthBuffer = depthBuffer;
}

void PipeLineState::ResetPipeLineState()
{
	VertexShader = "";
	FragmentShader = "";
	ComputeShader = "";
	PassType = "";
	NumberOfRTVs = 1;
	Topology = TopologyType::Triangle;
	Fill = FillMode::Solid;
	Cull = CullMode::Back;
	DepthBuffer = false;
}



///// PIPELINESTATE LOADER /////
PipeLineStateLoader::~PipeLineStateLoader()
{
}

void PipeLineStateLoader::CreateGraphicsPipeLineStateObject(const PipeLineState& pipeLineState)
{
#pragma region Shader_Compiles
	std::string extension = ".hlsl";
	std::string path = "assets/Shaders/";
	std::wstring vsShaderFileName = std::wstring(path.begin(), path.end()) + std::wstring(pipeLineState.VertexShader.begin(), pipeLineState.VertexShader.end()) + std::wstring(extension.begin(), extension.end());
	std::wstring fsShaderFileName = std::wstring(path.begin(), path.end()) + std::wstring(pipeLineState.FragmentShader.begin(), pipeLineState.FragmentShader.end()) + std::wstring(extension.begin(), extension.end());

	///// COMPILE SHADERS //////
	ID3DBlob* vertexBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	if (SUCCEEDED(D3DCompileFromFile(
		vsShaderFileName.c_str(), // filename
		nullptr,		// optional macros
		nullptr,		// optional include files
		"main",			// entry point
		"vs_5_1",		// shader model (target)
		D3DCOMPILE_OPTIMIZATION_LEVEL3,				// shader compile options			// here DEBUGGING OPTIONS
		0,				// effect compile options
		&vertexBlob,	// double pointer to ID3DBlob		
		&errorBlob			// pointer for Error Blob messages.
						// how to use the Error blob, see here
						// https://msdn.microsoft.com/en-us/library/windows/desktop/hh968107(v=vs.85).aspx
	)))
	{
	}
	if(errorBlob)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		errorBlob->Release();
		errorBlob = nullptr;
	}


	ID3DBlob* fragmentBlob = nullptr;
	if (SUCCEEDED(D3DCompileFromFile(
		fsShaderFileName.c_str(), // filename
		nullptr,		// optional macros
		nullptr,		// optional include files
		"main",			// entry point
		"ps_5_1",		// shader model (target)
		D3DCOMPILE_OPTIMIZATION_LEVEL3,				// shader compile options			// here DEBUGGING OPTIONS
		0,				// effect compile options
		&fragmentBlob,	// double pointer to ID3DBlob		
		&errorBlob			// pointer for Error Blob messages.
						// how to use the Error blob, see here
						// https://msdn.microsoft.com/en-us/library/windows/desktop/hh968107(v=vs.85).aspx
	)))
	{
	}
	if (errorBlob)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		errorBlob->Release();
		errorBlob = nullptr;
	}

#pragma endregion Shader_Compiles

	////// COMPILE PIPELINESTATE //////
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};

	// Specify pipeline stages:
	gpsd.pRootSignature = Renderer::Get()->GetRootSignature();
	gpsd.VS.pShaderBytecode = reinterpret_cast<void*>(vertexBlob->GetBufferPointer());
	gpsd.VS.BytecodeLength = vertexBlob->GetBufferSize();
	gpsd.PS.pShaderBytecode = reinterpret_cast<void*>(fragmentBlob->GetBufferPointer());
	gpsd.PS.BytecodeLength = fragmentBlob->GetBufferSize();
	gpsd.GS.pShaderBytecode = nullptr;

	// Specify topology type
	{
		if (pipeLineState.Topology == TopologyType::Triangle)
			gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		else if (pipeLineState.Topology == TopologyType::Line)
			gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		else
			gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	}

	// Specify render target and depthstencil usage
	for (size_t i = 0; i < pipeLineState.NumberOfRTVs; i++)
	{
		if (pipeLineState.DepthBuffer)
		{
			gpsd.RTVFormats[i] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}
		else
		{
			gpsd.RTVFormats[i] = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}
	gpsd.NumRenderTargets = pipeLineState.NumberOfRTVs;

	gpsd.SampleDesc.Count = 1;
	gpsd.SampleMask = UINT_MAX;

	// Specify depth buffer
	if (pipeLineState.DepthBuffer)
	{
		// Set up Depth/Stencil
		D3D12_DEPTH_STENCIL_DESC DSVDesc;
		DSVDesc.DepthEnable = true;
		DSVDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		DSVDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		DSVDesc.StencilEnable = FALSE;
		DSVDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		DSVDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

		const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
		{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };

		DSVDesc.FrontFace = defaultStencilOp;
		DSVDesc.BackFace = defaultStencilOp;

		gpsd.DepthStencilState = DSVDesc;

		// Same as DSV Resource
		gpsd.DSVFormat = DXGI_FORMAT_D32_FLOAT;

		// Rasterizer
		D3D12_RASTERIZER_DESC rastDesc;
		rastDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rastDesc.CullMode = D3D12_CULL_MODE_NONE;
		rastDesc.FrontCounterClockwise = FALSE;
		rastDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		rastDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		rastDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		rastDesc.DepthClipEnable = TRUE;
		rastDesc.MultisampleEnable = FALSE;
		rastDesc.AntialiasedLineEnable = FALSE;
		rastDesc.ForcedSampleCount = 0;
		rastDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		gpsd.RasterizerState = rastDesc;
	}

	// Specify rasterizer behaviour.
	{
		if (pipeLineState.Fill == FillMode::Solid)
			gpsd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		else
			gpsd.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
			gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	}

	// Specify blend descriptions.
	D3D12_RENDER_TARGET_BLEND_DESC defaultRTdesc = {
		false, false,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsd.BlendState.RenderTarget[i] = defaultRTdesc;

	// Create PipeLineState
	if (SUCCEEDED(Renderer::Get()->GetDevice()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&Assetmanager::Get()->GetPipeLineState(pipeLineState.PassType)))))
	{
	}
	else
	{
		printf("Graphics Shader could not be created in CreateGraphicsPipeLineStateObject");
	}

	// Release the blobs
	vertexBlob->Release();
	fragmentBlob->Release();
}

void PipeLineStateLoader::CreateComputePipeLineStateObject(const PipeLineState& pipeLineState)
{
	std::string extension = ".hlsl";
	std::string path = "assets/Shaders/";
	std::wstring computeShaderFileName = std::wstring(path.begin(), path.end()) + std::wstring(pipeLineState.ComputeShader.begin(), pipeLineState.ComputeShader.end()) + std::wstring(extension.begin(), extension.end());

	///// COMPILE SHADERS //////
	ID3DBlob* computeBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	D3DCompileFromFile(
		computeShaderFileName.c_str(), // filename
		nullptr,		// optional macros
		nullptr,		// optional include files
		"main",		// entry point
		"cs_5_1",		// shader model (target)
		D3DCOMPILE_OPTIMIZATION_LEVEL3,				// shader compile options			// here DEBUGGING OPTIONS
		0,				// effect compile options
		&computeBlob,	// double pointer to ID3DBlob		
		&errorBlob			// pointer for Error Blob messages.
						// how to use the Error blob, see here
						// https://msdn.microsoft.com/en-us/library/windows/desktop/hh968107(v=vs.85).aspx
	);

	if (errorBlob)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		errorBlob->Release();
	}

	////// COMPILE PIPELINESTATE //////
	D3D12_COMPUTE_PIPELINE_STATE_DESC cpsd = {};

	//Specify pipeline stages:
	cpsd.pRootSignature = Renderer::Get()->GetRootSignature();
	cpsd.CS.pShaderBytecode = reinterpret_cast<void*>(computeBlob->GetBufferPointer());
	cpsd.CS.BytecodeLength = computeBlob->GetBufferSize();


	// Create PipeLineState
	if (SUCCEEDED(Renderer::Get()->GetDevice()->CreateComputePipelineState(&cpsd, IID_PPV_ARGS(&Assetmanager::Get()->GetPipeLineState(pipeLineState.PassType)))))
	{
	}
	else
	{
		printf("Compute Shader could not be created in CreateComputePipeLineStateObject");
	}

	// Release the blobs
	computeBlob->Release();
}