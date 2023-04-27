#pragma once

#include <backends/common/FileUtil.h>
#include <backends/common/ShaderTools.h>

static ngli::ShaderTools shaderTools(DEBUG_D3D12_ACTIVATED);

namespace ngli
{

static char get_d3d12_backend_id()
{
    return 'd';
}

struct D3DShaderCompiler
{
    /// @brief Take the source of a file with an extension, generate a filename from the hash of the content of the file
    /// @param src source of the file
    /// @param ext extension
    /// @return temp file where the source was dump
    std::string dumpSrcIntoFile(const std::string& src, const std::string& ext)
    {
        size_t h1 = std::hash<std::string>{}(get_d3d12_backend_id() + src);
        tmpDir = std::filesystem::path(ngli::FileUtil::tempDir() + "/" + "nodegl")
            .make_preferred()
            .string();
        std::filesystem::create_directories(tmpDir);
        std::string tmpFile = std::filesystem::path(tmpDir + "/" + "tmp_" + std::to_string(h1) + ext)
            .make_preferred()
            .string();
        ngli::FileUtil::Lock lock(tmpFile);
        if(!std::filesystem::exists(tmpFile))
        {
            ngli::FileUtil::writeFile(tmpFile, src);
        }
        return tmpFile;
    }

    /// @brief Compile a source
    /// @param src source of the file to compile
    /// @param ext extension
    /// @return the path including the filename without extension
    std::string compile(const std::string& src, const std::string& ext)
    {
        std::string tmpFile = dumpSrcIntoFile(src, ext);

        std::string outDir = tmpDir;
        glslFiles = { tmpFile };
        int flags = ngli::ShaderTools::PATCH_SHADER_LAYOUTS_GLSL/* |
            ngli::ShaderTools::REMOVE_UNUSED_VARIABLES*/;
        flags |= ngli::ShaderTools::FLIP_VERT_Y;

        spvFiles = shaderTools.compileShaders(
            glslFiles, outDir, ngli::ShaderTools::FORMAT_GLSL, {}, flags);

        hlslFiles = shaderTools.convertShaders(spvFiles, outDir, ngli::ShaderTools::FORMAT_HLSL);
        dxcFiles = shaderTools.compileShaders(hlslFiles, outDir, ngli::ShaderTools::FORMAT_HLSL);
        hlslMapFiles = shaderTools.generateShaderMaps(hlslFiles, outDir, ngli::ShaderTools::FORMAT_HLSL);

        return ngli::FileUtil::splitExt(spvFiles[0])[0];
    }

    // Cache compiled shader programs in temp folder
    std::string tmpDir;
    std::vector<std::string> glslFiles, spvFiles, glslMapFiles;
    std::vector<std::string> hlslFiles, dxcFiles, hlslMapFiles;
};

}