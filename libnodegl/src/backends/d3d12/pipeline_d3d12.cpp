/*
 * Copyright 2019 GoPro Inc.
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


extern "C" {
#include "format.h"
#include "memory.h"
#include "type.h"
#include "buffer.h"
}



#include "buffer_d3d12.h"
#include "gpu_ctx_d3d12.h"
#include "pipeline_d3d12.h"
#include "program_d3d12.h"
#include "texture_d3d12.h"
#include "topology_d3d12.h"
#include "util_d3d12.h"

#include <backends/d3d12/impl/D3DBuffer.h>
#include <backends/d3d12/impl/D3DComputePipeline.h>
#include <backends/d3d12/impl/D3DGraphics.h>
#include <backends/d3d12/impl/D3DGraphicsPipeline.h>
#include <glm/gtc/type_ptr.hpp>

#include <map>

using VertexInputAttributeDescription =
ngli::D3DGraphicsPipeline::VertexInputAttributeDescription;

struct attribute_binding
{
    pipeline_attribute_desc desc;
    const struct buffer* buffer;
};

struct buffer_binding
{
    pipeline_buffer_desc desc;
    const struct buffer* buffer;
};

struct texture_binding
{
    pipeline_texture_desc desc;
    const struct texture* texture;
};

static int pipeline_set_uniforms(pipeline_d3d12* s)
{
    int nb_buffers = ngli_darray_count(&s->buffer_bindings);
    for(int i = 0; i < nb_buffers; i++)
    {
        const buffer_binding* binding =
            (buffer_binding*)ngli_darray_get(&s->buffer_bindings, i);
        const struct buffer* buffer = (struct buffer*)binding->buffer;
        int ret = ngli_buffer_upload((struct buffer*)buffer, buffer->data,
                                     buffer->size, 0);
        if(ret < 0)
            return ret;
    }
    return 0;
}

static int init_attributes_data(pipeline* s, const pipeline_params* params)
{
    //gpu_ctx_d3d12* gtctx = (gpu_ctx_d3d12*)s->gpu_ctx;
    pipeline_d3d12* s_priv = (pipeline_d3d12*)s;

    ngli_darray_init(&s_priv->vertex_buffers, sizeof(ngli::D3DBuffer*), 0);

    const pipeline_layout* layout = &params->layout;
    for(int i = 0; i < layout->nb_attributes; i++)
    {
        ngli::D3DBuffer* buffer = NULL;
        if(!ngli_darray_push(&s_priv->vertex_buffers, &buffer))
            return NGL_ERROR_MEMORY;
    }
    return 0;
}

pipeline* ngli_pipeline_d3d12_create(struct gpu_ctx* gpu_ctx)
{
    pipeline_d3d12* s = (pipeline_d3d12*)ngli_calloc(1, sizeof(*s));
    if(!s)
        return NULL;
    s->parent.gpu_ctx = gpu_ctx;
    return (pipeline*)s;
}

static std::vector<VertexInputAttributeDescription>
get_vertex_attributes(ngli::D3DVertexShaderModule* vs, const pipeline_params* params)
{
    std::vector<VertexInputAttributeDescription> attrs(vs->attributes.size());
    for(int j = 0; j < attrs.size(); j++)
    {
        auto& attr = attrs[j];
        attr.v = &vs->attributes[j];
        attr.offset = params->layout.attributes_desc[j].offset;
    }
    return attrs;
}

static std::set<std::string>
get_instance_attributes(const pipeline_attribute_desc* attrs, int num_attrs)
{
    std::set<std::string> instance_attrs;
    for(int j = 0; j < num_attrs; j++)
    {
        const auto& attr = attrs[j];
        if(attr.rate)
            instance_attrs.insert(attr.name);
    }
    return instance_attrs;
}

static int init_bindings(pipeline* s, const pipeline_params* params)
{
    //gpu_ctx_d3d12* gpu_ctx = (gpu_ctx_d3d12*)s->gpu_ctx;
    pipeline_d3d12* s_priv = (pipeline_d3d12*)s;

    ngli_darray_init(&s_priv->attribute_bindings, sizeof(attribute_binding), 0);
    ngli_darray_init(&s_priv->buffer_bindings, sizeof(buffer_binding), 0);
    ngli_darray_init(&s_priv->texture_bindings, sizeof(texture_binding), 0);

    const pipeline_layout* layout = &params->layout;
    for(int i = 0; i < layout->nb_attributes; i++)
    {
        const pipeline_attribute_desc* desc = &layout->attributes_desc[i];

        attribute_binding binding;
        binding.desc = *desc;

        if(!ngli_darray_push(&s_priv->attribute_bindings, &binding))
            return NGL_ERROR_MEMORY;
    }

    for(int i = 0; i < layout->nb_buffers; i++)
    {
        const pipeline_buffer_desc* desc = &layout->buffers_desc[i];

        buffer_binding binding;
        binding.desc = *desc;

        if(!ngli_darray_push(&s_priv->buffer_bindings, &binding))
            return NGL_ERROR_MEMORY;
    }

    for(int i = 0; i < layout->nb_textures; i++)
    {
        const pipeline_texture_desc* desc = &layout->textures_desc[i];

        texture_binding binding;
        binding.desc = *desc;

        if(!ngli_darray_push(&s_priv->texture_bindings, &binding))
            return NGL_ERROR_MEMORY;
    }

    return 0;
}

static int pipeline_graphics_init(pipeline* s, const pipeline_params* params)
{
    gpu_ctx_d3d12* gpu_ctx = (gpu_ctx_d3d12*)s->gpu_ctx;
    pipeline_d3d12* s_priv = (struct pipeline_d3d12*)s;
    program_d3d12* program = (program_d3d12*)s->program;
    pipeline_graphics* graphics = &s->graphics;
    graphicstate* gs = &graphics->state;

    const rendertarget_desc* rt_desc = &graphics->rt_desc;
    const attachment_desc* color_attachment_desc = &rt_desc->colors[0];
    const attachment_desc* depth_attachment_desc = &rt_desc->depth_stencil;

    int ret = init_attributes_data(s, params);
    if(ret < 0)
        return ret;

    ngli::D3DGraphicsPipeline::State state;
#if defined(NGLI_GRAPHICS_BACKEND_VULKAN)
    state.renderPass = get_compat_render_pass(gpu_ctx->graphics_context,
                                              params->graphics.rt_desc);
#endif
    state.numColorAttachments = params->graphics.rt_desc.nb_colors;

    state.primitiveTopology = to_d3d12_topology(s->graphics.topology);

    state.blendEnable = gs->blend;

    state.blendColorOp = to_d3d12_blend_op(gs->blend_op);
    state.blendSrcColorFactor = to_d3d12_blend_factor(gs->blend_src_factor);
    state.blendDstColorFactor = to_d3d12_blend_factor(gs->blend_dst_factor);
    state.blendAlphaOp = to_d3d12_blend_op(gs->blend_op_a);
    state.blendSrcAlphaFactor = to_d3d12_blend_factor(gs->blend_src_factor_a);
    state.blendDstAlphaFactor = to_d3d12_blend_factor(gs->blend_dst_factor_a);

    state.depthTestEnable = gs->depth_test;
    state.depthWriteEnable = gs->depth_write_mask;
    state.depthFunc = to_d3d12_compare_op(gs->depth_func);

    state.stencilEnable = gs->stencil_test;
    state.stencilReadMask = gs->stencil_read_mask;
    state.stencilWriteMask = gs->stencil_write_mask;
    state.frontStencilFailOp = to_d3d12_stencil_op(gs->stencil_fail);
    state.frontStencilDepthFailOp = to_d3d12_stencil_op(gs->stencil_depth_fail);
    state.frontStencilPassOp = to_d3d12_stencil_op(gs->stencil_depth_pass);
    state.frontStencilFunc = to_d3d12_compare_op(gs->stencil_func);
    state.backStencilFailOp = to_d3d12_stencil_op(gs->stencil_depth_fail);
    state.backStencilDepthFailOp = to_d3d12_stencil_op(gs->stencil_depth_fail);
    state.backStencilPassOp = to_d3d12_stencil_op(gs->stencil_depth_pass);
    state.backStencilFunc = to_d3d12_compare_op(gs->stencil_func);
    state.stencilRef = gs->stencil_ref;

    state.colorWriteMask = to_d3d12_color_mask(gs->color_write_mask);

    state.cullMode = to_d3d12_cull_mode(gs->cull_mode);

    state.numSamples = glm::max(rt_desc->samples, 1);

    state.frontFace = ngli::FRONT_FACE_COUNTER_CLOCKWISE;

    // Handle attribute stride mismatch
    const pipeline_layout* layout = &params->layout;
    for(int j = 0; j < layout->nb_attributes; j++)
    {
        auto src_attr_desc = &layout->attributes_desc[j];
        auto dst_attr_desc = program->vs->findAttribute(src_attr_desc->name);
        if(!dst_attr_desc)
            continue; // unused variable
        uint32_t src_attr_stride = src_attr_desc->stride;
        uint32_t dst_attr_stride =
            dst_attr_desc->elementSize * dst_attr_desc->count;
        if(src_attr_stride != dst_attr_stride)
        {
            dst_attr_desc->elementSize =
                src_attr_desc->stride / dst_attr_desc->count;
        }
    }
    s_priv->gp = ngli::D3DGraphicsPipeline::newInstance(
        gpu_ctx->graphics_context, state, program->vs, program->fs,
        to_d3d12_format(color_attachment_desc->format),
        depth_attachment_desc ? to_d3d12_format(depth_attachment_desc->format)
                              : DXGI_FORMAT_UNKNOWN,
        get_vertex_attributes(program->vs, params),
        get_instance_attributes(layout->attributes_desc,
                                layout->nb_attributes));

    return 0;
}

static int pipeline_compute_init(pipeline* s, const pipeline_params* params)
{
    gpu_ctx_d3d12* gpu_ctx = (gpu_ctx_d3d12*)s->gpu_ctx;
    pipeline_d3d12* s_priv = (struct pipeline_d3d12*)s;
    program_d3d12* program = (program_d3d12*)params->program;
    s_priv->cp =
        ngli::D3DComputePipeline::newInstance(gpu_ctx->graphics_context, program->cs);
    return 0;
}

int ngli_pipeline_d3d12_init(pipeline* s, const pipeline_params* params)
{
    pipeline_d3d12* s_priv = (pipeline_d3d12*)s;

    s->type = params->type;
    s->graphics = params->graphics;
    s->program = params->program;

    init_bindings(s, params);

    int ret;
    if(params->type == NGLI_PIPELINE_TYPE_GRAPHICS)
    {
        ret = pipeline_graphics_init(s, params);
        if(ret < 0)
            return ret;
    }
    else if(params->type == NGLI_PIPELINE_TYPE_COMPUTE)
    {
        ret = pipeline_compute_init(s, params);
        if(ret < 0)
            return ret;
    }
    else
    {
        ngli_assert(0);
    }

    return 0;
}

static int bind_pipeline(pipeline* s)
{
    pipeline_d3d12* pipeline = (pipeline_d3d12*)s;
    gpu_ctx_d3d12* gpu_ctx = (gpu_ctx_d3d12*)s->gpu_ctx;
    ngli::D3DCommandList* cmd_buf = gpu_ctx->cur_command_buffer;
    if(pipeline->gp)
        gpu_ctx->graphics->bindGraphicsPipeline(cmd_buf, pipeline->gp);
    else if(pipeline->cp)
        gpu_ctx->graphics->bindComputePipeline(cmd_buf, pipeline->cp);
    return 0;
}

int ngli_pipeline_d3d12_set_resources(pipeline* s,
                                     const pipeline_resources* resources)
{
    pipeline_d3d12* s_priv = (pipeline_d3d12*)s;

    ngli_assert(ngli_darray_count(&s_priv->attribute_bindings) ==
                resources->nb_attributes);
    for(int i = 0; i < resources->nb_attributes; i++)
    {
        int ret =
            ngli_pipeline_d3d12_update_attribute(s, i, resources->attributes[i]);
        if(ret < 0)
            return ret;
    }

    ngli_assert(ngli_darray_count(&s_priv->buffer_bindings) ==
                resources->nb_buffers);
    for(int i = 0; i < resources->nb_buffers; i++)
    {
        const buffer_binding* buffer_binding_data =
            (const buffer_binding*)ngli_darray_get(&s_priv->buffer_bindings,
                                                    i);
        const pipeline_buffer_desc* buffer_desc = &buffer_binding_data->desc;
        int ret = ngli_pipeline_d3d12_update_buffer(s, i, resources->buffers[i],
                                                   buffer_desc->offset,
                                                   buffer_desc->size);
        if(ret < 0)
            return ret;
    }

    ngli_assert(ngli_darray_count(&s_priv->texture_bindings) ==
                resources->nb_textures);
    for(int i = 0; i < resources->nb_textures; i++)
    {
        int ret =
            ngli_pipeline_d3d12_update_texture(s, i, resources->textures[i]);
        if(ret < 0)
            return ret;
    }

#if 0 // TODO
    int ret = pipeline_update_blocks(s, resources);
    if(ret < 0)
        return ret;
#endif

    return 0;
}

int ngli_pipeline_d3d12_update_attribute(pipeline* s, int index,
                                        const buffer* p_buffer)
{
    pipeline_d3d12* s_priv = (pipeline_d3d12*)s;

    if(index == -1)
        return NGL_ERROR_NOT_FOUND;

    ngli_assert(s->type == NGLI_PIPELINE_TYPE_GRAPHICS);

    attribute_binding* attr_binding = (attribute_binding*)ngli_darray_get(
        &s_priv->attribute_bindings, index);
    ngli_assert(attr_binding);

    const buffer* current_buffer = attr_binding->buffer;
    if(!current_buffer && p_buffer)
        s_priv->nb_unbound_attributes--;
    else if(current_buffer && !p_buffer)
        s_priv->nb_unbound_attributes++;

    attr_binding->buffer = p_buffer;

    ngli::D3DBuffer** vertex_buffers =
        (ngli::D3DBuffer**)ngli_darray_data(&s_priv->vertex_buffers);
    if(p_buffer)
    {
        buffer_d3d12* buffer = (struct buffer_d3d12*)p_buffer;
        vertex_buffers[index] = buffer->mBuffer;
    }
    else
    {
        vertex_buffers[index] = NULL;
    }

    return 0;
}

int ngli_pipeline_d3d12_update_uniform(pipeline* s, int index, const void* value)
{
    return NGL_ERROR_GRAPHICS_UNSUPPORTED;
}

int ngli_pipeline_d3d12_update_texture(pipeline* s, int index,
                                      const texture* p_texture)
{
    gpu_ctx_d3d12* gpu_ctx = (gpu_ctx_d3d12*)s->gpu_ctx;
    pipeline_d3d12* s_priv = (pipeline_d3d12*)s;

    if(index == -1)
        return NGL_ERROR_NOT_FOUND;

    texture_binding* binding =
        (texture_binding*)ngli_darray_get(&s_priv->texture_bindings, index);
    ngli_assert(binding);
    binding->texture = p_texture;

    return 0;
}

int ngli_pipeline_d3d12_update_buffer(pipeline* s, int index,
                                     const buffer* p_buffer, int offset,
                                     int size)
{
    gpu_ctx_d3d12* gpu_ctx = (struct gpu_ctx_d3d12*)s->gpu_ctx;
    pipeline_d3d12* s_priv = (pipeline_d3d12*)s;

    if(index == -1)
        return NGL_ERROR_NOT_FOUND;

    buffer_binding* binding =
        (buffer_binding*)ngli_darray_get(&s_priv->buffer_bindings, index);
    ngli_assert(binding);

    binding->buffer = p_buffer;
    binding->desc.offset = offset;
    binding->desc.size = size;

    return 0;
}

static ngli::D3DShaderModule* get_shader_module(program_d3d12* program, int stage)
{
    switch(stage)
    {
        case 0:
            return program->vs;
            break;
        case 1:
            return program->fs;
            break;
        case 2:
            return program->cs;
            break;
    }
    return nullptr;
}

static int get_binding(pipeline_d3d12* s_priv, int set)
{
#if defined(NGLI_GRAPHICS_BACKEND_METAL)
    return set;
#else
    // Binding graphic or compute shader
    return s_priv->gp ? s_priv->gp->descriptorBindings[set]
        : s_priv->cp->descriptorBindings[set];
#endif
}

static void bind_buffers(ngli::D3DCommandList* cmd_buf, pipeline* s)
{
    gpu_ctx_d3d12* gpu_ctx = (gpu_ctx_d3d12*)s->gpu_ctx;
    pipeline_d3d12* s_priv = (pipeline_d3d12*)s;
    int nb_buffers = ngli_darray_count(&s_priv->buffer_bindings);
    program_d3d12* program = (program_d3d12*)s->program;
    for(int j = 0; j < nb_buffers; j++)
    {
        const buffer_binding* binding =
            (buffer_binding*)ngli_darray_get(&s_priv->buffer_bindings, j);
        const buffer_d3d12* buffer = (buffer_d3d12*)binding->buffer;
        const pipeline_buffer_desc& buffer_desc = binding->desc;
        ngli::D3DShaderModule* shader_module =
            get_shader_module(program, buffer_desc.stage);
        if(buffer_desc.type == NGLI_TYPE_UNIFORM_BUFFER)
        {
            auto buffer_info =
                shader_module->findUniformBufferInfo(buffer_desc.name);
            if(!buffer_info)
                continue; // unused variable
            gpu_ctx->graphics->bindUniformBuffer(
                cmd_buf, buffer->mBuffer, get_binding(s_priv, buffer_info->set),
                buffer_info->shaderStages);
        }
        else
        {
            auto buffer_info =
                shader_module->findStorageBufferInfo(buffer_desc.name);
            if(!buffer_info)
                continue; // unused variable
            gpu_ctx->graphics->bindStorageBuffer(
                cmd_buf, buffer->mBuffer, get_binding(s_priv, buffer_info->set),
                buffer_info->shaderStages, buffer_info->readonly);
        }
    }
}

static void bind_textures(ngli::D3DCommandList* cmd_buf, pipeline* s)
{
    gpu_ctx_d3d12* gpu_ctx = (gpu_ctx_d3d12*)s->gpu_ctx;
    pipeline_d3d12* s_priv = (pipeline_d3d12*)s;
    int nb_textures = ngli_darray_count(&s_priv->texture_bindings);
    program_d3d12* program = (program_d3d12*)s->program;
    for(int j = 0; j < nb_textures; j++)
    {
        const texture_binding* binding =
            (texture_binding*)ngli_darray_get(&s_priv->texture_bindings, j);
        const pipeline_texture_desc& texture_desc = binding->desc;
        ngli::D3DShaderModule* shader_module =
            get_shader_module(program, texture_desc.stage);
        auto texture_info =
            shader_module->findDescriptorInfo(texture_desc.name);
        if(!texture_info)
            continue;
        const texture_d3d12* texture = (texture_d3d12*)binding->texture;
        if(!texture)
            texture = (texture_d3d12*)gpu_ctx->dummy_texture;
        if(texture)
        {
            gpu_ctx->graphics->bindTexture(
                cmd_buf, texture->v, get_binding(s_priv, texture_info->set));
        }
    }
}

static void bind_vertex_buffers(ngli::D3DCommandList* cmd_buf, pipeline* s)
{
    gpu_ctx_d3d12* gpu_ctx = (gpu_ctx_d3d12*)s->gpu_ctx;
    pipeline_d3d12* s_priv = (pipeline_d3d12*)s;
    int nb_attributes = ngli_darray_count(&s_priv->attribute_bindings);
    program_d3d12* program = (program_d3d12*)s->program;
    for(int j = 0; j < nb_attributes; j++)
    {
        const attribute_binding* attr_binding =
            (const attribute_binding*)ngli_darray_get(
                &s_priv->attribute_bindings, j);
        const pipeline_attribute_desc* attr_desc = &attr_binding->desc;
        auto dst_attr_desc = program->vs->findAttribute(attr_desc->name);
        if(!dst_attr_desc)
            continue; // unused variable
        const buffer_d3d12* buffer = (const buffer_d3d12*)attr_binding->buffer;
        uint32_t dst_attr_stride =
            dst_attr_desc->elementSize * dst_attr_desc->count;
        gpu_ctx->graphics->bindVertexBuffer(
            cmd_buf, buffer->mBuffer, dst_attr_desc->location, dst_attr_stride);
    }
}

static void set_viewport(ngli::D3DCommandList* cmd_buf, gpu_ctx_d3d12* gpu_ctx)
{
    int* vp = gpu_ctx->viewport;
    gpu_ctx->graphics->setViewport(
        gpu_ctx->cur_command_buffer,
        { vp[0], vp[1], vp[2], vp[3] });
}

static void set_scissor(ngli::D3DCommandList* cmd_buf, gpu_ctx_d3d12* gpu_ctx)
{
    int* sr = gpu_ctx->scissor;
    const struct rendertarget* rt = gpu_ctx->cur_rendertarget;
    if(!rt)
        return;
    gpu_ctx->graphics->setScissor(
        gpu_ctx->cur_command_buffer,
#ifdef NGLI_GRAPHICS_BACKEND_DIRECT3D12
        { sr[0], NGLI_MAX(rt->height - sr[1] - sr[3], 0), uint32_t(sr[2]),
         uint32_t(sr[3]) }
#else
    { sr[0], sr[1], sr[2], sr[3] }
#endif
    );
}

void ngli_pipeline_d3d12_draw(pipeline* s, int nb_vertices, int nb_instances)
{
    gpu_ctx_d3d12* gpu_ctx = (gpu_ctx_d3d12*)s->gpu_ctx;
    ngli::D3DCommandList* cmd_buf = gpu_ctx->cur_command_buffer;

    pipeline_set_uniforms((pipeline_d3d12*)s);

    bind_pipeline(s);
    set_viewport(cmd_buf, gpu_ctx);
    set_scissor(cmd_buf, gpu_ctx);

    bind_vertex_buffers(cmd_buf, s);
    bind_buffers(cmd_buf, s);
    bind_textures(cmd_buf, s);

    gpu_ctx->graphics->draw(cmd_buf, nb_vertices, nb_instances);
}

void ngli_pipeline_d3d12_draw_indexed(pipeline* s, const buffer* indices,
                                     int indices_format, int nb_indices,
                                     int nb_instances)
{
    gpu_ctx_d3d12* gpu_ctx = (gpu_ctx_d3d12*)s->gpu_ctx;
    ngli::D3DCommandList* cmd_buf = gpu_ctx->cur_command_buffer;

    pipeline_set_uniforms((pipeline_d3d12*)s);

    bind_pipeline(s);
    set_viewport(cmd_buf, gpu_ctx);
    set_scissor(cmd_buf, gpu_ctx);

    bind_vertex_buffers(cmd_buf, s);
    bind_buffers(cmd_buf, s);
    bind_textures(cmd_buf, s);

    gpu_ctx->graphics->bindIndexBuffer(cmd_buf, ((buffer_d3d12*)indices)->mBuffer,
                                       to_d3d12_index_format(indices_format));

    gpu_ctx->graphics->drawIndexed(cmd_buf, nb_indices, nb_instances);
}

void ngli_pipeline_d3d12_dispatch(pipeline* s, int nb_group_x, int nb_group_y,
                                 int nb_group_z, int threads_per_group_x,
                                 int threads_per_group_y, int threads_per_group_z)
{
    gpu_ctx_d3d12* gpu_ctx = (gpu_ctx_d3d12*)s->gpu_ctx;
    ngli::D3DCommandList* cmd_buf = gpu_ctx->cur_command_buffer;

    pipeline_set_uniforms((pipeline_d3d12*)s);

    gpu_ctx->graphics->beginComputePass(cmd_buf);

    bind_pipeline(s);
    bind_vertex_buffers(cmd_buf, s);
    bind_buffers(cmd_buf, s);
    bind_textures(cmd_buf, s);

    gpu_ctx->graphics->dispatch(cmd_buf, nb_group_x, nb_group_y, nb_group_z,
                                threads_per_group_x, threads_per_group_y,
                                threads_per_group_z);

#if defined(NGLI_GRAPHICS_BACKEND_VULKAN)
    // TODO: add pipeline barrier API to NGLI and ngfx
    const VkMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
    };
    const VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    const VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    vkCmdPipelineBarrier(vk(cmd_buf)->v, src_stage, dst_stage, 0, 1, &barrier,
                         0, NULL, 0, NULL);
#endif

    gpu_ctx->graphics->endComputePass(cmd_buf);
}

void ngli_pipeline_d3d12_freep(pipeline** sp)
{
    if(!*sp)
        return;

    pipeline* s = *sp;
    pipeline_d3d12* s_priv = (pipeline_d3d12*)s;

    if(s_priv->gp)
        delete s_priv->gp;
    if(s_priv->cp)
        delete s_priv->cp;

    ngli_darray_reset(&s_priv->texture_bindings);
    ngli_darray_reset(&s_priv->buffer_bindings);
    ngli_darray_reset(&s_priv->attribute_bindings);

    ngli_darray_reset(&s_priv->vertex_buffers);

    ngli_freep(sp);
}
