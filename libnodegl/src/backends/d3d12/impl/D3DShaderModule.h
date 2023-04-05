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


namespace ngli
{
/** \class ShaderModule
 *
 *   This class provides support for shader modules
 *   including vertex, fragment and compute shaders.
 *   It can optionally provide shader reflection info,
 *   provided via the ShaderTools module.
 *   Shaders can be precompiled offline, or compiled at runtime.
 *   For best performance it's best to precompile them offline.
 */
class D3DDevice;

class D3DShaderModule
{

public:
	/** Information associated with a descriptor.
	 *  A descriptor is a resource that's bound to the shader.
	 */
	struct DescriptorInfo
	{
/** The descriptor name */
		std::string name;
		/** The set layout index */
		uint32_t set;
		/** The descriptor type */
		DescriptorType type;
	};
	typedef std::vector<DescriptorInfo> DescriptorInfos;
	DescriptorInfos descriptors;
	/** Find descriptor info by name */
	inline DescriptorInfo* findDescriptorInfo(const std::string& name)
	{
		for(auto& desc : descriptors)
		{
			if(desc.name == name)
				return &desc;
		}
		return nullptr;
	}
	/** Information about a member variable of a buffer.
	 *  The buffer layout can be defined using a struct.
	 */
	struct BufferMemberInfo
	{
/** The offset of the member variable (in bytes) */
		uint32_t offset,
		/** The size of the member variable (in bytes) */
			size,
		/** The array count, if the member variable is an array */
			arrayCount,
		/** The offset between elements in the array (if the member variable is an array) */
			arrayStride;
	};
	typedef std::map<std::string, BufferMemberInfo> BufferMemberInfos;
	/** Information about a buffer */
	struct BufferInfo
	{
/** The buffer name */
		std::string name;
		/** The set layout index */
		uint32_t set;
		/** If true, this buffer is bound as readonly, else it is bound as read/write */
		bool readonly;
		/** The shader stages that have access to the buffer */
		ShaderStageFlags shaderStages;
		/** The information of the buffer member variables */
		BufferMemberInfos memberInfos;
	};
	typedef std::map<std::string, BufferInfo> BufferInfos;
	inline BufferInfo* findUniformBufferInfo(const std::string& name)
	{
		auto it = uniformBufferInfos.find(name);
		if(it == uniformBufferInfos.end())
			return nullptr;
		return &it->second;
	}
	inline BufferInfo* findStorageBufferInfo(const std::string& name)
	{
		auto it = shaderStorageBufferInfos.find(name);
		if(it == shaderStorageBufferInfos.end())
			return nullptr;
		return &it->second;
	}
	BufferInfos uniformBufferInfos, shaderStorageBufferInfos;

	void initBindings(std::ifstream& in, ShaderStageFlags shaderStages);
	void initBindings(const std::string& filename, ShaderStageFlags shaderStages);

	bool initFromFile(const std::string& filename);
	virtual ~D3DShaderModule();
	D3D12_SHADER_BYTECODE d3dShaderByteCode{};
	void initFromByteCode(void* bytecodeData, uint32_t bytecodeSize);
	bool compile(const std::string& filename);

public:
	// informative filename source for the shader
	std::string mFilename;
};

/** \class VertexShaderModule
 *  This class supports vertex shader modules
 */
class D3DVertexShaderModule : public D3DShaderModule
{
public:
	/** Create vertex shader module from file
		@param filename: This is the filename prefix for the files associated with this
		shader module.  When precompiled shaders is enabled as a configuration option,
		this will load the precompiled shader along with any corresponding reflection info
		if available. Otherwise, this will load the shader code and compile it at runtime */
	static std::unique_ptr<D3DVertexShaderModule> newInstance(D3DDevice* device, const std::string& filename);

	/** Describes a vertex shader attribute */
	struct AttributeDescription
	{
/** The semantic keyword describing the attribute.  Some backends like DirectX 12 provide
	predefined semantics like POSITION, TEXCOORD for describing the attribute */
		std::string semantic;
		/** The layout location index */
		uint32_t location;
		/** The attribute format */
		VertexFormat format;
		/** The attribute name */
		std::string name;
		/** The attribute count */
		uint32_t count;
		/** The size of each element (bytes) */
		uint32_t elementSize;
	};
	std::vector<AttributeDescription> attributes;
	/** Find attribute by name */
	inline AttributeDescription* findAttribute(const std::string& name)
	{
		for(auto& attr : attributes)
		{
			if(attr.name == name)
				return &attr;
		}
		return nullptr;
	}
	void initBindings(const std::string& filename);

	virtual ~D3DVertexShaderModule() {}
};

class D3DFragmentShaderModule : public D3DShaderModule
{
public:
	static std::unique_ptr<D3DFragmentShaderModule> newInstance(D3DDevice* device, const std::string& filename);

	void initBindings(const std::string& filename)
	{
		D3DShaderModule::initBindings(filename, SHADER_STAGE_FRAGMENT_BIT);
	}
	virtual ~D3DFragmentShaderModule() {}
};

class D3DComputeShaderModule : public D3DShaderModule
{
public:
	static std::unique_ptr<D3DComputeShaderModule> newInstance(D3DDevice* device, const std::string& filename);
	virtual ~D3DComputeShaderModule() {}
	void initBindings(const std::string& filename)
	{
		D3DShaderModule::initBindings(filename, SHADER_STAGE_COMPUTE_BIT);
	}
};

} // namespace ngli