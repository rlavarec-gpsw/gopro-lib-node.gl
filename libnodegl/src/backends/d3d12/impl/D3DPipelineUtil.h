#pragma once	
#include <backends/d3d12/impl/D3DShaderModule.h>

namespace ngli
{

struct D3DPipelineUtil
{
	enum PipelineType { PIPELINE_TYPE_COMPUTE, PIPELINE_TYPE_GRAPHICS };
	using IsReadOnly = std::function<bool(const D3DShaderModule::DescriptorInfo& info)>;
	static void
		parseDescriptors(std::map<uint32_t, D3DShaderModule::DescriptorInfo>& uniforms,
			std::vector<uint32_t>& uniformBindings,
			std::vector<CD3DX12_ROOT_PARAMETER1>& d3dRootParams,
			std::vector<std::unique_ptr<CD3DX12_DESCRIPTOR_RANGE1>>& d3dDescriptorRanges,
			PipelineType pipelineType,
			IsReadOnly& isReadOnly);
};

}