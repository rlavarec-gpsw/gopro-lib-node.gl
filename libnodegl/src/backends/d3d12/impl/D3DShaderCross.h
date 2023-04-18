#pragma once

namespace ngli
{

class D3DShaderCross
{
public:
	/// @brief Enable to cross compile shader in hlsl, will ouput generated shader in same directory
	/// @param directory Path where to find the shader
	/// @param vert Vertex shader filename with extension
	/// @param frag Fragment shader filename with extension
	/// @param comp Compute shader filename with extension
	D3DShaderCross(const std::string& directory, const std::string& vert, const std::string& frag, const std::string& comp = "");

private:
	void compile(const std::string& dir, const std::string& file);
};

}