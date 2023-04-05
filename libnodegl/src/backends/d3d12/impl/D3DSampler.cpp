/*
 * Copyright 2021 GoPro Inc.
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
#include "D3DSampler.h"
#include <backends/d3d12/impl/D3DGraphicsContext.h>
#include <backends/d3d12/impl/D3DDescriptorHandle.h>

namespace ngli
{


D3DSampler* D3DSampler::newInstance(D3DGraphicsContext* ctx, const D3DSamplerDesc& samplerDesc)
{
	D3DSamplerDesc d3dSamplerDesc(samplerDesc);
	D3DSampler* d3dSampler = new D3DSampler();
	d3dSampler->create(ctx, d3dSamplerDesc);
	return d3dSampler;
}

void D3DSampler::create(D3DGraphicsContext* ctx, const D3DSamplerDesc& samplerDesc)
{
	this->ctx = ctx;
	desc = samplerDesc;
	handle = std::make_unique<D3DDescriptorHandle>();
	ctx->d3dSamplerDescriptorHeap.getHandle(*handle);
	D3D_TRACE(ctx->d3dDevice.mID3D12Device->CreateSampler(&samplerDesc, handle->cpuHandle));
}
}