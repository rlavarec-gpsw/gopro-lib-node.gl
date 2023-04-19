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
#include "D3DFence.h"
#include <backends/d3d12/impl/D3DDevice.h>

namespace ngli
{

D3DFence* D3DFence::newInstance(D3DDevice* device, Value flag)
{
    auto d3dFence = new D3DFence();
    d3dFence->create(device->mID3D12Device.Get(), flag);
    return d3dFence;
}

void D3DFence::create(ID3D12Device* device, Value flag)
{
    HRESULT hResult;
    D3D_TRACE_CALL(device->CreateFence(flag, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mID3D12Fence)));
    mID3D12Fence->SetName(L"D3DFence");
    mFenceHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if(mFenceHandle == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
}

void D3DFence::wait()
{
    HRESULT hResult;
    if(mID3D12Fence->GetCompletedValue() == SIGNALED)
        return;
    D3D_TRACE_CALL(mID3D12Fence->SetEventOnCompletion(SIGNALED, mFenceHandle));
    D3D_TRACE(WaitForSingleObjectEx(mFenceHandle, INFINITE, FALSE));
}

bool D3DFence::isSignaled()
{
    return (mID3D12Fence->GetCompletedValue() == SIGNALED);
}

void D3DFence::reset()
{
    HRESULT hResult;
    D3D_TRACE_CALL(mID3D12Fence->Signal(UNSIGNALED));
}

}