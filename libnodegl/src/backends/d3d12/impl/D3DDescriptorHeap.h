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
#include <backends/d3d12/impl/D3DDescriptorHandle.h>

namespace ngli
{
class D3DDescriptorHeap
{
public:
	void create(ID3D12Device* d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE type,
				UINT maxDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
	~D3DDescriptorHeap();
	bool getHandle(D3DDescriptorHandle& handle);
	void freeHandle(D3DDescriptorHandle* handle);
	UINT maxDescriptors;
	D3D12_DESCRIPTOR_HEAP_TYPE type;
	ComPtr<ID3D12DescriptorHeap> mID3D12DescriptorHeap;
private:
	std::unique_ptr<D3DDescriptorHandle> head;
	int index = 0;
	uint32_t descriptorSize = 0;
	std::vector<uint8_t> state;
	std::mutex threadMutex;
	int numDescriptors = 0;
};
} // namespace ngli