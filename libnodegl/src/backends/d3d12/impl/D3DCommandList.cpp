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
#include "D3DCommandList.h"
#include <backends/d3d12/impl/D3DGraphicsContext.h>

namespace ngli
{

D3DCommandList* D3DCommandList::newInstance(D3DGraphicsContext* ctx, D3D12_COMMAND_LIST_TYPE level)
{
    D3DCommandList* d3dCommandList = new D3DCommandList();
    d3dCommandList->create(ctx->d3dDevice.mID3D12Device.Get(), D3D12_COMMAND_LIST_TYPE(level));
    return d3dCommandList;
}

void D3DCommandList::create(ID3D12Device* device,
                            D3D12_COMMAND_LIST_TYPE type)
{
    HRESULT hResult;
    V(device->CreateCommandAllocator(type, IID_PPV_ARGS(&mCmdAllocator)));
    mCmdAllocator->SetName(L"D3DCommandList:mCmdAllocator");
    V(device->CreateCommandList(0, type, mCmdAllocator.Get(), nullptr, IID_PPV_ARGS(&mGraphicsCommandList)));
    mGraphicsCommandList->SetName(L"D3DCommandList:mGraphicsCommandList");
    V(mGraphicsCommandList->Close());
}

void D3DCommandList::begin()
{
    HRESULT hResult;
    V(mCmdAllocator->Reset());
    V(mGraphicsCommandList->Reset(mCmdAllocator.Get(), nullptr));
}

void D3DCommandList::end()
{
    HRESULT hResult;
    V(mGraphicsCommandList->Close());
}

}