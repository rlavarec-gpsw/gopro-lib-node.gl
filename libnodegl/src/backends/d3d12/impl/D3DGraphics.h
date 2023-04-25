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

#include <backends/d3d12/impl/D3DFramebuffer.h>

namespace ngli
{
class D3DCommandList;
class D3DPipeline;
class D3DRenderPass;
class D3DSampler;
class D3DComputePipeline;
class D3DGraphicsContext;
class D3DGraphicsPipeline;
class D3DBuffer;

class D3DGraphics
{
public:
  /** Create the graphics module
   *  @param GraphicsContext The graphics context
   */
	static D3DGraphics* newInstance(D3DGraphicsContext* ctx);

	void create() {}
  /** Destroy the graphics module */
	virtual ~D3DGraphics() {}
  /** Begin a compute pass */
	void beginComputePass(D3DCommandList* commandBuffer) {}
  /** End a compute pass */
	void endComputePass(D3DCommandList* commandBuffer) {}
  /** Begin a render pass
  *   @param commandBuffer The graphics command buffer
  *   @param renderPass    The render pass object
  *   @param framebuffer   The target framebuffer
  *   @param clearColor    The clear color
  *   @param clearDepth    The depth buffer clear value
  *   @param clearStencil  The stencil buffer clear value
  */
	void beginRenderPass(D3DCommandList* commandBuffer, D3DRenderPass* renderPass,
						 D3DFramebuffer* framebuffer,
						 float clearDepth = 1.0f,
						 uint32_t clearStencil = 0);
	void setRenderTargets(D3DCommandList* d3dCommandList,
		const std::vector<D3DFramebuffer::D3DAttachment*>& colorAttachments,
		const D3DFramebuffer::D3DAttachment* depthStencilAttachment);
  /** End the render pass
  *   @param commandBuffer The graphics command buffer
  */
	void endRenderPass(D3DCommandList* commandBuffer);
  /** Begin GPU profiling.
	  Use beginProfile/endProfile to profile a group of commands in the command buffer.
  *   @param commandBuffer The command buffer
  */
	void beginProfile(D3DCommandList* commandBuffer);
  /** End GPU profiling
  *   @param commandBuffer The command buffer
  *   @return The profiling result
  */
	uint64_t endProfile(D3DCommandList* commandBuffer);
  /** Bind a buffer as a per-vertex input to the vertex shader module
   *  @param commandBuffer The command buffer
   *  @param buffer The input buffer
   *  @param location The target attribute location
   *  @param stride The size of each element (bytes)
   */
	void bindVertexBuffer(D3DCommandList* commandBuffer, D3DBuffer* buffer,
						  uint32_t location, uint32_t stride);
  /** Bind a buffer of vertex indices, for indexed drawing
   *  @param commandBuffer The command buffer
   *  @param buffer The input buffer
   *  @param indexFormat the format of the indices
   */
	void bindIndexBuffer(D3DCommandList* commandBuffer, D3DBuffer* buffer,
						 IndexFormat indexFormat = INDEXFORMAT_UINT32);
  /** Bind a buffer as uniform input to shader module(s).
  *   A uniform buffer is stored in the GPU's high-speed cache memory, with a size limitation,
	  and the GPU has read-only access.
  *   @param commandBuffer The command buffer
  *   @param buffer The input buffer
  *   @param binding The target binding
  *   @param shaderStageFlags The target shader module(s)
  */
	void bindUniformBuffer(D3DCommandList* commandBuffer, D3DBuffer* buffer,
						   uint32_t binding,
						   ShaderStageFlags shaderStageFlags);
  /** Bind a buffer as storage input to shader module(s).
  *   A shader storage buffer is stored in the GPU's DDR memory
	 (or in shared system memory on systems which have a shared
	 memory architecture, like a mobile device).
	 The GPU has read/write access.
  *   @param commandBuffer The command buffer
  *   @param buffer The input buffer
  *   @param binding The target binding
  *   @param shaderStageFlags The target shader module(s)
  *   @param readonly The buffer is readonly
  */
	void bindStorageBuffer(D3DCommandList* commandBuffer, D3DBuffer* buffer,
						   uint32_t binding,
						   ShaderStageFlags shaderStageFlags, bool readonly);
  /** Bind compute pipeline.
  *   The compute pipeline defines the GPU pipeline parameters to perform compute operations.
  *   @param cmdBuffer The command buffer
  *   @param computePipeline The compute pipeline.
  */
	void bindComputePipeline(D3DCommandList* cmdBuffer,
							 D3DComputePipeline* computePipeline);
  /** Bind graphics pipeline.
  *   The graphics pipeline defines the GPU pipeline parameters to perform graphics operations.
  *   @param cmdBuffer The command buffer
  *   @param computePipeline The graphics pipeline.
  */
	void bindGraphicsPipeline(D3DCommandList* cmdBuffer,
							  D3DGraphicsPipeline* graphicsPipeline);
  /** Bind texture.
  *   If the texture has a built-in sampler, this also binds the sampler.
  *   @param cmdBuffer The command buffer
  *   @param texture The input texture
  *   @param set The descriptor set index
  */
	void bindTexture(D3DCommandList* commandBuffer, D3DTexture* texture,
					 uint32_t set);
  /** Bind texture to an image unit.
 *   This supports random-access reading and writing to the texture from the shader.
 *   @param cmdBuffer The command buffer
 *   @param texture The input texture
 *   @param set The descriptor set index
 */
	void bindTextureAsImage(D3DCommandList* commandBuffer, D3DTexture* texture,
		uint32_t set);
  /** Bind sampler.
	  This allows the GPU shader module to sample a texture.
	  When using a texture with a built-in sampler, this call is not necessary.
	  @param cmdBuffer THe command buffer
	  @param sampler The texture sampler
	  @param set The descriptor set index
  */
	void bindSampler(D3DCommandList* commandBuffer, D3DSampler* sampler,
		uint32_t set);

	// TODO: copyBuffer: ToBuffer, copyBuffer: ToTexture, copyTexture: ToBuffer,
	// blit
	 /** Draw primitives.
  *   The primitive type is speficied in the graphics pipeline object.
  *   This function supports instanced drawing: drawing multiple instances of each primitive.
  *   @param cmdBuffer The command buffer
  *   @param vertexCount The number of vertices
  *   @param instanceCount The number of instances
  *   @param firstVertex The starting vertex index
  *   @param firstInstance The starting instance index
  */
	void draw(D3DCommandList* cmdBuffer, uint32_t vertexCount,
			  uint32_t instanceCount = 1, uint32_t firstVertex = 0,
			  uint32_t firstInstance = 0);
  /** Draw primitives.
  *   The primitive type is speficied in the graphics pipeline object.
  *   This function supports indexed drawing: specfying the vertices indirectly using indexes.
  *   It also supports instanced drawing.
  *   @param cmdBuffer The command buffer
  *   @param indexCount The number of indices
  *   @param instanceCount The number of instances
  *   @param firstIndex The starting index
  *   @param vertexOffset The offset added to the indices
  *   @param firstInstance The starting index of the first instance
  */
	void drawIndexed(D3DCommandList* cmdBuffer, uint32_t indexCount,
					 uint32_t instanceCount = 1, uint32_t firstIndex = 0,
					 int32_t vertexOffset = 0,
					 uint32_t firstInstance = 0);
  /** Dispatch compute worker threads
  *   @param cmdBuffer The command buffer
  *   @param groupCountX, groupCountY, groupCountZ The number of groups (tensor)
  *   @param threadsPerGroupX, threadsPerGroupY, threadsPerGroupZ The number of threads per group (tensor)
  *   If these parameters are already specified in the shader, then it is not necessary to set these parameters in this function call (set to -1).
  */
	void dispatch(D3DCommandList* cmdBuffer, uint32_t groupCountX,
				  uint32_t groupCountY, uint32_t groupCountZ,
				  int32_t threadsPerGroupX = -1, int32_t threadsPerGroupY = -1,
				  int32_t threadsPerGroupZ = -1);
	  /** Set the viewport
  *   This defines the mapping of view coordinates to NDC coordinates.
  *   @param cmdBuffer The command buffer
  *   @param rect The viewport rect
  */
	void setViewport(D3DCommandList* cmdBuffer, rect rect);
  /** Set the scissor rect
  *   @param cmdBuffer The command buffer
  *   @param rect The scissor rect
  */
	void setScissor(D3DCommandList* cmdBuffer, rect rect);

  /** Wait for the GPU to finish executing the command buffer.
  *   @param cmdBuffer The command buffer
  */
	void waitIdle(D3DCommandList* cmdBuffer);
	void resourceBarrier(
		D3DCommandList* cmdList, D3DFramebuffer::D3DAttachment* p,
		D3D12_RESOURCE_STATES currentState, D3D12_RESOURCE_STATES newState,
		UINT subresourceIndex = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	void setDescriptorHeaps(D3DCommandList* commandBuffer);

public:

	rect scissorRect;
	rect viewport;
	D3DPipeline* currentPipeline = nullptr;
	D3DRenderPass* currentRenderPass = nullptr;
	D3DFramebuffer* currentFramebuffer = nullptr;

protected:
	D3DGraphicsContext* mCtx;
};

} // namespace ngli
