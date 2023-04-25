/*
 * Copyright 2018 GoPro Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "program_d3d12.h"
extern "C" {
#include "memory.h"
}
#include "gpu_ctx_d3d12.h"/*
#include "ngfx/core/FileUtil.h"
#include "ngfx/core/ProcessUtil.h"
#include "ngfx/core/Util.h"*/

#include <backends/common/FileUtil.h>
#include <backends/common/ShaderTools.h>
#include <backends/d3d12/impl/D3DShaderModule.h>



namespace fs = std::filesystem;

enum { DEBUG_FLAG_VERBOSE = 1 };

static int DEBUG_FLAGS = DEBUG_FLAG_VERBOSE;
static ngli::ShaderTools shaderTools(DEBUG_FLAGS& DEBUG_FLAG_VERBOSE);

extern "C" {

    struct program* d3d12_program_create(struct gpu_ctx* gpu_ctx)
    {
        program_d3d12* s = (program_d3d12*)ngli_calloc(1, sizeof(*s));
        if(!s)
            return NULL;
        s->parent.gpu_ctx = gpu_ctx;
        return (struct program*)s;
    }


    static char get_d3d12_backend_id()
    {
        return 'd';
    }

    struct ShaderCompiler
    {
        std::string compile(std::string src, const std::string& ext)
        {
            size_t h1 = std::hash<std::string>{}(get_d3d12_backend_id() + src);
            tmpDir = fs::path(ngli::FileUtil::tempDir() + "/" + "nodegl")
                .make_preferred()
                .string();
            fs::create_directories(tmpDir);
            std::string tmpFile = fs::path(tmpDir + "/" + "tmp_" + std::to_string(h1) + ext)
                .make_preferred()
                .string();
            ngli::FileUtil::Lock lock(tmpFile);
            if(!fs::exists(tmpFile))
            {
                ngli::FileUtil::writeFile(tmpFile, src);
            }
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

    int d3d12_program_init(struct program* s, const program_params* p)
    {
        gpu_ctx_d3d12* gpu_ctx = (gpu_ctx_d3d12*)s->gpu_ctx;
        program_d3d12* program = (program_d3d12*)s;
        try
        {
            if(p->vertex)
            {
                ShaderCompiler sc;
                program->vs =
                    ngli::D3DVertexShaderModule::newInstance(gpu_ctx->graphics_context->device,
                                               sc.compile(p->vertex, ".vert"))
                    .release();
            }
            if(p->fragment)
            {
                ShaderCompiler sc;
                program->fs =
                    ngli::D3DFragmentShaderModule::newInstance(gpu_ctx->graphics_context->device,
                                                 sc.compile(p->fragment, ".frag"))
                    .release();
            }
            if(p->compute)
            {
                ShaderCompiler sc;
                program->cs =
                    ngli::D3DComputeShaderModule::newInstance(gpu_ctx->graphics_context->device,
                                                sc.compile(p->compute, ".comp"))
                    .release();
            }
        }
        catch(std::exception e)
        {
            return NGL_ERROR_EXTERNAL;
        }
        return 0;
    }

    void d3d12_program_freep(struct program** sp)
    {
        program_d3d12* program = (program_d3d12*)*sp;
        if(program->vs)
        {
            delete program->vs;
        }
        if(program->fs)
        {
            delete program->fs;
        }
        if(program->cs)
        {
            delete program->cs;
        }
        ngli_freep(sp);
    }
}
