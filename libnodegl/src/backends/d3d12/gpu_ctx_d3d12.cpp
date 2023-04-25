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

#include "gpu_ctx_d3d12.h"
extern "C" {
#include "format.h"
#include "math_utils.h"
#include "memory.h"
#include "surface_util_d3d12.h"
}
#include "buffer_d3d12.h"
#include "pipeline_d3d12.h"
#include "program_d3d12.h"
#include "rendertarget_d3d12.h"
#include "swapchain_util_d3d12.h"
#include "texture_d3d12.h"
#include "util_d3d12.h"

#include <backends/d3d12/impl/D3DGraphics.h>


#if DEBUG_GPU_CAPTURE
extern "C" {
#include "gpu_capture.h"
}
#endif

static int create_dummy_texture(struct gpu_ctx *s)
{
    gpu_ctx_d3d12 *s_priv                = (gpu_ctx_d3d12 *)s;
    auto &dummy_texture                 = s_priv->dummy_texture;
    dummy_texture                       = ngli_texture_create(s);
    texture_params dummy_texture_params = {
        .type    = NGLI_TEXTURE_TYPE_2D,
        .format  = NGLI_FORMAT_R8G8B8A8_UNORM,
        .width   = 1,
        .height  = 1,
        .samples = 1,
        .usage   = NGLI_TEXTURE_USAGE_SAMPLED_BIT |
                 NGLI_TEXTURE_USAGE_TRANSFER_SRC_BIT};

    ngli_texture_init(dummy_texture, &dummy_texture_params);
    return 0;
}

static void destroy_dummy_texture(struct gpu_ctx *s)
{
    struct gpu_ctx_d3d12 *s_priv = (gpu_ctx_d3d12 *)s;

    ngli_texture_freep(&s_priv->dummy_texture);
}

static int create_texture(gpu_ctx *s, int format, int width, int height,
                          int samples, int usage, texture **texturep)
{
    struct texture *texture = ngli_texture_create(s);
    if (!texture)
        return NGL_ERROR_MEMORY;

    const texture_params params = {
        .type    = NGLI_TEXTURE_TYPE_2D,
        .format  = format,
        .width   = width,
        .height  = height,
        .samples = samples,
        .usage   = usage,
    };

    int res = ngli_texture_init(texture, &params);
    if (res < 0) {
        ngli_texture_freep(&texture);
        return res;
    }

    *texturep = texture;

    return 0;
}

static struct gpu_ctx *d3d12_create(const struct ngl_config *config)
{
    LOG(INFO, "d3d12_create");
    gpu_ctx_d3d12 *s = (gpu_ctx_d3d12 *)ngli_calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    return (struct gpu_ctx *)s;
}

static int create_rendertarget(struct gpu_ctx *s, texture *color,
                               texture *resolve_color, texture *depth_stencil,
                               int color_load_op, int color_store_op,
                               int depth_stencil_load_op,
                               int depth_stencil_store_op,
                               rendertarget **rendertargetp)
{
    const ngl_config *config = &s->config;

    rendertarget *rt = ngli_rendertarget_create(s);
    if (!rt)
        return NGL_ERROR_MEMORY;

    rendertarget_params params = {};
    params.width               = config->width;
    params.height              = config->height;
    params.nb_colors           = 1;

    // Colors 0
    auto &p0                   = params.colors[0];
    p0.attachment              = color;
    p0.resolve_target          = resolve_color;
    p0.load_op                 = color_load_op;
    p0.clear_value[0]          = config->clear_color[0];
    p0.clear_value[1]          = config->clear_color[1];
    p0.clear_value[2]          = config->clear_color[2];
    p0.clear_value[3]          = config->clear_color[3];
    p0.store_op   = color_store_op;

    // depth stencil
    auto &p1      = params.depth_stencil;
    p1.attachment = depth_stencil;
    p1.load_op    = color_load_op;
    p1.store_op   = depth_stencil_store_op;

    int ret;

  /*  if(color)
    {*/
        ret = ngli_rendertarget_init(rt, &params);
   /* }
    else
    {
        const GLuint default_fbo_id = ngli_glcontext_get_default_framebuffer(gl);
        const GLuint fbo_id = default_fbo_id;
        ret = ngli_rendertarget_gl_wrap(rt, &params, fbo_id);
    }*/

    if(ret < 0)
    {
        ngli_rendertarget_freep(&rt);
        return ret;
    }
    *rendertargetp = rt;

    return 0;
}

static int create_onscreen_resources(struct gpu_ctx *s)
{
    gpu_ctx_d3d12 *s_priv = (gpu_ctx_d3d12 *)s;
    // use d3d12 default renderpass
    s_priv->default_rendertarget = nullptr;
    return 0;
}

static int create_offscreen_resources(struct gpu_ctx *s)
{
    gpu_ctx_d3d12 *s_priv     = (gpu_ctx_d3d12 *)s;
    const ngl_config *config = &s->config;
    gpu_ctx_d3d12::offscreen& offscreen_resources = s_priv->offscreen_resources;

    int usage = NGLI_TEXTURE_USAGE_COLOR_ATTACHMENT_BIT |
                NGLI_TEXTURE_USAGE_TRANSFER_SRC_BIT;
    if (config->samples > 0)
        usage |= NGLI_TEXTURE_USAGE_SAMPLED_BIT;

    auto &color_texture = offscreen_resources.color_texture;

    int res = create_texture(s, NGLI_FORMAT_R8G8B8A8_UNORM, config->width,
                       config->height, config->samples, usage, &color_texture);
    if (res < 0)
        return res;


    auto &color_resolve_texture = offscreen_resources.color_resolve_texture;
    if (config->samples)
    {
        res = create_texture(s, NGLI_FORMAT_R8G8B8A8_UNORM, config->width,
                             config->height, 1,
                             NGLI_TEXTURE_USAGE_COLOR_ATTACHMENT_BIT |
                                 NGLI_TEXTURE_USAGE_TRANSFER_SRC_BIT |
                                 NGLI_TEXTURE_USAGE_SAMPLED_BIT,
                             &color_resolve_texture);
        if (res < 0)
            return res;
    }

    // Depth stencil
    auto& depth_stencil_texture =
        offscreen_resources.depth_stencil_texture;
    res = create_texture(
        s, to_ngli_format(s_priv->graphics_context->depthStencilFormat),
        config->width, config->height, config->samples,
        NGLI_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        &depth_stencil_texture);
    if(res < 0)
        return res;


    struct texture* color = color_resolve_texture ? color_resolve_texture : color_texture;
    struct texture* resolve_color = color_resolve_texture ? color_texture    : NULL;

    auto &rt = offscreen_resources.rt;
    // TODO: use STORE_OP_DONT_CARE for depth buffer?
    res = create_rendertarget(s, color, resolve_color,
                              depth_stencil_texture, NGLI_LOAD_OP_CLEAR,
                              NGLI_STORE_OP_STORE, NGLI_LOAD_OP_CLEAR,
                              NGLI_STORE_OP_STORE, &rt);
    if (res < 0)
        return res;

    auto &rt_load = offscreen_resources.rt_load;
    res           = create_rendertarget(s, color, resolve_color,
                                        depth_stencil_texture, NGLI_LOAD_OP_LOAD,
                                        NGLI_STORE_OP_STORE, NGLI_LOAD_OP_LOAD,
                                        NGLI_STORE_OP_STORE, &rt_load);
    if (res < 0)
        return res;

    s_priv->default_rendertarget = rt;

    s_priv->default_rendertarget_load = rt_load;

    return 0;
}

static void d3d12_set_clear_color(struct gpu_ctx *s, const float *color);

static int d3d12_init(struct gpu_ctx *s)
{
    const ngl_config *config = &s->config;
    if(config->offscreen)
    {
        if(config->width <= 0 || config->height <= 0)
        {
            LOG(ERROR, "could not create offscreen context with invalid dimensions (%dx%d)",
                config->width, config->height);
            return NGL_ERROR_INVALID_ARG;
        }
    }
    else
    {
        if(config->capture_buffer)
        {
            LOG(ERROR, "capture_buffer is not supported by onscreen context");
            return NGL_ERROR_INVALID_ARG;
        }
    }

    gpu_ctx_d3d12 *ctx = (gpu_ctx_d3d12 *)s;
#if DEBUG_GPU_CAPTURE
    const char *var = getenv("NGL_GPU_CAPTURE");
    s->gpu_capture  = var && !strcmp(var, "yes");
    if (s->gpu_capture) {
        s->gpu_capture_ctx = ngli_gpu_capture_ctx_create(s);
        if (!s->gpu_capture_ctx) {
            LOG(ERROR, "could not create GPU capture context");
            return NGL_ERROR_MEMORY;
        }
        int ret = ngli_gpu_capture_init(s->gpu_capture_ctx);
        if (ret < 0) {
            LOG(ERROR, "could not initialize GPU capture");
            s->gpu_capture = 0;
            return ret;
        }
    }
#endif
    /* FIXME */
    s->features            = -1;
    ctx->graphics_context  = ngli::D3DGraphicsContext::newInstance("NGLApplication", false);
    if(!ctx->graphics_context)
        return NGL_ERROR_MEMORY;

    auto &graphics_context = ctx->graphics_context;
#if DEBUG_GPU_CAPTURE
    if (s->gpu_capture)
        ngli_gpu_capture_begin(s->gpu_capture_ctx);
#endif
    if (config->offscreen)
    {
        ctx->surface = surface_util_d3d12::create_offscreen_surface( config->width, config->height);
    }
    else
    {
        ctx->surface = surface_util_d3d12::create_surface_from_window_handle(
            graphics_context, config->platform, config->display, config->window,
            config->width, config->height);
        ctx->swapchain_util =  swapchain_util_d3d12::newInstance(graphics_context, config->window);
    }
    graphics_context->setSurface(ctx->surface);
    ctx->graphics = ngli::D3DGraphics::newInstance(graphics_context);

    if (config->offscreen)
    {
        create_offscreen_resources(s); 
    }
    else
    {
        create_onscreen_resources(s);
    }

    create_dummy_texture(s);

    const int *viewport = config->viewport;
    if (viewport[2] > 0 && viewport[3] > 0) {
        memcpy(ctx->viewport, viewport, sizeof(ctx->viewport));
    } else {
        const int default_viewport[] = {0, 0, config->width, config->height};
        memcpy(ctx->viewport, default_viewport, sizeof(ctx->viewport));
    }

    const int scissor[] = {0, 0, config->width, config->height};
    memcpy(ctx->scissor, scissor, sizeof(ctx->scissor));

    d3d12_set_clear_color(s, config->clear_color);

    auto &default_rt_desc = ctx->default_rendertarget_desc;
    if (config->offscreen) {
        default_rt_desc.nb_colors         = 1;
        default_rt_desc.colors[0].format  = NGLI_FORMAT_R8G8B8A8_UNORM;
        default_rt_desc.samples           = config->samples;
        default_rt_desc.colors[0].resolve = config->samples > 0 ? 1 : 0;
        default_rt_desc.depth_stencil.format =
            to_ngli_format(graphics_context->depthStencilFormat);
        default_rt_desc.depth_stencil.resolve = 0;
    } else {
        default_rt_desc.nb_colors = 1;
        default_rt_desc.colors[0].format =
            to_ngli_format(graphics_context->surfaceFormat);
        default_rt_desc.samples           = config->samples;
        default_rt_desc.colors[0].resolve = config->samples > 0 ? 1 : 0;
        default_rt_desc.depth_stencil.format =
            to_ngli_format(graphics_context->depthStencilFormat);
    }

    // TODO: query programmatically

    s->limits.max_color_attachments              = 8;
    s->limits.max_texture_dimension_1d           = 16384;
    s->limits.max_texture_dimension_2d           = 16384;
    s->limits.max_texture_dimension_3d           = 2048;
    s->limits.max_texture_dimension_cube         = 16384;
    s->limits.max_compute_work_group_count[0]    = 65535;
    s->limits.max_compute_work_group_count[1]    = 65535;
    s->limits.max_compute_work_group_count[2]    = 65535;
    s->limits.max_compute_work_group_invocations = 1024;
    s->limits.max_compute_work_group_size[0]     = 1024;
    s->limits.max_compute_work_group_size[1]     = 1024;
    s->limits.max_compute_work_group_size[2]     = 1024;
    s->limits.max_draw_buffers        = s->limits.max_color_attachments;
    s->limits.max_samples             = 8;
    s->limits.max_texture_image_units = 0;
    s->limits.max_uniform_block_size  = INT_MAX;

    if (config->hud)
        ctx->enable_profiling = true;

    return 0;
}

static int d3d12_resize(struct gpu_ctx *s, int width, int height,
                       const int *viewport)
{
    const ngl_config *config = &s->config;
    if (config->offscreen) {
        LOG(ERROR, "resize operation is not supported by offscreen context");
        return NGL_ERROR_UNSUPPORTED;
    }
    // TODO Renan
    //TODO("w: %d h: %d", width, height);
    return 0;
}

static int d3d12_set_capture_buffer(struct gpu_ctx *s, void *capture_buffer)
{
 //   struct gpu_ctx_d3d12 *s_priv = (struct gpu_ctx_d3d12 *)s;
    if (!s->config.offscreen)
        return NGL_ERROR_INVALID_USAGE;
    s->config.capture_buffer = capture_buffer;
    return 0;
}

static int d3d12_begin_update(struct gpu_ctx *s, double t) { return 0; }

static int d3d12_end_update(struct gpu_ctx *s, double t) { return 0; }

static void d3d12_begin_render_pass(struct gpu_ctx *s, struct rendertarget *rt);
static void d3d12_end_render_pass(struct gpu_ctx *s);

static int d3d12_begin_draw(struct gpu_ctx *s, double t)
{
    gpu_ctx_d3d12 *s_priv = (gpu_ctx_d3d12 *)s;
//    ngli::D3DGraphicsContext *ctx = s_priv->graphics_context;
    if (!s->config.offscreen) {
        s_priv->swapchain_util->acquire_image();
    }
    s_priv->cur_command_buffer = s_priv->graphics_context->drawCommandBuffer();
    ngli::D3DCommandList *cmd_buf     = s_priv->cur_command_buffer;
//    const struct ngl_config *config = &s->config;
    cmd_buf->begin();
    if (s_priv->enable_profiling) {
        s_priv->graphics->beginProfile(cmd_buf);
    }
    return 0;
}

static int d3d12_query_draw_time(struct gpu_ctx *s, int64_t *time)
{
    gpu_ctx_d3d12 *s_priv      = (gpu_ctx_d3d12 *)s;
    *time                     = s_priv->profile_data.time;
    s_priv->profile_data.time = 0;
    return 0;
}

static int d3d12_end_draw(struct gpu_ctx *s, double t)
{
    gpu_ctx_d3d12 *s_priv = (gpu_ctx_d3d12 *)s;
    ngli::D3DGraphicsContext *ctx = s_priv->graphics_context;
    if (s_priv->enable_profiling) {
        s_priv->profile_data.time =
            s_priv->graphics->endProfile(s_priv->cur_command_buffer);
    }
    s_priv->cur_command_buffer->end();
    if (s->config.offscreen) {
        ctx->submit(s_priv->cur_command_buffer);
        if (s->config.capture_buffer) {
            uint32_t size = s->config.width * s->config.height * 4;
            auto &output_color_resolve_texture = s_priv->offscreen_resources.color_resolve_texture;
            auto &output_color_texture = s_priv->offscreen_resources.color_texture;
            auto &output_texture =
                ((texture_d3d12 *)(output_color_resolve_texture
                                      ? output_color_resolve_texture
                                      : output_color_texture))->v;
            output_texture->download(s->config.capture_buffer, size);
        } else {
            if (ctx->queue)
                ctx->queue->waitIdle();
        }
    } else {
        s_priv->swapchain_util->present(s_priv->cur_command_buffer);
    }
    return 0;
}

static void d3d12_wait_idle(struct gpu_ctx *s)
{
    gpu_ctx_d3d12 *s_priv = (gpu_ctx_d3d12 *)s;
    if (s_priv->cur_command_buffer)
        s_priv->graphics->waitIdle(s_priv->cur_command_buffer);
}

static void d3d12_destroy(struct gpu_ctx *s)
{
    gpu_ctx_d3d12 *ctx = (gpu_ctx_d3d12 *)s;

    d3d12_wait_idle(s);

#if DEBUG_GPU_CAPTURE
    if (s->gpu_capture)
        ngli_gpu_capture_end(s->gpu_capture_ctx);
    ngli_gpu_capture_freep(&s->gpu_capture_ctx);
#endif

    auto output_color_texture =
        ((texture *)ctx->offscreen_resources.color_texture);
    auto output_color_resolve_texture =
        ((texture *)ctx->offscreen_resources.color_resolve_texture);
    auto output_depth__stencil_texture =
        ((texture *)ctx->offscreen_resources.depth_stencil_texture);
//    auto dummy_texture = ctx->dummy_texture;
    if (output_depth__stencil_texture)
        ngli_texture_freep(&output_depth__stencil_texture);
    if (output_color_texture)
        ngli_texture_freep(&output_color_texture);
    if (output_color_resolve_texture)
        ngli_texture_freep(&output_color_resolve_texture);
    destroy_dummy_texture(s);
    if (ctx->default_rendertarget)
        ngli_rendertarget_freep(&ctx->default_rendertarget);
    if (ctx->swapchain_util)
        delete ctx->swapchain_util;
    if (ctx->surface)
        delete ctx->surface;
    delete ctx->graphics;
    delete ctx->graphics_context;
}

static int d3d12_transform_cull_mode(struct gpu_ctx *s, int cull_mode)
{
    return cull_mode;
}

static void d3d12_transform_projection_matrix(struct gpu_ctx *s, float *dst)
{
    static const NGLI_ALIGNED_MAT(matrix) = {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f,  0.5f, 1.0f,
    };
    ngli_mat4_mul(dst, matrix, dst);
}

static void d3d12_get_rendertarget_uvcoord_matrix(struct gpu_ctx *s, float *dst)
{
    static const NGLI_ALIGNED_MAT(matrix) = NGLI_MAT4_IDENTITY;
    memcpy(dst, matrix, sizeof(matrix));
}

static struct rendertarget *d3d12_get_default_rendertarget(struct gpu_ctx *s,
                                                          int load_op)
{
    gpu_ctx_d3d12 *s_priv = (gpu_ctx_d3d12 *)s;
    switch (load_op) {
    case NGLI_LOAD_OP_DONT_CARE:
    case NGLI_LOAD_OP_CLEAR:
        return s_priv->default_rendertarget;
    case NGLI_LOAD_OP_LOAD:
        return s_priv->default_rendertarget_load;
    default:
        ngli_assert(0);
    }
}

static const struct rendertarget_desc *
d3d12_get_default_rendertarget_desc(struct gpu_ctx *s)
{
    struct gpu_ctx_d3d12 *s_priv = (struct gpu_ctx_d3d12 *)s;
    return &s_priv->default_rendertarget_desc;
}

static void begin_render_pass(struct gpu_ctx_d3d12 *s_priv,
                              rendertarget_d3d12 *rt_priv)
{
    ngli::D3DGraphics *graphics     = s_priv->graphics;
    ngli::D3DGraphicsContext *ctx   = s_priv->graphics_context;
    ngli::D3DCommandList *cmd_buf = s_priv->cur_command_buffer;
    if (!rt_priv) {
        // use d3d12 default renderpass
        ctx->beginRenderPass(cmd_buf, graphics);
        return;
    }
    ngli::D3DRenderPass *render_pass = rt_priv->render_pass;

    ngli::D3DFramebuffer *framebuffer = rt_priv->output_framebuffer;

    ngli_assert(framebuffer->colorAttachments.size() >= rt_priv->parent.params.nb_colors);

    for(int i = rt_priv->parent.params.nb_colors; i--;)
    {
        framebuffer->colorAttachments[i]->mAttachement = rt_priv->parent.params.colors[i];
    }

    graphics->beginRenderPass(cmd_buf, render_pass, framebuffer);
}

static void end_render_pass(struct gpu_ctx_d3d12 *s_priv, rendertarget_d3d12 *)
{
    ngli::D3DGraphics *graphics     = s_priv->graphics;
    ngli::D3DCommandList *cmd_buf = s_priv->cur_command_buffer;
    if (graphics->currentRenderPass)
        graphics->endRenderPass(cmd_buf);
}

static void d3d12_begin_render_pass(struct gpu_ctx *s, struct rendertarget *rt)
{
    gpu_ctx_d3d12 *s_priv       = (gpu_ctx_d3d12 *)s;
    rendertarget_d3d12 *rt_priv = (rendertarget_d3d12 *)rt;
    if (rt_priv)
    {
        const auto &attachments = rt_priv->output_framebuffer->d3dAttachments;

        for (uint32_t j = 0; j < attachments.size(); j++)
        {
            auto output_texture = attachments[j].mD3DAttachementBasic.texture;
            if (output_texture->imageUsageFlags & NGLI_TEXTURE_USAGE_COLOR_ATTACHMENT_BIT)
            {
                output_texture->changeLayout(
                    s_priv->cur_command_buffer,
                    ngli::IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            }
            else if (output_texture->imageUsageFlags & NGLI_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                output_texture->changeLayout(
                    s_priv->cur_command_buffer,
                    ngli::IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            }
        }
    }

    begin_render_pass(s_priv, rt_priv);

    s_priv->current_rendertarget = rt;
}

static void d3d12_end_render_pass(struct gpu_ctx *s)
{
    gpu_ctx_d3d12 *s_priv       = (gpu_ctx_d3d12 *)s;
    rendertarget_d3d12 *rt_priv = (rendertarget_d3d12 *)s_priv->current_rendertarget;

    end_render_pass(s_priv, rt_priv);

    if (rt_priv) {
        const auto &attachments = rt_priv->output_framebuffer->d3dAttachments;
        for (uint32_t j = 0; j < attachments.size(); j++) {
            auto output_texture = attachments[j].mD3DAttachementBasic.texture;
            if (output_texture->imageUsageFlags & NGLI_TEXTURE_USAGE_SAMPLED_BIT) {
                ngli_assert(output_texture->numSamples == 1);
                output_texture->changeLayout(
                    s_priv->cur_command_buffer,
                    ngli::IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }
        }
    }
    s_priv->current_rendertarget = NULL;
}

static void d3d12_set_viewport(struct gpu_ctx *s, const int *vp)
{
    struct gpu_ctx_d3d12 *s_priv = (struct gpu_ctx_d3d12 *)s;
    memcpy(s_priv->viewport, vp, sizeof(s_priv->viewport));
}

static void d3d12_get_viewport(struct gpu_ctx *s, int *viewport)
{
    struct gpu_ctx_d3d12 *s_priv = (struct gpu_ctx_d3d12 *)s;
    memcpy(viewport, &s_priv->viewport, sizeof(s_priv->viewport));
}

static void d3d12_set_scissor(struct gpu_ctx *s, const int *sr)
{
    struct gpu_ctx_d3d12 *s_priv = (struct gpu_ctx_d3d12 *)s;
    memcpy(&s_priv->scissor, sr, sizeof(s_priv->scissor));
}

static void d3d12_get_scissor(struct gpu_ctx *s, int *scissor)
{
    struct gpu_ctx_d3d12 *s_priv = (struct gpu_ctx_d3d12 *)s;
    memcpy(scissor, &s_priv->scissor, sizeof(s_priv->scissor));
}

static void d3d12_set_clear_color(struct gpu_ctx *s, const float *color)
{
    struct gpu_ctx_d3d12 *s_priv = (struct gpu_ctx_d3d12 *)s;
    memcpy(s_priv->clear_color, color, sizeof(s_priv->clear_color));
}

static int d3d12_get_preferred_depth_format(struct gpu_ctx *s)
{
    gpu_ctx_d3d12 *ctx = (gpu_ctx_d3d12 *)s;
    return to_ngli_format(ctx->graphics_context->depthFormat);
}

static int d3d12_get_preferred_depth_stencil_format(struct gpu_ctx *s)
{
    gpu_ctx_d3d12 *ctx = (gpu_ctx_d3d12 *)s;
    return to_ngli_format(ctx->graphics_context->depthStencilFormat);
}

extern "C" const struct gpu_ctx_class ngli_gpu_ctx_d3d12 = {
    .name               = "d3d12",
    .create             = d3d12_create,
    .init               = d3d12_init,
    .resize             = d3d12_resize,
    .set_capture_buffer = d3d12_set_capture_buffer,
    .begin_update       = d3d12_begin_update,
    .end_update         = d3d12_end_update,
    .begin_draw         = d3d12_begin_draw,
    .end_draw           = d3d12_end_draw,
    .query_draw_time    = d3d12_query_draw_time,
    .wait_idle          = d3d12_wait_idle,
    .destroy            = d3d12_destroy,

    .transform_cull_mode             = d3d12_transform_cull_mode,
    .transform_projection_matrix     = d3d12_transform_projection_matrix,
    .get_rendertarget_uvcoord_matrix = d3d12_get_rendertarget_uvcoord_matrix,

    .get_default_rendertarget      = d3d12_get_default_rendertarget,
    .get_default_rendertarget_desc = d3d12_get_default_rendertarget_desc,

    .begin_render_pass = d3d12_begin_render_pass,
    .end_render_pass   = d3d12_end_render_pass,

    .set_viewport = d3d12_set_viewport,
    .get_viewport = d3d12_get_viewport,
    .set_scissor  = d3d12_set_scissor,
    .get_scissor  = d3d12_get_scissor,

    .get_preferred_depth_format = d3d12_get_preferred_depth_format,
    .get_preferred_depth_stencil_format = d3d12_get_preferred_depth_stencil_format,

    .buffer_create = d3d12_buffer_create,
    .buffer_init   = d3d12_buffer_init,
    .buffer_upload = d3d12_buffer_upload,
    .buffer_map    = d3d12_buffer_map,
    .buffer_unmap  = d3d12_buffer_unmap,
    .buffer_freep  = d3d12_buffer_freep,

    .pipeline_create           = d3d12_pipeline_create,
    .pipeline_init             = d3d12_pipeline_init,
    .pipeline_set_resources    = d3d12_pipeline_set_resources,
    .pipeline_update_attribute = d3d12_pipeline_update_attribute,
    .pipeline_update_uniform   = d3d12_pipeline_update_uniform,
    .pipeline_update_texture   = d3d12_pipeline_update_texture,
    .pipeline_update_buffer    = d3d12_pipeline_update_buffer,
    .pipeline_draw             = d3d12_pipeline_draw,
    .pipeline_draw_indexed     = d3d12_pipeline_draw_indexed,
    .pipeline_dispatch         = d3d12_pipeline_dispatch,
    .pipeline_freep            = d3d12_pipeline_freep,

    .program_create = d3d12_program_create,
    .program_init   = d3d12_program_init,
    .program_freep  = d3d12_program_freep,

    .rendertarget_create = d3d12_rendertarget_create,
    .rendertarget_init   = d3d12_rendertarget_init,
    .rendertarget_freep  = d3d12_rendertarget_freep,

    .texture_create          = d3d12_texture_create,
    .texture_init            = d3d12_texture_init,
    .texture_upload          = d3d12_texture_upload,
    .texture_generate_mipmap = d3d12_texture_generate_mipmap,
    .texture_freep           = d3d12_texture_freep,
};
