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

#pragma once

namespace ngli
{
class D3DGraphicsContext;
class D3DSurface;
}


extern "C" {

	struct surface_util_d3d12
	{
		static ngli::D3DSurface* create_offscreen_surface(int w, int h);

		static ngli::D3DSurface*
			create_surface_from_window_handle(ngli::D3DGraphicsContext* ctx, int platform,
											  uintptr_t display, uintptr_t window,
											  uintptr_t width, uintptr_t height);
	};

}