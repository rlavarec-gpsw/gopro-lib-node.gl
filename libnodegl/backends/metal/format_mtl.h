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

#ifndef FORMAT_MTL_H
#define FORMAT_MTL_H

#ifdef __cplusplus
extern "C" {
#include "format.h"
}

#include "Metal.hpp"

const MTL::PixelFormat mtl_format_map[NGLI_FORMAT_NB] = {
    [NGLI_FORMAT_UNDEFINED]           = MTL::PixelFormatInvalid,
    /*  8 bit formats */
    [NGLI_FORMAT_R8_UNORM]            = MTL::PixelFormatR8Unorm,
    [NGLI_FORMAT_R8_SNORM]            = MTL::PixelFormatR8Snorm,
    [NGLI_FORMAT_R8_UINT]             = MTL::PixelFormatR8Uint,
    [NGLI_FORMAT_R8_SINT]             = MTL::PixelFormatR8Sint,
    /* 16 bit formats */
    [NGLI_FORMAT_R8G8_UNORM]          = MTL::PixelFormatRG8Unorm,
    [NGLI_FORMAT_R8G8_SNORM]          = MTL::PixelFormatRG8Snorm,
    [NGLI_FORMAT_R8G8_UINT]           = MTL::PixelFormatRG8Uint,
    [NGLI_FORMAT_R8G8_SINT]           = MTL::PixelFormatRG8Sint,

    [NGLI_FORMAT_R16_UINT]            = MTL::PixelFormatR16Uint,
    [NGLI_FORMAT_R16_SINT]            = MTL::PixelFormatR16Sint,
    [NGLI_FORMAT_R16_SFLOAT]          = MTL::PixelFormatR16Float,
    /* 32 bit formats */
    [NGLI_FORMAT_R8G8B8A8_UNORM]      = MTL::PixelFormatRGBA8Unorm,
    [NGLI_FORMAT_R8G8B8A8_SNORM]      = MTL::PixelFormatRGBA8Snorm,
    [NGLI_FORMAT_R8G8B8A8_UINT]       = MTL::PixelFormatRGBA8Uint,
    [NGLI_FORMAT_R8G8B8A8_SINT]       = MTL::PixelFormatRGBA8Sint,

    [NGLI_FORMAT_R32_UINT]            = MTL::PixelFormatR32Uint,
    [NGLI_FORMAT_R32_SINT]            = MTL::PixelFormatR32Sint,
    [NGLI_FORMAT_R32_SFLOAT]          = MTL::PixelFormatR32Float,
    /* 64 bit formats */
    [NGLI_FORMAT_R16G16B16A16_UNORM]  = MTL::PixelFormatRGBA16Unorm,
    [NGLI_FORMAT_R16G16B16A16_SNORM]  = MTL::PixelFormatRGBA16Snorm,
    [NGLI_FORMAT_R16G16B16A16_UINT]   = MTL::PixelFormatRGBA16Uint,
    [NGLI_FORMAT_R16G16B16A16_SINT]   = MTL::PixelFormatRGBA16Sint,
    [NGLI_FORMAT_R16G16B16A16_SFLOAT] = MTL::PixelFormatRGBA16Float,

    [NGLI_FORMAT_R32G32_UINT]         = MTL::PixelFormatRG32Uint,
    [NGLI_FORMAT_R32G32_SINT]         = MTL::PixelFormatRG32Sint,
    [NGLI_FORMAT_R32G32_SFLOAT]       = MTL::PixelFormatRG32Float,
    /* 128 bit formats */
    [NGLI_FORMAT_R32G32B32A32_UINT]   = MTL::PixelFormatRGBA32Uint,
    [NGLI_FORMAT_R32G32B32A32_SINT]   = MTL::PixelFormatRGBA32Sint,
    [NGLI_FORMAT_R32G32B32A32_SFLOAT] = MTL::PixelFormatRGBA32Float,
    /* depth and stencil formats */
    [NGLI_FORMAT_D16_UNORM]           = MTL::PixelFormatDepth16Unorm,
    [NGLI_FORMAT_D24_UNORM_S8_UINT]   = MTL::PixelFormatDepth24Unorm_Stencil8,
    [NGLI_FORMAT_D32_SFLOAT]          = MTL::PixelFormatDepth32Float,
    [NGLI_FORMAT_D32_SFLOAT_S8_UINT] = MTL::PixelFormatDepth32Float_Stencil8, 
    [NGLI_FORMAT_S8_UINT] = MTL::PixelFormatStencil8
};
#endif // __cplusplus
#endif
