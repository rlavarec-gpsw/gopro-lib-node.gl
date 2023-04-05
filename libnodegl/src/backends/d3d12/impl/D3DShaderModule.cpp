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
#include "D3DShaderModule.h"

#include <d3dcompiler.h>
#include <memory>


namespace ngli
{

#define VF_ITEM(s, count, elementSize)                                         \
  {                                                                            \
#s, { s, count, elementSize }                                              \
  }
struct VertexFormatInfo
{
	VertexFormat format;
	uint32_t count, elementSize;
};
static const std::map<std::string, VertexFormatInfo> vertexFormatMap = {
	VF_ITEM(VERTEXFORMAT_FLOAT, 1, 4),
	VF_ITEM(VERTEXFORMAT_FLOAT2, 1, 8),
	VF_ITEM(VERTEXFORMAT_FLOAT3, 1, 12),
	VF_ITEM(VERTEXFORMAT_FLOAT4, 1, 16),
	{"VERTEXFORMAT_MAT4", {VERTEXFORMAT_FLOAT4, 4, 16}}
};

#define ITEM(s)                                                                \
  { #s, s }

static const std::map<std::string, VertexInputRate> vertexInputRateMap = {
	ITEM(VERTEX_INPUT_RATE_VERTEX),
	ITEM(VERTEX_INPUT_RATE_INSTANCE),
};

static const std::map<std::string, DescriptorType> descriptorTypeMap = {
	ITEM(DESCRIPTOR_TYPE_SAMPLER),
	ITEM(DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER),
	ITEM(DESCRIPTOR_TYPE_SAMPLED_IMAGE),
	ITEM(DESCRIPTOR_TYPE_STORAGE_IMAGE),
	ITEM(DESCRIPTOR_TYPE_UNIFORM_BUFFER),
	ITEM(DESCRIPTOR_TYPE_STORAGE_BUFFER)
};

static void parseAttributes(std::ifstream& in,
				std::vector<D3DVertexShaderModule::AttributeDescription>& attrs)
{
	std::string token;
	uint32_t numAttributes;
	in >> token >> numAttributes;
	attrs.resize(numAttributes);
	for(uint32_t j = 0; j < numAttributes; j++)
	{
		auto& attr = attrs[j];
		std::string formatStr;
		in >> attr.name >> attr.semantic >> attr.location >> formatStr;
		auto formatInfo = vertexFormatMap.at(formatStr);
		attr.format = formatInfo.format;
		attr.count = formatInfo.count;
		attr.elementSize = formatInfo.elementSize;
	}
}

static void parseDescriptors(std::ifstream& in,
							 std::vector<D3DShaderModule::DescriptorInfo>& descs)
{
	std::string token;
	int numDescriptors;
	in >> token >> numDescriptors;
	descs.resize(numDescriptors);
	for(uint32_t j = 0; j < uint32_t(numDescriptors); j++)
	{
		auto& desc = descs[j];
		std::string descriptorTypeStr;
		in >> desc.name >> descriptorTypeStr >> desc.set;
		desc.type = descriptorTypeMap.at(descriptorTypeStr);
	}
}

static void parseBufferMemberInfos(std::ifstream& in,
					   D3DShaderModule::BufferMemberInfos& memberInfos)
{
	uint32_t numMemberInfos;
	in >> numMemberInfos;
	for(uint32_t j = 0; j < numMemberInfos; j++)
	{
		D3DShaderModule::BufferMemberInfo memberInfo;
		std::string memberName;
		in >> memberName >> memberInfo.offset >> memberInfo.size >>
			memberInfo.arrayCount >> memberInfo.arrayStride;
		memberInfos[memberName] = memberInfo;
	}
}

static void parseBufferInfos(std::ifstream& in, std::string key,
							 D3DShaderModule::BufferInfos& bufferInfos,
							 ShaderStageFlags shaderStages)
{
	std::string token;
	in >> token;
	if(token != key)
		return;
	uint32_t numUniformBufferInfos;
	in >> numUniformBufferInfos;
	for(uint32_t j = 0; j < numUniformBufferInfos; j++)
	{
		D3DShaderModule::BufferInfo bufferInfo;
		in >> bufferInfo.name >> bufferInfo.set >> bufferInfo.readonly;
		bufferInfo.shaderStages = shaderStages;
		parseBufferMemberInfos(in, bufferInfo.memberInfos);
		bufferInfos[bufferInfo.name] = std::move(bufferInfo);
	}
}

void D3DShaderModule::initBindings(std::ifstream& in,
								ShaderStageFlags shaderStages)
{
	parseDescriptors(in, descriptors);
	parseBufferInfos(in, "UNIFORM_BUFFER_INFOS", uniformBufferInfos,
					 shaderStages);
	parseBufferInfos(in, "SHADER_STORAGE_BUFFER_INFOS", shaderStorageBufferInfos,
					 shaderStages);
}

void D3DShaderModule::initBindings(const std::string& filename,
								ShaderStageFlags shaderStages)
{
	std::ifstream in(filename);
	if(!in.is_open())
		NGLI_ERR("cannot open file: %s", filename.c_str());
	initBindings(in, shaderStages);
	in.close();
}

void D3DVertexShaderModule::initBindings(const std::string& filename)
{
	std::ifstream in(filename);
	if(!in.is_open())
		NGLI_ERR("cannot open file: %s", filename.c_str());
	parseAttributes(in, attributes);
	D3DShaderModule::initBindings(in, SHADER_STAGE_VERTEX_BIT);
	in.close();
}


bool D3DShaderModule::initFromFile(const std::string& filename)
{
	mFilename = filename;
#ifdef USE_PRECOMPILED_SHADERS
	File file;
	if(!file.read(filename + ".hlsl.dxc"))
	{
		NGLI_ERR("cannot read file: %s.hlsl.dxc", filename.c_str());
		return false;
	}
	initFromByteCode(file.data.get(), file.size);
#else
	compile(filename + ".hlsl");
#endif
	return true;
}

bool D3DShaderModule::compile(const std::string& filename)
{
	HRESULT hResult;
	UINT compileFlags = 0;
	if(DEBUG_SHADERS)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	ComPtr<ID3DBlob> byteCode;
	ComPtr<ID3DBlob> errorBlob;
	std::string target;
	if(strstr(filename.c_str(), "vert"))
		target = "vs_5_1";
	else if(strstr(filename.c_str(), "frag"))
		target = "ps_5_1";
	else if(strstr(filename.c_str(), "comp"))
		target = "cs_5_1";
	hResult = D3DCompileFromFile(StringUtil::toWString(filename).c_str(), nullptr,
								 nullptr, "main", target.c_str(), compileFlags, 0,
								 &byteCode, &errorBlob);
	if(FAILED(hResult))
	{
		NGLI_LOG("%s %s", (char*)errorBlob->GetBufferPointer(), filename.c_str());
		return false;
	}
	V0(hResult, "%s", filename.c_str());
	initFromByteCode(byteCode->GetBufferPointer(),
					 uint32_t(byteCode->GetBufferSize()));
	return true;
}

void D3DShaderModule::initFromByteCode(void* bytecodeData,
									   uint32_t bytecodeSize)
{
	d3dShaderByteCode.pShaderBytecode = malloc(bytecodeSize);
	d3dShaderByteCode.BytecodeLength = bytecodeSize;
	memcpy((void*)d3dShaderByteCode.pShaderBytecode, bytecodeData, bytecodeSize);
}

D3DShaderModule::~D3DShaderModule()
{
	if(d3dShaderByteCode.pShaderBytecode)
	{
		free((void*)d3dShaderByteCode.pShaderBytecode);
	}
}

template <typename T>
static std::unique_ptr<T> createShaderModule(D3DDevice* device,
											 const std::string& filename)
{
	auto d3dShaderModule = std::make_unique<T>();
	if(!d3dShaderModule->initFromFile(filename))
	{
		return nullptr;
	}
	d3dShaderModule->initBindings(filename + ".hlsl.map");
	return d3dShaderModule;
}

std::unique_ptr<D3DVertexShaderModule>
D3DVertexShaderModule::newInstance(D3DDevice* device, const std::string& filename)
{
	return createShaderModule<D3DVertexShaderModule>(device, filename);
}

std::unique_ptr<D3DFragmentShaderModule>
D3DFragmentShaderModule::newInstance(D3DDevice* device, const std::string& filename)
{
	return createShaderModule<D3DFragmentShaderModule>(device, filename);
}

std::unique_ptr<D3DComputeShaderModule>
D3DComputeShaderModule::newInstance(D3DDevice* device, const std::string& filename)
{
	return createShaderModule<D3DComputeShaderModule>(device, filename);
}

}
