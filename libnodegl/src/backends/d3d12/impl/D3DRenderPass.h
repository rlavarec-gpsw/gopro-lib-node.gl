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
extern "C" {
#include "rendertarget.h"
}

namespace ngli {
class D3DGraphicsContext;
class D3DFramebuffer;
/** \class D3DRenderPass
   *
   *  This class defines the base class for a render pass
   *  which allows rendering to an onscreen or offscreen surface
   *  via a framebuffer object.
   *  The user will typically call the getRenderPass member function
   *  in the graphics context to get a render pass object corresponding
   *  to a render pass 
   uration
   *
   */
class D3DRenderPass {
public:
  void create(D3DGraphicsContext *ctx,
              D3D12_RESOURCE_STATES initialResourceState,
              D3D12_RESOURCE_STATES finalResourceState,
              AttachmentLoadOp colorLoadOp = NGLI_LOAD_OP_CLEAR,
              AttachmentStoreOp colorStoreOp = NGLI_STORE_OP_STORE,
              AttachmentLoadOp depthLoadOp = NGLI_LOAD_OP_CLEAR,
              AttachmentStoreOp depthStoreOp = NGLI_STORE_OP_DONT_CARE);
  virtual ~D3DRenderPass() {}
  D3D12_RESOURCE_STATES initialResourceState, finalResourceState;
  AttachmentLoadOp colorLoadOp, depthLoadOp;
  AttachmentStoreOp colorStoreOp, depthStoreOp;

private:
  D3DGraphicsContext *ctx;
  D3DFramebuffer* currentFramebuffer = nullptr;
};
} // namespace ngli