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

#include <stdafx.h>
#include "texture_d3d12.h"

#include <backends/d3d12/impl/D3DSamplerDesc.h>

extern "C" {
#include "log.h"
#include "memory.h"
}

#include "gpu_ctx_d3d12.h"
#include "util_d3d12.h"

struct texture* d3d12_texture_create(struct gpu_ctx* gpu_ctx)
{
    texture_d3d12* s = (texture_d3d12*)ngli_calloc(1, sizeof(*s));
    if(!s)
        return NULL;
    s->parent.gpu_ctx = gpu_ctx;
    return (struct texture*)s;
}

int d3d12_texture_init(struct texture* s, const struct texture_params* p)
{
    struct texture_d3d12* s_priv = (struct texture_d3d12*)s;
    struct gpu_ctx_d3d12* ctx = (struct gpu_ctx_d3d12*)s->gpu_ctx;
    auto& gpu_ctx = ctx->graphics_context;

    s->params = *p;
    s_priv->bytes_per_pixel = get_bpp(p->format);
    bool gen_mipmaps = p->mipmap_filter != NGLI_MIPMAP_FILTER_NONE;

    uint32_t depth = (p->type == NGLI_TEXTURE_TYPE_3D) ? p->depth : 1;
    s->params.depth = depth;
    uint32_t array_layers = (p->type == NGLI_TEXTURE_TYPE_CUBE) ? 6 : 1;
    uint32_t size =
        s_priv->bytes_per_pixel * p->width * p->height * depth * array_layers;

    D3D12_FILTER_REDUCTION_TYPE reduction = D3D12_FILTER_REDUCTION_TYPE_STANDARD;

    ngli::D3DSamplerDesc samplerDesc;
    samplerDesc.minFilter = (FilterMode)p->min_filter;
    samplerDesc.magFilter = (FilterMode)p->mag_filter;
    samplerDesc.mipmapFilter = (MipMapFilterMode)p->mipmap_filter;
    samplerDesc.Filter = D3D12_ENCODE_BASIC_FILTER(samplerDesc.minFilter == NGLI_FILTER_LINEAR? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT,
                                                   samplerDesc.magFilter == NGLI_FILTER_LINEAR? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT,
                                                   samplerDesc.mipmapFilter == NGLI_MIPMAP_FILTER_LINEAR? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT,
                                                   reduction);
    samplerDesc.AddressU = to_d3d12_wrap_mode(p->wrap_s);
    samplerDesc.AddressV = to_d3d12_wrap_mode(p->wrap_t);
    samplerDesc.AddressW = to_d3d12_wrap_mode(p->wrap_r);

    s_priv->v = ngli::D3DTexture::newInstance(
        gpu_ctx, ctx->graphics, nullptr, to_d3d12_format(p->format), size,
        p->width, p->height, depth, array_layers, p->usage,
        to_d3d12_texture_type(p->type), gen_mipmaps,
        p->samples == 0 ? 1 : p->samples, &samplerDesc);

    return 0;
}

int d3d12_texture_upload(struct texture* s, const uint8_t* data,
                             int linesize)
{
    texture_d3d12* texture = (struct texture_d3d12*)s;
    if(linesize == 0)
        linesize = s->params.width;
    uint32_t size = texture->bytes_per_pixel * linesize * s->params.height *
        s->params.depth * texture->v->arrayLayers;
    texture->v->upload((void*)data, size, 0, 0, 0, s->params.width,
                       s->params.height, s->params.depth,
                       texture->v->arrayLayers, -1,
                       texture->bytes_per_pixel * linesize);
    return 0;
}

int d3d12_texture_generate_mipmap(struct texture* s)
{
    texture_d3d12* texture = (struct texture_d3d12*)s;
    gpu_ctx_d3d12* gpu_ctx = (gpu_ctx_d3d12*)s->gpu_ctx;
    ngli::D3DCommandList* cmd_buffer = gpu_ctx->cur_command_buffer;
    texture->v->generateMipmaps(cmd_buffer);
    if(texture->v->imageUsageFlags & NGLI_TEXTURE_USAGE_SAMPLED_BIT)
    {
        texture->v->changeLayout(cmd_buffer,
                                 ngli::IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    return 0;
}

void d3d12_texture_freep(struct texture** sp)
{
    if(!sp)
        return;
    texture* s = *sp;

    // Wait until the texture has been upload by ExecuteCommandLists to the gpu before releasing it.
    struct gpu_ctx* ctx = (struct gpu_ctx*)s->gpu_ctx;
    ctx->cls->wait_idle(ctx);

    texture_d3d12* s_priv = (struct texture_d3d12*)s;
    delete s_priv->v;
    ngli_freep(sp);
}

