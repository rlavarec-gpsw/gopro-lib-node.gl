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
#include "texture.h"
}

namespace ngli
{
class D3DBuffer;
class D3DCommandList;
class D3DGraphics;
class D3DGraphicsContext;
class D3DBlitOp;
class D3DDescriptorHandle;
class D3DSampler;
class D3DReadbackBuffer;
struct D3DSamplerDesc;

class D3DTexture
{
public:
  /** Create a texture
   *  @param graphicsContext The graphics context
   *  @param graphics The graphics object
   *  @param data The texture data.  The user can pass nullptr to allocate a blank texture
   *  @param format The pixel format
   *  @param size   The texture size
   *  @param w      The texture width
   *  @param h      The texture height
   *  @param d      The texture depth
   *  @param arrayLayers The number of array layers
   *  @param imageUsageFlags A bitmask of flags describing the image usage
   *  @param textureType The texture type
   *  @param genMipmaps If true, generate mipmaps
   *  @param numSamples The number of samples when using multisampling
   *  @param samplerDesc The sampler description, or nullptr if the texture is not sampled
   *  @param dataPitch The pitch (size of each row of the input data in bytes, including padding, or -1 if there's no padding)
   */
	static D3DTexture*
		newInstance(D3DGraphicsContext* graphicsContext, D3DGraphics* graphics, void* data,
			   DXGI_FORMAT format, uint32_t size, uint32_t w, uint32_t h, uint32_t d,
			   uint32_t arrayLayers,
			   ImageUsageFlags imageUsageFlags = (NGLI_TEXTURE_USAGE_SAMPLED_BIT | NGLI_TEXTURE_USAGE_TRANSFER_SRC_BIT | NGLI_TEXTURE_USAGE_TRANSFER_DST_BIT),
			   TextureType textureType = TEXTURE_TYPE_2D, bool genMipmaps = false,
			   uint32_t numSamples = 1, const D3DSamplerDesc* samplerDesc = nullptr, int32_t dataPitch = -1);

	void init(D3DGraphicsContext* ctx, D3DGraphics* graphics, void* data,
				uint32_t size, uint32_t w, uint32_t h, uint32_t d,
				uint32_t arrayLayers, DXGI_FORMAT format,
				ImageUsageFlags usageFlags, TextureType textureType = TEXTURE_TYPE_2D,
				bool genMipmaps = false, uint32_t numSamples = 1,
				const D3DSamplerDesc* samplerDesc = nullptr, int32_t dataPitch = -1);
	void createFromHandle(D3DGraphicsContext* ctx, D3DGraphics* graphics, void* handle,
				uint32_t w, uint32_t h, uint32_t d,
				uint32_t arrayLayers, DXGI_FORMAT format,
				ImageUsageFlags usageFlags, TextureType textureType,
				uint32_t numSamples,
				const D3DSamplerDesc* samplerDesc = nullptr);
	void updateFromHandle(void* handle);
	virtual ~D3DTexture();
	void upload(void* data, uint32_t size, uint32_t x = 0, uint32_t y = 0,
				uint32_t z = 0, int32_t w = -1, int32_t h = -1, int32_t d = -1,
				int32_t arrayLayers = -1, int32_t numPlanes = -1,
				int32_t dataPitch = -1);
	void download(void* data, uint32_t size, uint32_t x = 0, uint32_t y = 0,
				 uint32_t z = 0, int32_t w = -1, int32_t h = -1, int32_t d = -1,
				 int32_t arrayLayers = -1, int32_t numPlanes = -1);
	void changeLayout(D3DCommandList* commandBuffer,
					  ImageLayout imageLayout);
	void
		resourceBarrierTransition(D3DCommandList* cmdList, D3D12_RESOURCE_STATES newState,
						UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	void generateMipmaps(D3DCommandList* commandBuffer);
	D3DDescriptorHandle* getRtvDescriptor(uint32_t level = 0,
										 uint32_t baseLayer = 0,
										 uint32_t layerCount = 1,
										 uint32_t plane = 0);
	D3DSampler* getSampler(D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_POINT);
	D3DDescriptorHandle* getSrvDescriptor(uint32_t baseMipLevel,
										 uint32_t numMipLevels, uint32_t plane = 0);
	D3DDescriptorHandle* getUavDescriptor(uint32_t mipLevel, uint32_t plane = 0);
	void setName(const std::string& name);

public:
	ComPtr<ID3D12Resource> mID3D12Resource;

	struct RtvData
	{
		D3D12_RENDER_TARGET_VIEW_DESC desc;
		D3DDescriptorHandle* handle = nullptr;
	};
	std::vector<RtvData> rtvDescriptorCache;
	std::vector<std::unique_ptr<D3DSampler>> samplerCache;
	std::vector<D3DDescriptorHandle*> defaultSrvDescriptor;
	uint32_t numPlanes = 1;
	std::vector<D3DDescriptorHandle*> defaultRtvDescriptor;
	std::vector<D3DDescriptorHandle*> defaultUavDescriptor;
	std::unique_ptr<D3DDescriptorHandle> dsvDescriptor;
	std::vector<std::unique_ptr<D3DDescriptorHandle>> cbvSrvUavDescriptors;
	std::vector<std::unique_ptr<D3DDescriptorHandle>> rtvDescriptors;
	D3DSampler* defaultSampler = nullptr;

	struct SrvData
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc;
		D3DDescriptorHandle* handle = nullptr;
		uint32_t plane = 0;
	};
	std::vector<SrvData> srvDescriptorCache;
	struct UavData
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
		D3DDescriptorHandle* handle = nullptr;
	};
	std::vector<UavData> uavDescriptorCache;
	D3D12_RESOURCE_DESC resourceDesc;
	D3D12_RESOURCE_FLAGS resourceFlags;
	std::vector<D3D12_RESOURCE_STATES> mCurrentResourceState;
	uint32_t numSubresources = 0;
	bool isRenderTarget = false;

	struct GenMipmapData
	{
		std::vector<D3DBlitOp> ops;
	};
	std::unique_ptr<GenMipmapData> genMipmapData;

	std::string name;
	DXGI_FORMAT format;
	uint32_t w = 0, h = 0, d = 1, arrayLayers = 1, mipLevels = 1, numSamples = 1;
	uint32_t size = 0;
	std::vector<uint32_t> planeWidth, planeHeight, planeSize;
	ImageUsageFlags imageUsageFlags;
	TextureType textureType;
private:
	void getResourceDesc();
	void createResource();
	void createDepthStencilView();
	void downloadFn(D3DCommandList* cmdList, D3DReadbackBuffer& readbackBuffer,
					D3D12_BOX& srcRegion,
					D3D12_PLACED_SUBRESOURCE_FOOTPRINT& dstFootprint,
					int subresourceIndex = 0);
	void uploadFn(D3DCommandList* cmdList, void* data, uint32_t size,
				  D3DBuffer* stagingBuffer, uint32_t x = 0, uint32_t y = 0,
				  uint32_t z = 0, int32_t w = -1, int32_t h = -1, int32_t d = -1,
				  int32_t arrayLayers = -1, int32_t numPlanes = -1,
				  int32_t dataPitch = -1);
	void generateMipmapsFn(D3DCommandList* cmdList);
	DXGI_FORMAT getViewFormat(DXGI_FORMAT resourceFormat, uint32_t planeIndex = 0);
	D3D12_RENDER_TARGET_VIEW_DESC
		getRtvDesc(TextureType textureType, DXGI_FORMAT format, uint32_t numSamples,
			uint32_t level, uint32_t baseLayer, uint32_t layerCount, uint32_t plane);
	D3DGraphicsContext* ctx = nullptr;
	D3DGraphics* graphics = nullptr;
	ID3D12Device* d3dDevice = nullptr;
};

} // namespace ngli
