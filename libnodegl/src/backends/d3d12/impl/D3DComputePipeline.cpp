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
#include <stdafx.h>
#include "D3DComputePipeline.h"
#include <backends/d3d12/impl/D3DShaderModule.h>
#include <backends/d3d12/impl/D3DPipelineUtil.h>
#include <backends/d3d12/impl/D3DGraphicsContext.h>
#include <backends/d3d12/pipeline_d3d12.h>

extern "C" {
#include "type.h"
}

namespace ngli
{

void D3DComputePipeline::getBindings(
	std::vector<uint32_t*> pDescriptorBindings)
{
	for(uint32_t j = 0; j < pDescriptorBindings.size(); j++)
		*pDescriptorBindings[j] = descriptorBindings[j];
}

void D3DComputePipeline::create(
	D3DGraphicsContext* ctx,
	const std::vector<CD3DX12_ROOT_PARAMETER1>& rootParameters,
	D3D12_SHADER_BYTECODE shaderByteCode)
{
	D3DPipeline::create(ctx);
	HRESULT hResult;
	auto d3dDevice = ctx->d3dDevice.mID3D12Device;
	createRootSignature(rootParameters);
	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = d3dRootSignature.Get();
	desc.CS.pShaderBytecode = shaderByteCode.pShaderBytecode;
	desc.CS.BytecodeLength = shaderByteCode.BytecodeLength;
	D3D_TRACE_CALL(d3dDevice->CreateComputePipelineState(&desc,
											IID_PPV_ARGS(&d3dPipelineState)));
	d3dPipelineState->SetName(L"d3dPipelineState");
}

D3DComputePipeline* D3DComputePipeline::newInstance(pipeline_d3d12* pipeline, D3DGraphicsContext* graphicsContext,
										 D3DComputeShaderModule* cs, struct pipeline_resources* resources)
{
	std::vector<CD3DX12_ROOT_PARAMETER1> d3dRootParams;
	std::vector<D3D12_STATIC_SAMPLER_DESC> d3dSamplers;
	std::vector<std::unique_ptr<CD3DX12_DESCRIPTOR_RANGE1>> d3dDescriptorRanges;
//	uint32_t numDescriptors = uint32_t(cs->descriptors.size());
	std::map<uint32_t, D3DShaderModule::DescriptorInfo> descriptors;
	uint32_t descriptorBindingsSize = 0;
	for(auto& csDescriptor : cs->descriptors)
	{
		descriptors[csDescriptor.set] = csDescriptor;
		descriptorBindingsSize = std::max(descriptorBindingsSize, csDescriptor.set + 1);
		
		// Add support for gl_NumWorkGroups
		if(csDescriptor.name == "SPIRV_Cross_NumWorkgroups")
		{
			bool tSPIRV_Cross_NumWorkgroupsExist = false;

			// Search if the SPIRV_Cross_NumWorkgroups already added
			int nb_buffers = ngli_darray_count(&pipeline->buffer_bindings);
			for(int i = 0; i < nb_buffers; i++)
			{
				const buffer_binding* binding =
					(buffer_binding*)ngli_darray_get(&pipeline->buffer_bindings, i);
				if(strcmp(binding->desc.name, "SPIRV_Cross_NumWorkgroups") == 0)
				{
					tSPIRV_Cross_NumWorkgroupsExist = true;
					break;
				}
			}

			// If SPIRV_Cross_NumWorkgroups is not added, add it
			if(!tSPIRV_Cross_NumWorkgroupsExist)
			{
				// Describe it
				buffer_binding binding;
				auto& desc = binding.desc;
				strcpy(desc.name, "SPIRV_Cross_NumWorkgroups");
				desc.type = NGLI_TYPE_UNIFORM_BUFFER;
				desc.binding = csDescriptor.set;
				desc.access = NGLI_ACCESS_READ_BIT;
				desc.stage = NGLI_PROGRAM_SHADER_COMP;
				desc.offset = 0;
				desc.size = sizeof(uint32_t)*3;

				binding.buffer = pipeline->bufferNumWorkgroups[0] = ngli_buffer_create(pipeline->parent.gpu_ctx);
				if(!binding.buffer)
					return 0;

				if(!ngli_darray_push(&pipeline->buffer_bindings, &binding))
					return 0;


				const int size = desc.size;

				int ret = ngli_buffer_init((buffer*)binding.buffer, size, NGLI_BUFFER_USAGE_TRANSFER_SRC_BIT | NGLI_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
				if(ret < 0)
					return 0;

				// No data, data will be uploaded with d3d12_update_NumWorkgroups
				std::vector<uint32_t> tData;
				tData.resize(3);
				ret = ngli_buffer_upload((buffer*)binding.buffer, tData.data(), size, desc.offset);
				if(ret < 0)
					return 0;
			}
		}
	}

	D3DComputePipeline* d3dComputePipeline = new D3DComputePipeline();
	d3dComputePipeline->descriptorBindings.resize(descriptorBindingsSize);

	D3DPipelineUtil::IsReadOnly isReadOnly = [&](const D3DShaderModule::DescriptorInfo& descriptorInfo) -> bool {
		if(descriptorInfo.type == DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			auto bufferInfo = cs->findUniformBufferInfo(descriptorInfo.name);
			return bufferInfo->readonly;
		}
		else if(descriptorInfo.type == DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			auto bufferInfo = cs->findStorageBufferInfo(descriptorInfo.name);
			return bufferInfo->readonly;
		}
		else
			return false;
	};

	D3DPipelineUtil::parseDescriptors(descriptors, d3dComputePipeline->descriptorBindings,
									  d3dRootParams, d3dDescriptorRanges,
									  D3DPipelineUtil::PIPELINE_TYPE_COMPUTE, isReadOnly);

	d3dComputePipeline->create(graphicsContext, d3dRootParams, cs->d3dShaderByteCode);
	return d3dComputePipeline;
}

}