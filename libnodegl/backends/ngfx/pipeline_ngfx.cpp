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

extern "C" {
#include "format.h"
#include "memory.h"
#include "type.h"
}

#include "buffer_ngfx.h"
#include "debugutil_ngfx.h"
#include "gpu_ctx_ngfx.h"
#include "ngfx/compute/ComputePipeline.h"
#include "ngfx/graphics/Buffer.h"
#include "ngfx/graphics/Graphics.h"
#include "ngfx/graphics/GraphicsPipeline.h"
#include "pipeline_ngfx.h"
#include "program_ngfx.h"
#include "texture_ngfx.h"
#include "util_ngfx.h"
#if defined(NGFX_GRAPHICS_BACKEND_VULKAN)
#include "ngfx/porting/vulkan/VKCommandBuffer.h"
#endif

#include <map>
using namespace ngfx;
using VertexInputAttributeDescription =
    GraphicsPipeline::VertexInputAttributeDescription;

struct attribute_binding {
    pipeline_attribute_desc desc;
    const struct buffer *buffer;
};

struct buffer_binding {
    pipeline_buffer_desc desc;
    const struct buffer *buffer;
};

struct texture_binding {
    pipeline_texture_desc desc;
    const struct texture *texture;
};

static int pipeline_set_uniforms(pipeline_ngfx *s)
{
    int nb_buffers = ngli_darray_count(&s->buffer_bindings);
    for (int i = 0; i < nb_buffers; i++) {
        const buffer_binding *binding =
            (buffer_binding *)ngli_darray_get(&s->buffer_bindings, i);
        const struct buffer *buffer = (struct buffer *)binding->buffer;
        int ret = ngli_buffer_upload((struct buffer *)buffer, buffer->data,
                                     buffer->size, 0);
        if (ret < 0)
            return ret;
    }
    return 0;
}

static int init_attributes_data(pipeline *s, const pipeline_params *params)
{
    gpu_ctx_ngfx *gtctx   = (gpu_ctx_ngfx *)s->gpu_ctx;
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;

    ngli_darray_init(&s_priv->vertex_buffers, sizeof(ngfx::Buffer *), 0);

    const pipeline_layout *layout = &params->layout;
    for (int i = 0; i < layout->nb_attributes; i++) {
        ngfx::Buffer *buffer = NULL;
        if (!ngli_darray_push(&s_priv->vertex_buffers, &buffer))
            return NGL_ERROR_MEMORY;
    }
    return 0;
}

pipeline *ngli_pipeline_ngfx_create(struct gpu_ctx *gpu_ctx)
{
    pipeline_ngfx *s = (pipeline_ngfx *)ngli_calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    s->parent.gpu_ctx = gpu_ctx;
    return (pipeline *)s;
}

static std::vector<VertexInputAttributeDescription>
get_vertex_attributes(VertexShaderModule *vs, const pipeline_params *params)
{
    std::vector<VertexInputAttributeDescription> attrs(vs->attributes.size());
    for (int j = 0; j < attrs.size(); j++) {
        auto &attr  = attrs[j];
        attr.v      = &vs->attributes[j];
        attr.offset = params->layout.attributes_desc[j].offset;
    }
    return attrs;
}

static std::set<std::string>
get_instance_attributes(const pipeline_attribute_desc *attrs, int num_attrs)
{
    std::set<std::string> instance_attrs;
    for (int j = 0; j < num_attrs; j++) {
        const auto &attr = attrs[j];
        if (attr.rate)
            instance_attrs.insert(attr.name);
    }
    return instance_attrs;
}

static int init_bindings(pipeline *s, const pipeline_params *params)
{
    gpu_ctx_ngfx *gpu_ctx = (gpu_ctx_ngfx *)s->gpu_ctx;
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;

    ngli_darray_init(&s_priv->attribute_bindings, sizeof(attribute_binding), 0);
    ngli_darray_init(&s_priv->buffer_bindings, sizeof(buffer_binding), 0);
    ngli_darray_init(&s_priv->texture_bindings, sizeof(texture_binding), 0);

    const pipeline_layout *layout = &params->layout;
    for (int i = 0; i < layout->nb_attributes; i++) {
        const pipeline_attribute_desc *desc = &layout->attributes_desc[i];

        attribute_binding binding;
        binding.desc = *desc;

        if (!ngli_darray_push(&s_priv->attribute_bindings, &binding))
            return NGL_ERROR_MEMORY;
    }

    for (int i = 0; i < layout->nb_buffers; i++) {
        const pipeline_buffer_desc *desc = &layout->buffers_desc[i];

        buffer_binding binding;
        binding.desc = *desc;

        if (!ngli_darray_push(&s_priv->buffer_bindings, &binding))
            return NGL_ERROR_MEMORY;
    }

    for (int i = 0; i < layout->nb_textures; i++) {
        const pipeline_texture_desc *desc = &layout->textures_desc[i];

        texture_binding binding;
        binding.desc = *desc;

        if (!ngli_darray_push(&s_priv->texture_bindings, &binding))
            return NGL_ERROR_MEMORY;
    }

    return 0;
}

static int pipeline_graphics_init(pipeline *s, const pipeline_params *params)
{
    gpu_ctx_ngfx *gpu_ctx       = (gpu_ctx_ngfx *)s->gpu_ctx;
    pipeline_ngfx *s_priv       = (struct pipeline_ngfx *)s;
    program_ngfx *program       = (program_ngfx *)s->program;
    pipeline_graphics *graphics = &s->graphics;
    graphicstate *gs            = &graphics->state;

    const rendertarget_desc *rt_desc             = &graphics->rt_desc;
    const attachment_desc *color_attachment_desc = &rt_desc->colors[0];
    const attachment_desc *depth_attachment_desc = &rt_desc->depth_stencil;

    int ret = init_attributes_data(s, params);
    if (ret < 0)
        return ret;

    GraphicsPipeline::State state;
#if defined(NGFX_GRAPHICS_BACKEND_VULKAN)
    state.renderPass = get_compat_render_pass(gpu_ctx->graphics_context,
                                              params->graphics.rt_desc);
#endif
    state.numColorAttachments = params->graphics.rt_desc.nb_colors;

    state.primitiveTopology = to_ngfx_topology(s->graphics.topology);

    state.blendEnable         = gs->blend;
    state.colorBlendOp        = to_ngfx_blend_op(gs->blend_op);
    state.srcColorBlendFactor = to_ngfx_blend_factor(gs->blend_src_factor);
    state.dstColorBlendFactor = to_ngfx_blend_factor(gs->blend_dst_factor);
    state.alphaBlendOp        = to_ngfx_blend_op(gs->blend_op_a);
    state.srcAlphaBlendFactor = to_ngfx_blend_factor(gs->blend_src_factor_a);
    state.dstAlphaBlendFactor = to_ngfx_blend_factor(gs->blend_dst_factor_a);

    state.depthTestEnable         = gs->depth_test;
    state.depthWriteEnable        = gs->depth_write_mask;
    state.depthFunc               = to_ngfx_compare_op(gs->depth_func);
    state.stencilEnable           = gs->stencil_test;
    state.stencilReadMask         = gs->stencil_read_mask;
    state.stencilWriteMask        = gs->stencil_write_mask;
    state.frontStencilFailOp      = to_ngfx_stencil_op(gs->stencil_fail);
    state.frontStencilDepthFailOp = to_ngfx_stencil_op(gs->stencil_depth_fail);
    state.frontStencilPassOp      = to_ngfx_stencil_op(gs->stencil_depth_pass);
    state.frontStencilFunc        = to_ngfx_compare_op(gs->stencil_func);
    state.backStencilFailOp       = to_ngfx_stencil_op(gs->stencil_depth_fail);
    state.backStencilDepthFailOp  = to_ngfx_stencil_op(gs->stencil_depth_fail);
    state.backStencilPassOp       = to_ngfx_stencil_op(gs->stencil_depth_pass);
    state.backStencilFunc         = to_ngfx_compare_op(gs->stencil_func);
    state.stencilRef              = gs->stencil_ref;

    state.colorWriteMask = to_ngfx_color_mask(gs->color_write_mask);

    state.cullModeFlags = to_ngfx_cull_mode(gs->cull_mode);

    state.numSamples = glm::max(rt_desc->samples, 1);

    state.frontFace = FRONT_FACE_COUNTER_CLOCKWISE;

    // Handle attribute stride mismatch
    const pipeline_layout *layout = &params->layout;
    for (int j = 0; j < layout->nb_attributes; j++) {
        auto src_attr_desc = &layout->attributes_desc[j];
        auto dst_attr_desc = program->vs->findAttribute(src_attr_desc->name);
        if (!dst_attr_desc)
            continue; // unused variable
        uint32_t src_attr_stride = src_attr_desc->stride;
        uint32_t dst_attr_stride =
            dst_attr_desc->elementSize * dst_attr_desc->count;
        if (src_attr_stride != dst_attr_stride) {
            dst_attr_desc->elementSize =
                src_attr_desc->stride / dst_attr_desc->count;
        }
    }
    s_priv->gp = GraphicsPipeline::create(
        gpu_ctx->graphics_context, state, program->vs, program->fs,
        to_ngfx_format(color_attachment_desc->format),
        depth_attachment_desc ? to_ngfx_format(depth_attachment_desc->format)
                              : PIXELFORMAT_UNDEFINED,
        get_vertex_attributes(program->vs, params),
        get_instance_attributes(layout->attributes_desc,
                                layout->nb_attributes));

    return 0;
}

static int pipeline_compute_init(pipeline *s, const pipeline_params *params)
{
    gpu_ctx_ngfx *gpu_ctx = (gpu_ctx_ngfx *)s->gpu_ctx;
    pipeline_ngfx *s_priv = (struct pipeline_ngfx *)s;
    program_ngfx *program = (program_ngfx *)params->program;
    s_priv->cp =
        ComputePipeline::create(gpu_ctx->graphics_context, program->cs);
    return 0;
}

int ngli_pipeline_ngfx_init(pipeline *s, const pipeline_params *params)
{
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;

    s->type     = params->type;
    s->graphics = params->graphics;
    s->program  = params->program;

    init_bindings(s, params);

    int ret;
    if (params->type == NGLI_PIPELINE_TYPE_GRAPHICS) {
        ret = pipeline_graphics_init(s, params);
        if (ret < 0)
            return ret;
    } else if (params->type == NGLI_PIPELINE_TYPE_COMPUTE) {
        ret = pipeline_compute_init(s, params);
        if (ret < 0)
            return ret;
    } else {
        ngli_assert(0);
    }

    return 0;
}

static int bind_pipeline(pipeline *s)
{
    pipeline_ngfx *pipeline = (pipeline_ngfx *)s;
    gpu_ctx_ngfx *gpu_ctx   = (gpu_ctx_ngfx *)s->gpu_ctx;
    CommandBuffer *cmd_buf  = gpu_ctx->cur_command_buffer;
    if (pipeline->gp)
        gpu_ctx->graphics->bindGraphicsPipeline(cmd_buf, pipeline->gp);
    else if (pipeline->cp)
        gpu_ctx->graphics->bindComputePipeline(cmd_buf, pipeline->cp);
    return 0;
}

int ngli_pipeline_ngfx_set_resources(pipeline *s,
                                     const pipeline_resources *resources)
{
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;

    ngli_assert(ngli_darray_count(&s_priv->attribute_bindings) ==
                resources->nb_attributes);
    for (int i = 0; i < resources->nb_attributes; i++) {
        int ret =
            ngli_pipeline_ngfx_update_attribute(s, i, resources->attributes[i]);
        if (ret < 0)
            return ret;
    }

    ngli_assert(ngli_darray_count(&s_priv->buffer_bindings) ==
                resources->nb_buffers);
    for (int i = 0; i < resources->nb_buffers; i++) {
        const buffer_binding *buffer_binding_data =
            (const buffer_binding *)ngli_darray_get(&s_priv->buffer_bindings,
                                                    i);
        const pipeline_buffer_desc *buffer_desc = &buffer_binding_data->desc;
        int ret = ngli_pipeline_ngfx_update_buffer(s, i, resources->buffers[i],
                                                   buffer_desc->offset,
                                                   buffer_desc->size);
        if (ret < 0)
            return ret;
    }

    ngli_assert(ngli_darray_count(&s_priv->texture_bindings) ==
                resources->nb_textures);
    for (int i = 0; i < resources->nb_textures; i++) {
        int ret =
            ngli_pipeline_ngfx_update_texture(s, i, resources->textures[i]);
        if (ret < 0)
            return ret;
    }

#if 0 // TODO
    int ret = pipeline_update_blocks(s, resources);
    if (ret < 0)
        return ret;
#endif

    return 0;
}

int ngli_pipeline_ngfx_update_attribute(pipeline *s, int index,
                                        const buffer *p_buffer)
{
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;

    if (index == -1)
        return NGL_ERROR_NOT_FOUND;

    ngli_assert(s->type == NGLI_PIPELINE_TYPE_GRAPHICS);

    attribute_binding *attr_binding = (attribute_binding *)ngli_darray_get(
        &s_priv->attribute_bindings, index);
    ngli_assert(attr_binding);

    const buffer *current_buffer = attr_binding->buffer;
    if (!current_buffer && p_buffer)
        s_priv->nb_unbound_attributes--;
    else if (current_buffer && !p_buffer)
        s_priv->nb_unbound_attributes++;

    attr_binding->buffer = p_buffer;

    ngfx::Buffer **vertex_buffers =
        (ngfx::Buffer **)ngli_darray_data(&s_priv->vertex_buffers);
    if (p_buffer) {
        buffer_ngfx *buffer   = (struct buffer_ngfx *)p_buffer;
        vertex_buffers[index] = buffer->v;
    } else {
        vertex_buffers[index] = NULL;
    }

    return 0;
}

int ngli_pipeline_ngfx_update_uniform(pipeline *s, int index, const void *value)
{
    return NGL_ERROR_GRAPHICS_UNSUPPORTED;
}

int ngli_pipeline_ngfx_update_texture(pipeline *s, int index,
                                      const texture *p_texture)
{
    gpu_ctx_ngfx *gpu_ctx = (gpu_ctx_ngfx *)s->gpu_ctx;
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;

    if (index == -1)
        return NGL_ERROR_NOT_FOUND;

    texture_binding *binding =
        (texture_binding *)ngli_darray_get(&s_priv->texture_bindings, index);
    ngli_assert(binding);
    binding->texture = p_texture;

    return 0;
}

int ngli_pipeline_ngfx_update_buffer(pipeline *s, int index,
                                     const buffer *p_buffer, int offset,
                                     int size)
{
    gpu_ctx_ngfx *gpu_ctx = (struct gpu_ctx_ngfx *)s->gpu_ctx;
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;

    if (index == -1)
        return NGL_ERROR_NOT_FOUND;

    buffer_binding *binding =
        (buffer_binding *)ngli_darray_get(&s_priv->buffer_bindings, index);
    ngli_assert(binding);

    binding->buffer      = p_buffer;
    binding->desc.offset = offset;
    binding->desc.size   = size;

    return 0;
}

static ShaderModule *get_shader_module(program_ngfx *program, int stage)
{
    switch (stage) {
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

static void bind_buffers(CommandBuffer *cmd_buf, pipeline *s)
{
    gpu_ctx_ngfx *gpu_ctx = (gpu_ctx_ngfx *)s->gpu_ctx;
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;
    int nb_buffers        = ngli_darray_count(&s_priv->buffer_bindings);
    program_ngfx *program = (program_ngfx *)s->program;
    for (int j = 0; j < nb_buffers; j++) {
        const buffer_binding *binding =
            (buffer_binding *)ngli_darray_get(&s_priv->buffer_bindings, j);
        const buffer_ngfx *buffer = (buffer_ngfx *)binding->buffer;
        const pipeline_buffer_desc &buffer_desc = binding->desc;
        ShaderModule *shader_module =
            get_shader_module(program, buffer_desc.stage);
        if (buffer_desc.type == NGLI_TYPE_UNIFORM_BUFFER) {
            auto buffer_info =
                shader_module->findUniformBufferInfo(buffer_desc.name);
            if (!buffer_info)
                continue; // unused variable
            gpu_ctx->graphics->bindUniformBuffer(
                cmd_buf, buffer->v, buffer_info->set,
                buffer_info->shaderStages);
        } else {
            auto buffer_info =
                shader_module->findStorageBufferInfo(buffer_desc.name);
            if (!buffer_info)
                continue; // unused variable
            gpu_ctx->graphics->bindStorageBuffer(
                cmd_buf, buffer->v, buffer_info->set,
                buffer_info->shaderStages, buffer_info->readonly);
        }
    }
}

static void bind_textures(CommandBuffer *cmd_buf, pipeline *s)
{
    gpu_ctx_ngfx *gpu_ctx = (gpu_ctx_ngfx *)s->gpu_ctx;
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;
    int nb_textures       = ngli_darray_count(&s_priv->texture_bindings);
    program_ngfx *program = (program_ngfx *)s->program;
    for (int j = 0; j < nb_textures; j++) {
        const texture_binding *binding =
            (texture_binding *)ngli_darray_get(&s_priv->texture_bindings, j);
        const pipeline_texture_desc &texture_desc = binding->desc;
        ShaderModule *shader_module =
            get_shader_module(program, texture_desc.stage);
        auto texture_info =
            shader_module->findDescriptorInfo(texture_desc.name);
        if (!texture_info)
            continue;
        const texture_ngfx *texture = (texture_ngfx *)binding->texture;
        if (!texture)
            texture = (texture_ngfx *)gpu_ctx->dummy_texture;
        if (texture) {
            gpu_ctx->graphics->bindTexture(
                cmd_buf, texture->v, texture_info->set);
        }
    }
}

static void bind_vertex_buffers(CommandBuffer *cmd_buf, pipeline *s)
{
    gpu_ctx_ngfx *gpu_ctx = (gpu_ctx_ngfx *)s->gpu_ctx;
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;
    int nb_attributes     = ngli_darray_count(&s_priv->attribute_bindings);
    program_ngfx *program = (program_ngfx *)s->program;
    for (int j = 0; j < nb_attributes; j++) {
        const attribute_binding *attr_binding =
            (const attribute_binding *)ngli_darray_get(
                &s_priv->attribute_bindings, j);
        const pipeline_attribute_desc *attr_desc = &attr_binding->desc;
        auto dst_attr_desc = program->vs->findAttribute(attr_desc->name);
        if (!dst_attr_desc)
            continue; // unused variable
        const buffer_ngfx *buffer = (const buffer_ngfx *)attr_binding->buffer;
        uint32_t dst_attr_stride =
            dst_attr_desc->elementSize * dst_attr_desc->count;
        gpu_ctx->graphics->bindVertexBuffer(
            cmd_buf, buffer->v, dst_attr_desc->location, dst_attr_stride);
    }
}

static void set_viewport(CommandBuffer *cmd_buf, gpu_ctx_ngfx *gpu_ctx)
{
    int *vp = gpu_ctx->viewport;
    gpu_ctx->graphics->setViewport(
        gpu_ctx->cur_command_buffer,
        {vp[0], vp[1], uint32_t(vp[2]), uint32_t(vp[3])});
}

static void set_scissor(CommandBuffer *cmd_buf, gpu_ctx_ngfx *gpu_ctx)
{
    int *sr                       = gpu_ctx->scissor;
    const struct rendertarget *rt = gpu_ctx->cur_rendertarget;
    if (!rt)
        return;
    gpu_ctx->graphics->setScissor(
        gpu_ctx->cur_command_buffer,
#ifdef NGFX_GRAPHICS_BACKEND_DIRECT3D12
        {sr[0], NGLI_MAX(rt->height - sr[1] - sr[3], 0), uint32_t(sr[2]),
         uint32_t(sr[3])}
#else
        {sr[0], sr[1], uint32_t(sr[2]), uint32_t(sr[3])}
#endif
    );
}

void ngli_pipeline_ngfx_draw(pipeline *s, int nb_vertices, int nb_instances)
{
    gpu_ctx_ngfx *gpu_ctx  = (gpu_ctx_ngfx *)s->gpu_ctx;
    CommandBuffer *cmd_buf = gpu_ctx->cur_command_buffer;

    pipeline_set_uniforms((pipeline_ngfx *)s);

    bind_pipeline(s);
    set_viewport(cmd_buf, gpu_ctx);
    set_scissor(cmd_buf, gpu_ctx);

    bind_vertex_buffers(cmd_buf, s);
    bind_buffers(cmd_buf, s);
    bind_textures(cmd_buf, s);

    gpu_ctx->graphics->draw(cmd_buf, nb_vertices, nb_instances);
}

void ngli_pipeline_ngfx_draw_indexed(pipeline *s, const buffer *indices,
                                     int indices_format, int nb_indices,
                                     int nb_instances)
{
    gpu_ctx_ngfx *gpu_ctx  = (gpu_ctx_ngfx *)s->gpu_ctx;
    CommandBuffer *cmd_buf = gpu_ctx->cur_command_buffer;

    pipeline_set_uniforms((pipeline_ngfx *)s);

    bind_pipeline(s);
    set_viewport(cmd_buf, gpu_ctx);
    set_scissor(cmd_buf, gpu_ctx);

    bind_vertex_buffers(cmd_buf, s);
    bind_buffers(cmd_buf, s);
    bind_textures(cmd_buf, s);

    gpu_ctx->graphics->bindIndexBuffer(cmd_buf, ((buffer_ngfx *)indices)->v,
                                       to_ngfx_index_format(indices_format));

    gpu_ctx->graphics->drawIndexed(cmd_buf, nb_indices, nb_instances);
}

void ngli_pipeline_ngfx_dispatch(pipeline *s, int nb_group_x, int nb_group_y,
                                 int nb_group_z)
{
    gpu_ctx_ngfx *gpu_ctx  = (gpu_ctx_ngfx *)s->gpu_ctx;
    CommandBuffer *cmd_buf = gpu_ctx->cur_command_buffer;

    pipeline_set_uniforms((pipeline_ngfx *)s);

    bind_pipeline(s);
    bind_vertex_buffers(cmd_buf, s);
    bind_buffers(cmd_buf, s);
    bind_textures(cmd_buf, s);

    TODO("pass threads_per_group as params");
    int threads_per_group_x = 1, threads_per_group_y = 1,
        threads_per_group_z = 1;
    gpu_ctx->graphics->dispatch(cmd_buf, nb_group_x, nb_group_y, nb_group_z,
                                threads_per_group_x, threads_per_group_y,
                                threads_per_group_z);

#if defined(NGFX_GRAPHICS_BACKEND_VULKAN)
    // TODO: add pipeline barrier API to NGLI and ngfx
    const VkMemoryBarrier barrier = {
        .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
    };
    const VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    const VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    vkCmdPipelineBarrier(vk(cmd_buf)->v, src_stage, dst_stage, 0, 1, &barrier,
                         0, NULL, 0, NULL);
#endif
}

void ngli_pipeline_ngfx_freep(pipeline **sp)
{
    if (!*sp)
        return;

    pipeline *s           = *sp;
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;

    if (s_priv->gp)
        delete s_priv->gp;
    if (s_priv->cp)
        delete s_priv->cp;

    ngli_darray_reset(&s_priv->texture_bindings);
    ngli_darray_reset(&s_priv->buffer_bindings);
    ngli_darray_reset(&s_priv->attribute_bindings);

    ngli_darray_reset(&s_priv->vertex_buffers);

    ngli_freep(sp);
}
