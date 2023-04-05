/*
 * Copyright 2019 GoPro Inc.
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

#include "format.h"
#include "format_d3d12.h"
#include "rendertarget.h"

namespace ngli
{
class D3DGraphicsContext;
class D3DRenderPass;
}


ngli::FilterMode to_d3d12_filter_mode(int filter);
ngli::FilterMode to_d3d12_mip_filter_mode(int filter);
ngli::TextureType to_d3d12_texture_type(int type);
ngli::IndexFormat to_d3d12_index_format(int indices_format);
D3D12_BLEND to_d3d12_blend_factor(int blend_factor);
D3D12_BLEND_OP to_d3d12_blend_op(int blend_op);
D3D12_COMPARISON_FUNC to_d3d12_compare_op(int compare_op);
D3D12_STENCIL_OP to_d3d12_stencil_op(int stencil_op);
ngli::ColorComponentFlags to_d3d12_color_mask(int color_write_mask);
D3D12_CULL_MODE to_d3d12_cull_mode(int cull_mode);
ngli::ImageUsageFlags to_d3d12_image_usage_flags(int usage_flags);
ngli::AttachmentLoadOp to_d3d12_load_op(int op);
ngli::AttachmentStoreOp to_d3d12_store_op(int op);
ngli::D3DRenderPass* get_render_pass(ngli::D3DGraphicsContext* ctx,
								  const struct rendertarget_params* params);
ngli::D3DRenderPass* get_compat_render_pass(ngli::D3DGraphicsContext* ctx,
										 const struct rendertarget_desc& desc);

inline uint32_t get_bpp(int format)
{
	return ngli_format_get_bytes_per_pixel(format);
}

