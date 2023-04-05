/*
 * Copyright 2021 GoPro Inc.
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
class D3DCommandList;
}

class swapchain_util_d3d12
{
  public:
    static swapchain_util_d3d12* newInstance(ngli::D3DGraphicsContext *ctx, uintptr_t window);

    swapchain_util_d3d12(ngli::D3DGraphicsContext *ctx, uintptr_t window);

    virtual ~swapchain_util_d3d12() {}

    virtual void acquire_image();
    virtual void present(ngli::D3DCommandList* cmd_buffer);

    ngli::D3DGraphicsContext *ctx = nullptr;
    uintptr_t window           = 0;
};

