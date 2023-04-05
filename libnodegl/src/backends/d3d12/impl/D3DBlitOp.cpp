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
#include "D3DBlitOp.h"
#include <backends/d3d12/impl/D3DGraphics.h>
#include <backends/d3d12/impl/D3DGraphicsContext.h>
#include <backends/d3d12/impl/D3DTexture.h>
#include <backends/d3d12/impl/D3DBufferUtils.h>
#include <backends/d3d12/impl/D3DGraphicsPipeline.h>
#include <backends/d3d12/impl/D3DShaderModule.h>
#include <backends/d3d12/impl/D3DSampler.h>

namespace ngli
{


D3DBlitOp::D3DBlitOp(D3DGraphicsContext* ctx, D3DTexture* srcTexture,
					 uint32_t srcLevel, D3DTexture* dstTexture,
					 uint32_t dstLevel, Region srcRegion, Region dstRegion,
					 uint32_t srcBaseLayer, uint32_t srcLayerCount,
					 uint32_t dstBaseLayer, uint32_t dstLayerCount)
	: ctx(ctx), srcTexture(srcTexture), srcLevel(srcLevel),
	dstTexture(dstTexture), dstLevel(dstLevel), srcRegion(srcRegion),
	dstRegion(dstRegion), srcBaseLayer(srcBaseLayer),
	srcLayerCount(srcLayerCount), dstBaseLayer(dstBaseLayer),
	dstLayerCount(dstLayerCount)
{
	outputFramebuffer.reset((D3DFramebuffer*)D3DFramebuffer::newInstance(
		ctx->device, ctx->defaultOffscreenRenderPass, { {dstTexture, dstLevel, 0} },
		dstTexture->w >> dstLevel, dstTexture->h >> dstLevel));
	std::vector<glm::vec2> pos = { glm::vec2(-1, 1), glm::vec2(-1, -1), glm::vec2(1, 1), glm::vec2(1, -1) };
	std::vector<glm::vec2> texCoord = { glm::vec2(0, 0), glm::vec2(0, 1), glm::vec2(1, 0), glm::vec2(1, 1) };
	bPos.reset((D3DBuffer*)createVertexBuffer<glm::vec2>(ctx, pos));
	bTexCoord.reset((D3DBuffer*)createVertexBuffer<glm::vec2>(ctx, texCoord));
	numVerts = uint32_t(pos.size());
	UBOData uboData = { srcLevel };
	bUbo.reset((D3DBuffer*)createUniformBuffer(ctx, &uboData, sizeof(uboData)));
	createPipeline();
	graphicsPipeline->getBindings({ &U_UBO, &U_TEXTURE }, { &B_POS, &B_TEXCOORD });
}

void D3DBlitOp::createPipeline()
{
	const std::string key = "d3dBlitOp";
	graphicsPipeline = (D3DGraphicsPipeline*)ctx->d3dPipelineCache.get(key);
	if(graphicsPipeline)
		return;
	D3DGraphicsPipeline::State state;
	state.primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	auto device = ctx->device;
	graphicsPipeline = (D3DGraphicsPipeline*)D3DGraphicsPipeline::newInstance(
		ctx, state,
		D3DVertexShaderModule::newInstance(device, NGLI_DATA_DIR "/d3dBlitOp.vert").get(),
		D3DFragmentShaderModule::newInstance(device, NGLI_DATA_DIR "/d3dBlitOp.frag")
			.get(),
		dstTexture->format, ctx->depthStencilFormat);
	ctx->d3dPipelineCache.add(key, graphicsPipeline);
}

void D3DBlitOp::draw(D3DCommandList* cmdList, D3DGraphics* graphics)
{
	graphics->bindGraphicsPipeline(cmdList, graphicsPipeline);
	graphics->bindVertexBuffer(cmdList, bPos.get(), B_POS, sizeof(glm::vec2));
	graphics->bindUniformBuffer(cmdList, bUbo.get(), U_UBO,
								SHADER_STAGE_FRAGMENT_BIT);
	graphics->bindVertexBuffer(cmdList, bTexCoord.get(), B_TEXCOORD,
							   sizeof(glm::vec2));
	D3D_TRACE(cmdList->mGraphicsCommandList->SetGraphicsRootDescriptorTable(
		U_TEXTURE, srcTexture->getSrvDescriptor(srcLevel, 1)->gpuHandle));
	D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	D3D_TRACE(cmdList->mGraphicsCommandList->SetGraphicsRootDescriptorTable(
		U_TEXTURE + 1, srcTexture->getSampler(filter)->handle->gpuHandle));
	graphics->draw(cmdList, 4);
}

void D3DBlitOp::apply(D3DGraphicsContext* ctx, D3DCommandList* cmdList,
					  D3DGraphics* graphics)
{
	ctx->beginOffscreenRenderPass(cmdList, graphics, outputFramebuffer.get());
	draw(cmdList, graphics);
	ctx->endOffscreenRenderPass(cmdList, graphics);
}

}