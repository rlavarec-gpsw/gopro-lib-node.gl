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
#include "D3DGraphics.h"
#include <backends/d3d12/impl/D3DGraphicsContext.h>
#include <backends/d3d12/impl/D3DBuffer.h>
#include <backends/d3d12/impl/D3DCommandList.h>
#include <backends/d3d12/impl/D3DComputePipeline.h>
#include <backends/d3d12/impl/D3DGraphicsPipeline.h>
#include <backends/d3d12/impl/D3DTexture.h>
#include <backends/d3d12/impl/D3DSampler.h>

namespace ngli
{

void D3DGraphics::bindComputePipeline(D3DCommandList* commandBuffer, D3DComputePipeline* computePipeline)
{
	auto d3dCommandList = commandBuffer->mGraphicsCommandList;
	auto d3dComputePipeline = computePipeline;
	D3D_TRACE(d3dCommandList->SetPipelineState(
		d3dComputePipeline->d3dPipelineState.Get()));
	D3D_TRACE(d3dCommandList->SetComputeRootSignature(
		d3dComputePipeline->d3dRootSignature.Get()));
	setDescriptorHeaps(commandBuffer);
	currentPipeline = computePipeline;
}

void D3DGraphics::setDescriptorHeaps(D3DCommandList* commandBuffer)
{
	auto d3dCommandList = commandBuffer->mGraphicsCommandList;
	auto cbvSrvUavHeap = mCtx->d3dCbvSrvUavDescriptorHeap.mID3D12DescriptorHeap.Get();
	auto samplerDescriptorHeap = mCtx->d3dSamplerDescriptorHeap.mID3D12DescriptorHeap.Get();
	std::vector<ID3D12DescriptorHeap*> descriptorHeaps = { cbvSrvUavHeap,
														   samplerDescriptorHeap };
	D3D_TRACE(d3dCommandList->SetDescriptorHeaps(UINT(descriptorHeaps.size()),
												 descriptorHeaps.data()));
}

void D3DGraphics::bindGraphicsPipeline(D3DCommandList* commandBuffer,
									   D3DGraphicsPipeline* graphicsPipeline)
{
	auto d3dCommandList = commandBuffer->mGraphicsCommandList;
	auto d3dGraphicsPipeline = graphicsPipeline;
	D3D_TRACE(d3dCommandList->SetPipelineState(
		d3dGraphicsPipeline->d3dPipelineState.Get()));
	D3D_TRACE(d3dCommandList->OMSetStencilRef(
		d3dGraphicsPipeline->d3dStencilRef));
	D3D_TRACE(d3dCommandList->IASetPrimitiveTopology(
		d3dGraphicsPipeline->d3dPrimitiveTopology));
	D3D_TRACE(d3dCommandList->SetGraphicsRootSignature(
		d3dGraphicsPipeline->d3dRootSignature.Get()));
	setDescriptorHeaps(commandBuffer);
	currentPipeline = graphicsPipeline;
}

void D3DGraphics::bindUniformBuffer(D3DCommandList* commandBuffer,
									D3DBuffer* buffer, uint32_t binding,
									ShaderStageFlags shaderStageFlags)
{
	auto d3dCommandList = commandBuffer->mGraphicsCommandList.Get();
	auto d3dBuffer = buffer;
	if(D3DGraphicsPipeline* graphicsPipeline =
			dynamic_cast<D3DGraphicsPipeline*>(currentPipeline))
	{
		D3D_TRACE(d3dCommandList->SetGraphicsRootConstantBufferView(
			binding, d3dBuffer->mID3D12Resource->GetGPUVirtualAddress()));
	}
	else if(D3DComputePipeline* computePipeline =
				dynamic_cast<D3DComputePipeline*>(currentPipeline))
	{
		D3D_TRACE(d3dCommandList->SetComputeRootConstantBufferView(
			binding, d3dBuffer->mID3D12Resource->GetGPUVirtualAddress()));
	}
}

void D3DGraphics::bindIndexBuffer(D3DCommandList* commandBuffer, D3DBuffer* buffer,
								  IndexFormat indexFormat)
{
	D3D12_INDEX_BUFFER_VIEW ib;
	ib.BufferLocation = buffer->mID3D12Resource->GetGPUVirtualAddress();
	ib.Format = DXGI_FORMAT(indexFormat);
	ib.SizeInBytes = buffer->mSize;
	commandBuffer->mGraphicsCommandList->IASetIndexBuffer(&ib);
}

void D3DGraphics::bindVertexBuffer(D3DCommandList* commandBuffer, D3DBuffer* buffer,
								   uint32_t location, uint32_t stride)
{
	D3D12_VERTEX_BUFFER_VIEW vb;
	vb.BufferLocation = buffer->mID3D12Resource->GetGPUVirtualAddress();
	vb.StrideInBytes = stride;
	vb.SizeInBytes = buffer->mSize;
	commandBuffer->mGraphicsCommandList->IASetVertexBuffers(location, 1, &vb);
}

void D3DGraphics::bindStorageBuffer(D3DCommandList* commandBuffer,
									D3DBuffer* buffer, uint32_t binding,
									ShaderStageFlags shaderStageFlags, bool readonly)
{
	auto d3dCommandList = commandBuffer->mGraphicsCommandList.Get();
	auto d3dBuffer = buffer;
	if(D3DGraphicsPipeline* graphicsPipeline =
			dynamic_cast<D3DGraphicsPipeline*>(currentPipeline))
	{
		if(!readonly)
		{
			D3D_TRACE(d3dCommandList->SetGraphicsRootUnorderedAccessView(
				binding, d3dBuffer->mID3D12Resource->GetGPUVirtualAddress()));
		}
		else
		{
			D3D_TRACE(d3dCommandList->SetGraphicsRootShaderResourceView(
				binding, d3dBuffer->mID3D12Resource->GetGPUVirtualAddress()));
		}
	}
	else if(D3DComputePipeline* computePipeline =
				dynamic_cast<D3DComputePipeline*>(currentPipeline))
	{
		if(!readonly)
		{
			D3D_TRACE(d3dCommandList->SetComputeRootUnorderedAccessView(
				binding, d3dBuffer->mID3D12Resource->GetGPUVirtualAddress()));
		}
		else
		{
			D3D_TRACE(d3dCommandList->SetComputeRootShaderResourceView(
				binding, d3dBuffer->mID3D12Resource->GetGPUVirtualAddress()));
		}
	}
}

void D3DGraphics::bindSampler(D3DCommandList* commandBuffer, D3DSampler* sampler,
	uint32_t set)
{
	auto d3dCommandList = commandBuffer->mGraphicsCommandList.Get();
	if(D3DGraphicsPipeline* graphicsPipeline =
		dynamic_cast<D3DGraphicsPipeline*>(currentPipeline))
	{
		D3D_TRACE(d3dCommandList->SetGraphicsRootDescriptorTable(
			set, sampler->handle->gpuHandle));
	}
	else if(D3DComputePipeline* computePipeline =
		dynamic_cast<D3DComputePipeline*>(currentPipeline))
	{
		D3D_TRACE(d3dCommandList->SetComputeRootDescriptorTable(
			set, sampler->handle->gpuHandle));
	}
}

void D3DGraphics::bindTexture(D3DCommandList* commandBuffer, D3DTexture* texture,
	uint32_t set)
{
	auto d3dCommandList = commandBuffer->mGraphicsCommandList.Get();
	auto d3dTexture = texture;
	uint32_t numPlanes = d3dTexture->numPlanes;
	if(d3dTexture->format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
		numPlanes = 1;
	//TODO: pass current image usage as param
	//imageUsageFlags defines all the possible usage scenarios, not the current usage scenario
	int imageUsageFlags = d3dTexture->imageUsageFlags;
	if(imageUsageFlags & IMAGE_USAGE_STORAGE_BIT)
		imageUsageFlags &= ~IMAGE_USAGE_SAMPLED_BIT;
	if(D3DGraphicsPipeline* graphicsPipeline =
		dynamic_cast<D3DGraphicsPipeline*>(currentPipeline))
	{
		if(imageUsageFlags & IMAGE_USAGE_STORAGE_BIT)
		{
			for(uint32_t j = 0; j < numPlanes; j++)
			{
				D3D_TRACE(d3dCommandList->SetGraphicsRootDescriptorTable(
					set + j, d3dTexture->defaultUavDescriptor[j]->gpuHandle));
			}
		}
		else if(imageUsageFlags & IMAGE_USAGE_SAMPLED_BIT)
		{
			for(uint32_t j = 0; j < numPlanes; j++)
			{
				D3D_TRACE(d3dCommandList->SetGraphicsRootDescriptorTable(
					set + j, d3dTexture->defaultSrvDescriptor[j]->gpuHandle));
			}
		}
	}
	else if(D3DComputePipeline* computePipeline =
		dynamic_cast<D3DComputePipeline*>(currentPipeline))
	{
		if(imageUsageFlags & IMAGE_USAGE_STORAGE_BIT)
		{
			for(uint32_t j = 0; j < numPlanes; j++)
			{
				D3D_TRACE(d3dCommandList->SetComputeRootDescriptorTable(
					set + j, d3dTexture->defaultUavDescriptor[j]->gpuHandle));
			}
		}
		else if(imageUsageFlags & IMAGE_USAGE_SAMPLED_BIT)
		{
			for(uint32_t j = 0; j < numPlanes; j++)
			{
				D3D_TRACE(d3dCommandList->SetComputeRootDescriptorTable(
					set + j, d3dTexture->defaultSrvDescriptor[j]->gpuHandle));
			}
		}
	}
	if((imageUsageFlags & IMAGE_USAGE_SAMPLED_BIT) && d3dTexture->defaultSampler)
	{
		bindSampler(commandBuffer, d3dTexture->defaultSampler, set + numPlanes);
	}
}

void D3DGraphics::bindTextureAsImage(D3DCommandList* commandBuffer, D3DTexture* texture,
		uint32_t set)
{
	auto d3dCommandList = commandBuffer->mGraphicsCommandList.Get();
	auto d3dTexture = texture;
	uint32_t numPlanes = d3dTexture->numPlanes;
	if(D3DGraphicsPipeline* graphicsPipeline =
		dynamic_cast<D3DGraphicsPipeline*>(currentPipeline))
	{
		for(uint32_t j = 0; j < numPlanes; j++)
		{
			D3D_TRACE(d3dCommandList->SetGraphicsRootDescriptorTable(
				set + j, d3dTexture->defaultUavDescriptor[j]->gpuHandle));
		}
	}
	else if(D3DComputePipeline* computePipeline =
		dynamic_cast<D3DComputePipeline*>(currentPipeline))
	{
		for(uint32_t j = 0; j < numPlanes; j++)
		{
			D3D_TRACE(d3dCommandList->SetComputeRootDescriptorTable(
				set + j, d3dTexture->defaultUavDescriptor[j]->gpuHandle));
		}
	}
}

void D3DGraphics::waitIdle(D3DCommandList* cmdBuffer)
{
	mCtx->d3dDevice.waitIdle();
}

void D3DGraphics::resourceBarrier(
	D3DCommandList* cmdList, D3DFramebuffer::D3DAttachment* p,
	D3D12_RESOURCE_STATES currentState, D3D12_RESOURCE_STATES newState,
	UINT subresourceIndex)
{
	if(p->texture)
		p->texture->resourceBarrierTransition(cmdList, newState, subresourceIndex);
	else
	{
		CD3DX12_RESOURCE_BARRIER resourceBarrier =
			CD3DX12_RESOURCE_BARRIER::Transition(p->resource, currentState,
												 newState, p->subresourceIndex);
		D3D_TRACE(cmdList->mGraphicsCommandList->ResourceBarrier(1, &resourceBarrier));
	}
}

void D3DGraphics::beginRenderPass(D3DCommandList* commandBuffer,
								  D3DRenderPass* renderPass,
								  D3DFramebuffer* framebuffer,
								  glm::vec4 clearColor, float clearDepth,
								  uint32_t clearStencil)
{
	auto d3dCtx = mCtx;
	auto d3dRenderPass = renderPass;
	auto d3dCommandList = commandBuffer;
	auto d3dFramebuffer = framebuffer;
	auto& colorAttachments = d3dFramebuffer->colorAttachments;
	auto& resolveAttachments = d3dFramebuffer->resolveAttachments;
	auto depthStencilAttachment = d3dFramebuffer->depthStencilAttachment;
	if(!resolveAttachments.empty())
	{
		for(auto& colorAttachment : colorAttachments)
		{
			resourceBarrier(d3dCommandList, colorAttachment,
							D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
							D3D12_RESOURCE_STATE_RENDER_TARGET,
							colorAttachment->subresourceIndex);
		}
	}
	else
	{
		for(auto& colorAttachment : colorAttachments)
		{
			resourceBarrier(d3dCommandList, colorAttachment,
							d3dRenderPass->finalResourceState,
							d3dRenderPass->initialResourceState,
							colorAttachment->subresourceIndex);
		}
	}
	auto cbvSrvUavHeap = d3dCtx->d3dCbvSrvUavDescriptorHeap.mID3D12DescriptorHeap.Get();
	auto samplerDescriptorHeap = d3dCtx->d3dSamplerDescriptorHeap.mID3D12DescriptorHeap.Get();
	std::vector<ID3D12DescriptorHeap*> descriptorHeaps = { cbvSrvUavHeap,
														   samplerDescriptorHeap };
	D3D_TRACE(d3dCommandList->mGraphicsCommandList->SetDescriptorHeaps(UINT(descriptorHeaps.size()),
													descriptorHeaps.data()));
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> colorAttachmentHandles(
		colorAttachments.size());
	setRenderTargets(d3dCommandList, d3dFramebuffer->colorAttachments,
		d3dFramebuffer->depthStencilAttachment);
	if(d3dRenderPass->colorLoadOp == ATTACHMENT_LOAD_OP_CLEAR)
	{
		for(auto& colorAttachment : colorAttachments)
		{
			D3D_TRACE(d3dCommandList->mGraphicsCommandList->ClearRenderTargetView(
				colorAttachment->cpuDescriptor, glm::value_ptr(clearColor), 0,
				nullptr));
		}
	}
	if(depthStencilAttachment && d3dRenderPass->depthLoadOp == ATTACHMENT_LOAD_OP_CLEAR)
	{
		D3D_TRACE(d3dCommandList->mGraphicsCommandList->ClearDepthStencilView(
			depthStencilAttachment->cpuDescriptor,
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, clearDepth,
			clearStencil, 0, nullptr));
	}
	currentRenderPass = renderPass;
	currentFramebuffer = framebuffer;
}

void D3DGraphics::setRenderTargets(D3DCommandList* d3dCommandList,
	const std::vector<D3DFramebuffer::D3DAttachment*>& colorAttachments,
	const D3DFramebuffer::D3DAttachment* depthStencilAttachment)
{
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> colorAttachmentHandles(
		colorAttachments.size());
	for(uint32_t j = 0; j < colorAttachments.size(); j++)
		colorAttachmentHandles[j] = colorAttachments[j]->cpuDescriptor;
	D3D_TRACE(d3dCommandList->mGraphicsCommandList->OMSetRenderTargets(
		UINT(colorAttachments.size()), colorAttachmentHandles.data(), FALSE,
		depthStencilAttachment ? &depthStencilAttachment->cpuDescriptor
		: nullptr));
}

void D3DGraphics::endRenderPass(D3DCommandList* commandBuffer)
{
	auto d3dCommandList = commandBuffer;
	auto d3dFramebuffer = currentFramebuffer;
	auto& colorAttachments = d3dFramebuffer->colorAttachments;
	auto& resolveAttachments = d3dFramebuffer->resolveAttachments;
	auto& depthStencilAttachment = d3dFramebuffer->depthStencilAttachment;
	auto& depthResolve = d3dFramebuffer->depthResolve;
	auto d3dRenderPass = currentRenderPass;
	if(!resolveAttachments.empty())
	{
		for(uint32_t j = 0; j < colorAttachments.size(); j++)
		{
			auto& colorAttachment = colorAttachments[j];
			auto& resolveAttachment = resolveAttachments[j];
			resourceBarrier(d3dCommandList, colorAttachment,
							D3D12_RESOURCE_STATE_RENDER_TARGET,
							D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
			resourceBarrier(d3dCommandList, resolveAttachment,
							D3D12_RESOURCE_STATE_PRESENT,
							D3D12_RESOURCE_STATE_RESOLVE_DEST);
			D3D_TRACE(d3dCommandList->mGraphicsCommandList->ResolveSubresource(
				resolveAttachment->resource, resolveAttachment->subresourceIndex,
				colorAttachment->resource, colorAttachment->subresourceIndex,
				colorAttachment->format));
			resourceBarrier(d3dCommandList, resolveAttachment,
							D3D12_RESOURCE_STATE_RESOLVE_DEST,
							d3dRenderPass->finalResourceState,
							resolveAttachment->subresourceIndex);
		}
	}
	else
	{
		for(auto& colorAttachment : colorAttachments)
		{
			resourceBarrier(
				d3dCommandList, colorAttachment, d3dRenderPass->initialResourceState,
				d3dRenderPass->finalResourceState, colorAttachment->subresourceIndex);
		}
	}
	if(depthResolve)
	{
		resourceBarrier(d3dCommandList, depthStencilAttachment,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
		resourceBarrier(d3dCommandList, depthResolve,
			D3D12_RESOURCE_STATE_DEPTH_READ,
			D3D12_RESOURCE_STATE_RESOLVE_DEST);
		DXGI_FORMAT texFormat = depthStencilAttachment->format;
		if(texFormat == DXGI_FORMAT_D16_UNORM)
			texFormat = DXGI_FORMAT_R16_TYPELESS;
		else if(texFormat == DXGI_FORMAT_D24_UNORM_S8_UINT)
			texFormat = DXGI_FORMAT_R24G8_TYPELESS;
		else if(texFormat == DXGI_FORMAT_D32_FLOAT)
			texFormat = DXGI_FORMAT_R32_TYPELESS;
		else if(texFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
			texFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		D3D_TRACE(d3dCommandList->mGraphicsCommandList->ResolveSubresource(
			depthResolve->resource, depthResolve->subresourceIndex,
			depthStencilAttachment->resource, depthStencilAttachment->subresourceIndex,
			texFormat));
		resourceBarrier(d3dCommandList, depthStencilAttachment,
			D3D12_RESOURCE_STATE_RESOLVE_DEST,
			d3dRenderPass->finalResourceState,
			depthResolve->subresourceIndex);
	}
	currentRenderPass = nullptr;
	currentFramebuffer = nullptr;
}

void D3DGraphics::beginProfile(D3DCommandList* commandBuffer)
{
	commandBuffer->mGraphicsCommandList->EndQuery(mCtx->d3dQueryTimestampHeap.v.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
}

uint64_t D3DGraphics::endProfile(D3DCommandList* commandBuffer)
{
	auto d3dCtx = mCtx;
	auto d3dCommandList = commandBuffer->mGraphicsCommandList;
	auto queryHeap = d3dCtx->d3dQueryTimestampHeap.v.Get();
	auto timestampResultBuffer = d3dCtx->d3dTimestampResultBuffer;
	d3dCommandList->EndQuery(queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 1);
	d3dCommandList->ResolveQueryData(queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, timestampResultBuffer.mID3D12Resource.Get(), 0);
	uint64_t* t = (uint64_t*)timestampResultBuffer.map();
	const UINT64 r = t[1] - t[0];
	timestampResultBuffer.unmap();
	return r;
}

void D3DGraphics::dispatch(D3DCommandList* commandBuffer, uint32_t groupCountX,
						   uint32_t groupCountY, uint32_t groupCountZ,
						   int32_t, int32_t, int32_t)
{
	D3D_TRACE(
		commandBuffer->mGraphicsCommandList->Dispatch(groupCountX, groupCountY, groupCountZ));
}

void D3DGraphics::draw(D3DCommandList* cmdBuffer, uint32_t vertexCount,
					   uint32_t instanceCount, uint32_t firstVertex,
					   uint32_t firstInstance)
{
	D3D_TRACE(cmdBuffer->mGraphicsCommandList->DrawInstanced(vertexCount, instanceCount,
											   firstVertex, firstInstance));
}

void D3DGraphics::drawIndexed(D3DCommandList* cmdBuffer, uint32_t indexCount,
							  uint32_t instanceCount, uint32_t firstIndex,
							  int32_t vertexOffset, uint32_t firstInstance)
{
	D3D_TRACE(cmdBuffer->mGraphicsCommandList->DrawIndexedInstanced(
		indexCount, instanceCount, firstIndex, vertexOffset, firstInstance));
}
void D3DGraphics::setViewport(D3DCommandList* cmdBuffer, rect r)
{
	viewport = r;
	D3D12_VIEWPORT d3dViewport = { float(r.x), float(r.y), float(r.w),
								  float(r.h), 0.0f,       1.0f };
	D3D_TRACE(cmdBuffer->mGraphicsCommandList->RSSetViewports(1, &d3dViewport));
}
void D3DGraphics::setScissor(D3DCommandList* cmdBuffer, rect r)
{
	scissorRect = r;
#ifdef ORIGIN_BOTTOM_LEFT
	D3D12_RECT d3dScissorRect = { long(r.x), long(r.y), long(r.x + r.w),
								 long(r.y + r.h) };
#else
	D3D12_RECT d3dScissorRect = { long(r.x), long(viewport.h - r.y - r.h),
								 long(r.x + r.w), long(viewport.h - r.y) };
#endif
	D3D_TRACE(cmdBuffer->mGraphicsCommandList->RSSetScissorRects(1, &d3dScissorRect));
}

D3DGraphics* D3DGraphics::newInstance(D3DGraphicsContext* ctx)
{
	D3DGraphics* d3dGraphics = new D3DGraphics();
	d3dGraphics->mCtx = ctx;
	d3dGraphics->create();
	return d3dGraphics;
}

}