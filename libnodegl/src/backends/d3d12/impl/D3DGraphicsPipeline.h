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
#include <backends/d3d12/impl/D3DPipeline.h>
#include <backends/d3d12/impl/D3DShaderModule.h>

namespace ngli
{
/** \struct BlendParams
 *
 *  This struct defines blend parameters and operations
 */
struct BlendParams
{
	D3D12_BLEND srcColorBlendFactor = D3D12_BLEND_SRC_ALPHA;
	D3D12_BLEND dstColorBlendFactor = D3D12_BLEND_INV_SRC_ALPHA;
	D3D12_BLEND srcAlphaBlendFactor = D3D12_BLEND_SRC_ALPHA;
	D3D12_BLEND dstAlphaBlendFactor = D3D12_BLEND_INV_SRC_ALPHA;
	D3D12_BLEND_OP colorBlendOp = D3D12_BLEND_OP_ADD;
	D3D12_BLEND_OP alphaBlendOp = D3D12_BLEND_OP_ADD;
	size_t key();
};

/** \struct StencilParams
 *
 *  This struct defines stencil parameters and operations
 */
struct StencilParams
{
    uint8_t stencilReadMask = DEFAULT_STENCIL_READ_MASK;
    uint8_t stencilWriteMask = DEFAULT_STENCIL_WRITE_MASK;
    D3D12_STENCIL_OP frontStencilFailOp = D3D12_STENCIL_OP_KEEP;
    D3D12_STENCIL_OP frontStencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    D3D12_STENCIL_OP frontStencilPassOp = D3D12_STENCIL_OP_KEEP;
	D3D12_COMPARISON_FUNC frontStencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    D3D12_STENCIL_OP backStencilFailOp = D3D12_STENCIL_OP_KEEP;
    D3D12_STENCIL_OP backStencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    D3D12_STENCIL_OP backStencilPassOp = D3D12_STENCIL_OP_KEEP;
	D3D12_COMPARISON_FUNC backStencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    uint32_t stencilRef = 0;
    size_t key();
};

class D3DGraphicsContext;

class D3DGraphicsPipeline : public D3DPipeline
{
public:
	struct State
	{
		D3D12_PRIMITIVE_TOPOLOGY primitiveTopology =
			D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		D3D12_FILL_MODE fillMode = D3D12_FILL_MODE_SOLID;
		bool blendEnable = false;
		D3D12_BLEND blendSrcColorFactor = D3D12_BLEND_SRC_ALPHA;
		D3D12_BLEND blendDstColorFactor = D3D12_BLEND_INV_SRC_ALPHA;
		D3D12_BLEND_OP blendColorOp = D3D12_BLEND_OP_ADD;
		D3D12_BLEND blendSrcAlphaFactor = D3D12_BLEND_SRC_ALPHA;
		D3D12_BLEND blendDstAlphaFactor = D3D12_BLEND_INV_SRC_ALPHA;
		D3D12_BLEND_OP blendAlphaOp = D3D12_BLEND_OP_ADD;
		UINT8 colorWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
		FrontFace frontFace = FRONT_FACE_COUNTER_CLOCKWISE;
		float lineWidth = 1.0f;
		bool depthTestEnable = false, depthWriteEnable = false;
		D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;
		bool stencilEnable = false;
		UINT8 stencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		UINT8 stencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		D3D12_STENCIL_OP frontStencilFailOp = D3D12_STENCIL_OP_KEEP;
		D3D12_STENCIL_OP frontStencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		D3D12_STENCIL_OP frontStencilPassOp = D3D12_STENCIL_OP_KEEP;
		D3D12_COMPARISON_FUNC frontStencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		D3D12_STENCIL_OP backStencilFailOp = D3D12_STENCIL_OP_KEEP;
		D3D12_STENCIL_OP backStencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		D3D12_STENCIL_OP backStencilPassOp = D3D12_STENCIL_OP_KEEP;
		D3D12_COMPARISON_FUNC backStencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		uint32_t stencilRef = 0;
		uint32_t numSamples = 1, numColorAttachments = 1;
	};
	struct Shaders
	{
		D3D12_SHADER_BYTECODE VS{}, PS{}, DS{}, HS{}, GS{};
	};

  /** \struct State
   *
   *  The graphics pipeline state.  This provides default parameter values.
      The user can just override specific parameters instead of redefining all the parameters */
//    struct State
//    {
///** The input type (e.g. triangles, triangle list, lines, etc. )*/
//        PrimitiveTopology primitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//        /** The polygon drawing mode (e.g. fill or wireframe) */
//        PolygonMode polygonMode = D3D12_FILL_MODE_SOLID;
//        /** A helper function for setting the blend parameters for a given porter duff blend mode */
///*       inline void setBlendMode(BlendMode mode)
//        {
//            blendEnable = true;
//            blendParams = BlendUtil::getBlendParams(mode);
//        }*/
//        /** Enable / disable blending */
//        bool blendEnable = false;
//        /** The blend params */
//        BlendParams blendParams;
//        /** The color write mask (which color bits are written to the color buffer) */
//        uint8_t colorWriteMask = COLOR_COMPONENT_R_BIT | COLOR_COMPONENT_G_BIT |
//            COLOR_COMPONENT_B_BIT | COLOR_COMPONENT_A_BIT;
///** The cull mode (e.g. front, or backface culling )*/
//        D3D12_CULL_MODE cullModeFlags = D3D12_CULL_MODE_BACK;
//        FrontFace frontFace = FRONT_FACE_COUNTER_CLOCKWISE;
//        /** The line width */
//        float lineWidth = 1.0f;
//        /** Enable depth test */
//        bool depthTestEnable = false,
//        /** Enable depth write */
//            depthWriteEnable = false;
//        /** The depth comparision function that filters which values are written to the depth buffer */
//        D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;
//        /** Enable stencil test */
//        bool stencilEnable = false;
//        /** The stencil parameters */
//        StencilParams stencilParams;
//        RenderPass* renderPass = nullptr;
//        /** The number of samples used for multisampling.
//         *  When this value is one, no multisampling is used. */
//        uint32_t numSamples = 1,
//        /** The number of color attachments (supports multiple render targets) */
//            numColorAttachments = 1;
//        /** Create a unique key associated with the pipeline state */
//        size_t key();
//    };
    struct Descriptor
    {
        DescriptorType type;
        ShaderStageFlags stageFlags = SHADER_STAGE_ALL;
    };
    struct VertexInputAttributeDescription
    {
        D3DVertexShaderModule::AttributeDescription* v = nullptr;
        int offset = 0;
    };
    /** Create graphics pipeline */
    static D3DGraphicsPipeline*
		newInstance(D3DGraphicsContext* graphicsContext, const D3DGraphicsPipeline::State& state,
			   D3DVertexShaderModule* vs, D3DFragmentShaderModule* fs,
			   DXGI_FORMAT colorFormat, DXGI_FORMAT depthStencilFormat,
               std::vector<VertexInputAttributeDescription> vertexAttributes = {},
               std::set<std::string> instanceAttributes = {});
    virtual ~D3DGraphicsPipeline() {}
    /** Get the binding indices associated with the descriptors and vertex attributes.
    *   The user passes an array of pointers, one pointer for each descriptor and vertex attribute.
    *   The user should pass the pointers associated with the descriptors and vertex attributes in the same order
    *   that they're declared in the shader.
    *   @param pDescriptorBindings This return parameter contains the descriptor binding indices.
    *   @param pVertexAttribBindings This return parameter contains the vertex attribute binding indices. */
	void create(D3DGraphicsContext* ctx, const D3DGraphicsPipeline::State& state,
				const std::vector<CD3DX12_ROOT_PARAMETER1>& rootParameters,
				const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputElements,
				const Shaders& shaders, DXGI_FORMAT colorFormat,
				DXGI_FORMAT depthStencilFormat);

	  /** Get the binding indices associated with the descriptors and vertex attributes.
  *   The user passes an array of pointers, one pointer for each descriptor and vertex attribute.
  *   The user should pass the pointers associated with the descriptors and vertex attributes in the same order
  *   that they're declared in the shader.
  *   @param pDescriptorBindings This return parameter contains the descriptor binding indices.
  *   @param pVertexAttribBindings This return parameter contains the vertex attribute binding indices. */
	void getBindings(std::vector<uint32_t*> pDescriptorBindings,
					 std::vector<uint32_t*> pVertexAttribBindings);

	D3D_PRIMITIVE_TOPOLOGY d3dPrimitiveTopology;
	uint32_t d3dStencilRef = 0;

	D3D12_PRIMITIVE_TOPOLOGY_TYPE
		getPrimitiveTopologyType(D3D_PRIMITIVE_TOPOLOGY topology);


    std::vector<uint32_t> descriptorBindings;
    std::vector<uint32_t> vertexAttributeBindings;

};

}; // namespace ngli
