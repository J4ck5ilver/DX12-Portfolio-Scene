#ifndef PIPELINESTATE_HPP
#define PIPELINESTATE_HPP

#include <d3d12.h>
#include <d3dcompiler.h>

#include <string>
#include <vector>

enum class TopologyType { Triangle, Line, Point };
enum class FillMode { Solid, WireFrame };
enum class CullMode { Back, Front, None };

///// PIPELINESTATE INFO /////
struct PipeLineState
{
	PipeLineState() = default;
	PipeLineState(const std::string vertexShader, const std::string fragmentShader, const std::string passType, const int nrOfRTVs = 1,
				TopologyType topologyType = TopologyType::Triangle, FillMode fillMode = FillMode::Solid, CullMode cullMode = CullMode::Back, bool depthBuffer = false);
	PipeLineState(const std::string computeShader, const std::string passType,
		TopologyType topologyType = TopologyType::Triangle, FillMode fillMode = FillMode::Solid, CullMode cullMode = CullMode::Back, bool depthBuffer = false);
	virtual ~PipeLineState() = default;

	void ResetPipeLineState();

	std::string VertexShader{ "" };
	std::string FragmentShader{ "" };
	std::string ComputeShader{ "" };
	std::string PassType{ "" };
	int NumberOfRTVs{ 1 };
	TopologyType Topology{ TopologyType::Triangle };
	FillMode Fill{ FillMode::Solid };
	CullMode Cull{ CullMode::Back };
	bool DepthBuffer{ false };
};

///// PIPELINESTATE LOADER /////
class PipeLineStateLoader
{
public:
	PipeLineStateLoader() = default;
	virtual ~PipeLineStateLoader();

	PipeLineStateLoader(const PipeLineStateLoader& obj) = delete;
	void operator=(const PipeLineStateLoader& obj) = delete;

	void CreateGraphicsPipeLineStateObject(const PipeLineState& pipeLineState);
	void CreateComputePipeLineStateObject(const PipeLineState& pipeLineState);

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
};
#endif