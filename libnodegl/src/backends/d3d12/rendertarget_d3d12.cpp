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

extern "C" {
#include "log.h"
#include "memory.h"
}
#include "gpu_ctx_d3d12.h"
#include "rendertarget_d3d12.h"
#include "texture_d3d12.h"
#include "util_d3d12.h"
#include <glm/gtc/type_ptr.hpp>
#include <vector>

struct rendertarget* ngli_rendertarget_d3d12_create(struct gpu_ctx* gpu_ctx)
{
    rendertarget_d3d12* s = new rendertarget_d3d12;
    if(!s)
        return NULL;
    s->parent.gpu_ctx = gpu_ctx;
    return (struct rendertarget*)s;
}

int ngli_rendertarget_d3d12_init(struct rendertarget* s,
                                const struct rendertarget_params* params)
{
    rendertarget_d3d12* s_priv = (rendertarget_d3d12*)s;
    gpu_ctx_d3d12* gpu_ctx = (gpu_ctx_d3d12*)s->gpu_ctx;
    auto& ctx = gpu_ctx->graphics_context;

    s->width = params->width;
    s->height = params->height;
    s->params = *params;

    std::vector<ngli::D3DFramebuffer::Attachment> attachments;
    uint32_t w = 0, h = 0;

    for(int i = 0; i < params->nb_colors; i++)
    {
        const attachment* color_attachment = &params->colors[i];
        const texture_d3d12* color_texture =
            (const texture_d3d12*)color_attachment->attachment;
        const texture_d3d12* resolve_texture =
            (const texture_d3d12*)color_attachment->resolve_target;
        const texture_params* color_texture_params =
            &color_texture->parent.params;
        if(i == 0)
        {
            w = color_texture->v->w;
            h = color_texture->v->h;
        }
        attachments.push_back({ color_texture->v, 0,
                               uint32_t(color_attachment->attachment_layer) });
        if(resolve_texture)
            attachments.push_back(
                { resolve_texture->v, 0,
                 uint32_t(color_attachment->resolve_target_layer) });
    }

    const attachment* depth_attachment = &params->depth_stencil;
    const texture_d3d12* depth_texture =
        (const texture_d3d12*)depth_attachment->attachment;
    if(depth_texture)
    {
        const texture_d3d12* resolve_texture =
            (const texture_d3d12*)depth_attachment->resolve_target;
        const texture_params* depth_texture_params =
            &depth_texture->parent.params;
        attachments.push_back({ depth_texture->v });
        if(resolve_texture)
            attachments.push_back({ resolve_texture->v });
    }

    s_priv->render_pass = get_render_pass(ctx, params);

    s_priv->output_framebuffer = ngli::D3DFramebuffer::newInstance(
        ctx->device, s_priv->render_pass, attachments, w, h);

    s_priv->parent.width = w;
    s_priv->parent.height = h;

    return 0;
}

void ngli_rendertarget_d3d12_resolve(struct rendertarget* s) {}

void ngli_rendertarget_d3d12_freep(struct rendertarget** sp)
{
    if(!*sp)
        return;
    rendertarget_d3d12* s_priv = (rendertarget_d3d12*)*sp;
    if(s_priv->output_framebuffer)
        delete s_priv->output_framebuffer;
    delete s_priv;
}
