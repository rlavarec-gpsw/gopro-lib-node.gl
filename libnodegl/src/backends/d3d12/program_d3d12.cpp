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
#include <backends/d3d12/impl/D3DShaderModule.h>
#include <backends/d3d12/impl/D3DShaderCompiler.h>



extern "C" {

    struct program* d3d12_program_create(struct gpu_ctx* gpu_ctx)
    {
        program_d3d12* s = (program_d3d12*)ngli_calloc(1, sizeof(*s));
        if(!s)
            return NULL;
        s->parent.gpu_ctx = gpu_ctx;
        return (struct program*)s;
    }



    int d3d12_program_init(struct program* s, const program_params* p)
    {
        gpu_ctx_d3d12* gpu_ctx = (gpu_ctx_d3d12*)s->gpu_ctx;
        program_d3d12* program = (program_d3d12*)s;
        try
        {
            if(p->vertex)
            {
                ngli::D3DShaderCompiler sc;
                program->vs =
                    ngli::D3DVertexShaderModule::newInstance(gpu_ctx->graphics_context->device,
                                               sc.compile(p->vertex, ".vert"))
                    .release();
            }
            if(p->fragment)
            {
                ngli::D3DShaderCompiler sc;
                program->fs =
                    ngli::D3DFragmentShaderModule::newInstance(gpu_ctx->graphics_context->device,
                                                 sc.compile(p->fragment, ".frag"))
                    .release();
            }
            if(p->compute)
            {
                ngli::D3DShaderCompiler sc;
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