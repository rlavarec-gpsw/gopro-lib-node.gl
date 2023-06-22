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
#pragma once
#include <backends/d3d12/impl/D3DPipeline.h>

extern "C" {
#include "pipeline.h"
}

struct buffer_binding
{
	pipeline_buffer_desc desc;
	const struct buffer* buffer;
};

struct pipeline_d3d12;

extern "C" {
	struct pipeline_resources;
}

namespace ngli
{

class D3DComputeShaderModule;

class D3DComputePipeline : public D3DPipeline
{
public:
	void create(D3DGraphicsContext* ctx,
				const std::vector<CD3DX12_ROOT_PARAMETER1>& rootParameters,
				D3D12_SHADER_BYTECODE shaderByteCode);

	static D3DComputePipeline* newInstance(pipeline_d3d12* pipeline, D3DGraphicsContext* graphicsContext, D3DComputeShaderModule* cs, struct pipeline_resources* resources);

	void getBindings(std::vector<uint32_t*> pDescriptorBindings);
	std::vector<uint32_t> descriptorBindings;

	ComPtr<ID3D12PipelineState> d3dPipelineState;
};

}; // namespace ngli