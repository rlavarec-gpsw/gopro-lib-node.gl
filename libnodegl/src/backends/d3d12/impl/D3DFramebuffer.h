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
extern "C" {
#include "rendertarget.h"
}

namespace ngli
{

class D3DTexture;
class D3DSwapchain;
class D3DRenderPass;
class D3DDevice;

/** \class D3DFramebuffer
 *
 *  This class defines the interface for a framebuffer object.
 *  It supports rendering to a destination surface, such as a texture
 *  or a window surface.
 */
class D3DFramebuffer
{
public:
	struct D3DAttachmentBasic
	{
/** The destination texture */
		D3DTexture* texture = nullptr;
		uint32_t level = 0; /**< The destination texture mipmap level */
		uint32_t layer = 0; /**< The destination texture array layer index */
   /** Get the attachment subresource index */
		uint32_t subresourceIndex();
	};

	struct D3DAttachment
	{
		void create(D3DTexture* texture, uint32_t level = 0, uint32_t baseLayer = 0, uint32_t layerCount = 1);
		void createFromSwapchainImage(D3DSwapchain* d3dSwapchain, uint32_t index);
		void createFromDepthStencilAttachment(D3DTexture* d3dDepthStencilAttachment);

		ID3D12Resource* resource = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor{};
		uint32_t subresourceIndex = 0;
		uint32_t imageUsageFlags = 0;
		uint32_t numSamples = 1;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		uint32_t layerCount = 1;

		D3DAttachmentBasic mD3DAttachementBasic;
		attachment mAttachement;
	};

	/** Create a framebuffer object
	 *  @param device The graphics device
	 *  @param renderPass The renderPass object
	 *  @param attachments The output attachments
	 *  @param w The destination width
	 *  @param h The destination height
	 *  @param layers The number of output layers
	 */
	static D3DFramebuffer* newInstance(D3DDevice* device, D3DRenderPass* renderPass,
							   const std::vector<D3DAttachmentBasic>& attachments,
							   uint32_t w, uint32_t h, uint32_t layers = 1);

	void create(std::vector<D3DAttachment>& attachments, int32_t w, uint32_t h,
				uint32_t layers = 1);
	~D3DFramebuffer() {}


	uint32_t w; /**< The output width */
	uint32_t h; /**< The output height */
	uint32_t layers; /**< The number of output layers */
	uint32_t numAttachments; /**< The number of attachments */

	std::vector<D3DAttachment> d3dAttachments;
	std::vector<D3DAttachment*> colorAttachments;
	std::vector<D3DAttachment*> resolveAttachments;
	D3DAttachment* depthStencilAttachment = nullptr;
	D3DAttachment* depthResolve = nullptr;
};

} // namespace ngli