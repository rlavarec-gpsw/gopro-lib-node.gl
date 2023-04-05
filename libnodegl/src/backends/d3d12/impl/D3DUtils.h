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

#include <log.h>

#include <d3dx12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <system_error>

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
//#define LOG_TO_DEBUG_CONSOLE

#if defined(_WIN32) && defined(LOG_TO_DEBUG_CONSOLE)
#include <Windows.h>
inline void debugMessage(FILE* filenum, const char* fmt, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);
    OutputDebugStringA((buffer));
}
#define LOG_FN debugMessage
#else
#define LOG_FN fprintf
#endif

#define NGLI_LOG(fmt, ...) LOG_FN(stderr, fmt "\n", ##__VA_ARGS__)
#define NGLI_ERR(fmt, ...) \
{ \
    char buffer[4096]; \
    snprintf(buffer, sizeof(buffer), "ERROR: [%s][%s][%d] " fmt "\n", __FILE__, \
        __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
    LOG_FN(stderr, "%s", buffer); \
    throw std::runtime_error(buffer); \
}
#define NGLI_LOG_TRACE(fmt, ...)                                               \
  NGLI_LOG("[%s][%s][%d] " fmt, __FILE__, __PRETTY_FUNCTION__, __LINE__,       \
           ##__VA_ARGS__)

#define NGLI_TODO(fmt, ...)                                                    \
  NGLI_LOG("[%s][%s][%d] TODO: " fmt, __FILE__, __FUNCTION__, __LINE__,        \
           ##__VA_ARGS__)

struct DebugUtil
{
    static inline void Exit(uint32_t code)
    {
        exit(code);
    };
};


/** Trace all Direct3D calls to log output */
const bool D3D_ENABLE_TRACE = false;//true;
const bool DEBUG_SHADERS = true;
// Enabling GPU validation slows down performance but it's useful for debugging
const bool ENABLE_GPU_VALIDATION = false;//true;

#define D3D_TRACE(func)                                                        \
  {                                                                            \
    if (D3D_ENABLE_TRACE)                                                      \
      NGLI_LOG("%s", #func);                                                   \
    func;                                                                      \
  }

#define V0(func, fmt, ...)                                                     \
  {                                                                            \
    if (D3D_ENABLE_TRACE)                                                      \
      NGLI_LOG("%s", #func);                                                   \
    hResult = func;                                                            \
    if (FAILED(hResult)) {                                                     \
      NGLI_ERR("%s failed: 0x%08X %s " fmt, #func, hResult,                    \
               std::system_category().message(hResult).c_str(),                \
               ##__VA_ARGS__);                                                 \
    }                                                                          \
  }

#define V(func) V0(func, "")
