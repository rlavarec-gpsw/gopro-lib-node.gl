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

#ifndef DEFINES_H
#define DEFINES_H

#ifdef __cplusplus
extern "C" {
#include "graphicstate.h"
#include "rendertarget.h"
#include "texture.h"
#include "topology.h"
}

//#include "utils_mtl.h"
#include "Metal.hpp"

const MTL::PrimitiveType mtl_topology_map[NGLI_PRIMITIVE_TOPOLOGY_NB] = {
    [NGLI_PRIMITIVE_TOPOLOGY_POINT_LIST] = MTL::PrimitiveTypePoint,
    [NGLI_PRIMITIVE_TOPOLOGY_LINE_LIST] = MTL::PrimitiveTypeLine,
    [NGLI_PRIMITIVE_TOPOLOGY_LINE_STRIP] = MTL::PrimitiveTypeLineStrip,
    [NGLI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST] = MTL::PrimitiveTypeTriangle,
    [NGLI_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP] = MTL::PrimitiveTypeTriangleStrip
};

const MTL::BlendFactor mtl_blend_factor_map[NGLI_BLEND_FACTOR_NB] = {
    [NGLI_BLEND_FACTOR_ZERO] = MTL::BlendFactorZero,
    [NGLI_BLEND_FACTOR_ONE] = MTL::BlendFactorOne,
    [NGLI_BLEND_FACTOR_SRC_COLOR] = MTL::BlendFactorSourceColor,
    [NGLI_BLEND_FACTOR_ONE_MINUS_SRC_COLOR] = MTL::BlendFactorOneMinusSourceColor,
    [NGLI_BLEND_FACTOR_DST_COLOR] = MTL::BlendFactorDestinationColor,
    [NGLI_BLEND_FACTOR_ONE_MINUS_DST_COLOR] = MTL::BlendFactorOneMinusDestinationColor,
    [NGLI_BLEND_FACTOR_SRC_ALPHA] = MTL::BlendFactorSourceAlpha,
    [NGLI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA] = MTL::BlendFactorOneMinusSourceAlpha,
    [NGLI_BLEND_FACTOR_DST_ALPHA] = MTL::BlendFactorDestinationAlpha,
    [NGLI_BLEND_FACTOR_ONE_MINUS_DST_ALPHA] = MTL::BlendFactorOneMinusDestinationAlpha
};

const MTL::BlendOperation mtl_blend_op_map[NGLI_BLEND_OP_NB] = {
    [NGLI_BLEND_OP_ADD] = MTL::BlendOperationAdd,
    [NGLI_BLEND_OP_SUBTRACT] = MTL::BlendOperationSubtract,
    [NGLI_BLEND_OP_REVERSE_SUBTRACT] = MTL::BlendOperationReverseSubtract,
    [NGLI_BLEND_OP_MIN] = MTL::BlendOperationMin,
    [NGLI_BLEND_OP_MAX] = MTL::BlendOperationMax
};

const MTL::CompareFunction mtl_compare_function_map[NGLI_COMPARE_OP_NB] = {
    [NGLI_COMPARE_OP_NEVER] = MTL::CompareFunctionNever,
    [NGLI_COMPARE_OP_LESS] = MTL::CompareFunctionLess,
    [NGLI_COMPARE_OP_EQUAL] = MTL::CompareFunctionEqual,
    [NGLI_COMPARE_OP_LESS_OR_EQUAL] = MTL::CompareFunctionLessEqual,
    [NGLI_COMPARE_OP_GREATER] = MTL::CompareFunctionGreater,
    [NGLI_COMPARE_OP_NOT_EQUAL] = MTL::CompareFunctionNotEqual,
    [NGLI_COMPARE_OP_GREATER_OR_EQUAL] = MTL::CompareFunctionGreaterEqual,
    [NGLI_COMPARE_OP_ALWAYS] = MTL::CompareFunctionAlways
};

const MTL::ColorWriteMask mtl_color_write_mask_map[16] = {
    [NGLI_COLOR_COMPONENT_R_BIT] = MTL::ColorWriteMaskRed,
    [NGLI_COLOR_COMPONENT_G_BIT] = MTL::ColorWriteMaskGreen,
    [NGLI_COLOR_COMPONENT_B_BIT] = MTL::ColorWriteMaskBlue,
    [NGLI_COLOR_COMPONENT_A_BIT] = MTL::ColorWriteMaskAlpha,
};

const MTL:: CullMode mtl_cull_mode_map[NGLI_CULL_MODE_NB] = {
    [NGLI_CULL_MODE_NONE] = MTL::CullModeNone,
    [NGLI_CULL_MODE_FRONT_BIT] = MTL::CullModeFront,
    [NGLI_CULL_MODE_BACK_BIT] = MTL::CullModeBack
};

const MTL::StencilOperation mtl_stencil_operation_map[NGLI_STENCIL_OP_NB] =  {
    [NGLI_STENCIL_OP_KEEP] = MTL::StencilOperationKeep,
    [NGLI_STENCIL_OP_ZERO] = MTL::StencilOperationZero,
    [NGLI_STENCIL_OP_REPLACE] = MTL::StencilOperationReplace,
    [NGLI_STENCIL_OP_INCREMENT_AND_CLAMP] = MTL::StencilOperationIncrementClamp,
    [NGLI_STENCIL_OP_DECREMENT_AND_CLAMP] = MTL::StencilOperationDecrementClamp,
    [NGLI_STENCIL_OP_INVERT] = MTL::StencilOperationInvert,
    [NGLI_STENCIL_OP_INCREMENT_AND_WRAP] = MTL::StencilOperationIncrementWrap,
    [NGLI_STENCIL_OP_DECREMENT_AND_WRAP] = MTL::StencilOperationDecrementWrap
};

const MTL::TextureType mtl_texture_type_map[NGLI_TEXTURE_TYPE_NB] = {
    [NGLI_TEXTURE_TYPE_2D] = MTL::TextureType2D,
    [NGLI_TEXTURE_TYPE_3D] = MTL::TextureType3D,
    [NGLI_TEXTURE_TYPE_CUBE] = MTL::TextureTypeCube
};

/*enum MTL::FilterMode {
    [NGLI_MIPMAP_FILTER_NONE = MTL::SamplerMipFilterNotMipmapped,
    [NGLI_MIPMAP_FILTER_NEAREST = MTL::SamplerMipFilterNearest,
    [NGLI_MIPMAP_FILTER_LINEAR = MTL::SamplerMipFilterLinear,
    [NGLI_FILTER_NEAREST = MTL::SamplerMinMagFilterNearest,
    [NGLI_FILTER_LINEAR = MTL::SamplerMinMagFilterLinear
};*/

const MTL::SamplerAddressMode mtl_sampler_address_mode_map[NGLI_NB_WRAP] = {
    [NGLI_WRAP_CLAMP_TO_EDGE] = MTL::SamplerAddressModeClampToEdge,
    [NGLI_WRAP_MIRRORED_REPEAT] = MTL::SamplerAddressModeMirrorRepeat,
    [NGLI_WRAP_REPEAT] = MTL::SamplerAddressModeRepeat
};

const MTL::LoadAction mtl_load_action_map[3] = {
    [NGLI_LOAD_OP_DONT_CARE] = MTL::LoadActionDontCare,
    [NGLI_LOAD_OP_LOAD] = MTL::LoadActionLoad,
    [NGLI_LOAD_OP_CLEAR] = MTL::LoadActionClear
};

/*const MTL::StoreAction mtl_store_action_map[NGLI_STORE_OP_NB] = {
    [NGLI_STORE_OP_DONT_CARE] = MTL::StoreActionDontCare,
    [NGLI_STORE_OP_STORE] = MTL::StoreActionStore,
    [NGLI_STORE_OP_MULTISAMPLE_RESOLVE] = MTL::StoreActionMultisampleResolve,
    [NGLI_STORE_OP_STORE_AND_MULTISAMPLE_RESOLVE] = MTL::StoreActionStoreAndMultisampleResolve
};*/
#endif // __cplusplus
#endif    
