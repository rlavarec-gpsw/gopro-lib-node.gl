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

#ifndef BUFFER_MTL_H
#define BUFFER_MTL_H

extern "C" {
#include "buffer.h"
}

#include "Metal.hpp"

#include "defines_mtl.h"

struct buffer_mtl {
    struct buffer parent;
    NS::SharedPtr<MTL::Buffer> buffer;
};

struct gctx;

struct buffer *ngli_buffer_mtl_create(struct gctx *gctx);
int ngli_buffer_mtl_init(struct buffer *s, int size, int usage);
int ngli_buffer_mtl_upload(struct buffer *s, const void *data, int size, int offset);
int ngli_buffer_mtl_map(struct buffer *s, int size, int offset, void **data);
int ngli_buffer_mtl_unmap(struct buffer *s);
void ngli_buffer_mtl_freep(struct buffer **sp);
#endif
