
#include <stdafx.h>
#include "D3DShaderCross.h"

#include <backends/common/ShaderTools.h>

namespace ngli
{

D3DShaderCross::D3DShaderCross(const std::string& directory, const std::string& vert, const std::string& frag, const std::string& comp)
{
	compile(directory, vert);
	compile(directory, frag);
	compile(directory, comp);
}

void D3DShaderCross::compile(const std::string& dir, const std::string& file)
{
	if(file.empty())
	{
		return;
	}

	static ngli::ShaderTools shaderTools(true);
	std::string outDir = dir;
	
	// If the dir is empty, get it from the filename
	if(outDir.empty())
	{
		outDir = std::filesystem::path(file).remove_filename().string();
	}
	
	ngli_assert(!outDir.empty());

	std::vector<std::string> glslFiles = { dir + file };

	auto spvFiles = shaderTools.compileShaders(glslFiles, outDir, ShaderTools::FORMAT_GLSL);
	auto spvMapFiles = shaderTools.generateShaderMaps(glslFiles, outDir, ShaderTools::FORMAT_GLSL);
	auto hlslFiles = shaderTools.convertShaders(spvFiles, outDir, ShaderTools::FORMAT_HLSL);
	auto dxcFiles = shaderTools.compileShaders(hlslFiles, outDir, ShaderTools::FORMAT_HLSL);
	auto hlslMapFiles = shaderTools.generateShaderMaps(hlslFiles, outDir, ShaderTools::FORMAT_HLSL);
}

}