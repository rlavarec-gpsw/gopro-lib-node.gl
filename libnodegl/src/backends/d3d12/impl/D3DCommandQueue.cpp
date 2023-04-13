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
#include "D3DCommandQueue.h"
#include <backends/d3d12/impl/D3DCommandList.h>
#include <backends/d3d12/impl/D3DGraphicsContext.h>

namespace ngli
{

void D3DCommandQueue::create(D3DGraphicsContext* ctx)
{
	HRESULT hResult;
	this->ctx = ctx;
	auto d3dDevice = ctx->d3dDevice.mID3D12Device.Get();
	D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT, 0,
										  D3D12_COMMAND_QUEUE_FLAG_NONE,
										  D3D12_COMMAND_LIST_TYPE_DIRECT };
	V(d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mID3D12CommandQueue)));
	mID3D12CommandQueue->SetName(L"D3DCommandQueue");
}

void D3DCommandQueue::present()
{
	ctx->d3dSwapchain->present();
}

void D3DCommandQueue::submit(D3DCommandList* commandBuffer)
{
	ID3D12CommandList* d3dCommandList = commandBuffer->mGraphicsCommandList.Get();
	ID3D12Fence* fence = nullptr;
	if(d3dCommandList == ctx->d3dCopyCommandList.mGraphicsCommandList.Get())
	{
		fence = ctx->d3dCopyFence.mID3D12Fence.Get();
	}
	else if(d3dCommandList == ctx->d3dComputeCommandList.mGraphicsCommandList.Get())
	{
	}
	else if(d3dCommandList == ctx->d3dOffscreenDrawCommandList.mGraphicsCommandList.Get())
	{
		fence = ctx->d3dOffscreenFence.mID3D12Fence.Get();
	}
	else
	{
		fence = ctx->d3dDrawFences[ctx->currentImageIndex].mID3D12Fence.Get();
	}
	submit(d3dCommandList, fence);
}

void D3DCommandQueue::submit(ID3D12CommandList* commandList, ID3D12Fence* fence)
{
	HRESULT hResult;
	D3D_TRACE(mID3D12CommandQueue->ExecuteCommandLists(1, &commandList));
	if(fence)
		V(mID3D12CommandQueue->Signal(fence, D3DFence::SIGNALED));
}

void D3DCommandQueue::signal(D3DFence* fence, D3DFence::Value value)
{
	HRESULT hResult;
	V(mID3D12CommandQueue->Signal(fence->mID3D12Fence.Get(), value));
}

void D3DCommandQueue::waitIdle()
{
	HRESULT hResult;
	D3DFence fence;
	fence.create(ctx->d3dDevice.mID3D12Device.Get());
	// Schedule a Signal command in the queue.
	V(mID3D12CommandQueue->Signal(fence.mID3D12Fence.Get(), D3DFence::SIGNALED));
	fence.wait();
	fence.reset();
}
}
