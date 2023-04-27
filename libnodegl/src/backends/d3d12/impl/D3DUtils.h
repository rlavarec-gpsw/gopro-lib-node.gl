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
#include <log.h>
#include <config.h>
}

using Microsoft::WRL::ComPtr;

namespace ngli
{
typedef uint32_t Flags;
typedef Flags PipelineStageFlags;
typedef Flags ShaderStageFlags;
typedef Flags FenceCreateFlags;
typedef Flags ImageUsageFlags;
typedef Flags ColorComponentFlags;
typedef Flags BufferUsageFlags;
struct Rect2D
{
    int32_t x, y;
    uint32_t w, h;
};
} // namespace ngfli


#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

#define NGLI_ERR(fmt, ...) \
{ \
    char buffer[4096]; \
    snprintf(buffer, sizeof(buffer), "ERROR: [%s][%s][%d] " fmt "\n", __FILE__, \
        __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
    LOG(ERROR, fmt, ##__VA_ARGS__); \
    throw std::runtime_error(buffer); \
}

#define NGLI_TODO(fmt, ...) LOG(WARNING, " TODO: " fmt,##__VA_ARGS__)
  
           

struct DebugUtil
{
    static inline void Exit(uint32_t code)
    {
        exit(code);
    };
};


/** Trace all Direct3D calls to log output */
const bool D3D_ENABLE_TRACE = (DEBUG_D3D12 | DEBUG_D3D12_TRACE) > 0?true:false;
const bool ENABLE_GPU_VALIDATION = (DEBUG_D3D12 | DEBUG_D3D12_GPU_VALIDATION) > 0?true:false;
const bool DEBUG_SHADERS = DEBUG_D3D12_ACTIVATED;
// Enabling GPU validation slows down performance but it's useful for debugging

#define D3D_TRACE(func)                                                        \
  {                                                                            \
    if (D3D_ENABLE_TRACE)                                                      \
      LOG(INFO, "%s", #func);                                                   \
    func;                                                                      \
  }

#define D3D_TRACE_ARG(func, fmt, ...)                                                     \
  {                                                                            \
    if (D3D_ENABLE_TRACE)                                                      \
      LOG(INFO, "%s", #func);                                                   \
    hResult = func;                                                            \
    if (FAILED(hResult)) {                                                     \
      NGLI_ERR("%s failed: 0x%08X %s " fmt, #func, hResult,                    \
               std::system_category().message(hResult).c_str(),                \
               ##__VA_ARGS__);                                                 \
    }                                                                          \
  }

#define D3D_TRACE_CALL(func) D3D_TRACE_ARG(func, "")

inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};

inline void ThrowIfFailed(HRESULT hr)
{
    if(FAILED(hr))
    {
        throw HrException(hr);
    }
}