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

#include "rendertarget.h"

namespace ngli
{
class D3DRenderPass;
class D3DFramebuffer;
}

struct rendertarget_d3d12
{
    struct rendertarget parent;
    ngli::D3DRenderPass* render_pass = nullptr;
    ngli::D3DFramebuffer* output_framebuffer = nullptr;
};

struct rendertarget* ngli_rendertarget_d3d12_create(struct gpu_ctx* gpu_ctx);
int ngli_rendertarget_d3d12_init(struct rendertarget* s,
                                const struct rendertarget_params* params);
void ngli_rendertarget_d3d12_resolve(struct rendertarget* s);
void ngli_rendertarget_d3d12_freep(struct rendertarget** sp);
