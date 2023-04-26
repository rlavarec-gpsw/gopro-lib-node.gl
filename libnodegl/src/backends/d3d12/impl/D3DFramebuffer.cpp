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
#include "D3DFramebuffer.h"
#include <backends/d3d12/impl/D3DTexture.h>
#include <backends/d3d12/impl/D3DSwapchain.h>
#include <backends/d3d12/impl/D3DRenderPass.h>

namespace ngli
{

uint32_t D3DFramebuffer::D3DAttachmentBasic::subresourceIndex()
{
	return layer * texture->mipLevels + level;
};

void D3DFramebuffer::D3DAttachment::create(D3DTexture* texture, uint32_t level, uint32_t baseLayer, uint32_t layerCount)
{
	this->mD3DAttachementBasic.texture = texture;
	this->mD3DAttachementBasic.level = level;
	this->mD3DAttachementBasic.layer = baseLayer;
	this->layerCount = layerCount;
	resource = texture->mID3D12Resource.Get();
	bool depthStencilAttachment =
		texture->imageUsageFlags & NGLI_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	cpuDescriptor = depthStencilAttachment
		? texture->dsvDescriptor->cpuHandle
		: texture->getRtvDescriptor(level, baseLayer, layerCount)->cpuHandle;
	subresourceIndex = baseLayer * texture->mipLevels + level;
	imageUsageFlags = texture->imageUsageFlags;
	numSamples = texture->numSamples;
	format = DXGI_FORMAT(texture->format);
}

void D3DFramebuffer::create(std::vector<D3DFramebuffer::D3DAttachment>& d3dAttachments,
							int32_t w, uint32_t h, uint32_t layers)
{
	this->d3dAttachments = d3dAttachments;
	this->numAttachments = uint32_t(d3dAttachments.size());
	this->w = w;
	this->h = h;
	auto it = this->d3dAttachments.begin();
	while(it != this->d3dAttachments.end())
	{
		if(it->imageUsageFlags & NGLI_TEXTURE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			it->mAttachement.load_op = NGLI_LOAD_OP_CLEAR;

			if(it->numSamples > 1)
			{
				colorAttachments.push_back(&(*it++));
				resolveAttachments.push_back(&(*it++));
			}
			else
			{
				colorAttachments.push_back(&(*it++));
			}
		}
		else if(it->imageUsageFlags & NGLI_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			it->mAttachement.load_op = NGLI_LOAD_OP_CLEAR;

			if(it->numSamples > 1)
			{
				depthStencilAttachment = &(*it++);
				if(it != this->d3dAttachments.end())
					depthResolve = &(*it++);
			}
			else
			{
				depthStencilAttachment = &(*it++);
			}
		}
	}
}

void D3DFramebuffer::D3DAttachment::createFromSwapchainImage(D3DSwapchain* d3dSwapchain, uint32_t index)
{
	resource = d3dSwapchain->renderTargets[index].Get();
	cpuDescriptor = d3dSwapchain->renderTargetDescriptors[index]->cpuHandle;
	subresourceIndex = 0;
	imageUsageFlags = NGLI_TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
	numSamples = 1;
	format = DXGI_FORMAT(d3dSwapchain->mFormat);
}

void D3DFramebuffer::D3DAttachment::createFromDepthStencilAttachment(D3DTexture* d3dDepthStencilAttachment)
{
	resource = d3dDepthStencilAttachment->mID3D12Resource.Get();
	cpuDescriptor = d3dDepthStencilAttachment->dsvDescriptor->cpuHandle;
	subresourceIndex = 0;
	imageUsageFlags = NGLI_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	numSamples = 1;
	format = DXGI_FORMAT(d3dDepthStencilAttachment->format);
	mD3DAttachementBasic.texture = d3dDepthStencilAttachment;
}

D3DFramebuffer* D3DFramebuffer::newInstance(D3DDevice* device, D3DRenderPass* renderPass,
								 const std::vector<D3DAttachmentBasic>& attachments,
								 uint32_t w, uint32_t h, uint32_t layers)
{
	D3DFramebuffer* d3dFramebuffer = new D3DFramebuffer();
	std::vector<D3DFramebuffer::D3DAttachment> d3dAttachments(attachments.size());
	for(uint32_t j = 0; j < attachments.size(); j++)
	{
		auto& d3dAttachment = d3dAttachments[j];
		auto& attachment = attachments[j];
		auto d3dTexture = attachment.texture;
		bool depthStencilAttachment =
			d3dTexture->imageUsageFlags & NGLI_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		d3dAttachment = {
			d3dTexture->mID3D12Resource.Get(),
			depthStencilAttachment
				? d3dTexture->dsvDescriptor->cpuHandle
				: d3dTexture
					  ->getRtvDescriptor(attachment.level, attachment.layer, layers)
					  ->cpuHandle,
			attachment.layer * d3dTexture->mipLevels + attachment.level,
			d3dTexture->imageUsageFlags,
			d3dTexture->numSamples,
			d3dTexture->format,
			1,
			attachment
		};
	}
	d3dFramebuffer->create(d3dAttachments, w, h, layers);
	return d3dFramebuffer;
}

}