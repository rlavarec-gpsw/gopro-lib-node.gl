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

#include <backends/common/FileUtil.h>
#include <d3dcompiler.h>
#include <dxcapi.h>


using Microsoft::WRL::ComPtr;

namespace ngli
{

#define VF_ITEM(s, count, elementSize)                                         \
  {                                                                            \
#s, { s, count, elementSize }                                              \
  }

struct VertexFormatInfo
{
	VertexFormat format;
	uint32_t count = 0;
	uint32_t elementSize=0;
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
	{
		NGLI_ERR("Binding in map not found: %s", key.c_str());
		return;
	}
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
	std::filesystem::path tFilename = FileUtil::getAbsolutePath(filename);
	std::ifstream in(tFilename.string());
	if(!in.is_open())
		NGLI_ERR("cannot open file: %s", tFilename.string().c_str());
	initBindings(in, shaderStages);
	in.close();
}

void D3DVertexShaderModule::initBindings(const std::string& filename)
{
	std::filesystem::path tFilename = FileUtil::getAbsolutePath(filename);
	std::ifstream in(tFilename.string());
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
	return true;
#else
	return compile(filename + ".hlsl");
#endif
}

//#define COMPILE_5_1_SHADER
bool D3DShaderModule::compile(const std::string& filename)
{
 #ifdef COMPILE_5_1_SHADER
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

	std::filesystem::path tFilename = FileUtil::getAbsolutePath(filename);

	hResult = D3DCompileFromFile(tFilename.wstring().c_str(), nullptr,
								 nullptr, "main", target.c_str(), compileFlags, 0,
								 &byteCode, &errorBlob);
	if(FAILED(hResult))
	{
		if(errorBlob)
			LOG(INFO, "%s %s", (char*)errorBlob->GetBufferPointer(), tFilename.c_str());
		else
			LOG(INFO, "%s", tFilename.c_str());
		return false;
	}
	D3D_TRACE_ARG(hResult, "%s", filename.c_str());
#else
	std::filesystem::path tPathFilename = FileUtil::getAbsolutePath(filename);
	std::wstring tFilename = tPathFilename.wstring();

	ComPtr<IDxcLibrary> library;
	HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
	if(FAILED(hr))
	{
		LOG(ERROR, "DxcCreateInstance CLSID_DxcLibrary: %s", tFilename.c_str());
		return false;
	}

	ComPtr<IDxcCompiler> compiler;
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
	if(FAILED(hr))
	{
		LOG(ERROR, "DxcCreateInstance CLSID_DxcCompiler: %s", tFilename.c_str());
		return false;
	}

	uint32_t codePage = CP_UTF8;
	ComPtr<IDxcBlobEncoding> sourceBlob;
	hr = library->CreateBlobFromFile(tFilename.c_str(), &codePage, &sourceBlob);
	if(FAILED(hr))
	{
		LOG(ERROR, "CreateBlobFromFile: %s", tFilename.c_str());
		return false;
	}

	std::wstring target;

	if(strstr(filename.c_str(), "vert"))
		target = L"vs_6_0";
	else if(strstr(filename.c_str(), "frag"))
		target = L"ps_6_0";
	else if(strstr(filename.c_str(), "comp"))
		target = L"cs_6_0";

	ComPtr<IDxcOperationResult> result;
	hr = compiler->Compile(
		sourceBlob.Get(), // pSource
		tFilename.c_str(), // pSourceName
		L"main", // pEntryPoint
		target.c_str(), // pTargetProfile
		NULL, 0, // pArguments, argCount
		NULL, 0, // pDefines, defineCount
		NULL, // pIncludeHandler
		&result); // ppResult
	if(SUCCEEDED(hr))
		result->GetStatus(&hr);
	if(FAILED(hr))
	{
		if(result)
		{
			ComPtr<IDxcBlobEncoding> errorsBlob;
			hr = result->GetErrorBuffer(&errorsBlob);
			if(SUCCEEDED(hr))
			{
				if(errorsBlob)
					LOG(INFO, "Compilation failed with errors: %s %s", (char*)errorsBlob->GetBufferPointer(), tFilename.c_str());
				else
					LOG(INFO, "Compilation failed with errors: %s", tFilename.c_str());
			}
		}
		// Handle compilation error...
	}
	ComPtr<IDxcBlob> byteCode;
	result->GetResult(&byteCode);

#endif
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
