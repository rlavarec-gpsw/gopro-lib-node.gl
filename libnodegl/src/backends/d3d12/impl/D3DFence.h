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
#include <backends/d3d12/impl/D3DDevice.h>
#include <backends/d3d12/impl/D3DGraphicsCore.h>

/** \class D3DFence
 *
 *  This class implements a fence synchronization mechanism.
 *  The CPU waits for the fence to be signaled by the GPU when an operation is completed.
 */

namespace ngli
{
class D3DFence
{
public:
	enum Value { UNSIGNALED, SIGNALED };

  /** Create the fence object
   *  @param device The GPU device handle
   *  @param flags Additional fence create flags (optional) */
	static D3DFence* newInstance(D3DDevice* device, Value flag = UNSIGNALED);

	void create(ID3D12Device* device, Value flag = UNSIGNALED);

	/** Destroy the fence object */
	virtual ~D3DFence() {}

	/** Wait for the fence to be signaled by the GPU */
	virtual void wait();

	/** Poll to see if the fence has been signaled by the GPU */
	virtual bool isSignaled();

	/** Reset the fence */
	virtual void reset();

	ComPtr<ID3D12Fence> mID3D12Fence;
	HANDLE mFenceHandle;

private:
	ID3D12Device* device;
};
} // namespace ngli
