/*
 * Copyright 2019 GoPro Inc.
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
#include "util_d3d12.h"
#include "format_d3d12.h"

#include <backends/d3d12/impl/D3DGraphicsCore.h>
#include <backends/d3d12/impl/D3DGraphicsContext.h>

extern "C" {
#include "graphicstate.h"
#include "texture.h"
}

ngli::FilterMode to_d3d12_filter_mode(int filter)
{
	static const std::map<int, ngli::FilterMode> filter_map = {
		{NGLI_FILTER_NEAREST, ngli::FILTER_NEAREST},
		{NGLI_FILTER_LINEAR,  ngli::FILTER_LINEAR }
	};
	return filter_map.at(filter);
}

ngli::FilterMode to_d3d12_mip_filter_mode(int filter)
{
	static const std::map<int, ngli::FilterMode> mip_filter_map = {
		{NGLI_MIPMAP_FILTER_NEAREST, ngli::FILTER_NEAREST},
		{NGLI_MIPMAP_FILTER_LINEAR,  ngli::FILTER_LINEAR }
	};
	return mip_filter_map.at(filter);
}

ngli::TextureType to_d3d12_texture_type(int type)
{
	static const std::map<int, ngli::TextureType> texture_type_map = {
		{NGLI_TEXTURE_TYPE_2D,   ngli::TEXTURE_TYPE_2D  },
		{NGLI_TEXTURE_TYPE_3D,   ngli::TEXTURE_TYPE_3D  },
		{NGLI_TEXTURE_TYPE_CUBE, ngli::TEXTURE_TYPE_CUBE}
	};
	return texture_type_map.at(type);
}

ngli::IndexFormat to_d3d12_index_format(int indices_format)
{
	static const std::map<int, ngli::IndexFormat> index_format_map = {
		{NGLI_FORMAT_R16_UNORM, ngli::INDEXFORMAT_UINT16},
		{NGLI_FORMAT_R32_UINT,  ngli::INDEXFORMAT_UINT32}
	};
	return index_format_map.at(indices_format);
}

D3D12_BLEND to_d3d12_blend_factor(int blend_factor)
{
	static const std::map<int, D3D12_BLEND> blend_factor_map = {
		{NGLI_BLEND_FACTOR_ZERO,					 D3D12_BLEND_ZERO			},
		{NGLI_BLEND_FACTOR_ONE,						 D3D12_BLEND_ONE			},
		{NGLI_BLEND_FACTOR_SRC_COLOR,				 D3D12_BLEND_SRC_COLOR		},
		{NGLI_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,		 D3D12_BLEND_INV_SRC_COLOR	},
		{NGLI_BLEND_FACTOR_DST_COLOR,				 D3D12_BLEND_DEST_COLOR		},
		{NGLI_BLEND_FACTOR_ONE_MINUS_DST_COLOR,		 D3D12_BLEND_INV_DEST_COLOR	},
		{NGLI_BLEND_FACTOR_SRC_ALPHA,				 D3D12_BLEND_SRC_ALPHA		},
		{NGLI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,		 D3D12_BLEND_INV_SRC_ALPHA	},
		{NGLI_BLEND_FACTOR_DST_ALPHA,				 D3D12_BLEND_DEST_ALPHA		},
		{NGLI_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,		 D3D12_BLEND_INV_DEST_ALPHA },
//		{NGLI_BLEND_FACTOR_CONSTANT_COLOR,			 D3D12_BLEND_BLEND_FACTOR   },
//		{NGLI_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR, D3D12_BLEND_INV_BLEND_FACTOR  }
	};

	return blend_factor_map.at(blend_factor);
}

D3D12_COMPARISON_FUNC to_d3d12_compare_op(int compare_op)
{
	static const std::map<int, D3D12_COMPARISON_FUNC> compare_op_map = {
		{NGLI_COMPARE_OP_NEVER,            D3D12_COMPARISON_FUNC_NEVER        },
		{NGLI_COMPARE_OP_LESS,             D3D12_COMPARISON_FUNC_LESS         },
		{NGLI_COMPARE_OP_EQUAL,            D3D12_COMPARISON_FUNC_EQUAL        },
		{NGLI_COMPARE_OP_LESS_OR_EQUAL,    D3D12_COMPARISON_FUNC_LESS_EQUAL   },
		{NGLI_COMPARE_OP_GREATER,          D3D12_COMPARISON_FUNC_GREATER      },
		{NGLI_COMPARE_OP_NOT_EQUAL,        D3D12_COMPARISON_FUNC_NOT_EQUAL    },
		{NGLI_COMPARE_OP_GREATER_OR_EQUAL, D3D12_COMPARISON_FUNC_GREATER_EQUAL},
		{NGLI_COMPARE_OP_ALWAYS,           D3D12_COMPARISON_FUNC_ALWAYS       }
	};

	return compare_op_map.at(compare_op);
}

D3D12_STENCIL_OP to_d3d12_stencil_op(int stencil_op)
{
	static const std::map<int, D3D12_STENCIL_OP> stencil_op_map = {
		{NGLI_STENCIL_OP_KEEP,                D3D12_STENCIL_OP_KEEP    },
		{NGLI_STENCIL_OP_ZERO,                D3D12_STENCIL_OP_ZERO    },
		{NGLI_STENCIL_OP_REPLACE,             D3D12_STENCIL_OP_REPLACE },
		{NGLI_STENCIL_OP_INCREMENT_AND_CLAMP, D3D12_STENCIL_OP_INCR_SAT},
		{NGLI_STENCIL_OP_DECREMENT_AND_CLAMP, D3D12_STENCIL_OP_DECR_SAT},
		{NGLI_STENCIL_OP_INVERT,              D3D12_STENCIL_OP_INVERT  },
		{NGLI_STENCIL_OP_INCREMENT_AND_WRAP,  D3D12_STENCIL_OP_INCR    },
		{NGLI_STENCIL_OP_DECREMENT_AND_WRAP,  D3D12_STENCIL_OP_DECR    }
	};

	return stencil_op_map.at(stencil_op);
}

D3D12_BLEND_OP to_d3d12_blend_op(int blend_op)
{
	static const std::map<int, D3D12_BLEND_OP> blend_op_map = {
		{NGLI_BLEND_OP_ADD,              D3D12_BLEND_OP_ADD             },
		{NGLI_BLEND_OP_SUBTRACT,         D3D12_BLEND_OP_SUBTRACT        },
		{NGLI_BLEND_OP_REVERSE_SUBTRACT, D3D12_BLEND_OP_REV_SUBTRACT	},
		{NGLI_BLEND_OP_MIN,              D3D12_BLEND_OP_MIN             },
		{NGLI_BLEND_OP_MAX,              D3D12_BLEND_OP_MAX             },
	};
	return blend_op_map.at(blend_op);
}

ngli::ColorComponentFlags to_d3d12_color_mask(int color_write_mask)
{
	return (color_write_mask & NGLI_COLOR_COMPONENT_R_BIT
				? D3D12_COLOR_WRITE_ENABLE_RED
				: 0) |
		(color_write_mask & NGLI_COLOR_COMPONENT_G_BIT
			 ? D3D12_COLOR_WRITE_ENABLE_GREEN
			 : 0) |
		(color_write_mask & NGLI_COLOR_COMPONENT_B_BIT
			 ? D3D12_COLOR_WRITE_ENABLE_BLUE
			 : 0) |
		(color_write_mask & NGLI_COLOR_COMPONENT_A_BIT
			 ? D3D12_COLOR_WRITE_ENABLE_ALPHA
			 : 0);
}

D3D12_CULL_MODE to_d3d12_cull_mode(int cull_mode)
{
	static std::map<int, D3D12_CULL_MODE> cull_mode_map = {
		{NGLI_CULL_MODE_NONE,      D3D12_CULL_MODE_NONE     },
		{NGLI_CULL_MODE_FRONT_BIT, D3D12_CULL_MODE_FRONT},
		{NGLI_CULL_MODE_BACK_BIT,  D3D12_CULL_MODE_BACK },
	};
	return cull_mode_map.at(cull_mode);
}

ngli::ImageUsageFlags to_d3d12_image_usage_flags(int usage_flags)
{
	static std::map<int, ngli::ImageUsageFlags> usage_flags_map = {
		{NGLI_TEXTURE_USAGE_TRANSFER_SRC_BIT,             ngli::IMAGE_USAGE_TRANSFER_SRC_BIT },
		{NGLI_TEXTURE_USAGE_TRANSFER_DST_BIT,             ngli::IMAGE_USAGE_TRANSFER_DST_BIT },
		{NGLI_TEXTURE_USAGE_SAMPLED_BIT,                  ngli::IMAGE_USAGE_SAMPLED_BIT      },
		{NGLI_TEXTURE_USAGE_STORAGE_BIT,                  ngli::IMAGE_USAGE_STORAGE_BIT      },
		{NGLI_TEXTURE_USAGE_COLOR_ATTACHMENT_BIT,		  ngli::IMAGE_USAGE_COLOR_ATTACHMENT_BIT },
		{NGLI_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, ngli::IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT },
		{NGLI_TEXTURE_USAGE_TRANSIENT_ATTACHMENT_BIT,     ngli::IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT },
	};
	int image_usage_flags = 0;
	for(auto& flag : usage_flags_map)
	{
		if(usage_flags & flag.first)
			image_usage_flags |= flag.second;
	}
	return image_usage_flags;
}

ngli::AttachmentLoadOp to_d3d12_load_op(int op)
{
	static std::map<int, ngli::AttachmentLoadOp> load_op_map = {
		{NGLI_LOAD_OP_LOAD,      ngli::ATTACHMENT_LOAD_OP_LOAD     },
		{NGLI_LOAD_OP_CLEAR,     ngli::ATTACHMENT_LOAD_OP_CLEAR    },
		{NGLI_LOAD_OP_DONT_CARE, ngli::ATTACHMENT_LOAD_OP_DONT_CARE}
	};
	return load_op_map.at(op);
}

ngli::AttachmentStoreOp to_d3d12_store_op(int op)
{
	static std::map<int, ngli::AttachmentStoreOp> store_op_map = {
		{NGLI_STORE_OP_DONT_CARE, ngli::ATTACHMENT_STORE_OP_DONT_CARE},
		{NGLI_STORE_OP_STORE,     ngli::ATTACHMENT_STORE_OP_STORE    }
	};
	return store_op_map.at(op);
}

ngli::D3DRenderPass* get_render_pass(ngli::D3DGraphicsContext* ctx,
							const struct rendertarget_params* params)
{
	bool hasDepthStencilAttachment =
		params->depth_stencil.attachment != nullptr;
	ngli::D3DGraphicsContext::RenderPassConfig rp_config;
	auto& colorAttachmentDescs = rp_config.colorAttachmentDescriptions;
	colorAttachmentDescs.resize(params->nb_colors);
	for(int j = 0; j < params->nb_colors; j++)
	{
		auto& v0 = params->colors[j];
		auto& v1 = colorAttachmentDescs[j];
		v1.format = to_d3d12_format(v0.attachment->params.format);
		v1.loadOp = to_d3d12_load_op(v0.load_op);
		v1.storeOp = to_d3d12_store_op(v0.store_op);
	}
	if(hasDepthStencilAttachment)
		rp_config.depthStencilAttachmentDescription = {
			to_d3d12_format(params->depth_stencil.attachment->params.format),
			std::nullopt, std::nullopt, to_d3d12_load_op(params->depth_stencil.load_op),
			to_d3d12_store_op(params->depth_stencil.store_op) };
	else
		rp_config.depthStencilAttachmentDescription = std::nullopt;
	rp_config.enableDepthStencilResolve =
		params->depth_stencil.resolve_target != nullptr;
	rp_config.numSamples =
		glm::max(params->colors[0].attachment->params.samples, 1);
	return ctx->getRenderPass(rp_config);
}

ngli::D3DRenderPass* get_compat_render_pass(ngli::D3DGraphicsContext* ctx,
										 const struct rendertarget_desc& desc)
{
	bool hasDepthStencilAttachment =
		desc.depth_stencil.format != NGLI_FORMAT_UNDEFINED;
	ngli::D3DGraphicsContext::RenderPassConfig rp_config;
	auto& colorAttachmentDescs = rp_config.colorAttachmentDescriptions;
	colorAttachmentDescs.resize(desc.nb_colors);
	for(int j = 0; j < desc.nb_colors; j++)
	{
		auto& v0 = desc.colors[j];
		auto& v1 = colorAttachmentDescs[j];
		v1.format = to_d3d12_format(v0.format);
		v1.loadOp = ngli::ATTACHMENT_LOAD_OP_DONT_CARE;
		v1.storeOp = ngli::ATTACHMENT_STORE_OP_DONT_CARE;
	}
	if(hasDepthStencilAttachment)
		rp_config.depthStencilAttachmentDescription = {
			to_d3d12_format(desc.depth_stencil.format), std::nullopt, std::nullopt,
			ngli::ATTACHMENT_LOAD_OP_DONT_CARE, ngli::ATTACHMENT_STORE_OP_DONT_CARE };
	else
		rp_config.depthStencilAttachmentDescription = std::nullopt;
	rp_config.enableDepthStencilResolve = desc.colors[0].resolve;
	rp_config.numSamples = glm::max(desc.samples, 1);
	return ctx->getRenderPass(rp_config);
}
