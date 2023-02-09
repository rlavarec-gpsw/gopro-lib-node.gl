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

#ifndef TEXTURE_MTL_H
#define TEXTURE_MTL_H

extern "C" {
#include "texture.h"
}

#include "defines_mtl.h"
#include "format_mtl.h"
#include "Metal.hpp"

struct texture_mtl {
    struct texture parent;

    NS::SharedPtr<MTL::Texture> texture;
    NS::SharedPtr<MTL::SamplerState> sampler_state;
    bool has_depth = false;
    bool has_stencil = false;
    uint32_t depth_format;
    uint32_t stencil_format;
    uint32_t size;
    uint32_t mipmap_levels;
};

struct texture *ngli_texture_mtl_create(struct gctx *gctx);
int ngli_texture_mtl_init(struct texture *s, const struct texture_params *params);
int ngli_texture_mtl_has_mipmap(const struct texture *s);
int ngli_texture_mtl_match_dimensions(const struct texture *s, int width, int height, int depth);
int ngli_texture_mtl_upload(struct texture *s, const uint8_t *data, int linesize);
int ngli_texture_mtl_generate_mipmap(struct texture *s);
int ngli_texture_mtl_freep(struct texture **sp);
#endif


