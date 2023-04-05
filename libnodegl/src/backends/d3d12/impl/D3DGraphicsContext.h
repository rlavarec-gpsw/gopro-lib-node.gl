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
#include <backends/d3d12/impl/D3DCommandList.h>
#include <backends/d3d12/impl/D3DDevice.h>
#include <backends/d3d12/impl/D3DFramebuffer.h>
#include <backends/d3d12/impl/D3DRenderPass.h>
#include <backends/d3d12/impl/D3DPipelineCache.h>
#include <backends/d3d12/impl/D3DCommandQueue.h>
#include <backends/d3d12/impl/D3DReadbackBuffer.h>
#include <backends/d3d12/impl/D3DQueryHeap.h>
#include <backends/d3d12/impl/D3DDescriptorHeap.h>
#include <backends/d3d12/impl/D3DTexture.h>
#include <backends/d3d12/impl/D3DSwapchain.h>

namespace ngli
{
class D3DSurface;
class D3DGraphics;

/** \class D3DGraphicsContext
*
*  This class provides an abstraction of a graphics context, as well as
*  various utility functions.  The process can have more than one graphics context.
*  Each graphics context can have either an onscreen or offscreen surface, and it also
*  supports headless mode (no surface attachment), which is especially useful for compute applications.
*/
class D3DGraphicsContext
{
public:
	using OnSelectDepthStencilFormats = std::function<void( const std::vector<DXGI_FORMAT>& depthStencilFormatCandidates,
															DXGI_FORMAT& depthFormat,
															DXGI_FORMAT& depthStencilFormat)>;

	~D3DGraphicsContext();



	/** Create the graphics context
	 *  @param appName The application name
	 *  @param enableDepthStencil Enable depth/stencil buffer
	 *  @bool debug Enable debugging and validation layers
	 *  @param onSelectDepthStencilFormats An optional callback that lets the user override the depth / depthStencil format
	 */
	static D3DGraphicsContext* newInstance(const char* appName,
								   bool enableDepthStencil = false,
								   bool debug = true,
								   OnSelectDepthStencilFormats onSelectDepthStencilFormats = nullptr);
	void init(const char* appName,
								   bool enableDepthStencil = false,
								   bool debug = true,
								   OnSelectDepthStencilFormats onSelectDepthStencilFormats = nullptr);

	/** Set the surface for the graphics context
	 *  This can be an offscreen or onscreen surface.
	 *  Also, if the user passes nullptr, then the graphics context will work in
	 *  "headless" mode.  One use case for headless mode is an application that only does compute operations.
	 *   @param surface The surface attachment (or nullptr for headless mode) */
	virtual void setSurface(D3DSurface* surface);
	/** Begin a render pass: for drawing to the main surface attached to the context.
	 *  This is a helper function that also sets the viewport and scissor rect.
		@param commandBuffer The command buffer
		@param graphics      The graphics interface for recording graphics commands to the command buffer */
	virtual void beginRenderPass(D3DCommandList* commandBuffer,
								 D3DGraphics* graphics);
	/** Begin an offscreen render pass.
	*   This is a helper function for drawing to a texture or to
		one or more attachments via a framebuffer object.
	*   @param commandBuffer The command buffer
	*   @param graphics The graphics interface for recording graphics commands to the command buffer
	*   @param outputFramebuffer The framebuffer object which provides a set of one or more attachments
	*   to draw to */
	virtual void beginOffscreenRenderPass(D3DCommandList* commandBuffer,
										  D3DGraphics* graphics,
										  D3DFramebuffer* outputFramebuffer);
	/** End render pass.  Every call to beginRenderPass should be accompanied by endRenderPass.
		@param commandBuffer The command buffer for recording graphics commands
		@param graphics      The graphics interface for recording graphics commands to the command buffer */
	virtual void endRenderPass(D3DCommandList* commandBuffer, D3DGraphics* graphics);
	/** End offscreen render pass.  Every call to beginOffscreenRenderPass should be accompanied by endOffscreenRenderPass.
		@param commandBuffer The command buffer for recording graphics commands
		@param graphics      The graphics interface for recording graphics commands to the command buffer */

	virtual void endOffscreenRenderPass(D3DCommandList* commandBuffer,
										D3DGraphics* graphics);
	/** Submit the command buffer to the graphics queue */
	virtual void submit(D3DCommandList* commandBuffer);

	virtual D3DCommandList* drawCommandBuffer(int32_t index = -1);
	virtual D3DCommandList* copyCommandBuffer();
	virtual D3DCommandList* computeCommandBuffer();

	/** \struct AttachmentDescription
	 *
	 *  This struct defines the description for an attachment, including the initial and final layout,
	 *  load operation, and store operation */
	struct AttachmentDescription
	{
		bool operator==(const AttachmentDescription& rhs) const
		{
			return rhs.format == format && rhs.initialLayout == initialLayout &&
				rhs.finalLayout == finalLayout && rhs.loadOp == loadOp && rhs.storeOp == storeOp;
		}
		DXGI_FORMAT format;
		std::optional<ImageLayout> initialLayout, finalLayout;
		AttachmentLoadOp loadOp;
		AttachmentStoreOp storeOp;
	};

	/** \struct RenderPassConfig
	 *
	 *  This struct defines a render pass configuration including the color attachments,
	 *  depth stencil attachment (optional), and number of samples when using multisampling */
	struct RenderPassConfig
	{
		bool operator==(const RenderPassConfig& rhs) const
		{
			return rhs.colorAttachmentDescriptions == colorAttachmentDescriptions &&
				rhs.depthStencilAttachmentDescription ==
				depthStencilAttachmentDescription &&
				rhs.enableDepthStencilResolve == enableDepthStencilResolve &&
				rhs.numSamples == numSamples;
		};
		/** Get the number of color attachments */
		uint32_t numColorAttachments() const
		{
			return uint32_t(colorAttachmentDescriptions.size());
		}
		/** The color attachments */
		std::vector<AttachmentDescription> colorAttachmentDescriptions;
		/** The depth stencil attachment (optional) */
		std::optional<AttachmentDescription> depthStencilAttachmentDescription;
		/** Enable multisampling on the depth / stencil buffer */
		bool enableDepthStencilResolve = false;
		/** The number of samples when using multisampling (set to 1 when not using multisampling) */
		uint32_t numSamples = 1;
	};
	/** Get a render pass object that supports a given configuration */
	virtual D3DRenderPass* getRenderPass(RenderPassConfig config);

	D3DDevice* device = nullptr;
	uint32_t numDrawCommandBuffers = 0;

	std::vector<D3DFramebuffer*> swapchainFramebuffers;
	D3DCommandQueue* queue = nullptr;
	D3DRenderPass* defaultRenderPass = nullptr;
	D3DRenderPass* defaultOffscreenRenderPass = nullptr;
	D3DSwapchain* swapchain = nullptr;
	D3DSurface* surface = nullptr;
	int32_t currentImageIndex = -1;
	std::vector<D3DFence*> frameFences;

	D3DFence* computeFence = nullptr;
	D3DFence* offscreenFence = nullptr;
//	Semaphore* presentCompleteSemaphore = nullptr,
//	Semaphore* renderCompleteSemaphore = nullptr;

	DXGI_FORMAT surfaceFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT defaultOffscreenSurfaceFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_UNKNOWN;

	glm::vec4 clearColor = glm::vec4(0.0f);

	struct D3DRenderPassData
	{
		RenderPassConfig config;
		D3DRenderPass d3dRenderPass;
	};
	std::vector<std::unique_ptr<D3DRenderPassData>> d3dRenderPassCache;
	ComPtr<IDXGIFactory4> d3dFactory;
	D3DDevice d3dDevice;
	D3DCommandQueue d3dCommandQueue;
	D3DDescriptorHeap d3dRtvDescriptorHeap, d3dCbvSrvUavDescriptorHeap,
		d3dSamplerDescriptorHeap, d3dDsvDescriptorHeap;
	D3DPipelineCache d3dPipelineCache;
	std::unique_ptr<D3DSwapchain> d3dSwapchain;
	std::vector<D3DCommandList> d3dDrawCommandLists;
	D3DCommandList d3dOffscreenDrawCommandList;
	D3DCommandList d3dCopyCommandList;
	D3DCommandList d3dComputeCommandList;
	D3DRenderPass* d3dDefaultRenderPass = nullptr;
	D3DRenderPass* d3dDefaultOffscreenRenderPass = nullptr;
	std::vector<D3DFramebuffer> d3dSwapchainFramebuffers;
	std::vector<D3DFence> d3dDrawFences;
	D3DFence d3dCopyFence;
	D3DFence d3dComputeFence;
	D3DFence d3dOffscreenFence;
	std::unique_ptr<D3DTexture> d3dDepthStencilView;
	D3DQueryHeap d3dQueryTimestampHeap;
	D3DReadbackBuffer d3dTimestampResultBuffer;
	bool offscreen = true;
	uint32_t numSamples = 1;
protected:
	bool debug = false, enableDepthStencil = false;
	OnSelectDepthStencilFormats onSelectDepthStencilFormats;
private:
	void createBindings();
	void createDescriptorHeaps();
	void createRenderPass(const RenderPassConfig& config,
						  D3DRenderPass& renderPass);
	void createFences(ID3D12Device* device);
	void createSwapchainFramebuffers(int w, int h);
	DXGI_FORMAT findSupportedFormat(const std::vector<DXGI_FORMAT>& formats, D3D12_FORMAT_SUPPORT1 formatSupport1);
#ifdef ENABLE_GPU_CAPTURE_SUPPORT
	std::unique_ptr<GPUCapture> gpuCapture;
	bool enableGPUCapture = false;
#endif
};

}; // namespace ngli
