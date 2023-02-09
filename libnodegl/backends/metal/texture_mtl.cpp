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

#include "texture_mtl.h"

#include <algorithm>
#include <cmath>

#include "Metal.hpp"

#include "memory.h"
#include "defines_mtl.h"
#include "format_mtl.h"
#include "utils_mtl.h"

int ngli_texture_mtl_generate_mipmap(struct texture *s) {
    struct texture_mtl *priv = (struct texture_mtl *)s;
    struct gctx_mtl *ctx = (struct gctx_mtl *)s->gctx;
    
    auto command_buffer = ctx->cmd_queue->commandBuffer();
    auto encoder = cmd_buffer->blitCommandEncoder();
    encoder->generateMipmaps(priv->texture);
    cmd_buffer->commit();
    cmd_buffer->waitUntilCompleted();

    encoder->release();
    cmd_buffer->release();
    return 0;
}
    

struct texture *ngli_texture_mtl_create(struct gctx *gctx) {
    texture_mtl *s = (texture_mtl *)ngli_calloc(1, sizeof(*s));
    if (!s)
        return NULL;

    s->parent.gctx = gctx;
    return (struct texture *)s;
}

int ngli_texture_mtl_init(struct texture *s, const struct texture_params *p) {
    struct texture_mtl *priv = (struct texture_mtl *)s;
    struct gctx_mtl *ctx = (struct gctx_mtl *)s->gctx;
    // TODO
    //auto &gctx = ctx->graphics_context; 

    s->params = *p;
    s->bytes_per_pixel = ngli_format_get_nb_comp(p->format);

    priv->has_depth = ngli_format_has_depth(p->format);
    priv->has_stencil = ngli_format_has_stencil(p->format);

    if (priv->has_depth && priv->has_stencil) {
        if (ctx->device->depth24Stencil8PixelFormatSupported) {
            priv->depth_format = priv->stencil_format = 
                MTL::PixelFormatDepth24Unorm_Stencil8;
        } else {
            priv->depth_format = priv->stencil_format = 
                MTL::PixelFormatDepth32Float_Stencil8;
        }
    } else if (priv->has_depth) {
        priv->depth_format = MTL::PixelFormatDepth32Float;
    } else if (priv->has_stencil) {
        priv->stencil_format = MTL::PixelFormatStencil8;
    }

    uint32_t gen_mipmap = (p->type == NGLI_TEXTURE_TYPE_3D) ? p->depth : 1;
    uint32_t array_layers = (p->type == NGLI_TEXTURE_TYPE_CUBE) ? 6 : 1;
    priv->size = s->bytes_per_pixel * p->width * p->height * p->depth * array_layers;
    
    NS::SharedPtr<MTL::TextureDescriptor> descriptor = 
        NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
    descriptor->setPixelFormat(get_mtl_format(priv->ctx, s->params.format));
    descriptor->setWidth(s->params.width);
    descriptor->setHeight(s->params.height);
    descriptor->setDepth(s->params.depth);
    int samples = get_supported_sample_count(ctx->device, s->params.samples);
    descriptor->setSampleCount(samples);

    // Setup usage
    bool used_as_sampler = false;
    MTL::TextureUsage usage = 0;
    if (s->params.usage & NGLI_TEXTURE_USAGE_SAMPLED_BIT) {
        usage |= MTL::TextureUsageShaderRead;
        used_as_sampler = true;
    }
    if (s->params.usage & NGLI_TEXTURE_USAGE_COLOR_ATTACHMENT_BIT) {
        usage |= MTL::TextureUsageRenderTarget;
    } else if (s->params.usage & NGLI_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        usage |= MTL::TextureUsageRenderTarget;
    }
    descriptor->setUsage(usage);
    descriptor->setArrayLength(array_layers);
    descriptor->setPixelFormat(get_mtl_format(priv->ctx, s->params.format));

    // texture type
    if (samples > 1) {
        if (get_mtl_texture_type(priv->ctx, s->params.type) == MTL::TextureType2D) {
            descriptor->setTextureType(MTL::TextureType2DMultisample);
        } else if (get_mtl_texture_type(priv->ctx, s->params.type) == MTL::TextureType2DArray) {
            descriptor->setTextureType(MTL::TextureType2DMultisampleArray);
        }
    } else {
        descriptor->setTextureType(get_mtl_texture_type(priv->ctx, s->params.type));
    }

    priv->mipmap_levels = (gen_mipmap && s->params.mipmap_filter != NGLI_MIPMAP_FILTER_NONE) ? 
        std::floor(std::log2(std::min(s->params.width, s->params.height))) + 1 : 1;
    descriptor->setMipmapLevelCount(priv->mipmap_levels);

    descriptor->setStorageMode(MTL::StorageModePrivate);
 
    priv->texture = NS::TransferPtr(ctx->device->newTexture(descriptor.get()));
 
    if (used_as_sampler) {
        NS::SharedPtr<MTL::SamplerDescriptor> sampler_descriptor = NS::TransferPtr(MTL::SamplerDescriptor::alloc()->init());
        sampler_descriptor->setMinFilter(get_mtl_filter_mode(priv->ctx, s->params.min_filter));
        sampler_descriptor->setMagFilter(get_mtl_filter_mode(priv->ctx, s->params.mag_filter));
        sampler_descriptor->setMipFilter(get_mtl_filter_mode(priv->ctx, s->params.mipmap_filter));
        priv->sampler_state = NS::TransferPtr(ctx->device->newSamplerState(sampler_descriptor.get()));
    }
    return 0;
}

int ngli_texture_mtl_has_mipmap(const struct texture *s) {
    return s->params.mipmap_filter != NGLI_MIPMAP_FILTER_NONE;
}

int ngli_texture_mtl_match_dimension(const struct texture *s, int width,
                                       int height, int depth) {
    const texture_params *params = &s->params;
    return params->width == width && params->height == height && params->depth == depth;
}

int ngli_texture_mtl_upload(struct texture *s, const uint8_t *data, int linesize) {
    if (!data) {
        return -1;
    }
    struct texture_mtl *priv = (struct texture_mtl *)s;
    uint32_t array_layers = (s->params.type == NGLI_TEXTURE_TYPE_CUBE) ? 6 : 1;
    NS::UInteger bytes_per_image = 0;
    if (get_mtl_texture_type(priv->ctx, s->params.type) == MTL::TextureType3D) {
        bytes_per_image = priv->size / s->params.depth;
    } else if (get_mtl_texture_type(priv->ctx, s->params.type) == MTL::TextureTypeCube) {
        bytes_per_image = priv->size / array_layers;
    }
    NS::UInteger bytes_per_row = priv->size / (s->params.height * s->params.depth * array_layers); 
    MTL::Region region = MTL::Region::Make3D(0, 0, 0, s->params.width, s->params.height,
                                         s->params.depth);
    uint8_t *src_data = (uint8_t *)data;
    for (uint32_t slice = 0; slice < array_layers; slice++) {
        priv->texture->replaceRegion(region, 0, slice, src_data, bytes_per_row,
                               bytes_per_image);
        src_data += bytes_per_image;
    }

    if (priv->mipmap_levels != 1) {
        texture_mtl_generate_mipmap(s);
    }
    return 0;
}

void ngli_texture_mtl_freep(struct texture **sp) {
    if (!sp)
        return;

    ngli_freep(sp);
}
