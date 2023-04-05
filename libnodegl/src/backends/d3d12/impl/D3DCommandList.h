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

#include <d3d12.h>

namespace ngli
{

class D3DGraphicsContext;

/** \class D3DCommandList
 *
 *  This class supports GPU command buffer operations,
 *  including recording draw commands and submitting them to the GPU.
 *  It supports primary and secondary command buffers.
 *  Secondary command buffers can be recorded in parallel, using multiple threads,
 *  and can be added to a primary command buffer.
 */

class D3DCommandList
{
public:
  /** Create the command buffer
   *  @param ctx The graphics context
   *  @param level The command buffer level
   */
    static D3DCommandList* newInstance(D3DGraphicsContext* ctx,
               D3D12_COMMAND_LIST_TYPE level = D3D12_COMMAND_LIST_TYPE_DIRECT);

    void create(ID3D12Device* device,
                D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
    ~D3DCommandList() {}

    void begin();
    void end();

    ComPtr<ID3D12GraphicsCommandList> mGraphicsCommandList;

private:
    ComPtr<ID3D12CommandAllocator> mCmdAllocator;
};

} // namespace ngli
