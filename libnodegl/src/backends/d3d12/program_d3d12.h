/*
 * Copyright 2018 GoPro Inc.
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

#include <backends/d3d12/impl/D3DShaderModule.h>
#include "program.h"
struct gpu_ctx;

struct program_d3d12
{
    struct program parent;
    ngli::D3DVertexShaderModule* vs = nullptr;
    ngli::D3DFragmentShaderModule* fs = nullptr;
    ngli::D3DComputeShaderModule* cs = nullptr;
};

struct program* ngli_program_d3d12_create(struct gpu_ctx* gpu_ctx);
int ngli_program_d3d12_init(struct program* s, const program_params* p);
void ngli_program_d3d12_freep(struct program** sp);
