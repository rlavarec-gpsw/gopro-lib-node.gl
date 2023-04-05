/*
 * Copyright 2020 GoPro Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.    See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.    The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.    You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.    See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#pragma once

namespace ngli
{
class D3DGraphicsContext;
class D3DReadbackBuffer;
class D3DCommandList;
class D3DFence;

class D3DBuffer
{
public:
	static D3DBuffer* newInstance(D3DGraphicsContext* ctx, const void* data, uint32_t size, BufferUsageFlags usageFlags);

	void init(D3DGraphicsContext* ctx, const void* data, uint32_t size, BufferUsageFlags bufferUsageFlags);

	void init(D3DGraphicsContext* ctx, const void* data, uint32_t size,
				D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT,
				D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE,
				D3D12_RESOURCE_STATES initialResourceState =
				D3D12_RESOURCE_STATE_GENERIC_READ);
	virtual ~D3DBuffer();
	void* map();
	void unmap();
	void upload(const void* data, uint32_t size, uint32_t offset = 0);
	void download(void* data, uint32_t size, uint32_t offset = 0);
	void setName(const std::string& name);

	ComPtr<ID3D12Resource> mID3D12Resource;

	std::string mName;
	uint32_t mSize = 0;

protected:

	D3DGraphicsContext* mCtx = nullptr;
	D3D12_HEAP_TYPE mHeapType;
	D3D12_RESOURCE_STATES mInitialResourceState;
	D3D12_RESOURCE_STATES mCurrentResourceState;
	D3DReadbackBuffer* d3dReadbackBuffer = nullptr;
	void* d3dReadBackBufferPtr = nullptr;
};

} // namespace ngli
