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
#include <d3d12.h>

namespace ngli {

#define	DEFAULT_STENCIL_READ_MASK	( 0xff )
#define	DEFAULT_STENCIL_WRITE_MASK	( 0xff )


enum BlendOp {
  BLEND_OP_ADD = D3D12_BLEND_OP_ADD,
  BLEND_OP_SUBTRACT = D3D12_BLEND_OP_SUBTRACT,
  BLEND_OP_REVERSE_SUBTRACT = D3D12_BLEND_OP_REV_SUBTRACT,
  BLEND_OP_MIN = D3D12_BLEND_OP_MIN,
  BLEND_OP_MAX = D3D12_BLEND_OP_MAX
};

enum CommandBufferLevel {
  COMMAND_BUFFER_LEVEL_PRIMARY = D3D12_COMMAND_LIST_TYPE_DIRECT,
  COMMAND_BUFFER_LEVEL_SECONDARY = D3D12_COMMAND_LIST_TYPE_BUNDLE
};

enum FrontFace { FRONT_FACE_COUNTER_CLOCKWISE, FRONT_FACE_CLOCKWISE };
enum TextureType {
	TEXTURE_TYPE_2D = D3D12_SRV_DIMENSION_TEXTURE2D,
	TEXTURE_TYPE_2DMS = D3D12_SRV_DIMENSION_TEXTURE2DMS,
	TEXTURE_TYPE_3D = D3D12_SRV_DIMENSION_TEXTURE3D,
	TEXTURE_TYPE_CUBE = D3D12_SRV_DIMENSION_TEXTURECUBE,
	TEXTURE_TYPE_2D_ARRAY = D3D12_SRV_DIMENSION_TEXTURE2DARRAY,
	TEXTURE_TYPE_2DMS_ARRAY = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY
};

enum PipelineStageFlagBits { PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
enum ShaderStageFlagBits {
  SHADER_STAGE_VERTEX_BIT = 1,
  SHADER_STAGE_TESSELLATION_CONTROL_BIT = 2,
  SHADER_STAGE_TESSELLATION_EVALUATION_BIT = 4,
  SHADER_STAGE_GEOMETRY_BIT = 8,
  SHADER_STAGE_FRAGMENT_BIT = 16,
  SHADER_STAGE_COMPUTE_BIT = 32,
  SHADER_STAGE_ALL_GRAPHICS = 64,
  SHADER_STAGE_ALL = 0xFF
};
/*
#define DEFINE_PIXELFORMATS(s, t0, t1)                                         \
  PIXELFORMAT_R##s##_##t0 = DXGI_FORMAT_R##s##_##t1,                           \
  PIXELFORMAT_RG##s##_##t0 = DXGI_FORMAT_R##s##G##s##_##t1,                    \
  PIXELFORMAT_RGBA##s##_##t0 = DXGI_FORMAT_R##s##G##s##B##s##A##s##_##t1

enum PixelFormat {
  PIXELFORMAT_UNDEFINED = DXGI_FORMAT_UNKNOWN,
  DEFINE_PIXELFORMATS(8, UNORM, UNORM),
  DEFINE_PIXELFORMATS(16, UINT, UINT),
  DEFINE_PIXELFORMATS(16, SFLOAT, FLOAT),
  DEFINE_PIXELFORMATS(32, UINT, UINT),
  DEFINE_PIXELFORMATS(32, SFLOAT, FLOAT),
  PIXELFORMAT_BGRA8_UNORM = DXGI_FORMAT_B8G8R8A8_UNORM,
  PIXELFORMAT_D16_UNORM = DXGI_FORMAT_D16_UNORM,
  PIXELFORMAT_D24_UNORM = DXGI_FORMAT_D24_UNORM_S8_UINT,
  PIXELFORMAT_D24_UNORM_S8_UINT = DXGI_FORMAT_D24_UNORM_S8_UINT,
  PIXELFORMAT_D32_SFLOAT = DXGI_FORMAT_D32_FLOAT,
  PIXELFORMAT_D32_SFLOAT_S8_UINT = DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
  PIXELFORMAT_NV12 = DXGI_FORMAT_NV12
};*/

enum IndexFormat {
  INDEXFORMAT_UINT16 = DXGI_FORMAT_R16_UINT,
  INDEXFORMAT_UINT32 = DXGI_FORMAT_R32_UINT
};

#define DEFINE_VERTEXFORMATS(s, t0, t1)                                        \
  VERTEXFORMAT_##t0 = DXGI_FORMAT_R##s##_##t1,                                 \
  VERTEXFORMAT_##t0##2 = DXGI_FORMAT_R##s##G##s##_##t1,                        \
  VERTEXFORMAT_##t0##3 = DXGI_FORMAT_R##s##G##s##B##s##_##t1,                  \
  VERTEXFORMAT_##t0##4 = DXGI_FORMAT_R##s##G##s##B##s##A##s##_##t1

enum VertexFormat { DEFINE_VERTEXFORMATS(32, FLOAT, FLOAT) };

enum DescriptorType {
  DESCRIPTOR_TYPE_SAMPLER,
  DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
  DESCRIPTOR_TYPE_SAMPLED_IMAGE,
  DESCRIPTOR_TYPE_STORAGE_IMAGE,
  DESCRIPTOR_TYPE_UNIFORM_BUFFER,
  DESCRIPTOR_TYPE_STORAGE_BUFFER
};
enum VertexInputRate {
  VERTEX_INPUT_RATE_VERTEX = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
  VERTEX_INPUT_RATE_INSTANCE = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA
};

enum FenceCreateFlagBits {
	FENCE_SIGNALED_BIT = 1
};

enum ImageLayout {
  IMAGE_LAYOUT_UNDEFINED,
  IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  IMAGE_LAYOUT_GENERAL,
  IMAGE_LAYOUT_PRESENT_SRC,
  IMAGE_LAYOUT_UNORDERED_ACCESS
};

} // namespace ngli
