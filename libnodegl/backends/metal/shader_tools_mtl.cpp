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
#include "SPIRV/GlslangToSpv.h"
#include "SPIRV/GLSL.std.450.h"

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
                                        std::string &spv_data ) {
    const struct {
        int stage;
        EShLanguage lang;
    } stages[] = {
        { NGLI_PROGRAM_SHADER_VERT, EShLangVertex },
        { NGLI_PROGRAM_SHADER_FRAG, EShLangFragment },
        { NGLI_PROGRAM_SHADER_COMP, EShLangCompute },
    };


    glslang::TProgram &program = *new glslang::TProgram;
    glslang::TShader *shader = new glslang::TShader(stages[stage].lang);
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
        delete &program;
        delete shader;
        return false;
    }*/

    if (!shader->parse(GetDefaultResources(), 450, false, EShMsgDefault)) { 
        LOG(ERROR, "unable to parse glsl: \n%s", shader->getInfoLog());
        delete &program;
        delete shader;
        return false;
    }

    program.addShader(shader);
    if (!program.link(EShMsgDefault)) {
        LOG(ERROR, "unable to link shader: \n%s", program.getInfoLog());
        delete &program;
        delete shader;
        return false;
    }

    if (program.getIntermediate(stages[stage].lang)) { 
        // compile to spirv
        spv::SpvBuildLogger logger;
        glslang::SpvOptions spv_options;
        spv_options.validate = true;
        std::vector<unsigned int> spirv;
        glslang::GlslangToSpv(*program.getIntermediate(stages[stage].lang), spirv,
                              &logger, &spv_options);
        for (auto word : spirv) {
            char *s = (char *)&word;
            for (int i = 0; i < 4; i++) {
                spv_data.push_back(s[i]);
            }
        }
        //glslang::OutputSpvBin(spirv, out_filename.c_str());
    } else {
        LOG(ERROR, "unable to get program intermediate");
        delete &program;
        delete shader;
        return false;
    }

    delete &program;
    delete shader;
    return true;
}
