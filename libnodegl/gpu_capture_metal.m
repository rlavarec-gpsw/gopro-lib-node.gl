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

#include <Metal/Metal.h>
#include "gpu_capture.h"
#include "config.h"
#include "log.h"
#include "memory.h"
#include "utils.h"

#if defined(BACKEND_VK)
#include <MoltenVK/vk_mvk_moltenvk.h>
#include "backends/vk/gpu_ctx_vk.h"
#endif

struct gpu_capture_ctx {
    struct gpu_ctx *gpu_ctx;
};

struct gpu_capture_ctx *ngli_gpu_capture_ctx_create(struct gpu_ctx *gpu_ctx)
{
    struct gpu_capture_ctx *s = ngli_calloc(1, sizeof(*s));
    s->gpu_ctx = gpu_ctx;
    return s;
}

int ngli_gpu_capture_init(struct gpu_capture_ctx *s)
{
    return 0;
}

int ngli_gpu_capture_begin(struct gpu_capture_ctx *s)
{
    NSError *error;
    MTLCaptureManager *capture_manager = [MTLCaptureManager sharedCaptureManager];
    MTLCaptureDescriptor *capture_descriptor = [[MTLCaptureDescriptor alloc] init];
    if (!capture_descriptor)
        return NGL_ERROR_EXTERNAL;
#if defined(BACKEND_VK)
    if (s->gpu_ctx->config.backend == NGL_BACKEND_VULKAN) {
        struct gpu_ctx_vk *gpu_ctx_vk = (struct gpu_ctx_vk *)s->gpu_ctx;
        struct vkcontext *vkcontext = gpu_ctx_vk->vkcontext;
        id<MTLDevice> mtl_device;
        vkGetMTLDeviceMVK(vkcontext->phy_device, &mtl_device);
        capture_descriptor.captureObject = mtl_device;
    }
#endif
    capture_descriptor.destination = MTLCaptureDestinationDeveloperTools;
    if (![capture_manager startCaptureWithDescriptor: capture_descriptor error:&error]) {
        LOG(ERROR, "failed to start capture, error: %s", [error.description UTF8String]);
        [capture_descriptor release];
        return NGL_ERROR_EXTERNAL;
    }
    [capture_descriptor release];
    return 0;
}

int ngli_gpu_capture_end(struct gpu_capture_ctx *s)
{
    MTLCaptureManager *capture_manager = [MTLCaptureManager sharedCaptureManager];
    [capture_manager stopCapture];
    return 0;
}

void ngli_gpu_capture_freep(struct gpu_capture_ctx **sp)
{
    if (!*sp)
        return;

    ngli_freep(sp);
}
