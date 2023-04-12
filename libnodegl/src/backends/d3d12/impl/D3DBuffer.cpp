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
#include "D3DBuffer.h"
#include <backends/d3d12/impl/D3DGraphicsContext.h>
#include <backends/d3d12/impl/D3DReadbackBuffer.h>
#include <backends/d3d12/impl/D3DDevice.h>

namespace ngli
{

D3DBuffer* D3DBuffer::newInstance(D3DGraphicsContext* ctx, const void* data, uint32_t size, BufferUsageFlags usageFlags)
{
	D3DBuffer* d3dBuffer = new D3DBuffer();
	d3dBuffer->init(ctx, data, size, usageFlags);
	return d3dBuffer;
}

void D3DBuffer::init(D3DGraphicsContext* ctx, const void* data, uint32_t size, BufferUsageFlags bufferUsageFlags)
{
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
	if(bufferUsageFlags & BUFFER_USAGE_TRANSFER_SRC_BIT)
		heapType = D3D12_HEAP_TYPE_UPLOAD;
		
	D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
	if(bufferUsageFlags & BUFFER_USAGE_STORAGE_BUFFER_BIT)
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
	//if (bufferUsageFlags & BUFFER_USAGE_VERTEX_BUFFER_BIT || bufferUsageFlags & BUFFER_USAGE_UNIFORM_BUFFER_BIT)
	//    initialResourceState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	init(ctx, data, size, heapType, resourceFlags, initialResourceState);
}

void D3DBuffer::init(D3DGraphicsContext* ctx, const void* data, uint32_t size,
					   D3D12_HEAP_TYPE heapType,
					   D3D12_RESOURCE_FLAGS resourceFlags,
					   D3D12_RESOURCE_STATES initialResourceState)
{
	HRESULT hResult;
	mCtx = ctx;
	mSize = size;
	mHeapType = heapType;
	mInitialResourceState = initialResourceState;
	CD3DX12_HEAP_PROPERTIES heapProperties(heapType);
	CD3DX12_RESOURCE_DESC resourceDesc =
		CD3DX12_RESOURCE_DESC::Buffer(size, resourceFlags);
	V(mCtx->d3dDevice.mID3D12Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
									  &resourceDesc, initialResourceState,
									  nullptr, IID_PPV_ARGS(&mID3D12Resource)));
	mCurrentResourceState = initialResourceState;
	if(data)
		upload(data, size, 0);
}

D3DBuffer::~D3DBuffer() {}

// TODO: add read/write flags for map
// current this maps for reading (readback buffer)
void* D3DBuffer::map()
{
	if(mHeapType == D3D12_HEAP_TYPE_DEFAULT)
	{
		assert(d3dReadbackBuffer == nullptr);
		auto& copyCommandList = mCtx->d3dCopyCommandList;
		copyCommandList.begin();
		d3dReadbackBuffer = new D3DReadbackBuffer();
		d3dReadbackBuffer->create(mCtx, mSize);
		if(mCurrentResourceState != D3D12_RESOURCE_STATE_COPY_SOURCE)
		{
			CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mID3D12Resource.Get(), mCurrentResourceState, D3D12_RESOURCE_STATE_COPY_SOURCE);
			D3D_TRACE(copyCommandList.mGraphicsCommandList.Get()->ResourceBarrier(1, &resourceBarrier));
		}
		D3D_TRACE(copyCommandList.mGraphicsCommandList.Get()->CopyBufferRegion(
			d3dReadbackBuffer->mID3D12Resource.Get(), 0, mID3D12Resource.Get(), 0, mSize));
		CD3DX12_RESOURCE_BARRIER resourceBarrier =
			CD3DX12_RESOURCE_BARRIER::Transition(mID3D12Resource.Get(),
												 D3D12_RESOURCE_STATE_COPY_SOURCE,
												 D3D12_RESOURCE_STATE_GENERIC_READ);
		D3D_TRACE(copyCommandList.mGraphicsCommandList.Get()->ResourceBarrier(1, &resourceBarrier));
		copyCommandList.end();
		mCtx->d3dCommandQueue.submit(&copyCommandList);
		mCtx->d3dCommandQueue.waitIdle();
		d3dReadBackBufferPtr = d3dReadbackBuffer->map();
		return d3dReadBackBufferPtr;
	}
	else
	{
		UINT8* ptr;
		HRESULT hResult;
		V(mID3D12Resource->Map(0, nullptr, reinterpret_cast<void**>(&ptr)));
		return ptr;
	}
}

void D3DBuffer::unmap()
{
	if(mHeapType == D3D12_HEAP_TYPE_DEFAULT)
	{
		assert(d3dReadbackBuffer);
		upload(d3dReadBackBufferPtr, mSize, 0);
		d3dReadbackBuffer->unmap();
		delete d3dReadbackBuffer; //TODO: queue for delete
		d3dReadbackBuffer = nullptr;
		d3dReadBackBufferPtr = nullptr;
	}
	else
	{
		D3D_TRACE(mID3D12Resource->Unmap(0, nullptr));
	}
}

void D3DBuffer::upload(const void* data, uint32_t size, uint32_t offset)
{
	if(mHeapType == D3D12_HEAP_TYPE_DEFAULT)
	{
		auto& copyCommandList = mCtx->d3dCopyCommandList;
		copyCommandList.begin();
		D3DBuffer stagingBuffer;
		if(data)
		{
			stagingBuffer.init(mCtx, data, size, D3D12_HEAP_TYPE_UPLOAD);
			stagingBuffer.mID3D12Resource->SetName(L"StagingBuffer");
			if(mCurrentResourceState != D3D12_RESOURCE_STATE_COPY_DEST)
			{
				CD3DX12_RESOURCE_BARRIER resourceBarrier =
					CD3DX12_RESOURCE_BARRIER::Transition(mID3D12Resource.Get(), mCurrentResourceState, D3D12_RESOURCE_STATE_COPY_DEST);
				D3D_TRACE(copyCommandList.mGraphicsCommandList.Get()->ResourceBarrier(1, &resourceBarrier));
				mCurrentResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
			}
			D3D_TRACE(copyCommandList.mGraphicsCommandList.Get()->CopyBufferRegion(
				mID3D12Resource.Get(), 0, stagingBuffer.mID3D12Resource.Get(), 0, size));
		}
		if(mCurrentResourceState != mInitialResourceState)
		{
			CD3DX12_RESOURCE_BARRIER resourceBarrier =
				CD3DX12_RESOURCE_BARRIER::Transition(mID3D12Resource.Get(), mCurrentResourceState, mInitialResourceState);
			D3D_TRACE(copyCommandList.mGraphicsCommandList.Get()->ResourceBarrier(1, &resourceBarrier));
		}
		copyCommandList.end();
		mCtx->d3dCommandQueue.submit(&copyCommandList);
		mCtx->d3dCommandQueue.waitIdle();
		mCurrentResourceState = mInitialResourceState;
	}
	else
	{
		uint8_t* dst = (uint8_t*)map();
		memcpy(dst + offset, data, size);
		unmap();
	}
}

void D3DBuffer::download(void* data, uint32_t size, uint32_t offset)
{
	uint8_t* src = (uint8_t*)map();
	memcpy(data, src + offset, size);
	unmap();
}

void D3DBuffer::setName(const std::string& name)
{
	mName = name;
	mID3D12Resource->SetName(StringUtil::toWString(name).c_str());
}


}