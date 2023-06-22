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
#include "D3DSwapchain.h"
#include <backends/d3d12/impl/Config.h>
#include <backends/d3d12/impl/D3DGraphicsContext.h>



namespace ngli
{
#define DEFAULT_SURFACE_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM

void D3DSwapchain::create(D3DGraphicsContext* ctx, D3DSurface* surface)
{
	HRESULT hResult;
	mCtx = ctx;
	auto d3dFactory = mCtx->d3dFactory.Get();
	auto d3dCommandQueue = mCtx->d3dCommandQueue.mID3D12CommandQueue.Get();
	numImages = PREFERRED_NUM_SWAPCHAIN_IMAGES;
	mW = surface->mW; mH = surface->mH;
	mFormat = DEFAULT_SURFACE_FORMAT;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {mW, mH,
										   DXGI_FORMAT(mFormat),
										   FALSE,
										   {1, 0},
										   DXGI_USAGE_RENDER_TARGET_OUTPUT,
										   numImages,
										   DXGI_SCALING_STRETCH,
										   DXGI_SWAP_EFFECT_FLIP_DISCARD,
										   DXGI_ALPHA_MODE_UNSPECIFIED,
										   0 };
	ComPtr<IDXGISwapChain1> swapchain;
	D3D_TRACE_CALL(d3dFactory->CreateSwapChainForHwnd(d3dCommandQueue, surface->mHwnd,
										 &swapChainDesc, nullptr, nullptr,
										 &swapchain));
	D3D_TRACE_CALL(swapchain.As(&v));
	getSwapchainRenderTargets();
	createSwapchainRenderTargetViews(mW, mH);
}

D3DSwapchain::~D3DSwapchain()
{
	mCtx->queue->waitIdle();
}

void D3DSwapchain::getSwapchainRenderTargets()
{
	HRESULT hResult;
	renderTargets.resize(numImages);
	for(UINT j = 0; j < numImages; j++)
	{
		D3D_TRACE_CALL(v->GetBuffer(j, IID_PPV_ARGS(&renderTargets[j])));
		renderTargets[j]->SetName(L"D3DSwapchain-RenderTarget");
	}
}

void D3DSwapchain::createSwapchainRenderTargetViews(uint32_t w, uint32_t h)
{
	HRESULT hResult;
	auto d3dDevice = mCtx->d3dDevice.mID3D12Device.Get();
	auto rtvDescriptorHeap = &mCtx->d3dRtvDescriptorHeap;
	renderTargetDescriptors.resize(numImages);
	for(UINT n = 0; n < numImages; n++)
	{
		D3D_TRACE_CALL(v->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));
		renderTargets[n]->SetName(L"D3DSwapchain-RenderTargetViews");
		auto& handle = renderTargetDescriptors[n];
		handle = std::make_unique<D3DDescriptorHandle>();
		rtvDescriptorHeap->getHandle(*handle);
		D3D_TRACE(d3dDevice->CreateRenderTargetView(
			renderTargets[n].Get(), nullptr, handle->cpuHandle));
	}
}

void D3DSwapchain::acquireNextImage()
{
	mCtx->currentImageIndex = v->GetCurrentBackBufferIndex();
	auto waitFence = mCtx->frameFences[mCtx->currentImageIndex];
	waitFence->wait();
	waitFence->reset();
}

void D3DSwapchain::present()
{
	HRESULT hResult;
	D3D_TRACE_CALL(v->Present(1, 0));
	mCtx->currentImageIndex = -1;
}

void D3DSwapchain::setName(const std::string& name)
{
	mName = name;
	for(uint32_t j = 0; j < numImages; j++)
	{
		auto& rt = renderTargets[j];
		std::wstring wname = StringUtil::toWString(name).c_str() + std::to_wstring(j);
		rt->SetName(wname.c_str());
	}
}

}