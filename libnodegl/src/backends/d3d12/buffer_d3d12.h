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

#pragma once


#include "buffer.h"

namespace ngli
{
    class D3DBuffer;
}

struct buffer_d3d12 {
    struct buffer parent;
    ngli::D3DBuffer* mBuffer=0;
};

struct buffer *d3d12_buffer_create(struct gpu_ctx *gpu_ctx);
int d3d12_buffer_init(struct buffer *s, int size, int usage);
int d3d12_buffer_upload(struct buffer *s, const void *data, int size, int offset);
int d3d12_buffer_map(struct buffer *s, int size, int offset, void **data);
void d3d12_buffer_unmap(struct buffer *s);
void d3d12_buffer_freep(struct buffer **sp);