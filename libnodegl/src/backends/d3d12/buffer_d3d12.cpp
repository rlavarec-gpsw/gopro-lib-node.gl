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

#include <stdafx.h>
#include "buffer_d3d12.h"
#include "gpu_ctx_d3d12.h"

#include <backends/d3d12/impl/D3DBuffer.h>


struct buffer *ngli_buffer_d3d12_create(struct gpu_ctx *gpu_ctx)
{
    struct buffer_d3d12* buffer = new buffer_d3d12();
    if (!buffer)
        return NULL;
    buffer->parent.gpu_ctx = gpu_ctx;
    return (struct buffer *)buffer;
}

int ngli_buffer_d3d12_init(struct buffer* buffer, int size, int bufferUsageFlags)
{
    struct gpu_ctx_d3d12* ctx = (struct gpu_ctx_d3d12*)buffer->gpu_ctx;
    struct buffer_d3d12* s_priv = (struct buffer_d3d12*)buffer;
    buffer->size = size;
    buffer->usage = bufferUsageFlags;
    // TODO: pass usage flags (e.g. vertex buffer, index buffer, uniform buffer,
    // etc
    s_priv->mBuffer = ngli::D3DBuffer::newInstance(
        ctx->graphics_context, NULL, size,
        ngli::BUFFER_USAGE_VERTEX_BUFFER_BIT | ngli::BUFFER_USAGE_INDEX_BUFFER_BIT |
            ngli::BUFFER_USAGE_UNIFORM_BUFFER_BIT | ngli::BUFFER_USAGE_STORAGE_BUFFER_BIT);
    return 0;
}

int ngli_buffer_d3d12_upload(struct buffer *s, const void *data, int size, int offset)
{
    struct buffer_d3d12 *s_priv = (struct buffer_d3d12 *)s;
    s_priv->mBuffer->upload(data, size);
    return 0;
}

int ngli_buffer_d3d12_map(struct buffer *s, int size, int offset, void **data)
{
    struct buffer_d3d12 *s_priv = (struct buffer_d3d12 *)s;
    s->data = s_priv->mBuffer->map();
    *data = s->data;
    return 0;
}

void ngli_buffer_d3d12_unmap(struct buffer *s)
{
    struct buffer_d3d12 *s_priv = (struct buffer_d3d12 *)s;
    s_priv->mBuffer->unmap();
    s->data = nullptr;
}

void ngli_buffer_d3d12_freep(struct buffer **sp)
{
    if (!*sp)
        return;

    struct buffer *s = *sp;
    struct gpu_ctx *ctx = (struct gpu_ctx*)s->gpu_ctx;
    ctx->cls->wait_idle(ctx);

    struct buffer_d3d12 *s_priv = (struct buffer_d3d12 *)s;
    delete s_priv;
    *sp=0;
}
