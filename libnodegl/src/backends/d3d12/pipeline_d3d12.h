/*
 * Copyright 2019 GoPro Inc.
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
#include "pipeline.h"
}

struct gpu_ctx;
struct glcontext;

namespace ngli
{
class D3DGraphicsPipeline;
class D3DComputePipeline;
}

struct pipeline_d3d12
{
    struct pipeline parent;

    struct darray buffer_bindings;    // buffer_binding
    struct darray texture_bindings;   // texture_binding
    struct darray attribute_bindings; // attribute_binding
    int nb_unbound_attributes;

    struct darray vertex_buffers; // d3d12::Buffer*

    ngli::D3DGraphicsPipeline* gp = nullptr;
    ngli::D3DComputePipeline* cp = nullptr;
};

struct pipeline* ngli_pipeline_d3d12_create(struct gpu_ctx* gpu_ctx);
int ngli_pipeline_d3d12_init(struct pipeline* s,
                            const struct pipeline_params* params);
int ngli_pipeline_d3d12_set_resources(struct pipeline* s,
                                     const pipeline_resources* resources);
int ngli_pipeline_d3d12_update_attribute(struct pipeline* s, int index,
                                        const struct buffer* buffer);
int ngli_pipeline_d3d12_update_uniform(struct pipeline* s, int index,
                                      const void* value);
int ngli_pipeline_d3d12_update_texture(struct pipeline* s, int index,
                                      const struct texture* texture);
int ngli_pipeline_d3d12_update_buffer(struct pipeline* s, int index,
                                     const struct buffer* buffer, int offset,
                                     int size);
void ngli_pipeline_d3d12_draw(struct pipeline* s, int nb_vertices,
                             int nb_instances);
void ngli_pipeline_d3d12_draw_indexed(struct pipeline* s,
                                     const struct buffer* indices,
                                     int indices_format, int nb_indices,
                                     int nb_instances);
void ngli_pipeline_d3d12_dispatch(struct pipeline* s, int nb_group_x,
                                 int nb_group_y, int nb_group_z,
                                 int threads_per_group_x,
                                 int threads_per_group_y,
                                 int threads_per_group_z);
void ngli_pipeline_d3d12_freep(struct pipeline** sp);
