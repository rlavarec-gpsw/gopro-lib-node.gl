/*
 * Copyright 2020-2022 GoPro Inc.
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
#include "gpu_ctx.h"
}
#include <backends/d3d12/impl/D3DCommandList.h>
#include <backends/d3d12/impl/D3DSurface.h>
#include <backends/d3d12/impl/D3DGraphicsContext.h>
#include <backends/d3d12/swapchain_util_d3d12.h>
#include <memory>

struct gpu_ctx_d3d12
{
    struct gpu_ctx parent;

    ngli::D3DGraphicsContext* graphics_context = nullptr;
    ngli::D3DGraphics* graphics = nullptr;
    ngli::D3DSurface* surface = nullptr;
    ngli::D3DCommandList* cur_command_buffer = nullptr;

    struct rendertarget* default_rendertarget = nullptr;
    struct rendertarget* default_rendertarget_load = nullptr;
    struct rendertarget_desc default_rendertarget_desc;
    rendertarget* current_rendertarget = nullptr;

    int viewport[4];
    int scissor[4];
    float clear_color[4];

    struct offscreen
    {
        texture* color_texture = nullptr;
        texture* depth_stencil_texture = nullptr;
        texture* color_resolve_texture = nullptr;
        texture* depth_stencil_resolve_texture = nullptr;
        rendertarget* rt = nullptr;
        rendertarget* rt_load = nullptr;

    } offscreen_resources;

    texture* dummy_texture = nullptr;

    swapchain_util_d3d12* swapchain_util = nullptr;

    bool enable_profiling = false;

    struct
    {
        uint64_t time = 0;
    } profile_data;
};
