/*
 * Copyright 2023 GoPro Inc.
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

#include "buffer_mtl.h"
extern "C" {
#include "memory.h"
}
//#include "gctx_mtl.h"

struct buffer *ngli_buffer_mtl_create(struct gctx *gctx) {
    buffer_mtl *s = (buffer_mtl *)ngli_calloc(1, sizeof(*s));
    if (!s)
        return nullptr;
    s->parent.gctx = gctx;
    return (struct buffer *)s;
}

int ngli_buffer_mtl_init(struct buffer *s, int size, int usage) {
    // align to 16
    size = (size + 15) / 16 * 16;

    struct gctx_mtl *ctx = (struct gctx_mtl *)s->gctx;
    struct buffer_mtl *priv = (struct buffer_mtl *)s;
    s->size = size;
    s->usage = usage;

    priv->buffer = NS::TransferPtr(ctx->device->newBuffer(size, MTL::ResourceStorageModeShared));
    return 0;
}

int ngli_buffer_mtl_upload(struct buffer *s, const void *data, int size, int offset) {
    if (data) {
        struct buffer_mtl *priv = (struct buffer_mtl *)s;
        memcpy((uint8_t *)priv->buffer->contents() + offset, data, size);
    }
    return 0;
}

int ngli_buffer_mtl_download(struct buffer *s, void *data, int size, int offset) {
    if (data) {
        struct buffer_mtl *priv = (struct buffer_mtl *)s;
        memcpy(data, (uint8_t *)priv->buffer->contents() + offset, size);
    }

    return 0;
}

int ngli_buffer_mtl_map(struct buffer *s, int size, int offset, void **data) {
    if (data) {
        struct buffer_mtl *priv = (struct buffer_mtl *)s;
        *data = priv->buffer->contents();
    }

    return 0;
}

int ngli_buffer_mtl_unmap(struct buffer *s) {
    return 0;
}

void ngli_buffer_mtl_freep(struct buffer **sp) {
    if (!sp) 
        return;

    (*sp)->gctx->cls->wait_idle((*sp)->gctx);
    buffer *s = *sp;
    ngli_freep(sp);
}

