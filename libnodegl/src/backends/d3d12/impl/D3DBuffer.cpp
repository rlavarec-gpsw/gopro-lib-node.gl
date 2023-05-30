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

extern "C" {
	#include "Buffer.h"
}

namespace ngli
{

D3DBuffer* D3DBuffer::newInstance(D3DGraphicsContext* ctx, const void* data, uint32_t size, int usageFlags)
{
	D3DBuffer* d3dBuffer = new D3DBuffer();
	d3dBuffer->init(ctx, data, size, usageFlags);
	return d3dBuffer;
}

bool D3DBuffer::init(D3DGraphicsContext* ctx, const void* data, uint32_t size, int bufferUsageFlags)
{
	mBufferUsageFlags = bufferUsageFlags;
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;

	if(bufferUsageFlags & NGLI_BUFFER_USAGE_TRANSFER_SRC_BIT)
		heapType = D3D12_HEAP_TYPE_UPLOAD;
	/*
	if(bufferUsageFlags & NGLI_BUFFER_USAGE_MAP_READ)
		heapType = D3D12_HEAP_TYPE_READBACK;*/

	// Cannot upload and readback
//	ngli_assert((bufferUsageFlags & NGLI_BUFFER_USAGE_TRANSFER_SRC_BIT) &&
//				!(bufferUsageFlags & NGLI_BUFFER_USAGE_MAP_READ));

	
	D3D12_HEAP_FLAGS heapFlag = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

	/*
		NGLI_BUFFER_USAGE_DYNAMIC_BIT = 1 << 0,
//		NGLI_BUFFER_USAGE_TRANSFER_SRC_BIT = 1 << 1,
		NGLI_BUFFER_USAGE_TRANSFER_DST_BIT = 1 << 2,
		NGLI_BUFFER_USAGE_UNIFORM_BUFFER_BIT = 1 << 3,
//		NGLI_BUFFER_USAGE_STORAGE_BUFFER_BIT = 1 << 4,
//		NGLI_BUFFER_USAGE_INDEX_BUFFER_BIT = 1 << 5,
//		NGLI_BUFFER_USAGE_VERTEX_BUFFER_BIT = 1 << 6,
		NGLI_BUFFER_USAGE_MAP_READ = 1 << 7,
		NGLI_BUFFER_USAGE_MAP_WRITE = 1 << 8,*/

	D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
	if(bufferUsageFlags & NGLI_BUFFER_USAGE_STORAGE_BUFFER_BIT)
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	
	if((bufferUsageFlags & NGLI_BUFFER_USAGE_MAP_WRITE) || (bufferUsageFlags & NGLI_BUFFER_USAGE_MAP_READ))
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;

	if(bufferUsageFlags & NGLI_BUFFER_USAGE_INDEX_BUFFER_BIT)
		initialResourceState |= D3D12_RESOURCE_STATE_INDEX_BUFFER;

	if(bufferUsageFlags & NGLI_BUFFER_USAGE_VERTEX_BUFFER_BIT)
		initialResourceState |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

	return init(ctx, data, size, heapType, heapFlag, resourceFlags, initialResourceState);
}

bool D3DBuffer::init(D3DGraphicsContext* ctx, const void* data, uint32_t size,
					   D3D12_HEAP_TYPE heapType,
					   D3D12_HEAP_FLAGS heapFlag,
					   D3D12_RESOURCE_FLAGS resourceFlags,
					   D3D12_RESOURCE_STATES initialResourceState)
{
	HRESULT hResult;
	mCtx = ctx;
	mSize = size;
	mHeapType = heapType;
	mHeapFlag = heapFlag;
	mResourceFlags = resourceFlags;
	mInitialResourceState = initialResourceState;
	CD3DX12_HEAP_PROPERTIES heapProperties(heapType);
	CD3DX12_RESOURCE_DESC resourceDesc =
		CD3DX12_RESOURCE_DESC::Buffer(size, resourceFlags);
	D3D_TRACE_CALL(mCtx->d3dDevice.mID3D12Device->CreateCommittedResource(&heapProperties, heapFlag,
									  &resourceDesc, initialResourceState,
									  nullptr, IID_PPV_ARGS(&mID3D12Resource)));
	if(mID3D12Resource)
		mID3D12Resource->SetName(L"D3DBuffer");
	mCurrentResourceState = initialResourceState;
	if(data)
		upload(data, size, 0);
	if(FAILED(hResult))
		return false;
	return true;
}

D3DBuffer::~D3DBuffer() {

}

// TODO: add read/write flags for map
// current this maps for reading (readback buffer)
void* D3DBuffer::map()
{
	if((mBufferUsageFlags & NGLI_BUFFER_USAGE_MAP_READ) || (mBufferUsageFlags & NGLI_BUFFER_USAGE_MAP_WRITE))
//	if(mHeapType == D3D12_HEAP_TYPE_READBACK)
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
		mIsMap = true;
		UINT8* ptr;
		HRESULT hResult;
		D3D_TRACE_CALL(mID3D12Resource->Map(0, nullptr, reinterpret_cast<void**>(&ptr)));
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
		if(mIsMap)
		{
			mIsMap = false;
			D3D_TRACE(mID3D12Resource->Unmap(0, nullptr));
		}
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
			if(stagingBuffer.mID3D12Resource)
			{
				stagingBuffer.mID3D12Resource->SetName(L"StagingBuffer");
			}

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
		if(data)
		{
			uint8_t* dst = (uint8_t*)map();
			memcpy(dst + offset, data, size);
			unmap();
		}
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