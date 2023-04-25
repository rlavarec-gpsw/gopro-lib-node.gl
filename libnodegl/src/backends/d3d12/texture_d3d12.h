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

#pragma once

extern "C" {
#include "texture.h"
}

namespace ngli
{
class D3DTexture;
}

struct texture_d3d12
{
    struct texture parent;
    int bytes_per_pixel = 0;
    ngli::D3DTexture* v;
};

struct texture* d3d12_texture_create(struct gpu_ctx* gpu_ctx);

int d3d12_texture_init(struct texture* s,
                           const struct texture_params* params);

int d3d12_texture_upload(struct texture* s, const uint8_t* data,
                             int linesize);
int d3d12_texture_generate_mipmap(struct texture* s);

void d3d12_texture_freep(struct texture** sp);
