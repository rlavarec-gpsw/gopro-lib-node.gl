
#include <stdafx.h>
#include "D3DSamplerDesc.h"


namespace ngli
{

static const std::vector<D3D12_FILTER> filterMap = {
	D3D12_FILTER_MIN_MAG_MIP_POINT,
	D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR,
	D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
	D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR,
	D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT,
	D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
	D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
	D3D12_FILTER_MIN_MAG_MIP_LINEAR
};

D3DSamplerDesc::D3DSamplerDesc(const texture_params* texture_params)
{
	if(texture_params)
	{
		uint32_t filterIndex = texture_params->min_filter << 2 | texture_params->mag_filter << 1 | texture_params->mipmap_filter;
		Filter = filterMap.at(filterIndex);
		AddressU = D3D12_TEXTURE_ADDRESS_MODE(texture_params->wrap_s);
		AddressV = D3D12_TEXTURE_ADDRESS_MODE(texture_params->wrap_t);
		AddressW = D3D12_TEXTURE_ADDRESS_MODE(texture_params->wrap_r);
	}
	else
	{
		Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	}
	MipLODBias = 0;
	MaxAnisotropy = D3D12_MAX_MAXANISOTROPY;
	ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	MinLOD = 0.0f;
	MaxLOD = D3D12_FLOAT32_MAX;
}
}
