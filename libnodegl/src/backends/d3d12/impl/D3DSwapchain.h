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
#include <backends/d3d12/impl/D3DDescriptorHeap.h>
#include <backends/d3d12/impl/D3DDevice.h>
#include <backends/d3d12/impl/D3DSurface.h>
#include <backends/d3d12/impl/D3DGraphicsCore.h>
#include <backends/d3d12/impl/D3DGraphicsContext.h>

/** \class Swapchain
 *  This class provides the base class for a swapchain abstraction
 *  A swapchain contains a set of one or more images that are presented to the
 *  display
 */

namespace ngli
{

class D3DSwapchain
{
public:
	void create(D3DGraphicsContext* ctx, D3DSurface* surface);
	virtual ~D3DSwapchain();
	void acquireNextImage();
	void present();
	void setName(const std::string& name);

	ComPtr<IDXGISwapChain3> v;
	std::vector<ComPtr<ID3D12Resource>> renderTargets;
	std::vector<std::unique_ptr<D3DDescriptorHandle>> renderTargetDescriptors;

	uint32_t numImages = 0;
	uint32_t mW = 0;
	uint32_t mH = 0;
	DXGI_FORMAT mFormat;
	std::string mName;

private:
	void getSwapchainRenderTargets();
	void createSwapchainRenderTargetViews(uint32_t w, uint32_t h);
	D3DGraphicsContext* mCtx=nullptr;
};

} // namespace ngli