/*
 * Copyright 2018-2022 GoPro Inc.
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

#include <stdafx.h>
#include "format_d3d12.h"
#include <format.h>


DXGI_FORMAT to_d3d12_format(int format)
{
    static std::vector<DXGI_FORMAT> format_map;
    struct InitFormatMap
    {
        InitFormatMap()
        {
            format_map.resize(NGLI_FORMAT_NB);
            format_map[NGLI_FORMAT_UNDEFINED] = DXGI_FORMAT_UNKNOWN;
            format_map[NGLI_FORMAT_R8_UNORM] = DXGI_FORMAT_R8_UNORM;
            format_map[NGLI_FORMAT_R8_SNORM] = DXGI_FORMAT_R8_SNORM;
            format_map[NGLI_FORMAT_R8_UINT] = DXGI_FORMAT_R8_UINT;
            format_map[NGLI_FORMAT_R8_SINT] = DXGI_FORMAT_R8_SINT;
            format_map[NGLI_FORMAT_R8G8_UNORM] = DXGI_FORMAT_R8G8_UNORM;
            format_map[NGLI_FORMAT_R8G8_SNORM] = DXGI_FORMAT_R8G8_SNORM;
            format_map[NGLI_FORMAT_R8G8_UINT] = DXGI_FORMAT_R8G8_UINT;
            format_map[NGLI_FORMAT_R8G8_SINT] = DXGI_FORMAT_R8G8_SINT;
            format_map[NGLI_FORMAT_R8G8B8_UNORM] = DXGI_FORMAT_UNKNOWN;         // R8G8B8 Doesn't exist in d3d12
            format_map[NGLI_FORMAT_R8G8B8_SNORM] = DXGI_FORMAT_UNKNOWN;         // R8G8B8 Doesn't exist in d3d12
            format_map[NGLI_FORMAT_R8G8B8_UINT] = DXGI_FORMAT_UNKNOWN;         // R8G8B8 Doesn't exist in d3d12
            format_map[NGLI_FORMAT_R8G8B8_SINT] = DXGI_FORMAT_UNKNOWN;         // R8G8B8 Doesn't exist in d3d12
            format_map[NGLI_FORMAT_R8G8B8_SRGB] = DXGI_FORMAT_UNKNOWN;   // R8G8B8 Doesn't exist in d3d12
            format_map[NGLI_FORMAT_R8G8B8A8_UNORM] = DXGI_FORMAT_R8G8B8A8_UNORM;
            format_map[NGLI_FORMAT_R8G8B8A8_SNORM] = DXGI_FORMAT_R8G8B8A8_SNORM;
            format_map[NGLI_FORMAT_R8G8B8A8_UINT] = DXGI_FORMAT_R8G8B8A8_UINT;
            format_map[NGLI_FORMAT_R8G8B8A8_SINT] = DXGI_FORMAT_R8G8B8A8_SINT;
            format_map[NGLI_FORMAT_R8G8B8A8_SRGB] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            format_map[NGLI_FORMAT_B8G8R8A8_UNORM] = DXGI_FORMAT_B8G8R8A8_UNORM;
            format_map[NGLI_FORMAT_B8G8R8A8_SNORM] = DXGI_FORMAT_B8G8R8A8_TYPELESS;     // Compatible format
            format_map[NGLI_FORMAT_B8G8R8A8_UINT] = DXGI_FORMAT_B8G8R8A8_TYPELESS;     // Compatible format
            format_map[NGLI_FORMAT_B8G8R8A8_SINT] = DXGI_FORMAT_B8G8R8A8_TYPELESS;     // Compatible format
            format_map[NGLI_FORMAT_R16_UNORM] = DXGI_FORMAT_R16_UNORM;
            format_map[NGLI_FORMAT_R16_SNORM] = DXGI_FORMAT_R16_SNORM;
            format_map[NGLI_FORMAT_R16_UINT] = DXGI_FORMAT_R16_UINT;
            format_map[NGLI_FORMAT_R16_SINT] = DXGI_FORMAT_R16_SINT;
            format_map[NGLI_FORMAT_R16_SFLOAT] = DXGI_FORMAT_R16_FLOAT;
            format_map[NGLI_FORMAT_R16G16_UNORM] = DXGI_FORMAT_R16G16_UNORM;
            format_map[NGLI_FORMAT_R16G16_SNORM] = DXGI_FORMAT_R16G16_SNORM;
            format_map[NGLI_FORMAT_R16G16_UINT] = DXGI_FORMAT_R16G16_UINT;
            format_map[NGLI_FORMAT_R16G16_SINT] = DXGI_FORMAT_R16G16_SINT;
            format_map[NGLI_FORMAT_R16G16_SFLOAT] = DXGI_FORMAT_R16G16_FLOAT;
            format_map[NGLI_FORMAT_R16G16B16_UNORM] = DXGI_FORMAT_UNKNOWN;     // R16G16B16 Doesn't exist in d3d12
            format_map[NGLI_FORMAT_R16G16B16_SNORM] = DXGI_FORMAT_UNKNOWN;     // R16G16B16 Doesn't exist in d3d12
            format_map[NGLI_FORMAT_R16G16B16_UINT] = DXGI_FORMAT_UNKNOWN;     // R16G16B16 Doesn't exist in d3d12
            format_map[NGLI_FORMAT_R16G16B16_SINT] = DXGI_FORMAT_UNKNOWN;     // R16G16B16 Doesn't exist in d3d12
            format_map[NGLI_FORMAT_R16G16B16_SFLOAT] = DXGI_FORMAT_UNKNOWN;     // R16G16B16 Doesn't exist in d3d12
            format_map[NGLI_FORMAT_R16G16B16A16_UNORM] = DXGI_FORMAT_R16G16B16A16_UNORM;
            format_map[NGLI_FORMAT_R16G16B16A16_SNORM] = DXGI_FORMAT_R16G16B16A16_SNORM;
            format_map[NGLI_FORMAT_R16G16B16A16_UINT] = DXGI_FORMAT_R16G16B16A16_UINT;
            format_map[NGLI_FORMAT_R16G16B16A16_SINT] = DXGI_FORMAT_R16G16B16A16_SINT;
            format_map[NGLI_FORMAT_R16G16B16A16_SFLOAT] = DXGI_FORMAT_R16G16B16A16_FLOAT;
            format_map[NGLI_FORMAT_R32_UINT] = DXGI_FORMAT_R32_UINT;
            format_map[NGLI_FORMAT_R32_SINT] = DXGI_FORMAT_R32_SINT;
            format_map[NGLI_FORMAT_R32_SFLOAT] = DXGI_FORMAT_R32_FLOAT;
            format_map[NGLI_FORMAT_R32G32_UINT] = DXGI_FORMAT_R32G32_UINT;
            format_map[NGLI_FORMAT_R32G32_SINT] = DXGI_FORMAT_R32G32_SINT;
            format_map[NGLI_FORMAT_R32G32_SFLOAT] = DXGI_FORMAT_R32G32_FLOAT;
            format_map[NGLI_FORMAT_R32G32B32_UINT] = DXGI_FORMAT_R32G32B32_UINT;
            format_map[NGLI_FORMAT_R32G32B32_SINT] = DXGI_FORMAT_R32G32B32_SINT;
            format_map[NGLI_FORMAT_R32G32B32_SFLOAT] = DXGI_FORMAT_R32G32B32_FLOAT;
            format_map[NGLI_FORMAT_R32G32B32A32_UINT] = DXGI_FORMAT_R32G32B32A32_UINT;
            format_map[NGLI_FORMAT_R32G32B32A32_SINT] = DXGI_FORMAT_R32G32B32A32_SINT;
            format_map[NGLI_FORMAT_R32G32B32A32_SFLOAT] = DXGI_FORMAT_R32G32B32A32_FLOAT;
            format_map[NGLI_FORMAT_D16_UNORM] = DXGI_FORMAT_D16_UNORM;
            format_map[NGLI_FORMAT_X8_D24_UNORM_PACK32] = DXGI_FORMAT_D24_UNORM_S8_UINT;     // Compatible format
            format_map[NGLI_FORMAT_D32_SFLOAT] = DXGI_FORMAT_D32_FLOAT;
            format_map[NGLI_FORMAT_D24_UNORM_S8_UINT] = DXGI_FORMAT_D24_UNORM_S8_UINT;
            format_map[NGLI_FORMAT_D32_SFLOAT_S8_UINT] = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            format_map[NGLI_FORMAT_S8_UINT] = DXGI_FORMAT_UNKNOWN;               // Doesn't exist -> DXGI_FORMAT_S8_UINT;
        }
    };
    static InitFormatMap gInitFormatMap;

    ngli_assert(format >= 0 && format < format_map.size());
    const DXGI_FORMAT ret = format_map[format];
    ngli_assert(format == NGLI_FORMAT_UNDEFINED || ret);
    return ret;
}

int to_ngli_format(DXGI_FORMAT format)
{
    switch(format)
    {
        case DXGI_FORMAT_R8_UNORM:            return NGLI_FORMAT_R8_UNORM;
        case DXGI_FORMAT_R8_SNORM:            return NGLI_FORMAT_R8_SNORM;
        case DXGI_FORMAT_R8_UINT:             return NGLI_FORMAT_R8_UINT;
        case DXGI_FORMAT_R8_SINT:             return NGLI_FORMAT_R8_SINT;
        case DXGI_FORMAT_R8G8_UNORM:          return NGLI_FORMAT_R8G8_UNORM;
        case DXGI_FORMAT_R8G8_SNORM:          return NGLI_FORMAT_R8G8_SNORM;
        case DXGI_FORMAT_R8G8_UINT:           return NGLI_FORMAT_R8G8_UINT;
        case DXGI_FORMAT_R8G8_SINT:           return NGLI_FORMAT_R8G8_SINT;
        //case DXGI_FORMAT_R8G8B8A8_UNORM:      return NGLI_FORMAT_R8G8B8_UNORM;          // R8G8B8 Doesn't exist in d3d12
        //case DXGI_FORMAT_R8G8B8A8_SNORM:      return NGLI_FORMAT_R8G8B8_SNORM;          // R8G8B8 Doesn't exist in d3d12
        //case DXGI_FORMAT_R8G8B8A8_UINT:       return NGLI_FORMAT_R8G8B8_UINT;           // R8G8B8 Doesn't exist in d3d12
        //case DXGI_FORMAT_R8G8B8A8_SINT:       return NGLI_FORMAT_R8G8B8_SINT;           // R8G8B8 Doesn't exist in d3d12
        //case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return NGLI_FORMAT_R8G8B8_SRGB;           // R8G8B8 Doesn't exist in d3d12
        case DXGI_FORMAT_R8G8B8A8_UNORM:      return NGLI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_SNORM:      return NGLI_FORMAT_R8G8B8A8_SNORM;
        case DXGI_FORMAT_R8G8B8A8_UINT:       return NGLI_FORMAT_R8G8B8A8_UINT;
        case DXGI_FORMAT_R8G8B8A8_SINT:       return NGLI_FORMAT_R8G8B8A8_SINT;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return NGLI_FORMAT_R8G8B8A8_SRGB;
        case DXGI_FORMAT_B8G8R8A8_UNORM:      return NGLI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:   return NGLI_FORMAT_B8G8R8A8_SNORM;
//            case DXGI_FORMAT_B8G8R8A8_TYPELESS:   return NGLI_FORMAT_B8G8R8A8_UINT;
//            case DXGI_FORMAT_B8G8R8A8_TYPELESS:   return NGLI_FORMAT_B8G8R8A8_SINT;
        case DXGI_FORMAT_R16_UNORM:           return NGLI_FORMAT_R16_UNORM;
        case DXGI_FORMAT_R16_SNORM:           return NGLI_FORMAT_R16_SNORM;
        case DXGI_FORMAT_R16_UINT:            return NGLI_FORMAT_R16_UINT;
        case DXGI_FORMAT_R16_SINT:            return NGLI_FORMAT_R16_SINT;
        case DXGI_FORMAT_R16_FLOAT:           return NGLI_FORMAT_R16_SFLOAT;
        case DXGI_FORMAT_R16G16_UNORM:        return NGLI_FORMAT_R16G16_UNORM;
        case DXGI_FORMAT_R16G16_SNORM:        return NGLI_FORMAT_R16G16_SNORM;
        case DXGI_FORMAT_R16G16_UINT:         return NGLI_FORMAT_R16G16_UINT;
        case DXGI_FORMAT_R16G16_SINT:         return NGLI_FORMAT_R16G16_SINT;
        case DXGI_FORMAT_R16G16_FLOAT:        return NGLI_FORMAT_R16G16_SFLOAT;
        //case DXGI_FORMAT_R16G16B16A16_UNORM:  return NGLI_FORMAT_R16G16B16_UNORM;     // R16G16B16 Doesn't exist in d3d12
        //case DXGI_FORMAT_R16G16B16A16_SNORM:  return NGLI_FORMAT_R16G16B16_SNORM;     // R16G16B16 Doesn't exist in d3d12
        //case DXGI_FORMAT_R16G16B16A16_UINT:   return NGLI_FORMAT_R16G16B16_UINT;      // R16G16B16 Doesn't exist in d3d12
        //case DXGI_FORMAT_R16G16B16A16_SINT:   return NGLI_FORMAT_R16G16B16_SINT;      // R16G16B16 Doesn't exist in d3d12
        //case DXGI_FORMAT_R16G16B16A16_FLOAT:  return NGLI_FORMAT_R16G16B16_SFLOAT;    // R16G16B16 Doesn't exist in d3d12
        case DXGI_FORMAT_R16G16B16A16_UNORM:  return NGLI_FORMAT_R16G16B16A16_UNORM;
        case DXGI_FORMAT_R16G16B16A16_SNORM:  return NGLI_FORMAT_R16G16B16A16_SNORM;
        case DXGI_FORMAT_R16G16B16A16_UINT:   return NGLI_FORMAT_R16G16B16A16_UINT;
        case DXGI_FORMAT_R16G16B16A16_SINT:   return NGLI_FORMAT_R16G16B16A16_SINT;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:  return NGLI_FORMAT_R16G16B16A16_SFLOAT;
        case DXGI_FORMAT_R32_UINT:            return NGLI_FORMAT_R32_UINT;
        case DXGI_FORMAT_R32_SINT:            return NGLI_FORMAT_R32_SINT;
        case DXGI_FORMAT_R32_FLOAT:           return NGLI_FORMAT_R32_SFLOAT;
        case DXGI_FORMAT_R32G32_UINT:         return NGLI_FORMAT_R32G32_UINT;
        case DXGI_FORMAT_R32G32_SINT:         return NGLI_FORMAT_R32G32_SINT;
        case DXGI_FORMAT_R32G32_FLOAT:        return NGLI_FORMAT_R32G32_SFLOAT;
        case DXGI_FORMAT_R32G32B32_UINT:      return NGLI_FORMAT_R32G32B32_UINT;
        case DXGI_FORMAT_R32G32B32_SINT:      return NGLI_FORMAT_R32G32B32_SINT;
        case DXGI_FORMAT_R32G32B32_FLOAT:     return NGLI_FORMAT_R32G32B32_SFLOAT;
        case DXGI_FORMAT_R32G32B32A32_UINT:   return NGLI_FORMAT_R32G32B32A32_UINT;
        case DXGI_FORMAT_R32G32B32A32_SINT:   return NGLI_FORMAT_R32G32B32A32_SINT;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:  return NGLI_FORMAT_R32G32B32A32_SFLOAT;
        case DXGI_FORMAT_D16_UNORM:           return NGLI_FORMAT_D16_UNORM;
//        case DXGI_FORMAT_D24_UNORM_S8_UINT:   return NGLI_FORMAT_X8_D24_UNORM_PACK32;    // Compatible format
        case DXGI_FORMAT_D32_FLOAT:           return NGLI_FORMAT_D32_SFLOAT;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:   return NGLI_FORMAT_D24_UNORM_S8_UINT;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:return NGLI_FORMAT_D32_SFLOAT_S8_UINT;
    //    case DXGI_FORMAT_S8_UINT:             return NGLI_FORMAT_S8_UINT;              // Doesn't exist -> DXGI_FORMAT_S8_UINT,
        default:                              ngli_assert(0);
    }
}
