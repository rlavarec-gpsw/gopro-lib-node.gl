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


extern "C" {
#include "log.h"
#include "memory.h"
#include "program.h"
}

#include "shader_tools_mtl.h"

#include <fstream>
#include <iostream>

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include "glslang/Include/ShHandle.h"
#include "glslang/SPIRV/GlslangToSpv.h"
#include "glslang/SPIRV/GLSL.std.450.h"

bool ShaderToolsMSL::glsl_initialized = false;

void ShaderToolsMSL::initialize() {
    ShaderToolsMSL::mutex.lock();
    if (!ShaderToolsMSL::glsl_initialized) {
        glslang::InitializeProcess();
        ShaderToolsMSL::glsl_initialized = true;
    }
    ShaderToolsMSL::mutex.unlock();
}

void ShaderToolsMSL::finalize() {
    ShaderToolsMSL::mutex.lock();
    if (ShaderToolsMSL::glsl_initialized) {
        glslang::FinalizeProcess();
        ShaderToolsMSL::glsl_initialized = false;
    }
    ShaderToolsMSL::mutex.unlock();
}

bool ShaderToolsMSL::compileGLSLToSpirv(int stage, const std::string &glsl_data,
                                        const std::string &out_filename ) {
    const struct {
        int stage;
        EShLanguage lang;
    } stages[] = {
        { NGLI_PROGRAM_SHADER_VERT, EShLangVertex },
        { NGLI_PROGRAM_SHADER_FRAG, EShLangFragment },
        { NGLI_PROGRAM_SHADER_COMP, EShLangCompute },
    };

    glslang::TProgram &proram = *new glslang::TProgram;
    glslang::TShader *shader = new glslang::TShader(stages[stage].lang);
    shader->setStrings(&(glsl_data.c_str()), 1);
    shader->setOverrideVersion(0);
    shader->setEnvInput(glslang::EShSourceGlsl, stages[stage].lang, glslang::EShClientOpenGL);
    shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
    shader->setEnvTarget(glslang::EShTargetSPV, glslang::EShTargetSPV_1_3);
    glslang::TShader::Includer includer;
    if (!shader->preprocess(glslang::GetDefaultResources(), 450, glslang::ENoProfile,
                        false, false, glslang::EShMsgDefault, &glsl_data, includer)) {
        LOG(ERROR, "unable to preprocess glsl: \n%s", shader->getInfoLog());
        delete &program;
        delete shader;
        return false;
    }

    if (!shader->parse(glslang::GetDefaultResources(), 450, false, glslang::EShMsgDefault, 
                       includir)) {
        LOG(ERROR, "unable to parse glsl: \n%s", shader->getInfoLog());
        delete &program;
        delete shader;
        return false;
    }

    program.addShader(shader);
    if (!program.link(glslang::EShMsgDefault)) {
        LOG(ERROR, "unable to link shader: \n%s", program.getInfoLog());
        delete &program;
        delete shader;
        return false;
    }

    if (program.getIntermediate(stages[stage].lang)) { 
        // compile to spirv
        SpvBuildLogger logger;
        glslang::SpvOptions spv_options;
        spvOptions.validate = true;
        std::vector<unsigned int> spirv;
        glslang::GlslangToSpv(*program.getIntermediate(stages[stage], spirv,
                              &logger, &spv_options);
        glslang::OutputSpvBin(spirv, out_filename.c_str());
    } else {
        LOG(ERROR, "unable to get program intermediate");
        delete &program;
        delete shader;
        return false;
    }

    delete *program;
    delete shader;
    return true;
}
