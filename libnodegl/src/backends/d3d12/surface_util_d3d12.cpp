/*
 * Copyright 2020 GoPro Inc.
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

#include "surface_util_d3d12.h"
//#include "debugutil_d3d12.h"
#include "gpu_ctx_d3d12.h"
#include <backends/d3d12/impl/D3DSurface.h>

ngli::D3DSurface* surface_util_d3d12::create_offscreen_surface(int w, int h)
{
	return new ngli::D3DSurface(w, h, true);
}

ngli::D3DSurface* surface_util_d3d12::create_surface_from_window_handle(
	ngli::D3DGraphicsContext* ctx, int platform, uintptr_t display,
	uintptr_t window, uintptr_t width, uintptr_t height)
{
	ngli::D3DSurface* d3d_surface = new ngli::D3DSurface();
	d3d_surface->mHwnd = HWND(window);
	d3d_surface->mW = width;
	d3d_surface->mH = height;
	d3d_surface->mOffscreen = false;
	return d3d_surface;
}
