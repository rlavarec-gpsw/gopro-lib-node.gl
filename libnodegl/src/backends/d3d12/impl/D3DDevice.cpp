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
#include <backends/d3d12/impl/D3DDevice.h>
#include <backends/d3d12/impl/D3DFence.h>
#include <backends/d3d12/impl/D3DGraphicsContext.h>

namespace ngli
{

void D3DDevice::create(D3DGraphicsContext* ctx)
{
	mD3DGraphicsContext = ctx;
	HRESULT hResult;
	auto factory = ctx->d3dFactory.Get();
	hardwareAdapter = nullptr;
	for(UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &hardwareAdapter); adapterIndex++)
	{
		DXGI_ADAPTER_DESC1 desc;
		hardwareAdapter->GetDesc1(&desc);
		if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		const wchar_t* gpu_filter_env = _wgetenv(L"GPU_FILTER");
		std::wstring gpu_filter = gpu_filter_env ? gpu_filter_env : L"";

		//Skip GPUs not matching user filter
		if(wcsstr(desc.Description, gpu_filter.c_str()) == nullptr)
			continue;

		// Check to see if the adapter supports Direct3D 12
		if(SUCCEEDED(D3D12CreateDevice(hardwareAdapter.Get(),
									   D3D_FEATURE_LEVEL_11_0,
									   _uuidof(ID3D12Device), nullptr)))
			break;
	}
	V(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0,
						IID_PPV_ARGS(&mID3D12Device)));
	mID3D12Device->SetName(L"D3DDevice");
}

void D3DDevice::waitIdle()
{
	mD3DGraphicsContext->d3dCommandQueue.waitIdle();
}

}