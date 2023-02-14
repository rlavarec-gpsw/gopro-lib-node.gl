/*
 * Copyright 2023 GoPro Inc.
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

#include <stdlib.h>

#include <cassert>
#include <fstream>
#include <iostream>

extern "C" {
#include "log.h"
#include "memory.h"
#include "program.h"
}

#include "shader_tools_mtl.h"

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Include/ShHandle.h>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/GLSL.std.450.h>

#include <spirv_cross/spirv_msl.hpp>

bool ShaderToolsMSL::glsl_initialized = false;

void ShaderToolsMSL::initialize() {
    ShaderToolsMSL::glslang_mutex.lock();
    if (!ShaderToolsMSL::glsl_initialized) {
        glslang::InitializeProcess();
        ShaderToolsMSL::glsl_initialized = true;
    }
    ShaderToolsMSL::glslang_mutex.unlock();
}

void ShaderToolsMSL::finalize() {
    ShaderToolsMSL::glslang_mutex.lock();
    if (ShaderToolsMSL::glsl_initialized) {
        glslang::FinalizeProcess();
        ShaderToolsMSL::glsl_initialized = false;
    }
    ShaderToolsMSL::glslang_mutex.unlock();
}

bool ShaderToolsMSL::compileGLSLToSpirv(int stage, const std::string &glsl_data,
                                        std::vector<uint32_t> &spv_data ) {
    const struct {
        int stage;
        EShLanguage lang;
    } stages[] = {
        { NGLI_PROGRAM_SHADER_VERT, EShLangVertex },
        { NGLI_PROGRAM_SHADER_FRAG, EShLangFragment },
        { NGLI_PROGRAM_SHADER_COMP, EShLangCompute },
    };


    auto program = std::make_unique<glslang::TProgram>();
    auto shader = std::make_unique<glslang::TShader>(stages[stage].lang);
    const char *data = glsl_data.c_str();
    shader->setStrings(&data, 1);
    shader->setOverrideVersion(0);
    shader->setEnvInput(glslang::EShSourceGlsl, stages[stage].lang, glslang::EShClientOpenGL, 450);
    shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
    shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
    /*glslang::TShader::Includer includer;
    if (!shader->preprocess(GetDefaultResources(), 450, ENoProfile,
                        false, false, EShMsgDefault, &glsl_data, includer)) {
        LOG(ERROR, "unable to preprocess glsl: \n%s", shader->getInfoLog());
        return false;
    }*/

    if (!shader->parse(GetDefaultResources(), 450, false, EShMsgDefault)) { 
        LOG(ERROR, "unable to parse glsl: \n%s", shader->getInfoLog());
        return false;
    }

    program->addShader(shader.get());
    if (!program->link(EShMsgDefault)) {
        LOG(ERROR, "unable to link shader: \n%s", program->getInfoLog());
        return false;
    }

    if (program->getIntermediate(stages[stage].lang)) { 
        // compile to spirv
        spv::SpvBuildLogger logger;
        glslang::SpvOptions spv_options;
        spv_options.validate = true;
        spv_data.clear();
        glslang::GlslangToSpv(*program->getIntermediate(stages[stage].lang), spv_data,
                              &logger, &spv_options);

    } else {
        LOG(ERROR, "unable to get program intermediate");
        return false;
    }

    return true;
}

bool ShaderToolsMSL::convertSpirvToMSL(const std::vector<uint32_t> &spv_data, std::string &msl_data) {
    auto compiler = std::make_unique<spirv_cross::CompilerMSL>(spv_data);
    msl_data.clear();
    msl_data = compiler->compile();
    return msl_data.size() != 0 ? true : false;
}

void ShaderToolsMSL::writeToFile(const std::string &filename, std::string &data) {
    ShaderToolsMSL::file_mutex.lock();
    std::ofstream out(filename);
    assert(out);
    out.write(data.data(), data.size());
    out.close();
    ShaderToolsMSL::file_mutex.unlock();
}

static int cmd(std::string str) {
    str += " >> /dev/null 2>&1";
    return system(str.c_str());
}

bool ShaderToolsMSL::compileMSL(const std::string &filename) {
    int result = cmd("xcrun -sdk macosx metal -c " + filename + " -o " +
                     filename + ".air && xcrun -sdk macos metallib " + 
                     filename + ".air -o " + filename + ".metallib"); 
    return result == 0 ? true : false;
}

