/*
 * Copyright 2019-2022 GoPro Inc.
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

#include <string.h>

#include "gpu_ctx.h"
#include "log.h"
#include "memory.h"
#include "internal.h"
#include "rendertarget.h"

extern const struct gpu_ctx_class ngli_gpu_ctx_gl;
extern const struct gpu_ctx_class ngli_gpu_ctx_gles;
extern const struct gpu_ctx_class ngli_gpu_ctx_vk;
extern const struct gpu_ctx_class ngli_gpu_ctx_d3d12;

static const struct {
    const struct gpu_ctx_class *cls;
} backend_map[NGL_NB_BACKEND] = {
#ifdef BACKEND_GL
    [NGL_BACKEND_OPENGL] = {
        .cls = &ngli_gpu_ctx_gl,
    },
#endif
#ifdef BACKEND_GLES
    [NGL_BACKEND_OPENGLES] = {
        .cls = &ngli_gpu_ctx_gles,
    },
#endif
#ifdef BACKEND_VK
    [NGL_BACKEND_VULKAN] = {
        .cls = &ngli_gpu_ctx_vk,
    },
#endif
#ifdef BACKEND_D3D12
    [NGL_BACKEND_D3D12] = {
        .cls = &ngli_gpu_ctx_d3d12,
    },
#endif
};

struct gpu_ctx *ngli_gpu_ctx_create(const struct ngl_config *config)
{
    if (config->backend < 0 ||
        config->backend >= NGL_NB_BACKEND) {
        LOG(ERROR, "unknown backend %d", config->backend);
        return NULL;
    }
    if (!backend_map[config->backend].cls) {
        LOG(ERROR, "backend \"%s\" not available with this build",
            ngli_get_backend_string_id(config->backend));
        return NULL;
    }

    struct ngl_config ctx_config = {0};
    int ret = ngli_config_copy(&ctx_config, config);
    if (ret < 0)
        return NULL;

    const struct gpu_ctx_class *cls = backend_map[config->backend].cls;
    struct gpu_ctx *s = cls->create(config);
    if (!s) {
        ngli_config_reset(&ctx_config);
        return NULL;
    }
    s->config = ctx_config;
    s->backend_str = ngli_get_backend_string_id(config->backend);
    s->cls = cls;
    return s;
}

int ngli_gpu_ctx_init(struct gpu_ctx *s)
{
    return s->cls->init(s);
}

int ngli_gpu_ctx_resize(struct gpu_ctx *s, int width, int height, const int *viewport)
{
    const struct gpu_ctx_class *cls = s->cls;
    return cls->resize(s, width, height, viewport);
}

int ngli_gpu_ctx_set_capture_buffer(struct gpu_ctx *s, void *capture_buffer)
{
    const struct gpu_ctx_class *cls = s->cls;
    return cls->set_capture_buffer(s, capture_buffer);
}

int ngli_gpu_ctx_begin_update(struct gpu_ctx *s, double t)
{
    return s->cls->begin_update(s, t);
}

int ngli_gpu_ctx_end_update(struct gpu_ctx *s, double t)
{
    return s->cls->end_update(s, t);
}

int ngli_gpu_ctx_begin_draw(struct gpu_ctx *s, double t)
{
    return s->cls->begin_draw(s, t);
}

int ngli_gpu_ctx_end_draw(struct gpu_ctx *s, double t)
{
    return s->cls->end_draw(s, t);
}

int ngli_gpu_ctx_query_draw_time(struct gpu_ctx *s, int64_t *time)
{
    return s->cls->query_draw_time(s, time);
}

void ngli_gpu_ctx_wait_idle(struct gpu_ctx *s)
{
    s->cls->wait_idle(s);
}

void ngli_gpu_ctx_freep(struct gpu_ctx **sp)
{
    if (!*sp)
        return;

    struct gpu_ctx *s = *sp;
    const struct gpu_ctx_class *cls = s->cls;
    if (cls)
        cls->destroy(s);

    ngli_config_reset(&s->config);
    ngli_freep(sp);
}

int ngli_gpu_ctx_transform_cull_mode(struct gpu_ctx *s, int cull_mode)
{
    return s->cls->transform_cull_mode(s, cull_mode);
}

void ngli_gpu_ctx_transform_projection_matrix(struct gpu_ctx *s, float *dst)
{
    s->cls->transform_projection_matrix(s, dst);
}

void ngli_gpu_ctx_begin_render_pass(struct gpu_ctx *s, struct rendertarget *rt)
{
    s->cls->begin_render_pass(s, rt);
}

void ngli_gpu_ctx_end_render_pass(struct gpu_ctx *s)
{
    s->cls->end_render_pass(s);
}

void ngli_gpu_ctx_get_rendertarget_uvcoord_matrix(struct gpu_ctx *s, float *dst)
{
    s->cls->get_rendertarget_uvcoord_matrix(s, dst);
}

struct rendertarget *ngli_gpu_ctx_get_default_rendertarget(struct gpu_ctx *s, int load_op)
{
    return s->cls->get_default_rendertarget(s, load_op);
}

const struct rendertarget_desc *ngli_gpu_ctx_get_default_rendertarget_desc(struct gpu_ctx *s)
{
    return s->cls->get_default_rendertarget_desc(s);
}

void ngli_gpu_ctx_set_viewport(struct gpu_ctx *s, const int *viewport)
{
    s->cls->set_viewport(s, viewport);
}

void ngli_gpu_ctx_get_viewport(struct gpu_ctx *s, int *viewport)
{
    s->cls->get_viewport(s, viewport);
}

void ngli_gpu_ctx_set_scissor(struct gpu_ctx *s, const int *scissor)
{
    s->cls->set_scissor(s, scissor);
}

void ngli_gpu_ctx_get_scissor(struct gpu_ctx *s, int *scissor)
{
    s->cls->get_scissor(s, scissor);
}

int ngli_gpu_ctx_get_preferred_depth_format(struct gpu_ctx *s)
{
    return s->cls->get_preferred_depth_format(s);
}

int ngli_gpu_ctx_get_preferred_depth_stencil_format(struct gpu_ctx *s)
{
    return s->cls->get_preferred_depth_stencil_format(s);
}
