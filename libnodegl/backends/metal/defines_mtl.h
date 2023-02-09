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

#pragma once
#include "graphicstate.h"
#include "texture.h"
#include "topology.h"

#include "utils_mtl.h"
#include "Metal.hpp"

enum Topology {
    MTL_NGLI_PRIMITIVE_TOPOLOGY_POINT_LIST = MTL::PrimitiveTypePoint,
    MTL_NGLI_PRIMITIVE_TOPOLOGY_LINE_LIST = MTL::PrimitiveTypeLine,
    MTL_NGLI_PRIMITIVE_TOPOLOGY_LINE_STRIP = MTL::PrimitiveTypeLineStrip,
    MTL_NGLI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = MTL::PrimitiveTypeTriangle,
    MTL_NGLI_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = MTL::PrimitiveTypeTriangleStrip
};

enum BlendFactor {
    MTL_NGLI_BLEND_FACTOR_ZERO = MTL::BlendFactorZero,
    MTL_NGLI_BLEND_FACTOR_ONE = MTL::BlendFactorOne,
    MTL_NGLI_BLEND_FACTOR_SRC_COLOR = MTL::BlendFactorSourceColor,
    MTL_NGLI_BLEND_FACTOR_ONE_MINUS_SRC_COLOR = MTL::BlendFactorOneMinusSourceColor,
    MTL_NGLI_BLEND_FACTOR_DST_COLOR = MTL::BlendFactorDestinationColor,
    MTL_NGLI_BLEND_FACTOR_ONE_MINUS_DST_COLOR = MTL::BlendFactorOneMinusDestinationColor,
    MTL_NGLI_BLEND_FACTOR_SRC_ALPHA = MTL::BlendFactorSourceAlpha,
    MTL_NGLI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = MTL::BlendFactorOneMinusSourceAlpha,
    MTL_NGLI_BLEND_FACTOR_DST_ALPHA = MTL::BlendFactorDestinationAlpha,
    MTL_NGLI_BLEND_FACTOR_ONE_MINUS_DST_ALPHA = MTL::BlendFactorOneMinusDestinationAlpha
};

enum BlendOp {
    MTL_NGLI_BLEND_OP_ADD = MTL::BlendOperationAdd,
    MTL_NGLI_BLEND_OP_SUBTRACT = MTL::BlendOperationSubtract,
    MTL_NGLI_BLEND_OP_REVERSE_SUBTRACT = MTL::BlendOperationReverseSubtract,
    MTL_NGLI_BLEND_OP_MIN = MTL::BlendOperationMin,
    MTL_NGLI_BLEND_OP_MAX = MTL::BlendOperationMax
};

enum DepthOp {
    MTL_NGLI_COMPARE_OP_NEVER = MTL::CompareFunctionNever,
    MTL_NGLI_COMPARE_LESS = MTL::CompareFunctionLess,
    MTL_NGLI_COMPARE_EQUAL = MTL::CompareFunctionEqual,
    MTL_NGLI_COMPARE_OP_LESS_OR_EQUAL = MTL::CompareFunctionLessEqual,
    MTL_NGLI_COMPARE_OP_GREATER = MTL::CompareFunctionGreater,
    MTL_NGLI_COMPARE_OP_NOT_EQUAL = MTL::CompareFunctionNotEqual,
    MTL_NGLI_COMPARE_OP_GREATER_OR_EQUAL = MTL::CompareFunctionGreaterEqual,
    MTL_NGLI_COMPARE_OP_ALWAYS = MTL::CompareFunctionAlways
};

enum ColorWriteBits {
    MTL_NGLI_COLOR_COMPONENT_R_BIT = MTL::ColorWriteMaskRed,
    MTL_NGLI_COLOR_COMPONENT_G_BIT = MTL::ColorWriteMaskGreen,
    MTL_NGLI_COLOR_COMPONENT_B_BIT = MTL::ColorWriteMaskBlue,
    MTL_NGLI_COLOR_COMPONENT_A_BIT = MTL::ColorWriteMaskAlpha,
};

enum CullMode {
    MTL_NGLI_CULL_MODE_NONE = MTL::CullModeNone,
    MTL_NGLI_CULL_MODE_FRONT_BIT = MTL::CullModeFront,
    MTL_NGLI_CULL_MODE_BACK_BIT = MTL::CullModeBack
};

enum StencilOp {
    MTL_NGLI_STENCIL_OP_KEEP = MTL::StencilOperationKeep,
    MTL_NGLI_STENCIL_OP_ZERO = MTL::StencilOperationZero,
    MTL_NGLI_STENCIL_OP_REPLACE = MTL::StencilOperationReplace,
    MTL_NGLI_STENCIL_OP_INCREMENT_AND_CLAMP = MTL::StencilOperationIncrementClamp,
    MTL_NGLI_STENCIL_OP_DECREMENT_AND_CLAMP = MTL::StencilOperationDecrementClamp,
    MTL_NGLI_STENCIL_OP_INVERT = MTL::StencilOperationInvert,
    MTL_NGLI_STENCIL_OP_INCREMENT_AND_WRAP = MTL::StencilOperationIncrementWrap,
    MTL_NGLI_STENCIL_OP_DECREMENT_AND_WRAP = MTL::StencilOperationDecrementWrap
};

enum FrontFace {
    MTL_NGLI_FRONT_FACE_COUNTER_CLOCKWISE = MTL::WindingCounterClockwise,
    MTL_NGLI_FRONT_FACE_CLOCKWISE = MTL::WindingClockwise
};

enum ShaderStageFlagBits {
    SHADER_STAGE_VERTEX_BIT = 1,
    SHADER_STAGE_TESSELLATION_CONTROL_BIT = 2,
    SHADER_STAGE_TESSELLATION_EVALUATION_BIT = 4,
    SHADER_STAGE_GEOMETRY_BIT = 8,
    SHADER_STAGE_FRAGMENT_BIT = 16,
    SHADER_STAGE_COMPUTE_BIT = 32,
    SHADER_STAGE_ALL_GRAPHICS = 31
};

enum TextureType {
    MTL_NGLI_TEXTURE_TYPE_2D = MTL::TextureType2D,
    MTL_NGLI_TEXTURE_TYPE_3D = MTL::TextureType3D,
    MTL_NGLI_TEXTURE_TYPE_CUBE = MTL::TextureTypeCube
};

enum FilterMode {
    MTL_NGLI_MIPMAP_FILTER_NONE = MTL::SamplerMipFilterNotMipmapped,
    MTL_NGLI_MIPMAP_FILTER_NEAREST = MTL::SamplerMipFilterNearest,
    MTL_NGLI_MIPMAP_FILTER_LINEAR = MTL::SamplerMipFilterLinear,
    MTL_NGLI_FILTER_NEAREST = MTL::SamplerMinMagFilterNearest,
    MTL_NGLI_FILTER_LINEAR = MTL::SamplerMinMagFilterLinear
};

enum SamplerAddressMode {
    MTL_NGLI_WRAP_CLAMP_TO_EDGE = MTL::SamplerAddressModeClampToEdge,
    MTL_NGLI_WRAP_MIRROED_REPEAT = MTL::SamplerAddressModeMirrorRepeat,
    MTL_NGLI_WRAP_REPEAT = MTL::SamplerAddressModeRepeat
};

enum AttachmentLoadOp {
    MTL_NGLI_LOAD_OP_DONT_CARE = MTL::LoadActionDontCare,
    MTL_NGLI_LOAD_OP_LOAD = MTL::LoadActionLoad,
    MTL_NGLI_LOAD_OP_CLEAR = MTL::LoadActionClear
};

enum AttachmentStoreOp {
    MTL_NGLI_STORE_OP_DONT_CARE = MTL::StoreActionDontCare,
    MTL_NGLI_STORE_OP_STORE = MTL::StoreActionStore,
    MTL_NGLI_STORE_OP_MULTISAMPLE_RESOLVE = MTL::StoreActionMultisampleResolve,
    MTL_NGLI_STORE_OP_STORE_AND_MULTISAMPLE_RESOLVE = MTL::StoreActionStoreAndMultisampleResolve
};
    
